---
Title: 'Onboarding: how the demo fits together'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0020-cardputer-ble-keyboard-host/main/bt_console.c
      Note: Reference pattern for esp_console REPL registration/start.
    - Path: 0028-cardputer-esp-event-demo/main/app_main.cpp
      Note: Baseline keyboard+esp_event+UI log demo that inspired 0030.
    - Path: 0030-cardputer-console-eventbus/README.md
      Note: Build and console usage instructions.
    - Path: 0030-cardputer-console-eventbus/main/Kconfig.projbuild
      Note: Project tunables for queue sizes
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: 'Single-file demo: esp_event loop'
    - Path: 0030-cardputer-console-eventbus/sdkconfig.defaults
      Note: Cardputer defaults and preferred console transport.
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T07:49:03.503841749-05:00
WhatFor: ""
WhenToUse: ""
---


# Onboarding: how the demo fits together

This document is the “start here” map for the Cardputer demo that combines **keyboard input**, **`esp_console`**, and an **`esp_event` bus**, while rendering an on-screen event log.

## Where the code lives

- Firmware project: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus`
  - `0030-cardputer-console-eventbus/main/app_main.cpp` — demo implementation (event bus + UI + keyboard + console)
  - `0030-cardputer-console-eventbus/main/Kconfig.projbuild` — tunables (queue sizes, periods, FPS)
  - `0030-cardputer-console-eventbus/sdkconfig.defaults` — Cardputer defaults + console transport (prefers USB Serial/JTAG)
  - `0030-cardputer-console-eventbus/README.md` — build + usage

## What this demo is trying to teach

1. **Event-driven core**: everything interesting inside the firmware is a typed event posted into a loop.
2. **Multiple producers**: keyboard task, console REPL task, and background tasks all post into the same bus.
3. **Single display owner**: UI is rendered from one task (avoids “two tasks drawing” bugs).

## Architecture (high level)

**Threads/tasks**

- **UI task** (`app_main` loop)
  - calls `esp_event_loop_run()` to dispatch pending events
  - renders the UI (M5GFX canvas) showing the rolling event log
- **Keyboard task** (`keyboard_task`)
  - scans Cardputer keyboard matrix via `cardputer_kb::MatrixScanner`
  - posts events into the bus (`esp_event_post_to`)
- **Console REPL task** (created by `esp_console_new_repl_*`)
  - parses `evt ...` commands
  - posts events into the bus (`esp_event_post_to`)
- **Background tasks**
  - heartbeat task posts memory stats
  - N random producer tasks post random values

**Dataflow**

1. Producer task constructs an event payload struct.
2. Producer calls `esp_event_post_to(s_loop, CARDPUTER_BUS_EVENT, <id>, &payload, sizeof(payload), ...)`.
3. UI task calls `esp_event_loop_run(s_loop, ...)`, which invokes handlers.
4. `demo_event_handler()` formats a log line and appends to `UiState.lines`.
5. UI renders the lines to the screen.

## Event bus design

- Event base: `CARDPUTER_BUS_EVENT`
- Loop handle: `s_loop` created with `task_name = NULL` so the UI task “owns” dispatch via `esp_event_loop_run()`.

**Event IDs and payload types (see `app_main.cpp`)**

- `BUS_EVT_KB_KEY` → `demo_kb_key_event_t`
- `BUS_EVT_KB_ACTION` → `demo_kb_action_event_t` (semantic nav/back actions)
- `BUS_EVT_CONSOLE_POST` → `demo_console_post_event_t`
- `BUS_EVT_CONSOLE_CLEAR` → no payload
- `BUS_EVT_RAND` → `demo_rand_event_t`
- `BUS_EVT_HEARTBEAT` → `demo_heartbeat_event_t`

## UI behavior / controls

- On-screen log shows recent events (keyboard, console, background tasks).
- Keyboard controls:
  - `Fn+W/S` (NavUp/NavDown bindings) scroll log
  - `Fn+1` (Back binding) jump to tail
  - `Del` clears the log
  - `Fn+Space` toggles pause/resume appending
- Console controls:
  - `evt post <text>` adds a `[con] ...` entry
  - `evt spam <n>` posts N events quickly
  - `evt clear` clears the on-screen log

## Related examples in this repo (recommended reading order)

- `0028-cardputer-esp-event-demo/main/app_main.cpp` — baseline: keyboard + `esp_event` + on-screen log (no console).
- `0020-cardputer-ble-keyboard-host/main/bt_console.c` — `esp_console` REPL pattern and command registration.
- `components/cardputer_kb/README.md` and `components/cardputer_kb/TUTORIAL.md` — keyboard matrix scanning + bindings.
- `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` — M5GFX patterns (display bring-up, canvas usage).
- `0029-mock-zigbee-http-hub/main/*` — a larger app that uses an internal event bus as the “spine” of the firmware.

## Known sharp edges / troubleshooting notes

- `idf.py monitor` is not a great interactive terminal for linenoise-based REPLs; if input is flaky, use `picocom`/`minicom` on the same serial port.
- The Cardputer can expose multiple serial devices (USB Serial/JTAG vs UART bridge); console transport is controlled by `sdkconfig` options (`CONFIG_ESP_CONSOLE_*`).

## How to extend this demo

- Add a new event type (new ID + payload struct + producer + handler formatting).
- Add an “event storm” mode to observe queue full / drops and teach backpressure (`s_post_drops`).
- Add a second loop (control-plane vs data-plane) to show multi-loop separation.
