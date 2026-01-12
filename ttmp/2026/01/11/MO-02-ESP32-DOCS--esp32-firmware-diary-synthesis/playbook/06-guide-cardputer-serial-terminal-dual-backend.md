---
Title: 'Guide: Cardputer serial terminal dual backend'
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
Summary: 'Expanded research and writing guide for the Cardputer serial terminal dual-backend doc.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter draft the USB/JTAG vs Grove UART terminal guide.'
WhenToUse: 'Use before writing the Cardputer serial terminal doc.'
---

# Guide: Cardputer serial terminal dual backend

## Purpose

This guide enables a ghostwriter to produce a focused doc on the Cardputer serial terminal firmware, which routes keyboard input to either USB Serial/JTAG or Grove UART while showing a local on-screen buffer. The final doc should help a developer choose and configure the correct backend and wire Grove UART safely.

## Assignment Brief

Write a single-topic guide that explains how keystrokes flow from the keyboard to the screen and out to the selected serial backend. Include a clear backend comparison and note the console conflict when USB Serial/JTAG is used for both REPL and raw terminal data.

## Environment Assumptions

- ESP-IDF 5.4.x
- Cardputer hardware
- USB Serial/JTAG console access
- Optional UART adapter for Grove pins

## Source Material to Review

- `esp32-s3-m5/0015-cardputer-serial-terminal/README.md`
- `esp32-s3-m5/0038-cardputer-adv-serial-terminal/README.md`
- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/README.md` (UART/Grove pin usage context)
- `esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/reference/01-diary.md`

## Technical Content Checklist

- Default Grove UART pins (G1 RX, G2 TX) and crossing TX/RX
- How the on-screen buffer updates and echoes
- Menuconfig options for backend selection (USB Serial/JTAG vs UART)
- Interaction with `esp_console` when USB Serial/JTAG is used for data
- Behavior when "Show incoming bytes" is enabled

## Pseudocode Sketch

Use a short data-flow sketch to explain routing.

```c
// Pseudocode: terminal routing
on_keypress(key):
  buffer.append(key)
  screen.render(buffer)
  if backend == USB_JTAG:
    usb_write(key)
  else if backend == GROVE_UART:
    uart_write(key)

on_serial_rx(bytes):
  if show_incoming:
    buffer.append(bytes)
    screen.render(buffer)
```

## Pitfalls to Call Out

- USB Serial/JTAG cannot host both REPL and raw serial terminal simultaneously.
- Grove UART wiring errors (TX/RX not crossed).
- Serial port re-enumeration during flash; use by-id path.

## Suggested Outline

1. What the terminal does and why it exists
2. Backend options and when to pick each
3. Wiring diagram for Grove UART
4. UI buffer behavior and echo modes
5. Troubleshooting checklist

## Commands

```bash
cd esp32-s3-m5/0015-cardputer-serial-terminal
idf.py set-target esp32s3
idf.py build flash monitor
```

## Exit Criteria

- Doc includes a clear backend comparison table.
- Doc states default Grove pin mapping and wiring instructions.
- Doc explains the USB/JTAG vs REPL conflict.

## Notes

Keep scope on terminal routing and buffer behavior; do not expand into keyboard scan internals (link to keyboard guide instead).
