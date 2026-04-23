---
Title: Implementation Guide
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
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_printer.c
      Note: ATOM printer UART implementation
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/main.c
      Note: Provisioning and WiFi lifecycle implementation
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/partitions.csv
      Note: 4MB flash partition layout sized for BLE provisioning
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/sdkconfig.defaults
      Note: ATOM Lite ESP-IDF build configuration
ExternalSources: []
Summary: Implementation guide for the ATOM Lite / ESP32-PICO-D4 ESP-IDF BLE provisioning firmware stored in 0092.
LastUpdated: 2026-04-23T16:20:00-04:00
WhatFor: Use when building, flashing, reviewing, or extending the M5 printer ATOM Lite ESP-IDF provisioning firmware.
WhenToUse: Use for ATOM Lite / ESP32-PICO-D4 work, not ATOMS3R / ESP32-S3 firmware.
---


# Implementation Guide

## Executive Summary

This ticket builds a fresh ESP-IDF firmware for the **M5Stack ATOM Thermal Printer Kit** controller: the **ATOM Lite / ESP32-PICO-D4**. The firmware source intentionally lives in:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
```

The firmware uses Espressif's `wifi_provisioning` manager with BLE transport. Users flash it over the ATOM Lite's FTDI USB serial bridge (`/dev/ttyUSB0`), provision WiFi with the **ESP BLE Provisioning** phone app, and receive a small status receipt from the thermal printer once WiFi connects.

## Problem Statement

The previous ESP-IDF provisioning source was created for **ATOMS3R / ESP32-S3**, which is the wrong target for the M5Stack ATOM Thermal Printer Kit. The printer kit's controller is an ATOM Lite with an ESP32-PICO-D4, 4 MB flash, FTDI USB-UART, and a UART-connected thermal printer module.

We need a clean ESP-IDF project that matches this hardware:

| Hardware item | Setting |
| --- | --- |
| ESP-IDF target | `esp32` |
| MCU | ESP32-PICO-D4 |
| Flash | 4 MB |
| Host USB interface | FTDI UART, `/dev/ttyUSB0` |
| Console | UART0 at 115200 baud via FTDI |
| Printer UART | UART2, 9600 8N1 |
| Printer TX/RX | TX GPIO23, RX GPIO33 |
| Optional printer CTS | GPIO19, documented but not enabled yet |
| Button | GPIO39 active-low |

## Proposed Solution

Create a standalone ESP-IDF project under the 0092 folder with:

- `sdkconfig.defaults` for ESP32, 4 MB flash, NimBLE-backed BLE provisioning, and UART console.
- A custom partition table with enough room for the BLE/WiFi provisioning binary.
- A single application that:
  - initializes NVS, WiFi, event loop, button, and printer UART;
  - checks whether WiFi credentials already exist through `wifi_prov_mgr_is_provisioned()`;
  - starts BLE provisioning when not provisioned;
  - starts WiFi station mode when already provisioned;
  - logs the provisioning service name, Security 1 PoP, and QR payload;
  - prints a receipt once an IP address is obtained;
  - erases NVS and reboots when GPIO39 is held for five seconds.

## Build and Flash Commands

From the project directory:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
. /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

If the serial autobaud/reset path is unreliable, lower the flashing speed:

```bash
idf.py -p /dev/ttyUSB0 -b 115200 flash monitor
```

Exit `idf.py monitor` with `Ctrl-]`.

## Provisioning Procedure

1. Flash and open the monitor.
2. Watch for a log like:

   ```text
   Provision with Espressif's 'ESP BLE Provisioning' app:
     Transport : BLE
     Device    : M5PRN_XXXXXX
     Security  : Security 1
     PoP       : 12345678
   ```

3. Open the **ESP BLE Provisioning** app.
4. Select the `M5PRN_XXXXXX` device.
5. Use Security 1 and PoP `12345678`.
6. Send the WiFi SSID/password.
7. Confirm the monitor logs `WiFi connected, IP=...`.
8. Confirm the printer emits a short status receipt.

## Design Decisions

### Use ESP-IDF target `esp32`, not `esp32s3`

The ATOM Lite uses ESP32-PICO-D4. The kernel log showing FTDI USB serial (`0403:6001`) is expected and is compatible with `idf.py flash monitor`.

### Use UART console, not USB Serial/JTAG

ATOM Lite does not provide ESP32-S3-style native USB Serial/JTAG. The correct monitor path is UART0 via the FTDI adapter, usually `/dev/ttyUSB0`.

### Use NimBLE BLE provisioning

The ESP-IDF 5.3.4 provisioning example uses NimBLE for BLE-only provisioning on ESP32, reducing memory pressure compared with full BTDM/Bluedroid mode.

### Let `wifi_prov_mgr` own WiFi credential storage

The provisioning manager stores credentials in the WiFi NVS namespace. The firmware uses `wifi_prov_mgr_is_provisioned()` instead of maintaining a parallel custom `provisioned` flag.

### Keep printer support minimal first

The printer protocol starts with ESC/POS-like basics: reset (`ESC @`), text, and line feeds. Image printing, barcode/QR helpers, CTS flow control, and web/MQTT services are future work.

## Alternatives Considered

### Port the ATOMS3R project directly

Rejected because it targeted the wrong chip family and board assumptions. It was useful only as conceptual reference for provisioning flow.

### Keep using Arduino/M5Atom firmware

Rejected for this ticket because the goal is native `idf.py build`, `idf.py flash`, and `idf.py monitor` workflow.

### Use SoftAP provisioning

Rejected for first implementation because BLE provisioning is friendlier on iPhone and avoids switching the phone to the device AP.

## Implementation Plan

Completed initial implementation:

1. Create docmgr ticket and relate 0092 firmware path.
2. Scaffold ESP-IDF project under 0092.
3. Add BLE provisioning app flow.
4. Add printer UART helper.
5. Add button factory reset.
6. Build with ESP-IDF 5.3.4 for target `esp32`.
7. Document flash/monitor commands.

Next validation work:

1. Flash hardware with `/dev/ttyUSB0`.
2. Verify BLE advertisement appears in the phone app.
3. Provision real WiFi credentials.
4. Verify monitor IP log and receipt output.
5. Decide whether to add LED status using SK6812/RMT.

## Review Notes

Start code review at:

- `main/main.c` — provisioning event flow, reset task, WiFi lifecycle.
- `main/app_printer.c` — UART setup and print helpers.
- `sdkconfig.defaults` — target-specific assumptions for ATOM Lite.
- `partitions.csv` — 4 MB flash layout and app size budget.

## Open Questions

- Whether the printer base needs CTS flow control on GPIO19 for large raster/image prints.
- Whether to keep PoP fixed (`12345678`) for development or generate per-device PoP for production.
- Whether to add a small HTTP/MQTT print API after provisioning succeeds.
