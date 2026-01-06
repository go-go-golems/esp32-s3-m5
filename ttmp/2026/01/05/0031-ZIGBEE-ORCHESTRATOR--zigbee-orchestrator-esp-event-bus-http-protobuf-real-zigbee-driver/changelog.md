# Changelog

## 2026-01-05

- Initial workspace created
- Drafted architecture/migration analysis (reuse 0029 shell; integrate real Zigbee via host+H2 NCP)
- Added initial implementation task breakdown
- Decision: 0031 host target is Cardputer (ESP32-S3)
- Decisions: commit to host+H2 NCP immediately; MVP clusters are OnOff + LevelControl; HTTP contract is 202+async results over WS
- Added detailed design doc for the end-to-end architecture (bus + HTTP 202 + nanopb/protobuf WS + registry + NCP integration)
- Expanded implementation task breakdown (bring-up sequence: console → bus → Wi‑Fi → HTTP API → WS → Zigbee → webapp)
- Refined tasks into an exhaustive, phase-ordered checklist (start with USB‑Serial/JTAG esp_console, then bus+commands, then Wi‑Fi, then API, then webapp)
- Implemented Phase 0–2 skeleton firmware (`0031-zigbee-orchestrator/`): USB‑Serial/JTAG esp_console + GW esp_event bus + monitor and demo commands; built/flashed and validated on Cardputer

## 2026-01-05

Backfilled diary (steps 15-16) and added investigation report for device rejoin loop + channel selection stuck on ch13.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md — Summarizes symptoms
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md — Recorded join/rejoin + channel-mask debugging steps.

