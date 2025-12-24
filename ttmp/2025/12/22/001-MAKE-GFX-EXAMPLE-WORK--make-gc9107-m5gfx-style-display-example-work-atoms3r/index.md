---
Title: Make GC9107/M5GFX-style display example work (AtomS3R)
Ticket: 001-MAKE-GFX-EXAMPLE-WORK
Status: closed
Topics:
    - esp32s3
    - esp-idf
    - display
    - gc9107
    - m5gfx
    - atoms3r
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5AtomS3-UserDemo/datasheets/GC9107_DataSheet_V1.2.pdf
      Note: Authoritative controller datasheet for specialist validation
    - Path: M5AtomS3-UserDemo/src/main.cpp
      Note: Known-working Arduino+M5Unified demo build target (PIO) used as ground truth for display behavior
    - Path: M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: M5GFX board/panel config showing GC9107 offset_y behavior and backlight strategy on AtomS3
    - Path: M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_GC9A01.hpp
      Note: M5GFX reference containing Panel_GC9107 + its init sequence (known-good via Arduino/PIO demos)
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/README.md
      Note: User-facing instructions and debugging knobs for tutorial 0008
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/main/Kconfig.projbuild
      Note: Menuconfig knobs (offset
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/main/hello_world_main.c
      Note: ESP-IDF esp_lcd-based GC9107 bring-up + animation + debug logs; current black-screen investigation lives here
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/managed_components/espressif__esp_lcd_gc9107/esp_lcd_gc9107.c
      Note: Managed GC9107 panel driver implementation we compare against M5GFX
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/sdkconfig
      Note: Current effective configuration values (gap=32
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T22:06:49.469888233-05:00
WhatFor: ""
WhenToUse: ""
---



# Make GC9107/M5GFX-style display example work (AtomS3R)

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- esp-idf
- display
- gc9107
- m5gfx
- atoms3r

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
