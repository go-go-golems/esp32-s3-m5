#!/bin/bash
# Build ATOMS3R ESP-IDF BLE Provisioning Firmware
# Created: 2026-04-22

set -e

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/22/ATOMS3R-ESPPROV--create-atoms3r-esp-idf-ble-provisioning-firmware/sources/atoms3r-esp-idf"

cd "$PROJECT_DIR"
. ~/esp/esp-idf/export.sh

# Set target
idf.py set-target esp32

# Build
idf.py build

echo ""
echo "Build complete!"
echo "Binary: build/atoms3r-esp-idf.bin"
