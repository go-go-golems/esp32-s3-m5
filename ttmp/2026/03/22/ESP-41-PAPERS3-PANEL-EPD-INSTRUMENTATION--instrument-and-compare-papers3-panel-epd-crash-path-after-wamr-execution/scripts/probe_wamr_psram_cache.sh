#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console"
IDF_EXPORT="/home/manuel/esp/esp-idf-5.3.4/export.sh"
SERIAL_PROBE="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/serial_probe_sequence.py"

echo "[psram-cache] clearing stale monitor holders for ${PORT}"
pkill -f "idf_monitor.py -p ${PORT}" 2>/dev/null || true
pkill -f "esp_idf_monitor -p ${PORT}" 2>/dev/null || true
pkill -f "idf.py -C ${PROJECT_DIR} -p ${PORT} monitor" 2>/dev/null || true
sleep 1

echo "[psram-cache] flashing ${PROJECT_DIR}"
unset IDF_PYTHON_ENV_PATH IDF_PATH
source "${IDF_EXPORT}" >/dev/null
idf.py -C "${PROJECT_DIR}" -p "${PORT}" flash

echo "[psram-cache] probing instantiate + PSRAM sync sequence"
python "${SERIAL_PROBE}" \
  --port "${PORT}" \
  --boot-settle-seconds 2.5 \
  --prompt-timeout 4.0 \
  --capture-seconds 8.0 \
  --command "wasm replay psram-persistent-init" \
  --command "wasm instantiate-bare-keepalive return-42" \
  --command "wasm replay psram-persistent-touch-sync"
