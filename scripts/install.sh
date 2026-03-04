#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" == "Darwin" ]]; then
  bash "$(dirname "$0")/../building-scripts/install-macos.sh" --install --user "$@"
else
  bash "$(dirname "$0")/../building-scripts/install-linux.sh" --install --user "$@"
fi


