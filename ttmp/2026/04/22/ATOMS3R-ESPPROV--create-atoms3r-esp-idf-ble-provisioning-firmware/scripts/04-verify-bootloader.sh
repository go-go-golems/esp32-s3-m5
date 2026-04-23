#!/bin/bash
# Verify bootloader was flashed correctly
# Created: 2026-04-22

set -e

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/22/ATOMS3R-ESPPROV--create-atoms3r-esp-idf-ble-provisioning-firmware/sources/atoms3r-esp-idf"
PORT="/dev/ttyUSB0"
BAUD="115200"

cd "$PROJECT_DIR"
. ~/esp/esp-idf/export.sh

echo "Reading back bootloader from flash..."
esptool.py --chip esp32 --port "$PORT" --baud "$BAUD" \
  read_flash 0x1000 0x7000 /tmp/bootloader_readback.bin

echo ""
echo "Comparing with original..."
cmp -l build/bootloader/bootloader.bin /tmp/bootloader_readback.bin | wc -l || true

echo ""
echo "Bootloader verification complete."
