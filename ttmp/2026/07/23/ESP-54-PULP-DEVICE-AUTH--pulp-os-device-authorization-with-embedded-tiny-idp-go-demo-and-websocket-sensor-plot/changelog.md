# Changelog

## 2026-07-23

- Initial workspace created


## 2026-07-23

Created the evidence-backed intern implementation guide and investigation diary: strict embedded tiny-idp requires no source changes; proposed native auth/token ownership, protected REST/WebSocket APIs, and a bounded e-ink plot architecture. Focused tiny-idp device tests passed.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/23/ESP-54-PULP-DEVICE-AUTH--pulp-os-device-authorization-with-embedded-tiny-idp-go-demo-and-websocket-sensor-plot/design-doc/01-device-authorization-and-realtime-demo-analysis-design-and-intern-implementation-guide.md — Primary 17-section design and implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/23/ESP-54-PULP-DEVICE-AUTH--pulp-os-device-authorization-with-embedded-tiny-idp-go-demo-and-websocket-sensor-plot/reference/01-investigation-diary.md — Chronological research evidence and decisions


## 2026-07-23

Validated design, diary, and index frontmatter; docmgr doctor passed all checks.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/23/ESP-54-PULP-DEVICE-AUTH--pulp-os-device-authorization-with-embedded-tiny-idp-go-demo-and-websocket-sensor-plot/index.md — Ticket validation and navigation entry


## 2026-07-23

Uploaded the index, 1,208-line implementation guide, and investigation diary as one reMarkable bundle: /ai/2026/07/23/ESP-54-PULP-DEVICE-AUTH/ESP-54 PULP Device Auth and Sensor Stream Guide.pdf

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/23/ESP-54-PULP-DEVICE-AUTH--pulp-os-device-authorization-with-embedded-tiny-idp-go-demo-and-websocket-sensor-plot/design-doc/01-device-authorization-and-realtime-demo-analysis-design-and-intern-implementation-guide.md — Uploaded primary guide


## 2026-07-23

Implemented and hardware-proved embedded tiny-idp service, native device auth, bearer-confined REST/WSS, QR-assisted SENSOR LINK UI, and PSRAM-backed TLS; server commit c6f742b, firmware commit 4c2364c. Final fault/sleep/soak acceptance remains open.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/net_auth.cpp — Native device authorization implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/tools/js/apps/pulp.js — QR-assisted SENSOR LINK and live plot

## 2026-07-23

Added hardware probes 20-23 and fixed a probe-discovered pre-Wi-Fi lwIP Invalid mbox crash; verified authorized-session preservation, protected REST 200, authenticated WSS samples, zero drops, and zero JS exceptions (commit e97c589).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/js_probes.cpp — Live auth, REST, WSS, and QR acceptance probes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/net_auth.cpp — Safe Wi-Fi lifecycle guard before auth HTTP

## 2026-07-23

Hardware acceptance exercised denial, bounded ring wrap, deep sleep quiesce/wake, server and Wi-Fi reconnect, a timed 30-minute WSS soak, and natural token expiry. Token cleared and socket stopped on expiry; malformed payload and full SENSOR LINK panel soak remain.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/net_auth.cpp — Denied and expired token lifecycle evidence
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/net_socket.cpp — Reconnect, ring wrap, soak, and expiry shutdown evidence

## 2026-07-23

Completed malformed auth/WS parser probes, fixed malformed sample containment, and passed the final 30-minute SENSOR LINK panel soak: 881 presents, 3,778 WSS messages, stable heap, 101,580 internal events with zero queue drops, and zero JS exceptions (commits 2dd2356, de57051, 10f4864).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/net_auth.cpp — Production-path malformed OAuth response probe
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/net_socket.cpp — Fragmentation, oversize, discontinuity, binary, and ring probes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/tools/js/apps/pulp.js — Malformed sample containment and final UI soak


## 2026-07-23

Ticket closed

## 2026-07-24

Added a 6,200-word tiny-idp/ESP32 integration friction and maintainer improvement guide with public API proposals, LAN TLS tooling, browser test automation, documentation redesign, intern phases, decision records, and validation strategy.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/23/ESP-54-PULP-DEVICE-AUTH--pulp-os-device-authorization-with-embedded-tiny-idp-go-demo-and-websocket-sensor-plot/design-doc/02-tiny-idp-and-esp32-integration-friction-analysis-and-maintainer-improvement-guide.md — Postmortem and upstream improvement roadmap


## 2026-07-24

Validated frontmatter and ticket hygiene, completed reMarkable dry-run, and uploaded ESP-54 Tiny-IDP ESP32 Integration Improvement Guide.pdf to /ai/2026/07/24/ESP-54-PULP-DEVICE-AUTH.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/23/ESP-54-PULP-DEVICE-AUTH--pulp-os-device-authorization-with-embedded-tiny-idp-go-demo-and-websocket-sensor-plot/design-doc/02-tiny-idp-and-esp32-integration-friction-analysis-and-maintainer-improvement-guide.md — Validated and delivered report
