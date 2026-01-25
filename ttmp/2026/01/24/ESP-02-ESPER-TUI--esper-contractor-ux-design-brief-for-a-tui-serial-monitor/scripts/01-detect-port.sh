#!/usr/bin/env bash
set -euo pipefail

# Prints the best-guess serial port path for an ESP32-S3 over USB Serial/JTAG.
# Prefers stable /dev/serial/by-id links.

detect_port() {
  local byid
  byid="$(ls -1 /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* 2>/dev/null | head -n 1 || true)"
  if [[ -n "${byid}" ]]; then
    echo "${byid}"
    return 0
  fi

  # Fallbacks
  if [[ -e /dev/ttyACM0 ]]; then
    echo /dev/ttyACM0
    return 0
  fi
  if compgen -G "/dev/ttyACM*" > /dev/null; then
    ls -1 /dev/ttyACM* | head -n 1
    return 0
  fi

  echo "ERROR: could not auto-detect an ESP32 serial port (no /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* and no /dev/ttyACM*)." >&2
  exit 1
}

detect_port

