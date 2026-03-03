#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build-windows-cross"
PF_BIN="${BUILD_DIR}/bin/pffconsole.exe"
PP_BIN="${BUILD_DIR}/bin/pixelgen.exe"
PIXELFERRITE_BIN="${BUILD_DIR}/bin/pixelferrite.exe"

MINGW_PREFIX="${MINGW_PREFIX:-x86_64-w64-mingw32}"
GENERATOR=""

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

success() {
  echo -e "${GREEN}$*${RESET}"
}

usage() {
  cat <<EOF
Usage: bash building-scripts/build-windows-from-linux.sh [options]

Cross-compiles PixelFerrite on Linux for Windows using MinGW-w64.

Options:
  --mingw-prefix <prefix>   MinGW triplet prefix (default: x86_64-w64-mingw32)
  --build-dir <path>        Custom build directory
  --generator <name>        CMake generator (default: Ninja if available)
  --clean                   Remove build directory before configure
  -h, --help                Show this help

Environment:
  MINGW_PREFIX              Same as --mingw-prefix
EOF
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || fail "Required command not found: $1"
}

CLEAN_BUILD=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mingw-prefix)
      [[ $# -ge 2 ]] || fail "--mingw-prefix requires a prefix"
      MINGW_PREFIX="$2"
      shift 2
      ;;
    --build-dir)
      [[ $# -ge 2 ]] || fail "--build-dir requires a path"
      BUILD_DIR="$2"
      PF_BIN="${BUILD_DIR}/bin/pffconsole.exe"
      PP_BIN="${BUILD_DIR}/bin/pixelgen.exe"
      PIXELFERRITE_BIN="${BUILD_DIR}/bin/pixelferrite.exe"
      shift 2
      ;;
    --generator)
      [[ $# -ge 2 ]] || fail "--generator requires a value"
      GENERATOR="$2"
      shift 2
      ;;
    --clean)
      CLEAN_BUILD=1
      shift
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

if [[ "$(uname -s)" != "Linux" ]]; then
  fail "This script is for Linux hosts only."
fi

require_cmd cmake
require_cmd "${MINGW_PREFIX}-gcc"
require_cmd "${MINGW_PREFIX}-g++"
require_cmd "${MINGW_PREFIX}-windres"

if [[ -z "${GENERATOR}" ]]; then
  if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
  fi
fi

if [[ "${CLEAN_BUILD}" -eq 1 ]]; then
  info "Cleaning build directory ${BUILD_DIR}"
  rm -rf "${BUILD_DIR}"
fi

info "Configuring Windows cross-build"
if [[ -n "${GENERATOR}" ]]; then
  cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
    -G "${GENERATOR}" \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER="${MINGW_PREFIX}-gcc" \
    -DCMAKE_CXX_COMPILER="${MINGW_PREFIX}-g++" \
    -DCMAKE_RC_COMPILER="${MINGW_PREFIX}-windres" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
  cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER="${MINGW_PREFIX}-gcc" \
    -DCMAKE_CXX_COMPILER="${MINGW_PREFIX}-g++" \
    -DCMAKE_RC_COMPILER="${MINGW_PREFIX}-windres" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

info "Building Windows executables"
cmake --build "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 2)"

[[ -f "${PF_BIN}" ]] || fail "Build completed but executable not found: ${PF_BIN}"
[[ -f "${PP_BIN}" ]] || fail "Build completed but executable not found: ${PP_BIN}"
cp -f "${PF_BIN}" "${PIXELFERRITE_BIN}"

if command -v file >/dev/null 2>&1; then
  file "${PF_BIN}" | grep -qi "PE32" || warn "pffconsole.exe was built but file type check did not match PE32."
  file "${PP_BIN}" | grep -qi "PE32" || warn "pixelgen.exe was built but file type check did not match PE32."
fi

success "Windows binaries created:"
echo "  ${PF_BIN}"
echo "  ${PP_BIN}"
echo "  ${PIXELFERRITE_BIN}"


