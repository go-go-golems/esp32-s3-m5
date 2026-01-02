---
Title: 'Cardputer: keyboard scancode calibration firmware'
Ticket: 0023-CARDPUTER-KB-SCANCODES
Status: active
Topics:
    - cardputer
    - keyboard
    - scancodes
    - esp32s3
    - esp-idf
    - debugging
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/apps/app_keyboard/select_menu/select_menu.cpp
      Note: Example of navigation using raw key numbers (not arrow HID codes)
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Vendor HAL matrix scan implementation (ground truth reference)
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h
      Note: Vendor HAL key-value map and key-number (matrix index) conventions
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/keyboard/keymap.h
      Note: HID usage constants (includes arrow keys) and modifier bit definitions
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Raw matrix scanning + debug logging reference (scan_state/in_mask)
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Keyboard scan + higher-level key mapping patterns used in Cardputer projects
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Current CardputerKeyboard scanner used by the demo-suite (edge events + modifiers)
    - Path: esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/build.sh
      Note: Build/flash/monitor helper (commit 1fc1f77)
    - Path: esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp
      Note: Live 4x14 matrix UI + pressed keyNum logging (commit 1fc1f77)
    - Path: esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/kb_scan.cpp
      Note: Raw matrix scan + alt IN0/IN1 autodetect (commit 1fc1f77)
    - Path: esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/tools/cfg_to_header.py
      Note: Host helper to convert CFG JSON into a header
    - Path: esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings.h
      Note: Reusable action/chord decoder (subset match
    - Path: esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h
      Note: Captured semantic binding table from wizard output
    - Path: esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/layout.h
      Note: Shared key legend table + keynum helpers (commit cdb68fa)
    - Path: esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp
      Note: Reusable scanner implementation (commit cdb68fa)
ExternalSources: []
Summary: Design + plan for a standalone ESP-IDF firmware that interactively discovers Cardputer keyboard scan codes (including Fn-combos) and outputs a reusable decoder configuration.
LastUpdated: 2026-01-02T02:30:00Z
WhatFor: ""
WhenToUse: ""
---




# Cardputer: keyboard scancode calibration firmware

## Overview

We have evidence that our current “keycodes” / mappings may not correspond cleanly to what the Cardputer keyboard *actually* emits, and that arrow navigation is accessed through **Fn combos**. This ticket proposes a **separate diagnostic firmware** dedicated to:

- discovering the raw matrix scanning behavior (pins, scan states, masks)
- identifying stable “scan identifiers” for keys and combos (Fn+key, Shift+key, etc.)
- generating a decoder config that we can reuse in other firmware (e.g. the demo-suite)
- producing verbose serial logs so we can debug wiring/layout mismatch quickly

## Key docs

- Analysis / design (implementation-ready): `analysis/01-keyboard-scancode-calibration-firmware.md`

## Tasks

See [tasks.md](./tasks.md).

## Changelog

See [changelog.md](./changelog.md).

