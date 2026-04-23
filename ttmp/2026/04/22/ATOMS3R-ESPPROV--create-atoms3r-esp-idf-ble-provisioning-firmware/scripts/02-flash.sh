#!/bin/bash
# Flash ATOMS3R ESP-IDF firmware
# Created: 2026-04-22

set -e

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/22/ATOMS3R-ESPPROV--create-atoms3r-esp-idf-ble-provisioning-firmware/sources/atoms3r-esp-idf"
PORT="/dev/ttyUSB0"
BAUD="115200"

cd "$PROJECT_DIR"
. ~/esp/esp-idf/export.sh

echo "=== Flashing ATOMS3R ESP-IDF Firmware ==="
echo ""
echo "IMPORTANT: Hold the button on your ATOM printer during flash!"
read -p "Press ENTER when ready (holding button)..."

# Erase flash first
echo "Erasing flash..."
esptool.py --chip esp32 --port "$PORT" --baud "$BAUD" erase_flash

# Flash all partitions
echo "Flashing..."
esptool.py --chip esp32 --port "$PORT" --baud "$BAUD" \
  --before default_reset --after hard_reset \
  write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/atoms3r-esp-idf.bin

echo ""
echo "Done! Release the button now."
