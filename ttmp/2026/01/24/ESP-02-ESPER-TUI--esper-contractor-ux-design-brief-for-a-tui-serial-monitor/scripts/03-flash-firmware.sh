#!/usr/bin/env bash
set -euo pipefail

ESPIDF_EXPORT="${ESPIDF_EXPORT:-$HOME/esp/esp-idf-5.4.1/export.sh}"
ESPPORT="${ESPPORT:-}"

if [[ -z "${ESPPORT}" ]]; then
  ESPPORT="$(./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/01-detect-port.sh)"
fi

if [[ ! -f "${ESPIDF_EXPORT}" ]]; then
  echo "ERROR: ESP-IDF export script not found: ${ESPIDF_EXPORT}" >&2
  exit 1
fi

echo "Flashing to: ${ESPPORT}" >&2

source "${ESPIDF_EXPORT}"
cd esper/firmware/esp32s3-test

idf.py -p "${ESPPORT}" flash

