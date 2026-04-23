---
Title: ATOM Lite ESP-IDF BLE Provisioning Firmware for M5 Printer
Ticket: ATOMLITE-PRINTER-PROV
Status: active
Topics:
    - esp-idf
    - firmware
    - ble
    - provisioning
    - m5stack
    - atom-lite
    - thermal-printer
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/index.md
      Note: Source-directory overview pointing back to this docmgr ticket
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
      Note: Firmware source directory for the ATOM Lite ESP-IDF provisioning implementation
ExternalSources: []
Summary: Build native ESP-IDF BLE WiFi provisioning firmware for the ATOM Lite controller in the M5Stack ATOM Thermal Printer Kit.
LastUpdated: 2026-04-23T16:25:00-04:00
WhatFor: Use to track the 0092 ATOM Lite printer provisioning firmware implementation.
WhenToUse: Use for the ATOM Lite / ESP32-PICO-D4 M5 printer, not ATOMS3R / ESP32-S3.
---


# ATOM Lite ESP-IDF BLE Provisioning Firmware for M5 Printer

## Overview

This ticket tracks a fresh ESP-IDF firmware for the **M5Stack ATOM Thermal Printer Kit** controller. The target is the **ATOM Lite / ESP32-PICO-D4**, not ATOMS3R.

The firmware source is intentionally stored in the 0092 project folder:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
```

## Current Status

- ESP-IDF project scaffolded for target `esp32`.
- BLE WiFi provisioning implemented with `wifi_provisioning` manager and BLE transport.
- Printer UART implemented on UART2: TX GPIO23, RX GPIO33, 9600 8N1.
- GPIO39 five-second factory reset implemented by erasing NVS and rebooting.
- Build validated with ESP-IDF 5.4.1.
- Hardware flash/provisioning validation remains open.

## Key Links

- [Implementation Guide](./design-doc/01-implementation-guide.md)
- [Diary](./reference/01-diary.md)
- [Tasks](./tasks.md)
- [Changelog](./changelog.md)

## Build Quick Start

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Review Focus

- `main/main.c` — provisioning and WiFi event flow.
- `main/app_printer.c` — UART2 thermal printer helper.
- `sdkconfig.defaults` — ATOM Lite ESP-IDF configuration.
- `partitions.csv` — app partition sized for BLE provisioning.
