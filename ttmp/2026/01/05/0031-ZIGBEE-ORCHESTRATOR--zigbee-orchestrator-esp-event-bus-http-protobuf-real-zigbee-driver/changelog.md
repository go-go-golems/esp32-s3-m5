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


## 2026-01-05

Add ticket scripts for tmux monitoring + H2 erase/flash + storage wipe; record environment blockers (DNS/serial) in diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md — Step 17 documents tool/runtime blockers.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/21-h2-erase-zigbee-storage-only.sh — Repro step for channel persistence hypothesis.


## 2026-01-05

Vendor H2 NCP firmware sources under thirdparty/esp-zigbee-sdk and confirm local build (esp32h2).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/thirdparty/esp-zigbee-sdk/README.md — Explains vendored subset and build/flash.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/dependencies.lock — Adjusted to use repo-relative local component path.


## 2026-01-05

Incorporate authorization-timeout diagnosis: add TCLK + link-key exchange + nvram-erase controls (host console + H2 NCP), and update investigation report with Experiment 0.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c — Add zb tclk/lke/nvram commands and status reads.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c — Apply TC preconfigured key + link-key exchange requirement; add nvram-erase-at-start ZNSP handlers.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md — New Experiment 0 based on decoded logs.


## 2026-01-05

Add security-focused debug path for join loop: implement TCLK get/set, link-key exchange toggle, and nvram erase-at-start controls; update investigation report + diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md — Step 19 records new diagnosis + controls.


## 2026-01-05

Add Experiment 0 automation script (TCLK/LKE/permit-join) and host flash helper; document serial permission block in diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/30-experiment0-security.sh — Automates security-first debug sequence.


## 2026-01-05

Hardened monitoring workflow: use 'idf.py monitor --no-reset' (H2 resets can inject ESP-ROM logs onto Grove UART and corrupt SLIP/ZNSP); documented mitigation and checked task.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md — Added warning
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md — Added Step 21
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh — Use --no-reset


## 2026-01-05

Validated Experiment 0 wiring (TCLK set/get + link-key-exchange toggle + permit_join) and added a pyserial capture tool to log H2 output without idf.py monitor TTY/DTR issues.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md — Documented Experiment 0 controls
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md — Added Step 22
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/11-capture-serial.py — New capture tool
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/h2-raw.log — Captured H2 frames
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/host-exp0.log — Captured host transcript


## 2026-01-05

Added repeatable dual host/H2 capture tool for pairing runs; recorded LKE=on pairing evidence showing Device authorized timeout + short address churn; documented permit-join seconds clamp (uint8).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md — Notes on permit-join clamp
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md — Added Step 23 with pairing outcomes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/13-dual-drive-and-capture.py — Dual capture tool
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-05-pairing-lke-on-224457/h2.log — Evidence

