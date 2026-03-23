#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
shift || true

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control"
BUILD_DIR="${PROJECT_DIR}/build-system-allocator"
IDF_EXPORT="/home/manuel/esp/esp-idf-5.3.4/export.sh"
SERIAL_PROBE="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/23/ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL--papers3-minimal-wamr-allocator-control-firmware-to-isolate-instantiate-vs-psram-contamination/scripts/serial_probe_sequence.py"

COMMAND_ARGS=()
if [ "$#" -eq 0 ]; then
  COMMAND_ARGS+=(--command "wasm status")
  COMMAND_ARGS+=(--command "wasm replay psram-persistent-init")
  COMMAND_ARGS+=(--command "wasm instantiate-bare-keepalive return-42")
  COMMAND_ARGS+=(--command "wasm replay psram-persistent-touch-sync")
else
  for command in "$@"; do
    COMMAND_ARGS+=(--command "$command")
  done
fi

echo "[allocator-system] clearing stale monitor holders for ${PORT}"
pkill -f "idf_monitor.py -p ${PORT}" 2>/dev/null || true
pkill -f "esp_idf_monitor -p ${PORT}" 2>/dev/null || true
pkill -f "idf.py -C ${PROJECT_DIR} -p ${PORT} monitor" 2>/dev/null || true
sleep 1

echo "[allocator-system] building and flashing ${PROJECT_DIR} (${BUILD_DIR})"
unset IDF_PYTHON_ENV_PATH IDF_PATH
source "${IDF_EXPORT}" >/dev/null
idf.py -C "${PROJECT_DIR}" -B "${BUILD_DIR}" \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.system_allocator" \
  build flash -p "${PORT}"

echo "[allocator-system] probing commands: ${COMMAND_ARGS[*]}"
python "${SERIAL_PROBE}" \
  --port "${PORT}" \
  --boot-settle-seconds 2.5 \
  --prompt-timeout 8.0 \
  --capture-seconds 10.0 \
  "${COMMAND_ARGS[@]}"
