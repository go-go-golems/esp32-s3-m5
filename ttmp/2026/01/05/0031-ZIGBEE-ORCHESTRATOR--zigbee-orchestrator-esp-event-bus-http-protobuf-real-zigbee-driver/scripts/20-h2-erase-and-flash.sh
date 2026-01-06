#!/usr/bin/env bash
set -euo pipefail

# Erase + flash the H2 NCP firmware.
#
# Usage:
#   H2_PORT=/dev/ttyACM0 ./scripts/20-h2-erase-and-flash.sh
#
# By default, uses the Unit Gateway H2 USB Serial/JTAG port by-id and the
# upstream esp-zigbee-sdk example project path. Override via H2_PROJ/H2_PORT.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
H2_PROJ_DEFAULT="${ROOT_DIR}/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp"
H2_PROJ="${H2_PROJ:-$H2_PROJ_DEFAULT}"
H2_PORT="${H2_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_48:31:B7:CA:45:5B-if00}"

if [[ ! -d "$H2_PROJ" ]]; then
  echo "ERROR: H2 project not found: $H2_PROJ" >&2
  echo "Set H2_PROJ=/abs/path/to/esp_zigbee_ncp" >&2
  exit 2
fi

if [[ ! -e "$H2_PORT" ]]; then
  echo "ERROR: H2_PORT not found: $H2_PORT" >&2
  echo "Set H2_PORT=/dev/ttyACM? or a /dev/serial/by-id/ path." >&2
  exit 2
fi

echo "H2 project: $H2_PROJ"
echo "H2 port:    $H2_PORT"

idf.py -C "$H2_PROJ" set-target esp32h2
idf.py -C "$H2_PROJ" -p "$H2_PORT" erase-flash
idf.py -C "$H2_PROJ" -p "$H2_PORT" flash
