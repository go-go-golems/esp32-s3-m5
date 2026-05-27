#!/usr/bin/env bash
set -euo pipefail

REQUIRED_ESP_IDF_VERSION_PREFIX="${REQUIRED_ESP_IDF_VERSION_PREFIX:-5.4}"
IDF_EXPORT_SH="${IDF_EXPORT_SH:-$HOME/esp/esp-idf-5.4.2/export.sh}"

if [[ "${ESP_IDF_VERSION:-}" != ${REQUIRED_ESP_IDF_VERSION_PREFIX}* ]]; then
  if [[ ! -f "${IDF_EXPORT_SH}" ]]; then
    echo "ERROR: ESP-IDF export script not found at: ${IDF_EXPORT_SH}" >&2
    echo "Set IDF_EXPORT_SH to the correct path, e.g.:" >&2
    echo "  IDF_EXPORT_SH=\$HOME/esp/esp-idf-5.4.2/export.sh ./build.sh" >&2
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
