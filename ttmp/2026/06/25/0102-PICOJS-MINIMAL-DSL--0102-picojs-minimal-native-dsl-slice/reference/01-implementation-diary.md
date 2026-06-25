---
Title: Implementation Diary
Ticket: 0102-PICOJS-MINIMAL-DSL
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
Summary: "Diary for First firmware-native QuickJS builder API: app, panel, text, mount, frame, dump."
LastUpdated: 2026-06-25T15:30:00-07:00
WhatFor: "Chronological implementation notes for 0102 PicoJS Minimal Native DSL Slice."
WhenToUse: "Read before resuming this phase, before committing, and before hardware probe work."
---

# Diary

## Goal

This diary records the implementation of `0102-PICOJS-MINIMAL-DSL`. The phase goal is to prove that a JavaScript DSL app can be described through native QuickJS bindings and dumped over UART.

## Step 1: Create child ticket and phase guide

This step split the broad PicoJS device integration effort into a focused child ticket. The design guide defines the phase scope, console contract, validation strategy, and review risks so implementation can proceed in small commits.

The child ticket links back to the umbrella `0102-PICOJS-DEVICE-INTEGRATION` plan. Its work should remain narrow; later-phase features belong in their own child tickets unless the umbrella design is updated.

### Prompt Context

**User prompt (verbatim):** "alright, do it, and for each ticket write a design + implementation guide as usual (and upload to remarkable), then stsart working on tasks one by one, committing at appropriate intervals, and keeping a detailed diary."

**Assistant interpretation:** Create the phase tickets and their implementation guides, upload the documentation package to reMarkable, then begin work phase-by-phase with commits and diary updates.

**Inferred user intent:** Keep a complex firmware integration manageable, reviewable, and recoverable by splitting it into documented tickets with concrete validation gates.

**Commit (code):** N/A — ticket/documentation setup only.

### What I did

- Created child ticket `0102-PICOJS-MINIMAL-DSL`.
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
- Review only changes that are in scope for `0102-PICOJS-MINIMAL-DSL`.
- Validate using the commands listed in the design guide.

### Technical details

Phase commands to keep in view:

- `picojs load hello`
- `picojs frame 0`
- `picojs dump`


## Step 2: Implement minimal firmware-native PicoJS builder API

I implemented the first firmware-native QuickJS DSL slice inside `components/picojs_runtime`. The runtime now installs an `OS` object into the existing `qjs_service` context, creates native app/panel/text objects through QuickJS wrapper handles, renders a single framed panel into the PicoJS 40x20 screen buffer, and exposes a built-in `picojs load hello` console path.

This step proves the core device architecture: JavaScript describes the app with fluent builder calls, but firmware-owned C++ objects store the app model and render the text frame. The result is visible through `picojs dump`, so the feature can be validated over UART without reading the LCD.

### Prompt Context

**User prompt (verbatim):** "do it, whole ticket, same workflow"

**Assistant interpretation:** Complete the `0102-PICOJS-MINIMAL-DSL` child ticket end-to-end using the same workflow: implement, build, flash/probe by stable device ID, document, upload, and commit at appropriate intervals.

**Inferred user intent:** Move from the runtime skeleton to the first real native DSL slice while preserving the documented, console-driven validation discipline.

**Commit (code):** Pending — code and docs changed in this step.

### What I did

- Extended `components/picojs_runtime/include/picojs_runtime.h`:
  - included QuickJS API for `JSContext *`
  - added `picojs_runtime_install(JSContext *, picojs_runtime_t *)`
  - added `picojs_runtime_reset(picojs_runtime_t *)`
  - added `js_installed` to runtime status
- Reworked `components/picojs_runtime/picojs_runtime.cpp`:
  - added native `App`, `Panel`, and `TextWidget` model structs
  - added QuickJS class IDs and wrapper objects for `OS`, `App`, `Panel`, and `Text`
  - implemented `OS.app(name)`
  - implemented `App.panel(id)`, `App.statusbar(text)`, and `App.mount()`
  - implemented `Panel.frame(kind)`, `Panel.title(text)`, and `Panel.text(text)`
  - implemented `Text.at(xOrAlign, y)`, `Text.fg(color)`, and `Text.bold()`
  - added simple framed panel rendering and centered text rendering
- Updated `components/picojs_runtime/CMakeLists.txt` to require `quickjs_native`.
- Updated `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`:
  - installed PicoJS into QuickJS with `qjs_service_run()`
  - added `picojs install`
  - added `picojs load hello`
  - reinstalled PicoJS after `js reset`
  - cleared PicoJS native app state before QuickJS reset
- Added `scripts/01-minimal-dsl-probe.py` for repeatable by-id UART validation.
- Built, flashed, and probed the ESP32-P4 over:

```text
/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
```

### Why

- The first DSL slice needed to prove native QuickJS builder binding mechanics on the actual firmware, not only in the desktop host.
- `qjs_service_run()` keeps all QuickJS C API calls on the QuickJS service task.
- The built-in hello app gives a stable validation target before arbitrary script loading or more widgets are added.

### What worked

- Final build succeeded with ESP-IDF 5.4.2:

```text
0102-esp32-p4-visual-quickjs-repl.bin binary size 0xde640 bytes. Smallest app partition is 0x400000 bytes. 0x3219c0 bytes (78%) free.
```

- Flash succeeded on the by-id ESP32-P4 path.
- Minimal DSL probe passed:

```text
MINIMAL_DSL_PROBE PASS [True, True, True, True, True, True, True, True]
```

- The dump showed the native DSL-rendered hello app:

```text
[00] +- hello ------------------------------+
[01] |                                      |
[02] |                                      |
[03] |             HELLO DEVICE             |
...
[18] +--------------------------------------+
[19] native picojs minimal
```

- Reset/reinstall validation passed:

```text
js reset: ESP_OK picojs_clear=ESP_OK picojs_reinstall=ESP_OK
picojs: initialized=1 js_installed=1 cols=40 rows=20 apps=0 mounted=0 frames=0 last_frame_ms=0 errors=0
```

### What didn't work

- First build failed because QuickJS's `JS_CFUNC_DEF` macro uses designated initializer syntax that ESP-IDF's C++2b build rejected in this context:

```text
error: either all initializer clauses should be designated or none of them should be
note: in expansion of macro 'JS_CFUNC_DEF'
```

I fixed this by replacing `JS_SetPropertyFunctionList`/`JS_CFUNC_DEF` usage with a small helper that uses `JS_NewCFunction()` and `JS_SetPropertyStr()` for each method.

- First build also failed because `JS_ToInt32()` expects an `int32_t *`, while the widget model stored coordinates as `int`:

```text
error: invalid conversion from 'int*' to 'int32_t*' {aka 'long int*'} [-fpermissive]
```

I fixed this by parsing into local `int32_t` variables and assigning to the model's `int` fields.

- The initial reset path reinstalled `OS` but left the native app model from before reset. That was safe for this string-only minimal model but wrong as a future QuickJS ownership pattern. I added `picojs_runtime_reset()` and call it before `qjs_service_reset()`.

### What I learned

- On ESP-IDF, using `JS_NewCFunction` plus `JS_SetPropertyStr` is less fragile than QuickJS's C initializer macros from C++ component code.
- The native wrapper pattern works on-device: JS objects can be non-owning handles over firmware-owned app/panel/widget state.
- It is important to separate QuickJS context reset from native runtime reset now, before later phases add stored callbacks and `JSValue` ownership.

### What was tricky to build

- The biggest sharp edge was QuickJS's C API surface in C++ mode. The same builder pattern that works in the desktop native host needed adjustment for ESP-IDF's stricter compile settings.
- Reset ordering matters. Native app state must be cleared before destroying/recreating the QuickJS context once later phases store duplicated JS callbacks. This ticket added the reset hook before those stored callback paths exist.
- The prompt capture still includes boot-time mojibake/noise before `0102>`. The probe continues to key off the prompt substring and validates command output after the prompt.

### What warrants a second pair of eyes

- The current wrappers do not have QuickJS finalizers because native state is owned by `picojs_runtime`. This is intentional, but reviewers should confirm no wrapper owns memory.
- `OS.app()` currently replaces the single native app. That is sufficient for the minimal slice; multi-app or launch semantics belong in later tickets.
- `Text.fg()` and `Text.bold()` store style metadata but the current text dump renderer does not display style. Later renderer/widget tickets should decide how style affects LCD output and/or dump annotations.

### What should be done in the future

- Upload this ticket's updated guide/diary to reMarkable.
- Commit the minimal DSL slice.
- Start `0102-PICOJS-LAYOUT-WIDGETS` with `App.layout`, `Layout.row/col`, gauges, and a dashboard validation app.

### Code review instructions

- Start with `components/picojs_runtime/include/picojs_runtime.h` for the public API additions.
- Review `picojs_runtime_install`, `register_classes`, `set_func`, and the `js_*` builder methods in `components/picojs_runtime/picojs_runtime.cpp`.
- Review `cmd_picojs` and `js reset` handling in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`.
- Validate with:

```bash
cd 0102-esp32-p4-visual-quickjs-repl
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
../ttmp/2026/06/25/0102-PICOJS-MINIMAL-DSL--0102-picojs-minimal-native-dsl-slice/scripts/01-minimal-dsl-probe.py \
  --port /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
```

### Technical details

The built-in hello source is currently embedded in `app_main.cpp` as `kPicoJsHelloSource`:

```js
var app = OS.app('hello');
var p = app.panel('main').frame('rounded').title(' hello ');
p.text('HELLO DEVICE').at('center', 2).bold().fg('cyan');
app.statusbar('native picojs minimal');
app.mount();
'picojs hello loaded';
```
