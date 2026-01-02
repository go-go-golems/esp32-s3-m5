---
Title: LVGL demo menu + screen architecture (Cardputer)
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: Screen manager integration point
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp
      Note: Keypad indev delivery into LVGL
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T23:11:30.777378277-05:00
WhatFor: ""
WhenToUse: ""
---


# LVGL demo menu + screen architecture (Cardputer)

## Executive Summary

The firmware currently boots into a single LVGL screen. To make it a usable “demo catalog”, we will add a menu screen that lets you choose between multiple demos and provides a consistent way to return to the menu from any demo. The first new demo will be a “premium-feel” Pomodoro timer screen (big arc ring + big time + keyboard controls).

The architecture stays intentionally lightweight: a tiny screen manager switches between LVGL screens; each screen is built by a function and installs its own key handler. A single shared LVGL group is used for keypad focus and key event routing.

## Problem Statement

We need:
- A menu to choose between demos on-device, using only the Cardputer keyboard.
- A Pomodoro example that demonstrates higher-quality LVGL UI (smooth-ish updates, `lv_arc`, big typography).
- A clean structure so adding future demos doesn’t turn `app_main.cpp` into a monolith.

Constraints:
- Display is 240×135 (Cardputer) and is rendered via M5GFX (RGB565).
- Input is keyboard-only, delivered via our `lvgl_port_cardputer_kb` keypad indev.
- We want to avoid an over-engineered “scene framework”.

## Proposed Solution

### Lightweight screen manager

Add a small “demo manager” that:
- owns the demo list (names + factory functions),
- switches screens via `lv_scr_load(new_screen)`,
- preserves a single shared `lv_group_t` and assigns it to the keypad indev, and
- provides a consistent way to return to the menu (key = `Esc`).

Represent demos as an enum + functions (not polymorphic classes):
- `Menu`
- `Basics` (existing textarea/button demo)
- `Pomodoro`

### Shared keypad focus group

Key routing in LVGL works best when the keypad indev is assigned to a group and a focusable object is focused. The design uses:
- one `lv_group_t* group` created at boot,
- `lv_indev_set_group(kb_indev, group)`,
- each screen decides what objects to add to the group and which one to focus:
  - `Menu`: root container (handles keys directly)
  - `Basics`: textarea + button (typing and enter-to-click both work)
  - `Pomodoro`: root container (handles keys directly)

### Menu screen (keyboard-only)

UI:
- Title: “LVGL Demos”
- Menu items: “Basics”, “Pomodoro”
- Hint line: `↑/↓ select  Enter open`

Controls:
- `Up/Down`: change selection
- `Enter` or `Space`: open selected demo

### Pomodoro screen (premium Cardputer-friendly)

UI goals:
- Big ring arc (110×110) centered
- Big time label (MM:SS), ideally `lv_font_montserrat_32`
- Status label: RUNNING / PAUSED / DONE
- Hint line with controls

Controls:
- `Space` / `Enter`: start/pause
- `R` / `Backspace`: reset
- `[` / `]`: -1 / +1 minute (only when paused)
- `Esc`: return to menu

Timing:
- LVGL timer every 50ms to update the arc smoothly-ish
- Label updates only on second boundaries (reduces flicker/work)
- Clamp `dt` to 500ms so a busy loop doesn’t create a huge time jump

## Design Decisions

### Use a single `lv_group_t` for the whole app

Rationale:
- Avoid group leaks and keep focus behavior predictable.
- Keep “key event routing” as a single place to reason about.

### Use `Esc` as “return to menu” everywhere

Rationale:
- Consistent navigation affordance.
- Avoids conflicting with Pomodoro’s requested Backspace reset.

### Use LVGL Kconfig fonts (no custom font assets yet)

Rationale:
- Fast iteration and minimal asset pipeline.
- We can revisit custom fonts once we have multiple screens and want a consistent visual identity.

## Alternatives Considered

### Use LVGL `lv_list` with focusable buttons

Pros:
- Built-in focus and click semantics.

Cons:
- It’s easy to get into “focus weirdness” depending on styles and default group handling.

Decision: implement a simple menu selection model and explicit highlight styling.

### Build a formal scene framework (classes, virtual methods)

Pros:
- Clear separation.

Cons:
- Too much boilerplate for 2–3 demos.

Decision: use plain functions + structs; refactor later only if demo count grows.

## Implementation Plan

1. Add a `demo_manager` module:
   - screen enum and demo registry
   - `load_menu()`, `load_basics()`, `load_pomodoro()`
2. Refactor `app_main.cpp` to:
   - initialize the group once
   - load the menu as the initial screen
3. Implement menu UI + key handler
4. Implement Pomodoro UI + key handler + timer
5. Enable required fonts in `sdkconfig.defaults` (`montserrat_32`, `montserrat_12`)
6. Build; flash as needed; update diary + tasks; commit

## Open Questions

- Do we want `Del` / `Esc` to always return to menu, or should Basics keep `Del` as backspace inside the textarea only? (Current plan: `Esc` returns to menu; `bksp` remains backspace.)
- If colors appear swapped on the arc/time label, which knob do we standardize on: `M5GFX::setSwapBytes(true)` or LVGL `LV_COLOR_16_SWAP`?

## References

- Firmware: `esp32-s3-m5/0025-cardputer-lvgl-demo`
- LVGL<->M5GFX port reference: `esp32-s3-m5/imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp`
- Ticket analysis plan: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md`
