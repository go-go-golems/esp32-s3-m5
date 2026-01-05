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
      Note: WS broadcaster and HTTP server wiring
    - Path: 0029-mock-zigbee-http-hub/main/hub_pb.c
      Note: nanopb capture+encode implementation
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: esp_console commands (wifi + hub pb/seed)
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

## Step 3: Phase 3 (in progress) — switch WS endpoint to protobuf binary frames (no JSON, no backwards-compat)

This step is in progress. The intended end state is:

- keep HTTP API intact,
- implement `/v1/events/ws` as **protobuf** binary frames only,
- remove all JSON serialization codepaths,
- and ensure WS sending does not run inside the hub event loop task (queue + drops).

### What I did (so far)
- Began refactoring `hub_http.*` toward protobuf WS broadcast and removing JSON references.
- Began refactoring Kconfig from “WS JSON” to “WS protobuf”.

### What didn't work / current state
- The Phase 3 refactor is not finished yet; there are uncommitted changes in:
  - `0029-mock-zigbee-http-hub/main/hub_http.c`
  - `0029-mock-zigbee-http-hub/main/hub_http.h`
  - `0029-mock-zigbee-http-hub/main/hub_bus.c`
  - `0029-mock-zigbee-http-hub/main/Kconfig.projbuild`
  - `0029-mock-zigbee-http-hub/sdkconfig.defaults`

### What should be done in the future
- Finish Phase 3 by introducing a dedicated “stream bridge” task/queue (like 0030’s monitor queue pattern).
- Add `hub_pb_build_event(...)` (or equivalent) so WS publishing can be event-driven without re-encoding JSON.

## Technical details (commands and exact outputs worth preserving)

- Serial contention failure:
  - `Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable`
- `idf.py monitor` console warning (host-side):
  - `--- Warning: Writing to serial is timing out. Please make sure that your application supports an interactive console ...`
- Verified console scanning output example:
  - `found 5 networks (showing up to 5): ...`
- Verified protobuf dump example:
  - `len=25` plus hex dump after `hub pb on; hub seed; hub pb last`

## Related

- Design doc: `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/design-doc/01-mock-zigbee-hub-over-http-event-driven-architecture.md`
- Stabilization plan: `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/analysis/01-plan-stabilize-0029-mock-hub-using-0030-patterns-console-monitoring-nanopb-protobuf.md`
