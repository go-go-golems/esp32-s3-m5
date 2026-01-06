#!/usr/bin/env bash
set -euo pipefail

# Targeted erase of Zigbee persistence partitions on H2 (keeps everything else).
#
# Why: channel selection and commissioning can be overridden by persistent
# storage. This script wipes Zigbee storage while keeping the firmware intact.
#
# Usage:
#   H2_PORT=/dev/ttyACM0 ./scripts/21-h2-erase-zigbee-storage-only.sh
#
# Requirements:
# - IDF_PATH set (so we can find parttool.py)
# - The H2 project built at least once (so partition-table.bin exists), or the
#   script will build it.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
H2_PROJ_DEFAULT="${ROOT_DIR}/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp"
H2_PROJ="${H2_PROJ:-$H2_PROJ_DEFAULT}"
H2_PORT="${H2_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_48:31:B7:CA:45:5B-if00}"

if [[ -z "${IDF_PATH:-}" ]]; then
  echo "ERROR: IDF_PATH is not set (run: . $HOME/esp/esp-idf-*/export.sh)" >&2
  exit 2
fi

PARTTOOL="$IDF_PATH/components/partition_table/parttool.py"
if [[ ! -f "$PARTTOOL" ]]; then
  echo "ERROR: parttool.py not found at: $PARTTOOL" >&2
  exit 2
fi

if [[ ! -d "$H2_PROJ" ]]; then
  echo "ERROR: H2 project not found: $H2_PROJ" >&2
  exit 2
fi

if [[ ! -e "$H2_PORT" ]]; then
  echo "ERROR: H2_PORT not found: $H2_PORT" >&2
  exit 2
fi

BUILD_DIR="${H2_PROJ}/build"
PT_BIN="${BUILD_DIR}/partition_table/partition-table.bin"

if [[ ! -f "$PT_BIN" ]]; then
  echo "partition-table.bin missing; building project once to generate it..."
  idf.py -C "$H2_PROJ" set-target esp32h2
  idf.py -C "$H2_PROJ" build
fi

echo "Using partition table: $PT_BIN"
echo "Erasing zigbee partitions: zb_fct, zb_storage"

python3 "$PARTTOOL" --port "$H2_PORT" --partition-table-file "$PT_BIN" erase_partition --partition-name zb_fct
python3 "$PARTTOOL" --port "$H2_PORT" --partition-table-file "$PT_BIN" erase_partition --partition-name zb_storage

echo "Done. Reboot the H2 and watch formation logs."
