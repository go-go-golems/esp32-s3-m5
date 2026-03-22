#!/usr/bin/env bash
set -euo pipefail

echo "== serial ports =="
python -m serial.tools.list_ports -v

echo
echo "== lsusb =="
lsusb

echo
echo "== tty holders =="
if [[ $# -gt 0 ]]; then
  lsof -t "$1" || true
else
  lsof -t /dev/ttyACM0 || true
fi
