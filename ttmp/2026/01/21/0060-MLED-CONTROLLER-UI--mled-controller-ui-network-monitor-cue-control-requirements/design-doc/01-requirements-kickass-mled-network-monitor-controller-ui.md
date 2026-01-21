---
Title: 'Requirements: kickass MLED network monitor + controller UI'
Ticket: 0060-MLED-CONTROLLER-UI
Status: active
Topics:
    - ui
    - websocket
    - http
    - tooling
    - javascript
    - preact
    - zustand
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0049-xiao-esp32c6-mled-node/tools/mled_ping.py
      Note: Current discovery/status output; baseline for UI node list fields
    - Path: esp32-s3-m5/0049-xiao-esp32c6-mled-node/tools/mled_smoke.py
      Note: Current host-side cue workflow and discovery behaviors that the UI should match
    - Path: esp32-s3-m5/components/mled_node/include/mled_protocol.h
      Note: Protocol constants and message framing the controller must implement
    - Path: esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/design-doc/01-esp32-c6-node-firmware-as-a-network-node-protocol-design.md
      Note: Protocol semantics and timing model background for controller UI
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T18:15:29.412796302-05:00
WhatFor: ""
WhenToUse: ""
---


# Requirements: kickass MLED network monitor + controller UI

## Executive Summary

Build a “controller UI” that makes a LAN full of MLED nodes feel like a single instrument:

- **Observe**: discover nodes instantly, understand health at a glance, and drill down to “why is this node late/offline/dim/wrong pattern?” in seconds.
- **Control**: preview and trigger cues with confidence, coordinate show-time scheduling, and manage groups/presets without thinking about packet loss, epoch churn, or multi-NIC multicast weirdness.
- **Debug**: provide first-class visibility into timing, retransmits, ACKs, and clock sync so the protocol is not a black box.

This document specifies requirements for a “kickass” UI, including functional requirements, non-functional requirements, architecture constraints (browser cannot speak UDP), and acceptance criteria. It is written to be implementation-oriented: every requirement implies concrete UI surfaces, APIs, and behaviors.

## Scope

This UI is the operator-facing surface for:
- monitoring all nodes on the show network,
- selecting/arming an epoch and running time sync,
- preparing cues and scheduling fires,
- applying patterns live (per node, group, or all).

It is explicitly a **controller**, not firmware. It runs on a host (laptop/NUC/RPi) that can open UDP sockets on the LAN.

## Non-goals (for v1)

- Building a full DAW-style timeline editor (we want a strong “cue list + scheduler”, not a full production tool).
- Replacing the device’s local console UI (the node still provisions Wi‑Fi via `esp_console`).
- Internet/cloud features (accounts, multi-user cloud sync, remote control across NAT).

## Problem Statement

The MLED system is intentionally “distributed”: a controller sends UDP multicast messages and many nodes respond and act. The system works, but without a purpose-built UI it is hard to:

- know what nodes exist right now (discovery),
- know whether time sync is “good enough” for a show,
- confidently run two-phase cues (`CUE_PREPARE` + `CUE_FIRE`) and verify they took effect,
- diagnose failures when the network is imperfect (packet loss, reordering) or the host is complex (multiple NICs).

An operator needs a UI that:
- makes the network visible and legible,
- makes control safe (previews, arming, abort),
- makes failure modes explain themselves (not “it didn’t work”, but “node 3 did not ACK prepare; PONG shows old epoch; multicast egress wrong NIC”).

The most important hidden constraint: **a pure browser SPA cannot send/receive UDP multicast**, so the UI must be paired with a local controller process (“daemon”) or run as a desktop app with a privileged network bridge.

## Proposed Solution

### Architecture requirement: split UI and transport

**Requirement:** The system must have a “controller backend” that:
- speaks MLED/1 over UDP (multicast + unicast),
- performs discovery/time sync/cue scheduling reliably,
- exposes a local API for the UI (HTTP + WebSocket).

The UI should be a web app (e.g. Preact) that connects to the backend via:
- HTTP for request/response actions (start epoch, prepare cue, fire cue),
- WebSocket for live telemetry (node list, PONGs, clock stats, ACKs, errors).

#### Diagram: controller split

```text
-----------------------------+        UDP multicast/unicast        +----------------------+
  Controller backend (host)  |  <--------------------------------> |   MLED nodes (LAN)   |
  - UDP sockets              |                                      |  ESP32-C6, etc.      |
  - epoch + time sync        |                                      |  PING/PONG, cues     |
  - cue prepare/fire/retry   |                                      +----------------------+
  - metrics/logging          |
  +-----------------------+--+
                          |
                          | HTTP + WS (localhost / LAN, optional auth)
                          v
                +------------------+
                | Operator UI (web)|
                | - network view   |
                | - cue control    |
                | - diagnostics    |
                +------------------+
```

### Primary UI surfaces

The UI is composed of 6 core views that share a single mental model: “nodes exist in an epoch, have a clock, accept prepared cues, and execute fires at show time.”

1) **Network Overview (“Stage”)**
   - A grid/list of nodes with strong visual status cues (online/offline, epoch match, sync quality, current pattern, brightness).
   - Grouping, filtering, and sorting.
   - A persistent banner that shows controller state: selected NIC, epoch ID, show clock, sync status, multicast health.

2) **Node Details (“Inspector”)**
   - Live node telemetry: RSSI, uptime, last PONG time, current pattern/config, active cue, frame_ms.
   - Clock diagnostics: offset estimate, RTT, last beacon/time sync method.
   - Control actions scoped to that node: prepare pattern, fire cue, cancel cue, set brightness, request time sync probe.

3) **Cues (“Cue List + Library”)**
   - A library of named cue presets (pattern config + fades + metadata).
   - A cue list for the current show/epoch, with per-cue state (prepared on which nodes/groups, acked, fired, canceled).

4) **Show Clock (“Transport”)**
   - A clear “show clock” readout and a scheduler that can:
     - fire “now” (ASAP),
     - fire at `T = show_now + lead_ms`,
     - fire at an absolute show time.
   - Safety features: arm/disarm, panic stop (cancel/blackout), and a “dry run” mode that only logs.

5) **Diagnostics (“Truth”)**
   - Packet/ACK statistics, failure explanations, and a timeline of protocol events.
   - A “network interface + multicast routing” panel that directly addresses multi-NIC issues (bind IP, route checks).

6) **Settings (“Machine Room”)**
   - Multicast group/port, TTL, bind IP, retry/backoff policy, time sync knobs, and feature flags.

### Operating model

The UI should feel “musical”: it should always answer three questions instantly:

1) **What nodes are here?**
2) **Are we synced and ready to fire a cue?**
3) **What happened when I pressed the button?**

To achieve this, the backend must continuously run:
- periodic discovery pings (or maintain presence via PONG sampling),
- periodic beacons for time sync,
- unicast probes (TIME_REQ) when needed,
- queueing/retries for prepares and fires.

## Design Decisions

### DD1: UI-first observability (not “just controls”)

Reason: when a distributed system fails, the “control UI” becomes a debugging tool whether you like it or not. If observability is bolted on later, operators lose trust. The UI must make failure reasons legible.

### DD2: Backend owns UDP + reliability policy

Reason: UDP and multicast routing quirks are OS-dependent and not safely handled in a browser. Centralize:
- NIC selection,
- retry policy,
- dedup semantics (where applicable),
- clock estimation.

The UI should be declarative: “prepare cue X for group G” or “fire cue X at T”, and the backend translates that into correct MLED messages, retries, and status updates.

### DD3: Safety as a first-class feature

Reason: “kickass UI” is not just pretty. It must prevent irreversible mistakes:
- arming model (no show actions unless armed),
- clear scope controls (node vs group vs all),
- confirmation for destructive operations (panic blackout),
- audit log of actions (who/when/what).

### DD4: Time is the product

Reason: synchronized cues are the hardest part. The UI must surface:
- estimated clock offset and drift,
- worst-node sync quality,
- lead time vs expected jitter.

## Alternatives Considered

### A1: Pure browser SPA speaking UDP via Web APIs

Rejected: mainstream browsers cannot open UDP sockets or join multicast groups. Any workaround becomes a privileged extension or local helper anyway.

### A2: Device-hosted UI only

Rejected for this problem: device-hosted web UIs are excellent for provisioning and per-device control, but the controller needs to see and coordinate many nodes simultaneously and speak UDP as the “center of time”.

### A3: CLI-only operator surface

Rejected: CLIs are great for engineering validation but do not scale to show operation where the operator needs instant situational awareness and low cognitive load under time pressure.

## Implementation Plan

This is a requirements doc, but it still includes a recommended phased plan so we can ship something useful quickly.

### Phase 0: Backend contract (enables UI)

- Define a stable WebSocket event stream:
  - `node.upsert`, `node.remove`, `node.pong`
  - `controller.state`
  - `cue.prepared`, `cue.acked`, `cue.fired`, `cue.canceled`
  - `net.warning` (e.g., multicast bind mismatch)
- Define HTTP commands:
  - `POST /epoch/start`, `POST /epoch/stop`
  - `POST /cue/prepare`, `POST /cue/fire`, `POST /cue/cancel`
  - `POST /pattern/apply` (immediate “live” apply)
  - `POST /net/bind` (select NIC / bind-ip)

### Phase 1: Network Overview + minimal control

- Build the “Stage” view (node list + filters) and show controller state banner.
- Provide single-node “apply pattern” and “prepare/fire” flows.
- Include a basic “event timeline” (last 200 events).

### Phase 2: Cue library + groups + show transport

- Add cue presets with naming, tags, and parameter editors.
- Add group management (manual tags + auto rules: name prefix, RSSI threshold, “seen recently”).
- Add transport panel: armed/disarmed, lead time, scheduled fires.

### Phase 3: Deep diagnostics + trust features

- Clock diagnostics panel: drift, RTT distribution, “sync grade”.
- Packet health: send/recv counts, drop estimates, retry counts.
- Exportable logs: “save event timeline as JSON”, optional packet capture.

## Open Questions

1) **Dedup semantics:** should the controller intentionally retransmit `CUE_FIRE` (same cue_id/execute_at) for reliability? If yes, should the UI show “fire attempts” vs “fire confirmed”?
2) **Epoch UX:** do we want manual epoch IDs or a “New Show” button that generates and persists an epoch + cue set?
3) **Multi-controller:** do we ever want multiple operator clients? If yes, define locking/ownership and audit.
4) **Security model:** is “LAN-only + optional token” sufficient, or do we need pairing?

## Requirements (Detailed)

### R0: Terminology (UI-visible)

- **Node**: a device that replies to `PING` with `PONG` and accepts cues.
- **Epoch**: a controller-defined show session identifier; nodes gate cue handling on it.
- **Show time**: a u32 millisecond clock derived from node local time + offset.
- **Cue**: a named pattern configuration stored by `CUE_PREPARE` and triggered by `CUE_FIRE`.

### R1: Discovery and node list

R1.1 The UI must discover nodes automatically within 2 seconds on a healthy LAN.
- Acceptance: new node powers on, appears in UI without manual IP entry.

R1.2 The UI must display per-node “last seen” age and highlight stale nodes.
- Categories: `fresh <2s`, `warm <10s`, `stale <30s`, `offline >=30s` (tunable).

R1.3 The UI must support sorting and filtering by:
- name, node_id, RSSI, group tag, epoch match/mismatch, sync grade, pattern, offline/stale.

R1.4 The UI must support manual tagging and group creation:
- create group from selection,
- auto-group by name prefix or regex,
- persist groups across sessions.

### R2: Controller state + network interface clarity

R2.1 The UI must make the “selected NIC / bind IP” explicit at all times.
- If multiple NICs exist, the UI must show the chosen one and warn if it changes.

R2.2 The system must provide an operator-friendly explanation for missing PONGs:
- Example diagnosis: “Unicast ping works, multicast PONGs missing. Likely multicast egress mismatch. Suggested fix: bind to 192.168.1.112.”

R2.3 The UI must allow selecting:
- multicast group/port,
- TTL,
- bind IP (or “auto”).

### R3: Node details and live telemetry

R3.1 Node inspector must show:
- name, node_id, IP:port (from last PONG),
- RSSI, uptime, frame_ms,
- current pattern type + brightness,
- active cue id,
- controller epoch id currently held by node,
- last PONG timestamp.

R3.2 Node inspector must show clock diagnostics:
- estimated offset (ms),
- last sync method (beacon vs time probe),
- last RTT and a small history sparkline (optional).

R3.3 Node inspector must provide “test actions”:
- ping (unicast and/or multicast),
- time probe (send TIME_REQ or cause backend to respond),
- apply pattern live,
- prepare cue,
- fire cue now / at +lead_ms,
- cancel cue.

### R4: Cue authoring and presets

R4.1 The UI must provide a “pattern editor” for each supported pattern type:
- form inputs with validation, sensible defaults, and an immediate preview mode.

R4.2 The UI must allow saving presets:
- name, tags, description,
- JSON export/import for sharing.

R4.3 The UI must show the binary “wire representation” (optional, developer toggle):
- useful for debugging packing/unpacking mismatches.

### R5: Two-phase cue workflow

R5.1 Prepare workflow must be explicit:
- “Prepare cue X for group G” should show progress:
  - `sent` → `acked (n/m)` → `ready`.

R5.2 Fire workflow must be safe:
- default fire uses `T = show_now + lead_ms` (lead_ms configurable, default 200–800ms).
- UI must show countdown and “late node risk” indicator if worst-node sync grade is poor.

R5.3 Cancel workflow must exist:
- cancel cue for scope (node/group/all),
- cancel scheduled fire(s),
- optional “panic stop” that applies blackout/off to all.

### R6: Event timeline and auditability

R6.1 UI must show a chronological event timeline with filters:
- discovery, beacons, time sync, prepare, ack, fire, apply confirmations, warnings/errors.

R6.2 UI must support exporting the timeline for bug reports:
- JSON export (machine-readable),
- optional Markdown summary.

### R7: Diagnostics and “explain failures”

R7.1 The UI must present a “why did it not work?” panel for the last action.
Examples:
- “Cue prepare was sent but node did not ACK: node is offline (last seen 45s).”
- “Fire was sent; node applied late: RTT spike; sync grade degraded; recommend increasing lead_ms.”
- “Post-fire status unknown: multicast PING/PONG not reaching nodes; try bind-ip.”

R7.2 The UI must show packet-level aggregates:
- sent counts by message type,
- received counts by message type,
- ACK success rate and median latency,
- time sync RTT stats.

### R8: Non-functional requirements

R8.1 Responsiveness
- Node list updates should feel “live” (target <200ms from backend event to UI render).

R8.2 Scale
- Must handle at least 128 nodes without UI lag on a modern laptop.

R8.3 Robustness
- UI must tolerate backend restart (auto-reconnect WebSocket, show “reconnecting” state).
- Backend must tolerate node churn (rapid joins/leaves) without memory growth.

R8.4 Operator safety
- Provide an “armed” mode gate; dangerous actions require arming (and optionally a confirmation).

R8.5 Accessibility
- Keyboard navigation for critical flows (select nodes, fire cue).
- Color is not the only indicator (icons/text for status).

### R9: Acceptance test scenarios (must pass)

1) **Discovery**: power-cycle a node; UI shows it within 2s; shows stale when unplugged.
2) **Multi-NIC**: with Wi‑Fi+Ethernet enabled, operator can choose bind IP and get reliable discovery.
3) **Prepare/ACK**: prepare cue to a group; UI shows ack counts; missing ACK clearly attributed.
4) **Scheduled fire**: fire with lead time; UI shows countdown; nodes apply within acceptable jitter; late nodes flagged.
5) **Cancel/Panic**: cancel scheduled cue; panic blackout works and is confirmed.

## References

- MLED node protocol and host tooling: `esp32-s3-m5/0049-xiao-esp32c6-mled-node/tools/`
- Node protocol/network component: `esp32-s3-m5/components/mled_node/`
- Prior device-hosted UI patterns (preact/zustand): `esp32-s3-m5/0048-cardputer-js-web/`, `esp32-s3-m5/0045-xiao-esp32c6-preact-web/`

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
