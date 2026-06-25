---
Title: Implementation Diary
Ticket: 0102-PICOJS-LAYOUT-WIDGETS
Status: active
Topics:
    - esp32-p4
    - quickjs
    - picocalc
    - visual-repl
    - javascript
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for Rows/cols layout, gauges, status bar, and dashboard-style rendering."
LastUpdated: 2026-06-25T15:30:00-07:00
WhatFor: "Chronological implementation notes for 0102 PicoJS Layout and Widgets."
WhenToUse: "Read before resuming this phase, before committing, and before hardware probe work."
---

# Diary

## Goal

This diary records the implementation of `0102-PICOJS-LAYOUT-WIDGETS`. The phase goal is to expand the native DSL from a single panel into useful dashboard layouts.

## Step 1: Create child ticket and phase guide

This step split the broad PicoJS device integration effort into a focused child ticket. The design guide defines the phase scope, console contract, validation strategy, and review risks so implementation can proceed in small commits.

The child ticket links back to the umbrella `0102-PICOJS-DEVICE-INTEGRATION` plan. Its work should remain narrow; later-phase features belong in their own child tickets unless the umbrella design is updated.

### Prompt Context

**User prompt (verbatim):** "alright, do it, and for each ticket write a design + implementation guide as usual (and upload to remarkable), then stsart working on tasks one by one, committing at appropriate intervals, and keeping a detailed diary."

**Assistant interpretation:** Create the phase tickets and their implementation guides, upload the documentation package to reMarkable, then begin work phase-by-phase with commits and diary updates.

**Inferred user intent:** Keep a complex firmware integration manageable, reviewable, and recoverable by splitting it into documented tickets with concrete validation gates.

**Commit (code):** N/A — ticket/documentation setup only.

### What I did

- Created child ticket `0102-PICOJS-LAYOUT-WIDGETS`.
- Added this implementation diary.
- Wrote `design-doc/01-design-and-implementation-guide.md`.
- Defined initial scope, non-goals, console contract, validation strategy, and risks.

### Why

- The umbrella task is too broad to implement safely as one monolithic ticket.
- This phase has its own validation target and can be reviewed independently.

### What worked

- The ticket workspace and document structure were created through `docmgr`.
- The phase guide now provides an implementation boundary and a stopping condition.

### What didn't work

- N/A for this documentation setup step.

### What I learned

- Splitting by console-observable vertical slices gives each child ticket a concrete pass/fail loop.

### What was tricky to build

- The main challenge is scope control: adjacent phases touch the same files, especially `app_main.cpp`, so commits must remain explicit and diaries must record which ticket owns each behavior.

### What warrants a second pair of eyes

- Whether the phase boundaries are still right after the first firmware build/probe results.

### What should be done in the future

- Implement the phase tasks and update this diary after each commit or failed validation attempt.

### Code review instructions

- Start with this ticket's design guide.
- Review only changes that are in scope for `0102-PICOJS-LAYOUT-WIDGETS`.
- Validate using the commands listed in the design guide.

### Technical details

Phase commands to keep in view:

- `picojs load dashboard`
- `picojs frame 1000`
- `picojs dump`


## Step 2: Add layout rows, gauges, and a dashboard app

I completed the first dashboard-oriented native DSL slice. The firmware PicoJS runtime now supports `App.layout(fn)` with synchronous `Layout.row()` and `Layout.col()` builder calls, binds panels to named layout regions, renders gauge widgets, and exposes a built-in `picojs load dashboard` program that can be validated entirely from the UART console.

This step keeps JavaScript callbacks out of the frame path for now. Gauge values support literal numbers and a small native source string (`"battery"`) that changes with frame count. That gives the ticket a deterministic dynamic value without calling QuickJS from the console task during `picojs frame`; later timer/callback work should move JS callback execution onto the QuickJS service task.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Continue with the next child ticket in the planned sequence, which is `0102-PICOJS-LAYOUT-WIDGETS`, using the same build/flash/probe, diary, commit, and reMarkable workflow.

**Inferred user intent:** Keep advancing through the device-side PicoJS phases without stopping after the minimal DSL slice.

**Commit (code):** Pending — code and docs changed in this step.

### What I did

- Extended `components/picojs_runtime/picojs_runtime.cpp`:
  - added `LayoutSegment` and `Rect` models
  - added `LayoutBuilder`
  - added `GaugeWidget`
  - added `App.layout(fn)`
  - added `Layout.row(size, id)` and `Layout.col(size, id)` through `JS_NewCFunctionMagic`
  - added layout recomputation and panel-to-region binding
  - added `Panel.gauge()`
  - added `Gauge.at(x, y)`, `Gauge.label(text)`, `Gauge.value(valueOrSource)`, `Gauge.width(n)`, and `Gauge.showPct()`
  - added text rendering inside each panel's inner region
  - added gauge rendering with percent text
- Extended `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`:
  - added embedded `kPicoJsDashboardSource`
  - allowed `picojs load dashboard`
  - updated console help text for `picojs load hello|dashboard`
- Added `scripts/01-layout-widgets-probe.py` for repeatable by-id UART validation.
- Built, flashed, and probed the ESP32-P4 over:

```text
/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
```

### Why

- The minimal DSL could render only one full-screen panel. Useful PicoCalc apps need named screen regions and compact status/dashboard widgets.
- Row/column layout and gauges are the next smallest vertical slice before more complex input, timers, and LCD frame rendering.
- The dashboard app provides a stable console snapshot that proves region binding, panel framing, centered text, gauges, and status bar output.

### What worked

- Build succeeded with ESP-IDF 5.4.2:

```text
0102-esp32-p4-visual-quickjs-repl.bin binary size 0xdfa30 bytes. Smallest app partition is 0x400000 bytes. 0x3205d0 bytes (78%) free.
```

- Flash succeeded on the ESP32-P4 by-id port.
- The layout/widgets probe passed:

```text
LAYOUT_WIDGETS_PROBE PASS [True, True, True, True, True, True, True, True]
```

- The dashboard dump showed distinct bar/body rows and gauge rendering:

```text
[00] PicoJS Dashboard
[01] +- system -----------------------------+
[03] |         ESP32-P4 native DSL          |
[05] |  batt [##############------] 73%     |
[07] |  heap [############--------] 62%     |
[19] dashboard native picojs
```

- A second frame updated the battery source deterministically:

```text
[05] |  batt [##############------] 74%     |
```

### What didn't work

- During implementation I temporarily replaced the forward declarations and native builder methods while adding `make_layout_object()`/`make_gauge_object()`. I restored the missing `js_*` methods before building.
- I avoided QuickJS callback-valued gauges in this ticket. Calling JS callbacks from `picojs_runtime_frame()` would currently run QuickJS on the console task, not the `qjs_service` task. That belongs in the frame/timer ticket where frame execution can be moved onto the QuickJS owner task.

### What I learned

- `JS_NewCFunctionMagic()` works for the row/col builder pattern in ESP-IDF C++ code and avoids the `JS_CFUNC_DEF` initializer issue from the previous ticket.
- The single-axis layout model is enough to prove dashboard-style apps: row 0 can be a command/status bar and the remaining body can be a framed region.
- Keeping dynamic gauge values native for now preserves the thread ownership invariant while still validating changing frame output.

### What was tricky to build

- The layout builder object is a synchronous stack-owned builder. That is safe for `app.layout(function (l) { ... })` only because the script uses it inside the callback. The object must not be stored by user scripts. This matches the ticket design decision and should be revisited if user scripts need persistent layout objects.
- Gauge width must fit inside the panel's inner rectangle. The renderer clamps width against available space and keeps the output ASCII-only for stable UART assertions.
- `picojs frame` currently runs rendering on the console task. That is acceptable for literal/native values, but not for JS callbacks. The next callback-heavy phases must respect QuickJS service task ownership.

### What warrants a second pair of eyes

- Whether native metric sources such as `"battery"` should remain string-based or become explicit APIs like `OS.metric("battery")` later.
- Whether single-axis layout should reject mixed row/col calls instead of silently using the first axis.
- Whether style fields (`fg`, `bold`) should start affecting the text dump or remain renderer-only metadata.

### What should be done in the future

- Commit this layout/widgets slice.
- Upload this ticket's updated guide and diary to reMarkable.
- Continue to `0102-PICOJS-INPUT-APP-MODE` or, if callback/timer ownership is more urgent, move to `0102-PICOJS-FRAME-TIMERS` before physical key routing.

### Code review instructions

- Start with the layout model and `recompute_layout()` in `components/picojs_runtime/picojs_runtime.cpp`.
- Review `js_app_layout()` and `js_layout_add()` for synchronous builder semantics.
- Review `draw_gauge()` and `gauge_value()` for deterministic frame behavior.
- Review the embedded dashboard source and `picojs load` dispatch in `app_main.cpp`.
- Validate with:

```bash
cd 0102-esp32-p4-visual-quickjs-repl
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
../ttmp/2026/06/25/0102-PICOJS-LAYOUT-WIDGETS--0102-picojs-layout-and-widgets/scripts/01-layout-widgets-probe.py \
  --port /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
```

### Technical details

The embedded dashboard app currently uses this DSL shape:

```js
var app = OS.app('dashboard');
app.layout(function (l) { l.row(1, 'bar').row('*', 'body'); });
app.panel('bar').text('PicoJS Dashboard').at(0, 0).bold().fg('cyan');
var body = app.panel('body').frame('single').title(' system ');
body.text('ESP32-P4 native DSL').at('center', 1).bold().fg('cyan');
body.gauge().at(2, 3).label('batt').value('battery').width(20).showPct();
body.gauge().at(2, 5).label('heap').value(62).width(20).showPct();
app.statusbar('dashboard native picojs');
app.mount();
'picojs dashboard loaded';
```
