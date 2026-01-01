# 0020 — Cardputer BLE Keyboard Host (WIP)

Goal: pair/connect to a BLE HID keyboard (HOGP) as a Central and print key events over the USB Serial/JTAG console, with an `esp_console` control plane.

## Build

From this directory:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh
idf.py set-target esp32s3
idf.py build
```

## Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

## Console Quickstart

At the `ble>` prompt:

- `scan on 30`
- `devices`  # or: devices <substr> | devices clear | devices events on|off
- `connect 0`
- `pair 0`  # or: pair <addr> [pub|rand]
- `keylog on`
