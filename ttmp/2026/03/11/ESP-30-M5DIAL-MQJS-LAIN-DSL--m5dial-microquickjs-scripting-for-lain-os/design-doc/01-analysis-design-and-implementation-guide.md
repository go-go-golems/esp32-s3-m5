---
Title: Analysis, design, and implementation guide
Ticket: ESP-30-M5DIAL-MQJS-LAIN-DSL
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - javascript
    - ui
    - websocket
    - webserver
    - http
    - wifi
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: Reference MicroQuickJS service lifecycle and bootstrap pattern to port into 0074
    - Path: 0074-m5dial-web-remote/firmware/main/app_main.cpp
      Note: Current app-task ownership
    - Path: 0074-m5dial-web-remote/firmware/main/remote_client.cpp
      Note: Current websocket ingress and command parsing path for browser-to-device control
    - Path: 0074-m5dial-web-remote/server/hub.go
      Note: Current broker topology to extend from ui_command routing into script routing
    - Path: 0074-m5dial-web-remote/web/src/store.ts
      Note: Current browser runtime
ExternalSources: []
Summary: Detailed intern-facing design for adding an on-device MicroQuickJS service and a JavaScript DSL to the 0074 M5Dial Lain OS runtime.
LastUpdated: 2026-03-11T21:18:35-04:00
WhatFor: Explain the current architecture, the relevant QuickJS precedent, and a practical implementation plan for browser-delivered scripting on the M5Dial.
WhenToUse: Use when implementing or reviewing the planned QuickJS service, native Lain DSL bindings, websocket script transport, or app-task ownership changes.
---


# Analysis, design, and implementation guide

## Executive summary

`0074-m5dial-web-remote` already has the three runtime layers needed for a scriptable Lain OS:

1. ESP32 firmware that owns the physical device state and display.
2. Go websocket server that brokers messages between devices and browsers.
3. React frontend that renders a narrative radio UI and already sends structured commands back to the device.

What it does **not** have yet is an on-device scripting runtime. The closest existing precedent in this repository is `0048-cardputer-js-web`, which already embeds `mquickjs` plus `mqjs_service` and evaluates JavaScript in a dedicated service task.

The proposed direction is:

- add a MicroQuickJS service to the 0074 firmware,
- expose a small, explicit `lain` JavaScript API,
- route browser-sent source code to the device over the existing `/ws/browser` -> `/ws/device` path,
- keep the application task as the only code allowed to mutate screen-visible device state.

The key design constraint is ownership: JavaScript should **request** Lain OS actions, not mutate the `AppContext` directly. The JS runtime runs in its own service task; the device UI remains owned by `app_task`.

## Problem statement

The current Lain OS firmware and browser experience are still command-shaped rather than script-shaped.

Today the browser can send individual commands such as:

- `set_mode`
- `set_station`
- `set_band`
- `show_reveal`

Those are enough to drive the radio experience, but they are not expressive enough for:

- scripted sequences,
- dynamic station generation,
- testing complex device behavior,
- prototyping narrative interactions,
- uploading custom behaviors without reflashing firmware.

The user request is more ambitious than “add one more command.” The goal is to run `microquickjs` on the device and let the browser send JavaScript over websocket so the device can execute custom code against a clean DSL.

## Scope

### In scope

- on-device MicroQuickJS service in 0074 firmware,
- JavaScript bindings for Lain OS primitives,
- websocket transport for browser-to-device script execution,
- browser UX for entering/running scripts and seeing results,
- result and event frames flowing back to the browser,
- implementation guidance detailed enough for a new intern.

### Out of scope for v1

- secure multi-user script authorization,
- persistent script storage in flash,
- package management,
- arbitrary filesystem/network APIs in JS,
- long-running background programs with scheduler semantics.

## Current-state architecture

### 1. 0074 firmware structure

The project README says the firmware is an ESP-IDF app using USB console, `wifi_mgr`, and an outbound websocket client, while the server and web UI are separate processes. See:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/README.md`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/PLAYBOOK.md`

The firmware entrypoint in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/app_main.cpp` shows a clear task split:

- `io_task` polls the M5Dial hardware and writes `InputEvent` objects into `input_queue`.
- `app_task` owns the mutable device UI model and draws either debug mode or radio mode.
- `remote_client` pushes telemetry outward and receives inbound browser-originated commands.

Important current structures:

- `AppMode` with `kDebug` and `kRadio`
- `RadioState` with station definitions, current frequency position, band, lock status, and temporary reveal text
- `AppContext` which contains `input_queue`, `ui_command_queue`, `position`, `sequence`, and `radio`

Observed in code:

- `AppContext` is defined in `app_main.cpp` lines 95-107.
- radio rendering is in `draw_radio_screen()` at lines 269-395.
- input handling is in `handle_input_event()` at lines 431-496.
- browser command handling is in `handle_ui_command()` at lines 500-586.
- queue setup and task startup happen in `app_main()` at lines 640-680.

### 2. Current firmware command ingress

`remote_client.cpp` is already parsing browser-originated JSON messages from the websocket and turning them into `RemoteUiCommand` objects pushed into a FreeRTOS queue.

Observed behavior in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.cpp`:

- `enqueue_ui_command()` parses inbound JSON with `cJSON`.
- only messages with `type: "ui_command"` are accepted.
- the command name is mapped to a local enum.
- the queue entry contains `request_id`, `command`, `text`, and `value`.

Supported command types as of now, from `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.h` lines 35-51:

- `kShowMessage`
- `kSetPosition`
- `kSetMode`
- `kSetStation`
- `kSetBand`
- `kShowReveal`

This is important because it already gives us the first half of the future scripting model:

- a transport,
- a queue,
- an app-task-owned mutation path.

### 3. Current Go server role

`/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/main.go` exposes:

- `/api/status`
- `/ws/device`
- `/ws/browser`
- static-file serving for the embedded React bundle

`/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/hub.go` is the real message broker.

Observed responsibilities:

- track connected browsers and devices,
- serialize writes per websocket connection,
- forward browser `ui_command` frames to the correct device,
- store recent `DeviceState`,
- broadcast `device_event` and `server_status` snapshots.

This is already the right shape for script transport. We do not need a second control plane.

### 4. Current React runtime

`/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/store.ts` owns the browser-side runtime model:

- station definitions,
- synthetic audio behavior,
- dwell timers for hidden stations,
- websocket connection logic,
- `sendUiCommand()`,
- `initRadioMode()`.

Observed behavior:

- `connectWs()` listens to `server_snapshot`, `server_status`, `ui_command_result`, and `device_event`.
- `sendUiCommand()` serializes command frames and assigns `request_id`.
- `initRadioMode()` sends `set_mode` and then one `set_station` command per station definition.

`/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/app.tsx` is now a Lain-themed terminal/radio UI, with:

- a connect screen,
- a radio visualization,
- a station log,
- hidden-text reveal animation,
- no script editor yet.

### 5. Existing QuickJS precedent in this repository

`0048-cardputer-js-web` is the most relevant pattern to copy.

Key files:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/README.md`

What `0048` already proves:

- `mquickjs` and `mqjs_service` can run inside an ESP-IDF project.
- the JS VM can be wrapped in a dedicated service task with bounded memory and timeouts.
- bootstrapped JS namespaces can expose a narrow DSL (`encoder.on`, `emit`, `__0048_take_lines`).
- JS output/events can be flushed back to a browser-facing channel.

Observed in `js_service.cpp`:

- `js_service_start()` creates a dedicated service with configured stack, queue length, arena size, and stdlib.
- `job_bootstrap()` defines the JavaScript-side API namespace.
- `js_service_eval_to_json()` evaluates JS safely and reports `{ ok, output, error, timed_out }`.
- event callbacks are posted into the service rather than executed ad hoc from random tasks.

## Gap analysis

The current 0074 stack is close, but not ready, for scripting.

### What already exists

- browser -> server -> device message path,
- device -> server -> browser acknowledgement path,
- app-task ownership of mutable UI state,
- Lain radio primitives already encoded as discrete commands,
- an in-repo QuickJS service implementation pattern.

### What is missing

1. No JS runtime in 0074 firmware.
2. No reusable application-command bus independent of websocket command names.
3. No native bindings that map JS calls to Lain OS actions.
4. No websocket message type for script source or script results.
5. No browser editor/console for authoring scripts.
6. No guardrails specific to remote code execution.

### Critical design constraint

The easiest implementation would be “run JS, let native bindings write directly into `AppContext`.”

That would be wrong.

Why:

- `AppContext` is currently owned by `app_task`.
- direct mutation from the JS service task would create cross-task state ownership bugs.
- future rendering and control logic will become much harder to reason about.

So the correct design is:

- JS bindings enqueue typed app commands,
- `app_task` consumes those commands,
- visible state changes happen there,
- results are reported back asynchronously.

## Proposed solution

### High-level architecture

```text
Browser script editor
    |
    | script_eval { request_id, code, device_id }
    v
Go /ws/browser
    |
    | route to active device socket
    v
ESP32 remote_client
    |
    | queue script request to JS service
    v
MicroQuickJS service task
    |
    | call native lain.* bindings
    v
App command queue
    |
    | app_task applies state changes
    v
Display + telemetry + script_result/script_event/script_console
    |
    v
Go server -> browser websocket
```

### New firmware components

Recommended new files under `0074-m5dial-web-remote/firmware/main/`:

- `js_service.cpp`
- `js_service.h`
- `lain_js_bindings.cpp`
- `lain_js_bindings.h`
- `app_commands.h`
- `app_commands.cpp`

Optional if the code grows:

- `script_protocol.h`
- `script_protocol.cpp`

### New central abstraction: AppCommand

Today the firmware has `RemoteUiCommand`, which is tied to one transport and one schema.

For scripting, the better abstraction is an internal app command that represents intent independent of origin:

```cpp
enum class AppCommandType : uint8_t {
  kSetMode,
  kSetPosition,
  kSetBand,
  kDefineStation,
  kShowReveal,
  kShowMessage,
};

struct AppCommand {
  AppCommandType type;
  uint32_t request_id;
  char source[16];   // "ui", "js", "console"
  int32_t value;
  char text[96];
};
```

Why this matters:

- websocket UI commands can be translated into `AppCommand`,
- JS bindings can also emit `AppCommand`,
- `app_task` handles one queue and one set of mutation rules.

### JS service model

Use the same model as `0048`:

1. one dedicated `mqjs_service` task,
2. bounded arena memory,
3. bounded evaluation deadline,
4. bootstrapped global namespace,
5. post work into that task, do not run JS directly from networking callbacks.

Recommended service responsibilities:

- maintain the JS VM lifecycle,
- expose `lain.*` functions,
- evaluate source code from incoming script requests,
- emit structured result and console frames,
- never draw to the display directly.

### Proposed JavaScript DSL

The JS API should be small, unsurprising, and directly mapped to visible device primitives.

#### Namespace

```js
lain.device.id()
lain.device.mode()
lain.device.showMessage(text)

lain.radio.setMode("debug" | "radio")
lain.radio.tune(position)
lain.radio.band("AM" | "FM" | "WIRED")
lain.radio.lock(true | false)
lain.radio.reveal(text, durationMs)
lain.radio.defineStation(position, {
  type: "empty" | "clear" | "static" | "hidden" | "distorted",
  name: "phantom_relay"
})

lain.runtime.emit(topic, payload)
lain.runtime.log(...parts)
lain.runtime.now()
lain.runtime.sleep(ms)   // optional later, not required in v1
```

#### Example script

```js
lain.radio.setMode("radio")
lain.radio.band("WIRED")

for (let i = 0; i < 5; i++) {
  lain.radio.defineStation(i * 12, {
    type: i % 2 === 0 ? "clear" : "hidden",
    name: "layer_" + i,
  })
}

lain.radio.tune(12)
lain.radio.reveal("close the world", 2500)
lain.runtime.log("script finished")
```

#### Design principles for the DSL

- imperative and obvious,
- maps to device concepts already present in 0074,
- avoids raw pointer/state access,
- narrow enough that the native layer remains auditable.

## Websocket API design

### Browser -> server -> device: `script_eval`

```json
{
  "type": "script_eval",
  "device_id": "m5dial-b76a94",
  "request_id": 1042,
  "filename": "<browser>",
  "code": "lain.radio.setMode('radio'); lain.runtime.log('ok')",
  "timeout_ms": 100
}
```

### Device -> server -> browser: `script_result`

```json
{
  "type": "script_result",
  "device_id": "m5dial-b76a94",
  "request_id": 1042,
  "ok": true,
  "timed_out": false,
  "output": "",
  "error": null,
  "ts_ms": 123456
}
```

### Device -> server -> browser: `script_console`

```json
{
  "type": "script_console",
  "device_id": "m5dial-b76a94",
  "request_id": 1042,
  "level": "info",
  "text": "script finished",
  "ts_ms": 123457
}
```

### Device -> server -> browser: `script_event`

```json
{
  "type": "script_event",
  "device_id": "m5dial-b76a94",
  "request_id": 1042,
  "topic": "radio.reveal",
  "payload": { "text": "close the world" },
  "ts_ms": 123458
}
```

### Immediate server feedback: `script_result` with queued/rejected status

This can mirror the existing `ui_command_result` behavior or stay separate. I recommend staying separate and making server acceptance explicit:

```json
{
  "type": "script_result",
  "device_id": "m5dial-b76a94",
  "request_id": 1042,
  "ok": false,
  "stage": "server",
  "error": "device m5dial-b76a94 is not connected"
}
```

## Detailed implementation design

### Phase 1: refactor command ownership inside the firmware

Goal:

- create a transport-agnostic app command queue,
- make websocket commands and future JS bindings both use it.

Work:

1. Introduce `AppCommand`.
2. Replace direct `RemoteUiCommand` handling in `app_task` with translation into `AppCommand`.
3. Keep `handle_ui_command()` logic but rename/rework it into `apply_app_command()`.

Pseudocode:

```cpp
void remote_client_on_ui_command(RemoteUiCommand ui) {
  AppCommand cmd = translate_ui_to_app(ui);
  xQueueSend(app_command_q, &cmd, 0);
}

void app_task(...) {
  while (true) {
    drain_input_events();
    drain_app_commands();
    render_if_dirty();
  }
}
```

### Phase 2: add MicroQuickJS service

Goal:

- bring the `0048` service pattern into 0074 firmware.

Work:

1. Add `mquickjs` and `mqjs_service` to firmware `CMakeLists.txt`.
2. Copy the service lifecycle pattern from `0048`:
   - start mutex,
   - `mqjs_service_start()`,
   - configured arena bytes,
   - bootstrap job.
3. Define a 0074-specific bootstrap namespace:
   - `globalThis.lain`
   - `globalThis.lain.device`
   - `globalThis.lain.radio`
   - `globalThis.lain.runtime`

Do **not** copy the `0048` HTTP eval endpoint design. The transport in 0074 is external-server-hosted, not device-hosted.

### Phase 3: implement native bindings

Bindings should be thin shims from JS to `AppCommand`.

Examples:

```cpp
static JSValue js_lain_radio_setMode(JSContext* ctx) {
  const char* mode = JS_ToCString(ctx, JS_GetArg(ctx, 0));
  AppCommand cmd = {};
  cmd.type = AppCommandType::kSetMode;
  cmd.value = strcmp(mode, "radio") == 0 ? 1 : 0;
  snprintf(cmd.source, sizeof(cmd.source), "js");
  enqueue_app_command(cmd);
  return JS_UNDEFINED;
}
```

```cpp
static JSValue js_lain_runtime_log(JSContext* ctx) {
  std::string text = join_args(ctx);
  script_emit_console("info", text);
  return JS_UNDEFINED;
}
```

Important rule:

- bindings may validate,
- bindings may enqueue,
- bindings may emit logs/events,
- bindings must not touch `ctx->radio` or `ctx->position` directly.

### Phase 4: extend remote_client and protocol handling

The current `remote_client` only knows `ui_command` as an inbound browser-originated message type.

Add a second ingress path:

- `type: "script_eval"`

Recommended change:

- keep `remote_client.cpp` responsible only for parsing and routing inbound wire messages,
- add a JS-service request queue separate from `ui_command_queue`.

Pseudocode:

```cpp
if (type == "script_eval") {
  ScriptEvalRequest req = parse_script_eval(root);
  xQueueSend(js_eval_q, &req, 0);
}
```

### Phase 5: extend Go hub routing

`hub.go` currently handles `ui_command` from browsers and forwards it to devices.

Add support for:

- `script_eval` browser frames,
- `script_result` / `script_console` / `script_event` device frames.

Recommended server behavior:

1. Browser sends `script_eval`.
2. Server validates `device_id` and forwards the payload unchanged.
3. Device sends structured result/event frames.
4. Server records those frames in history and broadcasts them to connected browsers.

This preserves the existing broker model and avoids inventing a second path.

### Phase 6: add browser authoring UX

The current React app has no editor surface. Add a small script console first, not a full IDE.

Recommended first browser feature set:

- text area or code editor,
- run button,
- clear output button,
- result panel,
- console/event log panel,
- device selector reuse from current app.

Possible minimal store additions:

```ts
type ScriptRun = {
  requestId: number
  code: string
  status: 'sent' | 'queued' | 'ok' | 'error' | 'timed_out'
  output: string
  error: string | null
}
```

Future enhancement:

- upgrade to CodeMirror after the transport is stable.

## Recommended file-by-file implementation plan

### Firmware

Read first:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/app_main.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt`

Create or modify:

1. `firmware/main/CMakeLists.txt`
2. `firmware/main/app_commands.h`
3. `firmware/main/app_commands.cpp`
4. `firmware/main/js_service.h`
5. `firmware/main/js_service.cpp`
6. `firmware/main/lain_js_bindings.h`
7. `firmware/main/lain_js_bindings.cpp`
8. `firmware/main/remote_client.cpp`
9. `firmware/main/remote_client.h`
10. `firmware/main/app_main.cpp`

### Server

Read first:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/main.go`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/hub.go`

Modify:

1. add new browser message parsing branch for `script_eval`
2. record script result/event frames in history
3. broadcast script frames to browsers

### Web

Read first:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/store.ts`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/app.tsx`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/styles.css`

Modify:

1. add script send function to the store
2. add result/event handling
3. add an editor/output panel to the UI

## API reference for the intern

### Current command names already in use

From `remote_client.cpp` and `app_main.cpp`:

- `show_message`
- `set_position`
- `set_mode`
- `set_station`
- `set_band`
- `show_reveal`

These should become **internal building blocks** of the JS DSL, not remain the final authoring surface.

### Recommended DSL-to-command mapping

| JS call | Internal action |
| --- | --- |
| `lain.radio.setMode("radio")` | enqueue `kSetMode` |
| `lain.radio.tune(42)` | enqueue `kSetPosition` |
| `lain.radio.band("FM")` | enqueue `kSetBand` |
| `lain.radio.defineStation(12, {...})` | enqueue `kDefineStation` |
| `lain.radio.reveal("text", 3000)` | enqueue `kShowReveal` |
| `lain.device.showMessage("hello")` | enqueue `kShowMessage` |

### Example browser transport helper

```ts
function sendScript(deviceId: string, code: string, timeoutMs = 100) {
  socket.send(JSON.stringify({
    type: 'script_eval',
    device_id: deviceId,
    request_id: nextRequestId++,
    filename: '<browser>',
    timeout_ms: timeoutMs,
    code,
  }))
}
```

## Testing and validation strategy

### Firmware validation

1. `idf.py build`
2. `idf.py -p /dev/ttyACM0 flash`
3. `idf.py -p /dev/ttyACM0 monitor`
4. verify:
   - JS service starts,
   - memory budget logs are sane,
   - script evaluation returns result frames,
   - app-task-owned state still updates correctly.

### Server validation

1. `go test ./...`
2. live websocket validation:
   - server accepts `script_eval`,
   - routes to device,
   - logs `script_result` back.

### Web validation

1. `npm run build`
2. manual dev run:
   - Vite page connects,
   - run script,
   - result/output panels update,
   - device display changes accordingly.

### Example end-to-end script for first validation

```js
lain.device.showMessage("hello wired")
lain.radio.setMode("radio")
lain.radio.band("WIRED")
lain.radio.tune(12)
lain.runtime.log("done")
```

Expected result:

- device shows message,
- device enters radio mode,
- tuned position changes,
- browser receives `script_console` with `done`,
- browser receives final `script_result.ok === true`.

## Risks, tradeoffs, and guardrails

### 1. Remote code execution risk

This feature is literally remote code execution on an embedded device. The current websocket design trusts the local environment, not authenticated users.

Recommendation:

- default remote script execution to disabled,
- enable only via USB console command or build flag,
- document this clearly.

### 2. State ownership risk

If JS native bindings mutate app state directly, subtle race conditions will follow.

Recommendation:

- make app-task ownership an explicit invariant,
- enforce it in code review.

### 3. Resource exhaustion risk

QuickJS can consume memory or CPU aggressively.

Recommendation:

- fixed arena size,
- per-eval deadline,
- queue length cap,
- reject oversized source payloads,
- report `timed_out` explicitly.

### 4. Transport mismatch risk

The 0074 README still documents `:8080`, while the current playbook and Vite proxy are using `:18080`. That operational mismatch is not the central design problem here, but it is real and should be corrected during implementation so script transport docs do not inherit the wrong port.

## Alternatives considered

### Alternative A: implement scripts as browser-side macros only

Rejected because the user explicitly wants the device to execute custom code. Browser-side macros do not provide on-device logic or future autonomy.

### Alternative B: add an HTTP script endpoint on the device

Rejected for 0074 because the device does not host the main browser UI. The external Go server is already the rendezvous point and should stay that way.

### Alternative C: let JS call C functions that mutate `AppContext` directly

Rejected because it breaks task ownership and creates concurrency bugs.

### Alternative D: ship full Node-like APIs into QuickJS

Rejected for v1 because the audit surface becomes too large. The value here is the DSL over device primitives, not a general-purpose embedded JS platform.

## Open questions

1. Should remote script execution be physically gated by console command?
2. Should scripts be allowed to persist station definitions into NVS?
3. Should the browser support named scripts, snippets, or only raw source at first?
4. Should `show_reveal` evolve into a richer multi-line primitive before the DSL is finalized?

## References

Primary current-state files:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/README.md`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/PLAYBOOK.md`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/app_main.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/main.go`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/server/hub.go`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/store.ts`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/web/src/app.tsx`

Reference implementation to adapt:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/README.md`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
