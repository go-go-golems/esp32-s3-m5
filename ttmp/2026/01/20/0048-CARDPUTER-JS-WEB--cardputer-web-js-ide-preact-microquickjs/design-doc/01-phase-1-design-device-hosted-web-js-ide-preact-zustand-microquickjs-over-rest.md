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

## Open Questions

- Wi‑Fi UX: SoftAP-only, STA-only, or hybrid? (Prior art exists for both.)
- Execution model: should we introduce “sessions” (per-browser VM) or keep a single device-global VM?
- Output semantics: do we require `console.log` capture in Phase 1, or only return values/exceptions?
- Authentication: do we require a token/password for eval endpoints, or assume isolated networks?

## References

- `esp32-s3-m5/ttmp/.../reference/02-prior-art-and-reading-list.md` (this ticket)
- `esp32-s3-m5/docs/playbook-embedded-preact-zustand-webui.md`
- `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- ESP-IDF `esp_http_server`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html
- QuickJS upstream: https://bellard.org/quickjs/
