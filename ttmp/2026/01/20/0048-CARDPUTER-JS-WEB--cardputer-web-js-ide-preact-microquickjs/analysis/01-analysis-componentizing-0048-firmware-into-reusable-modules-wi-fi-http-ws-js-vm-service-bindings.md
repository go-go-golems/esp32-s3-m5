---
Title: 'Analysis: Componentizing 0048 firmware into reusable modules (Wi-Fi, HTTP/WS, JS VM service, bindings)'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: WS fd list + async send prior art
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: WS broadcaster prior art
    - Path: 0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c
      Note: Original wifi_mgr prior art
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: Asset serving + WS broadcaster patterns
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: Queue-owned VM + stable ctx->opaque/log routing
    - Path: 0048-cardputer-js-web/main/wifi_mgr.c
      Note: Wi-Fi manager duplicated from 0046
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.c
      Note: ctx->write_func(ctx->opaque
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Existing JsEvaluator integration and its ctx->opaque pattern
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:25:09.614282832-05:00
WhatFor: ""
WhenToUse: ""
---


# Analysis: Componentizing 0048 firmware into reusable modules (Wi‑Fi, HTTP/WS, JS VM service, bindings)

## Thesis (acerbic, but fair)

`0048-cardputer-js-web` is not “a firmware”. It is a small framework accidentally assembled out of project-local files:

- a Wi‑Fi provisioning subsystem (STA + NVS creds + scan/connect)
- an interactive console UX for provisioning (esp_console commands)
- an HTTP server that serves embedded assets and exposes a REST API
- a WebSocket hub that maintains a client set and broadcasts frames safely
- a concurrency-safe MicroQuickJS host (single owner, bounded queues, timeouts)
- a hardware event producer (encoder) connected to both JS callbacks and WS telemetry

If we leave these as “project files”, we will keep re-solving the same problems and re-discovering the same edge cases (like `ctx->opaque` being used for multiple purposes).

Componentization is not a style preference. It is how we stop paying interest on every new ticket.

## Executive summary (what to do)

1) Extract the already-duplicated Wi‑Fi + console provisioning code into reusable ESP‑IDF components **without changing its public API** (low risk, immediate payoff).
2) Extract the WebSocket “FD snapshot + async send + free callback” broadcaster into a reusable WS hub component (low risk, used by 0017/0029/0048).
3) Introduce a shared MicroQuickJS host primitive (`mqjs_vm`) that makes the engine invariants explicit:
   - stable `ctx->opaque` type
   - a log router (capture vs console) rather than pointer swapping
   - one place to apply `globalThis` fixes and syntax constraints
4) Keep `JsEvaluator` (REPL) API stable by refactoring it *internally* to use the new host primitive. Existing firmwares keep working; the internals stop being a landmine when reused outside the REPL.

This document describes the current coupling, proposes component boundaries and APIs, and explicitly analyzes backward compatibility impact.

## Scope / Non-goals

In scope:

- How to split 0048 into reusable components with stable APIs.
- How that relates to existing prior art (especially `imports/esp32-mqjs-repl`’s `JsEvaluator` + stdlib runtime).
- What changes would be breaking, what can be additive, and how to keep existing firmwares working.

Non-goals:

- Actually performing the refactor (this is a design/analysis doc).
- Perfect library naming (we propose concrete names, but the main point is boundaries + invariants).
- Adding compatibility shims unless explicitly demanded later.

## The current 0048 architecture (what exists today)

### Top-level module map (0048)

Firmware project root:

- `esp32-s3-m5/0048-cardputer-js-web/`
  - `main/app_main.cpp` — brings up console, Wi‑Fi, HTTP, JS service
  - `main/wifi_mgr.c/.h` — Wi‑Fi STA manager + NVS credential handling + scan/connect
  - `main/wifi_console.c/.h` — esp_console command `wifi ...` (scan/join/status/connect/clear)
  - `main/http_server.cpp/.h` — esp_http_server routes: assets, `/api/status`, `/api/js/eval`, `/ws`
  - `main/js_service.cpp/.h` — queue-owned MicroQuickJS context (eval + callbacks + emit→WS)
  - `main/js_console.cpp/.h` — esp_console `js eval ...` command (routes to `js_service`)
  - `main/encoder_telemetry.cpp/.h` — encoder poll task; forwards delta/click to JS service; broadcasts WS telemetry
  - `main/chain_encoder_uart.cpp/.h` — M5 chain encoder UART driver (optional)
  - `web/` — Preact UI + deterministic build into `main/assets/`

### Ownership graph (this is the real API)

There are two “single-threaded islands” with strict ownership:

1) **MicroQuickJS context** is owned by `js_service` task only.
2) **esp_http_server** internal thread(s) are owned by ESP-IDF; user tasks must use `httpd_ws_send_data_async` rather than attempting to send WS frames from arbitrary contexts.

Everything else is “producers that enqueue work”:

- HTTP POST handler enqueues eval requests → JS service.
- Encoder task enqueues click/delta → JS service and broadcasts telemetry via WS.
- JS service flushes JS-emitted events to WS.

### Incident-driven invariants (hard-earned)

0048 now documents and implements two invariants that any reusable component must keep intact:

1) **MicroQuickJS feature set is ES5-ish in this repo**:
   - Arrow functions in bootstraps caused `SyntaxError: invalid lvalue`.
2) **MicroQuickJS uses `ctx->opaque` for multiple subsystems**:
   - both timeout interrupt handler and printing/logging (`JS_PrintValueF`).
   - treating `ctx->opaque` as “a random pointer slot” causes crashes.

See:

- `reference/04-postmortem-microquickjs-bootstrap-failures-arrow-functions-globalthis-null.md`
- `reference/05-postmortem-js-print-ev-crash-ctx-opaque-log-func.md`

## Candidate component boundaries (what to extract, and why)

Component boundaries should follow two rules:

1) A component owns a resource with hard invariants (e.g., a VM context, a WS client set).
2) A component exposes an API that is stable across many projects and easy to reason about.

### 1) Wi‑Fi manager (`wifi_mgr`) — extract first

Evidence it is already a reusable component in practice:

- `0048-cardputer-js-web/main/wifi_mgr.c` is explicitly “Adapted from: 0046”.
- `0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c/.h` exports the same API:
  - `wifi_mgr_start()`, `wifi_mgr_get_status()`, `wifi_mgr_scan()`, `wifi_mgr_set_credentials()`, …

Public API today (0046 + 0048; identical signatures):

- `esp_err_t wifi_mgr_start(void);`
- `esp_err_t wifi_mgr_get_status(wifi_mgr_status_t* out);`
- `esp_err_t wifi_mgr_scan(wifi_mgr_scan_entry_t* out, size_t max_out, size_t* out_n);`
- `esp_err_t wifi_mgr_set_credentials(const char* ssid, const char* password, bool save_to_nvs);`
- `esp_err_t wifi_mgr_connect(void);`
- `void wifi_mgr_set_on_got_ip_cb(wifi_mgr_on_got_ip_cb_t cb, void* ctx);`

Recommendation:

- Create an ESP‑IDF component under `esp32-s3-m5/components/` (or `components/networking/`) that provides `wifi_mgr.c/.h`.
- Keep the function names and semantics *exactly* as-is in the first extraction.

Backward compatibility impact:

- Existing projects that keep their local `wifi_mgr.c` continue to build.
- Projects migrating to the component update include paths and `CMakeLists.txt` dependencies, but do not need logic changes.
- No breaking API changes required.

Potential improvements (additive, optional later):

- Add optional hooks: “on status change” callback, “on disconnect reason” callback.
- Add a `wifi_mgr_stop()` and document idempotency.

### 2) Wi‑Fi console provisioning (`wifi_console`) — extract alongside wifi_mgr

The console UX is also duplicated (0046 and 0048 both have it), and it is pure glue:

- `wifi` command implements: `status`, `scan`, `join`, `set`, `connect`, `disconnect`, `clear`.
- It depends only on `esp_console`, `wifi_mgr`, and some printing helpers.

Recommendation:

- Extract into `components/wifi_console/` (or bundle with `wifi_mgr` as `netprov_console`).
- Export a single entry point:
  - `void wifi_console_start(void);`

Backward compatibility impact:

- Same as above: migrate via include/dependency changes only.

### 3) Embedded asset server (`httpd_assets_embed`) — extract as a micro-component

0048 contains a very specific bug fix that is easy to reintroduce:

- Browser error `Uncaught SyntaxError: illegal character U+0000` caused by serving embedded assets including a trailing NUL from the assembler-generated blob.
- Fix implemented via `embedded_txt_len(start,end)` which trims a trailing 0 byte.

This is the kind of “one-line fix” that becomes an expensive mystery when rediscovered on a new ticket.

Recommendation:

- Extract a tiny helper that serves embedded assets robustly:
  - `size_t httpd_embedded_len_trim_nul(const uint8_t* start, const uint8_t* end);`
  - `esp_err_t httpd_send_embedded(httpd_req_t* req, const char* content_type, const uint8_t* start, const uint8_t* end, const char* cache_control);`

Backward compatibility impact:

- None; this is new.

### 4) WebSocket hub (`httpd_ws_hub`) — extract the FD snapshot + async send pattern

Prior art already exists in multiple projects:

- `0017-atoms3r-web-ui/main/http_server.cpp` uses per-client payload copy + `httpd_ws_send_data_async` + free callback.
- `0029-mock-zigbee-http-hub/main/hub_http.c` uses a similar async send pattern.
- `0048-cardputer-js-web/main/http_server.cpp` duplicates that pattern again.

The repeated pattern is:

1) Track connected WS client FDs (add/remove; prune stale).
2) Snapshot the FD list under a lock.
3) For each FD, `malloc` a copy of the payload and send with `httpd_ws_send_data_async`.
4) Free in callback and drop clients on send failure / non-websocket fd info.

Recommendation:

- Extract to `components/httpd_ws_hub/` with an explicit API:

```c
typedef struct ws_hub ws_hub_t;

ws_hub_t* ws_hub_create(httpd_handle_t server, size_t max_clients);
void ws_hub_destroy(ws_hub_t* hub);

void ws_hub_on_connect(ws_hub_t* hub, int fd);   // called from ws handler on HTTP_GET
void ws_hub_on_close(ws_hub_t* hub, int fd);     // called on CLOSE or error

esp_err_t ws_hub_broadcast_text(ws_hub_t* hub, const char* text);
```

Design note:

- Keep it dumb. No JSON parsing. No application schema.
- Accept that max clients is small; keep fixed-size arrays for predictability.

Backward compatibility impact:

- Projects can keep their local broadcaster until migrated.
- Migration requires only replacing local client-list code with hub usage.

### 5) HTTP API skeleton (`httpd_api_mini`) — optional extraction

There are small patterns worth sharing, but they are “optional” because HTTP APIs vary more by project:

- `send_json(...)` helper sets type + cache-control.
- Bounded POST body read (max length, proper error codes).
- Status endpoint formatting.

Recommendation:

- Keep as a collection of helper functions, not a “framework”.
- Extract only if at least 3 projects need the exact same helpers.

### 6) Encoder telemetry (`encoder_service`) — split driver from policy

0048’s `encoder_telemetry.cpp` currently mixes:

- device-specific driver config (`ChainEncoderUart::Config`)
- event aggregation (`take_delta`, `take_click_kind`)
- policy decisions:
  - when to broadcast
  - what JSON schema to emit over WS
  - when to forward to JS callbacks

Recommendation:

- Define a narrow driver interface (pure data acquisition) and a service/policy module:

Driver interface:

- `bool encoder_driver_init(const encoder_driver_config_t* cfg);`
- `int32_t encoder_driver_take_delta(void);`
- `int encoder_driver_take_click_kind(void);`

Service module responsibilities:

- coalesce deltas, create event structs, and forward to:
  - WS telemetry broadcaster
  - JS service callback queue

Backward compatibility impact:

- This is mostly internal to 0048 today. Extracting can be done without affecting other projects until adopted.

### 7) MicroQuickJS host (`mqjs_vm`) + service (`mqjs_service`) — the high-value extraction

This is the most valuable componentization target, and also the one with the sharpest edges.

We already have a prior-art “JS evaluator”:

- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h`:
  - `class JsEvaluator final : public IEvaluator`
  - `EvalResult EvalLine(std::string_view line)`
  - `Reset`, `GetStats`, `Autoload`

But 0048’s needs differ:

- multiple producers (HTTP handler, encoder task) that must not touch the VM
- asynchronous callbacks
- bounded timeouts for both eval and callbacks
- an outbound “JS emits events” channel flushed to the browser

The correct reuse story is not “use JsEvaluator everywhere”.
The correct story is “identify the minimal engine-hosting primitive that both JsEvaluator and js_service can use safely”.

#### 7.1: Identify the “engine-hosting primitive”

Call it `mqjs_vm` (name negotiable). It should own:

- `JSContext* ctx`
- fixed arena buffer
- a stable `ctx->opaque` struct (the only type ever stored in `ctx->opaque`)
- a log router function (capture vs stdout)
- an interrupt handler (timeouts)
- a place to apply global object fixes (`globalThis`)
- a helper to print/format JS values safely

Why this matters:

- The `print(ev)` crash in 0048 is an engine-hosting bug, not a “feature bug”.
- These are cross-cutting invariants that should not be re-implemented in each project.

Concrete evidence (engine behavior):

- In `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.c`:
  - `JS_SetContextOpaque` sets `ctx->opaque`
  - `JS_SetLogFunc` sets `ctx->write_func`
  - `js_printf/js_vprintf` call `ctx->write_func(ctx->opaque, ...)`
  - `JS_PrintValueF` ultimately prints via that same path

Therefore, any safe embed must treat `ctx->opaque` and `ctx->write_func` as a coupled pair.

#### 7.2: How this relates to the existing JsEvaluator (REPL)

The existing `JsEvaluator.cpp` uses this pattern:

```c++
JS_SetContextOpaque(ctx, &out);
JS_SetLogFunc(ctx, write_to_string);
JS_PrintValueF(ctx, v, JS_DUMP_LONG);
```

It does not restore a stable opaque, and it leaves the write function installed.

In the REPL, this can “appear to work” if JS code never triggers printing in a context where the opaque is invalid.
But as a reusable foundation, it is brittle.

Recommendation:

- Do not change the public `IEvaluator` or `JsEvaluator` API.
- Refactor `JsEvaluator` to use `mqjs_vm` internally:
  - `JsEvaluator` becomes a small adapter from “line-based REPL evaluator” to “VM host primitive”.
  - Existing users of `JsEvaluator` keep working.

Impact on existing firmwares:

- Minimal to none, because the API surface remains `IEvaluator` + `JsEvaluator` methods.
- Behavior changes are limited to bug fixes and more robust `print` handling; that is acceptable if documented.

#### 7.3: The service layer (`mqjs_service`) for multi-producer systems

`mqjs_vm` is the “engine”.
`mqjs_service` is the “concurrency wrapper”.

Service responsibilities:

- owns a task/thread that is the only caller of `mqjs_vm`/MicroQuickJS
- accepts requests from producers via queues:
  - eval requests (with response path)
  - hardware events (delta/click)
- invokes callbacks with deadlines
- flushes outbound events to transports (WS, logs)

In 0048, this is concretely:

- `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
  - `js_service_eval_to_json(...)`
  - `js_service_post_encoder_click(...)`
  - `js_service_update_encoder_delta(...)`
  - bootstraps:
    - encoder callback registration (`encoder.on/off`)
    - `emit(...)` + `__0048_take_lines(...)` flush

Recommendation:

- Extract `mqjs_service` as a reusable component that depends on `mqjs_vm`.
- Keep 0048-specific JS bootstraps (encoder and event schemas) outside the generic service, or allow them via “bootstrap string list” provided at init time.

Backward compatibility impact:

- New component; no breakage for old projects.
- Adoption is opt-in.

## Compatibility analysis (what breaks, what doesn’t)

This is the key question: can we improve shared code without breaking existing firmwares?

### Wi‑Fi manager / console

Extraction strategy:

- Lift files into a shared component, keep APIs identical.

Impact:

- No API change, only include/path and build dependency changes for projects that adopt it.
- Projects that do not adopt it are unaffected.

### WS hub

Extraction strategy:

- Introduce a new component and migrate projects one by one.

Impact:

- No API change in existing projects until they opt in.

### MicroQuickJS: where changes can be dangerous

There are three layers:

1) `mquickjs` engine (`components/mquickjs/mquickjs.c/.h`) — foundational; changing it affects *everything*.
2) stdlib runtime (`main/esp32_stdlib_runtime.c`) — shared by projects that include that stdlib.
3) embeddings (`JsEvaluator`, `js_service`) — application-level integration.

Rule:

- Prefer changing layer (3) first.
- Change layer (2) only if needed.
- Change layer (1) only if you must, and treat it like a platform change.

#### Can we refactor JsEvaluator without breaking old firmwares?

Yes, if we keep the public API stable:

- `IEvaluator` remains the same.
- `JsEvaluator` remains the same from callers’ perspective.

Potential behavioral differences (acceptable with doc updates):

- `print(...)` becomes reliably safe (no crashes).
- `ctx->opaque` is always a stable struct, which may indirectly affect any obscure code relying on reading it (unlikely; `JsEvaluator` does not expose `JSContext*`).

#### Would adding timeouts to JsEvaluator be breaking?

It can be additive if:

- default timeout is “disabled” (or large enough to not change behavior),
- timeout is configurable via a new setter or config flag.

But timeouts are also a semantic change: they can turn infinite loops into exceptions. That is a behavior change even if “improvement”.

Recommendation:

- Keep JsEvaluator semantics stable.
- Put “bounded deadline” behavior in `mqjs_service` (systems that need it already accept that constraint).

## Proposed component library structure (concrete, adoptable)

### Component set (suggested)

Networking/provisioning:

- `components/wifi_mgr/`
  - `wifi_mgr.c/.h`
  - `Kconfig` for optional defaults (scan list size, reconnect policy)
- `components/wifi_console/`
  - `wifi_console.c/.h`
  - depends on `wifi_mgr`, `esp_console`

HTTP/WS:

- `components/httpd_assets_embed/`
  - helpers for embedded blobs + NUL trimming + cache headers
- `components/httpd_ws_hub/`
  - fd tracking + broadcast helpers built on `httpd_ws_send_data_async`

JS:

- `components/mqjs_vm/`
  - owns `JSContext` + log router + stable opaque + print helpers + globalThis fix
- `components/mqjs_service/`
  - owns a task + queues; consumes `mqjs_vm`
  - optional “bootstrap strings” hook for application-provided JS glue

Hardware:

- `components/encoder_service/`
  - common event structs, coalescing policies
  - depends on a driver interface component

### Wiring pattern (how projects consume)

In a project’s `main/CMakeLists.txt`:

- depend on components for “boring” parts:
  - wifi provisioning
  - WS hub
  - JS host
- keep project-local code for:
  - application schema choices (what JSON frames look like)
  - application-specific bindings (what globals exist in JS)
  - application UI assets and endpoints

## Recommendations for API design (to keep reuse real)

1) Keep component APIs small and boring.
   - “Boring” APIs survive reuse; “clever” ones become untestable.
2) Do not leak heavy types across component boundaries unless necessary.
   - For example, `httpd_handle_t` is okay for WS hub.
   - Exposing `JSContext*` widely is not okay for JS service: it invites violating ownership.
3) Make invariants executable:
   - `mqjs_vm_init()` should install `globalThis` and the log router by default.
   - provide a “self-test” hook in debug builds (e.g., `print({a:1})` sanity).
4) Prefer additive evolution over “refactor and break”.
   - Most improvements can be added as new functions or optional config without breaking call sites.

## Concrete “next extraction” plan (low risk → high payoff)

1) Extract `wifi_mgr` + `wifi_console` into components.
   - Update 0046 + 0048 to use them.
2) Extract WS hub from 0017/0029/0048 into one reusable component.
3) Introduce `mqjs_vm` and migrate 0048 `js_service` to use it internally.
4) Refactor `imports/esp32-mqjs-repl/.../JsEvaluator` internally to use `mqjs_vm` without changing its header API.
5) (Optional) Share JS bootstraps as “bundled bootstrap strings” patterns, but keep application-specific globals separate.

## Appendix: Key files and symbols cited

0048:

- `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
  - `embedded_txt_len(...)` (NUL trimming)
  - `http_server_ws_broadcast_text(...)` (async send pattern)
- `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
  - `ensure_ctx()`
  - `struct CtxOpaque` / `write_to_log(...)` / `print_value(...)` (stable opaque + log routing)
  - `install_bootstrap_phase2b()` (encoder callbacks)
  - `install_bootstrap_phase2c()` + `__0048_take_lines` (event queue flush)
- `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c/.h`
  - `wifi_mgr_*` API

Prior art:

- `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c/.h`
  - earliest instance of the Wi‑Fi manager API used in 0048
- `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
  - WS fd list + async send pattern
- `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - WS broadcast helper pattern
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h/.cpp`
  - existing REPL-oriented evaluator API and implementation
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.c`
  - `JS_SetContextOpaque`, `JS_SetLogFunc`, and the write path (`ctx->write_func(ctx->opaque, ...)`)
