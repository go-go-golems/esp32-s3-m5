---
title: "M5 Printer ATOM Lite ESP-IDF BLE Provisioning Firmware"
tags:
  - project
  - m5stack
  - atom-lite
  - esp32-pico-d4
  - esp-idf
  - ble
  - wifi-provisioning
  - thermal-printer
created: 2026-04-23
repo: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision
status: active
type: project
---

# M5 Printer ATOM Lite ESP-IDF BLE Provisioning Firmware

This directory contains the firmware source for docmgr ticket `ATOMLITE-PRINTER-PROV`.

Authoritative ticket docs live at:

```text
../ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer
```

Firmware source lives at:

```text
source/atomlite-printer-prov
```

## Target

| Item | Value |
| --- | --- |
| Kit | M5Stack ATOM Thermal Printer Kit / K118 |
| Controller | ATOM Lite |
| MCU | ESP32-PICO-D4 |
| ESP-IDF target | `esp32` |
| USB interface | FTDI UART, usually `/dev/ttyUSB0` |
| Printer UART | UART2, 9600 8N1, TX GPIO23, RX GPIO33 |
| Button | GPIO39 active-low, hold 5 seconds to erase NVS/reboot |

## Quick build

```bash
cd source/atomlite-printer-prov
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
