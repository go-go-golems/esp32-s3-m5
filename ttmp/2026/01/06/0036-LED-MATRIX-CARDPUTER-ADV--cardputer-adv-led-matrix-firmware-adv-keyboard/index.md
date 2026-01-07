---
Title: Cardputer-ADV LED matrix firmware (ADV keyboard)
Ticket: 0036-LED-MATRIX-CARDPUTER-ADV
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - console
    - usb-serial-jtag
    - keyboard
    - sensors
    - serial
    - debugging
    - flashing
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0036-cardputer-adv-led-matrix-console
      Note: 'New firmware project (Phase 1): esp_console over USB Serial/JTAG + MAX7219 chain driver'
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: |-
        Added blink mode (loop on/off with pauses) and made bare  print help
        Added blink mode (loop on/off with pauses) and made bare 'matrix' print help
        Gamma-mapped brightness command (bright/gamma) on top of MAX7219 intensity
    - Path: 0036-cardputer-adv-led-matrix-console/main/max7219.c
      Note: Reduced SPI clock for signal integrity
    - Path: ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_read_boot_log.py
      Note: Non-TTY boot-log capture helper (used for validation in automation)
    - Path: ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_send_matrix_commands.py
      Note: Non-TTY esp_console command sender (init + patterns) for validation
ExternalSources:
    - local:01-cardputer-adv-comparison.md
Summary: ""
LastUpdated: 2026-01-06T19:38:40-05:00
WhatFor: ""
WhenToUse: ""
---






# Cardputer-ADV LED matrix firmware (ADV keyboard)

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp-idf
- esp32s3
- cardputer
- console
- usb-serial-jtag
- keyboard
- sensors
- serial
- debugging
- flashing

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
