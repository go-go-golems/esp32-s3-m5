---
Title: Implementation plan and intern guide
Ticket: ESP-28-M5DIAL-WEB-REMOTE
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - ui
    - websocket
    - webserver
    - http
    - wifi
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/web/src/ui/store.ts
      Note: Browser-side single WebSocket plus local store/reducer pattern useful for the server-hosted React client
    - Path: 0072-m5dial-timer-demo/main/app_main.cpp
      Note: Baseline M5Dial dual-task architecture and queue ownership
    - Path: 0072-m5dial-timer-demo/main/input_events.h
      Note: Compact normalized input event vocabulary already proven on M5Dial
    - Path: 0072-m5dial-timer-demo/main/m5dial_board.cpp
      Note: Working M5Dial hardware abstraction, pins, touch, and input normalization
    - Path: 0073-m5dial-film-developer-timer/main/app_main.cpp
      Note: Evidence that the same M5Dial base can support multiple UI modes and richer app state
    - Path: ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/sources/local/01-esp32-knob-web.md
      Note: Imported source note proposing the external server event-pipeline shape now adopted more directly
ExternalSources:
    - local:esp32-knob-web.md
Summary: Build a self-contained M5Dial firmware project that connects to an external web server, streams normalized dial input upstream, and controls a React widget served by that server.
LastUpdated: 2026-03-11T23:20:00-04:00
WhatFor: Detailed architecture, design rationale, and implementation plan for a M5Dial remote-control firmware that connects to a server-hosted React UI.
WhenToUse: Use when implementing or reviewing the M5Dial firmware client, the dial-to-server protocol, or the server-hosted React integration contract.
---

# Implementation plan and intern guide

## Executive Summary

This ticket now assumes a different deployment model than the first draft. The M5Dial should not host the React app. Instead, the M5Dial firmware should connect over Wi-Fi to a separate web server. That server hosts the React widget, receives dial events from the device, and redistributes those events to browser clients.

That change simplifies the embedded side and aligns more closely with the imported source note. The firmware becomes a network client plus a local status UI. The web server becomes the place that serves React, manages browser sessions, and translates device events into widget updates. The M5Dial remains the physical control surface.

For an intern, the mental model is:

1. The M5Dial samples encoder, button, and touch input.
2. The firmware normalizes those into small JSON events.
3. The firmware sends those events to a remote server.
4. The server serves the React app and pushes the resulting control stream to browser tabs.
5. The browser widget updates in response.

## Problem Statement And Scope

The requested system is a remote control built around the M5Dial that drives a React widget in a web browser. The critical clarification is that the browser app is served by a web server, not by the dial itself. The firmware therefore needs to act as a client to that server.

In scope:

- A new M5Dial firmware project folder
- Working M5Dial hardware bring-up and normalized input events
- Wi-Fi client behavior on the device
- Outbound connection from the device to a web server
- A device-to-server protocol for dial input, status, and heartbeats
- A server-to-browser flow that lets a React widget react to device input
- A local LVGL status screen on the dial
- A detailed, intern-friendly implementation guide

Out of scope for v1:

- Serving React assets from the dial
- A generic event-bus platform with multiple brokers
- OTA update infrastructure
- Multi-device orchestration beyond simple device IDs
- A full server implementation in this repo unless the future implementation chooses to add one

## Current State Analysis

### 1. Existing M5Dial firmware already solves the hardware and task split

`0072-m5dial-timer-demo/main/app_main.cpp:17-103` provides the strongest local precedent for the embedded side. It already separates hardware polling from UI/model ownership using:

- one task for I/O polling
- one task for LVGL and application state
- a FreeRTOS queue carrying `InputEvent` values

That structure should remain. Network client code should be added around it, not used as an excuse to redesign the working embedded runtime.

`0072-m5dial-timer-demo/main/input_events.h:7-27` and `0072-m5dial-timer-demo/main/m5dial_board.cpp:181-351` already prove that the M5Dial can emit a compact normalized event vocabulary:

- encoder delta
- short press
- long press
- swipe

That is exactly the right shape to send upstream to a web server.

### 2. Existing M5Dial projects show that the local UI can stay small and status-oriented

`0073-m5dial-film-developer-timer/main/app_main.cpp:30-179` shows that the M5Dial can hold richer application state and multiple modes while still using the same board abstraction and queue-driven loop.

For this ticket, that means the dial screen should stay focused on:

- connectivity state
- device ID / room / current target
- last input summary
- optional widget feedback from the server

The dial does not need to mirror the entire browser widget visually.

### 3. The imported source note now matches the deployment boundary better

The imported note at `ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/sources/local/01-esp32-knob-web.md:1-27` proposes the core shape:

```text
device ingress -> normalize/project -> one browser WebSocket stream
```

That high-level flow still holds. The difference is that the device should now connect outward to the server rather than behave as the server.

The same note at `.../01-esp32-knob-web.md:29-43` and `:45-246` also recommends semantic topics and normalized `DeviceEvent`, `UIIntent`, and `BrowserEnvelope` structures. Even if the eventual server does not literally use Watermill, those message-shaping ideas are still good design input.

### 4. Existing browser-side code still provides useful client patterns

`0048-cardputer-js-web/web/src/ui/store.ts:47-137` is still relevant, even though the dial is no longer hosting the app. It shows a good browser-side pattern:

- one WebSocket connection
- reconnect with backoff
- parse and store recent events
- keep a small focused store rather than burying protocol handling all over the UI

That pattern should move to the server-hosted React app.

## Gap Analysis

The repo already has:

- working M5Dial hardware code
- a proven input-event abstraction
- examples of browser-side live event handling

The missing piece is a M5Dial firmware project that behaves as a remote network client instead of an HTTP/WebSocket server.

Specific gaps:

1. No existing M5Dial project in this repo opens and maintains an outbound WebSocket session to a server.
2. No current ticket in this repo documents a device-to-server protocol for a dial controller.
3. The earlier draft overfit to device-hosted web UI examples; that deployment shape is now explicitly wrong for this ticket.
4. The browser/server boundary now matters more than the embedded asset pipeline.

## Proposed Architecture

### 1. High-level system model

```text
M5Dial hardware
  |
  v
board abstraction (encoder/button/touch polling)
  |
  v
InputEvent queue
  |
  v
firmware owner task
  |\
  | \-> local LVGL status screen
  |
  +-> protocol encoder
        |
        v
   outbound Wi-Fi client connection
        |
        v
   web server
     |- serves React app
     |- receives device events
     |- fans out browser updates
     \- optionally sends widget feedback back to the dial
        |
        v
   browser React widget
```

Ownership:

- firmware owns physical input, local status, connection lifecycle, and outbound event delivery
- server owns browser session management and event redistribution
- React owns widget rendering and UI semantics

### 2. Connection model

There are two good v1 options:

1. Device opens one outbound WebSocket to the server
2. Device uses HTTP POST for ingress plus a second channel for server-to-device feedback

Recommendation:

- use one outbound WebSocket from device to server for both directions

Reason:

- lower latency
- simpler heartbeat story
- fewer moving parts than mixing POST and push
- maps well to the imported note’s event-stream architecture

Recommended server endpoints:

| Endpoint | Client | Purpose |
| --- | --- | --- |
| `/` | browser | serve the React application |
| `/ws` | browser | browser event stream from server |
| `/device/ws` | M5Dial | device ingress and optional server-to-device feedback |
| `/api/session` | browser | initial session snapshot |
| `/api/devices/:id` | browser/server ops | optional device state lookup |

### 3. Self-contained firmware interpretation

With this clarified architecture, “self-contained within its firmware folder” should be interpreted as:

- the firmware project can be built, flashed, and understood from its own folder
- the device does not compile code from unrelated sibling firmware examples
- protocol docs, local status UI, network client code, and board support all live in that folder

It should not be interpreted as “the dial must also host the web app”. That requirement has now been explicitly superseded.

### 4. Recommended firmware folder layout

```text
0074-m5dial-web-remote/
  CMakeLists.txt
  sdkconfig.defaults
  partitions.csv
  build.sh
  README.md
  main/
    app_main.cpp
    input_events.h
    m5dial_board.h
    m5dial_board.cpp
    lvgl_port_m5dial.h
    lvgl_port_m5dial.cpp
    remote_protocol.h
    remote_protocol.cpp
    remote_model.h
    remote_model.cpp
    remote_controller.h
    remote_controller.cpp
    dial_client.h
    dial_client.cpp
    ui_status_screen.h
    ui_status_screen.cpp
  third_party/
    any vendored local helper code required for the dial build
  docs/
    protocol.md
    server-contract.md
```

Notice what is absent:

- no `web/`
- no `main/assets/`
- no embedded React bundle

## Protocol Design

### 1. Device-to-server messages

The device should send semantic input events, not raw pin changes.

Recommended payloads:

```ts
type DialHello = {
  type: 'hello'
  protocol: 1
  device_id: string
  firmware: string
  board: 'm5dial'
  capabilities: string[]
}

type DialInput = {
  type: 'input'
  seq: number
  ts_ms: number
  source: 'encoder' | 'button' | 'touch'
  action: 'delta' | 'short_press' | 'long_press' | 'swipe'
  delta?: number
  swipe?: 'left' | 'right' | 'up' | 'down'
}

type DialStatus = {
  type: 'status'
  seq: number
  ts_ms: number
  wifi_state: 'connecting' | 'connected' | 'disconnected'
  ip?: string
  server_connected: boolean
}

type DialHeartbeat = {
  type: 'heartbeat'
  seq: number
  ts_ms: number
}
```

### 2. Server-to-device messages

The server does not need to micromanage the device in v1, but a few messages are useful:

```ts
type ServerAck = {
  type: 'ack'
  seq: number
}

type ServerDisplayState = {
  type: 'display_state'
  label: string
  value_text?: string
  mode?: string
  accent?: string
}

type ServerPing = {
  type: 'ping'
  ts_ms: number
}
```

The most useful optional message is `display_state`, because it lets the server tell the dial what the browser widget is currently focused on.

### 3. Browser-facing messages

The browser should not talk directly to the dial. The browser should talk to the server, and the server should fan out normalized events.

A good browser-facing message shape is:

```ts
type BrowserEnvelope = {
  channel: 'dial' | 'system'
  topic: 'dial.input' | 'dial.status' | 'dial.display'
  device_id: string
  seq: number
  ts_ms: number
  payload: Record<string, unknown>
}
```

That keeps the browser session model independent from the embedded transport implementation.

## Firmware Design

### 1. Runtime pieces

The firmware should contain four main subsystems:

1. `M5DialBoard`
   - reads encoder, button, touch
   - produces `InputEvent`

2. `RemoteModel`
   - stores connection state
   - stores last input summary
   - stores last server-provided display label/value

3. `DialClient`
   - owns the outbound server connection
   - sends `hello`, `status`, `input`, and `heartbeat`
   - receives `display_state`, `ack`, and `ping`

4. `UiStatusScreen`
   - shows Wi-Fi state, server connectivity, and the current label/value

### 2. Task model

Recommended task split:

1. `io_task`
   - exactly like `0072`
   - polls board hardware
   - posts `InputEvent` values into a queue

2. `app_task`
   - owns `RemoteModel`
   - drains the input queue
   - updates LVGL
   - serializes outbound protocol messages
   - consumes messages arriving from `DialClient`

3. `dial_client_task`
   - maintains the outbound connection
   - retries with backoff
   - sends heartbeats
   - posts received server messages into an inbound queue

This preserves the strongest property of the existing firmware examples: one owner task mutates the main application model.

### 3. Wi-Fi behavior

Recommended order:

1. Try configured STA credentials.
2. If connection fails, show disconnected state clearly.
3. Keep the console on USB Serial/JTAG for provisioning or debugging.

Do not block firmware usefulness on an on-device provisioning UX in v1. For now, a console-assisted configuration path is enough.

## Server Design Expectations

The server is outside the firmware folder, but the firmware ticket still needs to describe what the server must do.

Required server responsibilities:

1. Accept a device connection on `/device/ws`.
2. Accept browser connections on `/ws` or use SSE if preferred for browser fanout.
3. Serve the React app from `/`.
4. Map inbound device messages into browser-facing envelopes.
5. Optionally send compact `display_state` messages back to the device.

Suggested pipeline:

```text
M5Dial /device/ws -> server ingress -> normalize/project -> browser fanout -> React store/reducer
```

This is much closer to the imported note than the earlier device-hosted draft was.

## React Widget Design

### 1. Browser state model

The React app should have two distinct layers of state:

1. connection/session state from the server
2. widget state driven by dial input

Example:

```ts
type ConnectionState = {
  connected: boolean
  activeDeviceId: string | null
  lastSeq: number
}

type WidgetState = {
  mode: 'browse' | 'adjust'
  focusIndex: number
  value: number
}
```

### 2. Browser reducer example

```ts
function reduceDialInput(state: WidgetState, msg: BrowserEnvelope): WidgetState {
  if (msg.topic !== 'dial.input') return state

  const p = msg.payload

  if (p.source === 'encoder' && p.action === 'delta') {
    const delta = Number(p.delta ?? 0)
    return state.mode === 'adjust'
      ? { ...state, value: clamp(state.value + delta) }
      : { ...state, focusIndex: clampFocus(state.focusIndex + delta) }
  }

  if (p.source === 'button' && p.action === 'short_press') {
    return { ...state, mode: state.mode === 'browse' ? 'adjust' : 'browse' }
  }

  return state
}
```

### 3. Feedback loop to the dial

When widget state changes meaningfully, the server should derive a compact dial-display summary and push it back to the device:

```json
{
  "type": "display_state",
  "label": "Brightness",
  "value_text": "72%",
  "mode": "adjust"
}
```

That is how the dial screen stays informative without needing to render the full browser UI.

## Detailed Implementation Plan

### Phase 0. Start from the working M5Dial base

Goal:

- create the new firmware project from `0072`

Steps:

1. Copy the minimal M5Dial project structure.
2. Keep `M5DialBoard`, `InputEvent`, and LVGL port logic.
3. Remove assumptions tied to the timer demo domain.
4. Localize any externally referenced helper sources.

### Phase 1. Add a dial-focused local status UI

Goal:

- show enough state on the dial to debug networked behavior

Display:

- Wi-Fi state
- server connection state
- active device ID
- last input
- latest widget label/value from server

### Phase 2. Add Wi-Fi client and outbound connection management

Goal:

- make the dial connect to the server reliably

Files:

- `main/dial_client.h`
- `main/dial_client.cpp`

Responsibilities:

- connect to Wi-Fi
- open WebSocket to configured server URL
- reconnect with backoff
- send `hello` then `status`
- send heartbeat every N seconds

### Phase 3. Encode and send normalized input messages

Goal:

- turn `InputEvent` into protocol messages

Rules:

- encoder delta becomes `input/delta`
- short press becomes `input/short_press`
- long press becomes `input/long_press`
- swipe becomes `input/swipe`

Do not send raw GPIO details.

### Phase 4. Process server feedback locally

Goal:

- let the server describe current widget state back to the dial

Handle:

- `display_state`
- `ack`
- `ping`

This should update `RemoteModel`, not mutate UI objects directly from the network callback.

### Phase 5. Document the server contract explicitly

Goal:

- make implementation handoff clean even if server work happens elsewhere

Deliverables:

- `docs/protocol.md`
- `docs/server-contract.md`

Include:

- endpoint list
- message schemas
- reconnect expectations
- browser fanout expectations

### Phase 6. End-to-end validation

Goal:

- prove the whole loop works

Flow:

1. Dial boots and joins Wi-Fi.
2. Dial connects to the server.
3. Server marks device connected.
4. Browser loads the React app from the server.
5. Browser receives live dial events.
6. Widget updates.
7. Server sends `display_state` back to the dial.
8. Dial screen reflects current widget state.

## Pseudocode

### Firmware boot

```cpp
extern "C" void app_main() {
  board.init();
  input_queue = xQueueCreate(...);
  inbound_server_queue = xQueueCreate(...);

  start_wifi();

  xTaskCreatePinnedToCore(io_task, ...);
  xTaskCreatePinnedToCore(app_task, ...);
  xTaskCreatePinnedToCore(dial_client_task, ...);
}
```

### App task

```cpp
while (true) {
  if (xQueueReceive(input_queue, &event, wait_ticks) == pdTRUE) {
    remote_controller.handle_input(event, model);
    dial_client_send(remote_protocol_encode_input(model.next_seq(), event));
  }

  if (xQueueReceive(inbound_server_queue, &msg, 0) == pdTRUE) {
    remote_controller.handle_server_message(msg, model);
  }

  ui_status_screen.apply(model.snapshot());
  lv_timer_handler();
}
```

### Dial client task

```cpp
while (true) {
  ensure_wifi_connected();
  ensure_server_socket_connected();

  send_hello_once();
  flush_outbound_messages();
  read_server_messages_into_queue();
  maybe_send_heartbeat();

  vTaskDelay(pdMS_TO_TICKS(25));
}
```

## Testing And Validation Strategy

### Firmware validation

- `idf.py set-target esp32s3`
- `idf.py build`
- confirm no compile-time references escape the project folder

### Device/network validation

- dial joins Wi-Fi
- dial reaches server
- dial reconnects after temporary network loss

### Protocol validation

- `hello` sent first
- `input` messages sequence correctly
- heartbeat interval is stable
- malformed server messages do not crash the device

### Browser validation

- browser app loads from the server, not the dial
- browser reacts to dial input
- reconnecting the browser does not require rebooting the dial

### Local screen validation

- no server connected: screen clearly says so
- server connected: screen shows current widget label/value
- last input is visible for debugging

## Risks, Alternatives, And Open Questions

### Main risks

- Outbound connection reliability is the new core embedded risk.
- The server contract can become muddy if browser-facing and device-facing messages are not separated cleanly.
- The dial UI can become confusing if too much server-derived state is shown.

### Alternatives considered

1. Device-hosted React app
   - rejected because the user explicitly corrected this deployment model

2. HTTP POST only from the dial
   - possible, but weaker for bidirectional feedback and presence

3. Raw hardware telemetry to the server
   - rejected because semantic input messages are cleaner and more stable

### Open questions

1. Should the device connect to exactly one configured server URL or discover it?
2. Do we want mutual authentication in v1, or just a device ID and shared token?
3. Should browser fanout use WebSocket or SSE?
4. How much widget state should be reflected back to the dial screen?

Recommendation:

- one configured server URL
- simple token auth in v1
- WebSocket on both device and browser sides
- minimal `display_state` feedback only

## References

- `0072-m5dial-timer-demo/main/app_main.cpp:17-103`
- `0072-m5dial-timer-demo/main/input_events.h:7-27`
- `0072-m5dial-timer-demo/main/m5dial_board.cpp:14-38`
- `0072-m5dial-timer-demo/main/m5dial_board.cpp:181-351`
- `0073-m5dial-film-developer-timer/main/app_main.cpp:30-179`
- `0048-cardputer-js-web/web/src/ui/store.ts:47-137`
- `ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/sources/local/01-esp32-knob-web.md:1-246`
