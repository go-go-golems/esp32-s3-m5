---
Title: Implementation diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Demo-suite firmware scaffold (display init + keyboard + list view)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.h
      Note: Reusable ListView widget API (A2)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Cardputer matrix keyboard scanner used for input mapping
ExternalSources: []
Summary: Step-by-step implementation diary for the 0021 M5GFX demo-suite firmware, separate from the initial research diary.
LastUpdated: 2026-01-02T00:45:00Z
WhatFor: ""
WhenToUse: ""
---

# Implementation diary

## Step 1: Create demo-suite project scaffold + A2 list view skeleton

This step creates a new ESP-IDF project for the demo-suite and implements the first reusable UI building block: a keyboard-driven ListView (starter scenario A2). The immediate goal is a compile-ready firmware scaffold that can serve as the home/menu for all subsequent demos.

### What I did
- Created new ESP-IDF project `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite` reusing the vendored `M5GFX` component via `EXTRA_COMPONENT_DIRS`.
- Implemented `CardputerKeyboard` (matrix scan) to emit keypress edge events (W/S/Enter/Del).
- Implemented `ListView` widget with selection + scrolling + ellipsis truncation.
- Wired it together in `app_main` so the device boots into a simple menu list.

### Why
- The ticket’s tasks require a “demo catalog app”; the ListView is the core reusable widget for navigation, settings pages, and file pickers.
- Getting bring-up, input, and stable rendering working early reduces risk for all later demo modules.

### What worked
- The project follows the same M5GFX component integration pattern as tutorials `0011` and `0015` (Cardputer autodetect + sprite present + `waitDMA()`).

### What didn't work
- N/A (initial scaffold).

### What I learned
- N/A (implementation underway).

### What was tricky to build
- Key event semantics: emitting edge events based on physical key positions (not shifted glyphs) to avoid “shift changes the key name” issues.

### What warrants a second pair of eyes
- Keyboard scan correctness on real hardware (primary vs alt IN0/IN1 autodetect).
- ListView layout math (line height + padding) against the Cardputer font metrics.

### What should be done in the future
- Add a small on-screen footer that documents the current key bindings (align with the design doc’s navigation contract).
- Add a “demo details/help overlay” screen (so `Enter` opens a per-demo help page before running the demo).

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` then review:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`

## Step 2: Compile the new firmware

This step validates that the new project builds cleanly under ESP-IDF 5.4.1 and produces a flashable binary. The main outcome is a confirmed compile baseline to iterate on for the remaining starter scenarios (A1/B2/E1/B3).

### What I did
- Built the project:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `cd esp32-s3-m5/0022-cardputer-m5gfx-demo-suite`
  - `idf.py set-target esp32s3`
  - `idf.py build`

### Why
- The ticket’s “demo suite” is only useful if it compiles and can be flashed as a known-good baseline.

### What worked
- `idf.py build` succeeded and produced `build/cardputer_m5gfx_demo_suite.bin`.

### What didn't work
- Initial compilation errors caught and fixed during bring-up:
  - Ambiguous `LovyanGFX` type due to an incorrect forward declaration (fixed by including the proper header and using `lgfx::v1::LovyanGFX`).
  - `KeyPos` visibility issue (fixed by making the struct public).

### What I learned
- The M5GFX headers use an `lgfx::v1` namespace pattern; forward declaring `lgfx::LovyanGFX` is a footgun.

### What was tricky to build
- Making the UI/widget layer type-safe without accidentally shadowing library types (namespace collisions are surprisingly easy here).

### What warrants a second pair of eyes
- Confirm that the chosen `lgfx::v1::LovyanGFX` type annotation is consistent with other code in this repo and won’t make later integration (sprites, overlays) awkward.

### What should be done in the future
- Add a minimal “scene switcher” abstraction before implementing A1/B2/E1 so the launcher remains clean.

### Code review instructions
- Rebuild from scratch and confirm no local-only state is required:
  - `idf.py fullclean && idf.py build`

## Step 3: Commit the compile-ready baseline

This step records the point where the demo-suite scaffold first compiled successfully and was committed, so subsequent work (A1/B2/E1/B3) can build on a known-good baseline.

**Commit:** `4a751b7` — "✨ Cardputer: add M5GFX demo-suite scaffold"

### What I did
- Committed the new project (`0022-cardputer-m5gfx-demo-suite`) plus the `0021-M5-GFX-DEMO` ticket workspace updates.

### Why
- The fastest way to iterate is to lock in a working baseline early, then layer scenarios on top without mixing in unrelated changes.

### What worked
- Commit includes a successful `idf.py build` baseline.

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- Keeping the commit limited to the demo-suite + its ticket docs, because the repo working tree contained unrelated modifications.

### What warrants a second pair of eyes
- Whether committing the full ticket workspace docs (analysis + design + diaries) in the same baseline commit is desirable, or if future tickets prefer a docs-only commit separate from the code scaffold.

### What should be done in the future
- Keep commits per scenario small (A1, B2, E1, B3) so regressions are easy to bisect.

### Code review instructions
- Start with `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` (menu + wiring), then:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
