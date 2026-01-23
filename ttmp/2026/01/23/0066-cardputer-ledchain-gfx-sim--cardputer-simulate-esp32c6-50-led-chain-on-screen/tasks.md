# Tasks

## TODO

- [x] MVP: create new firmware project `0066-cardputer-adv-ledchain-gfx-sim` (builds, flashes, boots)
- [x] MVP: console REPL over USB Serial/JTAG; `sim` command skeleton (`help`, `status`)
- [x] MVP: integrate canonical pattern engine (`components/ws281x`) without real WS281x output
- [x] MVP: implement virtual strip buffer (50 LEDs) and RGB→RGB565 rendering (no gamma)
- [x] MVP: render 50-LED chain on screen (choose initial topology: 10×5 grid)
- [x] MVP: console commands to set patterns + params (off/rainbow/chase/breathing/sparkle)
- [x] MVP: basic on-screen overlay (pattern name + brightness + frame_ms)
- [x] MVP: smoke test on hardware (Cardputer) and record validation notes in diary

## Later (post-MVP)

- [ ] Optional: keyboard controls for pattern switching (Cardputer keyboard component)
- [ ] Optional: receive MLED/1 over UDP multicast to mirror ESP32-C6 control plane
- [ ] Optional: show-time sync (`execute_at_ms`) + cue semantics for true mirroring
- [ ] Optional: gamma correction toggle to better match perceived LED brightness

## Web Server

- [x] Add Wi-Fi console commands (`wifi ...`) in the same REPL as `sim ...`
- [x] Start `esp_http_server` after Wi-Fi got IP
- [x] Add `GET /api/status` (JSON)
- [x] Add `POST /api/apply` (JSON) to set pattern + params + brightness + frame_ms
- [x] Serve a minimal `/` HTML control page (embedded asset)
- [ ] Smoke test: connect to Wi-Fi from console and hit `/api/status`

## MicroQuickJS

- [x] Integrate `mquickjs` (MicroQuickJS) into 0066 firmware
- [x] Generate a 0066-specific stdlib with a `sim` global object for pattern control
- [x] Add `js` console command for `eval` + `repl` + `stats` + `reset`
- [x] Expose simulator control APIs to JS (`sim.setPattern(...)`, etc.)
- [x] Smoke test on hardware: `js eval sim.status()` and setting patterns/brightness
- [ ] Optional: add `sim.statusJson()` (or seed `JSON.stringify(sim.status())` helper)
- [ ] Optional: add SPIFFS-backed `load(path)` + `:autoload` for JS libraries

## MQJS Timers + GPIO Sequencer

- [ ] Design: setTimeout / interval callbacks + sequencing (this doc)
- [ ] Implement: `setTimeout(fn, ms)` and `clearTimeout(id)`
- [ ] Implement: periodic callback primitive (`setInterval` or `every(ms, fn)`)
- [ ] Implement: GPIO G3/G4 toggle primitives (and explicit pin mapping)
- [ ] Document: example toggle sequences (JS snippets)

## Engine Task + RPC

- [ ] Design: move pattern engine into its own task + control queue + snapshots
- [ ] Implement: engine task owns `led_patterns_t` and publishes latest pixels snapshot
- [ ] Implement: synchronous RPC for config updates (console/web/js) with timeouts
- [ ] Implement: async notifications (optional) without engine↔JS deadlocks

---

# Detailed Implementation Tasks (Timers + GPIO + Engine Task)

Design decisions captured from user (2026-01-23):
- GPIO “G3/G4” refers to **board labels**, not ESP32 GPIO numbers (mapping must be identified and documented).
- Timing: **ms resolution is OK**, jitter is acceptable.
- JS execution: move to **`mqjs_service` VM-owner task** (no JS execution inside `esp_console` handler).
- Timer API: implement **`every(ms, fn)`** (not `setInterval` initially); `setTimeout` arg handling: **easiest**.
- Engine task semantics: blocking means **ack/enqueued** (not “rendered a frame”).
- Engine tick: **60 Hz**, UI `frame_ms` remains the pacing knob for on-screen refresh.
- UI consumption: **latest-frame snapshot** (double-buffer), not a frame queue.

## Phase 0 — Prep / Pin Mapping

- [x] Add Kconfig knobs for JS arena + G3/G4 mapping (`CONFIG_TUTORIAL_0066_JS_MEM_BYTES`, `CONFIG_TUTORIAL_0066_G3_GPIO`, `CONFIG_TUTORIAL_0066_G4_GPIO`)
- [ ] Find the actual ESP32-S3 GPIO numbers corresponding to Cardputer board labels **G3** and **G4**
  - [ ] Search this repo for prior usage of “G3/G4” in Cardputer projects
  - [ ] If not found, consult M5Cardputer / Cardputer-ADV docs/schematics and record the mapping
  - [ ] Decide + document electrical mode (push-pull output, active-high; note if configurable)
  - [ ] Add a short “pin mapping” note into the ticket README/design doc

## Phase 1 — Move JS execution onto `mqjs_service` (foundation)

- [x] Add a 0066-specific `js_service` wrapper that starts `mqjs_service` with `js_stdlib`
  - [x] Choose arena size (Kconfig `CONFIG_TUTORIAL_0066_JS_MEM_BYTES` or fixed default)
  - [x] Provide `js_service_eval(...)` and `js_service_post(job...)` wrappers
- [x] Update `js eval` to call `mqjs_service_eval` (blocking) instead of direct `JS_Eval`
- [x] Update `js repl` to use `mqjs_service_eval` for each line (still line-oriented; no second serial reader)
- [x] Ensure all existing `sim.*` JS APIs keep working under the service model
- [ ] Smoke test script (ticket `scripts/`):
  - [x] Update/add `serial_smoke_js_0066.py` to validate `js eval sim.status()` works via the service
- [x] Commit: “0066: run JS on mqjs_service task”

## Phase 2 — Timers: `setTimeout/clearTimeout` + `every(ms, fn)`

- [x] Decide callback storage strategy: JS-side registry under `globalThis.__0066` (GC rooting)
  - [x] Create bootstrap job that ensures `__0066.timers = { cb: {}, ... }`
- [x] Implement `setTimeout(fn, ms)` and `clearTimeout(id)`
  - [x] Validate args and bounds (`ms >= 0`, cap max timers, cap queue pressure)
  - [x] Define semantics: “enqueue + best-effort execution”; support cancellation
  - [x] On callback throw: **log and cancel that timer/periodic handle** (choice)
- [x] Implement `every(ms, fn)` and `cancel(handle)`
  - [x] Return an integer id or small handle object (pick simplest)
  - [x] Implement coalescing/overrun behavior (skip vs catch-up); pick simplest “skip if late”
- [x] Timer kernel architecture
  - [x] Use a single scheduler task or `esp_timer` “next deadline” to wake scheduler
  - [x] Never execute JS from timer callback; only post jobs into `mqjs_service`
- [x] Add smoke script (ticket `scripts/`):
  - [x] Verify `every(50, fn)` increments an in-JS counter and can be canceled
- [x] Commit per step (bootstrap, setTimeout, every)

## Phase 3 — GPIO: G3/G4 toggle primitives for JS

- [x] Implement GPIO init module for the two pins (as outputs)
  - [x] Explicitly document mapping “G3/G4 → GPIO<num>”
  - [ ] Ensure does not conflict with USB Serial/JTAG console pins (per repo guidance)
- [ ] Add JS API surface (pick minimal):
  - [x] `gpio.toggle("G3")`, `gpio.write("G3", 0|1)` (userland) + `gpio.high/low` (native)
- [ ] Add example sequences (pure JS userland) using `every` / `setTimeout`
- [ ] Add smoke script (ticket `scripts/`):
  - [x] `gpio.write('G3',0)` and `gpio.toggle('G3')` best-effort exercise (no external validation)
- [x] Commit per step

## Phase 4 — Sequencer layer (userland-first)

- [ ] Add a “sequencer cookbook” doc in the ticket (JS snippets)
  - [ ] pulse N times
  - [ ] alternating G3/G4 pattern
  - [ ] simple state machine driven by `every`
  - [ ] cancellation patterns (prevent runaway)
- [ ] Optional (later): native declarative sequencer object (only if userland becomes unwieldy)

## Phase 5 — Engine Task + Control Queue (refactor)

- [x] Add an `engine_task` that owns the pattern state and renders frames at **60 Hz**
  - [x] Maintain `frame_ms` as UI pacing; engine tick is fixed 60 Hz
  - [x] Publish a **latest-frame snapshot** (double-buffer) for UI + web + status
- [x] Add an engine control queue with message types:
  - [x] set pattern cfg
  - [x] set frame_ms (UI pacing value)
  - [ ] get status (optional)
- [x] Define blocking semantics: JS/console “blocking” = **ack/enqueued** (with timeout)
- [x] Update existing subsystems to use the queue:
  - [x] `sim ...` console commands enqueue set-cfg / set-frame
  - [x] HTTP `/api/apply` enqueues set-cfg / set-frame
  - [x] JS `sim.*` APIs enqueue set-cfg / set-frame (blocking + non-blocking variants)
- [x] Remove/limit direct mutex-based writes once queue path is authoritative
- [x] Smoke test (hardware):
  - [x] verify UI still animates
  - [x] verify console + web + JS still control patterns
- [x] Commit per step (engine task skeleton, snapshot, queue write path, migration)

## Phase 6 — Optional async engine→JS notifications (no deadlocks)

- [ ] Add “event posting” from engine to JS as **mqjs_service jobs** (never blocking engine)
  - [ ] Coalesce events to avoid flooding (at most one pending “frame event”)
  - [ ] Provide JS registration API (`engine.on("frame", fn)` or similar)

## Phase 7 — Documentation + reMarkable uploads

- [ ] Update ticket design docs if decisions shift (pin mapping, timer semantics)
- [ ] Keep diary entries at each phase (exact commands, failures, validations)
- [x] Upload updated ticket bundle to reMarkable after major milestones
