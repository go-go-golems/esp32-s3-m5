#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
shift || true

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console"
IDF_EXPORT="/home/manuel/esp/esp-idf-5.3.4/export.sh"
SERIAL_PROBE="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/serial_probe_sequence.py"

COMMAND_ARGS=()
if [ "$#" -eq 0 ]; then
  COMMAND_ARGS+=(--command "wasm replay clear-only")
else
  for command in "$@"; do
    COMMAND_ARGS+=(--command "$command")
  done
fi

echo "[panel-epd] clearing stale monitor holders for ${PORT}"
pkill -f "idf_monitor.py -p ${PORT}" 2>/dev/null || true
pkill -f "esp_idf_monitor -p ${PORT}" 2>/dev/null || true
pkill -f "idf.py -C ${PROJECT_DIR} -p ${PORT} monitor" 2>/dev/null || true
sleep 1

echo "[panel-epd] building and flashing ${PROJECT_DIR}"
unset IDF_PYTHON_ENV_PATH IDF_PATH
source "${IDF_EXPORT}" >/dev/null
idf.py -C "${PROJECT_DIR}" -p "${PORT}" build flash

echo "[panel-epd] probing commands: ${COMMAND_ARGS[*]}"
python "${SERIAL_PROBE}" \
  --port "${PORT}" \
  --boot-settle-seconds 2.5 \
  --prompt-timeout 4.0 \
  --capture-seconds 8.0 \
  "${COMMAND_ARGS[@]}"
