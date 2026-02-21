---
Title: mquickjs Matrix Scripting API Architecture and Integration Blueprint
Ticket: ESP-02-JS-MATRIX-API
Status: active
Topics:
    - esp32
    - esp-idf
    - mquickjs
    - javascript
    - led-matrix
    - rest
    - rtos
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp
      Note: Prior art for JS service bootstrap and HTTP eval JSON responses
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp
      Note: Prior art for timer scheduling and JS callback dispatch
    - Path: 0067-esp-c3-led-matrix-http/main/http_server.c
      Note: Current REST matrix endpoints and JSON parsing patterns used as integration baseline
    - Path: 0067-esp-c3-led-matrix-http/main/matrix_engine.c
      Note: Current matrix animation/rendering engine analyzed for JS integration constraints
    - Path: components/mqjs_service/mqjs_service.cpp
      Note: Reusable single-owner JS task+queue service recommended for 0067
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Interpreter API surface and interrupt/eval primitives analyzed
ExternalSources: []
Summary: Deep analysis for integrating mquickjs into 0067 to run matrix scripts over REST with real-time control primitives.
LastUpdated: 2026-02-21T16:52:03.90317528-05:00
WhatFor: ""
WhenToUse: ""
---


# mquickjs Matrix Scripting API Architecture and Integration Blueprint

## Executive Summary

This document describes how to evolve `0067-esp-c3-led-matrix-http` into a scriptable LED matrix runtime where JavaScript programs are submitted over REST and can drive animations, text, and raw pixels with tight timing control. The recommended path is to reuse the already-proven `mqjs_service` architecture from shared components and prior firmware (`0066-cardputer-adv-ledchain-gfx-sim`) instead of embedding ad hoc JavaScript calls directly in HTTP handlers.

The design keeps hard boundaries between concerns:

- The matrix engine remains the sole owner of MAX7219 drawing state.
- The JavaScript VM remains single-owner through a dedicated JS task.
- REST and console frontends become command producers, not engine owners.
- Timing-sensitive callbacks use dedicated timer task(s) that post work into the JS task.

The result is deterministic concurrency, robust failure behavior under malformed scripts, and an API surface that supports both low-level pixel control and high-level built-in animations.

## Problem Statement

The firmware today already supports:

- Wi-Fi + `esp_console` shell.
- REST endpoints for text and predefined animations.
- A C-based matrix animation engine with scroll/wave/drop modes.

The missing capability is programmable behaviors: sending JavaScript over REST to define animation logic on-device. The primary challenge is not “how to parse JavaScript,” but how to preserve real-time behavior and system stability when untrusted or buggy scripts run continuously.

Key constraints:

- Hardware: ESP32-C3 with constrained RAM and a shared CPU budget.
- Display: 12 chained MAX7219 8x8 modules (96x8 logical canvas).
- Existing matrix engine is C and stateful (`matrix_engine.c`).
- Existing JS runtime patterns exist in repo but are not integrated into 0067.
- Tight loops are required for visually smooth animation.

Success criteria:

- Scripts can be uploaded/executed via REST.
- Scripts can drive pixels, text, and animations.
- Runtime remains responsive to stop/reset/status requests.
- Wi-Fi console remains available for diagnostics and control.

## Fundamentals Every New Developer Needs

### 1) MAX7219 matrix fundamentals

The MAX7219 is a serial LED driver chip. In a daisy chain, one SPI write shifts data through all modules. With 12 modules:

- Logical width = `12 * 8 = 96` pixels.
- Height = `8` pixels.
- Physical mapping depends on chain order and orientation.
- Frame updates are usually row-by-row writes.

Current firmware specifics (`0067-esp-c3-led-matrix-http/main/Kconfig.projbuild`):

- `DIN/MOSI = GPIO4`
- `CS = GPIO5`
- `CLK/SCK = GPIO6`
- `CHAIN_LEN default = 12`

### 2) FreeRTOS ownership model

Embedded failures often come from multiple tasks mutating the same state. This repo’s mature pattern is: one owner task per mutable subsystem.

- `matrix_engine` owns frame buffer and mode state.
- `mqjs_service` owns all `mquickjs` calls.
- Timer subsystem schedules events and posts callbacks to JS owner.
- HTTP handlers only parse/validate/dispatch.

This is the central architectural principle to preserve.

### 3) mquickjs fundamentals in this repo

The imported `mquickjs` runtime is C-based and uses a fixed arena model:

- `JS_NewContext(arena_ptr, arena_bytes, stdlib)` creates VM context with bounded memory.
- `JS_SetInterruptHandler(...)` allows timeout/deadline interruption.
- `JS_Eval(...)` and `JS_Call(...)` run code in the owning thread.

A practical consequence: no concurrent VM entry from multiple tasks. All JS entrypoints must serialize through one service queue.

## Current Codebase State (What Exists Today)

### Firmware target baseline: `0067-esp-c3-led-matrix-http`

Important files:

- `0067-esp-c3-led-matrix-http/main/app_main.c`
- `0067-esp-c3-led-matrix-http/main/http_server.c`
- `0067-esp-c3-led-matrix-http/main/matrix_engine.c`
- `0067-esp-c3-led-matrix-http/main/matrix_console.c`
- `0067-esp-c3-led-matrix-http/main/max7219.c`

Key observations:

- `app_main.c` initializes matrix engine, Wi-Fi manager, and console.
- `http_server.c` exposes `/api/matrix/status`, `/api/matrix/text`, `/api/matrix/anim`, `/api/matrix/stop`.
- `matrix_engine.c` already has a 5x7 glyph table, scroll/wave/drop animation logic, repeat count, and orientation controls.
- `matrix_console.c` already has a parser with examples and status output.

### Reusable JS service component already available

Shared component files:

- `components/mqjs_service/include/mqjs_service.h`
- `components/mqjs_service/include/mqjs_vm.h`
- `components/mqjs_service/mqjs_service.cpp`
- `components/mqjs_service/mqjs_vm.cpp`

This gives us:

- Dedicated JS task + queue.
- Synchronous eval API returning output/error/timed_out.
- Async job posting API for callback-style work.
- Deadline interruption based on `esp_timer_get_time()`.

### Proven prior-art integration in `0066`

Relevant files:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`
- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp`
- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
- `0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp`

This stack proves all critical patterns needed for 0067:

- HTTP `/api/js/eval` endpoint with bounded body.
- `setTimeout/clearTimeout` backed by an RTOS timer task.
- JS bootstrap helpers (`every`, `cancel`) in pure JS.
- VM memory dumps and reset endpoint.

## Proposed Solution

### High-level architecture

```text
                    +------------------------------+
HTTP / REST ------> |  HTTP Server Task            |
Console cmd ------> |  Parse + Validate + Dispatch |
                    +--------------+---------------+
                                   |
                                   | matrix commands
                                   v
                    +------------------------------+
                    | Matrix Engine Task/State     |
                    | - framebuffer                |
                    | - text/scroll/drop builtins  |
                    | - JS frame application       |
                    +------------------------------+

                                   ^
                                   | frame + control calls
                                   |
                    +--------------+---------------+
                    | JS Service Task (mqjs_service)|
                    | - single VM owner             |
                    | - script lifecycle            |
                    | - API bindings (matrix.*)     |
                    +--------------+---------------+
                                   ^
                                   |
                    +--------------+---------------+
                    | JS Timer Task                 |
                    | schedule/cancel + callback    |
                    +------------------------------+
```

### Why this architecture

It reuses existing, battle-tested abstractions in this repository and avoids introducing shared-state races between C engine code and JS execution. It also provides clear operational controls:

- Stop one script without reboot.
- Reset VM without reinitializing full Wi-Fi stack.
- Bound runtime and memory behavior.

### Scope of changes

1. Add JS runtime and timer components to 0067 build.
2. Add matrix-native JS bindings to expose low-level and high-level control.
3. Add REST endpoints for script upload/run/stop/status/memory.
4. Add console subcommands for script operations and examples.
5. Keep existing matrix REST/console APIs backward compatible.

## Design Decisions and Rationale

### Decision A: Reuse `mqjs_service`, do not call JS directly from HTTP handlers

Rationale:

- Prevents cross-task VM reentrancy bugs.
- Keeps latency spikes isolated.
- Enables future producers (WebSocket, UART, schedule events) without redesign.

### Decision B: Keep matrix C engine as rendering authority

Rationale:

- Existing code already handles MAX7219 mapping/orientation/repeat behavior.
- JS should request rendering actions through an API, not bypass SPI driver internals.
- Easier to test and reason about frame semantics.

### Decision C: Provide layered JS API

Rationale:

Developers need both granular and simple control. A layered API supports both:

- Layer 0: direct pixel/frame primitives.
- Layer 1: text/glyph helpers.
- Layer 2: built-in animations.
- Layer 3: orchestration/time helpers.

### Decision D: Cooperative “tight timing” over busy-loop spinning

Rationale:

Busy loops in JS starve other tasks and create unstable latency. Better pattern:

- Use microsecond clock primitives (`nowUs()`),
- Render quickly,
- Yield with deterministic sleep primitives (`sleepMs`, frame sync),
- Enforce watchdog deadlines.

## Detailed Integration Blueprint

### 1) Build and component wiring

Update `0067-esp-c3-led-matrix-http/CMakeLists.txt`:

- Add component dirs:
- `../components/mqjs_service`
- `../imports/esp32-mqjs-repl/mqjs-repl/components`

Update `0067-esp-c3-led-matrix-http/main/CMakeLists.txt`:

- Add source files:
- `mqjs/js_service.cpp`
- `mqjs/mqjs_timers.cpp`
- `mqjs/esp32_stdlib_runtime.c` (or `matrix_js_runtime.c` equivalent)
- Add `mqjs_service` and `mquickjs` related dependencies.

Add Kconfig knobs in `main/Kconfig.projbuild`:

- `CONFIG_TUTORIAL_0067_JS_MEM_BYTES` (VM arena)
- `CONFIG_TUTORIAL_0067_JS_MAX_BODY` (REST body cap)
- `CONFIG_TUTORIAL_0067_JS_DEFAULT_TIMEOUT_MS`
- `CONFIG_TUTORIAL_0067_JS_MAX_TIMERS`

### 2) Runtime startup lifecycle

At boot (`app_main.c`):

- Initialize matrix engine first.
- Start Wi-Fi manager.
- Start console.
- Start JS service after matrix engine is ready.
- Bind runtime to matrix engine handle/state.

Pseudo sequence:

```c
matrix_engine_init(...);
wifi_mgr_start();
wifi_console_start(...);
js_service_start(/* matrix binding */);
```

### 3) JS-to-matrix call path

Preferred path:

- JS code calls `matrix.present(frame)`.
- Native binding validates dimensions and payload.
- Binding dispatches to matrix engine API (thread-safe call boundary).
- Matrix engine performs final physical mapping and flush.

If needed for throughput, add an engine “batch frame apply” API to avoid per-pixel lock overhead.

### 4) Timer system integration

Use 0066-style timer task adapted for 0067 namespace, e.g. `__0067.timers.cb`.

Core responsibilities:

- Store one-shot timeouts by `id` with due timestamp.
- Wake at nearest due timer.
- On expiration, post callback job to JS service queue.
- Never call JS directly from timer task.

This preserves strict single-thread VM ownership.

## Real-Time Behavior and Tight Timing Strategy

### Timing requirements reality check

For a 96x8 mono matrix, 30-60 FPS is visually smooth. Hard real-time sub-millisecond determinism is unnecessary for display rendering but frame pacing jitter above ~10-20 ms becomes visible.

Design target:

- Typical animation loop at 20-40 FPS.
- Script operations per frame bounded.
- Present path amortized and predictable.

### Runtime primitives for deterministic pacing

Expose these primitives to JS:

- `matrix.nowUs()` returns monotonic microseconds.
- `matrix.sleepMs(ms)` yields task for at least `ms`.
- `matrix.syncFps(targetFps)` waits until next frame boundary.

Example animation loop:

```javascript
const fps = 30;
const dtUs = Math.floor(1_000_000 / fps);
let nextUs = matrix.nowUs();

while (!matrix.shouldStop()) {
  drawFrame();
  matrix.present();
  nextUs += dtUs;
  matrix.sleepUntilUs(nextUs);
}
```

The important behavior is cooperative yielding. Avoid pure compute spin loops in JS.

### Deadline and interruption policy

Use per-operation deadline via `mqjs_service_eval(..., timeout_ms)` and per-job deadlines for callbacks.

Policy recommendations:

- REST eval default timeout: 250 ms.
- Long-running scripts run as background programs with heartbeat and stop token.
- If timeout fires, return `timed_out=true` and preserve diagnostic error text.

## JavaScript API Design

### API goals

- Full control from raw pixels to animation presets.
- Explicit, unsurprising semantics.
- Stable API names for CLI/REST tooling.

### Namespace proposal

Use global namespace `matrix` and optional `matrix.anim` helper namespace.

Core object sketch:

```javascript
matrix.width();
matrix.height();
matrix.clear();
matrix.fill(on);
matrix.setPixel(x, y, on);
matrix.getPixel(x, y);
matrix.present();

matrix.setText(text);               // static text mode
matrix.startScroll(text, opts);     // opts: { fps, pauseMs, repeat, wave }
matrix.startDrop(text, opts);       // opts: { fps, pauseMs, repeat }
matrix.stop();

matrix.setIntensity(v);             // 0..15
matrix.setOrientation({ reverse, flipv });
matrix.status();

matrix.nowUs();
matrix.sleepMs(ms);
matrix.sleepUntilUs(tUs);
matrix.shouldStop();
```

### Layered API contract

Layer 0 (lowest-level framebuffer):

- `matrix.setPixel(x,y,on)`
- `matrix.getPixel(x,y)`
- `matrix.clear()`
- `matrix.fill(on)`
- `matrix.present()`

Layer 1 (text/glyph):

- `matrix.drawChar(ch, x, y)`
- `matrix.drawText(text, x, y, spacing)`
- `matrix.measureText(text)`

Layer 2 (built-ins):

- `matrix.startScroll(text, opts)`
- `matrix.startDrop(text, opts)`
- `matrix.stop()`

Layer 3 (timed orchestration):

- `setTimeout(fn, ms)` / `clearTimeout(id)`
- `every(ms, fn)` and `cancel(handle)` bootstrap helpers
- `matrix.sleepMs`, `matrix.sleepUntilUs`

### API behavior for predefined animations

The predefined C animations should remain callable by JS with same semantics as REST/console:

- `repeat=0` means infinite.
- `fps` clamped to safe range (1..60).
- Empty text allowed when explicitly intended (clear/blank behavior) via explicit method, not ambiguous parsing.

### JS examples for documentation and console

Example 1: pixel snake

```javascript
matrix.clear();
for (let x = 0; x < matrix.width(); x++) {
  matrix.clear();
  matrix.setPixel(x, 3, 1);
  matrix.present();
  matrix.sleepMs(20);
}
```

Example 2: delegated built-in wave

```javascript
matrix.startScroll("HELLO JS", { fps: 20, pauseMs: 250, repeat: 3, wave: true });
```

Example 3: cooperative continuous loop

```javascript
let x = 0;
while (!matrix.shouldStop()) {
  matrix.clear();
  matrix.drawText(".", x, 0, 1);
  matrix.present();
  x = (x + 1) % matrix.width();
  matrix.sleepMs(33);
}
```

## REST API Design for Script Runtime

### Endpoint set

Recommended additions under `/api/js` and `/api/matrix/js`:

- `POST /api/js/eval`
- `POST /api/js/reset`
- `GET /api/js/mem`
- `GET /api/js/status`
- `POST /api/matrix/js/start`
- `POST /api/matrix/js/stop`

`/api/js/eval` is request/response and short-running. `/api/matrix/js/start` is for managed long-running scripts.

### Request/response contracts

`POST /api/js/eval`

- Body: raw JavaScript text.
- Response:

```json
{"ok":true,"output":"...","error":null,"timed_out":false}
```

`POST /api/matrix/js/start`

- Body JSON:

```json
{
  "script": "...",
  "name": "wave-demo",
  "timeout_ms": 1000,
  "replace": true
}
```

- Response JSON:

```json
{
  "ok": true,
  "running": true,
  "script_id": "wave-demo",
  "started_at_ms": 1700000000
}
```

`GET /api/js/status`

- Include running state, last error, memory budget, and queue depth if available.

### Input hardening

- Reject oversized bodies with HTTP 413.
- Return explicit parse errors with line references when possible.
- Keep text encoding as UTF-8 but normalize unsupported glyphs to spaces when mapped to 5x7 font.

## Console Integration Design

The console is already Wi-Fi-backed through `wifi_console`. Add a `js` command family and optionally matrix-js aliases.

Suggested commands:

- `js eval <CODE>`
- `js file <PATH>` (if/when file storage exists)
- `js reset`
- `js mem`
- `js status`
- `js examples`

Suggested matrix-script shortcuts:

- `matrix js run <CODE>`
- `matrix js stop`
- `matrix js status`

The parser style can mirror existing `matrix_console.c` command dispatch.

## Concurrency Model and Critical Invariants

### Invariants

- Never call `JS_Eval`, `JS_Call`, or other VM APIs outside JS service task.
- Never let timer task call JS directly.
- Matrix hardware writes are always mediated by matrix engine APIs.
- Script stop must be idempotent.

### Data ownership rules

- HTTP owns request buffer only until dispatch completion.
- JS service owns evaluation state and callbacks.
- Matrix engine owns framebuffer and animation mode state.

### Script preemption model

Implement stop token checked by native API calls and loop helpers:

- `matrix.shouldStop()` reads atomic stop flag.
- `matrix.sleepMs` / `sleepUntilUs` return early when stop requested.
- `POST /api/matrix/js/stop` sets flag and optionally cancels timers.

## Memory, Safety, and Failure Handling

### Memory budgeting

The VM uses fixed arena allocation. Add a conservative default and expose config:

- Example default: `128 KB` for VM arena.
- Increase only if scripts require bigger objects/arrays.

Memory failure behavior:

- JS OOM should fail current eval gracefully.
- Service remains alive after failed eval.
- Report OOM in response error string.

### Failure classes and expected behavior

Script syntax error:

- Response: `ok=false`, parse/exception string.
- VM remains healthy.

Script timeout:

- Response: `ok=false` and/or `timed_out=true`.
- Optional auto-stop of managed script.

Timer callback throw:

- Log warning.
- Keep runtime alive.
- Continue other timers unless policy says otherwise.

## Implementation Plan (Concrete File-Level Work)

### Phase 1: Wire runtime dependencies

Files:

- `0067-esp-c3-led-matrix-http/CMakeLists.txt`
- `0067-esp-c3-led-matrix-http/main/CMakeLists.txt`
- `0067-esp-c3-led-matrix-http/main/Kconfig.projbuild`

Actions:

- Add `mqjs_service` + imported `mquickjs` component dirs.
- Add JS runtime source set.
- Add JS memory/body/timeout configs.

### Phase 2: Add JS service and timers for 0067

Files (new recommended):

- `0067-esp-c3-led-matrix-http/main/mqjs/js_service.h`
- `0067-esp-c3-led-matrix-http/main/mqjs/js_service.cpp`
- `0067-esp-c3-led-matrix-http/main/mqjs/mqjs_timers.h`
- `0067-esp-c3-led-matrix-http/main/mqjs/mqjs_timers.cpp`

Actions:

- Port 0066 architecture, rename namespace internals to `__0067`.
- Provide startup/reset/eval/mem/status APIs.
- Integrate timer callback posting.

### Phase 3: Implement matrix JS bindings

Files (new recommended):

- `0067-esp-c3-led-matrix-http/main/mqjs/esp32_stdlib_runtime.c` (or split modules)
- `0067-esp-c3-led-matrix-http/main/mqjs/matrix_js_bindings.c`

Actions:

- Add low-level framebuffer primitives.
- Add wrappers for existing matrix engine text/animation APIs.
- Add timing helpers.

### Phase 4: REST integration

Files:

- `0067-esp-c3-led-matrix-http/main/http_server.c`
- `0067-esp-c3-led-matrix-http/main/http_server.h`

Actions:

- Add `/api/js/eval`, `/api/js/reset`, `/api/js/mem`, `/api/js/status`.
- Add `/api/matrix/js/start`, `/api/matrix/js/stop` (optional in first pass).
- Preserve existing matrix endpoints unchanged.

### Phase 5: Console integration

Files:

- `0067-esp-c3-led-matrix-http/main/matrix_console.c`
- `0067-esp-c3-led-matrix-http/main/js_console.c` (recommended separate)
- `0067-esp-c3-led-matrix-http/main/app_main.c`

Actions:

- Register new `js` command tree.
- Add `examples` command that prints tested snippets.
- Keep existing `matrix` command behavior stable.

### Phase 6: Validation and profiling

Validation checklist:

- Build/flash/monitor on Stamp C3.
- Run basic JS eval and exception cases.
- Run 20-40 FPS scripted loop for 10+ minutes.
- Validate console responsiveness while script runs.
- Validate stop/reset reliability under load.

## Pseudocode Sketches

### A) Safe eval path

```pseudo
http_js_eval_handler(req):
  code = read_body_with_limit(req, JS_MAX_BODY)
  if invalid -> 400/413

  if js_service not started -> 500

  result = js_service_eval(code, timeout_ms_default)
  return json(result)
```

### B) Managed long-running script

```pseudo
script_start(script, opts):
  acquire script_control_lock
  set stop_flag = true for old script
  cancel timers
  reset stop_flag = false
  enqueue js job: bootstrap + run script main loop
  mark running=true
```

### C) JS timer callback fire

```pseudo
timer_task_loop:
  wait_until_next_due_or_command
  if schedule/cancel command: mutate timer slots
  if due timers: for each due id
    post job to js_service(job_fire_timeout(id))
```

## Deep Dive: Hardware and Throughput Budgeting

Developers often underestimate how helpful simple throughput math is for animation architecture. Even approximate calculations can prevent over-design and clarify where optimization work actually matters.

### MAX7219 payload size per frame

For one MAX7219 module, one row update is effectively:

- 1 register byte
- 1 data byte

For `N` chained modules, each row flush writes `2 * N` bytes. For 12 modules:

- Row payload = `24 bytes`
- Full 8-row frame payload = `24 * 8 = 192 bytes`

### Theoretical SPI transfer time

At 100 kHz (`CONFIG_TUTORIAL_0067_MATRIX_SPI_HZ` default):

- 192 bytes = 1536 bits
- Transfer time ~= `1536 / 100000 = 15.36 ms`

At 1 MHz:

- Transfer time ~= `1.536 ms`

This is why SPI clock selection materially changes perceived smoothness. If frame payload already consumes most of the 33 ms frame budget (for 30 FPS), JS-side sophistication will not help. The first lever is often SPI clock and flush batching strategy.

### Frame budget example

Target: 30 FPS (`33.3 ms per frame`), 100 kHz SPI:

- Matrix transfer: ~15.4 ms
- JS drawing logic: e.g. 2-6 ms
- Locking/coordination/overhead: e.g. 1-3 ms
- Remaining slack: 9-15 ms

At 60 FPS (`16.7 ms per frame`) with 100 kHz SPI, transfer alone can dominate budget. Conclusion: for reliable 60 FPS on 12 modules, SPI frequency and/or partial updates become mandatory.

### Practical tuning recommendations

- Increase SPI clock cautiously and verify cable/chain stability.
- Keep the frame buffer in contiguous memory and avoid per-pixel dynamic allocation.
- Prefer one `present()` per frame over many tiny flushes.
- Avoid JSON encoding in high-frequency loop paths.

## Deep Dive: Runtime State Machine

To make operations reliable under resets/timeouts/stop requests, define explicit script runtime states. This avoids ambiguous behavior when endpoints race each other.

### Proposed state model

```text
IDLE
  -> STARTING      (script accepted, VM preparing)
  -> RUNNING       (script loop active)
  -> STOPPING      (stop requested, timers cancelled)
  -> IDLE          (clean exit)

Any state
  -> ERROR         (fatal script/runtime error)
ERROR
  -> IDLE          (after reset or successful recovery)
```

### State transition triggers

- `POST /api/matrix/js/start`: `IDLE -> STARTING` then `RUNNING`.
- `POST /api/matrix/js/stop`: `RUNNING -> STOPPING -> IDLE`.
- Timeout or unhandled runtime fault: `RUNNING -> ERROR`.
- `POST /api/js/reset`: force `IDLE` with VM rebuild.

### Why this matters

Without explicit states:

- Repeated `start` requests can produce ghost script behavior.
- Stop may return success while callbacks continue firing.
- Observability becomes weak (“why is matrix moving if status says idle?”).

With explicit states:

- Endpoints can return consistent HTTP codes and payloads.
- Console and REST can display same authoritative runtime status.
- QA can write deterministic test assertions.

## Deep Dive: JS Binding Semantics and Edge Cases

### String and glyph handling

The current `matrix_engine.c` glyph table supports ASCII 32..126. For JS and REST:

- Preserve UTF-8 in transport.
- During rasterization, map unsupported codepoints to `' '` or replacement strategy.
- Keep behavior explicit in docs and status output.

Recommended helper:

```c
// Pseudocode
char map_codepoint_to_glyph(uint32_t cp) {
  if (cp >= 32 && cp <= 126) return (char)cp;
  return ' ';
}
```

This prevents crashes or corrupted output when clients send punctuation-heavy strings, emoji, or multibyte scripts.

### Empty-string semantics

Empty strings are a common UX trap in CLI/REST flows. Explicitly define behavior:

- `matrix.setText(\"\")` => clear text mode output (blank display).
- `matrix.startScroll(\"\", ...)` => reject with clear error (invalid animation payload), unless explicitly configured otherwise.
- REST and console should preserve quoted empty values correctly.

### Numeric coercion and validation

For JS bindings, avoid implicit coercion surprises:

- Parse ints with strict range checks.
- Reject NaN and infinities where numeric types are required.
- Clamp only where clamping is documented; otherwise fail fast.

Example policy:

- `setIntensity(v)` rejects if `v < 0 || v > 15`.
- `startScroll(opts.fps)` clamps 1..60 because it is a pacing hint.

## Deep Dive: Observability and Debugging

A scriptable runtime needs first-class diagnostics. Otherwise failures appear as “flicker” or “stalled animation” without root cause.

### Logging strategy

Log categories should be separable:

- `0067_js_service`: eval lifecycle, start/stop/reset, deadlines.
- `0067_js_timers`: schedule/cancel/fire counts.
- `0067_matrix`: mode transitions, frame pacing warnings.
- `0067_http`: endpoint-level validation and rejection reasons.

### Status snapshots

Expose status in both console and REST with the same fields:

- runtime state (`idle|starting|running|stopping|error`)
- active script id/name
- last error string
- last timeout timestamp
- timer slot usage (`used/max`)
- vm memory summary (if available)

### Fault triage playbook

If animation stalls:

1. Query `/api/js/status`.
2. Run `js mem` and inspect pressure.
3. Stop script, start minimal script (`matrix.setPixel(...)`) to isolate renderer path.
4. If minimal script works, issue is likely user script logic/timing.
5. If minimal script fails, inspect matrix engine status and SPI clock.

If callbacks stop firing:

1. Check timer slot exhaustion.
2. Inspect queue backpressure (`mqjs_service_post` timeouts).
3. Confirm callback exceptions are not canceling scheduling loop.

## Detailed Validation Matrix

The following matrix should be used as acceptance gates before calling this integration complete.

### Build and boot validation

- Cold boot with no Wi-Fi credentials: console available, matrix idle.
- Boot with saved credentials: obtains IP and REST serves UI.
- JS service startup failure path returns explicit errors without crash.

### API contract validation

- `POST /api/js/eval` small valid script => `ok=true`.
- `POST /api/js/eval` syntax error => `ok=false`, parse error.
- `POST /api/js/eval` oversized body => HTTP 413.
- `POST /api/js/reset` while idle and while running => consistent success/failure policy.

### Timing validation

- 10-minute run at 20 FPS with stable frame pacing.
- 10-minute run at 30 FPS with no watchdog resets.
- Stress run with rapid stop/start loop from REST and console.

### Concurrency validation

- Simultaneous REST requests for status and eval do not deadlock.
- Timer callbacks firing during stop transition do not resurrect stopped script.
- Matrix built-in C animations still work after JS runtime reset.

### Fault-injection validation

- Inject infinite JS loop and confirm deadline interruption works.
- Inject callback throwing exception; confirm service stays alive.
- Fill timer slots to max and confirm graceful rejection behavior.

### Hardware-path validation

- Verify orientation controls still function in JS and legacy endpoints.
- Verify punctuation and symbol rendering using current glyph map.
- Verify blank text/space-only strings are handled per defined semantics.

## Rollout Plan for Intern Contributors

To reduce risk and onboarding overhead, assign work in vertical slices rather than one giant branch.

Slice 1: Runtime plumbing only

- Build adds `mqjs_service`.
- Add `/api/js/eval` simple echo semantics.
- No matrix bindings yet.

Slice 2: Low-level matrix bindings

- Implement `setPixel/clear/present`.
- Add sample scripts and console examples.
- Validate frame pacing.

Slice 3: High-level wrappers and compatibility

- Add `startScroll/startDrop` wrappers.
- Keep existing `/api/matrix/*` behavior unchanged.

Slice 4: observability and hardening

- Add status/mem endpoints.
- Add structured runtime state and fault telemetry.

Slice 5: polish and docs

- Finalize examples, error messages, and operator runbook.

This staged strategy allows quick user-visible progress while preserving system stability.

## Alternatives Considered

### Alternative 1: Run JS directly in HTTP task

Rejected because:

- Blocks HTTP responsiveness.
- Risks VM reentrancy when multiple requests overlap.
- Harder to support callbacks/timers safely.

### Alternative 2: Replace C matrix engine with JS-only renderer

Rejected because:

- Duplicates mature hardware mapping logic.
- Higher CPU overhead for all animation paths.
- Harder to preserve existing REST/console compatibility.

### Alternative 3: External scripting host (off-device)

Rejected for this ticket because:

- Removes offline autonomy.
- Increases network dependency and latency.
- Conflicts with requirement to send scripts and run locally on ESP.

## Risks and Mitigations

Risk: Script monopolizes CPU.

- Mitigation: eval deadlines + cooperative sleep primitives + stop token.

Risk: Memory exhaustion in VM.

- Mitigation: fixed arena budget + `js mem` diagnostics + strict body limits.

Risk: Timing jitter during Wi-Fi spikes.

- Mitigation: keep draw path compact, prefer FPS sync, avoid heavy JSON in per-frame path.

Risk: API complexity for interns.

- Mitigation: layered API and examples from simple to advanced, plus console examples.

## Recommended API Quick Reference (Draft)

```text
matrix.width() -> int
matrix.height() -> int
matrix.clear() -> void
matrix.fill(on:boolean) -> void
matrix.setPixel(x:int, y:int, on:boolean) -> void
matrix.getPixel(x:int, y:int) -> boolean
matrix.present() -> void

matrix.drawChar(ch:string, x:int, y:int) -> void
matrix.drawText(text:string, x:int, y:int, spacing?:int) -> void
matrix.measureText(text:string) -> int

matrix.setText(text:string) -> void
matrix.startScroll(text:string, opts?:{fps?:int,pauseMs?:int,repeat?:int,wave?:bool}) -> void
matrix.startDrop(text:string, opts?:{fps?:int,pauseMs?:int,repeat?:int}) -> void
matrix.stop() -> void

matrix.setIntensity(v:int) -> void
matrix.setOrientation(opts:{reverse?:bool,flipv?:bool}) -> void
matrix.status() -> object

matrix.nowUs() -> int64
matrix.sleepMs(ms:int) -> void
matrix.sleepUntilUs(tUs:int64) -> void
matrix.shouldStop() -> bool
```

## Open Questions

1. Should long-running scripts be single-instance global, or multiple named jobs with arbitration?
2. Should REST expose authenticated mode for script endpoints in non-dev deployments?
3. Should `matrix.present()` be immediate or rate-limited by engine policy?
4. Should scripts be persisted in NVS/SPIFFS for reboot restore, or transient only?
5. Should API include direct column-buffer uploads for maximum throughput?

## References

Code files inspected:

- `0067-esp-c3-led-matrix-http/main/app_main.c`
- `0067-esp-c3-led-matrix-http/main/http_server.c`
- `0067-esp-c3-led-matrix-http/main/matrix_engine.c`
- `0067-esp-c3-led-matrix-http/main/matrix_console.c`
- `0067-esp-c3-led-matrix-http/main/Kconfig.projbuild`
- `components/mqjs_service/include/mqjs_service.h`
- `components/mqjs_service/mqjs_service.cpp`
- `components/mqjs_service/include/mqjs_vm.h`
- `components/mqjs_service/mqjs_vm.cpp`
- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`
- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp`
- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
- `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
