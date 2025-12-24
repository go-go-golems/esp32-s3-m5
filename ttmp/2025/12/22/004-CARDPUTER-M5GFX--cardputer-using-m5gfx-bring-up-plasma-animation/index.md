---
Title: 'Cardputer: using M5GFX (bring-up + plasma animation)'
Ticket: 004-CARDPUTER-M5GFX
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - display
    - m5gfx
    - st7789
    - spi
    - animation
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: Ground-truth Cardputer ST7789 SPI pins/offsets/rotation/backlight in M5GFX autodetect
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Vendor pattern using M5GFX + LGFX_Sprite canvas
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/partitions.csv
      Note: Known-good Cardputer partition table to compare
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/sdkconfig.defaults
      Note: Known-good Cardputer sdkconfig.defaults to compare flashing/partition settings
    - Path: esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/main/hello_world_main.cpp
      Note: Reference M5GFX bring-up and DMA-synced full-frame blit
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp
      Note: Reference plasma implementation using M5Canvas + waitDMA
    - Path: esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: New Cardputer chapter 0011 scaffold using M5GFX autodetect
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T22:06:59.427230297-05:00
WhatFor: ""
WhenToUse: ""
---



# Cardputer: using M5GFX (bring-up + plasma animation)

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
- cardputer
- display
- m5gfx
- st7789
- spi
- animation

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
