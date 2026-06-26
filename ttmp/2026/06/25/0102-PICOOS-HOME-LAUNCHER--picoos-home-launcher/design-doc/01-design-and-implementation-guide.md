---
Title: Design and Implementation Guide
Ticket: 0102-PICOOS-HOME-LAUNCHER
Status: active
Topics:
    - esp32-p4
    - picojs
    - picoos
    - quickjs
    - firmware
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: Home launcher app source
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Menu focus and launch callbacks used by Home
    - Path: ttmp/2026/06/25/0102-PICOOS-HOME-LAUNCHER--picoos-home-launcher/scripts/01-home-launcher-probe.py
      Note: Hardware validation probe
ExternalSources: []
Summary: Design for the firmware-native 40x20 PicoOS home launcher.
LastUpdated: 2026-06-25T23:59:00-07:00
WhatFor: Guide launcher layout, app ordering, and app-registry visibility work.
WhenToUse: Read before changing the Home app, /apps display, or launcher app list.
---


# Design and Implementation Guide

## Executive Summary

The PicoOS home launcher is now a compact firmware-native app that fits the PicoCalc display. It shows a battery/clock header, a two-column focusable app grid, and a one-line footer with the selected app and launch hint.

This ticket intentionally prioritizes daily-use core apps: `repl`, `hello`, `sysmon`, `snake`, `calc`, `settings`, and `notes`. `music` and `chat` are omitted for now as requested.

## Problem Statement

The imported devkit home design assumes a larger 40x30 environment and was clipped on the ESP32-P4 firmware display. In addition, the visual `/apps` command previously showed only the first five registered apps, hiding `sysmon`, `snake`, and future entries.

The launcher needs to make installed apps discoverable, navigable from keyboard input, and launchable without requiring console commands.

## Current Implementation

Implemented in commit `d2069f05fbc6f7eff846e30d5b35ea8f6c0c5aeb`:

- `kPicoJsHomeSource` renders a 40x20-friendly launcher.
- Home app entries: `repl`, `hello`, `sysmon`, `snake`, `calc`, `settings`, `notes`.
- Arrow keys move through the grid.
- Enter launches the selected app through `onPick()` and `OS.launch()`.
- Escape launches `repl`.
- `/apps` now shows all current registry entries in compact two-column LCD output.

## Design Decisions

### Decision: Home grid excludes diagnostics demos

- **Context:** `dashboard` and `interactive` remain registered but are older demo apps.
- **Decision:** keep them visible in `/apps`, but omit them from the daily-use Home grid.
- **Rationale:** Home should stay small and focused; `/apps` remains the full registry view.
- **Status:** accepted for this phase.

### Decision: Two-column launcher layout

- **Context:** The firmware text backend is 40 columns by 20 rows.
- **Decision:** use a two-column grid with a short footer.
- **Rationale:** seven apps fit without scrolling and remain readable.
- **Consequence:** future app growth needs paging or categories.

## Implementation Plan

Done:

1. Rewrite `kPicoJsHomeSource` for 40x20.
2. Wire app selection to focusable menu methods.
3. Add launch callbacks using `OS.launch`.
4. Register `calc`, `settings`, and `notes`.
5. Fix `/apps` LCD truncation.
6. Validate with `scripts/01-home-launcher-probe.py`.

Future phases:

1. Add disabled/missing app states once registry metadata supports them.
2. Add categories or paging when the app list exceeds one screen.
3. Add persistent last-selected app after multi-app state support lands.

## Testing and Validation

Run:

```bash
source ~/esp/esp-idf-5.4.2/export.sh
cd 0102-esp32-p4-visual-quickjs-repl
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
cd ..
ttmp/2026/06/25/0102-PICOOS-HOME-LAUNCHER--picoos-home-launcher/scripts/01-home-launcher-probe.py
```

Expected result: `HOME_LAUNCHER_PROBE PASS`.

Manual smoke commands:

- `screen eval /apps`
- `screen dump`
- `picoos launcher`
- `picoos key right`
- `picoos key enter`
- `picoos status`

## Open Questions

- Should Home eventually include `dashboard` and `interactive`, or should those become hidden developer tools?
- Should app ordering be static or derived from registry metadata?

## References

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`
- `components/picojs_runtime/picojs_runtime.cpp`
- `components/picoos_core/picoos_core.cpp`
- `ttmp/2026/06/25/0102-PICOOS-HOME-LAUNCHER--picoos-home-launcher/scripts/01-home-launcher-probe.py`
