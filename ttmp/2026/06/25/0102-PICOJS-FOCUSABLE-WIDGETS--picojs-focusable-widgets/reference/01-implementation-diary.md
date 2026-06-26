---
Title: Implementation Diary
Ticket: 0102-PICOJS-FOCUSABLE-WIDGETS
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
Summary: "Implemented focusable PicoJS widget movement, selection, and onPick launch callbacks."
LastUpdated: 2026-06-25T23:59:00-07:00
WhatFor: "Chronological implementation notes for focusable PicoJS widgets."
WhenToUse: "Use when extending menu/list focus semantics or debugging JS key dispatch."
---

# Diary

## Goal

Capture the first firmware-native focusable widget pass: menu selection state, movement helpers, `onPick()` callbacks, and launch-request handoff from QuickJS apps into the PicoOS supervisor.

## Step 1: Focusable menu callbacks and PicoOS launch handoff

This step turns generic PicoJS widgets from mostly static render objects into small interactive controls. Menus can now move selection, clamp the selected item, expose the selected value, and execute `onPick()` callbacks on the QuickJS task.

The important integration point is that JavaScript callbacks do not call the supervisor directly. `OS.launch(name)` records a bounded launch request in `picojs_runtime`, and `picoos_core` consumes that request after key dispatch to switch apps safely from supervisor code.

### Prompt Context

**User prompt (verbatim):** "create and implement tickets 1, 2, and 3, and test independently without involving me unless necessary"

**Assistant interpretation:** Create the first three child tickets from the PicoOS roadmap, implement the focusable-widget and launcher/app groundwork, and validate on hardware without requiring user interaction.

**Inferred user intent:** Move from a proof-of-concept PicoJS renderer toward a daily-usable app launcher with keyboard navigation and launchable apps.

**Commit (code):** d2069f05fbc6f7eff846e30d5b35ea8f6c0c5aeb — "0102: add focusable launcher widgets and core apps"

### What I did
- Added focus storage for widget `onPick` callbacks in `components/picojs_runtime/picojs_runtime.cpp`.
- Added selection helpers for item count, item label, clamped selection, movement, value lookup, and callback invocation.
- Added JS methods `.move(dx, dy)`, `.pick()`, `.value()`, and a real `.onPick(fn)` implementation.
- Added `picojs_runtime_take_launch_request(...)` to publish deferred app-launch requests.
- Updated `components/picoos_core/picoos_core.cpp` to consume launch requests after `picojs_runtime_key_js(...)`.
- Added `scripts/01-focusable-menu-probe.py` and ran it against flashed ESP32-P4 firmware.

### Why
- The home launcher needs arrow-key focus and Enter-to-launch behavior.
- QuickJS callbacks must remain on the QuickJS service task, while app switching should remain supervisor-owned.
- A bounded launch-request mailbox avoids direct PicoOS coupling inside the runtime component.

### What worked
- `picoos launcher` renders with `select repl` initially.
- `picoos key right` moves selection to `hello`.
- `picoos key enter` launches `hello` through `onPick()` → `OS.launch()` → supervisor handoff.
- Two down movements from home select and launch `calc`.
- Probe result: `FOCUSABLE_MENU_PROBE PASS`.

### What didn't work
- No hardware validation failures remained after implementation.
- One compile failure occurred while improving `/apps` rendering, unrelated to widget callbacks: `error: '%-12s' directive output may be truncated`. It was fixed by using bounded precision in the format string.

### What I learned
- The existing generic widget path was enough for an initial menu/list behavior layer; it did not require a new widget class yet.
- Launching from JavaScript is safest when treated as an intent that the supervisor consumes after JS dispatch returns.

### What was tricky to build
- The main sharp edge was keeping QuickJS ownership and supervisor ownership separate. Calling PicoOS launch directly from native JS bindings would cross layers and make task ownership harder to reason about, so the runtime now records `launch_request[24]` and exposes a take/clear API.
- Selection also needed clamping after every mutation so sparse or single-item menus never expose out-of-range labels.

### What warrants a second pair of eyes
- Review JSValue lifetime handling for `onPick` replacement and cleanup in `picojs_runtime.cpp`.
- Review whether `launch_request[24]` is long enough for future app IDs or should become a shared constant.

### What should be done in the future
- Add scroll-window rendering for menus longer than the 40x20 display can show.
- Add disabled-item skipping and richer focus events when the full widget DSL matures.

### Code review instructions
- Start with `components/picojs_runtime/picojs_runtime.cpp`, especially `js_widget_on_pick`, `js_widget_move`, `js_widget_pick`, and `js_os_launch`.
- Then review `components/picoos_core/picoos_core.cpp` key handling after `picojs_runtime_key_js`.
- Validate with: `idf.py build`, flash, then run `ttmp/2026/06/25/0102-PICOJS-FOCUSABLE-WIDGETS--picojs-focusable-widgets/scripts/01-focusable-menu-probe.py`.

### Technical details
- Build command: `source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Flash command: `idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash`.
- Probe command: `ttmp/2026/06/25/0102-PICOJS-FOCUSABLE-WIDGETS--picojs-focusable-widgets/scripts/01-focusable-menu-probe.py`.
