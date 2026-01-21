---
Title: 'Phase 1 Design: Device-hosted Web JS IDE (Preact+Zustand + MicroQuickJS over REST)'
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
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: Reference implementation for embedded assets + esp_http_server routing and WS patterns
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/web/vite.config.ts
      Note: Reference deterministic Vite bundling (single JS/CSS
    - Path: esp32-s3-m5/docs/playbook-embedded-preact-zustand-webui.md
      Note: Playbook describing the firmware-friendly asset pipeline
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Authoritative MicroQuickJS C API primitives (JS_NewContext
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Reference for JS evaluation contract (EvalLine) and formatting/exception output
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h
      Note: Evaluator surface API that maps cleanly onto a REST execution handler
ExternalSources: []
Summary: 'Design for a Cardputer-hosted Web IDE: firmware serves an embedded Preact+Zustand code editor UI; browser POSTs JavaScript to the device; firmware executes via MicroQuickJS and returns formatted results/errors over REST.'
LastUpdated: 2026-01-20T23:02:28.997341209-05:00
WhatFor: Blueprint for implementing Phase 1 (HTTP + embedded UI + REST execution) with explicit modules, API contracts, memory/time limits, and concrete filenames/symbols from in-repo prior art.
WhenToUse: Use when implementing the Phase 1 MVP or reviewing architecture choices (asset pipeline, REST contract, evaluation semantics, safety).
---


# Phase 1 Design: Device-hosted Web JS IDE (Preact+Zustand + MicroQuickJS over REST)

## Executive Summary

We want the Cardputer to behave like a tiny, self-contained programming environment:

- The device serves a modern browser UI (a “code editor + run button + output panel”).
- The browser sends JavaScript code to the device over a simple HTTP API.
- The device executes that code via MicroQuickJS (a small QuickJS-derived engine) and returns:
  - the formatted return value, if any
  - any exception, if one was thrown
  - optional captured “print/console” output

The fundamental constraint is that we are doing “web development”, but on an embedded system with:

- small and fragmented memory,
- timing constraints (don’t stall the networking task forever),
- and a need for deterministic build outputs (embedded assets in firmware).

This design deliberately reuses proven patterns already present in this repo:

- Embedded Vite (Preact+Zustand) assets served via `esp_http_server` (`0017-atoms3r-web-ui`).
- WebSocket and server architecture patterns from ticket `0013` and playbook `docs/playbook-embedded-preact-zustand-webui.md`.
- MicroQuickJS evaluation and formatting from `imports/esp32-mqjs-repl/.../eval/JsEvaluator.cpp`.

Phase 1 MVP deliverable:

1) Cardputer connects to Wi‑Fi (SoftAP or STA).
2) User browses to `http://<device-ip>/`.
3) UI loads (embedded `index.html`, `assets/app.js`, `assets/app.css`).
4) User edits code and clicks “Run”.
5) UI POSTs code to `POST /api/js/eval`.
6) Device executes the code and returns a JSON response; UI renders output and/or error.

Phase 2 (encoder telemetry over WebSocket) is explicitly deferred to a separate design doc in this ticket.

## Problem Statement

We want a development loop that is:

- **interactive**: edit → run → observe output within seconds
- **device-hosted**: no external server required; works on a phone/laptop directly connected to the device
- **modern UI**: a real editor (syntax highlighting, keybindings) rather than a bare `<textarea>`
- **firmware-shaped**: deterministic build artifacts, stable routes, minimal RAM churn, bounded request sizes

More concretely, the problem is to bridge three domains that do not naturally fit together:

1) **Embedded HTTP servers** (esp_http_server) are optimized for small handlers and streaming bodies.
2) **Browser UIs** expect bundlers and lots of assets; firmware wants a small number of stable files.
3) **JavaScript engines** are designed to be general; the device needs predictability and safety when executing arbitrary user code.

If we do not specify contracts and limits up front, the system degenerates:

- the UI emits hashed filenames → firmware routes break or require a manifest parser
- requests accept unbounded code → memory exhaustion or watchdog resets
- JS code can loop forever → the HTTP server task stalls and the device becomes unreachable

## Proposed Solution

### 1) High-level architecture

At runtime the device provides:

- HTTP server (port 80):
  - static embedded assets:
    - `GET /` → `index.html`
    - `GET /assets/app.js` → bundled JS
    - `GET /assets/app.css` → bundled CSS
  - REST-ish API:
    - `GET /api/status` → device status JSON (connectivity proof + debugging)
    - `POST /api/js/eval` → execute JavaScript and return result JSON
    - (optional) `POST /api/js/reset` → reset interpreter state
    - (optional) `GET /api/js/stats` → memory/GC stats

The core mechanism is simple: **the browser sends code**, **the firmware executes it**, **the firmware returns formatted output**.

The non-obvious work is in the scaffolding:

- deterministic embedded asset pipeline,
- request size and evaluation time limits,
- an execution contract that is stable enough to build a UI on top of.

### 2) Firmware module boundaries (recommended)

Name the firmware project directory for clarity (illustrative):

```
0048-cardputer-js-web/
  main/
    app_main.c/.cpp
    wifi_app.c/.h                 # bring-up + IP acquisition + "browse URL"
    http_server.c/.h              # esp_http_server routes and content-types
    js_runner.c/.h                # MicroQuickJS context lifecycle + eval contract
    assets/                       # embedded web build output (Vite)
      index.html
      assets/app.js
      assets/app.css
  web/
    package.json
    vite.config.ts
    index.html
    src/...
```

This decomposition enforces a key design principle:

> **Networking code should not “know” how JavaScript works.**
> The HTTP handler should speak an execution contract (request → response), and `js_runner` should implement that contract.

### 3) Canonical prior art: files and symbols to copy (do not reinvent)

This design is explicitly grounded in the following existing implementations:

- Embedded assets + routing:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
    - asset symbols: `_binary_index_html_start`, `_binary_app_js_start`, `_binary_app_css_start`
    - server start: `http_server_start()`
    - WS helper patterns (useful later): `http_server_ws_broadcast_text()`, `http_server_ws_broadcast_binary()`
- MicroQuickJS evaluation (the “gold” for Phase 1):
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
    - execution: `JsEvaluator::EvalLine()` (calls `JS_Eval(..., JS_EVAL_REPL | JS_EVAL_RETVAL)`)
    - exception path: `JS_IsException()`, `JS_GetException()`
    - formatting: `JS_SetLogFunc()`, `JS_PrintValueF()`, helper `print_value()`
- Asset pipeline rules:
  - `esp32-s3-m5/docs/playbook-embedded-preact-zustand-webui.md`
  - `esp32-s3-m5/0017-atoms3r-web-ui/web/vite.config.ts`

The reading list for other related patterns lives in:

- `esp32-s3-m5/ttmp/.../reference/02-prior-art-and-reading-list.md`

### 4) REST API contract (Phase 1)

The API should be “boring” and firmware-friendly:

- avoid multipart parsing
- avoid many endpoints
- prefer plain-text bodies and small JSON responses

#### `GET /api/status` (connectivity proof)

Response (`application/json`):

```json
{
  "ok": true,
  "device": "cardputer",
  "ip": "192.168.4.1",
  "uptime_ms": 123456,
  "free_heap": 98765
}
```

Why this exists:

- It makes UI debugging easy (“is the device reachable?”).
- It gives a place to show memory pressure and avoid mystery crashes.

#### `POST /api/js/eval` (execute code)

Request:

- Method: `POST`
- Content-Type: `text/plain; charset=utf-8` (recommended)
- Body: JavaScript source text
- Maximum size: explicitly bounded (e.g. 16 KiB) to prevent memory exhaustion

Response (`application/json`):

```json
{
  "ok": true,
  "output": "42\n",
  "error": null,
  "timed_out": false
}
```

Failure case:

```json
{
  "ok": false,
  "output": "",
  "error": "error: ReferenceError: foo is not defined\n",
  "timed_out": false
}
```

Notes:

- The contract above intentionally treats “what the user sees” as a single string (`output` or `error`) at MVP time.
- If we later want richer output (structured values, captured logs, stdout/stderr split), we can evolve the schema to:
  - `result_value`, `logs`, `exception`, etc.
  - but we should not start there without a clear need.

#### Optional: `POST /api/js/reset`

Resets the interpreter to a clean state (clears global variables, resets heap).

This exists because a small embedded JS VM has no shame:

- If we allow “interactive” sessions, state can accumulate.
- Reset is the simplest recovery mechanism for users and for testing.

### 5) Interpreter model (MicroQuickJS)

There are two reasonable models:

1) **Stateless**: create a new JS context per request; evaluate code; discard.
2) **Stateful**: keep one context alive; evaluate successive snippets in the same global environment.

For an editor UI, *stateful* behavior is typically more useful (define helper functions once; run small changes repeatedly). It also matches the existing REPL design:

- `imports/esp32-mqjs-repl/.../JsEvaluator` keeps a `JSContext* ctx_` alive and calls `JS_Eval` repeatedly.

Therefore Phase 1 recommends:

- One global `JSContext` per “device” (not per HTTP client).
- A mutex around evaluation so only one eval runs at a time.

This implies a simple invariant:

> **At most one JavaScript evaluation runs at a time.**

This is a feature:

- It avoids re-entrancy issues and simplifies memory accounting.
- It yields deterministic behavior on a constrained system.

#### Memory budget and fragmentation

The MicroQuickJS implementation in this repo uses a fixed buffer:

- `JsEvaluator::kJsMemSize = 64 * 1024`
- `JsEvaluator::js_mem_buf_[]` (static)

This is attractive on embedded targets because it avoids heap fragmentation inside the JS runtime. The main risk is:

- user code grows the heap until “out of memory”

Mitigations:

- keep code size bounded (request max)
- expose `/api/js/reset` (and possibly auto-reset on OOM)
- optionally return “stats” to UI so memory pressure is visible

#### Timeouts and “infinite loops”

Arbitrary user JS can contain `while(true){}`. If we evaluate it on the HTTP server task, the device becomes unreachable.

Therefore the design recommends implementing a deadline mechanism using QuickJS’s interrupt facility:

- `JS_SetInterruptHandler(ctx, handler)`
- handler returns non-zero when the deadline has passed

Execution contract extension:

- If a deadline is exceeded, return:
  - `ok=false`
  - `timed_out=true`
  - `error="error: timeout\n"`

Even if the first version does not ship timeouts, the design doc treats it as a first-class requirement because it is the difference between “toy” and “usable tool”.

### 6) Output formatting: what does “print results back into the browser” mean?

In a browser REPL, you typically want at least two channels:

1) the value of the last expression (or `undefined`)
2) any explicit output (`print`, `console.log`)

The existing `JsEvaluator` already implements a valuable subset:

- it formats the return value with `JS_PrintValueF(..., JS_DUMP_LONG)`
- it formats exceptions similarly

Phase 1 MVP interpretation:

- “results” = return value and exception text

Optional enhancement (still Phase 1 compatible):

- add a JS global function `print(...args)` (and/or `console.log`) that appends to an output buffer
- return both:
  - `result_output` (formatted value)
  - `stdout` (captured print/log)

Implementation sketch (conceptual):

- before evaluation:
  - clear a `std::string` / ring-buffer output collector
  - register a native function that appends to it
- after evaluation:
  - include the buffer in JSON response

### 7) Frontend UI: Preact + Zustand + an editor component

The UI is a small SPA with three interacting concerns:

- **editor state**: the current code string
- **execution state**: idle/running, last response, errors
- **transport**: making the HTTP request, handling timeouts, rendering response

Zustand is well-suited because it encourages a small “store of facts and actions”:

```ts
type EvalResponse = { ok: boolean; output: string; error: string | null; timed_out: boolean }

type Store = {
  code: string
  running: boolean
  last: EvalResponse | null
  setCode: (code: string) => void
  run: () => Promise<void>
}
```

#### Choosing a code editor library

We need a real editor. Two obvious candidates:

- Monaco (VS Code editor): excellent UX, heavy payload
- CodeMirror 6: modular, smaller, good enough

Because this UI is firmware-hosted (served from flash, over Wi‑Fi, often to a phone), Phase 1 recommends CodeMirror 6:

- smaller bundle footprint,
- modular imports,
- easier to keep deterministic “one JS file” build.

#### Editor integration details (Preact)

In Preact, the standard pattern is:

- create an editor instance once in `useEffect`
- update content via transactions (do not recreate editor per keystroke)
- publish changes to Zustand in a throttled way if needed

This design intentionally does not prescribe a full CodeMirror integration; instead it constrains:

- the editor must not allocate or tear down large objects on each render
- the editor must produce a plain UTF‑8 JS source string for `POST /api/js/eval`

### 8) Asset build pipeline (Vite → embedded assets)

The firmware wants deterministic embedded assets.

We therefore reuse the established playbook:

- output into `main/assets/` directly
- emit stable filenames:
  - `index.html`
  - `assets/app.js`
  - `assets/app.css`
- avoid hashed filenames and dynamic chunks

Concrete implementation reference:

- `docs/playbook-embedded-preact-zustand-webui.md`
- `0017-atoms3r-web-ui/web/vite.config.ts`

This yields a powerful property:

> **Rebuild the UI without changing firmware source.**
> Only the embedded blobs change; the routes stay constant.

### 9) Failure modes and explicit limits (the “boring safety” layer)

Embedded systems fail in predictable ways; we should make those ways explicit.

- Request too large:
  - respond `413 Payload Too Large` (or `400`) with JSON error
- OOM in JS runtime:
  - return `ok=false` with an error string
  - optionally auto-reset interpreter after OOM
- Timeout:
  - return `timed_out=true` and advise user to simplify code
- Server memory pressure:
  - show `free_heap` in `/api/status`
  - consider returning `503` if too low to evaluate safely

## Design Decisions

### Use deterministic embedded assets (no hashed filenames)

Decision:

- Vite config produces a single JS bundle and single CSS file with stable names.

Rationale:

- Firmware route table is static; it should not depend on build-time filename hashes.
- Stability reduces “death by manifest parsing” on embedded devices.

### Use text/plain request bodies for JS evaluation

Decision:

- `POST /api/js/eval` accepts `text/plain` (raw JS).

Rationale:

- Avoids JSON escaping issues for code strings.
- Works naturally with streaming reads via `httpd_req_recv`.

### Prefer a single, stateful JS context (with a mutex)

Decision:

- Keep one MicroQuickJS context alive and evaluate sequentially.

Rationale:

- More useful interactive behavior.
- Matches existing evaluator (`JsEvaluator`) and reduces allocations.

### Prefer CodeMirror 6 over Monaco for the editor

Decision:

- Use CodeMirror 6 for Phase 1.

Rationale:

- Smaller payload, better suited to firmware-hosted UI.
- Easier to keep within “single deterministic bundle” constraints.

## Alternatives Considered

### Serve the UI from an external web server (not device-hosted)

Rejected for the ticket’s purpose:

- breaks the “self-contained tool” story
- adds network dependencies and deployment complexity

### Evaluate JS on a host machine, not on-device

Rejected for the ticket’s purpose:

- eliminates the key value: user-programmable device behavior without reflashing

### Per-request fresh JS contexts (stateless evaluation)

Plausible, but not chosen for Phase 1:

- simpler “purity” story
- worse UX (no persistent state)
- potentially more allocations and slower for repeated runs

## Implementation Plan

1) Clone the embedded asset serving pattern:
   - implement `/`, `/assets/app.js`, `/assets/app.css`, `/api/status`
2) Implement `js_runner` as a small wrapper around the existing `JsEvaluator` pattern:
   - define `js_runner_eval(code, &out_json)`
   - add a global mutex
3) Implement `POST /api/js/eval`:
   - read body with an explicit max size
   - call `js_runner_eval`
   - respond with JSON
4) Build the frontend:
   - Preact app shell + Zustand store
   - CodeMirror integration (minimal: JavaScript syntax highlighting + basic keymap)
   - “Run” button and output panel
5) Tighten safety:
   - add timeout via `JS_SetInterruptHandler`
   - add `/api/js/reset` (optional but recommended)
6) Write a validation playbook:
   - build + flash
   - connect + open UI
   - run a set of “known good” snippets and failure cases (syntax error, OOM, timeout)

## Implementation Retrospective (as of 2026-01-21)

This section describes what was actually implemented in the repo, where it diverged from the earlier design intent, and how the design should be written if we were starting from a blank page today.

### What shipped (file + symbol map)

Firmware (`esp32-s3-m5/0048-cardputer-js-web/`):

- `main/app_main.cpp`
  - `app_main()` — wires Wi‑Fi, console, and (on got-IP) starts the HTTP server.
  - `on_wifi_got_ip()` — got-IP callback that calls `http_server_start()`.
- `main/wifi_mgr.c`
  - `wifi_mgr_start()` — initializes NVS/netif/event loop/Wi‑Fi STA and registers event handlers.
  - `wifi_mgr_set_on_got_ip_cb()` — registers got-IP callback used to start the HTTP server.
  - `wifi_mgr_scan()`, `wifi_mgr_set_credentials()`, `wifi_mgr_connect()`, `wifi_mgr_disconnect()` — console-driven STA UX.
- `main/wifi_console.c`
  - `wifi_console_start()` — registers an `esp_console` command named `wifi`.
  - `cmd_wifi()` — implements `wifi status|scan|join|set|connect|disconnect|clear`.
- `main/http_server.cpp`
  - `http_server_start()` — starts `esp_http_server`, registers routes, and initializes JS runner.
  - `js_eval_post()` — `POST /api/js/eval` handler (bounded body read + JSON response).
  - `status_get()` — `GET /api/status` (STA state + IP + credential flags).
  - `root_get()`, `asset_app_js_get()`, `asset_app_css_get()` — static embedded assets.
  - (Phase 2) `ws_handler()` and `http_server_ws_broadcast_text()` — WS endpoint and broadcaster.
- `main/js_runner.cpp`
  - `js_runner_init()` — allocates fixed memory pool, creates `JSContext*`, installs interrupt handler.
  - `js_runner_eval_to_json()` — `eval(code) -> JSON` contract used by `js_eval_post()`.
  - `interrupt_handler()` — deadline-based evaluation timeout via `JS_SetInterruptHandler()`.
  - `print_value()` — formatting via `JS_SetLogFunc()` + `JS_PrintValueF()`.
- `main/CMakeLists.txt`
  - `EMBED_TXTFILES` — embeds the deterministic Vite output (`assets/index.html`, `assets/assets/app.js`, `assets/assets/app.css`).
  - `PRIV_REQUIRES mquickjs` — pulls in MicroQuickJS engine + stdlib.
- `main/Kconfig.projbuild`
  - `CONFIG_TUTORIAL_0048_JS_MAX_BODY`, `CONFIG_TUTORIAL_0048_JS_MEM_BYTES`, `CONFIG_TUTORIAL_0048_JS_TIMEOUT_MS` — bounds for the REST execution contract.

Frontend (`esp32-s3-m5/0048-cardputer-js-web/web/`):

- `web/src/ui/code_editor.tsx`
  - `CodeEditor` — CodeMirror 6 editor instance (mount once, updates via callbacks; `Mod-Enter` runs).
- `web/src/ui/store.ts`
  - `useStore` — Zustand store with `run()` (REST eval) and (Phase 2) `connectWs()` (WS connect/reconnect).
- `web/src/ui/app.tsx`
  - `App` — renders editor + output panel; shows `/api/status` link; shows WS/encoder telemetry (Phase 2).

### What changed relative to the original Phase 1 design

The code evolved across these commits (chronological, most relevant to Phase 1):

- `9340ed4` — initial skeleton: HTTP server + embedded assets + SoftAP (early) + `/api/js/eval`.
- `b6f3f3e` — replaced `<textarea>` with CodeMirror 6 and rebuilt embedded assets.
- `1f181e1` — build fixes so the firmware builds cleanly on ESP-IDF v5.4.1.
- `0ed4cda` — shifted to STA + `esp_console` Wi‑Fi manager (0046 pattern), starting HTTP server after got-IP.

### Runtime sequence (actual wiring)

The simplest way to understand the “as-built” system is as a sequence diagram with explicit ownership boundaries:

```
app_main()
  wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip)
  wifi_mgr_start()                // starts STA + registers event handlers
  wifi_console_start()            // registers `wifi` console command

on_wifi_got_ip()
  http_server_start()
    js_runner_init()              // creates global JSContext with fixed pool + timeout hook
    httpd_start()
    register routes:
      GET  /                      -> root_get()
      GET  /assets/app.js         -> asset_app_js_get()
      GET  /assets/app.css        -> asset_app_css_get()
      GET  /api/status            -> status_get()
      POST /api/js/eval           -> js_eval_post()

js_eval_post(req)
  read bounded body into buf      // size <= CONFIG_TUTORIAL_0048_JS_MAX_BODY
  json = js_runner_eval_to_json(buf)
  send_json(json)

js_runner_eval_to_json(code)
  lock global mutex
  set deadline = now + timeout
  val = JS_Eval(ctx, code, "<http>", JS_EVAL_REPL|JS_EVAL_RETVAL)
  if exception:
    exc = JS_GetException(ctx)
    err_string = JS_PrintValueF(exc)   // via log func -> std::string
    return {"ok":false,"error":...,"timed_out":...}
  else:
    if val !== undefined:
      out_string = JS_PrintValueF(val)
    return {"ok":true,"output":...,"timed_out":...}
  unlock global mutex
```

Three deliberate properties fall out of this:

1) **Evaluation is serialized** (mutex) so one global `JSContext` is safe-ish for MVP.
2) **Evaluation is bounded** (interrupt handler + deadline) so user code cannot stall forever.
3) **Assets are deterministic** (`app.js`/`app.css` stable names), so firmware routing is constant.

### Divergences (design intent vs implementation reality)

#### Wi‑Fi UX: hybrid/SoftAP vs STA+console

- Design intent: “SoftAP or STA” left as an open question.
- Implementation: **STA-only**, configured via `esp_console` (`wifi scan/join/status`), patterned after `0046`.
- Why: the user explicitly asked for the `esp_console` STA experience, and it yields a reproducible “device joins real network” workflow.
- Implication for the design: this shouldn’t have been an open question in Phase 1; it affects when the HTTP server can start and what `/api/status` should return.

#### Output semantics: return value/exception vs captured `print()`

- Design intent: optional captured “print/console output”.
- Implementation: Phase 1 returns **formatted return value or formatted exception**, but **does not capture `print()` output into the HTTP response**.
  - `print()` (from `imports/.../esp32_stdlib_runtime.c:js_print`) writes to `stdout` via `putchar/fwrite` and `JS_PrintValueF`, which means logs go to the device console, not back to the browser.
- Why: the simplest stable value contract is return-value/exception, and stdout capture requires a deeper integration between stdlib and per-request buffers.
- Implication: the design should distinguish “value printing” (pure formatting) from “user logging” (stdout), because they have different routing.

#### Execution model: mutex vs evaluator task

- Design intent: suggested mutex model as MVP and mentioned “don’t stall server task forever”.
- Implementation: mutex model is used; timeouts are installed; however **evaluation still happens inside the HTTP handler task**.
- Why: lowest complexity to get the vertical slice working.
- Implication: the design should have treated “HTTP handler must not run user code” as a first-class invariant, not a suggestion.

#### `/api/js/reset`

- Design intent: optional but recommended.
- Implementation: not implemented yet (design already predicted it would be valuable).

### What I learned (and what the design should have been from the start)

The “real” design is not “HTTP handler calls `JS_Eval`”. The real design is a small, explicit *execution service* with two carefully chosen contracts:

1) **An input contract** that bounds cost (bytes, time, and concurrency).
2) **An output contract** that is complete enough for a UI (value, logs, error, timing).

If I were writing the Phase 1 design from scratch today, I would start from these invariants:

- The HTTP task should do **parsing and I/O only**. User code runs in a dedicated evaluator task.
- Every request is “executed” by sending a message to the evaluator task and waiting (bounded) for the result.
- Logging and value printing are separate channels:
  - `result_value` (formatted return value)
  - `logs` (captured `print()` / console-like output)
- The JSON encoding must be correct and boring (a real JSON string encoder, not ad-hoc escaping).

#### Revised architecture (recommended)

```
HTTP Task
  POST /api/js/eval
    parse + bound body
    send EvalRequest(code, request_id) -> evaluator queue
    wait (bounded) for EvalResponse(request_id)
    return JSON

Evaluator Task (owns JSContext)
  loop:
    req = queue.recv()
    set up per-request output buffers
    install interrupt deadline
    run JS_Eval
    collect:
      logs (from print)
      value (from return)
      exception (if any)
      duration
    send response back
```

#### Revised pseudocode (with explicit request/response structs)

```c
typedef struct {
  uint32_t id;
  const char* code;
  size_t code_len;
  uint32_t timeout_ms;
} EvalRequest;

typedef struct {
  uint32_t id;
  bool ok;
  bool timed_out;
  uint32_t duration_ms;
  char* value;   // formatted return value (or "")
  char* logs;    // captured print output
  char* error;   // formatted exception (or "")
} EvalResponse;

// HTTP handler (no JS engine calls)
httpd_handler_eval(req):
  code = read_body_bounded(req, MAX_BODY)
  id = next_id()
  send(eval_q, {id, code, len, TIMEOUT})
  if (!recv(reply_q, id, DEADLINE)):
    return 504 + {"ok":false,"timed_out":true,...}
  return 200 + encode_json(response)

// Evaluator task (single owner of ctx)
eval_task():
  ctx = JS_NewContext(fixed_pool, &js_stdlib)
  while (recv(eval_q, &r)):
    buf_reset(value_buf, logs_buf, err_buf)
    set_deadline(ctx, now + r.timeout_ms)
    install_print_sink(ctx, &logs_buf)  // see note below
    v = JS_Eval(ctx, r.code, r.code_len, "<http>", JS_EVAL_REPL|JS_EVAL_RETVAL)
    if (JS_IsException(v)):
      exc = JS_GetException(ctx)
      err_buf = format(exc)
      send(reply_q, {r.id, ok=false, error=err_buf, logs=logs_buf, ...})
    else:
      if (!JS_IsUndefined(v)):
        value_buf = format(v)
      send(reply_q, {r.id, ok=true, value=value_buf, logs=logs_buf, ...})
```

#### Note: how to capture `print()` properly in our repo

In the current stdlib (`imports/.../esp32_stdlib_runtime.c:js_print`), `print()` writes to `stdout`. For browser-returned logs, you want either:

1) a “host log function” native binding (e.g. `__host_log(str)`) that appends to a per-request buffer, and redefine:
   - `globalThis.print = (...args) => __host_log(args.join(' '))`
2) or a modified stdlib generator/runtime where `js_print` writes to an opaque per-request sink instead of `stdout`.

The correct choice depends on whether you want to keep using the imported generator/stdlib unchanged (choice 1) or treat `0048` as owning its own minimal stdlib (choice 2).

## Open Questions

- Wi‑Fi UX: **resolved for 0048** as STA + `esp_console` Wi‑Fi selection (0046 pattern). The remaining question is whether we ever want a hybrid STA+SoftAP fallback.
- Execution model: should we introduce “sessions” (per-browser VM) or keep a single device-global VM?
- Output semantics: do we require browser-returned `print()`/`console.log` capture (logs channel), or only return values/exceptions?
- Authentication: do we require a token/password for eval endpoints, or assume isolated networks?

## References

- `esp32-s3-m5/ttmp/.../reference/02-prior-art-and-reading-list.md` (this ticket)
- `esp32-s3-m5/docs/playbook-embedded-preact-zustand-webui.md`
- `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- ESP-IDF `esp_http_server`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html
- QuickJS upstream: https://bellard.org/quickjs/
