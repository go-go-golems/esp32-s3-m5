---
Title: 'Guide: Cardputer keyboard input'
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
Summary: 'Expanded research and writing guide for Cardputer keyboard input documentation.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter produce a precise, single-topic guide on Cardputer keyboard scanning and scancodes.'
WhenToUse: 'Use before writing the Cardputer keyboard input guide.'
---

# Guide: Cardputer keyboard input

## Purpose

This guide exists so a ghostwriter can create a focused, developer-facing document about the Cardputer keyboard. The final doc should answer a single question: how to scan the built-in keyboard matrix, interpret scancodes, and handle Fn combos reliably.

## Assignment Brief

Write a didactic, single-topic guide that teaches keyboard input on the Cardputer. The reader should walk away with a clear mental model of the matrix scan, an accurate pin map, and a practical recipe for debounce and edge detection. Treat the scancode calibrator as the authority for mapping raw positions to tokens. Avoid drifting into display, audio, or networking topics.

## Environment Assumptions

- ESP-IDF 5.4.1 toolchain
- M5Stack Cardputer hardware
- USB Serial/JTAG console access (`/dev/ttyACM*`)

## Source Material to Review

- `esp32-s3-m5/0007-cardputer-keyboard-serial/README.md` (pin map, scan notes, serial echo behavior)
- `esp32-s3-m5/0012-cardputer-typewriter/README.md` (keyboard-to-UI loop context)
- `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/README.md` (scancode calibration workflow)
- `esp32-s3-m5/0038-cardputer-adv-serial-terminal/README.md` (TCA8418 keyboard notes for ADV variant)
- `esp32-s3-m5/M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h` (authoritative pin matrix)
- `esp32-s3-m5/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2026/01/02/0023-CARDPUTER-KB-SCANCODES--cardputer-keyboard-scancode-calibration-firmware/reference/01-diary.md`

## Technical Content Checklist

- Pin matrix table: OUT pins `GPIO8/GPIO9/GPIO11`, IN pins `GPIO13/GPIO15/GPIO3/GPIO4/GPIO5/GPIO6/GPIO7` (verify in `keyboard.h`)
- Scan cadence (10ms scale; confirm in code)
- Debounce strategy and how edge detection is derived from level scans
- Event model: key down/up edges vs level polling
- Fn combos and key token mapping (reference calibrator output)
- Real-time serial echo pitfalls (line-buffered terminals)
- Cardputer-ADV variant note (TCA8418 over I2C; not the same as the built-in matrix)

## Pseudocode Sketch

Use the pseudocode in the final doc to explain the scan loop and edge detection.

```c
// Pseudocode: matrix scan and edge detection
state prev = 0;
loop every 10ms:
  state curr = 0
  for each out_pin in OUT_PINS:
    drive(out_pin, LOW)
    wait_us(50)
    in_mask = read_inputs(IN_PINS) // active-low -> invert
    curr = curr | (in_mask << row_offset)
    drive(out_pin, HIGH)
  edges = curr ^ prev
  key_down = edges & curr
  key_up = edges & ~curr
  prev = curr
  emit_events(key_down, key_up)
```

## Pitfalls to Call Out

- Line-buffered terminals may hide realtime key echo until newline.
- Local `sdkconfig` can override USB Serial/JTAG console defaults.
- Pin map differences between Cardputer and Cardputer-ADV.

## Suggested Outline

1. Overview: what the keyboard hardware is and what this doc covers
2. Pin matrix and wiring assumptions (table)
3. Scan loop and debounce logic (pseudo-code)
4. Key events and mapping to tokens
5. Fn combos and calibration workflow
6. Debugging: serial echo and console behavior
7. Troubleshooting checklist

## Commands

```bash
# Build and flash the baseline keyboard echo demo
cd esp32-s3-m5/0007-cardputer-keyboard-serial
idf.py set-target esp32s3
idf.py build flash monitor

# Run the scancode calibrator
cd ../0023-cardputer-kb-scancode-calibrator
./build.sh build
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash
```

## Exit Criteria

- The final doc includes an accurate pin map and scan loop description.
- The final doc explains debounce and edge detection with a clear example.
- The final doc documents the scancode calibration workflow and outputs.
- The final doc has a troubleshooting section for console echo issues.

## Notes

Keep the doc single-topic: keyboard scanning and scancodes. Link to the ADV keyboard only as a contrast so readers do not confuse the two inputs.
