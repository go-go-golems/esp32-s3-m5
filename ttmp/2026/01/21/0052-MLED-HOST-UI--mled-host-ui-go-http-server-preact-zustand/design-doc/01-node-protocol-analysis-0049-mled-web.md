---
Title: Node Protocol Analysis (0049 + mled-web)
Ticket: 0052-MLED-HOST-UI
Status: active
Topics:
    - ui
    - http
    - webserver
    - backend
    - preact
    - zustand
    - rest
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Protocol analysis for implementing a Go host controller compatible with MLED/1: UDP multicast transport, 32-byte little-endian header, message/payload layouts, node/controller behaviors (epoch + show clock + two-phase cues), and interoperability gotchas (TIME_REQ ephemeral source port, u32 wraparound time math)."
LastUpdated: 2026-01-21T18:40:55.134674499-05:00
WhatFor: "Serve as the protocol ground truth for building a Go host HTTP server + UI that discovers and controls MLED nodes locally."
WhenToUse: "Use when implementing the Go host, building packet encoders/decoders, debugging wire captures, or aligning UI operations with protocol semantics."
---

# Node Protocol Analysis (0049 + mled-web)

## Executive Summary

MLED/1 is a **local-network UDP multicast protocol** intended for synchronized multi-node LED control with minimal payload parsing on embedded devices.

The protocol’s “working core” is:
- **Transport**: IPv4 multicast `239.255.32.6:4626`, TTL=1.
- **Framing**: each datagram is `[32-byte header][payload]` (little-endian).
- **Authority + reset**: `epoch_id` (controller session) gates most actions; epoch change clears node cue state.
- **Show clock**: nodes keep `show_ms = local_ms + offset_ms`, where offset is learned from controller `BEACON` and optionally refined by node-initiated `TIME_REQ` / controller `TIME_RESP`.
- **Two-phase cues**:
  - `CUE_PREPARE` (targeted per-node; not time critical): store a `cue_id -> pattern_config`.
  - `CUE_FIRE` (multicast; time critical): “execute `cue_id` at show time T” where T is `header.execute_at_ms`.
- **Discovery/status**: controller multicasts `PING`; nodes reply unicast `PONG` with status + name.
- **Reliability**: UDP is lossy; “critical” multicasts (especially `CUE_FIRE`) are typically sent **multiple times**; `CUE_PREPARE` can request a node `ACK`.

For interoperability, the most authoritative sources are:
- `2026-01-21--mled-web/c/mledc/protocol.h` (wire structs/constants and payload sizes)
- `2026-01-21--mled-web/c/mledc/node.c` (node receive behavior: epoch gating, cue storage, scheduling, ack codes)
- `2026-01-21--mled-web/mled/controller.py` and `mled/protocol.py` (host behavior + Python wire packing)
- `2026-01-21--mled-web/ttmp/.../MO-001-MULTICAST-PY.../sources/local/protocol.md (imported).md` (pattern parameter mapping guidance)

This ticket’s Go host implementation should treat **C wire sizes and semantics** as the protocol truth, and use Python artifacts as a behavioral and UX reference.

## Problem Statement

Ticket `0052-MLED-HOST-UI` needs to implement a Go host controller that:

1) Discovers MLED nodes on the local network (no static IP list).
2) Controls nodes by sending patterns (both “apply now” and “prepare+fire cue”).
3) Maintains a stable show clock model compatible with existing node code.
4) Exposes an HTTP API and a small-footprint web UI that maps user actions to protocol messages safely and predictably.

To do that correctly we need a single, concrete protocol write-up that answers:
- exact byte layouts and payload sizes,
- which messages are epoch-gated vs accepted always,
- how targeting works (and where it is currently ignored),
- how `TIME_REQ/TIME_RESP` actually works on the wire,
- where UDP reliability hazards require controller-side mitigations.

## Proposed Solution

Use the `mled-web` C reference (`mledc/`) as the wire-format ground truth, and document the controller obligations by cross-checking:
- the Python controller behaviors and REST wrapper (`mled/controller.py`, `web/app.py`),
- the protocol review notes in `MO-001-MULTICAST-PY` (esp. pattern parameter mapping in `PatternConfig.data[12]`).

This document captures:
1) Transport constants and socket behavior.
2) Header layout and addressing flags.
3) Payload layouts and sizes for each message type.
4) Behavioral semantics for both node and controller.
5) “Gotchas” and invariants the Go host must preserve.

## Design Decisions

### Ground truth hierarchy

1) **C `protocol.h` sizes + pack/unpack routines are authoritative.**
   - Reason: fixed-size, embedded-friendly, explicit sizes; C node uses these sizes.
2) **C `node.c` behavior defines what nodes *actually do*.**
   - Reason: controller must be compatible with deployed nodes.
3) **Python `mled/protocol.py` is a “second implementation” useful for cross-checking.**
4) **README / high-level docs are informative but not binding.**

### “Apply pattern now” is an API-level operation, not a wire primitive

Existing sources do not define a `PATTERN_APPLY` message.
Instead, existing implementations map “apply now” to:
- `CUE_PREPARE` to each target node, then
- one or more `CUE_FIRE` multicasts scheduled for `show_ms + lead_ms`.

This is the recommended behavior for the Go host UI, because it:
- stays protocol-compatible,
- keeps the synchronized “go” packet small (`CUE_FIRE` is 4-byte payload),
- preserves the show-clock model and avoids reintroducing a large “fire time payload”.

## Alternatives Considered

### Alternative: implement a separate “pattern apply” message
- Pros: conceptually simpler in UI.
- Cons: not in existing protocol sources; would require node firmware changes and version negotiation.

### Alternative: JSON-over-UDP (or HTTP directly to nodes)
- Pros: easier debugging, tooling.
- Cons: violates “embedded fixed-layout minimal parsing” intent; conflicts with existing nodes and the proven two-phase cue approach.

### Alternative: ignore TIME_REQ/TIME_RESP and rely only on BEACON
- Pros: fewer message types.
- Cons: loses the ability to refine offsets and observe timing health; also conflicts with C node which sends TIME_REQ opportunistically on BEACON.

## Implementation Plan

This is a protocol analysis doc; the implementation plan is for consumers (the Go host design doc):

1) Implement Go pack/unpack for:
   - header (32 bytes),
   - `mled_cue_prepare_t` (28),
   - `mled_cue_fire_t` (4),
   - `mled_pong_t` (43),
   - `mled_ack_t` (8),
   - `mled_time_resp_t` (12).
2) Implement UDP socket behavior:
   - bind `:4626`,
   - join multicast group `239.255.32.6`,
   - set multicast TTL to 1,
   - allow selecting outbound interface.
3) Implement controller loops:
   - receive loop parsing datagrams and updating node cache,
   - beacon loop (default ~2Hz),
   - request-timeout tracking for `ACK` correlation.
4) Build HTTP API on top (see design doc).

## Open Questions

1) Should the Go host support `HELLO (0x02)` if nodes start emitting it in the future?
2) Should the Go host treat `target_mode` semantics for `CUE_FIRE/CUE_CANCEL` as “advisory only” since the C node currently doesn’t filter them?
3) Do we want controller-side deduplication and/or ack aggregation surfaced in the UI (e.g. “3/5 nodes acked”)?
4) Do we need multi-controller coexistence rules, or is “single controller per LAN” assumed (epoch is the deconfliction mechanism)?

## References

### Primary sources (wire format + behavior)
- `2026-01-21--mled-web/c/mledc/protocol.h`
- `2026-01-21--mled-web/c/mledc/protocol.c`
- `2026-01-21--mled-web/c/mledc/node.c`
- `2026-01-21--mled-web/c/mledc/time.c`
- `2026-01-21--mled-web/c/mledc/transport_udp.c`

### Secondary sources (host behavior + UX mapping)
- `2026-01-21--mled-web/mled/protocol.py`
- `2026-01-21--mled-web/mled/controller.py`
- `2026-01-21--mled-web/web/app.py`

### Protocol review notes (pattern parameter mapping)
- `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/design-doc/01-python-app-protocol-review.md`
- `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/sources/local/protocol.md (imported).md`

---

## UI Screens (Verbatim)

Even though this document focuses on protocol, ticket `0052-MLED-HOST-UI` treats the imported UI screenshots as the canonical operator experience target. The ASCII screenshots below are copied **verbatim** from:
`esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/sources/local/fw-screens.md`.

<!-- BEGIN FW-SCREENS-ASCII (verbatim) -->

## 1. Main View (Nodes + Quick Control)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller                                              │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  NODES                                          [ Select All ]     │
│                                                                    │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ ☑  🟢 pedestal-01       -42dBm     🎨 Rainbow              │    │
│  │ ☑  🟢 pedestal-02       -38dBm     🎨 Rainbow              │    │
│  │ ☑  🟢 wall-left         -45dBm     🎨 Rainbow              │    │
│  │ ☐  🟢 wall-right        -51dBm     🎨 Solid Blue           │    │
│  │ ☐  🟡 ceiling           -71dBm     🎨 Off          (weak)  │    │
│  │ ☐  🔴 floor-01          ---        offline (3m)            │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                    │
│  5 online · 1 offline · 3 selected                                 │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  QUICK APPLY                                                       │
│                                                                    │
│  [ 🌈 Rainbow ] [ 💙 Calm ] [ 🔥 Warm ] [ ⚡ Pulse ] [ ⬛ Off ]    │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  Brightness: [███████░░░] 70%          [ Apply to Selected ]       │
│                                                                    │
├────────────────────────────────────────────────────────────────────┤
│  [ 🏠 Nodes ]        [ 🎨 Patterns ]        [ ℹ️ Status ]          │
└────────────────────────────────────────────────────────────────────┘
```

---

## 2. Patterns View (Edit & Create Presets)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller                                              │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  PRESETS                                            [ + New ]      │
│                                                                    │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │  🌈 Rainbow          cycle, speed 50                       │    │
│  │  💙 Calm             solid #2244AA, 60%                    │    │
│  │  🔥 Warm             gradient orange→red                   │    │
│  │  ⚡ Pulse            strobe white, 2Hz                     │    │
│  │  ⬛ Off              solid black                           │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  EDIT: Rainbow                                                     │
│                                                                    │
│  Name:     [ Rainbow____________ ]                                 │
│                                                                    │
│  Type:     [ Rainbow Cycle ▼ ]                                     │
│                                                                    │
│  Speed:    [████░░░░░░] 50                                         │
│                                                                    │
│  Brightness: [███████░░░] 70%                                      │
│                                                                    │
│                                                                    │
│  [ 👁️ Preview on Selected ]    [ 💾 Save ]    [ 🗑️ Delete ]        │
│                                                                    │
├────────────────────────────────────────────────────────────────────┤
│  [ 🏠 Nodes ]        [ 🎨 Patterns ]        [ ℹ️ Status ]          │
└────────────────────────────────────────────────────────────────────┘
```

---

## 3. Status View (What's Wrong?)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller                                              │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  SYSTEM STATUS                                      ✅ Healthy     │
│                                                                    │
│  Controller:   192.168.1.112 (eth0)                                │
│  Multicast:    239.255.0.1:5000                                    │
│  Nodes:        5/6 online                                          │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  PROBLEMS                                                          │
│                                                                    │
│  ⚠️  ceiling — weak signal (-71dBm), may drop                      │
│  ❌ floor-01 — offline for 3 minutes                               │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  NODE DETAILS                                                      │
│                                                                    │
│  │ Node          │ Signal │ Uptime  │ Pattern     │ Status   │    │
│  ├───────────────┼────────┼─────────┼─────────────┼──────────┤    │
│  │ pedestal-01   │ -42dBm │ 2h 14m  │ Rainbow     │ 🟢 good  │    │
│  │ pedestal-02   │ -38dBm │ 2h 14m  │ Rainbow     │ 🟢 good  │    │
│  │ wall-left     │ -45dBm │ 1h 58m  │ Rainbow     │ 🟢 good  │    │
│  │ wall-right    │ -51dBm │ 2h 14m  │ Solid Blue  │ 🟢 good  │    │
│  │ ceiling       │ -71dBm │ 0h 42m  │ Off         │ 🟡 weak  │    │
│  │ floor-01      │ ---    │ ---     │ ---         │ 🔴 off   │    │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  [ 🔄 Refresh All ]                    [ ⚙️ Settings ]             │
│                                                                    │
├────────────────────────────────────────────────────────────────────┤
│  [ 🏠 Nodes ]        [ 🎨 Patterns ]        [ ℹ️ Status ]          │
└────────────────────────────────────────────────────────────────────┘
```

---

## 4. Settings (Tucked Away)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller › Settings                      [ ← Back ]   │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  NETWORK                                                           │
│                                                                    │
│  Bind IP:         [ eth0: 192.168.1.112 ▼ ]                        │
│  Multicast:       [ 239.255.0.1 ] : [ 5000 ]                       │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  TIMING                                                            │
│                                                                    │
│  Discovery interval:    [ 1000 ] ms                                │
│  Offline threshold:     [ 30   ] seconds                           │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  PRESETS                                                           │
│                                                                    │
│  [ 📤 Export All ]     [ 📥 Import ]                               │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  [ 💾 Save ]                                                       │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

<!-- END FW-SCREENS-ASCII (verbatim) -->

---

## Appendix A: Transport + socket behavior (what matters for Go)

### Constants (all sources agree)
- Multicast group: `239.255.32.6`
- UDP port: `4626`
- TTL: `1`
- Magic: `"MLED"` (bytes `0..3`), version `1`, header size `32`

### Controller (“host”) socket obligations
- Bind UDP socket to `:4626` so nodes can unicast reply to the controller’s source port.
- Join multicast group so the controller can also receive multicast packets if needed (mainly useful for shared capture/dev); strictly speaking, PONG/ACK/TIME_REQ are unicast to the controller, but joining multicast matches existing Python/C behavior.
- Reply to `TIME_REQ` **to the sender’s source port**:
  - C node sends `TIME_REQ` from an ephemeral socket (`transport_udp.c` creates `time_fd` bound to port 0).
  - Therefore `TIME_RESP` must be sent to `addr.IP:addr.Port` observed on receipt.

### Node socket behavior (relevant to host expectations)
- Nodes bind `:4626` and join multicast.
- Nodes reply unicast:
  - `PONG` to the `PING` sender IP/port.
  - `ACK` to the `CUE_PREPARE` sender IP/port.
  - `TIME_REQ` to the controller (destination port `4626`), but from an ephemeral source port; controller replies `TIME_RESP` to that ephemeral port.

---

## Appendix B: Header layout (32 bytes, little-endian)

```text
0  .. 3   magic           "MLED"
4         version         1
5         type            message type (u8)
6         flags           target mode + ACK_REQ (u8)
7         hdr_len         32
8  .. 11  epoch_id        u32
12 .. 15  msg_id          u32
16 .. 19  sender_id       u32  (controller=0; node=node_id)
20 .. 23  target          u32  (depends on target mode)
24 .. 27  execute_at_ms   u32  (show-time timestamp)
28 .. 29  payload_len     u16
30 .. 31  reserved        u16
```

### Flags
- bits `0..1`: target mode (`ALL=0`, `NODE=1`, `GROUP=2`)
- bit `2` (`0x04`): `ACK_REQ`

---

## Appendix C: Message types and payload layouts

### `BEACON (0x01)` (controller → multicast)
- Payload: none.
- Header fields:
  - `epoch_id`: controller epoch/session id.
  - `execute_at_ms`: controller show time “now”.
- Node behavior (C node):
  - accepted regardless of current epoch,
  - epoch change clears cue store and pending fires,
  - sets coarse show-time offset,
  - triggers `TIME_REQ` opportunistically.

### `PING (0x20)` (controller → multicast)
- Payload: none.
- Node behavior:
  - accepted regardless of epoch,
  - replies unicast `PONG` to sender IP/port, using `hdr.msg_id` for correlation.

### `PONG (0x21)` (node → controller, unicast)
- Payload: 43 bytes.
- Semantics: node status snapshot for UI discovery/monitoring.
- Note: node id is carried in `header.sender_id` (payload contains name, not id).

Payload (C `mled_pong_t`):
```text
u32 uptime_ms
i8  rssi_dbm
u8  state_flags     (bit0 running, bit1 paused, bit2 time_synced)
u8  brightness_pct
u8  pattern_type
u16 frame_ms
u32 active_cue_id
u32 controller_epoch
u32 show_ms_now
char name[16]
```

### `CUE_PREPARE (0x10)` (controller → multicast; targeted to NODE)
- Payload: 28 bytes.
- Header:
  - set `target_mode=NODE`, `target=node_id` for per-node prepare.
  - set `ACK_REQ` if you want an `ACK`.
- Node behavior (C node):
  - ignores packet if `target_mode=NODE` and `target != node_id`,
  - stores or updates cue entry,
  - sends `ACK` if requested with `code`:
    - `0` OK,
    - `1` unpack error,
    - `2` cue table full (no free slot).

### `CUE_FIRE (0x11)` (controller → multicast)
- Payload: 4 bytes (`cue_id`).
- Header:
  - `execute_at_ms` is the show-time to execute.
- Node behavior:
  - epoch-gated,
  - schedules fire; applies when `show_ms` is due (wrap-around-safe).
- Controller reliability rule of thumb:
  - send 2–3 identical `CUE_FIRE` messages (same `cue_id` and `execute_at_ms`) to mitigate UDP loss.

### `CUE_CANCEL (0x12)` (controller → multicast)
- Payload: 4 bytes (`cue_id`).
- Node behavior:
  - epoch-gated,
  - removes cue from cue table and pending fires.

### `TIME_REQ (0x03)` (node → controller, unicast)
- Payload: none.
- Header:
  - `msg_id` is the request id (node-generated).
- Controller behavior:
  - reply with `TIME_RESP` to the sender’s `addr.IP:addr.Port`.

### `TIME_RESP (0x04)` (controller → node, unicast)
- Payload: 12 bytes.

Payload (C `mled_time_resp_t`):
```text
u32 req_msg_id
u32 master_rx_show_ms
u32 master_tx_show_ms
```

Node behavior (C node):
- uses stored `t0_local_ms` for request id and computes new offset using a Cristian-style correction:
  - `rtt = duration(t0, t3)`
  - `srv_proc = duration(master_rx, master_tx)`
  - `effective = max(0, rtt - srv_proc)`
  - `one_way = effective/2`
  - `estimated_master_at_t3 = master_tx + one_way`
  - `offset = diff(estimated_master_at_t3, t3)`

### `ACK (0x22)` (node → controller, unicast)
- Payload: 8 bytes.

Payload (C `mled_ack_t`):
```text
u32 ack_for_msg_id
u16 code
u16 reserved
```
