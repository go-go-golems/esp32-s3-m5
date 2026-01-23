---
Title: "Postmortem: MicroQuickJS integration (0066)"
Ticket: 0066-cardputer-ledchain-gfx-sim
Status: draft
Topics:
  - microquickjs
  - mqjs_service
  - console
  - timers
  - gpio
DocType: reference
Intent: retrospective
Owners: []
RelatedFiles:
  - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.cpp
    Note: Console UX for `js eval|repl|reset|stats`
  - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp
    Note: VM-owner task wiring + bootstrap helpers
  - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c
    Note: Native bindings (`sim.*`, `setTimeout`, GPIO)
  - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp
    Note: One-shot timer scheduler task posting jobs into JS service
  - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/reference/01-diary.md
    Note: Source-of-truth chronological diary; this doc is an engineering synthesis
  - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/tasks.md
    Note: Task checklist for Phase 0..7
  - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/scripts/serial_smoke_js_0066.py
    Note: Reproducible smoke test for JS + timers + GPIO via /dev/ttyACM0
Summary: "Retrospective on adding a MicroQuickJS REPL, then migrating to mqjs_service, then adding timers and GPIO control."
LastUpdated: 2026-01-23
---

# Postmortem: MicroQuickJS work so far (Ticket 0066)

This is a detailed engineering retrospective of the JavaScript work done in the `0066-cardputer-adv-ledchain-gfx-sim` firmware: what changed, why it changed, what we learned about the MicroQuickJS runtime in this repo, and what risks/next steps are implied by the current implementation.

It is deliberately more “systems-engineering narrative” than the diary: the diary preserves chronology, while this document tries to preserve *mechanics* and *design invariants*.

## 0) Context and intended end-state

The “0066 simulator” is a Cardputer (ESP32-S3) firmware that draws a virtual 50-LED chain on-screen and runs the same pattern math as the ESP32-C6 WS281x firmware. The user intent for the scripting layer is:

- Provide an interactive, on-device scripting environment to drive the simulator and external pins.
- Be able to write sequences (think: small test rigs / choreography) in JS.
- Maintain correct concurrency: the simulator, web server, console, timer callbacks, and future engine task must not execute JS from arbitrary contexts.

The key “concurrency contract” we set early (and enforced in implementation) is:

> **All MicroQuickJS calls must happen on a single VM-owner task.**

Everything else (console commands, timers, GPIO “events”, future engine notifications) communicate to that owner via bounded queues.

## 1) Timeline of milestones (commit-driven)

The JS work evolved in three major steps:

1) **Direct MicroQuickJS REPL + sim bindings** (initial integration)
2) **Migration to `mqjs_service` (VM-owner task)** (foundational refactor)
3) **Timers + `every()` and GPIO control** (capabilities added on top)

Concrete commit landmarks (chronological):

- `d3f54a2` — `0066: add MicroQuickJS console + sim JS API`
  - Introduced `js` console command and `sim.*` bindings.
- `6b95217` — `0066: add mqjs stdlib + JS smoke scripts`
  - Added generated stdlib artifacts and ticket scripts for reproducible testing.
- `45612f6` — `0066: run JS via mqjs_service`
  - The major concurrency refactor: JS moved off the console handler thread.
- `e50d028` — `0066: add JS timers (setTimeout + every)`
  - Introduced timer scheduler task + `setTimeout` + `every/cancel`.
- `d22d698` — `0066: add JS GPIO control for G3/G4`
  - Implemented GPIO primitives and userland convenience helpers.

The diary and tasks checklist commits around these include validation details and rationale:

- `1e23222`, `c9ab4b8`, `14839f9` mark Phase completion/progress in ticket docs.

## 2) “What we shipped” (current JS surface area)

### Console UX

The primary user entry point is the console command:

- `js help|eval|repl|reset|stats`

Implementation location:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.cpp`

Operationally:

- `js eval <code>` runs one evaluation and prints output or prints an exception.
- `js repl` is a simple line-by-line REPL inside the console session (not a second serial reader).
- `js reset` recreates the JS VM.
- `js stats` prints MicroQuickJS memory statistics.

### JS-visible APIs (MVP scope)

#### Simulator control

Global object:

- `sim.*` (native bindings)

Implementation location:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`

Representative calls:

- `sim.status()`
- `sim.setFrameMs(ms)`
- `sim.setBrightness(pct)`
- `sim.setPattern("rainbow"|"chase"|"breathing"|"sparkle"|"off")`
- `sim.setRainbow(...)`, `sim.setChase(...)`, etc.

#### Timers

Primitives:

- `setTimeout(fn, ms)` → returns numeric id
- `clearTimeout(id)`

Convenience (JS bootstrap):

- `every(ms, fn)` → returns `{ id, cancelled, cancel() }`
- `cancel(handleOrId)` → accepts either numeric timeout id or `{cancel()}`

Implementation locations:

- Native: `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
- Scheduler task: `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp`
- JS bootstrap: `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`

#### GPIO (board labels G3/G4)

Native primitives (minimal, stable, hardware-facing):

- `gpio.high("G3"|"G4")`
- `gpio.low("G3"|"G4")`

Userland convenience (built on top, no native read required):

- `gpio.write("G3"|"G4", 0|1)`
- `gpio.toggle("G3"|"G4")`

Implementation locations:

- Native: `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
- JS bootstrap helpers: `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`

GPIO mapping is configurable (board labels, not raw ESP32 GPIO numbers):

- `CONFIG_TUTORIAL_0066_G3_GPIO` (default 3)
- `CONFIG_TUTORIAL_0066_G4_GPIO` (default 4)

Kconfig location:

- `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild`

## 3) Architecture: from “direct JS context” to “JS service task”

### The problem with the initial approach

The initial `MqjsEngine` approach (direct `JS_NewContext` / `JS_Eval` inside the console command handler) works for synchronous “type and run a line of JS”.

It fails (or becomes dangerously brittle) once you add:

- timers that fire later,
- GPIO-driven sequencing,
- future engine-task callbacks,

because those events originate outside the console call stack. If they try to execute JS directly, you quickly risk:

- executing JS concurrently from multiple tasks,
- calling into MicroQuickJS from ISR-like contexts,
- deadlocks if any callback tries to call back into the source task.

### The adopted pattern (`mqjs_service`)

We reused the repo’s canonical pattern for JS execution:

- `components/mqjs_service` (a queue + a dedicated FreeRTOS task)
- `components/mqjs_vm` (MicroQuickJS VM host utilities: printing values, exception strings, deadlines)

In `0066`, the wiring is encapsulated in:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`

The critical invariants are:

- **Exactly one task owns the JS VM** (`mqjs_service` task).
- Everyone else sends either:
  - a synchronous eval request (`mqjs_service_eval`), or
  - an asynchronous job (`mqjs_service_post`) to run on the VM owner.

The console `js eval` and `js repl` still appear synchronous to the user, but internally they are “RPC into the JS service”.

### Practical implications / limits

1) REPL is still line-oriented (good for MVP).
2) If JS code blocks forever (tight loop), the service becomes unresponsive; we partially mitigate this via `MqjsVm::SetDeadlineMs()` (timeouts).
3) Anything that posts jobs must assume the queue is bounded; “best-effort” semantics are acceptable for timers in MVP.

## 4) Timers: design, implementation, and lessons

### Why this design (scheduler task + post-to-VM)

We deliberately avoided executing JS inside `esp_timer` callbacks or other timer contexts.

Instead:

1) A timer scheduler task (`0066_js_tmr`) keeps a small list of scheduled deadlines.
2) When a deadline expires, it posts a job into `mqjs_service`.
3) That job runs the JS callback on the VM-owner task.

This preserves the “single VM owner” invariant and keeps the timing machinery in C small.

### Callback rooting strategy (GC + lifetime)

MicroQuickJS needs callbacks to remain reachable so GC does not collect them before they fire.

We root callbacks by storing them in a global registry:

- `globalThis.__0066.timers.cb[id] = fn`

Then on fire:

- call `fn()`
- clear it to `null` (one-shot semantics)

### Why `every()` is JS-level

We did *not* add `setInterval` to the ROM table / generated stdlib (would require regenerating `esp32_stdlib.h` and expanding the C glue).

Instead:

- implement only `setTimeout/clearTimeout` natively (already present as stubs),
- build `every(ms, fn)` in JS as recursive `setTimeout`.

This keeps the native ABI stable and puts “policy” (periodic rescheduling) in userland, which is easier to evolve.

### MicroQuickJS API differences we learned (important!)

The MicroQuickJS headers in this repo are *not* the full upstream QuickJS API:

- There is **no** `JS_FreeValue` exposed.
- There is **no** `JS_DupValue` exposed.

Early attempts to write timer code “like QuickJS” failed compilation.

The practical lesson:

> Treat the MicroQuickJS API as its own embedded API, not as QuickJS-with-less-features.

And code style should follow existing patterns in:

- `imports/esp32-mqjs-repl/.../esp32_stdlib_runtime.c`
- `0048-cardputer-js-web/main/js_service.cpp` (job-posting and bootstrap patterns)

## 5) GPIO: design, implementation, and lessons

### Board labels vs GPIO numbers

The user requirement is explicit: “G3/G4” refer to **board labels**.

We therefore introduced explicit Kconfig mappings (defaults inferred from prior tutorial mapping):

- `G1 → GPIO1`, `G2 → GPIO2` precedent in `0047-cardputer-adv-lvgl-chain-encoder-list/main/Kconfig.projbuild`
- so `G3 → GPIO3`, `G4 → GPIO4` as defaults, but configurable.

### Native surface

We implemented the minimal native surface:

- `gpio.high("G3")`, `gpio.low("G3")` and same for G4

Everything else (write/toggle/state) is JS sugar.

This reduces the chance we “paint ourselves into a corner” with a prematurely large GPIO API.

### Validation approach

We validated via the existing serial smoke test harness:

- `ttmp/.../scripts/serial_smoke_js_0066.py --port /dev/ttyACM0`

and confirmed the IDF gpio driver logs show the pins configured as output.

## 6) Tooling and reproducibility

We prioritized “repeatable, pasteable validation” over ad-hoc monitoring:

- `ttmp/.../scripts/serial_smoke_js_0066.py` exercises:
  - `sim.*`
  - `setTimeout`
  - `every/cancel`
  - `gpio.write/toggle`
- Logs are stored under:
  - `ttmp/.../various/serial-smoke-js-YYYYMMDD-HHMMSS.log`

This makes regressions easy to spot when future changes (engine task refactor, web server changes, etc.) alter timing or console behavior.

## 7) Known gaps / technical debt (explicitly acknowledged)

This section is intentionally blunt: it lists areas that are “fine for MVP”, but will matter as we scale.

### Timer engine limitations

- Fixed maximum timers (`kMaxTimers=32`); schedule beyond that is rejected (logged).
- Best-effort job posting: if `mqjs_service_post` fails due to backpressure, the callback is dropped (logged).
- `every(...)` cancellation semantics are userland and rely on the returned handle; this is fine but implies:
  - no built-in “list timers” or “cancel all” yet,
  - misbehaving scripts can schedule too much work.

### GPIO layering

- No `gpio.read(...)` in native layer (by design, MVP).
- `gpio.toggle` uses a JS-side state variable, not the real pin state.
  - This avoids needing a native read, but it means external changes to pin state aren’t observed.

### Future engine-task refactor interaction

Currently, `sim.*` native bindings call `sim_engine_*` directly via a global `sim_engine_t*` stored in `mqjs/esp32_stdlib_runtime.c`.

If we move the pattern engine into its own task/queue (planned Phase 5), we must:

- replace direct calls with queue messages / RPC semantics,
- decide whether `sim.*` remains synchronous (ack/enqueued) or gains async variants,
- ensure we don’t introduce deadlocks (JS service → engine queue → callback → JS service).

## 8) What I would do next (JS-related)

If the next implementation step is “engine task + queue”, the JS implications are:

1) Introduce a thread-safe `sim_control` API (queue-based) that can be called from:
   - console commands,
   - HTTP server handlers,
   - JS service (native sim bindings),
   without taking a mutex around the pattern engine.
2) Keep JS semantics “blocking = ack/enqueued” to avoid JS waiting on frame boundaries.
3) Consider adding an explicit `sim.apply({ ... })` that maps naturally to the web `/api/apply` payload, to reduce duplicated parsing logic.

