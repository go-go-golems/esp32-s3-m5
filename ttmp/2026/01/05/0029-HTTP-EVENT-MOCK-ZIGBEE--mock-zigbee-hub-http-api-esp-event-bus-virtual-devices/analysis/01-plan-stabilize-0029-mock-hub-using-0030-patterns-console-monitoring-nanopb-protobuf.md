---
Title: 'Plan: stabilize 0029 mock hub using 0030 patterns (console, monitoring, nanopb protobuf)'
Ticket: 0029-HTTP-EVENT-MOCK-ZIGBEE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - http
    - zigbee
    - backend
    - websocket
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0029-mock-zigbee-http-hub/main/app_main.c
      Note: Module boot order for 0029
    - Path: 0029-mock-zigbee-http-hub/main/hub_bus.c
      Note: Hub event loop
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: HTTP handlers + websocket client tracking + JSON broadcast
    - Path: 0029-mock-zigbee-http-hub/main/hub_types.h
      Note: Event IDs and payload structs (future protobuf schema source of truth)
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: esp_console WiFi REPL; console-backend pitfalls
    - Path: 0029-mock-zigbee-http-hub/main/wifi_sta.c
      Note: WiFi STA bring-up
    - Path: 0030-cardputer-console-eventbus/components/proto/CMakeLists.txt
      Note: Reference nanopb integration for ESP-IDF early expansion
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: 'Reference patterns: monitor queue/task and nanopb encode command'
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T10:12:20.979684385-05:00
WhatFor: ""
WhenToUse: ""
---


# Plan: stabilize 0029 mock hub using 0030 patterns (console, monitoring, nanopb protobuf)

## Goal (what “make it work” means)

`0029-mock-zigbee-http-hub` is already a good MVP: Wi‑Fi STA + HTTP API + an application `esp_event` bus + simulated devices + a WebSocket event stream. The work now is to turn it into a **reliable, debuggable, and extensible** baseline that new team members can:

- flash and use immediately,
- configure Wi‑Fi via `esp_console` without terminal weirdness,
- observe bus traffic without stalling the hub,
- and evolve toward a “Zigbee-shaped” coordinator architecture with a clean internal event taxonomy.

This analysis cross-references `0030-cardputer-console-eventbus`, which successfully validated:

- console ergonomics (`esp_console` commands + scripted tmux interaction),
- “don’t print from handlers” monitoring patterns,
- and nanopb/protobuf schema + build-time codegen + bounded embedded encoding.

The outcome of this plan is a sequence of small refactors/additions to 0029 that bring the same operational confidence as 0030, while preserving 0029’s headless HTTP-hub nature.

## Where the code is today (0029 map)

Firmware project:

- `0029-mock-zigbee-http-hub/main/app_main.c` — boots modules
- `0029-mock-zigbee-http-hub/main/wifi_sta.c` + `wifi_sta.h` — Wi‑Fi STA bring-up, NVS credentials, scanning
- `0029-mock-zigbee-http-hub/main/wifi_console.c` + `wifi_console.h` — `hub> wifi ...` REPL
- `0029-mock-zigbee-http-hub/main/hub_bus.c` + `hub_bus.h` — application event loop + command handlers
- `0029-mock-zigbee-http-hub/main/hub_http.c` + `hub_http.h` — HTTP API + WS endpoint + broadcasting
- `0029-mock-zigbee-http-hub/main/hub_registry.c` + `hub_registry.h` — in‑memory device DB
- `0029-mock-zigbee-http-hub/main/hub_sim.c` + `hub_sim.h` — simulated telemetry/state changes
- `0029-mock-zigbee-http-hub/main/hub_types.h` — event IDs + payload structs

“Reference implementation” patterns we want to carry over (0030 map):

- `0030-cardputer-console-eventbus/main/app_main.cpp` — `evt monitor ...` and `evt pb ...` patterns
- `0030-cardputer-console-eventbus/components/proto/CMakeLists.txt` — nanopb FetchContent + `CMAKE_BUILD_EARLY_EXPANSION` workaround
- `0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto` — schema structure + bounded fields
- `ttmp/2026/01/05/0030-.../playbook/01-playbook-esp-console-keyboard-ui-esp-event-nanopb-protobuf-0030.md` — operational “what to run” and failure modes

## Current runtime architecture (0029, as implemented)

### Event bus model

`0029-mock-zigbee-http-hub/main/hub_bus.c` defines:

- `ESP_EVENT_DEFINE_BASE(HUB_EVT);`
- A dedicated application event loop created with its own task:
  - `task_name = "hub_evt"`
  - `queue_size = CONFIG_TUTORIAL_0029_HUB_EVENT_QUEUE_SIZE`

So handlers run on the `hub_evt` task (this differs from 0030, where the UI task dispatches the loop).

### HTTP → bus commands, and replies

HTTP handlers follow the intended rule: parse/validate and post a `HUB_CMD_*` event.

Example pattern (simplified from `hub_http.c`):

```c
QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_status_t));
hub_cmd_scene_trigger_t cmd = {
  .hdr = {.req_id = ..., .reply_q = q},
  .scene_id = id,
};
esp_event_post_to(loop, HUB_EVT, HUB_CMD_SCENE_TRIGGER, &cmd, sizeof(cmd), 100ms);
xQueueReceive(q, &rep, 500ms);
```

The bus command handler (`hub_bus.c`) replies via `reply_status(...)` / `reply_device(...)` to the command’s `reply_q`.

This “RPC-over-event-bus” approach is useful for HTTP because it preserves the layering rule (HTTP doesn’t call registry directly) while still giving HTTP a synchronous response.

### WebSocket event stream

Bus handlers register an always-on stream publisher:

- `esp_event_handler_register_with(s_loop, HUB_EVT, ESP_EVENT_ANY_ID, &on_any_event_stream, NULL);`

`on_any_event_stream(...)` formats **one JSON line** per event and calls:

- `hub_http_events_broadcast_json(const char *json)` (in `hub_http.c`)

The broadcaster snapshots client FDs and uses `httpd_ws_send_data_async(...)` per client; it `malloc()`s a copy of the JSON payload for each client and frees it in a send callback.

### Wi‑Fi + console

Wi‑Fi uses the default system event loop:

- `esp_event_loop_create_default()` is called in `0029-mock-zigbee-http-hub/main/wifi_sta.c` during init

`wifi_console_start()` starts an `esp_console` REPL with prompt `hub> ` and exposes:

- `wifi status`
- `wifi scan [max]`
- `wifi set <ssid> <password> [save]`
- `wifi connect|disconnect|clear`

Credentials are persisted in NVS under namespace `wifi` (`ssid`/`pass`).

## What 0030 validated that 0029 should adopt

### 1) Operational validation loop (tmux-driven)

In 0030 we validated a full “flash + tmux + monitor + send commands + capture output” loop and confirmed that:

- `idf.py monitor` is good for logs, but interactive console input can be flaky depending on host/terminal.
- tmux-based scripted input is a reliable alternative for repeatable validations.

We should make 0029 validation similarly scriptable:

- start monitor in tmux,
- `send-keys` for `hub> wifi ...` commands,
- run `curl` for HTTP endpoints,
- optionally run a WS client to confirm stream behavior.

### 2) “Don’t print from handlers” monitoring

0030 added an `evt monitor on|off` that prints bus traffic to console without printing directly from the event handler. Instead:

- handler formats a short line and enqueues it to a queue (`xQueueSend(..., 0)`),
- a low-priority monitor task drains the queue and prints.

This pattern prevents handler stalls and reduces REPL prompt corruption.

0029 currently logs a lot from multiple tasks and also does WS work inside the bus handler. For stability/debuggability, we should:

- add a console monitor for hub events (toggable),
- and optionally decouple WS serialization/sending from the bus handler via an internal queue.

### 3) Protobuf/nanopb as “IDL + boundary format”

0030 implemented an embedded-only nanopb pipeline:

- `.proto` schema committed in-tree
- codegen at build time via `nanopb_generate_cpp(...)`
- bounded encoding into a fixed buffer

It also validated the ESP-IDF “early expansion” pitfall (`CMAKE_BUILD_EARLY_EXPANSION`) and the nanopb generator’s Python dependency (`protobuf` package required in the IDF python env).

0029’s WS stream currently emits JSON strings. This is fine for a first demo, but:

- it is CPU-heavy in the bus handler,
- it couples external observability to internal bus evolution,
- and it creates schema drift risk if we later add a TypeScript client or a host tool.

We should adopt the same “protobuf at the boundary” approach:

- keep internal bus payloads as fixed-size C structs (good for `esp_event`),
- add nanopb schema mirroring those payloads,
- stream **binary WS frames** (protobuf envelope) as an option (alongside JSON during transition).

## Key gaps/risks in 0029 (and why they matter)

### A) Console reliability (“I see prompt but can’t type”)

We’ve already observed this symptom in practice on related firmware:

- prompt appears, but `idf.py monitor` can’t reliably send input (`Writing to serial is timing out`).

This is not always a firmware bug; it can be host/monitor/terminal behavior. But we can reduce confusion by:

- making the console backend explicit and consistent (USB Serial/JTAG on S3),
- minimizing noisy printing while REPL is active (avoid concurrent `printf` storms),
- providing a tmux or `picocom` fallback in docs and scripts.

Where to adjust:

- `0029-mock-zigbee-http-hub/main/wifi_console.c` (`wifi_console_start`)
- `0029-mock-zigbee-http-hub/sdkconfig.defaults` (console backend)
- add a `scripts/validate_tmux.sh` style helper for deterministic validation

### B) WebSocket broadcaster is “malloc per event per client”

`hub_http_events_broadcast_json()` allocates a `malloc(len)` buffer for every client for every event. Under high event rates (telemetry), this can:

- fragment heap,
- introduce latency spikes in the `hub_evt` task (if called from there),
- and fail broadcasts on transient memory pressure.

It also doesn’t explicitly remove dead clients when `httpd_ws_get_fd_info` says they’re not WebSocket anymore.

Even if the hub still “works”, this makes debugging and demos flaky.

Where to adjust:

- `0029-mock-zigbee-http-hub/main/hub_http.c` (`hub_http_events_broadcast_json`, client bookkeeping)

### C) Bus handler does JSON formatting + WS broadcast inline

`on_any_event_stream(...)` runs on the bus loop task (`hub_evt`) and does:

1) JSON formatting (`snprintf` into a 512-byte buffer)
2) broadcast call that allocates and schedules async sends

This is correct in layering terms, but it ties bus throughput to “observer speed” and serialization overhead.

Better architecture (based on 0030’s monitor queue pattern):

- bus handler: map event into a small “stream record” struct and enqueue non-blocking
- stream task: serialize (JSON or protobuf) and send; track drop counts

Where to adjust:

- `0029-mock-zigbee-http-hub/main/hub_bus.c` (`on_any_event_stream`)
- new module: `hub_stream.c/.h` (internal queue + WS send)

### D) No “hub CLI” for bus/registry introspection

0029 has a Wi‑Fi console, but no direct commands to:

- list devices,
- inject commands without HTTP,
- toggle WS stream modes,
- show drop counters (bus queue / stream queue / WS send failures).

0030’s `evt` command is an extremely productive debug surface. We should introduce a similar (small) console surface in 0029.

## Proposed plan (incremental, low-risk)

This is written as a sequence of changes where each step ends with a concrete validation.

### Phase 0 — Baseline validation (no code changes)

Goal: confirm current 0029 works end-to-end on the target board before refactoring.

1) Flash and monitor:

```bash
source ~/esp/esp-idf-5.4.1/export.sh
idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor
```

2) In the `hub>` console:

```text
hub> wifi scan
hub> wifi set <ssid> <pass> save
hub> wifi connect
hub> wifi status
```

3) From host, hit HTTP endpoints (use the IP printed by `wifi_sta.c`):

```bash
curl -s http://<ip>/v1/health
curl -s http://<ip>/v1/devices
curl -s -X POST http://<ip>/v1/devices -d '{"type":"plug","name":"desk"}'
curl -s -X POST http://<ip>/v1/devices/1/interview
curl -s -X POST http://<ip>/v1/devices/1/set -d '{"on":true}'
```

4) Connect a WS client and confirm you receive JSON lines:

- endpoint: `ws://<ip>/v1/events/ws`

Exit criteria:

- no crashes
- at least one device add/set/report event shows up on the WS stream

If baseline fails, stop and fix that first before modularizing further.

### Phase 1 — Add hub console commands (0029 parity with 0030 ergonomics)

Goal: make 0029 controllable and observable without requiring HTTP tooling.

Add a new console command group, suggested name: `hub`.

Proposed verbs:

- `hub status` — show IP, device count, drop counters (bus/stream)
- `hub devices` — list device IDs + type/name/caps/on/level
- `hub monitor on|off|status` — print bus events to console via queue (0030 pattern)
- `hub post ...` (optional) — inject bus commands without HTTP for quick tests

Where to implement:

- new file: `0029-mock-zigbee-http-hub/main/hub_console.c` (modeled after `0030`’s `cmd_evt`)
- reuse existing `wifi_console_start()` REPL instance, but register multiple commands:
  - keep `wifi` command intact
  - add `hub` command

Validation:

- `hub> hub status`
- `hub> hub devices`
- `hub> hub monitor on` and observe event lines when HTTP actions occur

### Phase 2 — Make monitoring and WS streaming bounded and non-blocking

Goal: keep the hub bus task responsive even when observers are slow.

2A) Console monitor queue (low effort, immediate win)

Copy the 0030 technique:

- bus handler formats a short line and enqueues (`xQueueSend(..., 0)`)
- monitor task prints lines

This isolates the bus handler from `printf` latency and reduces REPL corruption.

2B) WS stream decoupling (recommended)

Introduce a “stream bridge” module that is the only part that talks to WS broadcasting.

Bus handler becomes:

```c
void on_any_event_stream(...) {
  hub_stream_record_t rec = map_event_to_record(id, data, ts_us);
  hub_stream_try_enqueue(&rec); // non-blocking, increments drops on overflow
}
```

Stream task becomes:

```c
for (;;) {
  hub_stream_record_t rec;
  xQueueReceive(q, &rec, portMAX_DELAY);
  // Option A: JSON
  // Option B: protobuf (Phase 3)
  hub_http_events_broadcast_*(...);
}
```

This makes it easy to:

- add drop counters,
- rate-limit or coalesce telemetry,
- switch encoding formats without touching bus core handlers.

Validation:

- hammer the hub (add multiple devices + let telemetry run)
- confirm bus command latency remains stable (HTTP requests keep responding)
- confirm drop counters behave (overflow increments, hub continues)

### Phase 3 — Add nanopb/protobuf schema for hub events (embedded side only)

Goal: eliminate schema drift and support a future typed client by streaming protobuf envelopes.

3A) Add a `.proto` schema that mirrors `hub_types.h`

Suggested location:

- `0029-mock-zigbee-http-hub/components/proto/defs/hub_bus.proto`

Schema shape (mirrors what worked in 0030):

- `enum EventId` with stable numeric values
- message per payload type
- `HubEvent` envelope:
  - `schema_version`
  - `EventId id`
  - `oneof payload`

Bounded fields:

- `string name` should have `(nanopb).max_length = ...`
- if we include repeated arrays, use `(nanopb).max_count = ...`

3B) Add nanopb integration as a component (reuse 0030’s exact build pattern)

Option 1 (fastest): copy the 0030 component pattern into 0029.

- `0029-mock-zigbee-http-hub/components/proto/CMakeLists.txt` modeled after:
  - `0030-cardputer-console-eventbus/components/proto/CMakeLists.txt`

Option 2 (better reuse across projects): create a shared repo-level component later (not required for 0029 stabilization).

Important: keep the ESP-IDF `CMAKE_BUILD_EARLY_EXPANSION` guard, as in 0030.

3C) Implement a binary WS endpoint that streams protobuf frames

Add a second endpoint:

- `GET /v1/events/ws.pb` (binary WS frames)

And keep the existing JSON endpoint for human debugging:

- `GET /v1/events/ws` (text WS frames)

Stream module chooses which encoding to use based on endpoint/client registration.

Validation:

- `hub> hub stream status` shows number of json/pb clients
- connect both endpoints and confirm:
  - JSON clients still work
  - protobuf clients receive binary frames that decode with the schema (host tool later)

### Phase 4 — Align docs and “known failure modes” (onboarding polish)

Goal: reduce future time lost on console/monitoring quirks.

Update:

- `0029-mock-zigbee-http-hub/README.md` with:
  - tmux validation steps (like we used successfully in 0030)
  - clear note about `idf.py monitor` input flakiness and alternatives
  - how to select console backend (USB Serial/JTAG) in menuconfig

Add a ticket playbook doc if needed (like 0030’s).

## Suggested task breakdown (for implementing this plan)

This is a proposed task list; it can be pasted into a ticket `tasks.md` when we start the implementation work.

1) Baseline validation and capture logs (flash, Wi‑Fi console, HTTP endpoints, WS stream).
2) Add `hub_console.c` with `hub status/devices/monitor`.
3) Add monitor queue + task (avoid printing in bus handler).
4) Add `hub_stream.c` bridge module + queue; move JSON serialization and WS send out of `hub_evt` task.
5) Add protobuf schema + nanopb build integration (`components/proto` + `.proto`).
6) Add protobuf WS endpoint + binary stream support (keep JSON endpoint).
7) Add drop counters + reporting (bus queue, stream queue, ws send failures).
8) Update docs + add a repeatable validation script (tmux).

## File/symbol cross-reference (so reviewers know where to look)

0029:

- `0029-mock-zigbee-http-hub/main/app_main.c` — boot order
- `0029-mock-zigbee-http-hub/main/wifi_console.c` — `wifi_console_start`, `cmd_wifi`
- `0029-mock-zigbee-http-hub/main/wifi_sta.c` — `hub_wifi_start`, `hub_wifi_scan`, NVS keys `wifi/ssid, wifi/pass`
- `0029-mock-zigbee-http-hub/main/hub_bus.c` — `hub_bus_start`, `on_cmd_device_*`, `on_any_event_stream`
- `0029-mock-zigbee-http-hub/main/hub_http.c` — `hub_http_events_broadcast_json`, WS client tracking, `events_ws_handler`
- `0029-mock-zigbee-http-hub/main/hub_types.h` — event IDs + payload structs to mirror in `.proto`

0030 patterns to reuse:

- `0030-cardputer-console-eventbus/main/app_main.cpp` — `monitor_enqueue_line`, `monitor_task`, `evt pb ...` encode path
- `0030-cardputer-console-eventbus/components/proto/CMakeLists.txt` — nanopb integration guard
- `0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto` — envelope + bounded fields style

## Appendix: API references (ESP-IDF modules touched by this plan)

- `esp_event`:
  - `esp_event_loop_create`, `esp_event_post_to`, `esp_event_handler_register_with`
- `esp_http_server` WebSocket:
  - `httpd_start`, `httpd_register_uri_handler`
  - `httpd_ws_send_data_async`, `httpd_ws_get_fd_info`
- `esp_console`:
  - `esp_console_cmd_register`, `esp_console_new_repl_usb_serial_jtag`, `esp_console_start_repl`
- nanopb:
  - `pb_encode.h`, `pb_get_encoded_size`, `pb_encode`
