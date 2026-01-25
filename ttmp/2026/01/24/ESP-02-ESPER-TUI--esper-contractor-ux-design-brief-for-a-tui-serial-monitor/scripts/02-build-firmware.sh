#!/usr/bin/env bash
set -euo pipefail

ESPIDF_EXPORT="${ESPIDF_EXPORT:-$HOME/esp/esp-idf-5.4.1/export.sh}"

if [[ ! -f "${ESPIDF_EXPORT}" ]]; then
  echo "ERROR: ESP-IDF export script not found: ${ESPIDF_EXPORT}" >&2
  exit 1
fi

source "${ESPIDF_EXPORT}"

cd esper/firmware/esp32s3-test

idf.py set-target esp32s3
idf.py build

