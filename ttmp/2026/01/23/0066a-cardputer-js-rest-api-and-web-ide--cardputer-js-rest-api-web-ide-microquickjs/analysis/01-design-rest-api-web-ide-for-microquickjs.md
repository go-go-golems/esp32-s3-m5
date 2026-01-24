---
Title: 'Design: REST API + Web IDE for MicroQuickJS'
Ticket: 0066a-cardputer-js-rest-api-and-web-ide
Status: active
Topics:
    - cardputer
    - microquickjs
    - http
    - web-ui
    - api
    - console
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: Reference REST endpoints + WS wiring
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: Reference mqjs_service integration + emit/flush pattern
    - Path: components/httpd_ws_hub/httpd_ws_hub.c
      Note: Reference WS broadcast hub and client tracking
    - Path: components/httpd_ws_hub/include/httpd_ws_hub.h
      Note: Reference WS hub API
    - Path: components/mqjs_service/include/mqjs_service.h
      Note: Reference JS service public API (VM ownership + eval/job)
    - Path: components/mqjs_service/mqjs_service.cpp
      Note: Reference JS VM owner task implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-23T19:44:30.885780915-05:00
WhatFor: ""
WhenToUse: ""
---


# Design: REST API + Web IDE for MicroQuickJS (Cardputer / ESP32‑S3)

This document specifies a practical design for adding a browser-based JavaScript “IDE” to a Cardputer-class firmware that embeds **MicroQuickJS**. The IDE is accessed over Wi‑Fi: the browser sends JS code to the device, the device runs it inside a single owned JS VM, and the browser displays results and a stream of structured events.

The design is explicitly patterned after `0048-cardputer-js-web`, which already demonstrates the working integration points in this repo:

- `POST /api/js/eval` to evaluate arbitrary JS on-device.
- `/ws` WebSocket to push device→browser JSON frames.
- A single VM-owner “JS service task” (`mqjs_service`) so *all JS execution happens on one thread*.
- A web UI (bundled JS/CSS) that provides an editor, “Run”, output, and event history.

> **Callout (the big idea): “Interpreter as a Service”**
>
> Treat the embedded JS interpreter not as a library you call from arbitrary tasks, but as a *service* with:
>
> - a strict single-thread execution rule,
> - a request/response protocol (REST),
> - and an event stream protocol (WebSocket or SSE).
>
> This eliminates most concurrency bugs by construction.

---

## 1) Problem statement (what we’re building)

We want the following workflow:

1) User opens `http://<device-ip>/` in a browser.
2) The page shows:
   - a code editor (write JS),
   - an output pane (return value + “print-like” output),
   - an event pane (structured device events: logs, telemetry, app events).
3) When the user clicks “Run” (or Ctrl/Cmd‑Enter), the page sends JS to the device.
4) Device evaluates the code inside MicroQuickJS and replies with a structured result.
5) During/after evaluation, device can push events to the browser over `/ws`.

Non-goals (for an MVP):

- multi-user collaboration, auth, ACLs;
- a full Node-like event loop with promises/microtasks;
- filesystem-backed “projects”.

---

## 2) Constraints (ESP32 + MicroQuickJS realities)

### 2.1 Resource constraints

On an ESP32-S3, the web IDE competes for:

- flash space (bundled JS/CSS assets),
- heap (JS arena, JSON responses, HTTP request buffers),
- CPU (your app + Wi‑Fi + HTTP + JS).

`0048` addresses this with explicit Kconfig knobs:

- `CONFIG_TUTORIAL_0048_JS_MEM_BYTES`: fixed VM arena size.
- `CONFIG_TUTORIAL_0048_JS_TIMEOUT_MS`: evaluation deadline.
- `CONFIG_TUTORIAL_0048_JS_MAX_BODY`: maximum HTTP body size for eval.

### 2.2 Concurrency constraints (the key invariant)

MicroQuickJS is not designed for concurrent calls from multiple tasks.

**Design invariant:**

> Exactly one FreeRTOS task owns the `JSContext` and performs `JS_Eval` / `JS_Call`.

In this repo, that invariant is implemented by `components/mqjs_service`:

- other tasks enqueue “eval” or “job” messages,
- the JS service task executes them serially.

### 2.3 Runtime constraints (MicroQuickJS ≠ full QuickJS)

MicroQuickJS in this repo is a compact embedded API, not upstream QuickJS.

Implication:

- avoid assuming you can use the full QuickJS API surface in C;
- copy patterns from existing firmwares (`0048` and the imported `esp32-mqjs-repl`) rather than writing “new embedding code” from scratch.

---

## 3) Reference system: `0048-cardputer-js-web`

This section describes how `0048` works end-to-end. Think of it as a “worked example” from which we extract a reusable design.

### 3.1 Module overview (server side)

**HTTP server**: `0048-cardputer-js-web/main/http_server.cpp`

- Serves embedded assets:
  - `GET /` → `index.html` (via `httpd_assets_embed_send`)
  - `GET /assets/app.js`, `GET /assets/app.css`
- REST:
  - `GET /api/status` → Wi‑Fi state + IP
  - `POST /api/js/eval` → run code
- WebSocket (optional, behind `CONFIG_HTTPD_WS_SUPPORT`):
  - `/ws` endpoint uses `components/httpd_ws_hub` to manage clients and broadcast frames.

**JS service**: `0048-cardputer-js-web/main/js_service.cpp`

- Starts `mqjs_service` with a generated stdlib.
- Bootstraps JS globals (notably `emit(...)` and a buffering mechanism).
- Runs eval requests and returns a JSON response string.
- After eval or callbacks, flushes buffered JS events to WebSocket.

**WebSocket hub**: `components/httpd_ws_hub/httpd_ws_hub.c`

- Tracks up to 8 clients.
- Broadcasts text frames using `httpd_ws_send_data_async`.
- Can invoke RX callbacks for browser→device frames (useful for future interactive IDE features).

### 3.2 The eval endpoint (REST)

`0048` implements:

- `POST /api/js/eval` where the HTTP body is **raw JS text** (not JSON).

The handler:

1) Bounds-checks the request body (`CONFIG_TUTORIAL_0048_JS_MAX_BODY`).
2) Reads the whole body into a heap buffer.
3) Calls:
   - `js_service_eval_to_json(buf, n, 0, "<http>")`
4) Responds with JSON text.

The response format is:

```json
{
  "ok": true,
  "output": "...\n",
  "error": null,
  "timed_out": false
}
```

If evaluation throws, `ok=false` and `error` is a string.

> **Callout: why “raw JS body” is excellent for MVP**
>
> Posting raw JS avoids JSON encoding overhead in the browser and on-device, and enables a trivial curl workflow. The tradeoff is extensibility (timeouts, filenames, options) which is easiest to address with a JSON envelope later.

### 3.3 Device→browser events (WebSocket)

The most valuable idea in `0048` is that it *does not* try to stream raw stdout to the browser. Instead:

1) Provide a JS function `emit(topic, payload)` that appends structured events into a small buffer.
2) After eval (and after callbacks), flush the buffer by evaluating a helper that returns newline-delimited JSON frames.
3) Broadcast each JSON line to `/ws`.

Mechanics:

- JS bootstrap defines:
  - `emit(topic, payload)` → pushes `{topic, payload, ts_ms}` into `__0048.events` (bounded)
  - `__0048_take_lines(source)` → returns JSON lines (also includes drop notices)
- C++ side:
  - `flush_js_events_to_ws(ctx, source)` evaluates `__0048_take_lines(...)` and broadcasts each line.

This avoids the common failure mode of “browser logs”:

- raw stdout is messy (partial lines, encoding, ordering),
- structured JSON frames are easy to parse and display, and survive refactors.

### 3.4 Web UI in `0048`

`0048`’s UI is served from embedded assets:

- `0048-cardputer-js-web/main/assets/index.html`
- `0048-cardputer-js-web/main/assets/assets/app.js` (bundled/minified)
- `0048-cardputer-js-web/main/assets/assets/app.css`

The UI:

- posts JS to `/api/js/eval`
- maintains a bounded WebSocket event history (debugging tool)
- uses an in-browser editor (the bundle includes CodeMirror machinery)

---

## 4) Generalized architecture for a REST API + Web IDE

We can factor the design into five layers:

1) **Transport**: HTTP + optional WebSocket.
2) **Interpreter service**: `mqjs_service` (single owner task).
3) **Eval RPC**: request parsing + deadline + result shaping.
4) **Event channel**: structured events JS→C→WS.
5) **Browser UI**: editor + output + event view.

### 4.1 The interpreter service layer

Reuse (as-is):

- `components/mqjs_service` and `components/mqjs_vm`.

Expose a small firmware-local API (like `0048`’s `js_service_*`) that wraps:

- start/reset,
- eval,
- “post job” helpers for callbacks,
- event flushing to WS.

### 4.2 The REST layer (RPC semantics)

At minimum:

- `POST /api/js/eval` → run code and return result.

Strongly recommended for a usable IDE:

- `POST /api/js/reset` → recover from infinite loops / memory bloat.
- `GET /api/js/stats` → arena usage, memory dump, queue status.
- `GET /api/status` → Wi‑Fi/IP + optionally “JS health”.

### 4.3 Event stream transport choice (WS vs SSE)

`0048` uses WebSocket because:

- ESP-IDF `esp_http_server` supports it,
- `httpd_ws_send_data_async` enables best-effort broadcast,
- it allows future bidirectional control (browser→device).

SSE is a simpler one-way alternative, but requires different buffering/reconnect semantics. For a Cardputer IDE, WebSocket is a good default.

---

## 5) Proposed REST API contract (0066a blueprint)

The design should be compatible with the `0048` MVP while allowing future growth.

### 5.1 `/api/js/eval` (two viable variants)

**Variant A (match 0048): raw JS body**

- Request: `POST /api/js/eval`
  - body: raw JS program
- Response: JSON result object:

```json
{
  "ok": true,
  "output": "...\n",
  "error": null,
  "timed_out": false
}
```

**Variant B (IDE-friendly): JSON envelope**

- Request: `POST /api/js/eval` with `application/json`

```json
{
  "code": "1+1\n",
  "filename": "<web>",
  "timeout_ms": 250,
  "flush_events": true
}
```

Response extends the result object with metadata:

```json
{
  "ok": true,
  "output": "2\n",
  "error": null,
  "timed_out": false,
  "ts_ms": 123456,
  "seq": 17
}
```

> **Recommendation:** implement Variant A first (fast path), but keep the internal code structured so switching to Variant B is “just request parsing”, not a redesign.

### 5.2 `/api/js/reset`

- `POST /api/js/reset`

Response:

```json
{ "ok": true }
```

Semantics:

- stop and restart the JS service (or recreate the `JSContext`),
- discard queued work.

### 5.3 `/api/js/stats`

- `GET /api/js/stats`

Possible response:

```json
{
  "ok": true,
  "arena_bytes": 65536,
  "queue_len": 16,
  "dump": "...\n"
}
```

Implementation note:

- `mqjs_vm` provides `DumpMemory(...)`, which can be captured on the JS task via a job and returned as a string.

### 5.4 `/api/status`

`0048`’s `/api/status` is already a good template, because the browser needs:

- IP,
- Wi‑Fi state,
- last disconnect reason.

For an IDE, it’s also useful to include:

- whether the JS service started successfully,
- current queue pressure (if measurable).

---

## 6) Event stream design (device→browser)

### 6.1 Event vocabulary

Even if “events are just JSON frames”, standardizing types improves tooling:

- `js_events`: flushed from JS `emit(...)`
- `js_events_dropped`: explicit overload signal
- `log`: device log line (optional)
- `telemetry`: app-defined structured data (encoder snapshots, sim status, etc.)

### 6.2 How JS creates events

Follow the `0048` pattern:

```js
emit("gpio", { pin: "G3", v: 1 });
emit("sim", { type: "rainbow", bri: 100 });
```

Keep the buffer bounded:

```js
__N.maxEvents = 64;
__N.events = [];
__N.dropped = 0;
```

### 6.3 Flush points (determinism)

General rule:

> After any “user-visible execution step”, flush buffered events.

In practice:

- after `/api/js/eval` completes,
- after callback jobs (hardware events) run.

### 6.4 Backpressure policy

There are two important bounded resources:

- the `mqjs_service` queue (work submission),
- the JS event buffer (`emit` queue).

`0048` makes overload explicit by:

- dropping events when buffer full (incrementing `dropped`),
- sending a `js_events_dropped` frame on flush.

This is a good policy for embedded systems: it keeps the system responsive and reveals overload.

---

## 7) Browser IDE design

### 7.1 MVP UI

The smallest useful web IDE can be a single HTML file:

- `<textarea>` for code,
- “Run” button,
- `<pre>` for output,
- WS status indicator,
- bounded event history.

This is ideal for early bring-up and avoids a frontend toolchain.

### 7.2 0048-class UI (recommended once stable)

`0048` demonstrates a much better UX:

- Code editor (bundled CodeMirror),
- Ctrl/Cmd‑Enter to run,
- event history bounded to 200 frames,
- WS connected indicator + decoded telemetry.

Tradeoffs:

- tooling complexity (build pipeline) vs UX.
- but the runtime model is still just “serve static assets from flash”.

### 7.3 Output semantics: return values vs `print`

Users expect `print(...)` to show up in the browser.

Three approaches:

1) Leave `print(...)` as “device console only”.
2) Override `print(...)` in bootstrap to `emit("print", "...")` (best UX).
3) Provide a separate `log(...)` function for browser-only output.

This decision should be explicit in the design because it affects the UX and the event bandwidth.

---

## 8) Implementation blueprint (how the pieces fit)

### 8.1 Reuse map (recommended components)

From this repo:

- `components/mqjs_service` and `components/mqjs_vm`: core JS execution model.
- `components/httpd_ws_hub`: WS client tracking + broadcast.
- `components/httpd_assets_embed`: embed + serve static assets from flash.
- `components/wifi_mgr`: connect and expose IP status.

From reference firmware:

- `0048-cardputer-js-web/main/http_server.cpp`: REST + WS wiring.
- `0048-cardputer-js-web/main/js_service.cpp`: eval→JSON + event flush.

### 8.2 Suggested firmware layout

```
main/
  http_server.{h,cpp}     // REST + WS + assets
  js_service.{h,cpp}      // starts mqjs_service; eval wrapper; bootstrap; flush
  esp32_stdlib_runtime.c  // native bindings (per firmware)
  assets/                 // index.html + app.js + app.css (prebuilt)
```

### 8.3 Pseudocode: eval handler (raw body)

```c
POST /api/js/eval:
  if body too large: return 413
  code = read_body()
  result_json = js_service_eval_to_json(code, timeout=default)
  return 200 with result_json
```

### 8.4 Pseudocode: JS service wrapper

```c
js_service_start():
  start mqjs_service with arena + stdlib
  run bootstrap job:
    define emit()
    define __take_lines()
    (optional) override print()

js_service_eval_to_json(code):
  mqjs_service_eval(...)
  build JSON response with escaped output/error
  post job_flush_eval (async)
  return JSON string
```

### 8.5 Pseudocode: flush events to WS

```c
flush_js_events_to_ws():
  s = JS_Eval("__take_lines('eval')")  // returns "json\njson\n..."
  for each line in split(s, '\n'):
    ws_broadcast_text(line)
```

---

## 9) Validation strategy (smoke tests matter)

Once implemented, the design should ship with at least:

- a serial helper that extracts IP from `wifi status`,
- a HTTP helper that:
  - GETs `/api/status`,
  - POSTs `/api/js/eval`,
  - optionally connects to `/ws` and asserts frames appear after `emit(...)`.

This reduces the cost of future refactors (the key goal of using contracts).

---

## 10) Security considerations (decide explicitly)

A JS IDE endpoint is effectively “remote code execution”.

For development, “no auth on trusted LAN” is common. If adding auth, keep it simple:

- check `Authorization: Bearer <token>` in handlers.

---

## 11) What “done” looks like for this design

For ticket 0066a (analysis only), “done” means:

- REST contract is clearly specified (endpoints, inputs, outputs, failure modes).
- Concurrency model is explicit and safe (`mqjs_service` single owner).
- Event streaming mechanism is clear (buffer + flush + WS broadcast).
- Browser UI plan is clear (MVP textarea vs 0048-class editor).

This document should be enough that a future implementation can be written without re-deriving how the pieces interact.

