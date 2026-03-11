---
Title: Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0074-m5dial-web-remote/firmware/main/app_main.cpp
      Note: Applies queued browser commands on the app task and updates the display state
    - Path: 0074-m5dial-web-remote/server/hub.go
      Note: Validation-critical websocket routing and ack logging path
    - Path: 0074-m5dial-web-remote/web/src/app.tsx
      Note: Command UI used for manual validation and device feedback
ExternalSources: []
Summary: Implementation diary for adding browser-to-device commands to the M5Dial remote-control stack.
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Track the implementation of a browser-originated command path that reaches the Go server and then the ESP32 firmware.

## Step 1: Create the ticket and define the command path

I created this ticket after the one-way telemetry path was already working. The immediate goal is to make browser-originated commands explicit and narrow, rather than adding a large bidirectional RPC system.

### Prompt Context

**User prompt (verbatim):** "Ok, let's do this: 

- fix the UI (no need for a ticket, just do it and commit)
- add a command to send UI commands from the react all the way back to the esp32. (make a new ticket with implementation plan and tasks and then work on them)"

**Assistant interpretation:** Fix the current UI issue separately, then create a new ticket specifically for the browser-to-device command path and implement it.

**Inferred user intent:** Keep the on-device UI cleanup separate from the new round-trip control feature, while still documenting the new feature properly.

### What I did

- Created ticket `ESP-29-M5DIAL-BROWSER-COMMANDS`.
- Added a design doc and diary.
- Wrote the initial implementation plan and tasks.

### What should be done in the future

- Record the concrete code changes and validation results once the command path is implemented.

## Step 2: Add firmware-side command parsing and acknowledgements

The firmware already had an outbound websocket client and an app task that owned the visible UI state. I kept that split intact and added a queue boundary so browser commands are parsed in `remote_client` but only applied in `app_task`.

This step also added an explicit `request_id` passthrough. That changed the feature from "browser can send a blind command" into "browser can match a specific click to a specific device acknowledgement," which is what made live validation practical.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Implement the firmware half of the browser-to-device control path without breaking the existing dial-to-server telemetry flow.

**Inferred user intent:** Make browser-originated commands safe to receive and observable on real hardware.

**Commit (code):** `c549955` — "feat: add browser command handling to firmware"

### What I did

- Added `RemoteUiCommand` and a dedicated FreeRTOS queue in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.h`.
- Parsed inbound websocket JSON in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.cpp`.
- Wired the queue into `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/app_main.cpp`.
- Added two command handlers:
  - `show_message`
  - `set_position`
- Added `ui_command_ack` responses that include the browser `request_id`.
- Built with:
  - `cd 0074-m5dial-web-remote/firmware`
  - `. $HOME/esp/esp-idf-5.4.1/export.sh`
  - `idf.py build`
- Flashed with:
  - `idf.py -p /dev/ttyACM0 flash`

### Why

- Websocket callbacks should not mutate shared UI state directly.
- The browser needed a correlation key to tell which acknowledgement matched which command.
- Free-form message text can contain quotes, so `ui_command_ack` needed JSON-safe serialization.

### What worked

- Firmware built successfully.
- Flashing to `/dev/ttyACM0` succeeded without the ACM device disappearing mid-flash.
- The dial came back online and resumed sending heartbeats after the flash.

### What didn't work

- Nothing failed in this step after the final patch set. The only warnings during build were the pre-existing deprecated PCNT driver warnings from `ESP32Encoder`.

### What I learned

- Using `cJSON_PrintUnformatted` for `ui_command_ack` is safer than hand-formatting JSON once user-entered text is part of the payload.
- Carrying the browser `request_id` through firmware is cheap and removes a lot of ambiguity during debug.

### What was tricky to build

- The tricky part was preserving the app-task ownership model. It would have been easy to update `position` or `last_ui_message` inside the websocket callback, but that would have mixed networking and UI state in the wrong thread of execution.
- The acknowledgement payload also had a hidden edge: once the React side can send arbitrary text, hand-built JSON becomes fragile. The fix was to build the ack with `cJSON` instead of string interpolation.

### What warrants a second pair of eyes

- Queue sizing and backpressure on repeated browser commands.
- Whether `request_id` should stay `uint32_t` or become a larger type if command volume grows.

### What should be done in the future

- Add at least one command that affects hardware behavior beyond text and position.
- Decide whether firmware should reject oversized or malformed command payloads more explicitly.

### Code review instructions

- Start in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.cpp` and `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/app_main.cpp`.
- Validate with `idf.py build`.
- Validate on hardware with `idf.py -p /dev/ttyACM0 flash` and watch for `ui_command_ack` frames arriving on the server.

### Technical details

- `RemoteUiCommand` carries:
  - `type`
  - `request_id`
  - `command`
  - `text`
  - `value`
- `ui_command_ack` carries:
  - `device_id`
  - `seq`
  - `request_id`
  - `command`
  - `status`
  - `pos`
  - `text`

## Step 3: Route browser commands through the Go hub

The server previously treated browser-originated websocket frames as unexpected input. This step converted the hub into a real broker for `ui_command` messages and fixed a concurrency problem in the process.

The important architectural change was to add per-connection write mutexes for both browser sockets and device sockets. Without that, broadcast writes and direct command-result replies could race each other on the same websocket.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Implement the server bridge so browser commands are validated, routed to the correct device, and logged clearly enough to debug.

**Inferred user intent:** Make the server the authoritative rendezvous point for command delivery and connection diagnostics.

**Commit (code):** `69a922f` — "feat: route browser ui commands to devices"

### What I did

- Reworked `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/hub.go`.
- Added `BrowserSocket` and `DeviceSocket` wrappers with per-connection write mutexes.
- Added active device socket tracking keyed by `device_id`.
- Validated browser frames as `ui_command`.
- Forwarded validated payloads unchanged to the target device websocket.
- Returned immediate `ui_command_result` frames to the initiating browser.
- Recorded `ui_command_ack` payloads into device state and history.
- Ran:
  - `gofmt -w 0074-m5dial-web-remote/server/hub.go`
  - `cd 0074-m5dial-web-remote/server && go test ./...`

### Why

- The hub needed a device registry before it could route browser commands.
- Immediate `ui_command_result` feedback makes it obvious whether the server rejected or queued a command before the device ack arrives.
- Concurrent writes on a Gorilla websocket connection are unsafe without coordination.

### What worked

- `go test ./...` passed.
- The server tracked the dial as connected and recorded command acknowledgements in `/api/status`.

### What didn't work

- Attempting to start a second server on `:18080` failed with `listen tcp :18080: bind: address already in use` because an existing instance was already running.

### What I learned

- The existing port conflict was not a blocker because the already-running server had the freshly built behavior and a live device connection.
- The server snapshot is a useful validation surface because it captures both live device state and raw history frames.

### What was tricky to build

- The subtle issue was write concurrency, not message parsing. Once the server both broadcasts status updates and writes direct command results, the old `*websocket.Conn` maps are no longer safe enough.
- Another nuance was stale disconnect handling. If a device reconnects and replaces its socket, an old socket closing later should not mark the device disconnected. The fix was to only unregister a socket when the closing connection matches the currently registered one.

### What warrants a second pair of eyes

- The choice to return `ui_command_result` only to the initiating browser rather than broadcasting it.
- Whether the device state should preserve a fuller command history instead of only the latest command fields.

### What should be done in the future

- Add metrics or counters for rejected browser commands.
- Consider pruning device records if disconnected devices accumulate over time.

### Code review instructions

- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/hub.go` from `HandleBrowserConn` through `routeCommandToDevice`.
- Validate with `go test ./...`.
- Exercise with a websocket client and confirm `ui_command_result` followed by `ui_command_ack`.

### Technical details

- Browser command contract:
  - `type: "ui_command"`
  - `device_id`
  - `command`
  - `request_id`
  - optional `text`
  - optional `value`
- Browser immediate feedback contract:
  - `type: "ui_command_result"`
  - `device_id`
  - `command`
  - `request_id`
  - `status`
  - optional `reason`

## Step 4: Add React command controls and validate the full round trip

The React app needed two things: a way to send commands and a way to explain what happened after each click. I added both, then validated the round trip against the live server and dial.

This step also updated the embedded production bundle so the Go server and the Vite app present the same command UI.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Add browser-side controls for the new command path and confirm they work against the actual ESP32.

**Inferred user intent:** Make the command path tangible and debuggable from the browser, not just wired in code.

**Commit (code):** `f9857ea` — "feat: add browser controls for dial commands"

### What I did

- Updated `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/store.ts` with:
  - `sendUiCommand(...)`
  - request ID generation
  - `ui_command_result` handling
  - `ui_command_ack` feedback handling
- Updated `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/app.tsx` with:
  - message command form
  - set-position command form
  - live feedback panel
  - device-state fields for last command and last text
- Updated `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/styles.css` for the new command UI.
- Rebuilt the embedded bundle and copied it into `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/static/`.
- Ran:
  - `cd 0074-m5dial-web-remote/web && npm run build`
- Validated live against `http://127.0.0.1:18080/api/status`.
- Sent a fresh websocket command with Node using `request_id=9001` and observed:
  - `ui_command_result` status `queued`
  - `ui_command_ack` status `applied`
  - matching `request_id=9001`

### Why

- A command path without browser feedback is hard to debug.
- Showing the last command fields in the UI makes it obvious when the dial has actually applied a browser-originated update.
- The embedded bundle should stay aligned with the Vite source tree.

### What worked

- `npm run build` passed.
- The live server snapshot already showed real `ui_command_ack` history from browser requests `1` and `2`.
- A fresh validation command returned:
  - `result {"status":"queued","request_id":9001}`
  - `ack {"status":"applied","request_id":9001,"text":"node validation"}`

### What didn't work

- A delete-and-replace copy of `server/static` using `rm -rf ../server/static/*` was blocked by command policy, so I switched to a straight copy from `dist/.` into `server/static/`.

### What I learned

- The browser-side feedback loop is much easier to reason about when immediate server acceptance and eventual device application are shown as separate states.
- The live `/api/status` snapshot is enough to confirm the server saw the acks even if a browser tab is not open.

### What was tricky to build

- The tricky part was avoiding false confidence from the websocket frame log alone. A raw frame in the browser does not prove the dial applied it, which is why the UI now distinguishes `sent`, `queued`, and `ack:applied`.
- Keeping the embedded server assets in sync is also easy to miss because the Vite app can look correct while the Go server is still serving older files. Rebuilding and copying the bundle closed that gap.

### What warrants a second pair of eyes

- Whether `lastCommandFeedback` should evolve from a single latest result into a small history list.
- Whether the command inputs need stricter client-side validation or throttling.

### What should be done in the future

- Add one or two canned action buttons beyond `show_message` and `set_position`.
- Consider a richer history table for browser-originated commands and their acks.

### Code review instructions

- Start with `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/store.ts`, then `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/app.tsx`.
- Validate with `npm run build`.
- Confirm end-to-end with a websocket client or the live UI and match `request_id` across `ui_command_result` and `ui_command_ack`.

### Technical details

- Validation command:
  - `type: "ui_command"`
  - `device_id: "m5dial-b76a94"`
  - `command: "show_message"`
  - `request_id: 9001`
  - `text: "node validation"`
- Validation response:
  - `ui_command_result` with `status: "queued"`
  - `device_event.payload.type: "ui_command_ack"`
  - `device_event.payload.request_id: 9001`
