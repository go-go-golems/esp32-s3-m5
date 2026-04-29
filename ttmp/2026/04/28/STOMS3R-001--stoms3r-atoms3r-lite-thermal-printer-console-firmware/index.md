---
Title: 'SToMS3R: AtomS3R Lite Thermal Printer Console Firmware'
Ticket: STOMS3R-001
Status: active
Topics:
    - esp32s3
    - atoms3r
    - thermal-printer
    - console
    - wifi
    - esp-idf
    - firmware
    - provisioning
    - escpos
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0037-cardputer-adv-fan-control-console/main/app_main.c
      Note: esp_console + USB Serial/JTAG pattern reference
    - Path: 0090-m5printer-research/docs/TECHNICAL-DEEP-DIVE.md
      Note: Complete ESC/POS protocol documentation and ATOM-PRINTER architecture analysis
    - Path: 0090-m5printer-research/reference/26-bitmap-printing.md
      Note: Bitmap printing reference for thermal printers
    - Path: 0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_printer.c
      Note: Existing ESP-IDF UART printer driver — UART init pattern reused
    - Path: stoms3r/main/app_main.c
      Note: Entry point — init NVS/WiFi/printer and start console REPL
    - Path: stoms3r/main/index.html
      Note: Web UI — text print
    - Path: stoms3r/main/nvs_store.c
      Note: NVS wrapper — save/load/erase WiFi credentials
    - Path: stoms3r/main/printer_cmd.c
      Note: Printer console commands — 9 commands with argtable3
    - Path: stoms3r/main/printer_drv.c
      Note: ESC/POS UART driver — all printer commands
    - Path: stoms3r/main/web_server.c
      Note: HTTP server — 4 endpoints
    - Path: stoms3r/main/wifi_cmd.c
      Note: WiFi console commands — scan/connect/status/disconnect/forget
    - Path: stoms3r/main/wifi_mgr.c
      Note: WiFi STA manager — scan/connect/disconnect/auto-reconnect
ExternalSources: []
Summary: 'SToMS3R firmware: ESP-IDF esp_console REPL on AtomS3R Lite driving K118 thermal printer over UART with WiFi management and NVS persistence.'
LastUpdated: 2026-04-28T21:51:31.829220665-04:00
WhatFor: Complete design and implementation guide for building the SToMS3R thermal printer firmware on M5Stack AtomS3R Lite.
WhenToUse: ""
---




# SToMS3R: AtomS3R Lite Thermal Printer Console Firmware

## Overview

SToMS3R ("Screw This, On My S3R") is a firmware project that turns an M5Stack
AtomS3R Lite (ESP32-S3) into a networked thermal printer controller using
`esp_console` over USB Serial/JTAG. The firmware provides interactive console
commands for WiFi management (scan, connect, disconnect, auto-reconnect from NVS)
and thermal printer control (text, barcodes, QR codes, bitmaps via ESC/POS).

### Key Documents

- **Design & Implementation Guide**: [design-doc/01-stoms3r-complete-design-and-implementation-guide.md](design-doc/01-stoms3r-complete-design-and-implementation-guide.md)
  — 2000+ line guide covering hardware, software stack, ESC/POS protocol,
  file-by-file implementation, and testing plan.
- **Diary**: [reference/01-diary.md](reference/01-diary.md)

### Hardware

| Component | Details |
|-----------|--------|
| Controller | M5Stack AtomS3R Lite (ESP32-S3-PICO-1-N8R8) |
| Printer | M5Stack K118 Thermal Printer Kit (58mm, 203dpi) |
| Connection | UART1 at 9600 baud, TX=GPIO5, RX=GPIO6 |
| Console | USB Serial/JTAG (no GPIO pins consumed) |
| Power | USB-C for logic, 12V/2.5A for printer mechanism |

### Previous Work

| Ticket | Description |
|--------|-------------|
| 0090 | K118 thermal printer research (ESC/POS, Arduino firmware analysis) |
| 0091 | BLE provisioning attempt |
| 0092 | ESP-IDF provisioning on ATOM Lite (ESP32-PICO-D4) |

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- atoms3r
- thermal-printer
- console
- wifi
- esp-idf
- firmware
- provisioning
- escpos

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
