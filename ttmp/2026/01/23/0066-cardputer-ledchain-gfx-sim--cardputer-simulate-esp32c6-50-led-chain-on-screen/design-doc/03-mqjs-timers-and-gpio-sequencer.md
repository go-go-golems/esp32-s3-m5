---
Title: "0066: MicroQuickJS Timers + GPIO Sequencer (Design + Analysis)"
Slug: 0066-mqjs-timers-and-gpio-sequencer
Short: "Design for setTimeout/setInterval and higher-level GPIO toggle sequencing (G3/G4), grounded in existing mqjs_service and 0048 callback patterns."
Topics:
  - esp32s3
  - esp-idf
  - cardputer
  - microquickjs
  - javascript
  - timers
  - gpio
  - sequencing
IsTemplate: false
IsTopLevel: false
ShowPerDefault: false
SectionType: DesignDoc
---

# 0066: MicroQuickJS Timers + GPIO Sequencer (Design + Analysis)

## 0. Executive summary

We want two new capabilities:

1) **GPIO toggling primitives** for two pins (“G3” and “G4”).
2) **JS callbacks scheduled in time**, i.e. `setTimeout` / periodic callbacks, enabling users to define **toggle sequences** in JavaScript.

The core engineering reality is that **MicroQuickJS code must run on a single VM owner task**. Therefore timers are not “just an API”; they are an execution model.

This document proposes:

- a **minimal, robust timer runtime** for MicroQuickJS on ESP32-S3,
- three layers of “sequencing” built on top of it (pure JS, hybrid, and native),
- and a migration plan that reuses existing components:
  - `components/mqjs_service` + `components/mqjs_vm`
  - callback calling patterns from `0048-cardputer-js-web/main/js_service.cpp`
  - console approach in `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.cpp`

No code is implemented in this step; this is design only.

---

## 1. Context and references (what exists today)

### 1.1 0066 current state (baseline)

0066 currently provides:

- a simulator core: `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.h`
- a USB Serial/JTAG `esp_console` REPL owned by `wifi_console`:
  - `components/wifi_console/wifi_console.c`
- a MicroQuickJS surface via an `esp_console` command:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.cpp`
  - and the JS API under a global `sim` object (pattern control).

Importantly: 0066 currently **stubs out** `setTimeout/clearTimeout` in the MicroQuickJS stdlib runtime:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`

### 1.2 Existing reusable JS ownership component: mqjs_service

Ticket 0055 produced a reusable queue-owned MicroQuickJS “service task”:

- `components/mqjs_service/include/mqjs_service.h`
- `components/mqjs_service/mqjs_service.cpp`
- built on `components/mqjs_vm` (deadline + logging capture).

The essential invariant (quoted as a design principle) is stated in:

- `ttmp/2026/01/21/0055-MQJS-SERVICE-COMPONENT--reusable-queue-owned-microquickjs-service-eval-callbacks-event-flush/design-doc/01-design-implementation-plan.md`

> “without ever calling MicroQuickJS from multiple tasks concurrently.”

### 1.3 Existing pattern for JS callbacks triggered by events

Firmware 0048 contains a mature example of “events originate elsewhere, JS callbacks run on VM thread”:

- `0048-cardputer-js-web/main/js_service.cpp`

It demonstrates three relevant patterns:

1) **bootstrap JS** that registers callbacks in a stable namespace:
   - writes into `globalThis.__0048.encoder.on_delta`, etc.
2) **retrieve callback function values** by property lookup (late binding):
   - `get_encoder_cb(...)`
3) **invoke callbacks safely** using `JS_Call` on the VM thread:
   - `call_cb(...)`

This is the template we should reuse for timers.

---

## 2. Requirements (what we want)

### 2.1 User-visible requirements

- **GPIO**: Provide JS functions to toggle two pins, “G3” and “G4”.
- **Timers**: Provide `setTimeout` and a “periodic callback” mechanism:
  - either `setInterval`, or `setInterval` implemented in JS on top of `setTimeout`.
- **Sequencing**: It must be possible to express deterministic sequences, e.g.:

  - “toggle G3 five times with 100 ms spacing”
  - “toggle G3 then toggle G4 after 50 ms, repeat”
  - “run a pulse pattern table”

### 2.2 Non-functional requirements

- Must not corrupt the USB Serial/JTAG console (still one REPL owner).
- Must not run MicroQuickJS concurrently from multiple tasks.
- Should not block the simulator UI/pattern rendering for long periods.
- Reasonable safety against runaway scripts:
  - deadlines / timeouts
  - bounded queue growth
  - bounded “timer backlog”

---

## 3. The fundamental constraint (and why it matters)

> Callout: **You cannot run JS in the ESP timer callback.**

ESP-IDF `esp_timer` callbacks execute in a system timer task context (or otherwise not under our control), and they must remain short and non-blocking. Calling MicroQuickJS from there violates:

- thread ownership,
- stack assumptions,
- and can deadlock or corrupt the VM.

Therefore, the design pattern is:

1) A timer fires and enqueues a small “job token” (or notifies) to the **JS owner task**.
2) The JS owner task pops due timers and calls JS functions.

This is exactly the pattern already baked into `components/mqjs_service`.

---

## 4. Timer implementation options (three viable kernels)

We have three ways to build timers. The “best” answer depends on desired accuracy, CPU overhead, and integration simplicity.

### Option T1: Dedicated JS-timers task (custom owner task)

Create a new FreeRTOS task that owns:

- the MicroQuickJS context
- a timer priority queue
- a wakeup mechanism

All `js eval ...` requests become “jobs” sent to this task.

Pros:
- maximum control
- fewer dependencies (no mqjs_service)

Cons:
- duplicates what `mqjs_service` already provides (queue, deadlines, single-owner VM discipline)
- more bespoke code to maintain

### Option T2: Use `components/mqjs_service` + a separate timer scheduler (recommended)

Keep a **single MicroQuickJS VM owner task** provided by `mqjs_service`.
Implement timers as a lightweight scheduler module that:

- owns timer state (ids, due times, intervals),
- and when timers are due, posts `mqjs_job_t` callbacks into the mqjs_service queue.

Pros:
- directly reuses the proven single-owner VM mechanism
- consolidates all VM access through one component

Cons:
- requires a clean “how do I call a JS function by reference?” story (see §6)

### Option T3: FreeRTOS software timers (`xTimerCreate`) + mqjs_service job posting

Instead of managing due times yourself, create one FreeRTOS timer per JS timer.
Each timer callback posts to `mqjs_service`.

Pros:
- conceptually simplest

Cons:
- heavy for many timers (one FreeRTOS timer object per `setTimeout`)
- still can’t run JS inside the callback
- complexity around cancellation and memory pressure

---

## 5. Timer API surface options (what JS sees)

The runtime kernel is independent from the JS API shape. Here are three API surfaces.

### API A: Browser-like

```js
var id = setTimeout(fn, ms)
clearTimeout(id)
var id2 = setInterval(fn, ms)
clearInterval(id2)
```

Pros:
- familiar
- easy to learn

Cons:
- requires either:
  - stdlib regeneration to add `setInterval/clearInterval`, or
  - a JS polyfill that implements interval using timeout.

### API B: Single primitive + library polyfills

Expose only:

```js
var id = setTimeout(fn, ms)
clearTimeout(id)
```

Then provide a shipped “stdlib JS” library (later) that defines:

```js
function setInterval(fn, ms) { ... }  // implemented using setTimeout recursion
function clearInterval(id) { ... }
```

Pros:
- minimizes required native bindings
- allows rapid iteration in JS space

Cons:
- we need a JS library loading mechanism (SPIFFS `load(...)` or embedded “bootstrap JS”)

### API C: Explicit scheduler object

```js
var t = timers.every(ms, fn)   // returns handle
t.cancel()
```

Pros:
- more explicit ownership

Cons:
- more new surface area, less familiar

---

## 6. The hard part: storing callbacks safely (GC rooting)

If we schedule JS functions for later execution, we must keep them alive.

There are two general strategies:

### Strategy R1: Store callback functions in a global registry object (recommended)

We create (or reuse) a stable namespace object, e.g.:

```js
globalThis.__0066 = globalThis.__0066 || {}
__0066.timers = __0066.timers || {}
__0066.timers.cb = __0066.timers.cb || {}
```

Then when `setTimeout(fn, ms)` is called, we:

1) assign a unique integer id `tid`
2) store `fn` at `__0066.timers.cb[tid] = fn`
3) return `tid`

When the timer fires, we:

1) fetch the function: `fn = __0066.timers.cb[tid]`
2) call it with `JS_Call`
3) delete it if one-shot: `__0066.timers.cb[tid] = null` (or delete)

This is close in spirit to 0048’s `__0048.encoder.on_delta`.

### Strategy R2: Store callbacks in C memory and treat them as “opaque”

In upstream QuickJS you might use `JS_DupValue` / `JS_FreeValue` reference counting.
MicroQuickJS in this repo is smaller and its public API surface is limited; if we cannot robustly “pin” a JSValue from C, Strategy R1 is safer and uses only:

- `JS_GetGlobalObject`
- `JS_GetPropertyStr` / `JS_SetPropertyStr`

---

## 7. How timers would be implemented with mqjs_service (recommended plan)

This section is “the algorithm”.

### 7.1 Proposed module boundaries

- **mqjs_service** (existing): owns VM + executes jobs.
- **mqjs_timers** (new): owns timer state and posts jobs into mqjs_service.
- **gpio_g34** (new): configures and toggles two pins.

### 7.2 Data model (C-side)

We want a timer wheel/heap keyed by due time:

```c
typedef enum { ONESHOT, INTERVAL } timer_kind_t;

typedef struct {
  uint32_t id;
  int64_t due_us;         // esp_timer_get_time-based
  uint32_t interval_ms;   // 0 for oneshot
  bool cancelled;
} mqjs_timer_t;
```

We also need:

- a monotonic time source: `esp_timer_get_time()`
- an id generator: incrementing `uint32_t`
- a bounded maximum timers count to avoid OOM

### 7.3 Wakeup mechanism choices

We can schedule wakeups in two ways:

**W1: One-shot esp_timer for next deadline**

- maintain `next_due_us`
- arm `esp_timer_start_once` for `(next_due_us - now_us)`
- timer callback posts “tick” into a queue

Pros: low CPU overhead, good timing
Cons: needs careful re-arming logic

**W2: Fixed-period tick (e.g., 1ms or 5ms)**

- like LVGL tick in `0047-cardputer-adv-lvgl-chain-encoder-list/main/lvgl_port_m5gfx.cpp`

Pros: simpler, bounded logic
Cons: constant wakeups even when no timers; worse power

Recommendation: W1 for oneshots/intervals with ms resolution.

### 7.4 Calling the JS function (job posted into mqjs_service)

When a timer becomes due, the timer subsystem posts a job:

```c
mqjs_job_t job = {
  .fn = mqjs_job_invoke_timer_cb,
  .user = (void*)(uintptr_t)tid,
  .timeout_ms = 25,
};
mqjs_service_post(svc, &job);
```

In the job function:

1) resolve `__0066.timers.cb[tid]` (property lookup)
2) if it’s a function, call it
3) if oneshot, remove it
4) return

The call itself should copy the 0048 technique:

- check `JS_IsFunction`
- check `JS_StackCheck`
- call with `JS_PushArg` and `JS_Call`
- log exceptions with `MqjsVm::GetExceptionString(...)`

### 7.5 What happens if callbacks schedule more timers?

This is allowed, but it is a source of complexity.

Two safe rules:

1) Timer modification from JS calls should go through timer APIs that are thread-safe
   (e.g., protected by a mutex in the timer module).
2) Timer firing should be “level-triggered”: after running callbacks, recompute next due and re-arm.

---

## 8. GPIO “G3/G4” API design options

We want minimal and explicit control of two specific pins, not a full GPIO library.

### Option G1: A `pins` namespace

```js
pins.G3.toggle()
pins.G4.toggle()
pins.G3.set(0|1)
pins.G3.get()
```

Pros:
- semantically clear (board-level naming)
Cons:
- requires adding nested objects (`G3`, `G4`) to stdlib or via bootstrap JS.

### Option G2: A small functional API

```js
gpio.toggle("G3")
gpio.write("G4", 1)
```

Pros:
- fewer objects; easy to bind from C
Cons:
- less ergonomic than property-style pins

### Option G3: Put it under `sim` (lowest friction)

```js
sim.g3Toggle()
sim.g4Toggle()
```

Pros:
- no new namespace
- avoids conflicts with the existing `gpio` object in the MicroQuickJS stdlib
Cons:
- conceptually mixes “simulator” and “IO”

Recommendation: Option G2 or G1 depending on how much we want “board naming”.

For MVP sequencing, G2 is easiest to bind and read.

---

## 9. Sequencer layer options (what we build on top of timers)

The timer kernel enables sequences; the “sequencer” is how we make them ergonomic.

### Sequencer S1: Pure JS userland (no native sequencer)

With `setTimeout`, users write:

```js
function pulse(pin, n, ms) {
  var i = 0;
  function step() {
    if (i++ >= n) return;
    gpio.toggle(pin);
    setTimeout(step, ms);
  }
  step();
}
```

Pros:
- zero additional native API
- maximal flexibility

Cons:
- code repetition in the REPL
- easy to create runaway recursion if not careful

This option becomes much nicer once we add a library loading story (`load(...)` or autoload), so users can keep their helpers.

### Sequencer S2: A thin “sequence runner” (native scheduling, user-defined plan)

API sketch:

```js
var seq = sequencer.create([
  { at: 0,    op: ["toggle", "G3"] },
  { at: 50,   op: ["toggle", "G4"] },
  { at: 100,  op: ["toggle", "G3"] },
])
seq.start({ loop: true, period: 200 })
seq.stop()
```

Pros:
- plan is data, not code (good for repeatability)
- native scheduler can enforce bounds and safety

Cons:
- needs more native surface area (stdlib regeneration + parsing plan objects)

### Sequencer S3: JS callback “track” scheduled at fixed intervals

We provide:

```js
var h = every(10, function(tick) { ... })
h.cancel()
```

Then users implement a state machine:

```js
var i = 0;
every(50, function() {
  if (i++ % 2) gpio.toggle("G3");
  else gpio.toggle("G4");
});
```

Pros:
- minimal API
- easy mental model

Cons:
- less precise for complex sequences (requires manual bookkeeping)

Recommendation:

- Start with S1 + S3 (pure JS + periodic callback).
- If needed, add S2 later for declarative/portable sequences.

---

## 10. Safety model (what prevents “REPL foot-guns”)

### 10.1 Resource bounding

- cap max timers (e.g. 64)
- cap max queued callbacks pending in mqjs_service (already bounded by queue_len)
- cap callback execution time using `MqjsVm::SetDeadlineMs` (like 0048 uses 25ms)

### 10.2 Failure semantics

If a callback throws:

- log the exception string (0048’s `log_js_exception` pattern)
- for intervals, decide:
  - keep running (default) or
  - auto-cancel on throw (safer)

### 10.3 Priority and jitter

We should define “timers are best-effort”:

- timers run when the VM owner task gets CPU and drains jobs
- UI rendering and Wi‑Fi can introduce jitter

If we need sub-millisecond toggling accuracy, we should not use JS callbacks at all; we would use a native driver (RMT, GPTimer) instead.

---

## 11. Concrete implementation plan (phased)

### Phase 1: Introduce mqjs_service into 0066 (foundation)

Migrate 0066 from “direct JS eval in console handler” to:

- `mqjs_service_start(...)`
- `js eval ...` posts an eval job and prints result

This aligns 0066 with 0048’s proven shape and makes timers safe.

### Phase 2: Implement setTimeout/clearTimeout (oneshots)

- Implement `js_setTimeout(fn, ms)` in the MicroQuickJS runtime:
  - validates args
  - allocates `tid`
  - stores callback in `__0066.timers.cb[tid]`
  - schedules due time
  - returns `tid`
- Implement `js_clearTimeout(tid)`
  - marks cancelled
  - removes callback from registry

### Phase 3: Provide “periodic”

Either:

- regenerate stdlib to add `setInterval/clearInterval`, or
- add a `every(ms, fn)` native primitive, or
- ship a JS polyfill library (later).

### Phase 4: GPIO primitives for G3/G4

Add a minimal `gpio` surface sufficient for sequencing:

- configure pins as output
- `gpio.toggle("G3")`, `gpio.toggle("G4")`

### Phase 5: Sequencer helpers

Start with documentation + examples (pure JS) and only introduce a native sequencer if userland becomes unwieldy.

---

## 12. Appendix: file map of prior art to consult during implementation

For VM ownership, deadlines, and safe callback invocation:

- `components/mqjs_service/mqjs_service.cpp`
- `components/mqjs_vm/mqjs_vm.cpp`
- `0048-cardputer-js-web/main/js_service.cpp` (bootstrap + callback + JS_Call pattern)

For console integration constraints:

- `components/wifi_console/wifi_console.c` (single esp_console REPL)
- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.cpp`

For timer patterns in ESP-IDF:

- `0047-cardputer-adv-lvgl-chain-encoder-list/main/lvgl_port_m5gfx.cpp` (periodic esp_timer)

