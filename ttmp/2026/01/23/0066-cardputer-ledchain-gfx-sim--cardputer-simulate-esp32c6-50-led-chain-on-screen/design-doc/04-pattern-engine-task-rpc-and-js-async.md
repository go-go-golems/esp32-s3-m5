---
Title: "0066: Pattern Engine Task + RPC Queue + JS Sync/Async (Design + Analysis)"
Slug: 0066-pattern-engine-task-rpc-and-js-async
Short: "Design for moving the LED pattern engine into a dedicated owner task with a control queue, preserving synchronous (blocking) JS calls and enabling async JS callbacks."
Topics:
  - esp32s3
  - esp-idf
  - freertos
  - cardputer
  - leds
  - simulation
  - microquickjs
  - javascript
  - concurrency
  - rpc
IsTemplate: false
IsTopLevel: false
ShowPerDefault: false
SectionType: DesignDoc
---

# 0066: Pattern Engine Task + RPC Queue + JS Sync/Async (Design + Analysis)

## 0. Executive summary

We want to restructure 0066 so that the **pattern engine is owned by a single task** (the “engine task”), and all other subsystems (UI renderer, web server, JS REPL, console commands) interact with it through a **control queue** and explicit request/response patterns.

We also want two JS interaction modes to coexist:

1) **Synchronous (blocking) JS calls** like `sim.setPattern("rainbow")` that return when applied.
2) **Asynchronous JS callbacks** (timers, sequencing, “on frame” notifications) that run later on the JS VM task without blocking the engine.

The key concurrency rule set is:

- **Engine state has a single owner** (engine task).
- **JS VM has a single owner** (`mqjs_service` task).
- Engine→JS communication is **one-way notifications** (avoid deadlocks).
- JS→engine synchronous calls are **RPC** with timeouts.

This design is deliberately aligned with existing prior art in this repo:

- VM ownership: `components/mqjs_service` + `components/mqjs_vm`
- VM-side callbacks/events: `0048-cardputer-js-web/main/js_service.cpp`
- Current 0066 “direct mutex” model: `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.*` + `sim_ui.cpp`

---

## 1. Current architecture (baseline)

Today, 0066 uses a shared-object + mutex model:

- `sim_engine_t` holds pattern config (`led_pattern_cfg_t`), `led_patterns_t`, virtual WS281x pixel buffer, and `frame_ms`:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.h`
- UI rendering runs in its own task and calls into the engine directly:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
  - loop:
    - `sim_engine_render(...)` (lock, render to pixels, unlock)
    - `sim_engine_get_cfg(...)` (lock, copy cfg, unlock)
    - draw to screen
- Console and JS bindings mutate state by calling `sim_engine_set_cfg(...)` / `sim_engine_set_frame_ms(...)`:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_console.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`

This is simple and correct, but it has limitations:

- No explicit ordering semantics for “multiple writes in rapid succession”.
- No place to centralize “frame stepping” policy (e.g., decouple compute tick from UI tick).
- Harder to add “events” (frame boundary, pattern end, etc.) cleanly.

---

## 2. Why an engine task? (motivation)

Moving to an engine task provides:

1) **Single writer** discipline without broad mutex locking across the system.
2) A natural place to define:
   - tick rate,
   - “apply config at next frame boundary” semantics,
   - transition scheduling (future: execute_at_ms).
3) A natural place to emit **events**:
   - frame tick,
   - applied-config acknowledgements,
   - “pattern cycle” milestones.

Most importantly for your goal:

> It gives us a clean anchor to build JS-driven sequences: JS can send commands to the engine, while the engine can (optionally) generate callbacks/events back into JS without ever sharing mutable state.

---

## 3. Proposed architecture (high level)

We introduce three explicit “owner domains”:

### 3.1 Engine task (pattern owner)

Owns:

- `led_patterns_t`
- current `led_pattern_cfg_t`
- current `frame_ms`
- virtual WS281x pixel buffer

Exports:

- control queue for commands (`set_cfg`, `set_frame_ms`, etc.)
- a snapshot for readers (pixel buffer + current cfg + timing)

### 3.2 UI task (renderer)

Consumes a snapshot:

- reads most recent rendered pixel buffer (copy) or consumes a “frame queue” of pixel frames.
- does not call `led_patterns_render_to_ws281x` directly.

### 3.3 JS owner task (MicroQuickJS VM owner)

Owns:

- the JSContext
- the timer system (from the previous design doc)

Exports:

- an API to enqueue “call this JS callback with these args” jobs.

For implementation, we reuse:

- `components/mqjs_service` job primitive for all JS execution.

---

## 4. Engine control API: message types and semantics

The engine queue is the sole “write path” into the pattern state.

### 4.1 Messages (control-plane)

Define a C enum:

```c
typedef enum {
  ENGINE_MSG_SET_CFG,
  ENGINE_MSG_SET_FRAME_MS,
  ENGINE_MSG_GET_STATUS,     // optional (RPC)
  ENGINE_MSG_GET_PIXELS,     // optional (RPC)
} engine_msg_type_t;
```

Each message has:

- payload (cfg / ms / etc.)
- optional “reply handle” for RPC (binary semaphore + response buffer pointer)

### 4.2 Blocking semantics (“RPC”)

For synchronous JS calls, we want:

> When `sim.setPattern("rainbow")` returns, the engine has acknowledged that it accepted the change.

Two flavors of “ack”:

1) **Applied**: the config will take effect on the next engine tick boundary (or immediately).
2) **Committed**: the config has been applied and a frame has been rendered with it.

The design should choose one and document it. Suggested:

- **Applied** ack is enough for interactivity and keeps latency low.
- Optionally provide a separate `sim.flush()` to wait for a render boundary.

### 4.3 Asynchronous semantics (“fire-and-forget”)

For callbacks and fast sequences, we also want non-blocking commands:

- `sim.postSetPattern(...)` that returns immediately and queues the command.

This prevents JS timer callbacks from piling up waiting on engine acks.

---

## 5. Engine → consumers: how frames and state get out

There are three viable patterns; pick based on memory and desired decoupling.

### Option E1: “Latest frame” snapshot (double-buffer)

Engine renders into an internal buffer, then publishes a pointer or copies into a shared “latest frame” buffer protected by a small mutex (or atomic pointer swap).

UI reads:

- `latest_pixels[3*led_count]`
- `latest_cfg`
- `latest_frame_ms` / timestamps

Pros:

- simplest
- UI never blocks engine for long (copy is small: 150 bytes for 50 LEDs)

Cons:

- UI may skip frames (fine for a simulator)

### Option E2: Frame queue (producer/consumer)

Engine enqueues rendered frames into a queue; UI pops frames and renders each.

Pros:

- explicit backpressure and frame accounting

Cons:

- more RAM (N frames * 150 bytes)
- can stall engine if UI is slow unless queue drops

### Option E3: UI-driven “pull” (RPC getPixels)

UI asks engine for pixels each frame via RPC.

Pros:

- no duplication of pixel memory

Cons:

- UI loop becomes coupled to engine responsiveness (jitter)

Recommendation: E1 (double-buffer snapshot). It is the closest to the existing 0066 mental model and is stable.

---

## 6. JS synchronous calls + engine RPC: can we still block?

Yes. The design is:

1) JS binding runs on the **JS owner task** (mqjs_service).
2) Binding posts an engine message that includes an ack primitive:
   - `SemaphoreHandle_t done`
   - result code
3) Binding waits on `done` with a timeout:
   - returns normally, or throws on timeout.

This preserves a synchronous JS API:

```js
sim.setPattern("chase")     // waits for ack
sim.setBrightness(50)       // waits for ack
```

Note: “blocking in JS” in this embedded world means “the `js eval ...` request blocks until done”. There is no preemptive JS thread scheduler or promises; it’s just a synchronous call returning when the underlying C binding returns.

---

## 7. JS async: how callbacks work without deadlocks

### 7.1 How async enters the system

Async sources:

- timer due
- GPIO edge / button (future)
- engine frame tick (optional)

Each async source is *not allowed* to call JS directly. It must:

- enqueue a job into `mqjs_service` (VM owner).

This is exactly how 0048 does encoder callbacks:

- `mqjs_service_post(s_svc, &job_handle_encoder_delta)` etc.
  - `0048-cardputer-js-web/main/js_service.cpp`

### 7.2 Avoiding engine↔JS deadlocks

Golden rule:

- The engine task must **never** wait on the JS task.

If the engine wants to “notify JS”, it posts a job and returns. No waiting.

If JS wants to “command engine”, it can wait on engine ack. That’s safe because it’s one-directional waiting.

This prevents cycles:

- JS waiting for engine while engine waiting for JS → deadlock.

### 7.3 Optional: event coalescing

If engine emits “frame” events at 60Hz, it can overwhelm the JS queue. Therefore:

- either do not emit per-frame events initially,
- or coalesce (only one pending “frame event” at a time, like 0048’s `s_enc_delta_pending` pattern).

---

## 8. Where to put the GPIO sequencing layer in this architecture

Given an engine task exists, GPIO sequencing can be done in two ways:

### 8.1 “GPIO is not the engine”: independent subsystem

GPIO toggles do not need to go through the pattern engine queue; they can be immediate, but should still be safe:

- JS binding posts a “gpio toggle” request to a small GPIO task or executes directly (fast).

Timers schedule toggles by posting “toggle” jobs.

### 8.2 “GPIO sequencing is a timeline”: unify under engine task

If you want “patterns + GPIO toggles share a single notion of time and ordering”, you can push GPIO operations through the engine queue as well:

- engine becomes the “timeline coordinator”.

This is more coherent if later you add `execute_at_ms`.

Trade-off: engine becomes more than “LED patterns”; it becomes “sequencer”.

---

## 9. Implementation plan (phased)

### Phase 1: Introduce engine task + snapshot output (no JS changes)

- Move `led_patterns_render_to_ws281x` calls into engine task.
- Publish latest pixels+cfg snapshot.
- Update UI to read snapshot only.

### Phase 2: Convert console + web + sim bindings to use engine queue writes

- Replace direct `sim_engine_set_cfg` calls with `engine_rpc_set_cfg` (with ack).

### Phase 3: Move JS execution onto mqjs_service (if not already)

0066 currently executes JS directly in the `esp_console` handler. For timers/async, we will need:

- `mqjs_service_start(...)`
- `js eval` becomes `mqjs_service_eval(...)`
- JS callbacks become jobs.

### Phase 4: Add timers + async callbacks

Per the prior doc:

- implement `setTimeout` and periodic primitives in a timer module that posts mqjs_service jobs
- keep callback registry in JS namespace `__0066`

### Phase 5: Add G3/G4 toggle APIs and sequencing helpers

Add the GPIO APIs, then provide example userland sequences.

---

## 10. Code reuse map (specific files)

VM owner task + jobs + deadlines:

- `components/mqjs_service/mqjs_service.cpp`
- `components/mqjs_vm/mqjs_vm.cpp`

Event coalescing + callback invocation idioms:

- `0048-cardputer-js-web/main/js_service.cpp`
  - `call_cb`, `get_encoder_cb`, `mqjs_service_post`, coalescing flag pattern

Current 0066 UI loop (to refactor):

- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`

Current 0066 state model (to replace/encapsulate):

- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.*`

---

## 11. Open questions (to settle before implementation)

1) What exactly are “G3” and “G4” on Cardputer-ADV:
   - board labels vs ESP32 GPIO numbers
   - confirm pin mapping + whether they are safe to use as outputs
2) Should synchronous JS calls wait for “applied” or “committed to a frame”?
3) Desired timer resolution (ms vs sub-ms) and acceptable jitter.
4) Whether engine should emit events into JS at all (and at what rate).

