#!/usr/bin/env bash
set -euo pipefail

# One-command helper for Tutorial 0045.
#
# Common usage:
#   ./build.sh web                 # build web bundle into main/assets/
#   ./build.sh build               # idf.py build
#   ./build.sh all                 # web + idf build
#   ./build.sh -p /dev/ttyACM0 flash monitor
#
# Override ESP-IDF export path if needed:
#   IDF_EXPORT_SH=/path/to/esp-idf/export.sh ./build.sh build

REQUIRED_ESP_IDF_VERSION_PREFIX="${REQUIRED_ESP_IDF_VERSION_PREFIX:-5.4}"
IDF_EXPORT_SH="${IDF_EXPORT_SH:-$HOME/esp/esp-idf-5.4.1/export.sh}"

if [[ "${ESP_IDF_VERSION:-}" != ${REQUIRED_ESP_IDF_VERSION_PREFIX}* ]]; then
  if [[ ! -f "${IDF_EXPORT_SH}" ]]; then
    echo "ERROR: ESP-IDF export script not found at: ${IDF_EXPORT_SH}" >&2
    echo "Set IDF_EXPORT_SH to the correct path, e.g.:" >&2
    echo "  IDF_EXPORT_SH=\$HOME/esp/esp-idf-5.4.1/export.sh ./build.sh build" >&2
    exit 1
  fi
  # shellcheck disable=SC1090
  source "${IDF_EXPORT_SH}" >/dev/null
fi

# Prefer the ESP-IDF python venv if available. Some shell environments leave a
# global python earlier in PATH (e.g. pyenv), which can break idf.py imports.
if [[ -n "${IDF_PYTHON_ENV_PATH:-}" && -x "${IDF_PYTHON_ENV_PATH}/bin/python" ]]; then
  export PATH="${IDF_PYTHON_ENV_PATH}/bin:${PATH}"
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${PROJECT_DIR}"

cmd="${1:-build}"

if [[ "${cmd}" == "web" ]]; then
  shift || true
  if ! command -v npm >/dev/null 2>&1; then
    echo "ERROR: npm not found; install Node.js to build the web bundle" >&2
    exit 1
  fi
  cd web
  npm ci
  npm run build
  exit 0
fi

if [[ "${cmd}" == "all" ]]; then
  shift || true
  "${PROJECT_DIR}/build.sh" web
  exec idf.py build "$@"
fi

exec idf.py "$@"
