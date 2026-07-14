# 0105 — Cardcore MeshCore Companion Terminal

An ESP-IDF 5.5.4 firmware foundation for an M5Stack Cardputer-ADV plus Cap
LoRa-1262. It will become a keyboard-first MeshCore **Companion** terminal;
it will not repeat traffic in the MVP.

## Current milestone

Task `5ptk` creates a reproducible ESP-IDF project and proves the pinned
Arduino-ESP32 component can be initialized at an intentionally narrow
compatibility boundary. No radio, display, keyboard, network, BLE, GPS, or SD
feature is enabled yet.

## Build

```bash
source ~/esp/esp-idf-5.5.4/export.sh
idf.py set-target esp32s3
rm -f sdkconfig                 # only when defaults change
idf.py build
```

## Flash and monitor

Use only one process for the attached USB Serial/JTAG port:

```bash
source ~/esp/esp-idf-5.5.4/export.sh
idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_AC:A7:04:04:88:F4-if00 flash monitor
```

Do not transmit until the Cap LoRa-1262 antenna is attached and its board
configuration has been validated in the raw-radio bring-up task.

## Documentation

Architecture, task plan, and diary live in ticket
`0104-CARDCORE-MESHCORE` under `ttmp/2026/07/13/`.
