---
Title: Diary
Ticket: 0030-CARDPUTER-CONSOLE-EVENTBUS
Status: active
Topics:
    - cardputer
    - keyboard
    - console
    - esp-idf
    - esp32s3
    - display
    - esp-event
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0030-cardputer-console-eventbus/components/proto/CMakeLists.txt
      Note: Codegen wiring introduced in Step 4
    - Path: 0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto
      Note: Schema introduced in Step 4
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Implementation work recorded in this diary.
    - Path: 0030-cardputer-console-eventbus/partitions.csv
      Note: Partition table fix (line 5 size field).
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T07:49:04.828361218-05:00
WhatFor: ""
WhenToUse: ""
---



# Diary

## Goal

Capture the implementation work for ticket `0030-CARDPUTER-CONSOLE-EVENTBUS`, including the code locations, commands run, failures encountered, and how to validate the demo on Cardputer.

## Step 1: Create ticket and survey existing examples

This ticket is meant to be a “small but complete” onboarding demo: keyboard input, a console REPL, and internal pub/sub using `esp_event`, with all received events visible on screen. The target audience is a new developer joining the team who needs to understand how our Cardputer demos are structured and where to find related patterns.

I based the approach on the existing `0028` demo (keyboard + `esp_event` + on-screen event log) and layered in the `esp_console` REPL style used in `0020` so we can post events through the console as well.

### What I did
- Created the ticket and initial docs with `docmgr`.
- Reviewed patterns in:
  - `0028-cardputer-esp-event-demo/main/app_main.cpp`
  - `0020-cardputer-ble-keyboard-host/main/bt_console.c`
  - `components/cardputer_kb/*`

### Why
- Reuse known-good Cardputer keyboard scanning + UI ownership patterns.
- Keep the demo minimal (single-file firmware logic) but realistic (multiple tasks producing events).

### What worked
- `0028` already demonstrates the “UI task dispatches esp_event loop” technique that avoids multi-task display contention.

### What didn't work
- N/A

### What I learned
- The `cardputer_kb` semantic actions are `NavUp/NavDown/NavLeft/NavRight/Back/...`; raw keys like `Del` are handled as key events (not actions).

### What was tricky to build
- N/A (survey only)

### What warrants a second pair of eyes
- N/A (survey only)

### What should be done in the future
- N/A

### Code review instructions
- Start with the onboarding analysis doc and then follow the “related examples” list it contains.

## Step 2: Implement demo firmware project (`0030-cardputer-console-eventbus`)

Implemented a new ESP-IDF firmware project that posts keyboard edges and console commands into a user `esp_event` loop, then renders received events on screen. The event loop is created with `task_name=NULL` and is dispatched via `esp_event_loop_run()` inside the UI loop, keeping M5GFX display ownership in one place.

**Commit (code):** 0a0ea42 — "0030: cardputer console esp_event bus demo"

### What I did
- Added new project: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus`
- Implemented:
  - `keyboard_task` → posts `BUS_EVT_KB_KEY` and `BUS_EVT_KB_ACTION`
  - `console_start` → starts an `esp_console` REPL and registers `evt ...` command to post/clear events
  - `demo_event_handler` → appends formatted event lines to `UiState.lines`
  - UI loop → calls `esp_event_loop_run()` then renders the log using M5GFX canvas
- Built the firmware:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0030-cardputer-console-eventbus set-target esp32s3 && idf.py -C 0030-cardputer-console-eventbus build`

### Why
- Provide a “minimal spine” example that mirrors our event-driven approach, but is small enough to grok in one sitting.

### What worked
- Build succeeds after fixing the early scaffolding issues (see below).

### What didn't work
- First build failed due to a malformed partition table CSV:
  - Command: `idf.py -C 0030-cardputer-console-eventbus build`
  - Error: `Error at line 5: Size field can't be empty`
  - Fix: copied the known-good `partitions.csv` format used by `0028`.
- First compile failed due to incorrect assumptions about `cardputer_kb` APIs:
  - Error: `cannot convert 'uint8_t*' to 'int*'` for `xy_from_keynum(...)`
  - Error: `Action::Up` / `Action::Down` / `Action::Pause` not found (correct ones are `NavUp/NavDown/...`)
  - Error: `down_keynums` doesn’t exist (use `ScanSnapshot.pressed_keynums`)
  - Fix: align implementation with the patterns in `0028-cardputer-esp-event-demo/main/app_main.cpp`.

### What I learned
- When building new Cardputer demos, the fastest path is to copy the working API usage patterns from `0028` rather than re-deriving them.

### What was tricky to build
- Balancing “single-file demo” simplicity with correct task ownership:
  - producers run in their own tasks
  - handlers run in the UI task via `esp_event_loop_run`
  - only the UI task draws to the display

### What warrants a second pair of eyes
- Confirm the console REPL backend selection and terminal behavior on different host setups (USB Serial/JTAG vs UART) is not confusing for new developers.

### What should be done in the future
- Add a short troubleshooting snippet to the project README showing `picocom`/`minicom` commands for interactive console use if `idf.py monitor` input is unreliable.

### Code review instructions
- Start at `0030-cardputer-console-eventbus/main/app_main.cpp`:
  - `CARDPUTER_BUS_EVENT` + `BUS_EVT_*` IDs
  - `keyboard_task`, `console_start`, `cmd_evt`
  - `demo_event_handler` and UI render loop
- Validate:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0030-cardputer-console-eventbus build`
  - Flash/monitor (Cardputer connected): `idf.py -C 0030-cardputer-console-eventbus flash monitor`

### Technical details
- Console prompt: `bus> `
- Console commands:
  - `evt post <text>`
  - `evt spam <n>`
  - `evt clear`
  - `evt status`

## Step 3: Add console bus monitor + extract reusable architecture pattern

Added a `evt monitor on|off` control to mirror bus traffic to the console. This is useful when the on-screen log is too limited or when debugging queue pressure; it also helps with “headless” runs where the screen isn’t visible. The implementation avoids printing directly in the event handler by enqueueing formatted lines to a small queue drained by a low-priority task.

In parallel, wrote a design/pattern document describing how to turn this style of firmware into reusable C modules or C++ classes (RAII wrappers), and what parts should become generic components for other firmwares.

### What I did
- Added a monitor queue + task and an `evt monitor on|off|status` verb in `0030-cardputer-console-eventbus/main/app_main.cpp`.
- Updated `0030-cardputer-console-eventbus/README.md` with the new command.
- Added the pattern doc: `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/02-pattern-esp-event-centric-architecture-c-and-c.md`

### Why
- Provide a second observation channel for event-driven firmware: screen + console.
- Capture a reusable architecture (“event bus is the spine”) for onboarding and future refactors into components.

### What worked
- `idf.py -C 0030-cardputer-console-eventbus build` succeeds after changes.

### What didn't work
- N/A

### What I learned
- Printing from arbitrary tasks while a linenoise REPL is running can garble the prompt; a monitor is still worth it, but should be togglable and use a queue to keep handlers non-blocking.

### What was tricky to build
- Ensuring the event handler stays fast: it now formats a short line and enqueues it with `xQueueSend(..., 0)` so it never blocks UI dispatch.

### What warrants a second pair of eyes
- Confirm the console monitor formatting for `BUS_EVT_CONSOLE_POST` is correct and that the monitor queue size/priority are appropriate under event storms.

### What should be done in the future
- If we turn this into a reusable component, make the monitor formatting pluggable (callback or per-event formatter table).

### Code review instructions
- Review `0030-cardputer-console-eventbus/main/app_main.cpp` for:
  - `monitor_task`, `monitor_enqueue_line`
  - `cmd_evt` `monitor` verb
  - event handler enqueue points
- Read the architecture writeup:
  - `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/02-pattern-esp-event-centric-architecture-c-and-c.md`

## Step 4: Add nanopb + protobuf schema and encode path (embedded-only)

Added a nanopb-backed protobuf schema for the 0030 bus and wired codegen + encoding into the firmware. The event bus remains fixed-size C structs for `esp_event`, and protobuf is used only as a boundary/serialization format (for now, validated by printing an encoded hex dump on the console).

This step is meant to “prove the toolchain”: `.proto` file → generated `.pb.c/.h` → encode an envelope from a real bus event with bounded memory usage.

**Commit (code):** 35f8f08 — "0030: add nanopb protobuf event encoding"

### What I did
- Added a new ESP-IDF component `proto`:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/components/proto/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto`
- Integrated nanopb via `FetchContent` with an ESP-IDF-safe `CMAKE_BUILD_EARLY_EXPANSION` guard and `find_package(Nanopb)` so `nanopb_generate_cpp(...)` runs at configure/build time.
- Updated `main` to `REQUIRES proto` and added an embedded-only encode path:
  - `evt pb on|off|status|last` captures the last bus event, encodes it via nanopb, and prints hex + length.
- Updated the firmware README with the new console verbs.
- Built to confirm generator + firmware link end-to-end:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0030-cardputer-console-eventbus build`

### Why
- Use protobuf as an IDL (“single source of truth”) for event payload shapes while keeping internal bus payloads fast and POD-friendly.
- Keep the current iteration embedded-only; WebSocket/TypeScript decoding can be layered later once the schema + encoder are stable.

### What worked
- `nanopb_generate_cpp(...)` produces `demo_bus.pb.c/.h` during the build and the firmware links cleanly.
- Encoding is bounded and deterministic (stack buffer + `pb_get_encoded_size` check).

### What didn't work
- Nanopb generator can fail if the Python protobuf runtime is missing in the Python env used by the generator plugin:
  - Error: `ModuleNotFoundError: No module named 'google'`
  - Fix (example for IDF 5.4 env): `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/bin/python -m pip install protobuf`

### What I learned
- ESP-IDF component “early expansion” can break naïve `FetchContent` usage; nanopb integration needs to be guarded with `if(NOT CMAKE_BUILD_EARLY_EXPANSION)` (see `espressif/esp-idf#15693`).

### What was tricky to build
- Mapping `BUS_EVT_*` IDs and fixed-size structs to a protobuf `oneof` envelope without introducing heap allocation or blocking inside event handlers.
- Keeping the console-friendly “pb last” encoding path robust (buffer sizing, `pb_encode` error handling).

### What warrants a second pair of eyes
- Confirm the chosen bounds in `demo_bus.proto` (e.g. `(nanopb).max_length`) match real-world usage and won’t silently truncate important logs.
- Confirm the buffer sizing strategy for encoding (`uint8_t buf[256]`) is acceptable, or decide on a shared max payload constant for all firmwares.

### What should be done in the future
- Add a dedicated “bus→protobuf bridge” module (and optionally WebSocket binary frames) rather than encoding inside the demo handler path.
- Add decode-side tooling (host/TS) in a separate ticket once schema stability is acceptable.

### Code review instructions
- Start at `0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto` (schema + bounds).
- Then review `0030-cardputer-console-eventbus/components/proto/CMakeLists.txt` for the IDF-safe nanopb integration.
- Finally review `0030-cardputer-console-eventbus/main/app_main.cpp`:
  - `s_pb_capture_enabled`, `s_pb_last`, `evt pb ...` verb, and the encoding path.
- Validate:
  - Build: `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0030-cardputer-console-eventbus build`
  - Run (console): `evt pb on` then generate events (keyboard or `evt post hi`) then `evt pb last`

## Related

- Onboarding analysis: `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/01-onboarding-how-the-demo-fits-together.md`
