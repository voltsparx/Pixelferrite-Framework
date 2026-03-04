#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build-linux"
PFF_BIN="${BUILD_DIR}/bin/pffconsole"
PIXELGEN_BIN="${BUILD_DIR}/bin/pixelgen"

SYSTEM_ROOT="/usr/local/lib/pixelferrite"
SYSTEM_LINK="/usr/local/bin/pixelferrite"
USER_ROOT="${HOME}/.local/lib/pixelferrite"
USER_LINK="${HOME}/.local/bin/pixelferrite"

MODE="install"
TARGET="system"
ACTION="install"
AUTO_YES=0
SUDO=""

INSTALL_ROOT=""
GLOBAL_LINK=""

EXISTING_ROOT=""
EXISTING_LINK=""

GREEN="\033[92m"
CYAN="\033[96m"
YELLOW="\033[93m"
RED="\033[91m"
RESET="\033[0m"

fail() {
  echo -e "${RED}Error: $*${RESET}" >&2
  exit 1
}

info() {
  echo -e "${CYAN}==> $*${RESET}"
}

warn() {
  echo -e "${YELLOW}Warning: $*${RESET}"
}

ok() {
  echo -e "${GREEN}$*${RESET}"
}

run_cmd() {
  if ! "$@"; then
    fail "Command failed: $*"
  fi
}

dir_has_entries() {
  local dir="$1"
  [[ -d "${dir}" ]] || return 1
  local first=""
  first="$(find "${dir}" -mindepth 1 -print -quit 2>/dev/null || true)"
  [[ -n "${first}" ]]
}

run_priv() {
  local path_hint="$1"
  shift

  local parent="${path_hint}"
  if [[ -e "${path_hint}" ]]; then
    parent="$(dirname "${path_hint}")"
  else
    while [[ ! -d "${parent}" && "${parent}" != "/" ]]; do
      parent="$(dirname "${parent}")"
    done
  fi

  if [[ -w "${parent}" ]]; then
    run_cmd "$@"
    return
  fi

  if [[ -n "${SUDO}" ]]; then
    run_cmd "${SUDO}" "$@"
    return
  fi

  fail "Need elevated privileges to modify ${path_hint}."
}

confirm_action() {
  local prompt="$1"
  if [[ "${AUTO_YES}" -eq 1 ]]; then
    return 0
  fi

  if [[ ! -t 0 ]]; then
    fail "${prompt} (non-interactive shell; rerun with --yes to auto-confirm)"
  fi

  local reply=""
  read -r -p "${prompt} [y/N]: " reply
  case "${reply}" in
    y|Y|yes|YES)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

ensure_build_dependencies() {
  local -a missing=()
  command -v cmake >/dev/null 2>&1 || missing+=("cmake")
  command -v ninja >/dev/null 2>&1 || missing+=("ninja")
  command -v g++ >/dev/null 2>&1 || missing+=("g++")
  command -v make >/dev/null 2>&1 || missing+=("make")

  if [[ ${#missing[@]} -eq 0 ]]; then
    info "Build dependencies already installed."
    return 0
  fi

  warn "Missing build dependencies: ${missing[*]}"

  local manager=""
  local manager_hint=""
  local -a install_cmd=()
  local update_cmd=()

  if command -v apt-get >/dev/null 2>&1; then
    manager="apt"
    manager_hint="$(command -v apt-get)"
    update_cmd=(apt-get update)
    install_cmd=(apt-get install -y build-essential cmake ninja-build)
  elif command -v dnf >/dev/null 2>&1; then
    manager="dnf"
    manager_hint="$(command -v dnf)"
    install_cmd=(dnf install -y gcc gcc-c++ make cmake ninja-build)
  elif command -v pacman >/dev/null 2>&1; then
    manager="pacman"
    manager_hint="$(command -v pacman)"
    install_cmd=(pacman -Sy --noconfirm base-devel cmake ninja)
  else
    fail "Package manager not recognized. Install these manually: ${missing[*]}"
  fi

  if ! confirm_action "Install missing dependencies using ${manager}?"; then
    fail "Cannot continue without required build dependencies."
  fi

  if [[ ${#update_cmd[@]} -gt 0 ]]; then
    info "Updating package index (${manager})"
    run_priv "${manager_hint}" "${update_cmd[@]}"
  fi

  info "Installing dependencies (${manager})"
  run_priv "${manager_hint}" "${install_cmd[@]}"

  local -a still_missing=()
  command -v cmake >/dev/null 2>&1 || still_missing+=("cmake")
  command -v ninja >/dev/null 2>&1 || still_missing+=("ninja")
  command -v g++ >/dev/null 2>&1 || still_missing+=("g++")
  command -v make >/dev/null 2>&1 || still_missing+=("make")
  if [[ ${#still_missing[@]} -gt 0 ]]; then
    fail "Dependencies still missing after installation: ${still_missing[*]}"
  fi
}

usage() {
  cat <<EOF
Usage: bash building-scripts/install-linux.sh [options]

Options:
  --install              Build and install
  --test                 Build only
  -y, --yes              Auto-confirm dependency installation prompts
  --system               Install to ${SYSTEM_ROOT}, expose ${SYSTEM_LINK}
  --user                 Install to ${USER_ROOT}, expose ${USER_LINK}
  --update-existing      Update an existing pixelferrite installation
  --uninstall-existing   Uninstall an existing pixelferrite installation
  --build-dir <path>     Custom build directory
  -h, --help             Show this help
EOF
}

detect_existing() {
  if [[ -L "${SYSTEM_LINK}" ]]; then
    local target
    target="$(readlink -f "${SYSTEM_LINK}")"
    if [[ "${target}" == */bin/pixelferrite ]]; then
      EXISTING_ROOT="$(dirname "$(dirname "${target}")")"
      EXISTING_LINK="${SYSTEM_LINK}"
      return 0
    fi
  fi

  if [[ -L "${USER_LINK}" ]]; then
    local target
    target="$(readlink -f "${USER_LINK}")"
    if [[ "${target}" == */bin/pixelferrite ]]; then
      EXISTING_ROOT="$(dirname "$(dirname "${target}")")"
      EXISTING_LINK="${USER_LINK}"
      return 0
    fi
  fi

  return 1
}

resolve_install_paths() {
  case "${TARGET}" in
    system)
      INSTALL_ROOT="${SYSTEM_ROOT}"
      GLOBAL_LINK="${SYSTEM_LINK}"
      ;;
    user)
      INSTALL_ROOT="${USER_ROOT}"
      GLOBAL_LINK="${USER_LINK}"
      ;;
    *)
      fail "Unsupported target: ${TARGET}"
      ;;
  esac
}

ensure_user_data_layout() {
  local config_root="${HOME}/.config/pixelferrite"
  local data_root="${HOME}/.local/share/pixelferrite"
  local ws_root="${data_root}/workspace/default"

  mkdir -p "${config_root}" "${data_root}/logs" "${data_root}/workspace"
  mkdir -p "${ws_root}/logs" "${ws_root}/reports" "${ws_root}/sessions"
  mkdir -p "${ws_root}/scripts" "${ws_root}/state" "${ws_root}/tmp" "${ws_root}/datasets"

  if [[ -f "${INSTALL_ROOT}/config/default.conf" && ! -f "${config_root}/default.conf" ]]; then
    cp -f "${INSTALL_ROOT}/config/default.conf" "${config_root}/default.conf"
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --install)
      MODE="install"
      shift
      ;;
    --test)
      MODE="test"
      shift
      ;;
    -y|--yes)
      AUTO_YES=1
      shift
      ;;
    --system)
      TARGET="system"
      shift
      ;;
    --user)
      TARGET="user"
      shift
      ;;
    --update-existing)
      ACTION="update"
      shift
      ;;
    --uninstall-existing)
      ACTION="uninstall"
      shift
      ;;
    --build-dir)
      [[ $# -ge 2 ]] || fail "--build-dir requires a value"
      BUILD_DIR="$2"
      PFF_BIN="${BUILD_DIR}/bin/pffconsole"
      PIXELGEN_BIN="${BUILD_DIR}/bin/pixelgen"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "Unknown option: $1"
      ;;
  esac
done

if [[ "${EUID}" -ne 0 && -x "$(command -v sudo 2>/dev/null || true)" ]]; then
  SUDO="sudo"
fi

if [[ "${ACTION}" != "install" ]]; then
  if ! detect_existing; then
    fail "No existing pixelferrite installation detected."
  fi
fi

if [[ "${ACTION}" == "uninstall" ]]; then
  info "Removing launcher ${EXISTING_LINK}"
  run_priv "${EXISTING_LINK}" rm -f "${EXISTING_LINK}"
  info "Removing install root ${EXISTING_ROOT}"
  run_priv "${EXISTING_ROOT}" rm -rf "${EXISTING_ROOT}"
  ok "Uninstalled pixelferrite."
  exit 0
fi

if [[ "${ACTION}" == "update" ]]; then
  if [[ "${EXISTING_ROOT}" == "${SYSTEM_ROOT}" ]]; then
    TARGET="system"
  elif [[ "${EXISTING_ROOT}" == "${USER_ROOT}" ]]; then
    TARGET="user"
  else
    fail "Unsupported existing root '${EXISTING_ROOT}' for auto-update."
  fi
fi

if [[ "${MODE}" == "install" || "${MODE}" == "test" ]]; then
  ensure_build_dependencies
fi

JOBS="$(nproc 2>/dev/null || echo 2)"
if command -v ninja >/dev/null 2>&1; then
  run_cmd cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
  run_cmd cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi
run_cmd cmake --build "${BUILD_DIR}" -j"${JOBS}"

[[ -x "${PFF_BIN}" ]] || fail "Missing binary: ${PFF_BIN}"
[[ -x "${PIXELGEN_BIN}" ]] || fail "Missing binary: ${PIXELGEN_BIN}"

if [[ "${MODE}" == "test" ]]; then
  ok "Test build ready:"
  echo "  ${PFF_BIN}"
  echo "  ${PIXELGEN_BIN}"
  exit 0
fi

resolve_install_paths
info "Installing to ${INSTALL_ROOT}"

STAGE_DIR="$(mktemp -d)"
cleanup() {
  rm -rf "${STAGE_DIR}"
}
trap cleanup EXIT

mkdir -p "${STAGE_DIR}/bin"

install -m 755 "${PFF_BIN}" "${STAGE_DIR}/bin/pffconsole"
install -m 755 "${PIXELGEN_BIN}" "${STAGE_DIR}/bin/pixelgen"
install -m 755 "${PFF_BIN}" "${STAGE_DIR}/bin/pixelferrite"

if dir_has_entries "${PROJECT_ROOT}/modules"; then
  cp -a "${PROJECT_ROOT}/modules" "${STAGE_DIR}/modules"
fi
if dir_has_entries "${PROJECT_ROOT}/modules/payloads"; then
  cp -a "${PROJECT_ROOT}/modules/payloads" "${STAGE_DIR}/payloads"
fi
if dir_has_entries "${BUILD_DIR}/lib"; then
  mkdir -p "${STAGE_DIR}/lib"
  cp -a "${BUILD_DIR}/lib/." "${STAGE_DIR}/lib/"
fi

if dir_has_entries "${PROJECT_ROOT}/docs" || dir_has_entries "${PROJECT_ROOT}/data" || \
   dir_has_entries "${PROJECT_ROOT}/labs" || [[ -f "${PROJECT_ROOT}/README.md" ]]; then
  mkdir -p "${STAGE_DIR}/resources"
  if dir_has_entries "${PROJECT_ROOT}/docs"; then
    mkdir -p "${STAGE_DIR}/resources/docs"
    cp -a "${PROJECT_ROOT}/docs/." "${STAGE_DIR}/resources/docs/"
  fi
  if dir_has_entries "${PROJECT_ROOT}/data"; then
    mkdir -p "${STAGE_DIR}/resources/data"
    cp -a "${PROJECT_ROOT}/data/." "${STAGE_DIR}/resources/data/"
  fi
  if dir_has_entries "${PROJECT_ROOT}/labs"; then
    mkdir -p "${STAGE_DIR}/resources/labs"
    cp -a "${PROJECT_ROOT}/labs/." "${STAGE_DIR}/resources/labs/"
  fi
  if [[ -f "${PROJECT_ROOT}/README.md" ]]; then
    cp -a "${PROJECT_ROOT}/README.md" "${STAGE_DIR}/resources/README.md"
  fi
fi

if [[ -d "${PROJECT_ROOT}/config" ]]; then
  cp -a "${PROJECT_ROOT}/config" "${STAGE_DIR}/config"
else
  mkdir -p "${STAGE_DIR}/config"
fi
cat > "${STAGE_DIR}/config/default.conf" <<EOF
# PixelFerrite default configuration
safety_mode=strict
module_root=modules
EOF

echo "PixelFerrite 0.1.0" > "${STAGE_DIR}/version.txt"

run_priv "${INSTALL_ROOT}" rm -rf "${INSTALL_ROOT}"
run_priv "${INSTALL_ROOT}" mkdir -p "${INSTALL_ROOT}"
run_priv "${INSTALL_ROOT}" cp -a "${STAGE_DIR}/." "${INSTALL_ROOT}/"

run_priv "$(dirname "${GLOBAL_LINK}")" mkdir -p "$(dirname "${GLOBAL_LINK}")"
run_priv "${GLOBAL_LINK}" ln -sfn "${INSTALL_ROOT}/bin/pixelferrite" "${GLOBAL_LINK}"

ensure_user_data_layout

ok "Installed framework root: ${INSTALL_ROOT}"
ok "Global launcher: ${GLOBAL_LINK}"
echo "Runtime user data:"
echo "  ${HOME}/.config/pixelferrite"
echo "  ${HOME}/.local/share/pixelferrite"
