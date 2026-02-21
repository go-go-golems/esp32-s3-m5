# Changelog

## 2026-02-21

- Initial workspace created


## 2026-02-21

Created 8+ page architecture analysis for integrating mquickjs into 0067 matrix firmware with REST script execution, timer primitives, and layered JS API design.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/matrix_engine.c — Current matrix engine analyzed as integration anchor
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_service.cpp — Reusable JS service architecture analyzed and referenced
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md — Primary long-form design deliverable


## 2026-02-21

Recorded detailed implementation diary with prompt context, command traces, findings, and review guidance for follow-on implementation work.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md — Step-by-step diary deliverable


## 2026-02-21

Uploaded bundled design+diary PDF to reMarkable at /ai/2026/02/21/ESP-02-JS-MATRIX-API as ESP-02-JS-MATRIX-API.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md — Included in uploaded bundle
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md — Included in uploaded bundle


## 2026-02-21

Implemented 0067 JS runtime integration end-to-end (runtime bridge, timers, stdlib bindings, REST + esp_console), validated on-device over Wi-Fi, and fixed C3 heap-contiguity failures via arena fallback in `mqjs_service`.

Also changed reset semantics to make soft reset the default (`js reset`, `/api/js/reset`) and added explicit hard reset paths (`js reset hard`, `/api/js/reset-hard`).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/mqjs/js_runtime_bridge.cpp — JS lifecycle, soft/hard reset behavior, eval/memory/status bridge
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/http_server.c — JS REST routes (`eval/reset/reset-hard/stop/mem/status`)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/js_console.c — `js` esp_console parser and examples
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_service.cpp — arena allocation diagnostics + fallback strategy
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/matrix_engine.c — script framebuffer integration and mode handling
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_js_smoke.sh — tracked API smoke test
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_matrix_smoke.sh — tracked matrix regression smoke test


## 2026-02-21

Added complex JS animation examples for matrix scripting and a reusable playback helper. Validated playback of all examples on-device over REST.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/01-plasma-ribbon.js — Procedural dual-ribbon plasma animation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/02-life-torus.js — Toroidal Game of Life animation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/03-comet-trails.js — Multi-comet trail simulation animation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/README.md — Usage documentation for playing/stopping examples
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh — Tracked helper to POST JS files to `/api/js/eval`
