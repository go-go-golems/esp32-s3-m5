# Changelog

## 2026-01-21

- Initial workspace created


## 2026-01-20

Step 1: Bootstrap ticket + vocabulary + diary (commit bb71c08)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md — Created and started frequent diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/vocabulary.yaml — Added QuickJS/JS/REST topics


## 2026-01-20

Step 2: Locate prior art + record reading list (commit 82fa6d7)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/02-prior-art-and-reading-list.md — Added curated doc+firmware map with filenames and symbols


## 2026-01-20

Step 3: Draft Phase 1 design doc (commit 1daac0a)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md — Added Phase 1 architecture


## 2026-01-20

Step 4: Draft Phase 2 WS encoder telemetry design (commit feae3f7)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/02-phase-2-design-encoder-position-click-over-websocket.md — Added Phase 2 WS message schema and encoder streaming architecture


## 2026-01-20

Step 5: Add playbooks and update index/tasks (commit 1d235f8)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/playbook/01-playbook-phase-1-web-ide-smoke-test-build-connect-run-snippets.md — Added Phase 1 smoke-test procedure
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/playbook/02-playbook-phase-2-websocket-encoder-telemetry-smoke-test.md — Added Phase 2 WS telemetry smoke-test procedure


## 2026-01-20

Step 8: Add consolidated MicroQuickJS guide

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/03-guide-using-microquickjs-mquickjs-in-our-esp-idf-setting.md — Repo-specific MicroQuickJS embedding/operation guide


## 2026-01-20

Step 10: Backfill diary + expand implementation task breakdown

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md — Rewrote diary into chronological complete steps
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/tasks.md — Expanded implementation checklist


## 2026-01-20

Step 10: Backfill diary + expand implementation task breakdown (commit c35795d)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md — Added commit hash for step 10


## 2026-01-20

Step 11: Start implementation (create 0048-cardputer-js-web skeleton) (commit 9340ed4)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp — Added embedded asset serving + /api/js/eval
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_runner.cpp — Added MicroQuickJS init + eval formatting
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/store.ts — Added minimal UI state + eval call

## 2026-01-20

Step 13: Replace textarea with CodeMirror 6 and rebuild embedded assets (commit b6f3f3e)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/assets/assets/app.js — Built deterministic embedded JS bundle
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/package.json — Added CodeMirror deps and stabilized build script
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/code_editor.tsx — Added CodeMirror 6 editor component (Mod-Enter run)

## 2026-01-21

Step 14: Source ESP-IDF 5.4.1 and fix firmware build blockers (commit 1f181e1)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/CMakeLists.txt — Limit EXTRA_COMPONENT_DIRS and add required exercizer_control component
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp — Fix HTTP 413 macro + silence httpd_uri initializers
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/sdkconfig.defaults — Enable custom partitions.csv + Cardputer flash defaults


## 2026-01-21

Step 15: Switch to esp_console-configured STA Wi-Fi (0046 pattern) (commit 0ed4cda)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp — /api/status now reports STA state + IP
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.c — esp_console Wi-Fi commands (wifi scan/join/status)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c — STA Wi-Fi manager with NVS credentials + status snapshot
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/playbook/01-playbook-phase-1-web-ide-smoke-test-build-connect-run-snippets.md — Updated Phase 1 playbook to STA/console-first


## 2026-01-21

Step 16: Record ESP-IDF v5.4.1 build verification + expand Phase 2 task breakdown (no code changes)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md — Step 16 build validation and index
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/tasks.md — Phase 2 tasks broken down


## 2026-01-21

Step 17: Phase 2 MVP WS endpoint + browser WS UI + encoder telemetry plumbing (commit a771bb8)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp — encoder -> JSON telemetry broadcaster
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp — /ws handler + WS broadcast
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/store.ts — WS connect/reconnect + parse
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md — Step 17


## 2026-01-21

Design docs: add implementation retrospectives + divergences + revised designs (Phase 1 + Phase 2)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md — Added Implementation Retrospective section grounded in shipped code/commits
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/02-phase-2-design-encoder-position-click-over-websocket.md — Added Implementation Retrospective section grounded in shipped code/commits


## 2026-01-21

Step 18: Fix U+0000 JS parse error by trimming EMBED_TXTFILES NUL; add esp_console 'js eval' (commit d59d037)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp — Trim trailing NUL for embedded app.js/app.css/index.html
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_console.cpp — New 'js eval' console command
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md — Step 18


## 2026-01-21

Step 19: Send encoder clicks as explicit WS events; fix single-click pending bug (commit 52d4f1f)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/chain_encoder_uart.cpp — Store kind+1; add take_click_kind
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp — Broadcast encoder_click events and encoder snapshots
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/store.ts — Parse encoder_click
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md — Step 19


## 2026-01-21

Docs: add Phase 2B design for on-device JS callbacks (Option B); UI: add JS capabilities help panel (commit 731d880)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.tsx — Help text rendered on main page
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/03-phase-2b-design-on-device-js-callbacks-for-encoder-events.md — New design doc for event-driven JS on device


## 2026-01-21

Step 21: Draft Phase 2C design doc (JS→WS arbitrary events) (commit 0b27788)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/04-phase-2c-design-js-websocket-event-stream-device-emits-arbitrary-payloads-to-ui.md — Design doc for JS→WS event stream and UI event history
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/tasks.md — Add Phase 2C task breakdown


## 2026-01-21

Step 22: Draft JS service task + queue structures design (commit 02c4e3e)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/05-phase-2bc-design-js-service-task-event-queues-eval-callbacks-emit-ws.md — Concrete design for JS service ownership + inbound/outbound queues
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/tasks.md — Add Phase 2B+C tasks (design+upload)


## 2026-01-21

Step 23: Add bounded WS event history panel in Web IDE (commit 9b40b88)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/assets/assets/app.js — Rebuilt embedded UI bundle
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.tsx — UI renders scrolling event history + Clear button
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/store.ts — Store keeps bounded history of WS frames


## 2026-01-21

Step 24: Change-driven encoder WS snapshots + white UI text (commit d08c0d2)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp — Only broadcast encoder snapshots on pos/delta changes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.css — White base text color


## 2026-01-21

Step 25: Add js_service task; route REST/console eval through it (commit caff6ce)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp — /api/js/eval now routes through js_service
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_console.cpp — js eval now routes through js_service
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Single-owner MicroQuickJS service task + eval queue


## 2026-01-21

Step 26: Phase 2B encoder.on callbacks (commit 36e7b63)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp — Forward delta/click events into js_service
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Inject encoder.on/off and invoke callbacks in VM


## 2026-01-21

Step 27: Phase 2C emit() + flush js_events over WS (commit 70e5b89)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — emit() bootstrap + __0048_take_for_ws + flush/broadcast
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.tsx — Document emit() + encoder callbacks in UI help


## 2026-01-21

Step 28: Emit overflow produces js_events_dropped WS frame (commit 1a7506c)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Flush now broadcasts js_events_dropped as a separate frame
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/tasks.md — Check legacy Phase 2C tasks 72/73


## 2026-01-21

Step 29: Fix JS bootstraps (MicroQuickJS no arrow functions) (commits 88cdfc6,eae0173)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Rewrite bootstraps to ES5 function syntax
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.tsx — Help panel no longer recommends arrow functions


## 2026-01-21

Step 30: Fix globalThis=null placeholder in stdlib (commit 5581587)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Set globalThis to JS_GetGlobalObject() before bootstraps


## 2026-01-21

Step 31: Add postmortem + playbook for JS service/bindings/events

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/playbook/03-playbook-queue-based-js-service-microquickjs-rest-eval-ws-events-encoder-bindings.md — End-to-end playbook for queue-based JS service + callbacks + emit
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/04-postmortem-microquickjs-bootstrap-failures-arrow-functions-globalthis-null.md — Postmortem and prevention checklist

