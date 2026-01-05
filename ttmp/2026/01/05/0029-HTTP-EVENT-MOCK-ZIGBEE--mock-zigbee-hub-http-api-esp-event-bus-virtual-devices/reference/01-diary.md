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
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: |-
        esp_console commands (wifi + hub pb/seed)
        Console-based validation (pb
    - Path: 0029-mock-zigbee-http-hub/sdkconfig.defaults
      Note: Console backend defaults and quiet logs
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
  - `hub> wifi set yolobolo bring3248camera`
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
  - `hub> wifi set yolobolo bring3248camera`
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
  - console: `wifi set yolobolo bring3248camera`, `wifi connect`, `hub pb on`, `hub seed`, `hub pb last`
  - host: `curl http://192.168.0.18/v1/health`, `curl http://192.168.0.18/v1/devices`, `POST /v1/devices/1/set`

### Technical details
- The WS protobuf stream is implemented but disabled by default; console `hub stream status` should still work and show `clients=0`.
