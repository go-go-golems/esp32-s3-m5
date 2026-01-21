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
