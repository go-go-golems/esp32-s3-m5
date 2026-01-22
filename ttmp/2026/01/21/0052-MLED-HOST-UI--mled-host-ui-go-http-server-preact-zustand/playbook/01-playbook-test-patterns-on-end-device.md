---
Title: 'Playbook: Test patterns on end device'
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
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/mled-server/internal/commands/apply.go
      Note: CLI apply verb used to test pattern types
    - Path: esp32-s3-m5/mled-server/internal/httpapi/server.go
      Note: Registers /api/events and wires engine events into SSE
    - Path: esp32-s3-m5/mled-server/internal/httpapi/sse.go
      Note: Implements SSE /api/events used by the playbook
    - Path: esp32-s3-m5/mled-server/internal/mledhost/patterns.go
      Note: Server-side mapping from PatternConfig to wire pattern; defines supported types
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T21:03:23.797380832-05:00
WhatFor: Repeatable manual QA procedure to validate that applying patterns through the host server changes the on-device LEDs as expected, and that the server reports state via REST + /api/events.
WhenToUse: Use when bringing up a new node firmware build, validating network/multicast behavior on a new LAN/interface, or confirming pattern parameter mappings (brightness/color/speed) end-to-end.
---


# Playbook: Test patterns on end device

## Purpose

Validate that an actual MLED/1 end device (ESP32 node) responds correctly to pattern application requests issued through the running Go host server:
- REST `POST /api/apply` (via `mled-server apply*` CLI verbs)
- live updates via SSE `GET /api/events`

This is a **human-in-the-loop** validation: the pass/fail criteria includes what you see on the LEDs.

## Environment Assumptions

- You have at least one end device running MLED/1 firmware powered on and connected to the same L2 network as your host.
- The device listens on UDP port `4626` and participates in multicast group `239.255.32.6` (default).
- You can run the Go server and CLI from this repo:
  - repo root: `/home/manuel/workspaces/2025-12-21/echo-base-documentation`
  - server module: `esp32-s3-m5/mled-server`
- For “real device” tests, only **one process** on the host can bind UDP `:4626`.
- You know which interface should be used for multicast (e.g. `en0`, `eth0`) if the OS default route is not correct.

## Commands

### 0) Start the server (recommended: tmux)

Pick an HTTP address; the default is `localhost:8765`, but for manual QA it’s convenient to use `:18765`:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server

# Example: run on localhost:18765 and use real multicast port 4626
go run ./cmd/mled-server serve \
  --http-addr localhost:18765 \
  --data-dir /tmp/mled-server-manual-var \
  --log-level info
```

If your LAN needs an explicit interface:

```bash
go run ./cmd/mled-server serve --iface en0 --http-addr localhost:18765 --data-dir /tmp/mled-server-manual-var
```

### 1) Confirm the server is reachable

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server
SERVER=http://localhost:18765

go run ./cmd/mled-server status --server "$SERVER" --output json
curl -sS "$SERVER/api/status" | jq .
```

### 2) Confirm the node is visible

Wait a few seconds (the server pings periodically), then:

```bash
go run ./cmd/mled-server nodes --server "$SERVER"
go run ./cmd/mled-server nodes --server "$SERVER" --output json | jq .
```

Pick a target `node_id` from the output (example below uses `0123ABCD`).

### 3) Tail SSE events while testing (optional but very useful)

```bash
curl -N "$SERVER/api/events"
```

Expected event types:
- `event: node` (payload is a node snapshot)
- `event: node.offline` (payload includes `node_id`)
- `event: ack` (payload includes `node_id` and `success`)
- `event: log` (payload includes `{ "message": "..." }`)

### 4) Pattern tests (apply to a single node)

Use `--node <hex>` to target one device. End each test by confirming `/api/nodes` shows the expected `current_pattern` fields and the LED output matches.

#### 4.1 Off

```bash
go run ./cmd/mled-server apply --server "$SERVER" --type off --node 0123ABCD
```

Expected:
- LED turns off (or minimum output).
- `nodes` shows `current_pattern.type == "off"` for that node (if node firmware reports it).

#### 4.2 Solid color (red, then green)

```bash
go run ./cmd/mled-server apply --server "$SERVER" --type solid --brightness 30 --param color=#FF0000 --node 0123ABCD
go run ./cmd/mled-server apply --server "$SERVER" --type solid --brightness 30 --param color=#00FF00 --node 0123ABCD
```

Expected:
- LED shows the requested color.
- Brightness changes are visible (30% should be noticeably dimmer than 75–100%).

#### 4.3 Rainbow (slow, then faster)

```bash
go run ./cmd/mled-server apply --server "$SERVER" --type rainbow --brightness 70 --param speed=10 --node 0123ABCD
go run ./cmd/mled-server apply --server "$SERVER" --type rainbow --brightness 70 --param speed=80 --node 0123ABCD
```

Expected:
- “Rainbow” animation is visible.
- `speed=10` is slower than `speed=80`.

#### 4.4 Pulse (white, slow then fast)

```bash
go run ./cmd/mled-server apply --server "$SERVER" --type pulse --brightness 60 --param color=#FFFFFF --param speed=10 --node 0123ABCD
go run ./cmd/mled-server apply --server "$SERVER" --type pulse --brightness 60 --param color=#FFFFFF --param speed=80 --node 0123ABCD
```

Expected:
- Brightness pulses from off/low to the specified max.
- Speed changes are visible.

### 5) Pattern tests (apply to all nodes)

Use `--all` (default for some verbs) only if you intend to affect every device:

```bash
go run ./cmd/mled-server apply-solid --server "$SERVER" --color "#FF00FF" --brightness 30 --all
```

### 6) Negative tests (expected errors)

The current server-side wire mapping does **not** support `gradient`:

```bash
go run ./cmd/mled-server apply --server "$SERVER" --type gradient --node 0123ABCD
```

Expected:
- CLI reports an error (HTTP 400), and no LED change occurs.

```bash
# Final safety step: turn everything off
go run ./cmd/mled-server apply --server "$SERVER" --type off --all
```

## Exit Criteria

- The device visibly changes patterns for each supported type (`off`, `solid`, `rainbow`, `pulse`) with expected parameter effects.
- `GET /api/events` stays connected and streams `event: node` updates while the device is active.
- `POST /api/apply` returns success responses (or “no targets” if you intentionally tested without nodes).
- Server logs show INFO-level entries for apply requests and any acks/offline transitions.

## Notes

- If you see `listen udp4 :4626: bind: address already in use`, another process is holding UDP `:4626` (often an older `mled-server serve` instance). Stop it before continuing.
- For real device tests, do **not** change multicast port away from `4626` unless the firmware is also configured for that port.
- If node discovery is flaky:
  - try `--iface <name>` on `serve`
  - confirm the device and host are on the same VLAN/L2 segment
  - confirm multicast is not filtered by the network.
