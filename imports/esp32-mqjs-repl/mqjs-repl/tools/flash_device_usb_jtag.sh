#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
flash_device_usb_jtag.sh

Stable flashing for ESP32-S3 devices using USB Serial/JTAG where `idf.py flash`
may hit transient disconnect/re-enumeration issues.

Defaults:
  - Port auto-detects via /dev/serial/by-id/*Espressif*USB_JTAG*
  - Flash uses: --before usb_reset --after no_reset
  - Then reboots using a watchdog reset to run the app

Usage:
  ./tools/flash_device_usb_jtag.sh [--port PATH] [--baud BAUD]

Examples:
  ./tools/flash_device_usb_jtag.sh
  ./tools/flash_device_usb_jtag.sh --port /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...-if00
  ./tools/flash_device_usb_jtag.sh --port /dev/ttyACM0 --baud 115200

EOF
}

PORT="auto"
BAUD="115200"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      PORT="$2"
      shift 2
      ;;
    --baud)
      BAUD="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown arg: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${PROJECT_DIR}"

resolve_port() {
  local port="$1"
  if [[ "${port}" != "auto" ]]; then
    echo "${port}"
    return 0
  fi

  local candidates=()
  if [[ -d /dev/serial/by-id ]]; then
    while IFS= read -r -d '' p; do
      candidates+=("$p")
    done < <(find /dev/serial/by-id -maxdepth 1 -type l -name '*Espressif*USB_JTAG*' -print0 2>/dev/null || true)
  fi

  if [[ ${#candidates[@]} -eq 1 ]]; then
    echo "${candidates[0]}"
    return 0
  fi
  if [[ ${#candidates[@]} -gt 1 ]]; then
    echo "error: multiple Espressif USB_JTAG ports found; pass --port explicitly:" >&2
    printf '  %s\n' "${candidates[@]}" >&2
    return 1
  fi

  if [[ -e /dev/ttyACM0 ]]; then
    echo "/dev/ttyACM0"
    return 0
  fi

  echo "error: could not auto-detect USB Serial/JTAG port; pass --port" >&2
  return 1
}

PORT="$(resolve_port "${PORT}")"
echo "Using port: ${PORT}" >&2

FLASH_ARGS_FILE="${PROJECT_DIR}/build/flash_args"
if [[ ! -f "${FLASH_ARGS_FILE}" ]]; then
  echo "error: missing ${FLASH_ARGS_FILE} (run ./build.sh build first)" >&2
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
    echo "error: unexpected flash_args line: ${line}" >&2
    exit 1
  fi
  write_args+=("${parts[0]}" "${parts[1]}")
done

python -m esptool \
  --chip esp32s3 \
  -p "${PORT}" \
  -b "${BAUD}" \
  --before usb_reset \
  --after no_reset \
  write_flash \
  "${extra_args[@]}" \
  "${write_args[@]}"

# Reboot into the flashed application. USB Serial/JTAG can re-enumerate; using
# by-id paths avoids hard-coding /dev/ttyACM0.
python -m esptool \
  --chip esp32s3 \
  -p "${PORT}" \
  --before no_reset \
  --after watchdog_reset \
  read_mac >/dev/null

echo "OK: flashed and rebooted (USB Serial/JTAG)" >&2

