---
Title: Implementation Diary
Ticket: 0102-PICOOS-HOME-LAUNCHER
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
Summary: "Implemented a 40x20 PicoOS home launcher with focusable app menu and full /apps visibility."
LastUpdated: 2026-06-25T23:59:00-07:00
WhatFor: "Chronological notes for the PicoOS home launcher implementation."
WhenToUse: "Use when changing launcher layout, registry visibility, or app launch behavior."
---

# Diary

## Goal

Capture the first usable PicoOS home launcher pass: a compact 40x20 layout, focusable app menu, app launch wiring, and registry visibility for all currently registered apps while intentionally skipping `music` and `chat`.

## Step 1: 40x20 launcher and complete visible app registry

This step replaces the earlier nested/cut-off devkit-like home rendering with a compact launcher designed for the PicoCalc display. It shows clock/battery context and a two-column app grid that fits within 40 columns and 20 rows.

It also fixes the visual REPL `/apps` command so the LCD output lists all registered apps instead of silently truncating after five entries.

### Prompt Context

**User prompt (verbatim):** (same as 0102-PICOJS-FOCUSABLE-WIDGETS Step 1)

**Assistant interpretation:** Implement the home launcher child ticket from the roadmap and validate that apps can be found and launched from the device UI.

**Inferred user intent:** Make PicoOS feel like a small app shell rather than a set of console-only demos.

**Commit (code):** d2069f05fbc6f7eff846e30d5b35ea8f6c0c5aeb — "0102: add focusable launcher widgets and core apps"

### What I did
- Rewrote the built-in `home` app source in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`.
- Added launcher entries for `repl`, `hello`, `sysmon`, `snake`, `calc`, `settings`, and `notes`.
- Bound arrow keys to `menu.move(...)`, Enter to `menu.pick()`, and Escape to `OS.launch('repl')`.
- Added `calc`, `settings`, and `notes` to the PicoOS registry.
- Updated `/apps` LCD rendering to show all ten current registered apps in two columns.
- Added and ran `scripts/01-home-launcher-probe.py`.

### Why
- The imported devkit home app targets a larger 40x30 emulator and was visibly clipped on the firmware's 40x20 display.
- The launcher should expose the apps the user actually wants next: `repl`, `hello`, `sysmon`, `snake`, `calc`, `settings`, and `notes`; `music` and `chat` remain intentionally omitted for now.
- `/apps` is an important device-side discovery command and should not hide registered apps.

### What worked
- `/apps` now reports `COUNT=10` and visibly lists `home`, `repl`, `hello`, `dashboard`, `interactive`, `sysmon`, `snake`, `calc`, `settings`, and `notes`.
- `picoos launcher` renders the full home app grid on rows 4–7 with the footer on row 19.
- Selecting `hello` and pressing Enter launches the hello app.
- Probe result: `HOME_LAUNCHER_PROBE PASS`.

### What didn't work
- The first `/apps` two-column implementation used unrestricted `%s` formatting and failed ESP-IDF's `-Werror=format-truncation` check. The final version uses `%-11.11s` for bounded columns.

### What I learned
- The 40x20 display can support a useful app launcher if the layout avoids deep nesting and reserves only one row for status/footer text.
- `/apps` output is more useful as a compact discovery view than as a verbose state table on the LCD.

### What was tricky to build
- The home launcher uses JavaScript widget callbacks but must ask the native supervisor to switch apps. That required the launch-request bridge implemented in the focusable-widget work.
- The display width is tight enough that even diagnostic `/apps` lines need bounded formatting to compile cleanly under ESP-IDF warnings-as-errors.

### What warrants a second pair of eyes
- Review whether `dashboard` and `interactive` should remain in `/apps` but absent from the user-facing home grid.
- Review app ordering and whether `repl` should stay first or be moved to a system/action row later.

### What should be done in the future
- Add disabled styling for apps not installed on a given build.
- Add paging if the registry grows beyond what the 40x20 launcher can show.

### Code review instructions
- Start in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` at `kPicoJsHomeSource` and the PicoOS app registration block.
- Review `/apps` handling in `evaluate_visual_input`.
- Validate with `ttmp/2026/06/25/0102-PICOOS-HOME-LAUNCHER--picoos-home-launcher/scripts/01-home-launcher-probe.py`.

### Technical details
- Build passed with binary size `0xe9670` and 77% of the 4 MB app partition free.
- Hardware probe used `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
