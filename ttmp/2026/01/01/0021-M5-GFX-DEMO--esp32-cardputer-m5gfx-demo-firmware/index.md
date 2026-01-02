---
Title: 'ESP32 Cardputer: M5GFX demo firmware'
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - ui
    - keyboard
    - esp32s3
    - esp-idf
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: Cardputer autodetect and panel/backlight wiring
    - Path: ../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp
      Note: Primary drawing/text/sprite/image API surface
    - Path: 0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: Baseline demo for M5Canvas + waitDMA
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/01-demo-firmware-real-world-scenario-catalog.md
      Note: Scenario-driven demo list to implement in the firmware
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/02-implementation-guide-list-view-selection-a2.md
      Note: Starter scenario A2 implementation guide
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/03-implementation-guide-status-bar-hud-overlay-a1.md
      Note: Starter scenario A1 implementation guide
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/04-implementation-guide-frame-time-perf-overlay-b2.md
      Note: Starter scenario B2 implementation guide
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/05-implementation-guide-terminal-log-console-e1.md
      Note: Starter scenario E1 implementation guide
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/06-implementation-guide-screenshot-to-serial-b3.md
      Note: Starter scenario B3 implementation guide
ExternalSources: []
Summary: Research + plan for an ESP-IDF Cardputer firmware that demos M5GFX/LovyanGFX features (primitives, text, sprites, images, QR, widgets, perf).
LastUpdated: 2026-01-01T19:46:11.216093331-05:00
WhatFor: ""
WhenToUse: ""
---






# ESP32 Cardputer: M5GFX demo firmware

## Overview

This ticket collects research and a concrete implementation plan for a **single “graphics demo suite” firmware** for the **ESP32-S3 M5 Cardputer**.

The firmware’s purpose is twofold:

1. Provide an interactive, on-device “catalog” of what **M5GFX / LovyanGFX** can do (primitives, text, sprites, image decode, transformations, widgets like `LGFX_Button`, etc.).
2. Provide copy/paste-ready reference demos that we can reuse in other firmware projects in `esp32-s3-m5/`.

The output of this ticket is primarily documentation (analysis + diary), plus a task plan for a future code drop (the actual firmware implementation can be tackled as follow-up work under this same ticket).

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- Analysis: `analysis/01-m5gfx-functionality-inventory-demo-plan.md`
- Diary: `reference/01-diary.md`

## Status

Current status: **active**

## Topics

- esp32
- esp32-s3
- cardputer
- m5stack
- m5gfx
- graphics
- widgets
- ui
- firmware
- demo

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
