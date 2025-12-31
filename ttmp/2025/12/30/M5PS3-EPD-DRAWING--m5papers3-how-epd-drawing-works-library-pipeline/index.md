---
Title: 'M5PaperS3: how EPD drawing works (library + pipeline)'
Ticket: M5PS3-EPD-DRAWING
Status: active
Topics:
    - epd
    - display
    - m5gfx
    - m5unified
    - esp-idf
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: components/M5GFX/docs/M5PaperS3.md
      Note: Docs stating EPDiy no longer used since v0.2.7
    - Path: components/M5GFX/src/M5GFX.cpp
      Note: PaperS3 board autodetect and Panel_EPD + Bus_EPD wiring
    - Path: components/M5GFX/src/lgfx/v1/panel/Panel_EPDiy.cpp
      Note: Legacy/optional backend gated on epdiy.h (not used here)
    - Path: components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp
      Note: Low-level scanline transfer over esp_lcd_i80
    - Path: components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Actual EPD update algorithm (LUTs
    - Path: main/hal/hal.h
      Note: Display alias is M5.Display (M5Unified/M5GFX stack)
    - Path: main/main.cpp
      Note: App-level full refresh loop + startWrite/endWrite usage
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-30T22:49:07.860306933-05:00
WhatFor: ""
WhenToUse: ""
---


# M5PaperS3: how EPD drawing works (library + pipeline)

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- epd
- display
- m5gfx
- m5unified
- esp-idf

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
