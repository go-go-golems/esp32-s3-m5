#!/usr/bin/env bash
set -euo pipefail

# Flash the 0031 Cardputer host firmware.
#
# Usage:
#   HOST_PORT=/dev/ttyACM1 ./scripts/22-host-flash.sh
#
# Defaults try to use stable /dev/serial/by-id/ names.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HOST_PROJ="${ROOT_DIR}/0031-zigbee-orchestrator"

HOST_PORT="${HOST_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00}"

if [[ ! -d "$HOST_PROJ" ]]; then
  echo "ERROR: Host project not found: $HOST_PROJ" >&2
  exit 2
fi

if [[ ! -e "$HOST_PORT" ]]; then
  echo "ERROR: HOST_PORT not found: $HOST_PORT" >&2
  echo "Set HOST_PORT=/dev/ttyACM? or a /dev/serial/by-id/ path." >&2
  exit 2
fi

echo "Host project: $HOST_PROJ"
echo "Host port:    $HOST_PORT"

idf.py -C "$HOST_PROJ" -p "$HOST_PORT" flash

