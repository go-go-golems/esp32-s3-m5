#!/usr/bin/env bash
set -euo pipefail

echo "== tty candidates =="
find /dev -maxdepth 1 \( -name 'ttyACM*' -o -name 'ttyUSB*' -o -name 'cu.usb*' -o -name 'tty.usb*' \) -print | sort || true

echo
echo "== /dev/serial links =="
find /dev/serial -maxdepth 2 -type l -print 2>/dev/null | sort || true

echo
echo "== usb devices =="
lsusb 2>/dev/null || true

echo
echo "== video devices =="
find /dev -maxdepth 1 \( -name 'video*' -o -name 'media*' \) -print | sort || true

echo
echo "== espressif usb sysfs entries =="
for dev in /sys/bus/usb/devices/*; do
  [[ -f "${dev}/idVendor" ]] || continue
  vendor="$(cat "${dev}/idVendor" 2>/dev/null || true)"
  [[ "${vendor}" == "303a" ]] || continue
  product="$(cat "${dev}/idProduct" 2>/dev/null || true)"
  manufacturer="$(cat "${dev}/manufacturer" 2>/dev/null || true)"
  product_name="$(cat "${dev}/product" 2>/dev/null || true)"
  echo "${dev}: ${vendor}:${product} ${manufacturer} ${product_name}"
done
