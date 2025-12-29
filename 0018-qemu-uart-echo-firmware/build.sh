#!/usr/bin/env bash
set -euo pipefail

# One-command build helper for Track C UART echo firmware.
#
# - Sources ESP-IDF only when needed (based on ESP_IDF_VERSION).
# - Then runs `idf.py build` (or any idf.py command you pass).
#
# Usage:
#   ./build.sh                 # == idf.py build
#   ./build.sh set-target esp32s3
#   ./build.sh build
#   ./build.sh qemu
#   ./build.sh qemu monitor
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

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${PROJECT_DIR}"

if [[ $# -eq 0 ]]; then
  set -- build
fi

exec idf.py "$@"


