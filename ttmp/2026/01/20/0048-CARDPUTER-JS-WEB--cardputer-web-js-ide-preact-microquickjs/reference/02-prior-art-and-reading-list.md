---
Title: Prior Art and Reading List
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Chain-Series-Internal-FW/Chain-Encder/protocol/M5Stack-Chain-Encoder-Protocol-V1-EN.pdf
      Note: Encoder protocol contract for Phase 2 telemetry
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: Concrete esp_http_server + WS + embedded assets patterns (symbols referenced in reading list)
    - Path: esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c
      Note: Concrete WS broadcaster and endpoint wiring (hub_http_events_broadcast_pb
    - Path: esp32-s3-m5/docs/playbook-embedded-preact-zustand-webui.md
      Note: Deterministic Vite bundling + embedding playbook
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Authoritative MicroQuickJS/QuickJS C API signatures used by evaluator
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Reference implementation of MicroQuickJS eval + exception + formatting
ExternalSources: []
Summary: 'A curated map of in-repo prior art (docs + example firmwares) and external references for implementing a device-hosted Web IDE on Cardputer: embedded Preact/Zustand UI, esp_http_server (REST + WebSocket), and MicroQuickJS execution.'
LastUpdated: 2026-01-20T22:57:10.699115147-05:00
WhatFor: Use as a bibliography and code-map while writing the Phase 1/Phase 2 design and when implementing the firmware; includes filenames and key symbol names to start from.
WhenToUse: Read first when starting work on 0048 so you reuse existing patterns (0017/0029/MO-033/0014/MO-036) instead of rediscovering them.
---


# Prior Art and Reading List

## Goal

Provide a map of (a) the best in-repo documentation and playbooks, (b) the most relevant example firmwares and source files, and (c) a minimal external reading list, so implementation work for `0048-CARDPUTER-JS-WEB` can be grounded in existing patterns rather than improvisation.

## Context

This ticket’s core idea is a *device-hosted* development loop:

- Firmware hosts a modern browser UI (Preact + Zustand + code editor).
- Browser sends JavaScript to the device over a simple REST API.
- Firmware runs that code via MicroQuickJS (QuickJS derivative) and returns results to the browser.

Phase 2 adds “push” telemetry:

- Firmware reads the encoder position + click.
- Firmware sends those values over WebSocket to the browser UI.

All the pieces already exist somewhere in this repo. The point of this document is to point to *where* they exist (docs and code), and to name the “entry points” (files and symbols) so a reviewer can immediately verify claims.

## Quick Reference

### In-repo docs to read (high signal)

#### Device-hosted web UI (HTTP + WS + embedded assets)

- `ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/design-doc/02-final-design-atoms3r-web-ui-graphics-upload-ws-terminal.md`
  - Why: end-to-end “device-hosted UI” architecture, including esp_http_server + WebSocket frame flow and embedded Preact/Zustand assets.
- `docs/playbook-embedded-preact-zustand-webui.md`
  - Why: deterministic Vite bundling rules (single JS + CSS, stable filenames) specifically designed for firmware embedding.
- `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/playbook/01-websocket-over-wi-fi-esp-idf-playbook.md`
  - Why: practical “how to debug WS in ESP-IDF” guide with concrete file pointers and smoke-test scripts.
- `ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/07-guide-device-hosted-ui-pattern-softap-sta-http-ws.md`
  - Why: a higher-level “pattern language” for SoftAP/STA + HTTP + WS device-hosted UX.

#### Preact + Zustand embedded UI patterns (recent, concrete)

- `ttmp/2026/01/20/MO-033-ESP32C6-PREACT-WEB--esp32-c6-device-hosted-preact-zustand-web-ui-with-esp-console-wi-fi-selection/design-doc/01-design-esp32-c6-console-configured-wi-fi-device-hosted-preact-zustand-counter-ui-embedded-assets.md`
  - Why: explicit “deterministic asset embedding + http_server routes” design, plus a console-driven Wi‑Fi join UX.

#### MicroQuickJS / REPL / storage integration

- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md`
  - Why: clean component split (console, REPL, evaluator) and a `JsEvaluator` pattern that maps naturally onto a REST execution endpoint.
- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`
  - Why: repo-accurate, copy/paste-ready manual for adding native bindings and user objects in MicroQuickJS (exact APIs and patterns).
- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/analysis/03-microquickjs-stdlib-atom-table-split-why-var-should-parse-current-state-ideal-structure.md`
  - Why: deep explanation of stdlib vs atom table generation and why “it compiles but crashes” happens when they drift.
- `ttmp/2026/01/01/0023-CARDPUTER-JS-NEXT--cardputer-js-repl-enhancements-and-next-steps-handoff/reference/01-enhancements-and-next-developer-handoff.md`
  - Why: practical handoff notes about stdlib regeneration, `mquickjs_atom.h`, and “what breaks first” when changing bindings.
- `imports/ESP32_MicroQuickJS_Playbook.md`
  - Why: end-to-end developer playbook (setup → integrate → debug) for MicroQuickJS on ESP32.
- `imports/MicroQuickJS_ESP32_Complete_Guide.md`
  - Why: deeper conceptual reference (memory model, C API mental model) to support “textbook-style” documentation.

#### Encoder (position + click) prior art

- `ttmp/2026/01/20/MO-036-CHAIN-ENCODER-LVGL--cardputer-lvgl-list-chain-encoder-navigation/design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md`
  - Why: executable spec + protocol PDF grounding for Chain Encoder framing/CRC semantics and UART integration patterns.

### Example firmwares and “start here” source files (filenames + symbols)

#### Embedded assets + HTTP + WebSocket (C/C++)

- `0017-atoms3r-web-ui/main/http_server.cpp`
  - Asset embedding symbols: `_binary_index_html_start`, `_binary_app_js_start`, `_binary_app_css_start`
  - HTTP server lifecycle: `http_server_start()`
  - WebSocket: `ws_handler()`, `http_server_ws_broadcast_text()`, `http_server_ws_broadcast_binary()`, `http_server_ws_set_binary_rx_cb()`
  - Why: minimal but production-shaped: wildcard URIs, explicit content-types, WS client tracking, async send.
- `0029-mock-zigbee-http-hub/main/hub_http.c`
  - HTTP server lifecycle: `hub_http_start()`
  - WebSocket: `events_ws_handler()`, `hub_http_events_broadcast_pb()`, `hub_http_events_client_count()`
  - Why: battle-tested WS broadcasting with shared buffers and reference counting.

#### Deterministic bundling for firmware embedding (frontend)

- `0017-atoms3r-web-ui/web/vite.config.ts`
  - Why: proven Vite configuration producing deterministic filenames appropriate for embedded serving.
- `0017-atoms3r-web-ui/web/src/store/ws.ts`
  - Why: minimal Zustand WebSocket store structure for reconnect + binary/text framing.

#### MicroQuickJS embedding and evaluation (firmware-side)

- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h`
  - Entry points: `JsEvaluator::EvalLine()`, `JsEvaluator::Reset()`, `JsEvaluator::GetStats()`, `JsEvaluator::Autoload()`
  - Memory budget constant: `JsEvaluator::kJsMemSize`
- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
  - Core evaluation call: `JS_Eval(ctx_, ..., "<repl>", JS_EVAL_REPL | JS_EVAL_RETVAL)`
  - Exception plumbing: `JS_IsException()`, `JS_GetException()`
  - Print/formatting strategy: `JS_SetLogFunc()`, `JS_PrintValueF()`, helper `print_value()`
- `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
  - Engine-level API primitives: `JS_NewContext()`, `JS_Eval()`, `JS_SetInterruptHandler()`, `JS_GC()`
  - Why: authoritative C API signatures and evaluation flags (`JS_EVAL_REPL`, `JS_EVAL_RETVAL`, etc.).

#### Encoder protocol and LVGL input device patterns

- `M5Chain-Series-Internal-FW/Chain-Encder/protocol/M5Stack-Chain-Encoder-Protocol-V1-EN.pdf`
  - Why: primary protocol contract (frame format, CRC rule, commands).
- `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Src/stm32g0xx_it.c`
  - Why: “executable spec” clarifying edge semantics (routing/relay, Index_id handling).
- `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Chain_Function/base_function.c`
  - Why: CRC implementation consistent with the PDF’s narrative rule.

### External (non-repo) references (minimum set)

- ESP-IDF `esp_http_server` (HTTP server + WebSocket support): `https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html`
- WebSocket protocol RFC 6455 (frame model, close handshake, masking): `https://www.rfc-editor.org/rfc/rfc6455`
- QuickJS upstream overview and docs (engine semantics): `https://bellard.org/quickjs/`
- Preact guide: `https://preactjs.com/guide/v10/getting-started/`
- Zustand repository/docs: `https://github.com/pmndrs/zustand`
- CodeMirror 6 docs (candidate code editor; smaller than Monaco): `https://codemirror.net/6/`

## Usage Examples

### “I want to implement Phase 1 in firmware”

1) Read the architecture prior art:
   - `ttmp/.../0013-ATOMS3R-WEBSERVER.../design-doc/02-final-design-...md`
2) Copy the concrete “embedded assets server” structure:
   - `0017-atoms3r-web-ui/main/http_server.cpp` (`http_server_start()`, asset symbols)
3) Use the “JS evaluator” as the reference implementation of “evaluate JS and get a printable result”:
   - `imports/esp32-mqjs-repl/.../eval/JsEvaluator.cpp` (`JsEvaluator::EvalLine()`)

### “I want to implement Phase 2 WebSocket telemetry”

1) Read WS debugging and reliability patterns:
   - `ttmp/.../0029-.../playbook/01-websocket-over-wi-fi-esp-idf-playbook.md`
2) Use a proven WS broadcast pattern:
   - C++ per-client copy: `http_server_ws_broadcast_text()` in `0017-atoms3r-web-ui/main/http_server.cpp`
   - C shared-buffer refcount: `hub_http_events_broadcast_pb()` in `0029-mock-zigbee-http-hub/main/hub_http.c`
3) For encoder semantics and framing:
   - `ttmp/.../MO-036-CHAIN-ENCODER-LVGL.../design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md`

## Related

This document is intentionally “just pointers”. The Phase 1/Phase 2 design documents in this ticket should cite it and then elaborate the concrete architecture, endpoints, message formats, and build pipelines.
