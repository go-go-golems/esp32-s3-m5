---
Title: 'Phase 2B Design: On-device JS callbacks for encoder events'
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/chain_encoder_uart.cpp
      Note: Current Chain Encoder driver; provides delta + click kinds
    - Path: 0048-cardputer-js-web/main/encoder_telemetry.cpp
      Note: Current encoder event source; would enqueue events to JS service
    - Path: 0048-cardputer-js-web/main/js_runner.cpp
      Note: Current sync eval implementation that would become a JS service task
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/example.c
      Note: Reference JS function calling pattern (js_rectangle_call)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Authoritative MicroQuickJS API (JS_PushArg/JS_Call
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T09:49:41.177783176-05:00
WhatFor: ""
WhenToUse: ""
---


# Phase 2B Design: On-device JS callbacks for encoder events

## Executive Summary

Phase 2B extends `0048` from “execute JavaScript when the user clicks Run” to “execute user-defined JavaScript callbacks when hardware events happen”.

Concretely:

- The user writes JS in the browser editor and registers callbacks (e.g. `encoder.on('click', fn)`).
- The firmware reads encoder motion + button events.
- The firmware delivers those events to the JS VM safely (without calling into the VM from arbitrary FreeRTOS tasks).
- The user’s callbacks run on-device in MicroQuickJS, and can mutate on-device state / call native bindings.

The critical design constraint is VM ownership: MicroQuickJS (`JSContext*`) is not thread-safe, so **every JS call (eval, callback invocation) must occur on a single owner task**.

This doc proposes a queue-driven “JS service task” that owns the `JSContext` and processes:

- `EvalRequest` (from HTTP `/api/js/eval` and from `esp_console` `js eval ...`)
- `EncoderEvent` (from the encoder driver tasks)

This gives:

- deterministic concurrency (one eval/callback at a time),
- bounded behavior (time limits + queue sizes),
- a clean boundary between I/O tasks and VM execution.

## Problem Statement

We want the device to be *programmable in real time*:

- a user edits JavaScript in the browser IDE,
- hits Run once to “install” event-driven logic,
- then hardware interaction (encoder motion/click) triggers JS code repeatedly without further HTTP calls.

This is *not* the same problem as “evaluate a string once”:

- Encoder events happen asynchronously.
- Encoder rotation can be bursty (rapid deltas) and must be coalesced/bounded.
- JS callbacks must be invoked from a safe context (single JSContext owner), not from ISR or from random driver tasks.
- JS callbacks must be bounded in time and memory (bad user code must not wedge the device).

Non-goals (for Phase 2B MVP):

- Multi-user isolation (“per browser session JS VMs”).
- Preemptive multitasking inside JS.
- High-throughput streaming of JS logs back to the browser.

Primary success criteria:

1) A user can register callbacks in JS (`encoder.on('delta', fn)` and `encoder.on('click', fn)`).
2) The firmware runs those callbacks on events without crashes, deadlocks, or reentrancy bugs.
3) The system remains debuggable: errors in callbacks are surfaced (console logs), and there is a clear reset path.

## Proposed Solution

### 1) Architecture: a single owner “JS service” task

Create a new firmware module (illustrative):

```
main/
  js_service.{h,cpp}          // owns JSContext, runs eval+callbacks
  js_bindings_encoder.{h,cpp} // exposes encoder API to JS (register callbacks)
  encoder_events.{h,c}        // event structs + queues
  http_server.cpp             // posts EvalRequest to js_service
  js_console.cpp              // posts EvalRequest to js_service
  encoder_telemetry.cpp       // posts EncoderEvent to js_service (in addition to WS broadcast)
```

**Single VM ownership rule**

> Only the JS service task may call MicroQuickJS functions (`JS_Eval`, `JS_Call`, `JS_GetException`, etc.).

All other tasks communicate with the JS service by enqueuing messages.

This design is motivated by two current realities in the codebase:

- Current synchronous eval path: `js_runner_eval_to_json()` in `0048-cardputer-js-web/main/js_runner.cpp` calls `JS_Eval` under a mutex.
- MicroQuickJS calling convention: function calls are stack-based using `JS_PushArg()` + `JS_Call()` (see `imports/esp32-mqjs-repl/.../components/mquickjs/example.c:js_rectangle_call`).

Mutex-guarded use is acceptable for “eval on demand” MVP, but event callbacks strongly prefer an owner task so we never:

- call into the VM from an encoder task,
- hold a global mutex while waiting on network I/O,
- or risk long callback execution delaying critical tasks.

### 2) Message types: eval requests and encoder events

Define a single queue into the JS service:

```c
typedef enum {
  JS_MSG_EVAL = 1,
  JS_MSG_ENCODER_DELTA = 2,
  JS_MSG_ENCODER_CLICK = 3,
  JS_MSG_RESET = 4,
} js_msg_type_t;

typedef struct {
  js_msg_type_t type;
  uint32_t seq;
  uint32_t ts_ms;
  union {
    struct {
      const char* code;
      size_t code_len;
      uint32_t timeout_ms;
      // reply mechanism (queue or callback) omitted here for clarity
    } eval;
    struct {
      int32_t delta;
      int32_t pos;
    } enc_delta;
    struct {
      int kind; // 0 single, 1 double, 2 long
    } enc_click;
  } u;
} js_msg_t;
```

**Queue sizing and loss policy**

- `encoder delta` can be bursty; we should allow coalescing:
  - encoder task accumulates deltas; JS service receives “latest delta/pos” periodically.
- `click` events are low rate; losing them is unacceptable:
  - prioritize click events over delta events if queue is full (implementation detail).

### 3) JS API surface: `encoder` object

Expose a global JS object:

```js
encoder.on('delta', (ev) => { /* ev.delta, ev.pos, ev.ts_ms */ })
encoder.on('click', (ev) => { /* ev.kind, ev.ts_ms */ })
encoder.off('delta')
encoder.off('click')
encoder.reset()   // optional: clear callbacks + reset internal state
```

The **only state** we must keep across evaluations is the callback function references.

#### How to store callback references safely (GC)

MicroQuickJS provides `JSGCRef` and GC ref management:

- `JS_AddGCRef(JSContext*, JSGCRef*)` / `JS_DeleteGCRef(JSContext*, JSGCRef*)`
- `JS_PushGCRef` / `JS_PopGCRef` macros for temporary roots.

We should store registered callbacks in C as `JSGCRef` so they are not collected:

```c
typedef struct {
  JSGCRef on_delta_ref; // holds JSValue (function) or JS_UNDEFINED
  JSGCRef on_click_ref;
} encoder_cb_state_t;
```

Registration pseudocode:

```c
// called from JS via encoder.on(...)
if (event == "delta") {
  JS_DeleteGCRef(ctx, &state->on_delta_ref);
  state->on_delta_ref.val = argv[1];  // must be function
  JS_AddGCRef(ctx, &state->on_delta_ref);
}
```

### 4) Calling a JS callback from C (MicroQuickJS calling convention)

MicroQuickJS uses a stack-based call API. A minimal worked pattern exists in:

- `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/example.c`
  - `js_rectangle_call()`:
    - `JS_PushArg(ctx, arg0)`
    - `JS_PushArg(ctx, func)`
    - `JS_PushArg(ctx, this_obj)`
    - `return JS_Call(ctx, argc)`

Callback invocation pseudocode (inside JS service task only):

```c
static void call_cb(JSContext* ctx, JSValue cb, JSValue ev_obj) {
  if (JS_StackCheck(ctx, 4)) return; // avoid blowing VM stack

  // Arrange stack: args..., func, this
  JS_PushArg(ctx, ev_obj);     // arg0: event object
  JS_PushArg(ctx, cb);         // func
  JS_PushArg(ctx, JS_NULL);    // this
  JSValue ret = JS_Call(ctx, 1);
  if (JS_IsException(ret)) {
    JSValue exc = JS_GetException(ctx);
    // format and log (using existing JS_PrintValueF pattern)
  }
  // FreeValue / PopGCRef as required (implementation detail).
}
```

Event object construction uses:

- `JS_NewObject(ctx)`
- `JS_SetPropertyStr(ctx, obj, "delta", JS_NewInt32(ctx, ...))`
- `JS_SetPropertyStr(ctx, obj, "ts_ms", JS_NewUint32(ctx, ...))`

### 5) Timeouts: bound callback runtime separately from eval runtime

Current `js_runner.cpp` uses:

- `JS_SetInterruptHandler(ctx, interrupt_handler)`
- `deadline_us` set before `JS_Eval`

For Phase 2B, we should have **two deadlines**:

- `eval_timeout_ms` for HTTP/console eval (user-triggered)
- `event_timeout_ms` for callbacks (system-triggered, typically smaller)

Policy recommendation:

- Default callback timeout: 10–50 ms.
- On timeout or repeated exceptions:
  - disable the callback and log an error (fail safe).

### 6) How the browser editor participates (installation model)

There are two reasonable models:

1) “Run installs callbacks”: user presses Run once; code registers callbacks; JS state persists.
2) “Always live”: editor changes are continuously shipped (not recommended; too disruptive).

Phase 2B chooses (1). The existing Phase 1 semantics (stateful VM, `JS_EVAL_REPL | JS_EVAL_RETVAL`) already support it.

Example user program:

```js
encoder.on('delta', (e) => {
  if (e.delta !== 0) print('delta', e.delta, 'pos', e.pos)
})

encoder.on('click', (e) => {
  if (e.kind === 0) print('single click')
})
```

### 7) Interaction with WebSocket telemetry

Phase 2B does not require WebSocket, but they can coexist:

- WebSocket is for browser observability and UI rendering.
- JS callbacks are for on-device behavior (e.g. drive GPIO, update internal state).

We should keep these planes separate:

- Encoder task → (a) WS broadcast, (b) JS service event queue.

This avoids making JS execution a dependency for “UI visibility”.

## Design Decisions

### Decision: single owner JS task (queue-driven) rather than calling JS from encoder tasks

Rationale:

- MicroQuickJS is not thread-safe.
- Encoders can be read from separate tasks; calling into JS from those tasks risks deadlocks and corruption.
- A queue-driven owner task makes correctness the default.

### Decision: callbacks are registered in JS via a small `encoder` API

Rationale:

- It mirrors familiar JS event emitter patterns (`on/off`).
- It keeps the API surface small and explicit.

### Decision: callback timeout is separate from eval timeout

Rationale:

- Event callbacks should be much shorter than “run user code once”.
- A strict small timeout is the difference between “interactive device” and “device wedges when you turn the knob”.

## Alternatives Considered

### Alternative A: call JS callbacks directly from the encoder telemetry task

Rejected:

- even with a mutex, it couples encoder timing to JS execution time.
- it creates tricky priority inversions (encoder task blocks behind long eval).

### Alternative B: run callbacks in the browser (WS only)

Rejected for Phase 2B goal:

- browser callbacks cannot drive on-device bindings without additional RPCs.
- it breaks the “device is programmable” story (logic becomes network-dependent).

### Alternative C: compile JS to bytecode on host and run bytecode only

Potential future direction, but rejected for Phase 2B MVP:

- requires a build pipeline and bytecode relocation discipline.
- editor workflow becomes heavier.

## Implementation Plan

1) Introduce `js_service` task that owns `JSContext*` (initially wrap the existing `js_runner` functionality).
2) Convert HTTP eval and console eval to send `JS_MSG_EVAL` to the JS service (remove “eval inside HTTP handler”).
3) Implement `encoder` JS bindings:
   - `encoder.on(event, fn)`
   - `encoder.off(event)`
4) Add `EncoderEvent` enqueue path from the encoder subsystem:
   - delta events (coalesced)
   - click events (reliable)
5) Implement callback invocation in the JS service:
   - build event object
   - call callback via `JS_PushArg`/`JS_Call`
   - bound runtime with event timeout
6) Add reset path:
   - `/api/js/reset` and `js reset` console command to clear callbacks and VM state.
7) Extend Phase 2 playbook:
   - “install callbacks” snippet
   - rotate/click and observe logs/effects
   - verify watchdog safety (intentional infinite loop in callback should time out and disable).

## Open Questions

- Do we want to expose the callback API as `encoder.on(...)` or a more explicit `encoder.onDelta(fn)` / `encoder.onClick(fn)`?
- Should callback exceptions disable the callback immediately, or after N failures?
- Where should callback logs go:
  - device console only (simplest), or
  - additionally buffer and expose via `/api/js/logs` (future)?
- How do we want to interleave callback execution with long `/api/js/eval` requests:
  - defer callbacks while eval is running, or
  - treat callbacks as higher priority and interrupt eval (risky, but sometimes desired)?

## References

- Phase 1 design and current eval contract:
  - `design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_runner.cpp`
- Phase 2 WS telemetry design:
  - `design-doc/02-phase-2-design-encoder-position-click-over-websocket.md`
- MicroQuickJS API + calling convention examples:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/example.c` (`js_rectangle_call`)
