---
Title: 'Playbook: Queue-based JS service (MicroQuickJS) + REST eval + WS events + encoder bindings'
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
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/chain_encoder_uart.cpp
      Note: Example hardware driver producing delta + click events
    - Path: 0048-cardputer-js-web/main/encoder_telemetry.cpp
      Note: |-
        Encoder event producer; forwards click/delta to js_service; WS telemetry frames
        Encoder event producer + forwarding
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: |-
        REST endpoint `/api/js/eval`; WS broadcast helper; asset serving patterns
        REST eval endpoint and WS broadcaster
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: |-
        JS service task; eval queue; encoder callback invocation; emit+flush to WS; globalThis fix
        Reference implementation of JS service + bootstraps + flush
    - Path: 0048-cardputer-js-web/web/src/ui/app.tsx
      Note: UI help text; event history panel
    - Path: 0048-cardputer-js-web/web/src/ui/store.ts
      Note: |-
        WS client parsing; bounded event history in UI
        UI WS history (debugging)
    - Path: 0048-cardputer-js-web/web/vite.config.ts
      Note: Deterministic asset output for firmware embedding
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: 'MicroQuickJS APIs: JS_Eval, JS_Call, interrupt handler, GC refs'
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c
      Note: Prior art for custom builtins and IO helpers in this repo’s MicroQuickJS stdlib
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T14:54:48.206432008-05:00
WhatFor: A repeatable, end-to-end recipe for building an embedded 'device-hosted Web IDE' where a queue-owned MicroQuickJS VM serves REST eval, processes hardware callbacks (encoder), and emits structured events to the browser over WebSocket.
WhenToUse: Use when creating a new firmware that embeds a JS VM and needs safe concurrency (multiple producers) plus a browser UI and realtime event stream.
---


# Playbook: Queue-based JS service (MicroQuickJS) + REST eval + WS events + encoder bindings

## Purpose

Build a repeatable embedded architecture:

- A single-owner **JS service task** runs MicroQuickJS (no concurrent VM access).
- A browser **code editor UI** sends JS snippets over REST (`POST /api/js/eval`).
- Hardware events (encoder delta/click) invoke **on-device JS callbacks** (`encoder.on(...)`).
- JS can publish structured events back to the browser via WebSocket (`emit(...)` → `js_events` frames).

The hard part is not “call `JS_Eval`”. The hard part is making the system safe, bounded, and debuggable under concurrency. This playbook is explicit about invariants, ordering, and failure modes.

## Environment Assumptions

Tooling:

- ESP-IDF v5.4.1 (or compatible) configured for ESP32-S3.
- Node.js toolchain (Vite build) for the embedded UI.

Repo assumptions (example paths):

- Firmware project exists at `esp32-s3-m5/<your-project>/`
- Embedded UI built into `main/assets/` with deterministic filenames:
  - `index.html`
  - `assets/app.js`
  - `assets/app.css`

Hardware assumptions:

- An encoder exists (built-in GPIO or external like M5 Chain Encoder UART).

Critical runtime constraints:

- MicroQuickJS context is not thread-safe: **single owner task only**.
- Treat the JS feature set as ES5 unless proven otherwise (avoid arrow functions).
- The imported stdlib may define `globalThis` as `null` (you must set it in C).

## Commands

These commands validate the whole stack: UI embed → REST eval → WS stream → callbacks → JS→WS events.

### 0) Build embedded web assets (deterministic filenames)

```bash
cd esp32-s3-m5/0048-cardputer-js-web/web
npm ci
npm run build
```

Exit criteria:

- `../main/assets/index.html`
- `../main/assets/assets/app.js`
- `../main/assets/assets/app.css`

### 1) Build + flash firmware

```bash
cd ../
source /home/manuel/esp/esp-idf-5.4.1/export.sh
idf.py -B build_esp32s3_v2 set-target esp32s3
idf.py -B build_esp32s3_v2 build
idf.py -B build_esp32s3_v2 flash monitor
```

### 2) Configure Wi‑Fi (STA) and open the UI

On the device console:

```text
0048> wifi scan
0048> wifi join 0 <password> --save
0048> wifi status
```

Exit criteria:

- Device prints browse URL, e.g. `http://<ip>/`
- Browser loads the embedded UI and shows WS connected

### 3) REST eval verification

```bash
curl -sS "http://<ip>/api/js/eval" \
  -H 'Content-Type: text/plain; charset=utf-8' \
  --data-binary '1+1' | jq .
```

### 4) Verify bootstrapped globals exist (encoder + emit)

```bash
curl -sS "http://<ip>/api/js/eval" \
  -H 'Content-Type: text/plain; charset=utf-8' \
  --data-binary 'typeof encoder + \" \" + typeof emit' | jq -r .output
```

Expected output:

```text
"object function"
```

### 5) Emit a WS-visible event

Run in the browser editor:

```js
emit("hello", {x:1})
"ok"
```

Exit criteria:

- WS history panel shows a `type:"js_events"` frame containing topic `hello`.

### 6) Register callbacks and emit from them (Phase 2B + Phase 2C together)

Run in the browser editor (ES5 syntax):

```js
encoder.on("click", function(ev) { emit("click", ev) })
encoder.on("delta", function(ev) { emit("delta", ev) })
"callbacks installed"
```

Then rotate/click the encoder:

- you should see `js_events` frames with topic `click` and `delta` in WS history.

### 6a) Verify `print(ev)` inside callbacks does not crash

This is the fastest regression test for a specific class of MicroQuickJS integration bugs (log sink + `ctx->opaque` type confusion).

Run:

```js
encoder.on("click", function(ev) {
  print(ev);           // should print to serial console without crashing
  emit("click", ev);   // should appear in WS history
})
"ok"
```

Exit criteria:

- Pressing the encoder button does not reboot/panic the device.
- Serial console prints a dumped object (or at least prints something sane).
- Browser WS history shows `js_events` with topic `click`.

## Exit Criteria

By the end you can demonstrate all four layers working together:

1) The device serves the UI (`/`, `/assets/app.js`, `/assets/app.css`) with no browser parse errors.
2) The REST eval endpoint returns formatted results and errors.
3) Encoder events reach JS callbacks (`encoder.on('delta'| 'click', fn)`).
4) JS-emitted events show up in the browser WS history as `type:"js_events"`.

## Notes

### MicroQuickJS invariant: `ctx->opaque` is shared by timeouts and printing

MicroQuickJS uses `ctx->opaque` in multiple subsystems. In particular:

- The `JS_SetInterruptHandler` timeout path receives `ctx->opaque` as its `opaque` argument.
- `print()` and object formatting (`JS_PrintValueF` / `js_dump_object`) write bytes via `ctx->log_func`, which also receives an `opaque` pointer derived from the same place.

Implication:

- Do not install a log sink that assumes `ctx->opaque` points at some other type (like `std::string*`) unless `ctx->opaque` is *always* of that type.
- If you need to “capture printing into a string”, implement a **log router**: keep `ctx->opaque` a stable struct containing both timeout state and an optional capture pointer, and have the log sink branch on whether capture is active.

Reference implementation:

- `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp` (`struct CtxOpaque`, `write_to_log`, `print_value`)
- Postmortem: `reference/05-postmortem-js-print-ev-crash-ctx-opaque-log-func.md`

Anti-pattern to avoid copying:

- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` currently uses a “swap `ctx->opaque` to a local `std::string`” pattern in `print_value(...)` and does not restore a stable `ctx->opaque`. It works for that REPL use-case only as long as no JS code path triggers printing while `ctx->opaque` is in an invalid state; it is not a safe template for callback-driven or timeout-enabled systems.

## Implementation guide (textbook-style “build the machine”)

This playbook is command-oriented, but new developers need a mental model. This section gives that model and then translates it into code structure.

### The machine

We are building a small distributed system:

- **Browser**: editor + output + WS log
- **Firmware HTTP server**: serves UI, implements REST, maintains WS clients
- **Firmware JS service**: owns the VM, executes eval/callbacks, flushes events to WS
- **Hardware drivers**: produce events (encoder)

The critical invariant is the single-owner rule:

> Only the JS service task is allowed to call MicroQuickJS APIs.

### Minimal file/symbol checklist (to replicate)

- `main/js_service.cpp`
  - `js_service_start`
  - `js_service_eval_to_json`
  - VM init: `ensure_ctx`:
    - `JS_NewContext(...)`
    - `JS_SetInterruptHandler(...)`
    - set `globalThis` to global object
    - run bootstraps
  - callback invocation:
    - `handle_encoder_delta`, `handle_encoder_click`
  - event flush:
    - `flush_js_events_to_ws`
- `main/http_server.cpp`
  - `POST /api/js/eval` → `js_service_eval_to_json`
  - `/ws` endpoint and `http_server_ws_broadcast_text`
- `main/encoder_telemetry.cpp`
  - produces delta/click
  - forwards to `js_service_update_encoder_delta` / `js_service_post_encoder_click`
- `web/src/ui/store.ts`
  - WS client and bounded event history

### Adding native bindings (optional, deeper)

If you must add a true C function callable from JS:

1) Look at prior art:
   - `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
2) Understand the stdlib generation table:
   - `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h`
3) Prefer injected JS helpers first; only add native functions when you need:
   - IO, performance, or privileged operations.
