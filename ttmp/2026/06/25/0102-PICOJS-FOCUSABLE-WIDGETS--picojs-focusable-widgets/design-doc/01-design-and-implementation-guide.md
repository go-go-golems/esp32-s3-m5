---
Title: Design and Implementation Guide
Ticket: 0102-PICOJS-FOCUSABLE-WIDGETS
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
    - Path: components/picojs_runtime/include/picojs_runtime.h
      Note: Launch request API declaration
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Focusable widget methods
    - Path: components/picoos_core/picoos_core.cpp
      Note: Consumes PicoJS launch requests after key dispatch
    - Path: ttmp/2026/06/25/0102-PICOJS-FOCUSABLE-WIDGETS--picojs-focusable-widgets/scripts/01-focusable-menu-probe.py
      Note: Hardware validation probe
ExternalSources: []
Summary: Design for focusable PicoJS menus/lists and callback-driven launch behavior.
LastUpdated: 2026-06-25T23:59:00-07:00
WhatFor: Guide future work on focusable widgets and keyboard-driven PicoJS apps.
WhenToUse: Read before changing generic widget selection, onPick callbacks, or launch handoff plumbing.
---


# Design and Implementation Guide

## Executive Summary

Focusable widgets are the first interaction layer above static PicoJS rendering. The initial implementation adds selection movement, clamped selection, selected-value lookup, and `onPick(fn)` callbacks for generic item widgets such as menus and lists.

The design deliberately keeps QuickJS runtime code separate from PicoOS supervisor code. JavaScript can request `OS.launch(name)`, but the runtime records that request and the supervisor consumes it after JS key dispatch returns.

## Problem Statement

The previous PicoJS widgets could render menu-like data but could not act like app controls. Home needed arrow navigation and Enter-to-launch, and future apps need the same mechanics for settings rows, lists, buttons, and forms.

The runtime also cannot safely let arbitrary native JS callbacks mutate supervisor state directly because QuickJS must run on the `qjs_service` task while app state belongs to `picoos_core`.

## Current Implementation

Implemented in commit `d2069f05fbc6f7eff846e30d5b35ea8f6c0c5aeb`:

- `Widget.select(index)` clamps selection.
- `Widget.move(dx, dy)` moves by item or grid row.
- `Widget.value()` returns the selected item label.
- `Widget.pick()` executes the stored callback.
- `Widget.onPick(fn)` stores/replaces a JS callback.
- `OS.launch(name)` records a bounded launch request.
- `picojs_runtime_take_launch_request(...)` lets `picoos_core` consume and clear the request.

## Design Decisions

### Decision: Deferred launch request instead of direct supervisor calls

- **Context:** `onPick()` runs while dispatching a key to QuickJS.
- **Options:** call PicoOS launch directly from the JS binding, return app ID to C++ caller, or store a runtime launch request.
- **Decision:** store a bounded launch request in `picojs_runtime` and let `picoos_core` consume it.
- **Rationale:** preserves task/layer ownership and makes launch intent explicit.
- **Status:** accepted for this phase.

### Decision: Add behavior to generic widgets first

- **Context:** Home, settings, and simple lists need common selection behavior.
- **Decision:** implement movement/pick behavior on the existing generic widget structure before adding specialized classes.
- **Consequence:** fast progress with a smaller API, but future work still needs first-class disabled items, scrolling, and widget-specific event contracts.

## Implementation Plan

Done:

1. Add callback storage and cleanup to `GenericWidget`.
2. Add item-count, item-label, and selection-clamp helpers.
3. Expose `.move`, `.pick`, `.value`, and `.onPick` through native QuickJS methods.
4. Add launch request API in `picojs_runtime.h`/`.cpp`.
5. Consume launch requests in `picoos_core` after JS key dispatch.
6. Validate with `scripts/01-focusable-menu-probe.py`.

Future phases:

1. Add disabled-item metadata and navigation skipping.
2. Add scroll-window rendering for menus/lists larger than the visible region.
3. Add focus/blur/change event names if app code needs more than `onPick`.

## Testing and Validation

Run:

```bash
source ~/esp/esp-idf-5.4.2/export.sh
cd 0102-esp32-p4-visual-quickjs-repl
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
cd ..
ttmp/2026/06/25/0102-PICOJS-FOCUSABLE-WIDGETS--picojs-focusable-widgets/scripts/01-focusable-menu-probe.py
```

Expected result: `FOCUSABLE_MENU_PROBE PASS`.

## Open Questions

- Should launch-request capacity be a shared `PICOOS_MAX_APP_ID_LEN` constant?
- Should disabled items be represented in widget item objects before adding forms/buttons?

## References

- `components/picojs_runtime/picojs_runtime.cpp`
- `components/picojs_runtime/include/picojs_runtime.h`
- `components/picoos_core/picoos_core.cpp`
- `ttmp/2026/06/25/0102-PICOJS-FOCUSABLE-WIDGETS--picojs-focusable-widgets/scripts/01-focusable-menu-probe.py`
