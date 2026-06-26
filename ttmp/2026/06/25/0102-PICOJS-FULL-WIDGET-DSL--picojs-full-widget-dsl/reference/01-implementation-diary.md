---
Title: Implementation Diary
Ticket: 0102-PICOJS-FULL-WIDGET-DSL
Status: active
Topics:
    - esp32-p4
    - picojs
    - picoos
    - quickjs
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Added first core apps exercising the expanded PicoJS widget/key DSL."
LastUpdated: 2026-06-25T23:59:00-07:00
WhatFor: "Chronological notes for the staged full-widget-DSL implementation."
WhenToUse: "Use when extending the PicoJS DSL beyond static panels, text, gauges, and basic menus."
---

# Diary

## Goal

Capture the first staged implementation for the broader widget DSL ticket. This is not the complete full DSL yet; it adds enough interaction primitives and app examples to validate text entry, menu/list selection, value updates, and simple callback-driven state changes on firmware.

## Step 1: Core app examples for calculator, settings, and notes

This step adds three small built-in PicoJS apps that exercise different parts of the emerging DSL. `calc` validates character-key dispatch and display updates, `settings` validates menu selection plus callback mutation, and `notes` validates printable text entry and backspace/delete handling.

The goal was to create concrete firmware-native examples before expanding the formal DSL surface further.

### Prompt Context

**User prompt (verbatim):** (same as 0102-PICOJS-FOCUSABLE-WIDGETS Step 1)

**Assistant interpretation:** Start the full widget DSL child ticket by adding practical built-in apps that can be used as acceptance tests for future widget work.

**Inferred user intent:** Increase compatibility with the imported PicoOS devkit while prioritizing useful core apps and deferring `music`/`chat`.

**Commit (code):** d2069f05fbc6f7eff846e30d5b35ea8f6c0c5aeb — "0102: add focusable launcher widgets and core apps"

### What I did
- Added built-in PicoJS sources for `calc`, `settings`, and `notes` in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`.
- Registered those apps with PicoOS.
- Extended `picojs load` validation and source selection for `calc`, `settings`, and `notes`.
- Added backspace mapping for the physical keyboard path so text-entry apps can delete characters.
- Fixed the existing snake demo's down-key movement so down increments `y` instead of decrementing it.
- Added and ran `scripts/01-core-apps-widget-probe.py`.

### Why
- The full devkit widget DSL is still larger than one patch, but these apps give immediate acceptance cases for menu callbacks, text entry, simple forms/settings behavior, and evaluator output.
- `calc`, `settings`, and `notes` were explicitly in the next-app roadmap, while `music` and `chat` are intentionally skipped for now.

### What worked
- `calc` accepts `1+2`, evaluates on Enter, and displays `= 3`.
- `settings` moves selection and toggles `echo off` to `echo on` on Enter.
- `notes` accepts printable keys and displays `type notes hereHi` after `H` and `i`.
- Probe result: `CORE_APPS_WIDGET_PROBE PASS`.

### What didn't work
- No remaining failures after the final build/flash/probe pass.
- The implementation intentionally does not claim full widget DSL parity yet; keypad/button/forms/editor widgets are still future work.

### What I learned
- The current `App.dispatch()` plus generic widget API can support useful small apps before specialized widget classes exist.
- Hardware validation through console-routed `picoos key ...` is enough to test app logic independently of the still-flaky physical keyboard path.

### What was tricky to build
- The apps must stay compact enough for 40x20 while still being good DSL tests. The implementation uses simple `Panel`, `Text`, `Menu`, and `footer` composition instead of adding a large new layout engine in this step.
- `OS.eval()` is currently a placeholder-style native eval bridge and is only used by the calculator for simple arithmetic; it should be treated as a prototype surface, not a hardened scripting sandbox.

### What warrants a second pair of eyes
- Review the calculator `OS.eval(expr)` surface for future security/sandbox constraints before exposing it beyond trusted built-ins.
- Review whether `settings` should persist state across launches once multi-app runtime state lands.

### What should be done in the future
- Implement proper button/keypad widgets.
- Implement form rows/toggles as first-class widgets instead of encoding settings as menu labels.
- Add an editor/viewer widget for notes with cursor movement and multi-line wrapping.

### Code review instructions
- Start with `kPicoJsCalcSource`, `kPicoJsSettingsSource`, and `kPicoJsNotesSource` in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`.
- Review key routing in `key_to_picoos_token` and the `picojs load` command validation.
- Validate with `ttmp/2026/06/25/0102-PICOJS-FULL-WIDGET-DSL--picojs-full-widget-dsl/scripts/01-core-apps-widget-probe.py`.

### Technical details
- Build command: `source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Probe assertions: calculator `= 3`, settings `echo on`, notes `type notes hereHi`.
