---
Title: Implementation Diary
Ticket: 0102-PICOJS-FRAME-TIMERS
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
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: Console frame/run jobs and built-in interactive app
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Frame/timer/loop/compute callback implementation and JS-task rendering
    - Path: ttmp/2026/06/25/0102-PICOJS-FRAME-TIMERS--0102-picojs-frame-loop-and-timers/scripts/01-frame-timers-probe.py
      Note: UART probe for timers and callback errors
ExternalSources: []
Summary: Diary for Deterministic frames, timers, loops, compute callbacks, and callback error handling.
LastUpdated: 2026-06-25T15:30:00-07:00
WhatFor: Chronological implementation notes for 0102 PicoJS Frame Loop and Timers.
WhenToUse: Read before resuming this phase, before committing, and before hardware probe work.
---


# Diary

## Goal

This diary records the implementation of `0102-PICOJS-FRAME-TIMERS`. The phase goal is to support time-based apps without giving JavaScript direct control over the FreeRTOS scheduler.

## Step 1: Create child ticket and phase guide

This step split the broad PicoJS device integration effort into a focused child ticket. The design guide defines the phase scope, console contract, validation strategy, and review risks so implementation can proceed in small commits.

The child ticket links back to the umbrella `0102-PICOJS-DEVICE-INTEGRATION` plan. Its work should remain narrow; later-phase features belong in their own child tickets unless the umbrella design is updated.

### Prompt Context

**User prompt (verbatim):** "alright, do it, and for each ticket write a design + implementation guide as usual (and upload to remarkable), then stsart working on tasks one by one, committing at appropriate intervals, and keeping a detailed diary."

**Assistant interpretation:** Create the phase tickets and their implementation guides, upload the documentation package to reMarkable, then begin work phase-by-phase with commits and diary updates.

**Inferred user intent:** Keep a complex firmware integration manageable, reviewable, and recoverable by splitting it into documented tickets with concrete validation gates.

**Commit (code):** N/A — ticket/documentation setup only.

### What I did

- Created child ticket `0102-PICOJS-FRAME-TIMERS`.
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
- Review only changes that are in scope for `0102-PICOJS-FRAME-TIMERS`.
- Validate using the commands listed in the design guide.

### Technical details

Phase commands to keep in view:

- `picojs frame 1000`
- `picojs run 10 100`
- `picojs status`
- `picojs dump`



## Step 2: Execute frames, timers, loops, and compute callbacks on the QuickJS task

This step moved PicoJS frame progression from a native-only render tick to a QuickJS-owned frame job. That makes function-valued text, status bars, gauges, `app.on("tick", ...)`, `app.loop(...)`, and `app.compute(...)` execute on the `qjs_service` task instead of calling QuickJS from the UART console task.

It also added a reusable serial helper and a deterministic frame/timer probe, so future device checks no longer need hand-written inline `termios` snippets.

### Prompt Context

**User prompt (verbatim):** "go ahead, test it all, then do the rendering + kieyboard on the hardware impementation. I'm going out running errands and what you to be done when i come back. Commit at appropriate intervals, and keep a detailed diary as you work."

**Assistant interpretation:** Finish the callback/input/LCD phases while the user is away, validate on the physical ESP32-P4 PicoCalc hardware, commit at useful boundaries, and keep the ticket diaries current.

**Inferred user intent:** Leave the device in a working state where PicoJS apps are time-driven, interactive, visible on the LCD, and backed by repeatable UART probes instead of manual inspection only.

**Commit (code):** 263323f6a46fb5cfae2fe3cf6b518aa8cc55a48b — "0102: add PicoJS timers input and LCD rendering"

### What I did
- Added stored QuickJS callback support for `App.on("tick", ms, fn)`, `App.loop(fps, fn)`, and `App.compute(fn)` in `components/picojs_runtime/picojs_runtime.cpp`.
- Added `picojs_runtime_frame_js(JSContext *, ...)` so frame advancement and callback rendering run inside a `qjs_service_run()` job.
- Kept the old native-only `picojs_runtime_frame()` path for status-only fallback, but changed console `picojs frame` and `picojs run` to use the JS-task path.
- Added function-valued text/status/gauge rendering support through `StoredValue` evaluation during frame render.
- Added `picojs run <count> <dt_ms>` for deterministic callback progression.
- Added reusable UART helper `0102-esp32-p4-visual-quickjs-repl/tools/picocalc_console.py`.
- Added probe `scripts/01-frame-timers-probe.py`.

### Why
- QuickJS values and functions must be invoked on the QuickJS service task, not from arbitrary console or keyboard tasks.
- Deterministic `picojs run` lets timer and loop behavior be tested without wall-clock sleeps.
- The reusable console helper removes repeated ad-hoc Python serial snippets and makes probes easier to audit.

### What worked
- Build passed with ESP-IDF 5.4.2: `source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Flash passed on `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- `scripts/01-frame-timers-probe.py` passed.
- Validation output included `picojs run: ESP_OK count=3 dt_ms=1000`, `frames=3`, `errors=0`, and a dump line `ticks: 3`.
- Callback error validation intentionally threw on the second compute callback and reported `errors=1` while keeping the app renderable with `n=2`.

### What didn't work
- Before this step, the frame command had rendered without executing JS callbacks; that was safe for static dashboard gauges but insufficient for timers and function-valued widgets.
- A display corruption report appeared during the same hardware session. It was not caused by timer callbacks; it was traced separately to the LCD SPI clock margin and fixed in the LCD renderer phase by lowering the default LCD SPI rate to 40 MHz.

### What I learned
- The existing `StoredValue` ownership wrapper was enough to retain callbacks safely as long as reset destroys PicoJS runtime state before `qjs_service_reset()`.
- Console-driven deterministic frames are a good substitute for a scheduler-owned app loop during bring-up.

### What was tricky to build
- The main sharp edge was task ownership: rendering can be requested from console commands, but function-valued widgets require QuickJS execution. The solution was to add small job structs in `app_main.cpp` and run `picojs_runtime_frame_js()` through `qjs_service_run()`.
- Callback errors must not unwind through the console command. The runtime counts exceptions in `last_error_count`, frees the returned `JSValue`, and continues rendering.

### What warrants a second pair of eyes
- Review `StoredValue` lifetime and reset ordering in `components/picojs_runtime/picojs_runtime.cpp`, especially callback vectors and `picojs_runtime_reset()`.
- Review timer accumulator behavior; current tick timers fire at most once per frame even if `dt_ms` spans multiple intervals, while loop callbacks can catch up with a guard limit.

### What should be done in the future
- Decide whether timers should catch up multiple missed intervals or intentionally coalesce to one callback per frame.
- Expose callback exception details over UART if debugging JS apps becomes painful.

### Code review instructions
- Start in `components/picojs_runtime/picojs_runtime.cpp` at `TimerCallback`, `LoopCallback`, `run_callbacks()`, and `picojs_runtime_frame_js()`.
- Then review `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` around `run_picojs_frame()` and `cmd_picojs()`.
- Validate with: `ttmp/2026/06/25/0102-PICOJS-FRAME-TIMERS--0102-picojs-frame-loop-and-timers/scripts/01-frame-timers-probe.py`.

### Technical details
- Build command: `cd 0102-esp32-p4-visual-quickjs-repl && source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Flash command: `idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash`.
- Probe command: `ttmp/2026/06/25/0102-PICOJS-FRAME-TIMERS--0102-picojs-frame-loop-and-timers/scripts/01-frame-timers-probe.py`.
- Key passing substrings: `frames=3`, `errors=0`, `ticks: 3`, then `frames=2`, `errors=1`, `n=2` for the intentional compute exception.
