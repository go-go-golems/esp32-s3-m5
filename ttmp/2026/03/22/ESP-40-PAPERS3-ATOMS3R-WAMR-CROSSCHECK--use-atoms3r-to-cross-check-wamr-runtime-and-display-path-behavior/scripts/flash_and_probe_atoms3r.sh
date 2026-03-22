#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00}"
COMMAND="${2:-wasm status}"
BAUD="${BAUD:-460800}"
PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console"
IDF_EXPORT="/home/manuel/esp/esp-idf-5.3.4/export.sh"
SCRIPT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK--use-atoms3r-to-cross-check-wamr-runtime-and-display-path-behavior/scripts"
FLASH_ARGS_FILE="${PROJECT_DIR}/build/flash_args"

echo "[probe] clearing stale monitor holders for ${PORT}"
pkill -f "idf_monitor.py -p ${PORT}" 2>/dev/null || true
pkill -f "esp_idf_monitor -p ${PORT}" 2>/dev/null || true
pkill -f "/dev/ttyACM0" 2>/dev/null || true
sleep 1

echo "[probe] building ${PROJECT_DIR}"
unset IDF_PYTHON_ENV_PATH IDF_PATH
source "${IDF_EXPORT}" >/dev/null
idf.py -C "${PROJECT_DIR}" build >/dev/null

if [[ ! -f "${FLASH_ARGS_FILE}" ]]; then
  echo "missing ${FLASH_ARGS_FILE}" >&2
  exit 1
fi

read -r first_line <"${FLASH_ARGS_FILE}"
mapfile -t image_lines < <(tail -n +2 "${FLASH_ARGS_FILE}")

extra_args=()
if [[ -n "${first_line}" ]]; then
  # shellcheck disable=SC2206
  extra_args=(${first_line})
fi

write_args=()
for line in "${image_lines[@]}"; do
  [[ -z "${line}" ]] && continue
  # shellcheck disable=SC2206
  parts=(${line})
  if [[ ${#parts[@]} -ne 2 ]]; then
    echo "unexpected flash_args line: ${line}" >&2
    exit 1
  fi
  write_args+=("${parts[0]}" "${PROJECT_DIR}/build/${parts[1]}")
done

echo "[probe] flashing over USB Serial/JTAG with usb_reset/no_reset"
python -m esptool \
  --chip esp32s3 \
  -p "${PORT}" \
  -b "${BAUD}" \
  --before usb_reset \
  --after no_reset \
  write_flash \
  "${extra_args[@]}" \
  "${write_args[@]}"

echo "[probe] watchdog reset into app"
python -m esptool \
  --chip esp32s3 \
  -p "${PORT}" \
  --before no_reset \
  --after watchdog_reset \
  read_mac >/dev/null

echo "[probe] sending command: ${COMMAND}"
"${SCRIPT_DIR}/serial_send_and_capture.py" \
  --port "${PORT}" \
  --prompt "atom>" \
  --boot-settle-seconds 2.5 \
  --prompt-timeout 4.0 \
  --capture-seconds 8.0 \
  --command "${COMMAND}"
