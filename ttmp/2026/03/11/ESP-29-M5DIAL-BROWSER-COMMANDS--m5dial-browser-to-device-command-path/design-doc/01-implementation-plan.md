---
Title: Implementation plan
Ticket: ESP-29-M5DIAL-BROWSER-COMMANDS
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
    - Path: 0074-m5dial-web-remote/firmware/main/remote_client.cpp
      Note: Parses inbound ui_command frames and emits ui_command_ack responses
    - Path: 0074-m5dial-web-remote/server/hub.go
      Note: Routes browser ui_command frames to active device sockets and broadcasts device state
    - Path: 0074-m5dial-web-remote/web/src/store.ts
      Note: Browser websocket store sends ui_command frames and tracks acks/results
ExternalSources: []
Summary: Add a browser-to-server-to-device command path so the React UI can send explicit commands back to the M5Dial firmware over the existing websocket topology.
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# Implementation plan

## Executive Summary

The current stack is mostly one-way: the M5Dial sends input upstream, the Go server rebroadcasts device state, and the React UI renders it. This ticket adds the reverse path so a browser operator can issue a UI command and have the ESP32 receive and act on it.

For v1, the command path stays narrow and explicit, but it now includes an acknowledgement path tied to a browser-generated request ID:

- browser sends a `ui_command` frame over `/ws/browser`
- Go server validates the target device and forwards the same command frame to the active device socket
- firmware receives the frame on its existing websocket client and applies it on the app task
- firmware emits `ui_command_ack` with the original `request_id`
- Go server rebroadcasts the ack as a normal device event and browser clients correlate it with the originating command

## Problem Statement

The system currently proves telemetry flow but not control flow from browser to hardware. That leaves an important integration gap: there is no way to test browser-originated commands, device acknowledgements, or firmware-side command handling.

## Proposed Solution

Implement a small command contract with two initial command kinds:

- `show_message`
  - payload: short display text
  - firmware behavior: show the message on the local screen and record it as the last UI command
- `set_position`
  - payload: integer position
  - firmware behavior: replace the local `position` field so the screen and next encoder delta reflect the new base position

Message flow:

```text
React -> ws/browser -> Go hub -> ws/device -> ESP32 remote_client -> app queue -> app state/screen
      <- ui_command_result <-        <- ui_command_ack <-                           <-
```

## Design Decisions

- Reuse the existing browser websocket rather than add a new HTTP POST endpoint.
- Keep commands device-targeted with explicit `device_id`.
- Route commands through a FreeRTOS queue on the ESP32 so websocket callbacks do not mutate UI/app state directly.
- Add a lightweight `request_id` so the browser can correlate `ui_command_result` and `ui_command_ack`.
- Serialize websocket writes per browser and per device connection in the Go hub to avoid concurrent write races.

## Alternatives Considered

- HTTP POST from browser to server: simpler to debug, but it introduces a second control channel when the browser already has a websocket.
- Direct browser-to-device websocket: rejected because the server should remain the rendezvous point and device registry.
- Full RPC/ack/retry protocol: useful later, but unnecessary for the first browser-to-device command proof.

## Implementation Plan

1. Add firmware-side command queue and command struct.
2. Parse inbound websocket frames in `remote_client` and enqueue recognized device commands.
3. Apply commands in `app_task`, update the local screen state, and emit `ui_command_ack`.
4. Extend the Go hub to store active device sockets and forward browser `ui_command` frames to the matching device.
5. Return immediate `ui_command_result` frames to the initiating browser client.
6. Extend the React store with `sendUiCommand(...)` and request/ack feedback handling.
7. Add a small command panel to the UI for message text and set-position numeric command.
8. Verify the round trip on live hardware by matching `request_id` across:
   - browser `ui_command_result`
   - server snapshot/history
   - firmware-originated `ui_command_ack`
