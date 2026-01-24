# Changelog

## 2026-01-23

- Initial workspace created


## 2026-01-23

Wrote REST+Web IDE design doc; expanded diary; added/checked tasks; related key reference source files.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066a-cardputer-js-rest-api-and-web-ide--cardputer-js-rest-api-web-ide-microquickjs/analysis/01-design-rest-api-web-ide-for-microquickjs.md — Design document updated with file relations
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066a-cardputer-js-rest-api-and-web-ide--cardputer-js-rest-api-web-ide-microquickjs/reference/01-diary.md — Diary updated with step-by-step research notes


## 2026-01-23

Begin implementation phase: added 0066 integration tasks and diary step for REST+WS+Web IDE rollout.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066a-cardputer-js-rest-api-and-web-ide--cardputer-js-rest-api-web-ide-microquickjs/reference/01-diary.md — Recorded implementation plan and target firmware
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066a-cardputer-js-rest-api-and-web-ide--cardputer-js-rest-api-web-ide-microquickjs/tasks.md — Added implementation tasks (7-12)


## 2026-01-23

Implement 0066 web IDE plumbing: embedded assets + /ws WebSocket endpoint (commit b7d17ea).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/assets/app.js — Browser-side eval + WS client
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/assets/index.html — Web IDE shell UI
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp — Added /assets/* and /ws endpoint + broadcast helper


## 2026-01-24

Implemented /api/js/eval + emit() buffer + flush to /ws in tutorial 0066 (commits 0d47667, eb7cea2).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp — Added /api/js/eval (raw body)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp — Added eval->JSON + emit buffer + WS flush
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/httpd_ws_hub/httpd_ws_hub.c — Added CONFIG_HTTPD_WS_SUPPORT stubs


## 2026-01-24

Flashed 0066 to /dev/ttyACM0; added smoke script; noted monitor requires TTY and left final interactive validation as task 13.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066a-cardputer-js-rest-api-and-web-ide--cardputer-js-rest-api-web-ide-microquickjs/reference/01-diary.md — Recorded flash + monitor limitation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066a-cardputer-js-rest-api-and-web-ide--cardputer-js-rest-api-web-ide-microquickjs/scripts/0066a-smoke-http-ws.sh — HTTP/WS smoke test helper

