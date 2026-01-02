---
Title: Diary
Ticket: 0023-CARDPUTER-KB-SCANCODES
Status: active
Topics:
    - cardputer
    - keyboard
    - scancodes
    - esp32s3
    - esp-idf
    - debugging
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Vendor matrix scan ground truth
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Raw scan_state/in_mask logging and IN0/IN1 variant handling
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Existing scanner patterns we may reuse
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T21:41:57.721916763-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Step-by-step implementation diary for the `0023-cardputer-kb-scancode-calibrator` firmware (a standalone tool firmware for discovering Cardputer keyboard matrix scancodes and Fn-combos).

## Context

- Ticket workspace: `esp32-s3-m5/ttmp/2026/01/02/0023-CARDPUTER-KB-SCANCODES--cardputer-keyboard-scancode-calibration-firmware/`
- Primary design/plan: `analysis/01-keyboard-scancode-calibration-firmware.md`
- Key prior art:
  - `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c` (raw scan logging)
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` (scanner patterns used elsewhere)
  - `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp` (vendor scan ground truth)

## Quick Reference

N/A (this diary is prose-first; copy/paste snippets live in analysis docs).

## Usage Examples

N/A (see per-step “Code review instructions”).

## Related

- Analysis/plan: `esp32-s3-m5/ttmp/2026/01/02/0023-CARDPUTER-KB-SCANCODES--cardputer-keyboard-scancode-calibration-firmware/analysis/01-keyboard-scancode-calibration-firmware.md`

## Step 1: Create the calibrator firmware scaffold + live matrix view

This step bootstraps a new, dedicated ESP-IDF firmware project for keyboard scancode calibration. The immediate outcome is a compile-ready program that brings up the Cardputer display and shows a live 4×14 matrix view, highlighting currently pressed physical keys and printing the corresponding vendor-style `keyNum` list to the serial log on change. This establishes the “ground truth” scan pipeline we can build the interactive calibration wizard on top of.

**Commit (code):** 1fc1f77 — "✨ Cardputer: add keyboard scancode calibrator scaffold"

### What I did
- Created new project: `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator`
- Implemented a raw matrix scanner with:
  - multi-key support (collect all asserted input bits across scan states)
  - alt IN0/IN1 autodetect (switches from `{13,15,...}` to `{1,2,...}` when activity is observed)
  - pressed-set outputs: `std::vector<KeyPos>` + `std::vector<uint8_t keyNum>`
- Implemented a minimal UI:
  - 4×14 grid labeled with the vendor legend
  - highlights pressed keys
  - prints `pressed: [keynums...]` in the footer and logs on change
- Added a `build.sh` helper consistent with other Cardputer projects (tmux-friendly).

### Why
- We need a standalone “measurement tool” firmware that does not depend on guessed semantic mappings (e.g. treating `W/S` as arrows). A live matrix display makes it obvious what the hardware is actually doing.

### What worked
- `./build.sh build` succeeded and produced a flashable binary.

### What didn't work
- N/A (baseline bring-up step).

### What I learned
- The simplest stable scancode representation for our codebase is vendor `keyNum` derived from `KeyPos` (4×14), not characters.

### What was tricky to build
- Cardputer has a known IN0/IN1 wiring variance; autodetect must only switch on observed activity, otherwise both pinsets read “idle high” and are indistinguishable.

### What warrants a second pair of eyes
- Validate the scan mapping math (`scan_state` + `in_mask` → `KeyPos`) against vendor implementation and physical reality on multiple devices.

### What should be done in the future
- Implement the actual calibration wizard (prompt → stable capture → confirm/redo/skip) and config export (JSON + header snippet).

### Code review instructions
- Start in `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/kb_scan.cpp`:
  - `KbScanner::scan()` mapping and autodetect behavior
- Then review the UI wiring in `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp`.
