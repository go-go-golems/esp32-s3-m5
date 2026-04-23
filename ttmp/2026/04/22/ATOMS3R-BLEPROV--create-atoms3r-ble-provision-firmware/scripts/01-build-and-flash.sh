#!/bin/bash
# Build and flash ATOMS3R BLE Provisioning Firmware (Arduino+NimBLE)
# Created: 2026-04-22

set -e

PROJECT_DIR="/tmp/atoms3r-flash"
PORT="/dev/ttyUSB0"
BAUD="115200"

echo "=== ATOMS3R BLE Provisioning Build & Flash ==="
echo ""

cd "$PROJECT_DIR"

# Build
echo "Building..."
pio run

# Flash
echo ""
echo "Flashing..."
echo "IMPORTANT: Hold the button on your ATOM printer during flash!"
read -p "Press ENTER when ready (holding button)..."

"$HOME/.platformio/penv/bin/python" "$HOME/.platformio/packages/tool-esptoolpy/esptool.py" \
  --chip esp32 --port "$PORT" --baud "$BAUD" \
  --before default_reset --after hard_reset \
  write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 ".pio/build/m5stack-atom/bootloader.bin" \
  0x8000 ".pio/build/m5stack-atom/partitions.bin" \
  0x10000 ".pio/build/m5stack-atom/firmware.bin"

echo ""
echo "Done! Release the button now."
