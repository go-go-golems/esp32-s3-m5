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

## Related

- Onboarding analysis: `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/01-onboarding-how-the-demo-fits-together.md`
