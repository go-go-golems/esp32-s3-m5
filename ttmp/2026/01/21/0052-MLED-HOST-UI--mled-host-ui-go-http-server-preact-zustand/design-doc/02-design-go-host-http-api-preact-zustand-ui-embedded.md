---
Title: 'Design: Go Host HTTP API + Preact/Zustand UI (Embedded)'
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
Summary: "Design for a single-binary Go host controller: UDP multicast MLED/1 implementation (discovery, time sync, prepare/fire cues, ACK tracking) + HTTP API + embedded Preact/Zustand SPA built with Vite and packaged via go generate + go:embed."
LastUpdated: 2026-01-21T18:40:56.370418272-05:00
WhatFor: "Blueprint for implementing the MLED host controller in Go and serving a small-footprint web UI (Preact + Zustand) from the same binary."
WhenToUse: "Use when implementing the Go service, choosing directory layout and build steps, or reviewing API/UI flows against protocol semantics."
---

# Design: Go Host HTTP API + Preact/Zustand UI (Embedded)

## Executive Summary

Implement a local “controller host” as a single Go binary that:

1) Runs an in-process MLED/1 controller (UDP multicast):
   - `BEACON` loop (epoch + show clock),
   - `PING` discovery and `PONG` node tracking,
   - node-initiated `TIME_REQ` → controller `TIME_RESP`,
   - `CUE_PREPARE`/`CUE_FIRE`/`CUE_CANCEL`,
   - optional `ACK` tracking for prepare reliability.
2) Exposes a stable HTTP API (`/api/...`) for the UI and other tooling.
3) Serves a minimal SPA UI built with **Vite + Preact** and state via **Zustand** (Redux-like ergonomics, smaller bundle).
4) Packages the UI into the binary using the **go-web-frontend-embed** pattern:
   - dev: Vite on `:3000`, Go API on `:3001` (proxy `/api` and events),
   - prod: `go generate` builds UI and copies artifacts into `internal/web/embed/public/`, `go build -tags embed` serves embedded assets.

This design intentionally mirrors the proven flows in `2026-01-21--mled-web/web/app.py` (REST) and the protocol semantics analyzed in `design-doc/01-node-protocol-analysis-0049-mled-web.md`, while reshaping the UI per `sources/local/fw-screens.md` (3-tab “exhibition edition”).

## Problem Statement

We need a “host-side” operator interface for MLED nodes that is:
- **local-first** (works on a LAN without cloud services),
- **fast to operate** (discover nodes, select, quick apply presets),
- **predictable** under UDP loss (retries/acks, clear UI feedback),
- **small-footprint** (preact + zustand; no heavyweight UI frameworks),
- **easy to ship** (one Go binary; UI embedded, no separate deploy step).

The host must also handle network specifics:
- joining multicast on the correct interface,
- sending multicast from the correct outbound interface,
- replying to TIME_REQ from ephemeral node ports,
- maintaining a show clock and epoch consistent with nodes.

## Proposed Solution

### Architecture overview

```text
                    +---------------------------+
                    |       Preact SPA          |
                    |  (Zustand store + UI)     |
                    +-------------+-------------+
                                  |
                                  | HTTP(S) / SSE (same origin)
                                  v
                    +---------------------------+
                    |        Go HTTP API        |
                    |  /api/* + SPA static      |
                    +-------------+-------------+
                                  |
                                  | method calls / channels
                                  v
                    +---------------------------+
                    |       Go MLED Engine      |
                    | UDP multicast + parsing   |
                    | node cache + ack tracker  |
                    +-------------+-------------+
                                  |
                                  | UDP multicast/unicast
                                  v
                    +---------------------------+
                    |         MLED Nodes        |
                    |       (ESP32-C6 etc.)     |
                    +---------------------------+
```

### Key design principle: one source of truth for node state

The Go MLED engine owns:
- sockets and packet IO,
- the node map (last seen + last PONG snapshot),
- controller epoch and show clock,
- “in-flight” operations (prepare ack tracking, last fired cue, etc).

The HTTP layer is a view/controller on that state:
- requests become engine actions,
- responses are snapshots derived from the engine,
- streaming updates (SSE) push incremental changes to the UI.

### Match protocol behavior, not wishful semantics

Per the protocol analysis:
- “apply now” is implemented as `CUE_PREPARE` (per node) + scheduled `CUE_FIRE` (multicast).
- `TIME_REQ` replies must go to the sender’s observed source port.
- show clock comparisons must be wrap-around-safe (`u32` math helpers).

## Design Decisions

### HTTP API: keep it boring, stable, and UI-friendly

- Use a REST-ish API (mirrors `mled-web/web/app.py`) to reduce surprises and allow quick curl/debugging.
- Add a streaming channel for state changes:
  - Prefer **SSE** (`EventSource`) for minimal dependencies and adequate “dashboard” semantics.
  - Keep WebSocket as an optional future enhancement if bidirectional streaming becomes necessary.

### Network interface selection is first-class

Multicast behavior depends heavily on interface choice. The server should:
- default to the OS default route interface,
- expose available interfaces in `/api/settings` (or `/api/net/interfaces`),
- allow the operator to select an interface (matches `fw-screens.md` “Bind IP” setting).

### Small asset size: Preact + Zustand, minimal UI chrome

- Use `preact` with Vite’s preact plugin (or `preact/compat` only if needed).
- Use `zustand` for state (Redux-like but lighter).
- Avoid heavy component libraries (Radix/Tailwind UI kits) unless explicitly requested later.

### Embed strategy: follow go-web-frontend-embed contract

Directory layout (recommended):
- Frontend: `ui/`
- Vite output: `ui/dist/public/`
- Go embed public dir: `internal/web/embed/public/`
- Go web server: `internal/web/` (FS provider + SPA handler + go generate builder).

## Alternatives Considered

### Two-process production (Go API + separate static hosting)
- Rejected: increases operational complexity; violates “single binary” goal.

### Web UI directly in Go templates
- Rejected: larger maintenance burden for interactive UI, harder state management, weaker developer loop.

### React + Redux Toolkit
- Rejected for this ticket: higher baseline bundle size; Preact+Zustand achieves the same UX with less overhead.

### WebSockets for everything
- Deferred: SSE is simpler and sufficient for “push node status updates” and activity logs.

## Implementation Plan

### Phase 0: Ground truth + requirements
1) Treat `design-doc/01-node-protocol-analysis-0049-mled-web.md` as the canonical protocol reference.
2) Treat `sources/local/fw-screens.md` as the target UI surface (Nodes/Patterns/Status + Settings).

### Phase 1: Go protocol + engine packages
1) Create Go module structure (names are suggestions; align with repo conventions when implementing):
   - `internal/mledproto/`:
     - constants, enums,
     - `Header` pack/unpack/validate,
     - payload pack/unpack for `CuePrepare`, `CueFire`, `Pong`, `Ack`, `TimeResp`,
     - u32 show-time helpers (`u32Diff`, `u32Duration`, `isDue`).
   - `internal/mledhost/`:
     - `Engine` (owns UDP conn(s), node map, epoch, show clock),
     - receive loop: parse datagrams, update node map, handle TIME_REQ,
     - send helpers: `SendBeacon`, `SendPing`, `PrepareCue`, `FireCue`, `CancelCue`,
     - ack tracker: correlate `ACK` to controller `msg_id` + node id.
2) Provide a config type:
   - selected interface / bind IP,
   - multicast group/port/TTL,
   - beacon interval, ping interval (optional),
   - offline thresholds, weak RSSI threshold,
   - default lead time for fires, repeat count for fire retransmits.

### Phase 2: HTTP API
1) Implement `net/http` server exposing:
   - `GET /api/status`
   - `GET /api/nodes`
   - `POST /api/nodes/discover`
   - `POST /api/nodes/ping`
   - `POST /api/pattern/send` (maps to prepare+fire)
   - `POST /api/cue/prepare`
   - `POST /api/cue/fire`
   - `POST /api/cue/cancel`
   - `GET/PUT /api/presets` (optional but recommended for the “Patterns” tab)
   - `GET/PUT /api/settings`
   - `GET /api/events` (SSE stream of node updates + logs + ack summaries)
2) Register SPA handler last at `/` (avoid shadowing `/api`).

### Phase 3: UI (Vite + Preact + Zustand)
1) Create `ui/` with:
   - routes/tabs: Nodes, Patterns, Status, Settings (Settings may be a subroute).
   - `zustand` stores:
     - nodes map + selection set,
     - presets,
     - controller status,
     - settings and persistence state.
2) Use:
   - plain `fetch` for request/response endpoints,
   - `EventSource('/api/events')` for live updates.
3) Keep CSS minimal (single stylesheet; no heavy design system).

### Phase 4: Embed + build wiring (go-web-frontend-embed)
1) Add `internal/web/` package:
   - embed FS (prod) + disk FS fallback (dev),
   - SPA handler (serve file if exists, else `index.html` fallback),
   - `go generate` builder to run `pnpm -C ui build` and copy `ui/dist/public` into `internal/web/embed/public`.
2) Add Makefile targets:
   - `dev-backend` (Go `:3001`)
   - `dev-frontend` (Vite `:3000`)
   - `build` (go generate + build with `-tags embed`)

### Phase 5: Validation
1) Protocol smoke:
   - run against the Python simulator (`2026-01-21--mled-web/tests/run_nodes.py`) to verify discovery + prepare/fire.
2) UI smoke:
   - `GET /` returns index.html,
   - `GET /api/status` returns epoch/show time.

## Open Questions

1) Do we need to support multiple concurrent controllers on the same LAN (beyond epoch gating), or is “one controller at a time” acceptable?
2) Should the host send `PING` continuously in the background (like a heartbeat), or only on demand / discover?
3) Do we want the host to originate `TIME_REQ` periodically (in addition to handling node-initiated), or keep time sync node-driven only?
4) How should presets persist (local file in ticket dir vs `$XDG_CONFIG_HOME`) for the intended deployment environment?
5) Should the UI be available to other devices on the LAN (bind `0.0.0.0`) or local-only by default (bind `127.0.0.1`)?

## References

- Protocol analysis: `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/01-node-protocol-analysis-0049-mled-web.md`
- UI requirements: `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/sources/local/fw-screens.md`
- Existing Python host + REST wrapper:
  - `2026-01-21--mled-web/mled/controller.py`
  - `2026-01-21--mled-web/web/app.py`
- C wire source:
  - `2026-01-21--mled-web/c/mledc/protocol.h`

---

## Detailed Design

## 1) Go MLED Engine

### 1.1 Responsibilities
- Own UDP socket bound to `:4626`.
- Join multicast group and set TTL.
- Maintain:
  - `epoch_id` (u32),
  - `msg_id` counter (u32),
  - show clock (`show_ms()`; u32 wrap),
  - node map keyed by `node_id` (u32).
- Receive loop:
  - parse header + payload,
  - handle `PONG` (update node snapshot),
  - handle `ACK` (resolve prepare acknowledgments),
  - handle `TIME_REQ` (reply with `TIME_RESP` to sender source port).
- Send methods used by HTTP layer.

### 1.2 Show clock model

Keep a controller-local monotonic base:
- `bootMono = time.Now()` (monotonic preserved in Go `time.Time`)
- `show_ms = (elapsed_ms + show_offset_ms) & 0xFFFFFFFF`

Where `show_offset_ms` is optional and defaults to 0.

Rationale:
- The controller only needs a stable, monotonic show clock; nodes compute their own offsets based on BEACON/TIME sync.
- A u32 clock matches wire format and avoids accidental widening/narrowing errors.

### 1.3 Packet IO in Go (multicast + unicast)

Recommendation: use `golang.org/x/net/ipv4` for predictable multicast control:
- join group on an explicit interface,
- set outbound interface,
- set TTL.

Core operations:
- `ListenPacket("udp4", ":4626")` → `ipv4.NewPacketConn(conn)`
- `JoinGroup(iface, &net.UDPAddr{IP: groupIP})`
- `SetMulticastTTL(1)`
- `SetMulticastInterface(iface)`

The same socket can:
- send multicast control packets,
- receive unicast `PONG/ACK/TIME_REQ` and (if needed) multicast packets.

### 1.4 Controller loops

Minimum set:
- **recv loop** (goroutine): `ReadFromUDP` and dispatch parse/handle.
- **beacon loop** (goroutine): every `beaconInterval` send `BEACON` with `execute_at_ms = show_ms()`.
- optional **ping loop** (goroutine): every `pingInterval` send `PING` to keep discovery fresh.

All three should be cleanly stoppable via context cancellation.

### 1.5 Node tracking model

```text
NodeRecord:
  node_id (u32)
  ip (string)
  last_seen (time.Time)
  last_pong (parsed PONG)
  derived_status:
    online/offline (by last_seen)
    weak (by RSSI threshold)
  ack_state (optional):
    last_prepare_msg_id
    last_ack_code
    last_ack_at
```

Offline rule (default):
- `offline` if `now - last_seen > offlineThreshold` (e.g. 3s for UI freshness; tuneable in Settings).

### 1.6 “Apply preset” semantics

UI wants an “Apply to selected” button. Implement it as:
1) Choose a `cue_id` (from request or allocate random/monotonic u32).
2) For each selected node:
   - send `CUE_PREPARE` (target_mode=NODE, target=node_id), set `ACK_REQ=true`.
3) Sleep a short “prep settle” window (e.g. 50–100ms) OR fire immediately if operator wants speed over confirmation.
4) Send `CUE_FIRE` multicast with:
   - payload `cue_id`,
   - `execute_at_ms = show_ms() + lead_ms` (default lead ~50ms),
   - repeat send `repeat` times (default 3) spaced by ~1ms.

This matches existing `mled-web/web/app.py` behavior and the intended two-phase protocol design.

---

## 2) HTTP API (suggested schema)

### 2.1 `GET /api/status`
Response:
```json
{
  "epochId": 1234567890,
  "showMs": 424242,
  "beaconIntervalMs": 500,
  "version": "dev",
  "uptimeMs": 12345
}
```

### 2.2 `POST /api/nodes/discover`
Request:
```json
{ "timeoutMs": 500 }
```
Behavior:
- send `PING`,
- collect `PONG`s for up to `timeoutMs`,
- return snapshot.

### 2.3 `GET /api/nodes?maxAge=5.0`
Response:
```json
{ "nodes": [ /* Node DTOs */ ] }
```

### 2.4 `POST /api/pattern/send`
Request:
```json
{
  "patternType": "RAINBOW",
  "params": { "speed": 15, "saturation": 100, "spreadX10": 25 },
  "brightnessPct": 70,
  "nodeIds": [ 123, 456 ],
  "cueId": 42,
  "leadTimeMs": 50,
  "repeat": 3,
  "ackReq": true
}
```

Response:
```json
{
  "success": true,
  "cueId": 42,
  "executeAtMs": 123456,
  "preparedNodeIds": [ 123, 456 ],
  "ack": { "ok": 2, "err": 0, "pending": 0 }
}
```

### 2.5 `GET /api/events` (SSE)
Events:
- `node`: node snapshot update
- `ack`: ack update (node_id + ack_for_msg_id + code)
- `log`: human-readable activity log line

Rationale:
- UI stays live without polling; asset size stays small.

---

## 3) UI: Preact + Zustand (tabbed “exhibition edition”)

## UI Screens (Verbatim)

The following ASCII screenshots are copied **verbatim** from the imported source:
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

### 3.1 Screens (from `fw-screens.md`)
- **Nodes**: node list with selection, quick preset buttons, brightness slider, “Apply to selected”.
- **Patterns**: preset CRUD (edit/create), parameter sliders.
- **Status**: “what’s broken?” — offline/weak nodes, timing sync indicators, recent acks/errors.
- **Settings**: bind IP/interface, multicast group/port, timing intervals, import/export presets.

### 3.2 Store shape (suggested)

```text
useAppStore:
  status: { epochId, showMs, ... }
  nodes: Map<nodeId, Node>
  selection: Set<nodeId>
  presets: Preset[]
  settings: Settings
  activityLog: ring buffer
```

### 3.3 Live updates
- Subscribe to `/api/events` with `EventSource`.
- On `node` events: upsert node and recompute derived status.
- On `ack` events: update per-node ack status + surface to Status view.

---

## 4) go-web-frontend-embed contract (implementation notes)

When implementing (not in this doc):
- Vite dev server:
  - runs `:3000`
  - proxies `/api` and `/api/events` to Go `:3001`
- Go server:
  - mounts `/api` first,
  - mounts SPA handler last at `/` with index fallback.

Production build:
- `go generate ./internal/web` runs `pnpm -C ui build` and copies `ui/dist/public` to `internal/web/embed/public`.
- `go build -tags embed ./cmd/mled-host` produces one binary serving API + UI.
