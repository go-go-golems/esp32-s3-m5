---
Title: 'Phase 2 Design: Encoder position + click over WebSocket'
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: Reference WS handler + async broadcast helpers for small text frames
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: Reference WS broadcasting patterns and backpressure handling
    - Path: ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/playbook/01-websocket-over-wi-fi-esp-idf-playbook.md
      Note: Step-by-step WS debugging and reliability playbook
    - Path: ttmp/2026/01/20/MO-036-CHAIN-ENCODER-LVGL--cardputer-lvgl-list-chain-encoder-navigation/design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md
      Note: Encoder protocol + LVGL integration prior art
ExternalSources: []
Summary: 'Design for Phase 2: read encoder position + click on the device and stream updates to the browser UI over esp_http_server WebSocket, with a minimal JSON message schema and backpressure-safe broadcasting.'
LastUpdated: 2026-01-20T23:06:08.017231774-05:00
WhatFor: Blueprint for adding a realtime WS channel to the Phase 1 Web IDE UI so the browser can display encoder telemetry (position + click) without polling.
WhenToUse: Use when implementing Phase 2 or reviewing how encoder input should be represented and transported over WebSocket.
---



# Phase 2 Design: Encoder position + click over WebSocket

## Executive Summary

Phase 2 adds one new capability to the Phase 1 system:

- the device streams **encoder position** and **click events** to the browser in realtime

The motivation is simple: HTTP request/response is a fine tool for “run this code”, but it is a poor tool for “show me a changing signal”. A WebSocket is the natural transport for low-latency telemetry on a device-hosted UI.

This design:

- defines a minimal JSON message schema (`type: "encoder"`, `pos`, `delta`, `pressed`, `seq`, `ts_ms`)
- specifies a firmware-side “read → coalesce → broadcast” pipeline that is backpressure-safe
- specifies a frontend-side “connect → parse → store → render” pipeline using Zustand

The design is grounded in repo prior art:

- ESP-IDF WebSocket playbook and broadcaster patterns:
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c` (`hub_http_events_broadcast_pb`, `events_ws_handler`)
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` (`http_server_ws_broadcast_text`, `ws_handler`)
- Encoder protocol grounding (if using the M5 Chain Encoder):
  - `esp32-s3-m5/ttmp/.../MO-036-CHAIN-ENCODER-LVGL.../design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md`
  - `M5Chain-Series-Internal-FW/Chain-Encder/protocol/M5Stack-Chain-Encoder-Protocol-V1-EN.pdf`

## Problem Statement

We want the browser UI to display:

- current encoder position (or “count”)
- whether the encoder is pressed (clicked)
- optionally: recent deltas/click transitions for debugging

Constraints:

- encoder events can be bursty (fast spinning)
- Wi‑Fi and WebSocket bandwidth is finite
- ESP-IDF `esp_http_server` is a shared resource; we must not block the server task with long-running operations

If we naïvely send a message for every encoder tick:

- the device may spend more time formatting JSON and sending frames than doing useful work
- clients may fall behind and accumulate queued frames

Therefore the design must specify:

- what the canonical “encoder state” is,
- how events are coalesced,
- what the browser can assume about ordering and freshness.

## Proposed Solution

### 1) System model: “state + deltas”

An encoder can be described by:

- a **position** `pos` (an integer counter, typically monotonic under rotation)
- a **button state** `pressed` (boolean)
- optionally a **delta** `delta` (change since last report)

We want the browser display to be correct even if it misses individual ticks. Therefore we treat:

- `pos` and `pressed` as authoritative state,
- `delta` as diagnostic / convenience.

### 2) Message schema (WebSocket text frames)

We use JSON text frames because:

- they are human-debuggable (browser console, `wscat`, etc.)
- payloads are tiny (tens of bytes)
- they integrate naturally with Zustand stores

Message:

```json
{
  "type": "encoder",
  "seq": 123,
  "ts_ms": 456789,
  "pos": 42,
  "delta": 1,
  "pressed": false
}
```

Definitions:

- `seq`: monotonically increasing message sequence number (device-local)
- `ts_ms`: device-local timestamp (monotonic milliseconds since boot)
- `pos`: encoder position (device-defined units; typically “ticks”)
- `delta`: change since the last message (`pos - last_pos`), can be 0
- `pressed`: current button state

Extensibility rule:

- every message has a `type` field so future messages can share one WS endpoint:
  - e.g. `type: "log"`, `type: "status"`, `type: "js"` (future)

### 3) WebSocket endpoint choice

We use:

- `GET /ws` upgraded to WebSocket

Rationale:

- consistent with existing examples in this repo
- a single endpoint reduces route and handler complexity

### 4) Firmware architecture (read → coalesce → broadcast)

The device has three conceptual stages:

1) **Encoder driver** produces events or state updates.
2) **Aggregator** maintains the authoritative state (pos/pressed) and coalesces bursts.
3) **WebSocket broadcaster** sends the latest state to connected clients at a bounded rate.

Recommended module split (illustrative names):

```
main/
  encoder/encoder.h,.c              # device-specific driver (GPIO or UART protocol)
  encoder/encoder_telemetry.h,.c    # state, coalescing, and "latest snapshot"
  http_server.c,.h                  # WS handler and broadcast helper
```

#### Encoder driver: hardware ambiguity handled explicitly

This ticket’s prompt says “encoder”, but does not specify *which* encoder.

Two plausible cases:

- **Built-in rotary encoder** (if the target hardware includes one)
- **External Chain Encoder (U207)** connected over Grove UART (Cardputer-ADV patterns)

We therefore define a minimal driver interface that can be backed by either implementation:

```c
typedef struct {
  int32_t pos;
  bool pressed;
  bool pressed_edge;    // true on transitions (optional)
} encoder_sample_t;

esp_err_t encoder_init(void);
esp_err_t encoder_read(encoder_sample_t* out);   // non-blocking; returns latest sample
```

If using the Chain Encoder, the protocol and framing rules should follow the MO-036 design doc and the protocol PDF; the driver becomes “UART framing + packet decode → pos/pressed”.

#### Aggregation and coalescing

We maintain an authoritative state:

```c
typedef struct {
  uint32_t seq;
  int32_t pos;
  bool pressed;
  int32_t last_sent_pos;
  bool last_sent_pressed;
  uint32_t last_sent_ms;
} encoder_state_t;
```

Coalescing rule (recommended):

- Sample the encoder at a modest rate (e.g. 100 Hz).
- Broadcast at a bounded rate (e.g. 20 Hz), or immediately on button edge.

This yields:

- fast UI updates (50 ms granularity is perceived as realtime)
- bounded network and CPU usage

#### WS broadcast strategy (backpressure safety)

Use a proven “client list + async send” pattern.

Two prior-art options:

- `0017` pattern: per-client payload copy + `httpd_ws_send_data_async` + free callback
  - simplest for small JSON strings
- `0029` pattern: shared buffer with refcount (better for larger payloads/binary)

Phase 2 recommends the `0017` approach because encoder JSON frames are tiny.

### 5) Frontend architecture (connect → parse → store → render)

Zustand store shape:

```ts
type EncoderMsg = { type: 'encoder'; seq: number; ts_ms: number; pos: number; delta: number; pressed: boolean }

type Store = {
  wsConnected: boolean
  encoder: { pos: number; delta: number; pressed: boolean; seq: number; ts_ms: number } | null
  connectWs: () => void
}
```

Parse rule:

- If message `type !== "encoder"`, ignore (or route to other reducers).
- If `seq` is less than current `seq`, ignore (defensive).

UI rendering:

- show `pos`
- show `pressed` (`true/false`)
- optionally show `delta` and `seq` for debugging

### 6) Failure modes and UX

- WebSocket disconnect:
  - UI shows “disconnected” indicator and attempts reconnect with backoff
- No encoder attached / driver init fails:
  - device still serves UI; WS encoder messages absent or include an error field

This mirrors a useful embedded principle:

> **When peripherals fail, keep the web UI reachable.**
> The UI is your debugging tool; do not make it hostage to optional hardware.

## Design Decisions

### Use JSON text frames (not binary) for Phase 2

Decision:

- encoder telemetry frames are JSON text.

Rationale:

- easy to debug end-to-end
- payloads are small enough that efficiency is not the bottleneck

### Coalesce encoder events; broadcast at bounded rate

Decision:

- sample frequently, broadcast at a bounded rate, and always include authoritative `pos`/`pressed`.

Rationale:

- prevents UI lag caused by event storms
- avoids filling WS send queues with stale intermediate states

## Alternatives Considered

### Polling over HTTP (`GET /api/encoder`)

Rejected:

- wastes bandwidth and battery (clients poll even when nothing changes)
- introduces latency at the polling interval
- does not generalize well to multiple realtime signals

### Send every encoder tick as a WS message

Rejected:

- event storms are common with encoders
- coalescing yields a better “human UI” signal while protecting the system

## Implementation Plan

1) Add a WebSocket endpoint (`/ws`) using the `0017` pattern:
   - client list + mutex + `httpd_ws_send_data_async`
2) Implement `encoder_init()` and `encoder_read()` for the chosen hardware:
   - if Chain Encoder: follow MO-036 and the protocol PDF
3) Add an encoder sampling task:
   - updates authoritative state
4) Add a broadcaster task:
   - emits JSON frames at ~20 Hz (and on click edges)
5) Extend the frontend:
   - WS reconnect store (can reuse `0017` patterns)
   - render encoder state in the UI
6) Write a Phase 2 validation playbook:
   - connect WS, rotate, click, confirm updates; test reconnect behavior

## Implementation Retrospective (as of 2026-01-21)

This section records what Phase 2 looks like in the actual codebase today, what diverged from the earlier Phase 2 design assumptions, and what a “better from scratch” Phase 2 design would specify more explicitly.

### What shipped (file + symbol map)

This work landed primarily in commit `a771bb8` (“0048: phase2 ws endpoint + encoder telemetry plumbing”).

Firmware (`esp32-s3-m5/0048-cardputer-js-web/`):

- `main/http_server.cpp`
  - `ws_handler(httpd_req_t*)` — `GET /ws` WebSocket endpoint (upgrade + client tracking).
  - `http_server_ws_broadcast_text(const char*)` — async per-client send (payload copied per client, freed in callback).
  - `http_server_start()` — registers `/ws` and calls `encoder_telemetry_start()` (best-effort).
- `main/encoder_telemetry.cpp`
  - `encoder_telemetry_start()` — starts a background task only if `CONFIG_TUTORIAL_0048_ENCODER_CHAIN_UART=y`.
  - `telemetry_task()` — polls encoder backend, formats JSON, broadcasts over WS on a fixed interval.
- `main/chain_encoder_uart.{h,cpp}`
  - `class ChainEncoderUart` — minimal UART framing driver (adapted from `0047-cardputer-adv-lvgl-chain-encoder-list`).
  - Key symbols: `ChainEncoderUart::init()`, `ChainEncoderUart::task_main()`, `ChainEncoderUart::cmd_get_inc()`.
- `main/Kconfig.projbuild`
  - Phase 2 toggles and parameters:
    - `CONFIG_TUTORIAL_0048_ENCODER_CHAIN_UART`
    - `CONFIG_TUTORIAL_0048_CHAIN_ENCODER_UART_NUM`, `..._TX_GPIO`, `..._RX_GPIO`, `..._POLL_MS`
    - `CONFIG_TUTORIAL_0048_ENCODER_WS_BROADCAST_MS`

Frontend (`esp32-s3-m5/0048-cardputer-js-web/web/`):

- `web/src/ui/store.ts`
  - `connectWs()` — WS connect + reconnect with exponential backoff.
  - `encoder` state slice — stores latest parsed `type:"encoder"` message.
- `web/src/ui/app.tsx`
  - Renders:
    - `WS: connected|disconnected`
    - `enc pos=... delta=... pressed=...` (latest telemetry)

### What actually happens at runtime (as-built)

The system is effectively two loops:

1) The browser opens a WebSocket (`/ws`) and waits for text frames.
2) The device’s telemetry task periodically broadcasts the latest encoder “state” snapshot.

Firmware-side pseudocode (as implemented):

```c
http_server_start():
  httpd_start()
  register GET /ws as websocket (ws_handler)
  encoder_telemetry_start()  // only starts a task if CHAIN_UART enabled

encoder_telemetry_start():
  if (!CONFIG_TUTORIAL_0048_ENCODER_CHAIN_UART) return
  xTaskCreate(telemetry_task)

telemetry_task():
  enc = ChainEncoderUart(cfg_from_kconfig)
  enc.init()
  pos = 0
  seq = 0
  every BROADCAST_MS:
    pos += enc.take_delta()
    pressed = enc.has_click_pending(); enc.clear_click_pending()
    delta = pos - last_sent_pos; last_sent_pos = pos
    msg = {"type":"encoder","seq":seq++,"ts_ms":...,"pos":pos,"delta":delta,"pressed":pressed}
    http_server_ws_broadcast_text(msg)
```

Browser-side pseudocode (as implemented):

```ts
connectWs():
  ws = new WebSocket(`ws://${location.host}/ws`)
  ws.onopen  -> wsConnected=true, reset backoff
  ws.onclose -> wsConnected=false, schedule reconnect with backoff
  ws.onmessage:
    msg = JSON.parse(ev.data)
    if (msg.type === 'encoder') store.encoder = msg
```

### Divergences (design intent vs implementation reality)

#### “pressed” is currently a click pulse, not a stable state

- Design intent: `pressed` as authoritative “button state”.
- Implementation reality: for the Chain Encoder backend, we treat a `click_pending` flag as `pressed=true` for exactly one broadcast interval, then reset it.
  - This is semantically closer to `click=true` (an event) than `pressed=true` (a state).
- Why: the chain encoder’s unsolicited button event is naturally an event stream; modeling it as a stable state requires additional state tracking and/or explicit press/release semantics.

**Design correction:** the schema should separate *state* from *events*, e.g.:

```json
{ "type":"encoder", "pos": 42, "delta": 1, "button": { "pressed": false, "event": "click" } }
```

or simply:

```json
{ "type":"encoder", "pos": 42, "delta": 1, "click": true }
```

#### Broadcast cadence and backpressure semantics are underspecified

- Design intent: “bounded rate, immediate on click edge”.
- Implementation reality: we broadcast at a fixed interval, regardless of whether `delta==0` and regardless of whether any clients are connected.
- Why: it’s simpler to validate WS transport end-to-end and get observability quickly.

**Design correction:** the design should specify when to send:

- send on change (`delta != 0`) or on button events
- optionally send a heartbeat at a slower interval (e.g. every 1–5s) to keep the UI “alive”

#### Encoder hardware is still not fully decided

- Design intent: explicitly called out ambiguity and recommended a driver interface.
- Implementation reality: we implemented **one backend** (Chain Encoder over UART) behind a Kconfig flag, but we have not implemented a “built-in rotary encoder (GPIO/PCNT)” backend yet.

This is the right shape (pluggable backends), but the design should have had a stronger “decision point” that forces choosing which physical encoder this Phase 2 is supposed to represent.

### What I learned (and how the Phase 2 design should have been written)

The Phase 2 design’s “interesting” part is not the JSON schema; it is the boundary discipline:

- The HTTP server owns sockets and WS client tracking.
- The telemetry subsystem owns sampling and coalescing.
- The encoder subsystem owns hardware specifics.

If I were rewriting Phase 2 from scratch, I would make three things explicit up front:

1) **Message semantics:** distinguish `button_state` from `button_event` (don’t overload `pressed`).
2) **Send policy:** specify “send-on-change + optional heartbeat”, with bounded maximum rate.
3) **Backend choice:** define the intended hardware(s) and require a Kconfig selection.

### Revised Phase 2 design (recommended)

#### Revised message schema (state + events)

```json
{
  "type": "encoder",
  "seq": 123,
  "ts_ms": 456789,
  "pos": 42,
  "delta": 1,
  "button": {
    "pressed": false,
    "event": "none"   // "click" | "double_click" | "long_press" | "none"
  }
}
```

#### Revised coalescing + broadcast pseudocode

```c
// Sample at high rate; broadcast at lower rate, but "event edges" trigger immediate send.
sample_task():
  while (true):
    s = encoder_read_sample()              // pos, pressed_state, event(optional)
    update_authoritative_state(s)
    if (s.button_event != NONE):
      notify(broadcast_task, "urgent")
    sleep(sample_period_ms)

broadcast_task():
  while (true):
    wait_until(change_or_timeout_or_urgent)
    if (no_clients_connected) continue
    snap = get_latest_snapshot()
    if (!snap.changed && !snap.urgent && !heartbeat_due) continue
    ws_broadcast_text(encode_json(snap))
```

The key property: the system remains responsive under bursts (fast spinning), but the network traffic remains bounded and meaningful.

## Open Questions

- Which encoder hardware is Phase 2 targeting:
  - built-in rotary encoder (if present), or
  - external Chain Encoder (U207) over Grove UART?
- Should “position” be absolute count or wrap/modulo for UI?
- Should button events be modeled as:
  - current `pressed` state only, or
  - explicit edge events (`click`, `double_click`, `long_press`)?

## References

- Phase 1 design (this ticket):
  - `esp32-s3-m5/ttmp/.../design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`
- WS patterns:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/ttmp/.../0029-HTTP-EVENT-MOCK-ZIGBEE.../playbook/01-websocket-over-wi-fi-esp-idf-playbook.md`
- Encoder grounding (Chain Encoder case):
  - `esp32-s3-m5/ttmp/.../MO-036-CHAIN-ENCODER-LVGL.../design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md`
  - `M5Chain-Series-Internal-FW/Chain-Encder/protocol/M5Stack-Chain-Encoder-Protocol-V1-EN.pdf`
- WebSocket spec:
  - https://www.rfc-editor.org/rfc/rfc6455
