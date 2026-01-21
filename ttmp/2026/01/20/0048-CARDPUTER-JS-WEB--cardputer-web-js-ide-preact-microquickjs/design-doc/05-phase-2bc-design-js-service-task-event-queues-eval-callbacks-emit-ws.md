---
Title: 'Phase 2B+C Design: JS service task (eval + callbacks + emit→WS) and queue structures'
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
    - Path: 0048-cardputer-js-web/main/js_runner.cpp
      Note: Current single-context MicroQuickJS eval path guarded by a mutex (baseline to refactor into a service task)
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: Current REST eval handler and WS broadcaster helper (`http_server_ws_broadcast_text`)
    - Path: 0048-cardputer-js-web/main/js_console.cpp
      Note: Current `esp_console` command `js eval ...` (should route through JS service)
    - Path: 0048-cardputer-js-web/main/encoder_telemetry.cpp
      Note: Current encoder source + WS telemetry (delta/click) (will feed JS callbacks)
    - Path: 0048-cardputer-js-web/main/chain_encoder_uart.cpp
      Note: Current chain encoder driver (delta + click kinds)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: MicroQuickJS calling + GC ref APIs (`JS_PushArg`/`JS_Call`, `JSGCRef`, interrupt handler)
ExternalSources: []
Summary: "Define a single-owner JS service task for MicroQuickJS that safely processes eval requests and hardware callbacks, and forwards JS-emitted structured events to the browser over WebSocket using bounded queues."
LastUpdated: 2026-01-21T10:09:13-05:00
WhatFor: "A concrete concurrency and queueing design that can be implemented in firmware with predictable bounds, and that supports Phase 2B (callbacks) and Phase 2C (emit→WS event stream)."
WhenToUse: "Use when implementing the next refactor: move `js_runner` into a JS service task, route `/api/js/eval` + `esp_console` eval through it, and add encoder→JS callbacks plus JS→WS event emission."
---

# Phase 2B+C Design: JS service task (eval + callbacks + emit→WS) and queue structures

## Executive Summary

The current `0048` firmware evaluates JavaScript by calling MicroQuickJS directly from the HTTP handler (`POST /api/js/eval`) under a mutex (`js_runner.cpp`). That works for “evaluate a string on demand”, but Phase 2B requires *event-driven* JS callbacks (encoder delta/click) and Phase 2C requires JS to emit structured events that are forwarded to the browser over WebSocket.

MicroQuickJS is not thread-safe, so “a mutex around JS_Eval” becomes brittle as soon as:

- multiple producer tasks exist (HTTP server, console, encoder task),
- asynchronous callbacks arrive while a long eval is running,
- we need deterministic ownership and bounded queues.

This doc designs a single-owner **JS service task** that is the only task allowed to call MicroQuickJS APIs (`JS_Eval`, `JS_Call`, `JS_GetException`, GC ref ops, etc.). All other tasks communicate with it via explicit, bounded message queues. The service task also bridges JS→browser by flushing JS-emitted events into a WebSocket outbox queue.

The intent is to be implementable directly in our repo, with concrete structs and invariants, and to unify:

- Phase 2B: `encoder.on('delta'|'click', fn)` callbacks invoked on-device.
- Phase 2C: `emit(topic, payload)` from JS, forwarded to UI as a WebSocket event stream.

## Problem Statement

We want a programmable device where:

1) The browser editor sends a string of JS to the device over REST and gets a result back (Phase 1).
2) Hardware events (encoder delta/click) can drive user JS callbacks on-device (Phase 2B).
3) User JS can publish structured “events happened” payloads to the browser UI over WebSocket (Phase 2C).

The core tension: these are *asynchronous* producers around a *single-threaded* interpreter.

### Constraints and sharp edges

- **MicroQuickJS context ownership:** a `JSContext*` must not be called concurrently from multiple tasks.
- **Time bounds:** user code can loop; we need interrupt/timeout (already present in `js_runner.cpp` via `JS_SetInterruptHandler` and a deadline).
- **Memory bounds:** the VM has a fixed heap; additionally, queues and WS frames must be bounded to avoid fragmentation and OOM.
- **Backpressure:** encoder delta can be bursty; button clicks must not be lost; JS “emit” can also become chatty.
- **Observability:** when we drop/coalesce, it must be visible (counters + explicit drop events).

Non-goals for this design:

- Multi-tenant security (per-browser isolation).
- Preemptive scheduling inside JS.
- Capturing unstructured `print()` to the browser (keep “emit” explicit).

## Proposed Architecture

### Task topology (conceptual)

We introduce two long-lived tasks and one shared queue:

1) **JS service task** (new):
   - owns `JSContext*`
   - processes messages: eval requests, encoder events, resets
   - invokes callbacks and flushes JS→WS events

2) **WS outbox task** (new but small, optional in MVP):
   - drains a bounded queue of outbound WS frames
   - calls `http_server_ws_broadcast_text(...)` for each frame

Existing producer tasks remain:

- HTTP server task(s): enqueue eval requests and wait for replies.
- `esp_console` command handler: enqueue eval requests and wait for replies.
- encoder telemetry task: continues to compute deltas/clicks; enqueues callback notifications (Phase 2B) and may still broadcast telemetry frames (Phase 2).

Why the outbox task?

- It decouples “interpreter work” from “broadcast to N clients”.
- It ensures a consistent drop policy for all WS traffic (encoder + JS events).
- It prevents accidental networking from inside a VM-critical section.

However, if we want minimal moving parts, the JS service can call `http_server_ws_broadcast_text` directly *after* releasing VM/locks. The outbox is the cleaner long-term design, and it’s simple enough to justify from the start.

### Dataflow overview

**REST eval path**:

Browser → `POST /api/js/eval` (HTTP handler) → enqueue `JS_MSG_EVAL_REQ` → JS service evaluates → reply JSON to handler → handler responds to browser.

**Console eval path**:

Serial console `js eval <code>` → enqueue `JS_MSG_EVAL_REQ` → JS service evaluates → reply JSON → command prints.

**Encoder callback path** (Phase 2B):

encoder telemetry task detects delta/click → enqueue `JS_MSG_ENCODER_*` (coalesced delta, reliable click) → JS service calls registered JS callbacks.

**JS emit path** (Phase 2C):

user JS calls `emit(topic, payload)` → events accumulate in JS-side queue → JS service flushes events after each message → outbox broadcasts envelope(s) to browser → UI appends to event history.

## Queue Structures (Concrete)

We design *two* main channels:

1) inbound: producers → JS service (`js_in_q`)
2) outbound: JS service → WS broadcaster (`ws_out_q`)

Additionally, we design one mailbox-style coalescer:

- `encoder_delta_mailbox` for bursty deltas (no queue spam).

### Inbound queue: `js_in_q`

Use a FreeRTOS queue with fixed-length slots:

- `QueueHandle_t js_in_q;`
- length: ~16–32 (tune)
- item: `js_msg_t` (small, copyable)

```c
typedef enum {
  JS_MSG_EVAL_REQ = 1,
  JS_MSG_ENCODER_DELTA = 2,  // optional if we choose mailbox-only
  JS_MSG_ENCODER_CLICK = 3,
  JS_MSG_RESET_VM = 4,
  JS_MSG_SHUTDOWN = 5,
} js_msg_type_t;

typedef struct js_eval_req js_eval_req_t;

typedef struct {
  js_msg_type_t type;
  uint32_t seq;
  uint32_t ts_ms;
  union {
    js_eval_req_t* eval;   // pointer, owned by JS service until completion
    struct { int32_t delta; int32_t pos; } enc_delta;
    struct { int kind; } enc_click; // 0 single, 1 double, 2 long
  } u;
} js_msg_t;
```

The `js_eval_req_t*` indirection is deliberate:

- an eval request needs a reply path and may own an arbitrary-length code buffer.
- keeping `js_msg_t` small avoids queue slot bloat.

#### Eval request: request object + reply semaphore

```c
typedef struct js_eval_req {
  // Owned buffers:
  char* code;          // malloc'd by producer; freed by JS service
  size_t code_len;

  // Options:
  uint32_t timeout_ms; // 0 = no timeout (discouraged)
  const char* filename; // e.g. "<http>" or "<console>"

  // Reply:
  SemaphoreHandle_t done;  // binary semaphore
  char* reply_json;        // malloc'd by JS service; freed by producer

  // (optional) Stats:
  bool timed_out;
  uint32_t dropped_events; // if flush had drops
} js_eval_req_t;
```

Producer lifecycle (HTTP handler / console):

1) allocate `js_eval_req_t` and `code` buffer (copy request body or CLI args)
2) create `done` semaphore (binary)
3) enqueue `JS_MSG_EVAL_REQ` with `u.eval = req`
4) wait on `done` (bounded wait > JS timeout + margin)
5) use `reply_json` (send over HTTP or print)
6) free `reply_json`, delete semaphore, free `req`

JS service lifecycle:

1) pop message
2) evaluate `req->code`
3) allocate `reply_json`
4) free `req->code`
5) signal `req->done`

This design makes ownership explicit and prevents use-after-free: the request struct exists until the producer receives the “done” signal.

#### Failure modes and policies

- If `xQueueSend` fails because `js_in_q` is full:
  - For HTTP eval: return `503` (server busy).
  - For console eval: print “busy”.
- For encoder click: if queue is full, this is serious; prefer a dedicated smaller queue or direct task notification. See “Priority and backpressure” below.

### Encoder delta mailbox: `encoder_delta_mailbox`

Encoder delta can arrive frequently. We do not want a queue message per tick.

We implement a mailbox that stores the **latest** state and a sequence stamp:

```c
typedef struct {
  int32_t pos;
  int32_t delta;
  uint32_t seq;
  uint32_t ts_ms;
} encoder_delta_snapshot_t;

static encoder_delta_snapshot_t g_enc_delta = {};
static SemaphoreHandle_t g_enc_mu; // or spin/critical section
static TaskHandle_t g_js_task;     // for notify
```

Encoder task updates mailbox:

1) compute new delta/pos
2) store into `g_enc_delta` under a small critical section
3) `xTaskNotifyGive(g_js_task)` (wake JS service)

JS service consumes mailbox:

1) wait for either `js_in_q` message or a notify
2) if notified, read mailbox (atomic snapshot)
3) if callback is registered, invoke it with the latest snapshot
4) coalesce automatically: bursts collapse to “latest”.

This design matches the *information nature* of delta: the UI/logic typically wants the most recent position, not every intermediate edge.

Button clicks are different: they are discrete events and should not be mailbox-coalesced; handle them via queue with “no drop” policy.

### Outbound queue: `ws_out_q`

We want a single mechanism that pushes frames to the browser:

- encoder telemetry frames (Phase 2)
- JS event frames (Phase 2C)
- drop notifications / internal warnings (optional)

Use a bounded queue of frame pointers:

```c
typedef struct {
  char* json;     // malloc'd payload (text frame)
  size_t len;     // bytes (no trailing NUL required)
} ws_frame_t;

QueueHandle_t ws_out_q; // e.g. length 32
```

Producers of WS frames:

- encoder telemetry task: enqueues `{"type":"encoder",...}` and `{"type":"encoder_click",...}`
- JS service task: enqueues `{"type":"js_events",...}` envelopes and `{"type":"js_events_dropped",...}`

WS outbox task:

1) blocks on `ws_out_q`
2) calls `http_server_ws_broadcast_text(frame->json)` (or a variant that accepts len)
3) frees `frame->json` and frame storage

Backpressure policy:

- If `ws_out_q` is full:
  - drop the newest *encoder snapshots* (lossy)
  - never drop click events if possible (see below)
  - for JS events: drop newest and increment a drop counter, and enqueue (or later synthesize) a `js_events_dropped` frame.

FreeRTOS queues don’t have built-in priority. If we need strict policies, we can use:

- two queues (`ws_out_q_hi` for clicks, `ws_out_q_lo` for snapshots/events)
- or one queue + “drop oldest snapshot to make room for click” by draining a snapshot item opportunistically (more complex)

A clean compromise:

- keep encoder snapshots at a fixed cadence (already the case) and treat them as lossy; if `xQueueSend` fails, just skip this tick.
- clicks and js_events envelopes use a separate `ws_out_q_hi` (length small, e.g. 8). The outbox task always drains hi first.

## JS VM Ownership, APIs, and Safety

### Single-owner invariant

> Only the JS service task calls MicroQuickJS APIs.

Concrete: only that task may touch:

- `JS_Eval`, `JS_Call`, `JS_GetException`, `JS_GC`, etc.
- `JSGCRef` registration (`JS_AddGCRef` / `JS_DeleteGCRef`)
- `JS_SetInterruptHandler` and the deadline opaque

Producers never call into MicroQuickJS. They enqueue messages and wait for replies.

### Timeout and interrupt handling

We keep the current interrupt strategy from `js_runner.cpp`:

- store a deadline in a C struct
- `JS_SetContextOpaque(ctx, &deadline)`
- `JS_SetInterruptHandler(ctx, interrupt_handler)`
- interrupt handler returns “stop” once time passes deadline

In the service task, set deadline for each message type:

- eval requests: use configured timeout (e.g. 250ms default)
- encoder callbacks: use a smaller timeout (e.g. 25ms), because callbacks must not stall event processing

### Callback registration (Phase 2B)

We expose a JS object `encoder` with:

- `encoder.on('delta', fn)`
- `encoder.on('click', fn)`
- `encoder.off('delta'|'click')`

The C side holds callbacks using GC refs:

```c
typedef struct {
  JSGCRef on_delta_ref;
  JSGCRef on_click_ref;
} encoder_cb_state_t;
```

This state lives in the JS service and is only mutated from there.

MicroQuickJS call convention reminder (from `mquickjs.h` and example.c):

- push args
- push function
- push this
- call `JS_Call(ctx, argc)`

### JS event emission (Phase 2C)

We keep the explicit event model from Phase 2C:

- JS calls `emit(topic, payload)` to record structured events.
- events are accumulated in a bounded JS-side queue (`globalThis.__0048.events`).
- after each eval/callback, the service flushes via `JSON.stringify(__0048_take())`.

#### Where does “emit” live?

We install a bootstrap script during VM init:

- defines `globalThis.emit`
- defines `globalThis.__0048_take`
- defines bounds: `__0048.maxEvents`, increments `__0048.dropped`

This avoids modifying the generated MicroQuickJS stdlib for MVP.

#### Flush discipline (important)

We flush at deterministic points:

- after each eval request
- after each encoder callback invocation

Flushing is part of the interpreter service’s message processing loop; it creates `js_events` envelopes for the WS outbox.

Why not flush on a timer?

- deterministic flush makes causality clearer: the browser sees “events produced by this callback/eval” soon after it runs.
- it reduces complexity (no extra timer task).

## Service Loop (Pseudo-code)

The JS service has three “inputs”:

- queue messages (`js_in_q`)
- notifications (encoder mailbox updates)
- optional reset events

Pseudo-code:

```c
for (;;) {
  js_msg_t msg;
  if (xQueueReceive(js_in_q, &msg, /*timeout=*/pdMS_TO_TICKS(50))) {
    switch (msg.type) {
      case JS_MSG_EVAL_REQ:
        js_eval_handle(msg.u.eval);
        js_flush_emit_to_ws(/*source=*/"eval");
        break;
      case JS_MSG_ENCODER_CLICK:
        js_call_encoder_click_cb(msg.u.enc_click.kind);
        js_flush_emit_to_ws(/*source=*/"callback");
        break;
      case JS_MSG_RESET_VM:
        js_reset_vm();
        break;
    }
  }

  // Encoder delta mailbox consumption (coalesced)
  if (ulTaskNotifyTake(/*clearOnExit=*/pdTRUE, /*ticksToWait=*/0) > 0) {
    encoder_delta_snapshot_t snap = enc_mailbox_take_latest();
    js_call_encoder_delta_cb(snap);
    js_flush_emit_to_ws(/*source=*/"callback");
  }
}
```

Key point: we flush after doing something that might have called `emit`. That is the bridge from “JS recorded events” to “browser sees them”.

## Design Decisions and Rationale

### Why an owner task vs a mutex

Mutex-only designs are adequate for single-producer eval, but they degrade as soon as asynchronous callbacks enter the picture. The owner task:

- establishes a single “interpreter time” order for all actions,
- makes backpressure explicit (queue sizes, drop policy),
- avoids deadlocks from calling interpreter functions while waiting on network I/O,
- makes it easy to unify eval and callbacks and to integrate “emit flush”.

### Why mailbox for delta

Delta is a stream with a natural “latest value” interpretation. A mailbox:

- prevents queue flooding
- provides a clean coalescing point
- matches our existing WebSocket snapshot cadence philosophy

Clicks remain queued events because they are discrete and should not be coalesced away.

### Why an outbox queue for WS frames

Broadcasting to multiple clients can involve:

- per-client allocations
- async send bookkeeping

Putting it behind a queue:

- makes it easy to drop telemetry frames without impacting interpreter responsiveness,
- centralizes rate limiting,
- yields a single place to instrument “drops”.

## Implementation Plan (Firmware)

1) Add `js_service.{h,cpp}` and move MicroQuickJS context ownership there.
2) Update `/api/js/eval` and `js eval` console command to enqueue `JS_MSG_EVAL_REQ`.
3) Add encoder callback registration binding (`encoder` object) in the JS service.
4) Add encoder event producers:
   - click → `JS_MSG_ENCODER_CLICK` (reliable)
   - delta → mailbox + notify (coalesced)
5) Add JS bootstrap for `emit(...)` and `__0048_take`.
6) Add WS outbox queue + task, route:
   - existing encoder telemetry frames
   - new `js_events` frames
7) Extend the Web UI with a bounded event history panel (Phase 2C UI task).

## Open Questions

1) Should eval requests be able to “install callbacks” and persist them across re-eval, or should we provide an explicit “install” concept?
2) Exact timeouts:
   - eval: 250ms? 500ms?
   - callbacks: 10–25ms?
3) WS frame length bound: what is the safe max JSON payload size (1KiB? 4KiB?) given our memory pool and client count?
4) Should we unify seq numbers across encoder + js events into a single global WS sequence?

## References

- Phase 2B callbacks design (Option B):
  - `design-doc/03-phase-2b-design-on-device-js-callbacks-for-encoder-events.md`
- Phase 2C JS→WS event stream design:
  - `design-doc/04-phase-2c-design-js-websocket-event-stream-device-emits-arbitrary-payloads-to-ui.md`
- Current eval baseline:
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_runner.cpp`
- Current WS broadcaster helper:
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
- MicroQuickJS APIs and calling convention:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
