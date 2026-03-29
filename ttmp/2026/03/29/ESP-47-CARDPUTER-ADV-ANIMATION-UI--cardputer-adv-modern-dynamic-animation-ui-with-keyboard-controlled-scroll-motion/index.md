---
Title: Cardputer ADV modern dynamic animation UI with keyboard-controlled scroll motion
Ticket: ESP-47-CARDPUTER-ADV-ANIMATION-UI
Status: active
Topics:
    - cardputer-adv
    - cardputer
    - ui
    - animation
    - keyboard
    - display
    - m5gfx
    - esp-idf
    - firmware
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Menu-driven Cardputer UI composition and dirty redraw pattern
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Scrollback and single-task display ownership reference
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp
      Note: Single display-owner UI task and sprite render loop
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp
      Note: ADV-capable keyboard backend detection and semantic event mapping
    - Path: ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/imports/retro_macos_line_minimap.html
      Note: Imported donor prototype that defines the minimap and eased scroll-target interaction
ExternalSources: []
Summary: Evidence-backed design ticket for a new Cardputer ADV animation UI that reuses proven keyboard, render-loop, and scroll-control patterns from the existing Cardputer firmwares.
LastUpdated: 2026-03-29T16:05:00-04:00
WhatFor: Use this ticket when building a new Cardputer ADV firmware with a dynamic minimap/scroll UI, keyboard-triggered motion, and smooth animated viewport updates.
WhenToUse: Use this ticket as the primary entry point before reading the design doc or implementing the firmware.
---


# Cardputer ADV modern dynamic animation UI with keyboard-controlled scroll motion

## Overview

This ticket captures the design for a new standalone Cardputer ADV firmware that presents a modern, animated UI inspired by the imported donor prototype in `imports/retro_macos_line_minimap.html`. The design is grounded in the existing firmware patterns that already work well in this repository:

- `0066-cardputer-adv-ledchain-gfx-sim/` for Cardputer ADV keyboard detection, a single display-owner UI task, and modal overlay state handling
- `0022-cardputer-m5gfx-demo-suite/` for menu/list navigation, sprite composition, and dirty-region rendering discipline
- `0030-cardputer-console-eventbus/` for scrollback state, keyboard action routing, and keeping display rendering inside one task

The main deliverable is the intern-focused design guide in `design-doc/01-cardputer-adv-dynamic-animation-ui-analysis-design-and-implementation-guide.md`, plus the diary in `reference/01-investigation-diary.md`.

## Key Links

- Primary design doc: `design-doc/01-cardputer-adv-dynamic-animation-ui-analysis-design-and-implementation-guide.md`
- Diary: `reference/01-investigation-diary.md`
- Imported donor artifact: `imports/retro_macos_line_minimap.html`
- Tasks: `tasks.md`
- Changelog: `changelog.md`

## Current Status

Current status: **active**

The ticket is already set up, the donor HTML has been imported, and the evidence-backed implementation guide has been written. The remaining operational step is the final reMarkable delivery, plus any future implementation work in a new firmware directory.

## Recommended Reading Order

1. Read the design doc executive summary and current-state architecture sections.
2. Read the donor HTML mapping section to understand what parts of the browser prototype should and should not survive the port.
3. Read the implementation phases and pseudocode sections.
4. Read the diary if you need command history, rationale, or review instructions.

## Ticket Structure

- `design-doc/` contains the main analysis/design/implementation guide
- `reference/` contains the chronological investigation diary
- `imports/` contains the imported donor HTML prototype
- `playbooks/` is available if later validation/flash procedures need to be added
- `scripts/` is available for future analysis helpers
- `various/` and `archive/` remain empty for now
