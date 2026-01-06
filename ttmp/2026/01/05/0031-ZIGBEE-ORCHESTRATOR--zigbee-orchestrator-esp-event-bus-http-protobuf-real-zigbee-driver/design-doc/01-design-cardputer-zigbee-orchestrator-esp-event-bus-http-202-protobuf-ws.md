---
Title: 'Design: Cardputer Zigbee orchestrator (esp_event bus + HTTP 202 + protobuf WS)'
Ticket: 0031-ZIGBEE-ORCHESTRATOR
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp-event
    - http
    - websocket
    - protobuf
    - nanopb
    - architecture
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0029-mock-zigbee-http-hub/components/hub_proto/CMakeLists.txt
      Note: Reference nanopb FetchContent + codegen pattern (ESP-IDF early-expansion guard)
    - Path: 0029-mock-zigbee-http-hub/main/hub_bus.c
      Note: Reference implementation for application esp_event loop creation + handler wiring
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: Reference implementation for HTTP handlers with nanopb protobuf decode/encode + 202-ish workflow
    - Path: 0029-mock-zigbee-http-hub/main/hub_stream.c
      Note: Reference implementation for bus->protobuf->WS broadcasting
    - Path: 0031-zigbee-orchestrator/main/Kconfig.projbuild
      Note: 0031 config knobs (queue size
    - Path: 0031-zigbee-orchestrator/main/app_main.c
      Note: Current bring-up app_main (starts bus + USB-Serial/JTAG esp_console)
    - Path: 0031-zigbee-orchestrator/main/gw_bus.c
      Note: GW esp_event loop + permit_join stub + monitor event tap
    - Path: 0031-zigbee-orchestrator/main/gw_console.c
      Note: USB-Serial/JTAG esp_console REPL startup
    - Path: 0031-zigbee-orchestrator/main/gw_console_cmds.c
      Note: Console commands (version
    - Path: 0031-zigbee-orchestrator/sdkconfig.defaults
      Note: 0031 defaults including USB-Serial/JTAG console
    - Path: ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.md
      Note: Zigbee host+NCP architecture constraints and UART lessons
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md
      Note: Higher-level analysis and decisions that this design doc implements
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T17:59:15.872719463-05:00
WhatFor: ""
WhenToUse: ""
---



# Design: Cardputer Zigbee orchestrator (esp_event bus + HTTP 202 + protobuf WS)

## Executive Summary

Build a Zigbee “orchestrator” firmware on **Cardputer (ESP32‑S3)** that exposes an HTTP API and internally routes all activity through an application `esp_event` bus. The Zigbee stack/radio runs on an external **ESP32‑H2** (Unit Gateway H2) as an **NCP**, and the host orchestrator talks to it over UART using the established host↔NCP protocol documented in ticket `001-ZIGBEE-GATEWAY`.

Key properties:

- **Strict module boundaries via `esp_event`**: HTTP, registry/persistence, Zigbee driver, and event streaming do not call each other directly.
- **Async HTTP contract (Option 202)**: Zigbee actions return `202 Accepted` with a `req_id`; completion/progress is reported later on the event bus and streamed to clients.
- **Protobuf everywhere on-device** (nanopb): HTTP request/response bodies and the WebSocket event stream use protobuf (binary) to avoid JSON overhead and keep schemas explicit.
- **Observability-first**: `/v1/events/ws` streams bus events as protobuf envelopes so browser tooling and scripts can observe the internal state machine.

## Problem Statement

We want to build a gateway-style application that “looks like” a future Zigbee coordinator product while staying debuggable and evolvable:

- Commands come in over HTTP (and/or console).
- Internally, everything is event-driven (join/interview/command/report flows).
- Multiple subsystems must coordinate safely under concurrency:
  - HTTP server runs in its own tasks.
  - Wi‑Fi and system components emit their own events.
  - Zigbee operations are asynchronous and can be delayed/retried.
- We need a clean, versionable data contract for:
  - embedded event payloads,
  - and a live bus stream for UX/tooling.

Constraints:

- Cardputer (ESP32‑S3) is the Wi‑Fi host.
- Zigbee runs on Unit Gateway H2 (ESP32‑H2) as an NCP; host↔NCP link is over UART.
- MVP functional scope:
  - commissioning: permit join,
  - device discovery: device announce + minimal interview,
  - control: OnOff + LevelControl,
  - telemetry: attribute report forwarding.

## Proposed Solution

### System overview (two firmwares)

We build and flash two cooperating firmwares:

1) **Host orchestrator** (Cardputer / ESP32‑S3):
   - Wi‑Fi STA
   - HTTP API (protobuf bodies)
   - WebSocket event stream (protobuf frames)
   - Application bus (`esp_event` loop)
   - Registry + persistence
   - Zigbee host-side driver (NCP client)

2) **Zigbee NCP** (Unit Gateway H2 / ESP32‑H2):
   - Zigbee stack
   - host protocol server (host sends requests; NCP sends responses/notifications)

High-level diagram:

```text
                    ┌──────────────────────────────────────────────────┐
                    │ Cardputer (ESP32‑S3)                              │
HTTP clients        │  - gw_http (protobuf)                             │
  curl/UI/scripts   │  - gw_stream: WS /v1/events/ws (protobuf)         │
   ┌───────────┐    │  - gw_bus: esp_event app loop                     │
   │  HTTP API │◄───┤  - gw_registry + NVS persistence                  │
   └───────────┘    │  - gw_zigbee_driver (host↔NCP)                    │
                    └───────────────┬──────────────────────────────────┘
                                    │ UART (SLIP framed host protocol)
                                    ▼
                    ┌──────────────────────────────────────────────────┐
                    │ Unit Gateway H2 (ESP32‑H2)                        │
                    │  - Zigbee NCP firmware                            │
                    │  - Zigbee stack + ZCL                             │
                    └──────────────────────────────────────────────────┘
```

### Host software architecture (bus-centric)

We use `esp_event` as our internal “bus” and enforce a simple contract:

- Producers (HTTP handlers, console commands, Zigbee driver callbacks) post events.
- Consumers (registry, persistence scheduler, stream) register handlers.
- No direct cross-module calls for “business logic”.

```text
HTTP/console
  parse+validate
      │
      ▼
  GW_CMD_* events  ───►  gw_loop (esp_event task)  ───►  handlers
                                    │
                                    ├── registry: update state
                                    ├── zigbee_driver: translate to NCP operations
                                    └── stream: publish to WS
```

### Module list (host)

**`gw_bus`**
- Owns: `GW_EVT` event base, event ID enum, payload structs, `gw_loop` creation.
- Registers handlers and exposes `esp_event_loop_handle_t gw_bus_loop(void)`.

**`gw_http`**
- Owns: HTTP server initialization + route handlers.
- Does: protobuf decode/encode (nanopb), posts `GW_CMD_*`, returns `202` for Zigbee actions.

**`gw_zigbee_driver`**
- Owns: host↔NCP link lifecycle and a `zigbee_worker` task/queue.
- Does:
  - receives `GW_CMD_*` commands (from bus) and enqueues them to the worker,
  - sends requests to the NCP and parses responses/notifications,
  - posts `GW_EVT_ZB_*` notifications and `GW_EVT_CMD_RESULT` back to the bus.

**`gw_registry`**
- Owns: stable device IDs + Zigbee identity (IEEE/NWK), endpoints, clusters, cached state (OnOff/Level), timestamps.
- Updates only from bus notifications (`GW_EVT_ZB_*`); never directly by HTTP.

**`gw_persist`** (can be part of registry in MVP)
- Owns: NVS persistence of mapping `IEEE → device_id` and last-known metadata.
- Uses debounced snapshot writes.

**`gw_stream`**
- Owns: WebSocket clients list, protobuf envelope encoding, broadcaster.
- Observes bus events and streams them to clients as WS binary frames.

**`gw_console`** (optional)
- Owns: `esp_console` commands for bring-up and debugging.
- Provides `monitor on|off` to print bus events without WS.

### Event taxonomy

We separate command events from notification events:

```text
GW_CMD_*  : intent (from HTTP/console)
GW_EVT_*  : facts (from Zigbee driver + registry changes)
GW_EVT_CMD_RESULT : correlation + status for async HTTP requests
```

Proposed IDs:

```c
ESP_EVENT_DECLARE_BASE(GW_EVT);
ESP_EVENT_DEFINE_BASE(GW_EVT);

typedef enum {
  // Commands
  GW_CMD_PERMIT_JOIN = 1,
  GW_CMD_DEVICE_INTERVIEW,
  GW_CMD_ZCL_CMD_DEVICE,
  GW_CMD_ZCL_CMD_GROUP,

  // Zigbee notifications
  GW_EVT_ZB_LINK_STATUS,
  GW_EVT_ZB_DEVICE_ANNOUNCE,
  GW_EVT_ZB_INTERVIEW_PROGRESS,
  GW_EVT_ZB_DEVICE_INTERVIEWED,
  GW_EVT_ZB_ATTR_REPORT,

  // Results
  GW_EVT_CMD_RESULT,
} gw_event_id_t;
```

Payload rules:

- Add `uint64_t req_id` to every command.
- Keep payloads bounded and copyable (no unbounded pointers or heap ownership).
- For ZCL payloads, treat the payload as opaque bytes for MVP (bounded size).

### Event loop design (`esp_event`)

We use an application event loop (not the default system loop) so we control queue size and task priority:

```c
static esp_event_loop_handle_t s_loop;

void gw_bus_init(void) {
  esp_event_loop_args_t args = {
    .queue_size = 64,
    .task_name = "gw_evt",
    .task_priority = 10,
    .task_stack_size = 6144,
    .task_core_id = 0,
  };
  ESP_ERROR_CHECK(esp_event_loop_create(&args, &s_loop));
}
```

ESP-IDF API references:

- `esp_event_loop_create()`, `esp_event_post_to()`, `esp_event_handler_register_with()`
- Header: `components/esp_event/include/esp_event.h`

### Zigbee integration: host + NCP (Unit Gateway H2)

We commit immediately to:

- **Cardputer host** (ESP32‑S3): runs HTTP/bus/registry/stream.
- **Unit Gateway H2** (ESP32‑H2): runs Zigbee stack as NCP.
- Host↔NCP over UART, using the known-good configuration and lessons in ticket 001.

Important: this is why we are *not* centering the design around `esp_zb_scheduler_alarm*`.

- `esp_zb_scheduler_alarm*` is a Zigbee “schedule into Zigbee task context” API used when the Zigbee stack runs on the same SoC.
- In host+NCP, the Zigbee stack is on the H2; on the S3 we’re interacting with it through the host↔NCP protocol, so our “bridge” is a host driver + worker task.

### Zigbee command execution pattern (worker task)

We keep Zigbee I/O out of the bus handler hot path:

- Bus handler translates `GW_CMD_*` into a `gw_zb_cmd_t` and enqueues it.
- A dedicated `zigbee_worker` task serializes all NCP requests and handles responses.

```text
HTTP handler → post GW_CMD_ZCL_CMD_DEVICE(req_id=123)
   → gw_loop handler → zigbee_queue push
      → zigbee_worker → send request to NCP
         → response/notify from NCP
            → post GW_EVT_CMD_RESULT(req_id=123, status=OK)
            → post GW_EVT_ZB_ATTR_REPORT(...)
```

Pseudocode:

```c
typedef enum {
  ZB_CMD_PERMIT_JOIN,
  ZB_CMD_INTERVIEW,
  ZB_CMD_ZCL_DEVICE,
  ZB_CMD_ZCL_GROUP,
} zb_cmd_kind_t;

typedef struct {
  zb_cmd_kind_t kind;
  uint64_t req_id;
  // union payload…
} gw_zb_cmd_t;

static QueueHandle_t s_zb_q;

static void on_gw_cmd(void *arg, esp_event_base_t base, int32_t id, void *data) {
  gw_zb_cmd_t cmd = translate_bus_cmd(id, data);
  if (!xQueueSend(s_zb_q, &cmd, 0)) {
    // queue full → async failure result (and HTTP handler can choose 503/429)
    gw_evt_cmd_result_t out = {.req_id = cmd.req_id, .status = ESP_ERR_TIMEOUT};
    (void)esp_event_post_to(gw_bus_loop(), GW_EVT, GW_EVT_CMD_RESULT, &out, sizeof(out), 0);
  }
}

static void zigbee_worker(void *arg) {
  gw_zb_cmd_t cmd;
  for (;;) {
    xQueueReceive(s_zb_q, &cmd, portMAX_DELAY);
    // send request to NCP; await response; translate result:
    gw_evt_cmd_result_t out = do_ncp_request(cmd);
    (void)esp_event_post_to(gw_bus_loop(), GW_EVT, GW_EVT_CMD_RESULT, &out, sizeof(out), 0);
  }
}
```

### Registry model (MVP)

We do not build a full “ZCL database” in MVP. We store only what we need to:

- show real Zigbee flows,
- route commands correctly,
- and produce a usable UI and HTTP surface.

Data model:

- Stable `device_id` (uint32) for local API identity.
- Zigbee identity:
  - IEEE (uint64, stable)
  - NWK short (uint16, changes; update on announce)
- Minimal capabilities:
  - endpoints + in/out cluster lists (from interview)
- Cached state:
  - OnOff on/off
  - LevelControl level

Why not full cluster support yet:

- “Full cluster support” implies deep ZCL parsing/encoding + tracking many attributes across many clusters, including persistence, reporting configuration, and device quirks.
- MVP focuses on OnOff/Level + attribute report forwarding; other reports can be forwarded as opaque events for tooling.

### HTTP contract (Option 202) — detailed

We explicitly choose the async contract for Zigbee actions:

- **Action endpoints**: `202 Accepted` with `req_id`.
- **Read endpoints** (registry snapshots): `200 OK` (synchronous) because they are local reads.

Why:

- Zigbee actions have variable latency; synchronous HTTP encourages timeouts and tangled control flow.
- The WebSocket event stream is the “truth” for completion and progress.

#### Option 202 response model

For any action endpoint:

- Request body includes optional `req_id`. If omitted, server assigns one.
- Server returns immediately:
  - status: `202 Accepted`
  - protobuf body: `ReplyAccepted { ok=true, req_id }`
- Later, server emits bus events:
  - progress events (optional)
  - final result event: `GW_EVT_CMD_RESULT { req_id, status, msg }`

Suggested endpoint list (MVP):

- `POST /v1/zigbee/permit_join`
- `POST /v1/devices/{id}/interview`
- `POST /v1/devices/{id}/zcl`

Read endpoints:

- `GET /v1/devices`
- `GET /v1/devices/{id}`

HTTP server API reference:

- `components/esp_http_server/include/esp_http_server.h`

### WebSocket event stream (`/v1/events/ws`)

We stream bus traffic as protobuf envelopes over WS:

- WS binary frames contain `GwEvent` messages.
- Each event includes:
  - `schema_version`
  - `id`
  - `ts_us`
  - a `oneof` payload

We reuse the proven 0029 approach:

- Maintain a bounded list of WS clients.
- Encode one event at a time (avoid building giant buffers).
- Broadcast using `httpd_ws_send_frame_async`.

WebSocket API reference:

- `httpd_ws_send_frame_async()`, `httpd_ws_get_fd_info()`, `httpd_ws_recv_frame()`

### Protobuf + nanopb build system

We follow the “embedded-only nanopb codegen” pattern used in 0029:

- A component (e.g. `components/gw_proto/`) contains `.proto` files under `defs/`.
- CMake uses nanopb’s `nanopb_generate_cpp()` to generate `.pb.c/.pb.h`.
- For ESP-IDF integration, guard `FetchContent` and `find_package(Nanopb)` under `if(NOT CMAKE_BUILD_EARLY_EXPANSION)` to avoid IDF early-expansion issues (see IDF issue `#15693` referenced in prior work).

Reference implementation to copy from:

- `esp32-s3-m5/0029-mock-zigbee-http-hub/components/hub_proto/CMakeLists.txt`

## Design Decisions

### Use `esp_event` as internal bus

Rationale:

- Enforces loose coupling.
- Gives deterministic serialized state mutation in one task.
- Makes it easy to “tap the bus” for WS streaming and for console monitoring.

### Commit to host+H2 NCP immediately

Rationale:

- Cardputer host provides Wi‑Fi; H2 provides 802.15.4/Zigbee.
- Matches the known-good architecture from ticket `001-ZIGBEE-GATEWAY`.

### Async HTTP (`202 Accepted`) for Zigbee actions

Rationale:

- Zigbee operations are inherently asynchronous; event stream is the canonical source of truth.
- Avoids blocking HTTP tasks and avoids misleading “done” replies.

### MVP: OnOff + LevelControl

Rationale:

- High-signal and common: exercises command path + attribute report path + registry.

## Alternatives Considered

### JSON HTTP + JSON WS

Rejected:

- Higher CPU and allocation pressure on embedded.
- Schema drift risk.
- We already have nanopb working patterns to reuse.

### Fully synchronous HTTP actions

Rejected for Zigbee actions:

- Ties RF timing to HTTP timeouts; makes the system appear flaky.
- Encourages blocking “reply queues” everywhere.

### Single-chip Zigbee stack on the host

Rejected:

- Not compatible with “Wi‑Fi + Zigbee” on the same chip using H2 (no Wi‑Fi).

## Implementation Plan

### Phase 0: Fork 0029 into a new 0031 firmware project

- Create `0031-zigbee-orchestrator/` by copying `0029-mock-zigbee-http-hub/`.
- Replace `hub_*` naming with `gw_*`.
- Keep HTTP + WS + nanopb working end-to-end even if the Zigbee driver is stubbed at first.

### Phase 1: Bring up host↔NCP link on Cardputer

- Use the known-good NCP firmware on the H2.
- Validate UART pins/wiring and ensure console is not corrupting the UART link.
- Add a `GW_EVT_ZB_LINK_STATUS` event and show it in WS/console.

### Phase 2: Commissioning + device discovery

- Implement `POST /v1/zigbee/permit_join` → `GW_CMD_PERMIT_JOIN`.
- On device announce, publish `GW_EVT_ZB_DEVICE_ANNOUNCE` and create/update registry entries.

### Phase 3: MVP control path

- Implement `POST /v1/devices/{id}/zcl` for:
  - OnOff commands,
  - LevelControl move-to-level.
- Publish `GW_EVT_CMD_RESULT` for correlation.
- Forward attribute reports (`GW_EVT_ZB_ATTR_REPORT`) and update cached state.

### Phase 4: Interview and richer registry

- Implement `POST /v1/devices/{id}/interview`.
- Store endpoints/clusters and expose them via `/v1/devices` replies.

## Open Questions

- Exact Cardputer↔Unit Gateway H2 wiring/pins and a bring-up playbook specific to this pair.
- Whether we keep the 0029 “minimal UI” and extend it to Zigbee device state, or use scripts first and UI later.
- How much interview depth we need for MVP (endpoints/clusters only vs some attribute reads).

## References (local)

- 0029 (proven HTTP/protobuf/WS/bus patterns):
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_stream.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`
- Real Zigbee host+NCP constraints + UART lessons:
  - `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.md`

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
