---
Title: Serial Cardputer Terminal (keyboard echo + UART/USB output)
Ticket: 001-SERIAL-CARDPUTER-TERMINAL
Status: completed
Topics:
    - esp32
    - esp32-s3
    - m5stack
    - cardputer
    - uart
    - usb-serial
    - console
    - keyboard
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/README.md
      Note: 0015 build/flash/config instructions
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild
      Note: 0015 Kconfig surface for serial backend selection and terminal semantics
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: '0015 implementation: keyboard->screen terminal + USB/UART backend TX + optional RX'
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/sdkconfig.defaults
      Note: 0015 reproducible defaults for Cardputer + backend behavior
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-25T23:03:45.256284061-05:00
WhatFor: ""
WhenToUse: ""
---



# Serial Cardputer Terminal (keyboard echo + UART/USB output)

## Overview

This ticket specifies a Cardputer “serial terminal” app: **type on the built-in keyboard**, see your input **immediately on the LCD**, and transmit the same bytes to a **menuconfig-selectable** serial backend (**USB-Serial-JTAG** or hardware **UART**).

The goal is an implementation-ready recipe that reuses the already-proven Cardputer building blocks in this repo, rather than re-deriving hardware details.

## Key Links

- **Build plan (analysis)**: `analysis/01-build-plan-cardputer-serial-terminal-keyboard-echo-uart-usb-backend.md`
- **Investigation diary**: `reference/01-diary.md`
- **Prior art (code)**:
  - `esp32-s3-m5/0007-cardputer-keyboard-serial/` (keyboard scan + serial echo UX fixes)
  - `esp32-s3-m5/0012-cardputer-typewriter/` (M5GFX canvas text UI)
- **Prior art (docs)**:
  - `esp32-s3-m5/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--.../` (design doc + diary)
  - `esp32-s3-m5/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--.../` (analysis + diary)

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32
- esp32-s3
- m5stack
- cardputer
- uart
- usb-serial
- console
- keyboard
- ui

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
