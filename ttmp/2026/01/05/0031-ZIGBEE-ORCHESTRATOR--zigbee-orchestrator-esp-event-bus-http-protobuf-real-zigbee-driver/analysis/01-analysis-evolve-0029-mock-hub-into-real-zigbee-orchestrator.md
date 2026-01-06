---
Title: 'Analysis: evolve 0029 mock hub into real Zigbee orchestrator'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0029-mock-zigbee-http-hub/components/hub_proto/CMakeLists.txt
      Note: nanopb component integration + codegen pattern
    - Path: 0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto
      Note: Existing protobuf IDL for hub events; template for Zigbee-backed event IDL
    - Path: 0029-mock-zigbee-http-hub/main/hub_bus.c
      Note: 0029 hub event loop creation + handler wiring
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: HTTP API + protobuf endpoints + WS handler registration
    - Path: 0029-mock-zigbee-http-hub/main/hub_stream.c
      Note: Bus->protobuf envelope->WebSocket bridge
    - Path: 0029-mock-zigbee-http-hub/main/hub_types.h
      Note: Current 0029 event taxonomy + payloads (to map to real Zigbee events)
    - Path: ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.md
      Note: Real Zigbee 2-chip architecture reference (CoreS3 host + H2 NCP; ZNSP over SLIP/UART)
    - Path: ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md
      Note: Background on NCP vs RCP architecture tradeoffs
    - Path: ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/analysis/01-plan-stabilize-0029-mock-hub-using-0030-patterns-console-monitoring-nanopb-protobuf.md
      Note: Prior stabilization plan for 0029; input to 0031 migration plan
    - Path: ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/design-doc/01-mock-zigbee-hub-over-http-event-driven-architecture.md
      Note: Original 0029 design goals + endpoint/event mapping
    - Path: ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/02-pattern-esp-event-centric-architecture-c-and-c.md
      Note: Generic esp_event-centric patterns to reuse
    - Path: ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/design-doc/01-design-protobuf-event-payloads-websocket-bridge-for-0030.md
      Note: protobuf/nanopb payload + WS bridge approach to reuse
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T17:32:01.471470016-05:00
WhatFor: ""
WhenToUse: ""
---

# Analysis: evolve 0029 mock hub into a real Zigbee orchestrator

## Goal

Take the 0029 “mock Zigbee hub” architecture (HTTP API + `esp_event` internal bus + protobuf/WS observability) and produce a new firmware (0031) where the “virtual devices” are replaced by **real Zigbee devices** (join/interview/command/report) while preserving:

- the event-driven module boundaries,
- a simple HTTP control plane,
- and a protobuf-based event stream for observability and UI tooling.

This document is an architecture + migration analysis. It is not an implementation log.

## Inputs (what we’re combining)

### 1) The “bus first” Zigbee orchestrator design (from the prompt)

Key ideas (paraphrased):

- Use `esp_event` as the internal pub/sub “bus” so HTTP, Zigbee, storage, and UI don’t call each other directly.
- Keep handlers serialized (one event loop task) to centralize state mutation.
- Respect Zigbee threading rules by funneling all Zigbee API calls through one controlled execution context:
  - either a dedicated Zigbee worker task that takes the Zigbee lock (Pattern A),
  - or scheduling work into Zigbee context using `esp_zb_scheduler_alarm*` (Pattern B).

### 2) 0029 current “mock hub” implementation (what we can reuse)

Firmware directory: `esp32-s3-m5/0029-mock-zigbee-http-hub/`

Modules (current state):

- `main/hub_bus.c`: dedicated application event loop (`esp_event_loop_create`) + command handlers.
- `main/hub_registry.c`: in-RAM device registry (virtual devices).
- `main/hub_sim.c`: periodic telemetry producer (virtual devices).
- `main/hub_http.c`: HTTP API; currently already supports **protobuf** bodies/replies using nanopb.
- `main/hub_stream.c`: bus → protobuf envelope → WebSocket broadcaster.
- `components/hub_proto/defs/hub_events.proto`: protobuf schema for event envelope + payloads.
- `main/ui/*`: a minimal UI that connects to `/v1/events/ws` and decodes the protobuf stream.

### 3) Real Zigbee bring-up + constraints (001 ticket)

Docs:

- `ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.md`
- `ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md`

Takeaways relevant to 0031:

- The “real Zigbee” path we’ve already documented is **two-chip**:
  - Wi‑Fi host: ESP32‑S3 (CoreS3 / Cardputer-class host)
  - Zigbee stack/radio: ESP32‑H2 (Unit Gateway H2) as **NCP**
  - Host↔NCP over UART using SLIP + ZNSP-like framing
- UART framing + console pin conflicts matter; the earlier work strongly prefers **USB‑Serial/JTAG console** to keep UART pins clean.

## Non-goals (for this ticket analysis)

- Backwards compatibility with JSON HTTP endpoints.
- Defining the TypeScript client types or UI application beyond “it consumes protobuf”.
- A full Zigbee device model (ZCL database, full cluster support). The first version should focus on:
  - join/announce,
  - basic interview,
  - a couple of commands (OnOff / Level),
  - attribute report forwarding.

## Glossary / background (terms referenced in this doc)

### `esp_zb_scheduler_alarm*` (what it is, and why it exists)

This refers to a small family of Zigbee SDK APIs that schedule a callback to run later on the Zigbee stack’s own execution context (i.e., “inside Zigbee”):

- Think: “enqueue this function so it runs on the Zigbee thread/task”.
- It’s used to respect Zigbee thread-safety constraints without taking locks in random tasks.

In the “single-chip Zigbee” architecture (Zigbee stack inside the same firmware), `esp_zb_scheduler_alarm*` is often the cleanest way to bridge from a non-Zigbee context (HTTP task, console task, `esp_event` handler) into the Zigbee context:

```text
HTTP/console/bus handler  →  schedule alarm  →  Zigbee task callback  →  Zigbee API calls
```

For the **host + H2 NCP** architecture (our chosen direction for 0031), the Zigbee stack itself runs on the H2, so these scheduler functions are not typically the mechanism the S3 “host orchestrator” uses. Instead, the host orchestrator sends requests to the NCP and receives notifications back over the host↔NCP protocol.

### “ZCL database” and “full cluster support” (what that means)

Zigbee devices speak ZCL (Zigbee Cluster Library). In practice, “full cluster support” implies:

- You understand endpoints and clusters (e.g., endpoint 1 supports clusters OnOff `0x0006`, LevelControl `0x0008`, etc).
- You can parse/emit ZCL frames for many clusters and commands (not just On/Off/Level).
- You maintain an attribute model: for each device/endpoint/cluster, you track attributes (attribute IDs, data types, last value, timestamps).
- You handle “reporting” configuration, reads, writes, default responses, error cases, manufacturer-specific commands, etc.

A “ZCL database” in our context means a persistent (or at least structured) store of:

- device identity (IEEE/NWK),
- endpoint list and cluster lists,
- discovered attributes and their last known values,
- optionally metadata (data type, min/max reporting, last-seen time).

We are explicitly *not* trying to build a complete ZCL database for MVP 0031. For an MVP orchestrator, we can:

- Track only what we need for UX/API:
  - OnOff state, Level value, and a small set of attribute reports we care about.
- Treat other clusters/attributes as “opaque reported data” forwarded to the event stream for tooling.

## Key design question: where does the Zigbee stack run?

There are two viable “real Zigbee” architectures, and the threading model differs.

### Option 1: Single-chip Zigbee (Zigbee stack inside the same firmware image)

This means building the orchestrator on a Zigbee-capable SoC (e.g., ESP32‑H2) and running Wi‑Fi is not feasible (H2 is not Wi‑Fi), so you’d need a different connectivity story. This option is likely not aligned with “HTTP over Wi‑Fi”.

### Option 2 (recommended): Two-chip host+NCP (Wi‑Fi S3 host, Zigbee stack on H2 NCP)

This matches the “real product direction” already documented in 001:

- Host (S3) runs Wi‑Fi + HTTP + event bus + persistence + event stream.
- H2 runs Zigbee stack as NCP.
- Host sends Zigbee commands over the host↔NCP link; host receives async notifications (device announce, reports, etc).

The rest of this analysis assumes **Option 2**.

## Proposed 0031 architecture (module map)

Reframe 0029 as a “hub shell” with one pluggable module swapped:

```text
                     ┌────────────────────┐
HTTP + console cmds  │  http_api / console│
  (protobuf bodies)  └─────────┬──────────┘
                               │  GW_CMD_* events
                               ▼
                        ┌────────────┐
                        │ esp_event   │  (application bus)
                        │ loop: gw    │
                        └─────┬───────┘
           GW_EVT_* events    │ GW_CMD_* commands
     (announces/reports/etc)  │
                               ▼
                ┌─────────────────────────┐
                │ zigbee_driver (host side)│
                │  - host↔NCP link         │
                │  - interview + commands  │
                └─────────┬───────────────┘
                          │ notifications
                          ▼
                 ┌──────────────────┐
                 │ device_registry  │  (stable IDs + capabilities)
                 │ + persistence    │
                 └──────────────────┘

     Bus tap for tooling:
       - websocket event stream (protobuf)
       - optional console “monitor on|off”
```

What changes vs 0029:

- `hub_sim` is replaced by `zigbee_driver`.
- The “device record” becomes a Zigbee device record (IEEE, NWK, endpoints, clusters).
- “Interview” becomes real: query endpoints/clusters and populate capabilities (in a minimal first pass).

What stays the same:

- `esp_event` is still the internal contract.
- HTTP handlers stay “parse/validate → post command event → reply” (no direct Zigbee calls).
- WebSocket still streams protobuf envelopes, now containing Zigbee-shaped payloads.

## Threading / execution patterns for “bus ↔ Zigbee” (choosing Pattern A vs B)

The prompt describes two patterns. For the two-chip host+NCP architecture, we want:

- determinism (single place where Zigbee host calls happen),
- no hidden cross-task assumptions,
- easy instrumentation (queue depth, drops).

### Pattern A (recommended for 0031): bus handler → Zigbee worker task

Implementation sketch:

- `gw_loop` (event loop task) runs bus handlers and owns all application state (`device_registry`, persistence scheduling, event stream enqueueing).
- A `zigbee_worker` FreeRTOS task receives “Zigbee commands” from a queue.
- The bus handler does *not* call Zigbee APIs directly; it copies a command into the queue.
- The Zigbee worker:
  - serializes all host-side Zigbee calls,
  - optionally holds a Zigbee lock if required by the Zigbee SDK integration used,
  - posts results/notifications back into the bus as `GW_EVT_*`.

Pros:

- Clear single choke point for Zigbee calls.
- No assumption that `esp_event` handler context is “safe for Zigbee”.
- Easily add backpressure (queue full → publish `GW_EVT_OVERLOAD` and return 503/429 upstream).

Cons:

- Two queues exist: the `esp_event` loop queue + the Zigbee worker queue. This is acceptable if both are bounded and monitored.

### Pattern B (good for single-chip Zigbee; probably not primary here): bus handler → schedule into Zigbee context

If the Zigbee stack ran inside the same image, the most “Zigbee-native” approach is:

- Convert `GW_CMD_*` into a scheduled Zigbee callback using `esp_zb_scheduler_alarm*`,
- Execute Zigbee APIs from that callback.

For host+NCP, we still can implement something “alarm-like”, but it will likely collapse back into “a worker that owns the host↔NCP interaction”, so Pattern A remains the simplest and least surprising.

## Event taxonomy: mapping 0029’s `HUB_*` to 0031’s `GW_*`

0029 currently uses:

- Commands: `HUB_CMD_DEVICE_ADD`, `HUB_CMD_DEVICE_SET`, `HUB_CMD_DEVICE_INTERVIEW`, `HUB_CMD_SCENE_TRIGGER`
- Notifications: `HUB_EVT_DEVICE_ADDED`, `HUB_EVT_DEVICE_INTERVIEWED`, `HUB_EVT_DEVICE_STATE`, `HUB_EVT_DEVICE_REPORT`

0031 should become “real Zigbee shaped”:

### Proposed event base

- `GW_EVT` as the application event base (`ESP_EVENT_DEFINE_BASE(GW_EVT)`).

### Proposed command IDs (examples)

- `GW_CMD_PERMIT_JOIN`
- `GW_CMD_DEVICE_INTERVIEW`
- `GW_CMD_ZCL_CMD_DEVICE` (unicast)
- `GW_CMD_ZCL_CMD_GROUP` (groupcast)
- `GW_CMD_READ_ATTR`
- `GW_CMD_SCENE_TRIGGER` (implemented as a macro issuing ZCL commands)

### Proposed notification IDs (examples)

- `GW_EVT_ZB_DEVICE_ANNOUNCE`
- `GW_EVT_ZB_DEVICE_INTERVIEW_PROGRESS` / `GW_EVT_ZB_DEVICE_INTERVIEWED`
- `GW_EVT_ZB_ATTR_REPORT` (ZCL report)
- `GW_EVT_ZB_LINK_STATUS` (joined network, permit join state, etc)
- Optional: `GW_EVT_CMD_RESULT` (async result correlation for HTTP 202 style)

### Payload design rules (carry over from 0029)

- Prefer small, copyable structs (event payload is copied by `esp_event_post_to()`).
- Add a request/operation correlation field (`req_id`/`op_id`) so HTTP can be async.
- Avoid variable-length heap allocation in event payloads; for ZCL frames use:
  - small fixed byte arrays with length fields, or
  - a pool/ring allocation strategy owned by the zigbee_driver and referenced by opaque IDs.

## Protobuf schema (nanopb) strategy

0029 already has a good embedded-friendly strategy:

- One top-level `HubEvent` envelope with `schema_version`, `id`, `ts_us`, and a `oneof payload`.
- Numeric `EventId` values match the C `enum` in `hub_types.h`.

0031 can keep this pattern but should likely fork it to avoid semantics drift:

- Create a new component, e.g. `components/gw_proto/` with `defs/gw_events.proto`.
- Keep:
  - `schema_version`
  - `EventId` matching C enums
  - `req_id` in command payloads
- Add Zigbee-specific types:
  - `ZigbeeAddr { fixed64 ieee; uint32 nwk; }`
  - `Endpoint { uint32 ep; repeated uint32 in_clusters; repeated uint32 out_clusters; }` (bounded max_count for nanopb)
  - `ZclFrame { uint32 cluster_id; uint32 command_id; bytes payload; }` with bounded max size
  - `AttrReport { ... attr_id, type, bytes value }` with bounded max size

Important: 0031 can remain “embedded only”; TypeScript generation is explicitly out of scope for now.

## HTTP API: evolve 0029 endpoints into Zigbee control endpoints

We want the HTTP shape to still “feel like coordinator orchestration” and to map 1:1 to bus commands.

Recommended minimal endpoints (protobuf bodies):

- `POST /v1/zigbee/permit_join` → `GW_CMD_PERMIT_JOIN`
- `GET /v1/devices` → snapshot from registry (Zigbee devices)
- `GET /v1/devices/{id}` → device details (IEEE/NWK/endpoints/caps)
- `POST /v1/devices/{id}/interview` → `GW_CMD_DEVICE_INTERVIEW`
- `POST /v1/devices/{id}/zcl` → `GW_CMD_ZCL_CMD_DEVICE`
- `POST /v1/groups/{group_id}/zcl` → `GW_CMD_ZCL_CMD_GROUP`
- `GET /v1/events/ws` → protobuf event stream (unchanged conceptually)

Keep the 0029 rule: HTTP handlers do not mutate registry and do not call Zigbee APIs. They only post bus commands and return an immediate reply (200/202) or an error (4xx/5xx).

## Device registry: from “virtual devices” to Zigbee device records

In 0029, `hub_device_t` is an in-memory structure with:

- stable `id`,
- `type`, `caps`,
- state fields.

In 0031, we need additional identity and capability fields:

- Stable internal `device_id` (uint32) to keep HTTP URLs short and stable.
- Zigbee identity:
  - IEEE (`uint64`)
  - NWK short (`uint16`) (changes; cache and update)
- Endpoints (list)
- Cluster capability set (per endpoint)
- Basic state cache (OnOff, Level, etc), driven by attribute reports.

Persistence strategy (first pass):

- Persist a mapping `IEEE → device_id` and the last known “device metadata” so a reboot doesn’t reshuffle IDs.
- Treat NWK short as ephemeral (always re-learn).

## Migration plan: “0029 but real”

The simplest route to a working 0031 is:

1) Copy 0029 firmware into a new project (keep build system + protobuf tooling intact).
2) Remove/disable `hub_sim` (telemetry generator).
3) Add `zigbee_driver` that:
   - owns host↔NCP initialization,
   - implements permit join,
   - listens for device announce and attr report callbacks,
   - posts `GW_EVT_*` into the bus.
4) Expand registry to store Zigbee device identity/capabilities.
5) Incrementally swap HTTP endpoints from “virtual” commands to Zigbee commands, preserving the “post to bus” discipline.
6) Keep WebSocket protobuf stream as the primary observability surface.

## Open questions (need an explicit decision before coding)

These are now **decided**:

1) Hardware target for 0031 host:
   - **Cardputer (ESP32‑S3 host)**
2) Zigbee split:
   - **Commit to host + Unit Gateway H2 NCP immediately** (host speaks host↔NCP protocol; Zigbee stack runs on H2).
3) First supported clusters/flows:
   - **OnOff + LevelControl** (plus basic attribute report forwarding).

4) HTTP contract style (explained below):
   - We will start with **202 Accepted + async results on the WS stream**, because Zigbee operations are naturally asynchronous and may take time (routing, retries, interview, etc).

## HTTP contract options (what the suggestions mean)

In practice, “HTTP contract style” is about whether your HTTP endpoint returns a “final answer” immediately, or returns “accepted; wait for the event bus to report completion”.

### Option A: `202 Accepted` + async results (recommended for Zigbee)

How it works:

- HTTP request posts `GW_CMD_*` to the bus and returns quickly with:
  - `202 Accepted`
  - a `req_id`/`op_id` that identifies the operation
- Later, when the operation actually happens (or fails), the firmware publishes one or more events:
  - e.g. `GW_EVT_CMD_RESULT`, `GW_EVT_ZB_INTERVIEW_PROGRESS`, `GW_EVT_ZB_DEVICE_INTERVIEWED`, etc
- The UI/tooling watches `/v1/events/ws` and correlates events using `req_id`/`op_id`.

Why this matches Zigbee:

- Many Zigbee actions are multi-step and/or time-bounded (permit join window, device interview stages, command retries).
- Treating the bus stream as the “source of truth” avoids lying in HTTP replies.

Minimal example shape:

```text
POST /v1/devices/12/zcl   (body: { req_id=1001, cluster=OnOff, cmd=On })
→ 202 { ok: true, req_id: 1001 }
→ later over WS: GW_EVT_CMD_RESULT { req_id:1001, status:OK }
```

### Option B: `200 OK` “best effort” synchronous reply

How it works:

- HTTP handler posts a bus command and then blocks waiting for a reply queue / condition variable, up to a timeout.
- If the operation finishes before the timeout, return `200 OK` with the result.
- If not, return `504 Gateway Timeout` (or `202 Accepted` anyway).

Why it’s tricky for Zigbee:

- It encourages “do work in the HTTP path”, which pressures you to make deeper layers synchronous.
- Timeouts become normal under RF noise / routing.
- It can make the whole system feel flaky even when it’s functioning correctly.

We can still support it for a few fast operations later (e.g., “read our cached registry”), but for real Zigbee operations we should start async-first.

## Where to start reading (for a new developer)

- 0029’s “hub shell” modules:
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_bus.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_stream.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`
- Real Zigbee host+NCP constraints:
  - `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.md`
  - `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md`
