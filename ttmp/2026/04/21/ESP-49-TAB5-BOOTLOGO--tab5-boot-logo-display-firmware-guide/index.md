---
Title: Tab5 boot logo display firmware guide
Ticket: ESP-49-TAB5-BOOTLOGO
Status: active
Topics:
    - firmware
    - display
    - lvgl
    - mipidsi
    - boot
    - esp-idf
    - m5stack
    - tab5
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c
      Note: Tutorial firmware entrypoint and current boot order.
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c
      Note: Final repaired display bring-up path using the BSP wrapper sequence.
    - Path: M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp
      Note: Factory reference initialization order used as the comparison baseline.
ExternalSources: []
Summary: Ticket covering the Tab5 boot logo firmware, including the initial design guide, the original failure analysis, the repaired display bring-up path, and the later PSRAM-based display-stability tuning.
LastUpdated: 2026-04-22T02:45:00Z
WhatFor: Use this ticket to understand, reproduce, and continue the Tab5 boot logo firmware effort.
WhenToUse: Use when onboarding to the Tab5 display bring-up work or reviewing both the failure and repaired states.
---

# Tab5 boot logo display firmware guide

## Overview

This ticket tracks the attempt to build a minimal Tab5 firmware that shows an M5 boot logo on the 5-inch display and then continues to run the Wi-Fi / HTTP demo stack.

The work has moved through three important phases:

1. initial design and architecture mapping,
2. failure reproduction and detailed bug analysis,
3. repaired display bring-up plus follow-up PSRAM tuning for display stability.

The project is now substantially healthier than the original failing build:

- the tutorial firmware exists under `esp32-s3-m5/0051-tab5-boot-logo`,
- it builds and flashes successfully,
- the binary fits in a 2 MB app partition,
- the original display-init hang is fixed,
- the app reaches its final `ready` state,
- and PSRAM now runs at 200 MHz in the tuned build.

The main remaining item is straightforward: a fresh human visual check should confirm whether the earlier fluttering edge artifact is fully gone after the 200 MHz PSRAM build.

## Key Links

- Design guide: `design-doc/01-tab5-boot-logo-display-firmware-design-and-implementation-guide.md`
- Failure analysis: `design-doc/02-tab5-boot-logo-firmware-bug-report-and-display-bring-up-failure-analysis.md`
- Resolution report: `design-doc/03-tab5-boot-logo-display-failure-resolution-report-and-residual-display-stability-notes.md`
- Diary: `reference/01-diary.md`
- Obsidian vault report copy: `reference/02-obsidian-vault-report-tab5-display-bring-up-failure-and-display-architecture.md`
- Tasks: `tasks.md`
- Changelog: `changelog.md`
- Scripts: `scripts/`

## Status

Current status: **active**

Current engineering conclusion:

- the original bring-up failure was caused by missing board preparation before ST7123 panel access,
- switching to the BSP-driven display path fixed the hard hang,
- later display instability matched DSI underrun from low PSRAM throughput,
- and enabling 200 MHz PSRAM aligned the project with the original firmware’s intended memory performance.

## Topics

- firmware
- display
- lvgl
- mipidsi
- boot
- esp-idf
- m5stack
- tab5

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- `design-doc/` - design notes, failure analysis, repair notes, and implementation guidance
- `reference/` - diary and supporting reference material
- `playbooks/` - command sequences and test procedures
- `scripts/` - ticket-local debugging scripts with numeric prefixes
- `various/` - working notes and research
- `archive/` - deprecated or reference-only artifacts
