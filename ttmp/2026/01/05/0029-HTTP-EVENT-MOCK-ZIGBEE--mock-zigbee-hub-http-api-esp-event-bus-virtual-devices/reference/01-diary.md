---
Title: Diary
Ticket: 0029-HTTP-EVENT-MOCK-ZIGBEE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - http
    - zigbee
    - backend
    - websocket
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto
      Note: Protobuf schema mirrored from hub_types.h
    - Path: 0029-mock-zigbee-http-hub/main/Kconfig.projbuild
      Note: Kconfig toggles for WS and console log noise
    - Path: 0029-mock-zigbee-http-hub/main/hub_bus.c
      Note: Hub event loop task and handlers
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: |-
        WS broadcaster and HTTP server wiring
        HTTP server + WS client list + route matching fixes
    - Path: 0029-mock-zigbee-http-hub/main/hub_pb.c
      Note: |-
        nanopb capture+encode implementation
        Build protobuf envelope for hub events
    - Path: 0029-mock-zigbee-http-hub/main/hub_stream.c
      Note: Queue+task bridge (protobuf WS stream)
    - Path: 0029-mock-zigbee-http-hub/main/ui/app.js
      Note: In-browser protobuf decoder + WS client
    - Path: 0029-mock-zigbee-http-hub/main/ui/index.html
      Note: Embedded web UI (served from /)
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: |-
        esp_console commands (wifi + hub pb/seed)
        Console-based validation (pb
    - Path: 0029-mock-zigbee-http-hub/sdkconfig.defaults
      Note: Console backend defaults and quiet logs
    - Path: ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/http_pb_hub.js
      Note: Host protobuf HTTP client
    - Path: ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/ws_hub_events.js
      Note: Host WS smoke client
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T10:49:37.935828904-05:00
WhatFor: ""
WhenToUse: ""
---




# Diary

## Goal

Capture the implementation work for ticket `0029-HTTP-EVENT-MOCK-ZIGBEE` with enough detail that someone else can:

- understand what was changed and why,
- reproduce the exact failures we saw,
- validate the firmware on real hardware,
- and continue the refactor sequence (Phase 1 → Phase 2 → Phase 3) without guessing.

## Step 1: Phase 1 — disable WS JSON + reduce console noise (stabilize “core hub”)

`0029` “didn’t work” in practice because the WebSocket JSON stream and concurrent logging made the system noisy and harder to interact with via `esp_console`. The first stabilization move was to **remove WS and JSON work from the hot path** and to keep the console prompt usable by lowering hub log verbosity once the REPL starts.

This step intentionally prioritizes: *Wi‑Fi console + HTTP API correctness* over observability features. We can reintroduce an event stream later once protobuf is in place.

**Commit (code):** 9b177e0 — "0029: disable WS JSON stream and quiet logs"

### What I did
- Added compile-time flags in `0029-mock-zigbee-http-hub/main/Kconfig.projbuild`:
  - `CONFIG_TUTORIAL_0029_ENABLE_WS_JSON` (default `n`) to fully gate JSON WS behavior.
  - `CONFIG_TUTORIAL_0029_QUIET_LOGS_WHILE_CONSOLE` (default `y`) to reduce noisy logs once REPL starts.
- Gated WebSocket JSON code in:
  - `0029-mock-zigbee-http-hub/main/hub_http.c` (endpoint registration + broadcaster becomes no-op when disabled).
  - `0029-mock-zigbee-http-hub/main/hub_bus.c` (removes handler registration when disabled).
- Reduced device-registry chatter:
  - `0029-mock-zigbee-http-hub/main/hub_registry.c` changes an `ESP_LOGI` to `ESP_LOGD`.
- After `esp_console_start_repl(...)`, reduced log levels:
  - `0029-mock-zigbee-http-hub/main/wifi_console.c` sets `hub_*` tags to `WARN`.
- Updated docs to reflect WS disabled-by-default:
  - `0029-mock-zigbee-http-hub/README.md`
  - `0029-mock-zigbee-http-hub/sdkconfig.defaults`

### Why
- The JSON WS stream did per-event/per-client allocation and extra CPU work, which made early debugging unreliable.
- Keeping the REPL prompt readable is a prerequisite for reliably configuring Wi‑Fi and iterating on HTTP behavior.

### What worked
- `idf.py -C 0029-mock-zigbee-http-hub build` succeeded after gating the WS code.
- `hub> wifi scan 5` worked reliably over `esp_console` when the console backend was correct (see next section).

### What didn't work
- Flash initially failed because the serial port was held open by a running monitor session:
  - Command: `idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash`
  - Error:
    - `Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable`
  - Fix: stop the monitor session (I killed the `tmux` monitor session) and re-run flash.
- The console backend was not consistently USB Serial/JTAG:
  - Symptom (boot log): `esp_console started over UART`
  - And `idf.py monitor` showed: `--- Warning: Writing to serial is timing out.`
  - Root cause: local `0029-mock-zigbee-http-hub/sdkconfig` had `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`, overriding `sdkconfig.defaults`.
  - Fix during validation: force a build with USB Serial/JTAG enabled (and UART disabled), and reflash.

### What I learned
- For Cardputer-style S3 boards, **USB Serial/JTAG must be the primary console backend** to keep `esp_console` interactive over `/dev/ttyACM0`.
- Even with a correct backend, `idf.py monitor` can be finicky for REPL input; tmux scripted `send-keys` is a reliable validation technique.

### What was tricky to build
- Ensuring WS JSON is truly “gone” from the hot path required gating both:
  - the HTTP endpoint registration and broadcaster, and
  - the bus-side handler registration (so the bus loop doesn’t format JSON at all).

### What warrants a second pair of eyes
- Confirm the chosen log-level reductions don’t hide important error information during normal hub bring-up (e.g., Wi‑Fi disconnect reasons).
- Confirm the default Kconfig choices align with the longer-term plan (WS JSON is transitional and should stay off).

### What should be done in the future
- Document in repo-level guidance that USB Serial/JTAG should be default for our S3 dev boards (user requested adding this to `AGENTS.md`).

### Code review instructions
- Start with:
  - `0029-mock-zigbee-http-hub/main/Kconfig.projbuild`
  - `0029-mock-zigbee-http-hub/main/hub_http.c`
  - `0029-mock-zigbee-http-hub/main/hub_bus.c`
  - `0029-mock-zigbee-http-hub/main/wifi_console.c`
- Validate:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub build`
  - Flash/monitor (Cardputer connected): `idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor`
  - In console: `wifi status`, `wifi scan 5`

### Technical details
- Working validation snippet used tmux:
  - Start monitor in tmux, then:
    - `wifi status`
    - `wifi scan 5`

## Step 2: Phase 2 — add nanopb schema + embedded encode path (still no WS)

Once the hub was quiet enough to interact with, the next step was to introduce protobuf/nanopb on the embedded side **without reintroducing WebSockets yet**. The goal was to validate:

- schema mirrors the existing C payload structs (`hub_types.h`),
- nanopb codegen is wired correctly under ESP-IDF,
- and we can encode real hub events into a bounded buffer and inspect the bytes via the console.

**Commit (code):** a389e64 — "0029: add nanopb schema and console protobuf dump"

### What I did
- Added a new component that owns schema + codegen:
  - `0029-mock-zigbee-http-hub/components/hub_proto/CMakeLists.txt`
  - `0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`
- Wired the component into the main build:
  - `0029-mock-zigbee-http-hub/main/CMakeLists.txt` adds `hub_proto` in `PRIV_REQUIRES`
- Added a minimal “capture last event and encode” module:
  - `0029-mock-zigbee-http-hub/main/hub_pb.c`
  - `0029-mock-zigbee-http-hub/main/hub_pb.h`
- Registered capture handler at boot:
  - `0029-mock-zigbee-http-hub/main/app_main.c` calls `hub_pb_register(hub_bus_get_loop())`
- Added new console commands:
  - `hub pb on|off|status|last` — enable capture + print hex dump of last protobuf envelope
  - `hub seed` — injects a few devices via the bus (no Wi‑Fi required) to generate deterministic event traffic for protobuf validation
  - Implemented in `0029-mock-zigbee-http-hub/main/wifi_console.c` by registering a second command (`hub`) alongside `wifi`.

### Why
- We want protobuf to become the IDL for event payloads and the eventual WS wire format.
- Encoding at the boundary (and keeping internal bus payloads as C structs) maintains performance and avoids heap allocation in the bus path.

### What worked
- Build succeeded and nanopb codegen ran during `idf.py build`.
- On-device validation (no Wi‑Fi required):
  - `hub> hub pb on`
  - `hub> hub seed`
  - `hub> hub pb last`
  - Output observed:
    - `len=25` followed by a hex dump

### What didn't work
- Attempting to connect to the provided Wi‑Fi credentials failed on this network during validation:
  - `hub> wifi set <ssid> <password>`
  - `hub> wifi connect`
  - Observed disconnect reason: `STA disconnected (reason=201) -> retry`
  - This is likely an environment issue (bad credentials / AP not reachable), not a firmware structural issue.

### What I learned
- For bring-up tests, **don’t couple protobuf validation to Wi‑Fi**; a local bus “seed” command gives a deterministic way to generate events.
- Nanopb codegen can be integrated in ESP-IDF reliably using the same `CMAKE_BUILD_EARLY_EXPANSION` pattern we validated in 0030.

### What was tricky to build
- Mapping `hub_types.h` payloads to protobuf envelopes while keeping everything bounded:
  - `name` strings must be bounded (nanopb max length).
  - floats encode fine, but don’t want to format them to strings (that was part of the JSON stream problem).

### What warrants a second pair of eyes
- Confirm numeric enum stability:
  - `hub_event_id_t` values in `hub_types.h` must stay aligned with `EventId` in `hub_events.proto`.
- Confirm message bounds are correct (`name` max length, etc.).

### What should be done in the future
- Implement Phase 3: remove JSON entirely and reintroduce WS as **protobuf binary frames**, with sending decoupled from the bus loop via a bounded queue.

### Code review instructions
- Start with:
  - `0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`
  - `0029-mock-zigbee-http-hub/components/hub_proto/CMakeLists.txt`
  - `0029-mock-zigbee-http-hub/main/hub_pb.c`
  - `0029-mock-zigbee-http-hub/main/wifi_console.c` (`hub` command)
- Validate:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub build`
  - Flash/monitor: `idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor`
  - In console: `hub pb on`, `hub seed`, `hub pb last`

### Technical details
- Console validation works reliably when run in tmux and commands are injected via `tmux send-keys`.

## Step 3: Phase 3 — protobuf WS stream architecture + HTTP stabilization (no JSON WS, decouple bus from IO)

Phase 3 brings the event stream back, but **not** as JSON: `/v1/events/ws` is now a protobuf-binary WebSocket (still disabled by default via `CONFIG_TUTORIAL_0029_ENABLE_WS_PB=n`). The architectural goal is that the hub event loop stays “cheap” and deterministic, so the expensive bits (protobuf encode + WS send + iterating clients) happen in a dedicated task, not inside the bus handler.

While validating HTTP bring-up, we also hit two issues that explained why “0029 doesn’t work” in practice: the default httpd task stack was too small for `cJSON_PrintUnformatted()` in `/v1/devices`, and ESP-IDF’s wildcard matcher didn’t match URIs like `/v1/devices/*/set` as expected. Both were fixed so the JSON HTTP API remains usable until we migrate it to protobuf.

**Commit (code):** 4af0531 — "0029: protobuf WS stream bridge + HTTP fixes"

### What I did
- Removed JSON WebSocket stream codepaths (and renamed the feature flag) so the bus loop no longer formats JSON:
  - `0029-mock-zigbee-http-hub/main/hub_bus.c`
  - `0029-mock-zigbee-http-hub/main/Kconfig.projbuild` (`CONFIG_TUTORIAL_0029_ENABLE_WS_PB`)
- Added a protobuf WS “bridge task” with a bounded queue:
  - bus handler copies payload into a queue item (cheap)
  - a dedicated task encodes nanopb + broadcasts binary WS frames
  - files: `0029-mock-zigbee-http-hub/main/hub_stream.c`, `0029-mock-zigbee-http-hub/main/hub_stream.h`
- Implemented protobuf WS broadcaster + client bookkeeping:
  - `0029-mock-zigbee-http-hub/main/hub_http.c`
  - `0029-mock-zigbee-http-hub/main/hub_http.h`
- Exposed a shared “build protobuf envelope from hub event” helper used by console tools and WS bridge:
  - `0029-mock-zigbee-http-hub/main/hub_pb.c`
  - `0029-mock-zigbee-http-hub/main/hub_pb.h`
- Added console observability for the stream bridge:
  - `hub stream status` prints `clients`, `drops`, `enc_fail`, `send_fail`
  - implemented in `0029-mock-zigbee-http-hub/main/wifi_console.c`
- Fixed `/v1/devices` crash:
  - increased httpd task stack to avoid `LoadStoreAlignment` crashes inside `cJSON_PrintUnformatted()`
  - change in `0029-mock-zigbee-http-hub/main/hub_http.c` (`cfg.stack_size = 8192`)
- Fixed HTTP route matching:
  - `/v1/devices/*/set` and `/v1/devices/*/interview` did not match reliably with ESP-IDF wildcard matching
  - registered `POST /v1/devices/*` once and dispatches based on suffix inside the handler
  - change in `0029-mock-zigbee-http-hub/main/hub_http.c`

### Why
- JSON WS was expensive and noisy; protobuf is the intended on-wire format and is bounded + schema-driven.
- Doing network I/O and encoding work inside an `esp_event` callback is a responsiveness hazard; the queue+task bridge keeps the bus loop predictable.
- `/v1/devices` and `/v1/devices/{id}/set` must work reliably so we can debug the hub via HTTP while WS is disabled.

### What worked
- Built successfully: `idf.py -C 0029-mock-zigbee-http-hub build`
- Flashed and interacted in tmux (USB Serial/JTAG console):
  - `hub pb on; hub seed; hub pb last` produced `len=...` and a hex dump
  - `hub stream status` prints counters (with WS disabled: `clients=0 ...`)
  - `wifi scan 5` shows nearby networks (including `yolobolo`)
- Connected to Wi‑Fi and validated the HTTP API from the host:
  - `hub> wifi set <ssid> <password>`
  - `hub> wifi connect` (device got `192.168.0.18`)
  - `curl http://192.168.0.18/v1/health` → `{"ok":true,...}`
  - `curl http://192.168.0.18/v1/devices` → `[]`
  - `POST /v1/devices` creates a device
  - `POST /v1/devices/1/set` now works (`{"ok":true}`)
  - `POST /v1/devices/1/interview` now works (`{"ok":true}`)

### What didn't work
- Serial/JTAG enumeration was flaky depending on which physical USB port the Cardputer was connected to:
  - `/dev/ttyACM0` sometimes disappeared and `esptool.py` failed with host-side I/O errors such as:
    - `termios.error: (5, 'Input/output error')`
    - `Could not open /dev/ttyACM0, the port is busy or doesn't exist`
  - Fix in practice: replug into a different USB port and retry.
- Before increasing httpd stack size, `GET /v1/devices` could crash the firmware:
  - `Guru Meditation Error: Core 0 panic'ed (LoadStoreAlignment)`
  - backtrace pointed to `devices_list_get` → `cJSON_PrintUnformatted`
- Before the route-dispatch fix, `POST /v1/devices/1/set` returned:
  - `Specified method is invalid for this resource`

### What I learned
- `esp_http_server` wildcard matching works well for “prefix-ish” routes like `/v1/devices/*`, but patterns like `/v1/devices/*/set` are not reliable; dispatching on suffix inside a single POST handler is a robust workaround.
- cJSON printing can be stack-hungry enough to crash the default httpd task stack; bumping `cfg.stack_size` is a pragmatic fix while we still serve JSON.

### What was tricky to build
- Keeping the “bus thread” clean required a strict boundary:
  - copy payloads into a bounded queue item
  - encode/send only in another task
- The queue payload union intentionally includes full command structs (including `hdr.reply_q`), but the protobuf builder must never dereference it; it only uses `hdr.req_id`.

### What warrants a second pair of eyes
- `hub_stream_payload_u` coverage and sizing:
  - confirm `payload_size_for_id()` matches the union members exactly,
  - confirm there are no future payload structs with pointers/variable data that would make “memcpy into union” unsafe.
- WS client list correctness when enabled:
  - confirm disconnect handling and refcount buffer lifetime are correct under churn.

### What should be done in the future
- Validate Phase 3 with WS protobuf enabled:
  - enable `CONFIG_TUTORIAL_0029_ENABLE_WS_PB=y`
  - connect a WS client and confirm binary frames decode as `hub_v1.HubEvent`
- Replace the JSON HTTP API with protobuf (no backwards compatibility required per ticket).

### Code review instructions
- Start with:
  - `0029-mock-zigbee-http-hub/main/hub_stream.c`
  - `0029-mock-zigbee-http-hub/main/hub_http.c`
  - `0029-mock-zigbee-http-hub/main/hub_pb.c`
- Validate:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub build`
  - `idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor`
  - console: `wifi set <ssid> <password>`, `wifi connect`, `hub pb on`, `hub seed`, `hub pb last`
  - host: `curl http://192.168.0.18/v1/health`, `curl http://192.168.0.18/v1/devices`, `POST /v1/devices/1/set`

### Technical details
- The WS protobuf stream is implemented but disabled by default; console `hub stream status` should still work and show `clients=0`.

## Step 4: Phase 4 — enable WS protobuf by default + debug WS client disconnects

After completing Phase 3, the next push is to actually **turn the protobuf WebSocket stream on** and validate it end-to-end. This step also addresses a common usability warning we saw in the logs: `httpd_uri: URI '/' not found` — which was simply because the firmware had no “home page” route yet.

Enabling WS uncovered a real integration issue: the WS handshake works, but clients disconnect quickly (and we aren’t seeing binary frames arrive on the host), even though the stream bridge starts and the event bus can generate traffic. This step captures the exact symptoms and the first mitigation attempts.

**Commit (code):** 13016a2 — "0029: enable protobuf WS stream by default"

### What I did
- Enabled WS protobuf by default:
  - `0029-mock-zigbee-http-hub/main/Kconfig.projbuild` changed `CONFIG_TUTORIAL_0029_ENABLE_WS_PB` default to `y`
  - `0029-mock-zigbee-http-hub/sdkconfig.defaults` set `CONFIG_TUTORIAL_0029_ENABLE_WS_PB=y`
  - `0029-mock-zigbee-http-hub/sdkconfig` set `CONFIG_TUTORIAL_0029_ENABLE_WS_PB=y` for local builds
- Added a minimal `/` route to avoid the “URI '/' not found” warning:
  - `0029-mock-zigbee-http-hub/main/hub_http.c` adds `index_get()` returning a plaintext endpoint list
- Ran on-device validation in tmux (USB Serial/JTAG console):
  - `idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor`
  - Verified Wi‑Fi autoconnect (from NVS), verified HTTP reachability:
    - `curl http://192.168.0.18/`
    - `curl http://192.168.0.18/v1/health`
- Tested WS client from the host using Node’s built-in WebSocket:
  - `node` script connects to `ws://192.168.0.18/v1/events/ws` and waits for frames
  - Also tried sending periodic text frames (`ws.send("ping")`) to keep the connection alive
- Began hardening the WS handler to not treat transient `EAGAIN` reads as fatal:
  - attempted to detect “socket would block” during `httpd_ws_recv_frame`
  - fixed compile error where I referenced a non-existent constant `ESP_ERR_HTTPD_SOCK_ERR` (ESP-IDF uses `ESP_FAIL` for ws recv failures)

### Why
- `CONFIG_TUTORIAL_0029_ENABLE_WS_PB` needs real end-to-end validation before we delete the remaining JSON HTTP API.
- The root route is used by humans; without it, browsers and curl tests are noisy and confusing.

### What worked
- `/dev/ttyACM0` flashing and USB Serial/JTAG console worked once the device was not physically disturbed and no other monitor session held the port.
- Firmware comes up, autoconnects to Wi‑Fi, and HTTP endpoints respond:
  - `curl http://192.168.0.18/` returns the endpoint list.
- WS handshake succeeds:
  - `hub> hub stream status` briefly shows `clients=1` after the host connects.

### What didn't work
- Serial contention is easy to trigger (and looks like “port missing”):
  - Error when a tmux session still had the port open:
    - `Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable`
  - Fix: `tmux kill-session -t hub-0029` (or close monitor) before flashing.
- WS client disconnects quickly and no frames are observed on the host:
  - Node client sees close code `1006` (abnormal close) after a few seconds.
  - Server logs show repeated recv/read warnings on the WS path:
    - `W (...) httpd_txrx: httpd_sock_err: error in recv : 11`
    - `W (...) httpd_ws: httpd_ws_recv_frame: Failed to receive the second byte`
  - `hub stream status` flips from `clients=1` back to `clients=0`, so the stream bridge stops enqueueing.
- Attempting to ignore transient WS recv errors initially failed to compile:
  - Error: `error: 'ESP_ERR_HTTPD_SOCK_ERR' undeclared`

### What I learned
- ESP-IDF’s WS receive path is sensitive to non-blocking reads (errno `11`/`EAGAIN`) and will log warnings if the handler calls `httpd_ws_recv_frame` when there isn’t a full header available yet.
- The “URI '/' not found” warning is expected when there is no index route; adding one removes noise without committing to a full web UI.

### What was tricky to build
- Validating WS requires coordinating three concurrent “clients”:
  - the serial monitor (tmux),
  - the Wi‑Fi/HTTP reachability tests (curl),
  - and a WS client that stays connected long enough to receive frames.
  Small timing issues (port bumps, reconnects) can make results look random.

### What warrants a second pair of eyes
- The WS receive handler logic in `events_ws_handler()`:
  - we want WS to be “read-only”, but we must not accidentally disconnect well-behaved clients due to transient recv timing.
- Client bookkeeping:
  - ensure `ws_client_add()`/`ws_client_remove()` reflect true connection state under error conditions.

### What should be done in the future
- Finish WS validation and check task `[8]`:
  - use a WS client that can stay connected and confirm we receive binary frames when hub events occur (`hub seed`, HTTP commands).
  - if needed, adjust `events_ws_handler()` to only parse frames when they are complete or to ignore certain recv errors without removing the client.

### Code review instructions
- Start with:
  - `0029-mock-zigbee-http-hub/main/hub_http.c` (`events_ws_handler`, `hub_http_events_broadcast_pb`, `index_get`)
  - `0029-mock-zigbee-http-hub/main/hub_stream.c` (enqueue gating on client count)
  - `0029-mock-zigbee-http-hub/main/wifi_console.c` (`hub stream status`)
- Validate:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub build`
  - `idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor`
  - console: `hub seed`, `hub stream status`
  - host: `curl http://192.168.0.18/`

### Technical details
- Key log lines for this phase:
  - `httpd_uri: URI '/' not found` (fixed by adding `/`)
  - `httpd_sock_err: error in recv : 11` and `httpd_ws_recv_frame: Failed to receive the second byte` (WS instability)

## Step 5: Phase 4 (completed) — WS stream validation + console visibility + HTTP routing cleanup

The goal of this step was to make the protobuf WS stream reliably usable during bring-up: **connect a WS client, generate hub events, and see protobuf binary frames arrive**, while keeping the `hub>` console responsive and providing accurate “stream status” introspection.

This also fixed a secondary issue observed during HTTP testing: `/v1/scenes/1/trigger` returned “Nothing matches the given URI” due to wildcard-matching limitations, even though the handler existed.

**Commit (code):** 13016a2 — "0029: enable protobuf WS stream by default"

### What I did
- WS handler + registration hardening in `0029-mock-zigbee-http-hub/main/hub_http.c`:
  - Set `.handle_ws_control_frames = false` so the HTTPD core handles PING/PONG/CLOSE internally (reduces noisy recv issues for idle clients).
  - Ensured the handler is “read-only” and just drains frames when invoked.
  - Fixed broadcaster behavior to **not aggressively drop clients** when `httpd_ws_get_fd_info(...)` reports `HTTPD_WS_CLIENT_HTTP` (handshake still in progress).
  - Replaced `atomic_size_t` client-count tracking with a simple cached value updated under the same mutex to avoid inconsistent visibility in practice.
- Fixed `/v1/scenes/*/trigger` routing to actually match:
  - Added a `scenes_post_subroute()` dispatcher and registered `HTTP_POST /v1/scenes/*` in `0029-mock-zigbee-http-hub/main/hub_http.c`.
- Fixed `hub stream status` “clients=0” misleading output path in `0029-mock-zigbee-http-hub/main/wifi_console.c`:
  - Print client count via a stable integer type (`uint32_t`) (and then ultimately fixed the underlying count logic as described above).
- Validation runs were done in tmux to avoid serial lock issues:
  - `tmux kill-session -t hub-0029 || true`
  - `tmux new-session -d -s hub-0029 "source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor"`

### Why
- The protobuf WS stream is the new “observability plane” and must be stable before we delete JSON/WS legacy paths.
- The console must be trusted during bring-up; if `hub stream status` lies, it makes debugging much slower.
- Fixing scene route matching removes a class of confusing 404s during API testing.

### What worked
- End-to-end WS stream validation from the host:
  - `node` WebSocket client connected to `ws://192.168.0.18/v1/events/ws` and received binary protobuf frames when events were generated.
  - Verified multi-client: two WS clients both received events in parallel.
- HTTP scene trigger routing now matches and returns a semantic response (instead of a wildcard-404):
  - `curl -sS -X POST http://192.168.0.18/v1/scenes/1/trigger`
  - Result: `{"ok":true}`

### What didn't work
- A build failed after introducing the new scenes subroute due to a missing forward declaration:
  - Command: `idf.py -C 0029-mock-zigbee-http-hub build`
  - Error (excerpt):
    - `hub_http.c:564:16: error: implicit declaration of function 'scene_trigger_post'`
    - `hub_http.c:571:18: error: static declaration of 'scene_trigger_post' follows non-static declaration`
  - Fix: added a prototype `static esp_err_t scene_trigger_post(httpd_req_t *req);` near the top of `hub_http.c`.
- `hub stream status` showed `clients=0` even while WS frames were clearly being delivered.
  - Root cause: the `atomic_size_t` tracking approach did not behave reliably for our usage; replaced with a cached count updated under the same mutex as the client list.

### What I learned
- `httpd_ws_get_fd_info()` can report `HTTPD_WS_CLIENT_HTTP` transiently; treating that as “disconnect” is too aggressive.
- In embedded bring-up, correctness of diagnostics (console `status` commands) is as important as feature correctness.

### What was tricky to build
- Coordinating timing between:
  - WS handshake completion,
  - event generation (seed/HTTP commands),
  - and the broadcaster’s interpretation of connection state (`httpd_ws_get_fd_info`).

### What warrants a second pair of eyes
- WS client lifecycle and edge cases:
  - confirm we don’t leak client fds in `hub_http.c` under churn (connect/close storms),
  - confirm the broadcaster’s behavior under slow clients (async queue growth).

### What should be done in the future
- Add explicit backpressure metrics per-client (if we keep async WS) or drop-policy tuning if the event rate increases.
- Continue Phase 4 follow-ups: move remaining JSON HTTP payloads to protobuf once the on-device schema is stable.

### Code review instructions
- Start with:
  - `0029-mock-zigbee-http-hub/main/hub_http.c` (WS registration, broadcaster, scene routing)
  - `0029-mock-zigbee-http-hub/main/wifi_console.c` (`hub stream status`)
- Validate on hardware:
  - Flash/monitor in tmux as described above.
  - Host checks:
    - `curl http://192.168.0.18/`
    - `curl http://192.168.0.18/v1/health`
    - `node -e 'const ws=new WebSocket(\"ws://192.168.0.18/v1/events/ws\"); ws.binaryType=\"arraybuffer\"; ws.onmessage=e=>console.log(Buffer.from(e.data).length);'`
  - Console checks:
    - `hub seed`
    - `hub stream status`

### Technical details
- The WS path is now intended to be “server push only”; clients don’t need to send data to receive events.

## Step 6: Phase 4–5 — embedded web UI + protobuf-only HTTP API (kill JSON)

This step makes the demo feel “alive” immediately from a browser: `/` now serves a small embedded UI that connects to `/v1/events/ws` and **decodes the protobuf frames client-side**. It also removes the last major JSON surface area by migrating the HTTP API to protobuf requests/responses (`application/x-protobuf`) so we can delete `cJSON` and the `json` component dependency.

The key design choice for the UI is to **avoid a JS protobuf runtime** for now: the browser ships with a tiny hand-written proto3 decoder for `hub.v1.HubEvent` (varints + length-delimited submessages + float32), which is sufficient for bring-up and can later be replaced by generated TypeScript.

**Commit (code):** N/A (will be filled once committed)

### What I did
- Embedded UI served from firmware:
  - Added `0029-mock-zigbee-http-hub/main/ui/index.html` and `0029-mock-zigbee-http-hub/main/ui/app.js`.
  - Embedded both files via `EMBED_FILES` in `0029-mock-zigbee-http-hub/main/CMakeLists.txt` and served them from `/` and `/app.js`.
  - Added `POST /v1/debug/seed` (plain text) so the UI can generate traffic without using the serial console.
- Fixed HTTPD URI handler exhaustion:
  - Increased `cfg.max_uri_handlers` because adding UI routes pushed the server past the default handler slot limit.
  - Root symptom in logs when the limit was hit: `httpd_register_uri_handler: no slots left for registering handler`.
- Migrated HTTP API from JSON to protobuf-only:
  - Removed all `cJSON` usage from `0029-mock-zigbee-http-hub/main/hub_http.c`.
  - `GET /v1/devices` now returns protobuf `hub.v1.DeviceList`.
  - `POST /v1/devices` expects protobuf `hub.v1.CmdDeviceAdd` and returns protobuf `hub.v1.Device`.
  - `POST /v1/devices/{id}/set` expects protobuf `hub.v1.CmdDeviceSet` and returns protobuf `hub.v1.ReplyStatus`.
  - Similar protobuf replies for interview/scene trigger.
  - Added `ReplyStatus` + `DeviceList` messages in `0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`.
  - Removed the `json` component dependency from `0029-mock-zigbee-http-hub/main/CMakeLists.txt`.
- Added host-side scripts into the ticket workspace for repeatable validation:
  - `ttmp/.../scripts/ws_hub_events.js` (WS stream smoke)
  - `ttmp/.../scripts/http_pb_hub.js` (protobuf HTTP client)

### Why
- UI makes the demo approachable: no need to watch logs to confirm the event bus is alive.
- Protobuf-only HTTP eliminates the JSON overhead (formatting, heap churn, bigger stacks) and aligns with the “protobuf everywhere” direction (WS + HTTP + future TS).

### What worked
- Firmware serves UI and JS:
  - `curl -D - http://192.168.0.18/` → `Content-Type: text/html`
  - `curl -D - http://192.168.0.18/app.js` → `Content-Type: application/javascript`
- WS stream works from browser/host when events exist:
  - `node .../scripts/ws_hub_events.js --host 192.168.0.18 --seed` receives binary frames.
- Protobuf HTTP endpoints work end-to-end:
  - `node .../scripts/http_pb_hub.js --host 192.168.0.18 add ...`
  - `node .../scripts/http_pb_hub.js --host 192.168.0.18 set ...`
  - `node .../scripts/http_pb_hub.js --host 192.168.0.18 list`

### What didn't work
- Initially the UI caused `/v1/events/ws` to 404 because the WS handler failed to register:
  - Log: `httpd_register_uri_handler: no slots left for registering handler`
  - Fix: `cfg.max_uri_handlers = 16`.
- The old JSON curl examples now fail (expected):
  - `POST /v1/devices` with JSON now returns `400 bad protobuf`.

### What I learned
- ESP-IDF’s default `max_uri_handlers` can be surprisingly tight once you start adding UI routes and a WS endpoint.
- A small hand-written protobuf decoder is a practical bridge for a bring-up UI (as long as we document the schema coupling clearly).

### What was tricky to build
- Keeping the decoder strict-but-forgiving:
  - ignore unknown fields (future-proofing)
  - decode only the fields we care about (bring-up)
  - avoid allocations in JS hot path

### What warrants a second pair of eyes
- Schema drift risk:
  - ensure `ui/app.js` decoder stays aligned with `hub_events.proto` as fields evolve.
- HTTP protobuf ergonomics:
  - confirm we like `CmdDeviceAdd/CmdDeviceSet` as HTTP request messages (vs introducing dedicated HTTP request/response messages).

### What should be done in the future
- Replace the hand-written browser decoder with generated TS types once the frontend project starts.
- Consider adding a tiny “control panel” in the UI that can POST protobuf commands (add/set) directly.

### Code review instructions
- Start with:
  - `0029-mock-zigbee-http-hub/main/ui/app.js`
  - `0029-mock-zigbee-http-hub/main/hub_http.c`
  - `0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`
- Validate on hardware:
  - `tmux new-session -d -s hub-0029 "source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor"`
  - open `http://192.168.0.18/` and click `Connect WS`, then `Seed Devices`.
  - run scripts:
    - `node ttmp/.../scripts/ws_hub_events.js --host 192.168.0.18 --seed`
    - `node ttmp/.../scripts/http_pb_hub.js --host 192.168.0.18 list`

### Technical details
- HTTP protobuf content-type: `application/x-protobuf`
- WS frames: protobuf-encoded `hub.v1.HubEvent` as binary frames
