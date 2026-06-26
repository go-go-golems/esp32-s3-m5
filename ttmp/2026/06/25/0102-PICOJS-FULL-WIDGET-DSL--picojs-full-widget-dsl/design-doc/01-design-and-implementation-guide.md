---
Title: Design and Implementation Guide
Ticket: 0102-PICOJS-FULL-WIDGET-DSL
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
      Note: Calc
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Widget/key behavior used by core app examples
    - Path: ttmp/2026/06/25/0102-PICOJS-FULL-WIDGET-DSL--picojs-full-widget-dsl/scripts/01-core-apps-widget-probe.py
      Note: Hardware validation probe
ExternalSources: []
Summary: Staged plan for full PicoJS widget DSL parity and current core-app implementation.
LastUpdated: 2026-06-25T23:59:00-07:00
WhatFor: Guide future implementation of richer PicoJS widgets and devkit app parity.
WhenToUse: Read before adding keypad, forms, editor widgets, or additional core apps.
---


# Design and Implementation Guide

## Executive Summary

The full PicoJS widget DSL remains a staged effort. This first implementation creates concrete core apps that exercise the currently available primitives and expose gaps for the next widget classes.

Implemented apps are `calc`, `settings`, and `notes`. They validate character input, callback-driven menu mutation, and simple editable text. They do not yet provide first-class keypad, form, toggle, or editor widgets.

## Problem Statement

The imported PicoOS devkit includes richer app patterns than the firmware runtime currently supports. Implementing every widget abstraction at once would be risky, so the firmware needs small acceptance apps that can drive the DSL incrementally.

The target is practical parity: make useful apps work on the device first, then extract repeated patterns into stable widgets.

## Current Implementation

Implemented in commit `d2069f05fbc6f7eff846e30d5b35ea8f6c0c5aeb`:

- `calc`: accepts arithmetic characters, evaluates on Enter, clears on Delete/Backspace.
- `settings`: menu-based settings rows; Enter toggles `echo` and returns Home from `back home`.
- `notes`: simple single-line text entry with Backspace/Delete handling.
- `picojs load` accepts `calc`, `settings`, and `notes`.
- Physical keyboard conversion maps Backspace to semantic `backspace` for app key dispatch.
- Snake down-key movement was corrected while touching app key handling.

## Design Decisions

### Decision: Build apps before formalizing all widgets

- **Context:** The full DSL includes keypad/buttons, settings rows/toggles, editors, tables, sparklines, and richer events.
- **Decision:** ship representative built-in apps first, then extract stable widget APIs from observed repetition.
- **Rationale:** hardware feedback catches layout/input issues faster than a large speculative API.
- **Status:** accepted for this phase.

### Decision: Treat calculator eval as trusted built-in prototype behavior

- **Context:** `calc` uses `OS.eval(expr)` for simple arithmetic.
- **Decision:** keep it as an internal built-in demonstration for now.
- **Consequence:** do not expose it as a general security boundary without a separate review.

## Implementation Plan

Done:

1. Add `calc`, `settings`, and `notes` built-in sources.
2. Register those apps in PicoOS.
3. Extend `picojs load` app ID validation.
4. Add Backspace key mapping for app dispatch.
5. Validate with `scripts/01-core-apps-widget-probe.py`.

Future phases:

1. Promote repeated settings-row behavior into a first-class form/toggle widget.
2. Add keypad/button widgets for calculator-like apps.
3. Add a multi-line editor/viewer for notes.
4. Polish existing spark/table/progress rendering and events.
5. Revisit devkit apps after `music` and `chat` are no longer intentionally skipped.

## Testing and Validation

Run:

```bash
source ~/esp/esp-idf-5.4.2/export.sh
cd 0102-esp32-p4-visual-quickjs-repl
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
cd ..
ttmp/2026/06/25/0102-PICOJS-FULL-WIDGET-DSL--picojs-full-widget-dsl/scripts/01-core-apps-widget-probe.py
```

Expected result: `CORE_APPS_WIDGET_PROBE PASS`.

Probe coverage:

- `calc`: `1+2` → `= 3`.
- `settings`: toggle selected `echo` row to `echo on`.
- `notes`: type `Hi` into the visible note line.

## Open Questions

- Should settings values persist in app-local state, global config, or both?
- Should notes store text in NVS, filesystem, or remain volatile until multi-app state exists?
- What sandbox restrictions should apply to `OS.eval`?

## References

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`
- `components/picojs_runtime/picojs_runtime.cpp`
- `ttmp/2026/06/25/0102-PICOJS-FULL-WIDGET-DSL--picojs-full-widget-dsl/scripts/01-core-apps-widget-probe.py`
