---
Title: 'Guide: USB Serial/JTAG esp_console setup'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: 'Expanded research and writing guide for USB Serial/JTAG esp_console setup.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter produce a precise esp_console setup guide.'
WhenToUse: 'Use before writing the USB Serial/JTAG console document.'
---

# Guide: USB Serial/JTAG esp_console setup

## Purpose

This guide enables a ghostwriter to create a single-topic doc that explains how to enable and troubleshoot `esp_console` over USB Serial/JTAG on ESP32-S3 boards. The doc should focus on exact config flags, the expected boot log clues, and the most common conflict: using USB Serial/JTAG for both a REPL and a raw data terminal.

## Assignment Brief

Write a didactic guide for developers who are puzzled when the REPL prompt does not accept input. The key outcome is a checklist that makes the console come alive over `/dev/ttyACM*` and helps identify when UART is accidentally selected.

## Environment Assumptions

- ESP-IDF 5.4.x
- USB Serial/JTAG hardware (`/dev/ttyACM*`)
- Menuconfig access for console configuration

## Source Material to Review

- `AGENT.md` (console defaults and required config flags)
- `esp32-s3-m5/0029-mock-zigbee-http-hub/README.md` (WiFi console commands + UART fallback warning)
- `esp32-s3-m5/0030-cardputer-console-eventbus/README.md` (REPL prompt, command examples)
- `esp32-s3-m5/0037-cardputer-adv-fan-control-console/README.md` (USB Serial/JTAG console expectation)
- `esp32-s3-m5/0038-cardputer-adv-serial-terminal/README.md` (REPL conflict with USB backend)
- `esp32-s3-m5/0025-cardputer-lvgl-demo/README.md` (console commands + screenshot capture)
- `esp32-s3-m5/ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2026/01/05/0029a-ADD-WIFI-CONSOLE--add-esp-console-wifi-config-scan-to-mock-zigbee-hub/reference/01-diary.md`

## Technical Content Checklist

- Required config flags:
  - `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
  - `CONFIG_ESP_CONSOLE_UART_DEFAULT=n`
  - `CONFIG_ESP_CONSOLE_UART_CUSTOM=n`
  - `CONFIG_ESP_CONSOLE_USB_CDC=n`
- How local `sdkconfig` can override `sdkconfig.defaults`
- How to detect the wrong backend (log line: `esp_console started over UART`)
- Serial port selection (`/dev/ttyACM*`, by-id path)
- Conflict note: if USB Serial/JTAG is used for a data terminal, skip starting `esp_console`
- Suggested fallback: use `picocom` or `minicom` when `idf.py monitor` cannot send input

## Pseudocode Sketch

Use a short config snippet in the doc to reinforce the required settings.

```text
# menuconfig intent summary
USB Serial/JTAG console: enabled
UART console: disabled
USB CDC console: disabled
```

## Pitfalls to Call Out

- UART console defaults will make the USB/JTAG REPL appear dead.
- `idf.py monitor` may not send input when multiple tools hold the port.
- USB Serial/JTAG used for a custom serial backend can conflict with REPL input.

## Suggested Outline

1. What USB Serial/JTAG is and when to use it
2. Required `menuconfig` settings (with exact symbols)
3. Verifying the console backend in logs
4. Connecting with `idf.py monitor` vs external serial tools
5. Troubleshooting checklist

## Commands

```bash
# Verify console configuration
idf.py menuconfig

# Build and monitor a known-good REPL example
cd esp32-s3-m5/0030-cardputer-console-eventbus
idf.py set-target esp32s3
idf.py build flash monitor
```

## Exit Criteria

- Doc lists exact Kconfig flags and explains why each matters.
- Doc explains how to detect UART fallback and fix it.
- Doc includes at least one concrete REPL prompt and command list.

## Notes

Keep the scope on console setup and troubleshooting; do not cover event bus logic beyond what is necessary to show a working REPL.
