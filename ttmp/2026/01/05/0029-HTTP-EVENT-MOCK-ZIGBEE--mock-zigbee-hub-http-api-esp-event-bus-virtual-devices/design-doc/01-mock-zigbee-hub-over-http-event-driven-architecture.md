---
Title: 'Mock Zigbee hub over HTTP: event-driven architecture'
Ticket: 0029-HTTP-EVENT-MOCK-ZIGBEE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - http
    - zigbee
    - backend
    - websocket
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_event/include/esp_event.h
      Note: esp_event API referenced by this design
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_http_server/include/esp_http_server.h
      Note: HTTP server API referenced by this design
    - Path: 0029-mock-zigbee-http-hub/main/hub_types.h
      Note: Concrete event IDs and payload structs used by the firmware
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T00:09:03.916108809-05:00
WhatFor: Design for a firmware demo that combines esp_http_server + esp_event to simulate a Zigbee-coordinator-style architecture using virtual devices.
WhenToUse: Use when implementing the 0029 mock hub firmware, reviewing the HTTP API shape, or mapping the mock architecture to a future real Zigbee driver.
---



# Mock Zigbee hub over HTTP: event-driven architecture

## Executive Summary

Build a “Mock Home Hub” firmware that exposes an HTTP API (and an event stream) and internally routes *everything* through an `esp_event` bus. The firmware manages a set of **virtual devices** (bulbs/plugs/sensors) whose behavior is simulated by timers/tasks, but whose lifecycle/events are intentionally shaped to look like a future Zigbee coordinator architecture:

- “announce/join” → add a device record
- “interview” → discover capabilities (fake clusters)
- “commands” → set on/off/level/etc
- “attribute reports” → periodic telemetry (“reports”)
- “scenes” → macro that issues multiple commands

This lets us perfect event-driven module boundaries, persistence strategy, and observability (SSE/WebSocket) before swapping the simulator for a real Zigbee driver.

## Problem Statement

We want a fun, high-signal way to study **HTTP + `esp_event`** without immediately pulling Zigbee into the build/debug loop.

Requirements:
- HTTP endpoints translate requests into bus commands (no direct cross-module calls from HTTP handlers).
- Internal modules communicate via `esp_event` (command → state change → notifications).
- Multiple “devices” run in parallel and emit periodic telemetry events.
- A client can observe bus traffic live (SSE or WebSocket).
- The design should be “close enough to Zigbee” that replacing device simulation with a Zigbee driver preserves the rest of the architecture.

## Proposed Solution

### High-level architecture

Single firmware with clear modules (these can be folders, components, or namespaces later):

- `bus` — application `esp_event` loop and event taxonomy (`HUB_EVT` + IDs)
- `http_api` — `esp_http_server` handlers that parse JSON and post `HUB_CMD_*`
- `device_registry` — in-memory database of devices + optional NVS persistence (debounced)
- `device_sim` — timers/tasks that emit periodic `HUB_EVT_DEVICE_REPORT` events and respond to commands
- `scene_engine` — stores scenes and executes them by posting bus commands
- `event_stream` — SSE (`GET /v1/events`) or WebSocket to observe bus traffic

Recommended control flow:

```text
HTTP request -> http_api -> HUB_CMD_* event -> registry/sim handlers ->
  state updated -> HUB_EVT_* notifications -> event_stream -> client
```

### Concurrency + event loop model

Use a dedicated *application* event loop (`esp_event_loop_create()`), not the default system loop. Two viable options:

1) **Dedicated event-loop task** (simplest): create loop with `task_name != NULL`, so handlers run on that task.
   - Pro: easy to reason about; HTTP server tasks only enqueue events.
   - Con: must keep handlers short; long handlers stall other events.

2) **Manual dispatch in a “hub task”**: create loop with `task_name = NULL`, and have a hub task call `esp_event_loop_run()`.
   - Pro: can interleave dispatch with periodic jobs; easier to add deterministic scheduling.
   - Con: a bit more plumbing.

For this demo, prefer (1) initially, then optionally add a second loop (telemetry/data-plane) if we want to demonstrate separation under load.

### Event base + IDs (command vs notification)

```c
ESP_EVENT_DECLARE_BASE(HUB_EVT);
ESP_EVENT_DEFINE_BASE(HUB_EVT);

typedef enum {
  // Commands (sources: HTTP, scene engine, test harness)
  HUB_CMD_DEVICE_ADD = 1,         // "announce/join"
  HUB_CMD_DEVICE_REMOVE,
  HUB_CMD_DEVICE_INTERVIEW,       // "discover clusters" (fake)
  HUB_CMD_DEVICE_SET,             // on/off/level/...
  HUB_CMD_SCENE_TRIGGER,

  // Notifications (sources: registry/device_sim)
  HUB_EVT_DEVICE_ADDED,
  HUB_EVT_DEVICE_REMOVED,
  HUB_EVT_DEVICE_INTERVIEWED,
  HUB_EVT_DEVICE_STATE,           // state changed (on/off/level)
  HUB_EVT_DEVICE_REPORT,          // telemetry report (power/temp/etc)
} hub_event_id_t;
```

Payload contracts should be:
- small POD structs (avoid large heap churn)
- versionable (include `schema_version` if needed)
- serializable to JSON for SSE/WS

### Device model (fake “clusters”)

Each device record has:
- `id` (uint32)
- `type` (enum/string: `plug`, `bulb`, `temp_sensor`, …)
- `name`
- `caps` (capabilities that mimic clusters: `onoff`, `level`, `power`, `temperature`, …)
- `state` (fields depend on caps)

Example HTTP-facing JSON:

```json
{
  "id": 12,
  "type": "plug",
  "name": "desk",
  "caps": ["onoff", "power"],
  "state": {"on": true, "power_w": 18.4}
}
```

### HTTP API (minimal, Zigbee-shaped)

Base path: `/v1`.

- `POST /v1/devices` — announce/join (add a virtual device)
  - body: `{"type":"plug","name":"desk","caps":["onoff","power"]}`
- `GET /v1/devices` — list devices (registry snapshot)
- `GET /v1/devices/{id}` — get device by id
- `POST /v1/devices/{id}/set` — command (maps to `HUB_CMD_DEVICE_SET`)
  - body examples:
    - `{"on":true}`
    - `{"level":42}`
- `POST /v1/devices/{id}/interview` — simulate capability discovery (`HUB_CMD_DEVICE_INTERVIEW`)
- `POST /v1/scenes` — create/update a scene (optional for MVP)
- `POST /v1/scenes/{id}/trigger` — triggers a macro (`HUB_CMD_SCENE_TRIGGER`)
- `GET /v1/events` — server-sent events stream (or WebSocket) of bus events

HTTP handler rule: parse → validate → post command event → respond.
No direct mutation of registry from HTTP handlers.

### Event stream: SSE vs WebSocket

Both are fine; the demo should pick one first:

**SSE (`text/event-stream`)**
- Pro: easiest to consume in a browser; simple framing.
- Con: long-lived HTTP connection; careful about thread context and flushing.

**WebSocket**
- Pro: bidirectional (later we can send commands); async send helpers exist.
- Con: more moving parts for the first demo.

Recommendation:
- Start with SSE for “watch the bus” (read-only).
- If we want multi-client + higher throughput, move to WebSocket and maintain per-client ring buffers.

SSE framing example (one event):

```text
event: HUB_EVT_DEVICE_REPORT
data: {"ts_ms":123456,"device_id":12,"power_w":18.4}

```

### Backpressure strategy

We need to demonstrate that event-driven systems must handle slow consumers:
- The internal `esp_event` loop has a finite queue: posting can return `ESP_ERR_TIMEOUT` when full.
- The event stream can be slower than the bus.

Proposed approach:
- Track “drops” counters for:
  - event loop queue drops (post timeout)
  - stream drops (per-client ring buffer overwrite)
- Keep handlers short; avoid doing heavy JSON building inside the event loop if that becomes expensive.
- For the event stream, buffer **formatted JSON strings** (or compact structs) in a bounded ring.

### Persistence

Persist registry to NVS as a “snapshot” blob with a debounce timer:
- On any registry change: mark dirty + start (or reset) a debounce timer (e.g. 500ms).
- When timer fires: serialize devices and write once.

This keeps the demo responsive while still showcasing durability.

## Design Decisions

### Use `esp_event` as the internal module boundary

Rationale:
- Forces a coordinator-like architecture: producers/consumers are decoupled, modules can be swapped.
- Matches Zigbee’s natural shape: join/interview/command/report are events.

### Separate command events from notification events

Rationale:
- Makes causality explicit in logs and in the event stream.
- Helps enforce layering: HTTP should only emit commands; registry/sim emit notifications.

### Prefer an application event loop (not the default loop)

Rationale:
- Keeps demo traffic isolated from system component events.
- Easier to debug and to reason about handler lifetimes.

## Alternatives Considered

### FreeRTOS queues everywhere (no esp_event)

Rejected because the goal is specifically to learn the `esp_event` registration/dispatch model and its operational sharp edges (queue sizing, handler runtime, posting semantics).

### Do “real Zigbee” first

Rejected for MVP because Zigbee integration complicates the loop (toolchain, commissioning, RF issues). The mock version should unblock architecture learning and UI/API iteration.

## Implementation Plan

1. Create the firmware project `0029-mock-zigbee-http-hub/` (headless, HTTP + event bus).
2. Implement `bus`:
   - define `HUB_EVT`, IDs, and payload structs
   - create app event loop and register core handlers
3. Implement `device_registry`:
   - add/list/get/update device records
   - add NVS snapshot persistence with debounce
4. Implement `device_sim`:
   - timer tick that emits `HUB_EVT_DEVICE_REPORT` for each device
   - command handler (`HUB_CMD_DEVICE_SET`) updates registry and emits `HUB_EVT_DEVICE_STATE`
5. Implement `http_api`:
   - endpoints for add/list/set/interview/scene trigger
   - each handler posts `HUB_CMD_*` and returns JSON `{ok:true}` (or error)
6. Implement `event_stream`:
   - SSE `GET /v1/events` with bounded buffering and drop stats
7. Add a minimal web page / curl examples for quick interaction.

## Open Questions

- Should scenes live in RAM only or also persist to NVS?
- Do we want one event loop or a two-loop “control-plane vs telemetry” demo (to demonstrate isolation under load)?
- SSE vs WebSocket as the default observability mechanism?
- Should the device report generator be `esp_timer`-based (single timer) or per-device tasks (more “parallelism”, more overhead)?

## References

- ESP-IDF `esp_event` API: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_event.html
- ESP-IDF HTTP server (`esp_http_server`): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html
