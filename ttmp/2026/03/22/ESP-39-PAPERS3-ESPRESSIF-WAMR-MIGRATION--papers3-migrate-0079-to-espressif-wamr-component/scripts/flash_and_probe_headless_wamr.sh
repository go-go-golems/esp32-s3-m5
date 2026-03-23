#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
shift || true

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console"
BUILD_DIR="${PROJECT_DIR}/build-headless"
SDKCONFIG_FILE="${BUILD_DIR}/sdkconfig"
IDF_EXPORT="/home/manuel/esp/esp-idf-5.3.4/export.sh"
SCRIPT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts"

COMMAND_ARGS=()
if [ "$#" -eq 0 ]; then
  COMMAND_ARGS+=(--command "wasm status")
  COMMAND_ARGS+=(--command "wasm run-preflush return-42")
  COMMAND_ARGS+=(--command "wasm replay hello-frame")
else
  for command in "$@"; do
    COMMAND_ARGS+=(--command "$command")
  done
fi

echo "[probe-headless] clearing stale monitor holders for ${PORT}"
pkill -f "idf_monitor.py -p ${PORT}" 2>/dev/null || true
pkill -f "esp_idf_monitor -p ${PORT}" 2>/dev/null || true
pkill -f "idf.py -C ${PROJECT_DIR} -p ${PORT} monitor" 2>/dev/null || true
sleep 1

echo "[probe-headless] building and flashing ${PROJECT_DIR} (headless variant)"
unset IDF_PYTHON_ENV_PATH IDF_PATH
source "${IDF_EXPORT}" >/dev/null
mkdir -p "${BUILD_DIR}"
rm -f "${SDKCONFIG_FILE}"
idf.py \
  -C "${PROJECT_DIR}" \
  -B "${BUILD_DIR}" \
  -D SDKCONFIG="${SDKCONFIG_FILE}" \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.headless" \
  -p "${PORT}" \
  build flash

echo "[probe-headless] probing commands: ${COMMAND_ARGS[*]}"
"${SCRIPT_DIR}/serial_probe_sequence.py" \
  --port "${PORT}" \
  --boot-settle-seconds 2.5 \
  --prompt-timeout 4.0 \
  --capture-seconds 8.0 \
  "${COMMAND_ARGS[@]}"
