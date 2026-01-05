# Changelog

## 2026-01-05

- Initial workspace created


## 2026-01-05

Created ticket + drafted design doc for mock Zigbee-style HTTP + esp_event hub

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/design-doc/01-mock-zigbee-hub-over-http-event-driven-architecture.md — Initial architecture + event/API design


## 2026-01-05

Uploaded design doc PDF to reMarkable: ai/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE/design-doc/01-mock-zigbee-hub-over-http-event-driven-architecture.pdf

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/design-doc/01-mock-zigbee-hub-over-http-event-driven-architecture.md — Source markdown uploaded


## 2026-01-05

Implemented initial mock hub firmware (WiFi STA + esp_event bus + HTTP API + WS event stream + device sim)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_bus.c — Command handlers + notifications
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c — HTTP endpoints + /v1/events/ws
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_sim.c — Telemetry reports


## 2026-01-05

Re-uploaded updated design doc PDF to reMarkable (force overwrite)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/design-doc/01-mock-zigbee-hub-over-http-event-driven-architecture.md — Updated source markdown re-uploaded


## 2026-01-05

Add analysis+plan to stabilize 0029 using proven 0030 patterns (console reliability, monitoring queues, WS decoupling, nanopb protobuf boundary)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/analysis/01-plan-stabilize-0029-mock-hub-using-0030-patterns-console-monitoring-nanopb-protobuf.md — Step-by-step plan and cross-file map


## 2026-01-05

Phase 1: disable WS JSON stream and reduce hub logging noise to stabilize console/HTTP (commit 9b177e0)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/README.md — Document WS disabled default and console backend pitfalls
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/Kconfig.projbuild — Add WS JSON toggle (off by default) + quiet-logs option
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_bus.c — Disable inline JSON WS publishing when WS is off
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c — Compile-time gate WS endpoint + JSON broadcast (no-op when disabled)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_console.c — Lower hub log levels after REPL starts
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/sdkconfig.defaults — Default WS off + quiet logs on


## 2026-01-05

Phase 2: add nanopb schema + capture/encode console tooling (hub pb ...) while WS remains disabled (commit a389e64)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/components/hub_proto/CMakeLists.txt — FetchContent nanopb + codegen with CMAKE_BUILD_EARLY_EXPANSION guard
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto — Hub event schema mirrored from hub_types.h
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/app_main.c — Register protobuf capture handler
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_pb.c — Capture last hub event and encode via nanopb
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_console.c — Add hub console command (hub seed


## 2026-01-05

Phase 3: replace JSON WS path with protobuf WS architecture (disabled by default), decouple bus from IO via queue+task, and stabilize HTTP /v1/devices routes (stack + wildcard dispatch) (code 4af0531)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c — HTTP server fixes + protobuf WS broadcast
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_pb.c — Protobuf envelope builder
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_stream.c — Queue+task protobuf WS bridge


## 2026-01-05

Phase 4: enable protobuf WS stream by default; fix WS client lifecycle + / index + scene trigger routing; validate WS stream (commit 13016a2)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/Kconfig.projbuild — enable WS by default
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c — WS handler + broadcaster + / + scenes wildcard dispatch
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_console.c — stream status client count
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/sdkconfig.defaults — default WS enabled

