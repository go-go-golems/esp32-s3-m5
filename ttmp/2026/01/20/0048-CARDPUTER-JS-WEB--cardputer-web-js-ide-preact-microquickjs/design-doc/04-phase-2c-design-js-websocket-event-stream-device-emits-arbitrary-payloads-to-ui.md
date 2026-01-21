---
Title: 'Phase 2C Design: JS→WebSocket event stream (device emits arbitrary payloads to UI)'
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
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: WebSocket client tracking + `http_server_ws_broadcast_text`
    - Path: 0048-cardputer-js-web/main/js_runner.cpp
      Note: MicroQuickJS init/eval/timeout; natural insertion point for JS-side emit bootstrap + flush
    - Path: 0048-cardputer-js-web/web/src/ui/store.ts
      Note: WebSocket message parsing; will be extended to keep bounded message history
    - Path: 0048-cardputer-js-web/web/src/ui/app.tsx
      Note: UI rendering; will add an event-history panel
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Authoritative MicroQuickJS API surface (no built-in JSON stringify helper)
ExternalSources: []
Summary: "Allow user JS to explicitly emit structured events that the firmware forwards to the browser over WebSocket, so the Web IDE can display a live, bounded history stream (debugging + future UI-driving)."
LastUpdated: 2026-01-21T15:00:00-05:00
WhatFor: "A safe, bounded, debuggable bridge from on-device JavaScript to the browser UI, without capturing console prints or requiring the JS VM to call into the network stack directly."
WhenToUse: "Use when JS running on-device needs to publish arbitrary payloads to the frontend (debug events, UI signals, telemetry beyond encoder)."
---

# Phase 2C Design: JS→WebSocket event stream (device emits arbitrary payloads to UI)

## Executive Summary

Phase 1 makes the Cardputer a “JavaScript calculator”: the browser sends a string, the device evaluates it, and the browser prints the result.

Phase 2 adds a second channel: the device pushes encoder telemetry to the browser over WebSocket.

Phase 2C generalizes the push channel: **JavaScript running on-device can explicitly emit events, and the firmware forwards those events over WebSocket to the browser UI**, which displays a bounded, scrollable event history.

Key points:

- This is **not “capture print()”**; it is a deliberate API: `emit(topic, payload)` (names TBD) that the user calls when they want the browser to see something.
- The JS VM must not “do networking”; it should **enqueue / record events**. The HTTP/WS layer remains the only layer that touches `esp_http_server` and sockets.
- The system must be **bounded** (memory, rate, and frame size) and **debuggable** (event schema, sequence numbers, observable drops).

This doc proposes a minimal MVP that requires no changes to MicroQuickJS stdlib generation:

1) Define a small JS “event accumulator” in `globalThis` (pure JS code, installed once at VM init).
2) After an eval or callback, flush accumulated events by executing `JSON.stringify(...)` inside the VM and broadcasting a single JSON frame (or a small batch) over WebSocket.
3) Extend the Preact UI to show a bounded event history stream (raw + parsed), so you can see encoder events, JS events, and parse errors in one place.

## Problem Statement

We need a way for code running on-device to communicate *arbitrary*, *structured* information back to the browser UI in real time.

Motivating examples:

- A user writes JS that controls some device logic and wants to publish status like:
  - “entered state X”
  - “I saw N button presses”
  - “current PID terms”
- Later phases (2B callbacks) want to invoke user code on encoder events and show “what happened” in the UI without requiring the user to keep a serial console attached.

Constraints:

- The device uses `esp_http_server` with WebSocket support (`CONFIG_HTTPD_WS_SUPPORT`).
- WebSocket broadcast already exists as a device→browser text-frame channel.
- MicroQuickJS (`JSContext*`) is not thread-safe; in Phase 1 it is protected by a mutex (`js_runner.cpp`).
- We must remain bounded: a single JS snippet must not be able to allocate unbounded event buffers or force the firmware to allocate multi-megabyte WebSocket frames.

Non-goals for Phase 2C MVP:

- Capturing `print(...)` output to the browser (that is a separate feature with its own semantics and backpressure problems).
- Inbound WebSocket control plane (browser→device). We can keep the WS channel outbound-only for now.

## Proposed Solution

### The core design: “VM records events; HTTP/WS sends them”

We create a simple, explicit bridge:

- **Producer:** JS code calls `emit(...)` to record an event.
- **Buffer:** events are accumulated in a bounded JS-side queue in `globalThis.__0048.events`.
- **Flush:** after JS runs (HTTP eval or callback), C code asks the VM for a JSON encoding of the queued events and clears the queue.
- **Transport:** C broadcasts a JSON envelope over WebSocket using the existing `http_server_ws_broadcast_text(...)`.
- **Consumer:** the browser UI listens to `/ws` and appends every received frame to a bounded “Event History” panel.

This structure keeps responsibilities clean:

- JS remains pure compute/state (no sockets).
- The networking boundary stays inside the HTTP server module.
- Every cross-boundary interaction is explicit, bounded, and testable.

### Why this design fits MicroQuickJS specifically

MicroQuickJS in this repo provides:

- `JS_Eval(...)` and value printing, plus basic object/property APIs.
- It does **not** provide a “C helper for JSON stringify”; the supported technique is to call JS’s own `JSON.stringify` from inside the VM (`mquickjs.h` has no JSON helper).

Therefore the simplest and most robust way to serialize arbitrary payloads is:

1) keep events as JS values (objects),
2) stringify them *inside* the VM,
3) transfer only the resulting string to C (via `JS_ToCStringLen`),
4) ship that as a WebSocket text frame.

### Event schema (wire format)

All outbound WebSocket frames remain small JSON objects, with a top-level `type` discriminator.

Existing frames:

- Encoder snapshot (current implementation):
  - `{"type":"encoder","seq":123,"ts_ms":...,"pos":...,"delta":...}`
- Encoder click event:
  - `{"type":"encoder_click","seq":...,"ts_ms":...,"kind":0|1|2}`

New Phase 2C frames:

1) A batch envelope emitted after eval/callback flush:

```json
{
  "type": "js_events",
  "seq": 1001,
  "ts_ms": 1234567,
  "source": "eval" | "callback",
  "events": [
    { "topic": "log", "payload": { "x": 1 }, "ts_ms": 1234567 },
    { "topic": "state", "payload": "ARMED", "ts_ms": 1234568 }
  ]
}
```

Notes:
- `seq` is for ordering; it is independent of encoder `seq` (can share a global WS seq if desired).
- `ts_ms` is device time (ms since boot); the UI can also stamp browser receive time.
- `payload` is arbitrary JSON *as produced by `JSON.stringify` inside the VM*.

2) A drop/overflow signal (when we clamp event buffers):

```json
{ "type": "js_events_dropped", "seq": 1002, "ts_ms": 1234570, "dropped": 17, "reason": "queue_full" }
```

### JS API surface (user-facing)

Phase 2C MVP (pure JS bootstrap, no custom stdlib needed):

```js
// global; available after VM init (installed by firmware)
emit(topic, payload)         // payload must be JSON-serializable
emit_json(topic, value)      // sugar for payload = value (stringified)
```

Semantics:
- `emit(topic, payload)` records a single event object:
  - `{ topic, payload, ts_ms: Date.now() }`
- `emit_json(topic, value)` records:
  - `{ topic, payload: value, ts_ms: Date.now() }`
  and relies on the final flush using `JSON.stringify(...)` to encode it correctly.

User examples (in the Web IDE editor):

```js
emit("log", { hello: "world", n: 42 })
emit("state", "ARMED")
```

Important: `emit(...)` is explicitly about publishing *structured events*, not about capturing console logs. This keeps the contract small and avoids “stdout streaming” backpressure complexity.

### Firmware implementation sketch (MVP)

#### 1) Install a JS bootstrap once per VM

At `js_runner_init()` time (or immediately after context creation), run a small snippet:

Pseudocode:

```c
// on init:
JS_Eval(ctx, BOOTSTRAP_JS, strlen(BOOTSTRAP_JS), "<boot>", JS_EVAL_REPL);
```

`BOOTSTRAP_JS` defines a namespace and bounded queue:

```js
globalThis.__0048 = globalThis.__0048 || {}
__0048.maxEvents = 64
__0048.events = __0048.events || []

globalThis.emit = (topic, payload) => {
  if (__0048.events.length >= __0048.maxEvents) {
    __0048.dropped = (__0048.dropped || 0) + 1
    return
  }
  __0048.events.push({ topic, payload, ts_ms: Date.now() })
}

globalThis.__0048_take = () => {
  const ev = __0048.events
  const dropped = __0048.dropped || 0
  __0048.events = []
  __0048.dropped = 0
  return { ev, dropped }
}
```

We keep the queue logic in JS (simple, portable), and only transfer a bounded JSON string to C.

#### 2) Flush after eval (send over WebSocket)

In `POST /api/js/eval` (in `main/http_server.cpp:js_eval_post`), after calling `js_runner_eval_to_json(...)` we do an additional “flush events” step:

Pseudocode:

```c
// after user code eval succeeded or failed:
// (we should flush even on exception: user code may emit before throwing)
const char* take = "JSON.stringify(__0048_take())";
JSValue v = JS_Eval(ctx, take, strlen(take), "<flush>", JS_EVAL_REPL | JS_EVAL_RETVAL);
if (!JS_IsException(v)) {
  // v is a JS string (JSON)
  const char* s = JS_ToCStringLen(ctx, &n, v, &tmpbuf);
  if (n <= MAX_WS_FRAME) {
    // wrap in an envelope if needed; broadcast
    http_server_ws_broadcast_text(envelope_json);
  }
}
```

Because the repo’s current `js_runner_eval_to_json()` hides the `JSContext*` behind a module boundary, there are two clean ways to integrate this:

Option C1 (minimal surface change):
- Add `js_runner_flush_events_to_json()` which:
  - runs the flush snippet,
  - returns a JSON string (or empty),
  - and clears the queue.
Then `js_eval_post()` calls it and broadcasts the result.

Option C2 (future-proof with Phase 2B):
- Replace `js_runner` mutex ownership with a “JS service task” (Phase 2B).
- The JS service emits events by appending to `__0048.events`, and periodically flushes to a C-side queue, which the WS broadcaster drains.

Phase 2C MVP prefers **Option C1**, because it is small and preserves the current architecture.

#### 3) Never call WebSocket APIs from inside JS

Even if we later expose a native `ws.send` function, the rule remains:

> JS produces data; firmware transports it.

This avoids calling `httpd_ws_send_data_async` while holding the JS mutex, and keeps all `esp_http_server` operations in a known context (`http_server.cpp` + broadcaster).

### UI implementation sketch: bounded event history

Add an event history panel to the Web IDE main page:

- Keep a bounded array in Zustand, e.g. `wsHistory: WsHistoryItem[]` with `max=200`.
- On every `ws.onmessage`, append:
  - raw frame string
  - parsed JSON if available
  - receive timestamp (browser time)
  - a stable incrementing client seq
- Render as a scrolling list with:
  - newest-at-bottom (typical log)
  - “Clear” button
  - optional auto-scroll (scroll to bottom on new messages)

This panel provides immediate value even before JS events exist, because it shows encoder telemetry and click events (and any future frames).

## Design Decisions

### Decision: explicit `emit(...)`, not implicit `print` capture

Rationale:
- `print` is unstructured and can be extremely chatty.
- Capturing stdout implies defining a new semantics for `print` (per-run vs global, backpressure, truncation, line buffering).
- For control and debugging, “structured events” are more useful than raw text.

We can still add “print capture” later, but it should be a separate design with its own constraints.

### Decision: JS-side accumulation + stringify-in-VM

Rationale:
- No need to extend the MicroQuickJS stdlib table in the MVP.
- The VM already has the most correct serializer for arbitrary JS objects: `JSON.stringify`.
- The firmware only deals in strings and bounded envelopes.

### Decision: bounded queues + explicit drop events

Rationale:
- On an embedded device, unbounded logs are a footgun.
- Dropping silently is confusing; explicit “dropped N events” makes the system observable.

## Alternatives Considered

### A) Native binding `ws.emit(topic, payload)` implemented in C

Pros:
- Events can bypass a JS accumulator array.
- Can push directly into a C ring buffer with strict bounds.

Cons (in this repo’s MicroQuickJS setup):
- Requires extending the stdlib function table / generated runtime in a controlled way.
- Increases coupling to MicroQuickJS internals; harder to keep in sync with imports.

This is a reasonable “Phase 2C+” step, but not required for MVP.

### B) Have `/api/js/eval` return events in the HTTP response only (no WebSocket)

Pros:
- No WS required; simplest transport.

Cons:
- Does not solve the callback/event-driven case (Phase 2B) where events happen without an HTTP request.
- The UI doesn’t get a unified stream of “device pushed” events.

### C) Create a second HTTP endpoint: `/api/js/events` to poll

Pros:
- Avoids WS complexity.

Cons:
- Polling is awkward, higher latency, and duplicates WS infrastructure already present.

## Implementation Plan

### Docs + UI (immediate)

1) Implement the Web IDE “Event History” panel:
   - bounded store
   - renders raw + parsed frames
2) Update JS help panel to mention `emit(...)` once firmware supports it.

### Firmware MVP (Option C1)

3) Add bootstrap JS installed at VM init (defining `emit` + `__0048_take`).
4) Add `js_runner_flush_events_to_json(...)`:
   - returns `{"ok":true,"events":[...]}` (or an empty array)
   - returns `dropped` count
5) In `http_server.cpp`:
   - after eval, call flush
   - if events exist, broadcast `{"type":"js_events",...,"events":[...]}` over WS

### Follow-on (aligns with Phase 2B)

6) When JS becomes an owner task (Phase 2B):
   - flush after each processed message (eval/callback), or on a timer
   - decouple “flush” from HTTP request lifetime

## Open Questions

1) Global ordering: should we unify encoder seq and js_events seq into a single “WS seq” counter?
2) Payload size limit: what is the safe max WS frame length for our ESP32-S3 memory budget (1 KiB? 4 KiB?)?
3) Should the UI offer “pause/hold” for the event history to inspect bursts?
4) Should `emit` support a “level” (`info/warn/error`) convention?
5) Where should “dropped events” be surfaced:
   - in the WS stream only,
   - also in `/api/status`,
   - and/or on the device console?

## References

Repo implementation touchpoints:

- WebSocket broadcast helper:
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp` (`http_server_ws_broadcast_text`, `ws_handler`)
- Current MicroQuickJS eval loop and timeout:
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_runner.cpp` (`js_runner_eval_to_json`, `interrupt_handler`)
- UI WS parsing (will host the bounded history):
  - `esp32-s3-m5/0048-cardputer-js-web/web/src/ui/store.ts`
- UI main page layout:
  - `esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.tsx`
- MicroQuickJS API surface (stringify must be done in JS):
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
