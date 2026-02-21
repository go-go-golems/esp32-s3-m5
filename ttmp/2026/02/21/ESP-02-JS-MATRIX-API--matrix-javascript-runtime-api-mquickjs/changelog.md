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

## 2026-02-21

Added a visual JS diagnostics sequence and guided runner, then debugged why user JS animations appeared inert on hardware.

Root causes and fixes:
- `matrix.stop()` in JS runtime latched the cooperative stop flag, so timer callbacks that checked `matrix.shouldStop()` aborted immediately.
- Timer callback jobs had a hardcoded 100 ms deadline; heavier scripts like `life-torus` were interrupted (`InternalError: interrupted`) and self-cancelled.

Validation results after fixes:
- `diag/06-walk-dot.js` now shows active framebuffer updates (`lit=1` sampled live).
- `01-plasma-ribbon.js`, `02-life-torus.js`, and `03-comet-trails.js` now all produce non-zero, changing lit-pixel counts on-device.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/mqjs/esp32_stdlib_runtime.c — fixed `matrix.stop()` semantics to avoid poisoning `shouldStop()` for new animations
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/mqjs/mqjs_timers.cpp — timer callback timeout now uses `CONFIG_TUTORIAL_0067_JS_EVAL_TIMEOUT_MS`
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/DIAGNOSTIC-SEQUENCE.md — step-by-step visual verification checklist
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/diag/00-env-status.js — diagnostic step set
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_run_js_diagnostics.sh — guided diagnostics runner
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/various/serial-capture-0067-life-debug.log — captured timeout evidence (`InternalError: interrupted`)

## 2026-02-21

Added a project-local JS API documentation spec in `0067` with a detailed getting-started section, comprehensive method/option reference, and trigger examples for REST + console workflows.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md — primary API spec and scripting guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/README.md — cross-link to docs guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md — diary tracking for docs task

## 2026-02-21

Uploaded `0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md` to reMarkable under `/ai/2026/02/21/ESP-02-JS-MATRIX-API`.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md — uploaded API guide source
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md — diary record for upload step

## 2026-02-21

Investigated intermittent long-run JS animation stalls and fixed unbounded timer callback table growth.

### Root cause

`setTimeout` IDs were monotonic and callback entries were nulled (not reused/deleted), causing `globalThis.__0067.timers.cb` key growth over long runtimes.

### Fix

Implemented bounded timeout ID-ring reuse (`1..1024`) in JS runtime so callback table size plateaus.

### Validation

`Object.keys(__0067.timers.cb).length` now stabilizes at `1024` (instead of unbounded growth) under long-running `03-comet-trails.js`.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/mqjs/esp32_stdlib_runtime.c — bounded timeout ID allocator and slot reuse
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md — detailed investigation notes and probe outputs

## 2026-02-21

Added runtime observability fields and a script-level animation registry API (`matrix.anim`) with lifecycle cleanup helpers.

### Included changes

- `/api/js/status` now reports timer usage, animation registry state, and heap metrics.
- `js status` console command now prints the same observability data.
- JS bootstrap now provides `matrix.anim.register/start/stop/clear/list/status` to make animation resources easier to manage.
- Added fallback status probe path to keep status metrics available under load.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/mqjs/js_runtime_bridge.h — expanded status structure
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/mqjs/js_runtime_bridge.cpp — animation registry bootstrap + status probe implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/http_server.c — `/api/js/status` response fields
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/js_console.c — extended `js status` output
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/04-anim-registry-demo.js — runnable registry API example
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md — updated API documentation for observability and `matrix.anim`

## 2026-02-21

Migrated core JS animation examples (`01-plasma-ribbon`, `02-life-torus`, `03-comet-trails`) from legacy ad-hoc global handle management to the structured `matrix.anim` lifecycle API.

### Included changes

- Rewrote examples to register/start named animations through `matrix.anim.register/start`.
- Added explicit cleanup closures per animation.
- Updated `JS-API-GUIDE.md` templates and cancellation guidance to the same `matrix.anim` pattern.
- Updated examples README to document the new structure for scripts `01`..`03`.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/01-plasma-ribbon.js
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/02-life-torus.js
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/js/03-comet-trails.js
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/examples/README.md
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md

## 2026-02-21

Normalized JS API documentation to the `matrix.anim` lifecycle style.

### Included changes

- Updated script lifecycle guidance in `JS-API-GUIDE.md` to recommend `matrix.anim` + `ctx.every/ctx.timeout` for long-running scripts.
- Reframed global timer helpers as low-level/debug APIs.
- Rewrote remaining guide snippets to use structured animation registration/start patterns.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md
