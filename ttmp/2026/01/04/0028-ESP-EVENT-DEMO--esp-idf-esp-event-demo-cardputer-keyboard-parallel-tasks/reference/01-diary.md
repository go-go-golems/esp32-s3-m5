---
Title: Diary
Ticket: 0028-ESP-EVENT-DEMO
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - keyboard
    - display
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_event/include/esp_event.h
      Note: esp_event API notes referenced in diary
    - Path: 0012-cardputer-typewriter/main/hello_world_main.cpp
      Note: Key event edge + dirty redraw baseline
    - Path: 0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Keyboard event tokenization reference
    - Path: 0028-cardputer-esp-event-demo/main/app_main.cpp
      Note: Implemented demo referenced in diary Step 5
    - Path: components/cardputer_kb/REFERENCE.md
      Note: Primary keyboard scan reference during exploration
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-04T23:31:31.041687934-05:00
WhatFor: Step-by-step diary while exploring this repo and ESP-IDF to design a Cardputer esp_event demo.
WhenToUse: Use while implementing/reviewing ticket 0028-ESP-EVENT-DEMO to understand decisions, tradeoffs, and validation steps.
---



# Diary

## Goal

Capture the exploration + design work needed to build an `esp_event` demo firmware where:
- keyboard keypresses are published as `esp_event` events
- a few parallel tasks publish random/periodic events
- a receiver task renders keyboard events on the Cardputer display

## Step 1: Ticket setup + initial orientation

Created the `docmgr` ticket workspace and scaffold docs (design doc + diary) so the exploration is traceable and reviewable.

Next is to inventory existing Cardputer keyboard and display patterns in this repo, then map them onto an `esp_event`-centric demo architecture.

### What I did
- Ran `docmgr status --summary-only` to confirm the docs root and config.
- Created ticket `0028-ESP-EVENT-DEMO` with topics `esp-idf,esp32s3,cardputer,keyboard,display,ui`.
- Added docs: a `design-doc` for architecture and this `reference` diary doc.
- Added initial tasks in `tasks.md` to track the analysis work.
- Identified ESP-IDF version from `.envrc` (`~/esp/esp-idf-5.4.1`).

### Why
- Keep the analysis anchored to concrete code references and a reproducible workflow.

### What worked
- `docmgr` workspace/doc scaffolding created cleanly under `ttmp/2026/01/04/...`.

### What didn't work
- N/A

### What I learned
- This repo already contains multiple Cardputer keyboard + display firmwares and a reusable keyboard component (`components/cardputer_kb`) that should be the baseline for the demo.

### What was tricky to build
- N/A (setup only)

### What warrants a second pair of eyes
- N/A (setup only)

### What should be done in the future
- As the demo design solidifies, relate the specific firmware/component files that informed each decision (keyboard scan, UI rendering, and esp_event usage).

### Code review instructions
- Start at `ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/index.md`.
- Review `ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/design-doc/01-esp-event-demo-architecture-event-taxonomy.md` as it fills in.

### Technical details
- Commands run:
  - `docmgr status --summary-only`
  - `docmgr ticket create-ticket --ticket 0028-ESP-EVENT-DEMO --title "ESP-IDF esp_event demo: Cardputer keyboard + parallel tasks" --topics esp-idf,esp32s3,cardputer,keyboard,display,ui`
  - `docmgr doc add --ticket 0028-ESP-EVENT-DEMO --doc-type design-doc --title "esp_event demo: architecture + event taxonomy"`
  - `docmgr doc add --ticket 0028-ESP-EVENT-DEMO --doc-type reference --title "Diary"`
  - `docmgr task add --ticket 0028-ESP-EVENT-DEMO --text "..."`

## Step 2: Survey Cardputer keyboard + display patterns in this repo

The key building blocks for an `esp_event` demo already exist here: a reusable Cardputer keyboard scanner component, and several display/UI patterns that can render “streams of events” (terminal/typewriter UIs).

The remaining design work is to decide where the `esp_event` loop runs (dedicated task vs. UI task calling `esp_event_loop_run`) and how the demo’s event taxonomy maps onto these existing primitives.

### What I did
- Read Cardputer keyboard component docs and APIs:
  - `components/cardputer_kb/include/cardputer_kb/scanner.h`
  - `components/cardputer_kb/include/cardputer_kb/layout.h`
  - `components/cardputer_kb/include/cardputer_kb/bindings.h`
  - `components/cardputer_kb/REFERENCE.md`
- Reviewed “human-friendly” keyboard event wrapper used by the demo-suite:
  - `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.{h,cpp}`
  - `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` (poll + handle + render loop)
  - `0022-cardputer-m5gfx-demo-suite/main/ui_console.{h,cpp}` (terminal-like renderer)
- Reviewed a simpler “typewriter” firmware that turns key edges into on-screen text:
  - `0012-cardputer-typewriter/main/hello_world_main.cpp`
- Reviewed the keyboard scancode visualizer/calibrator for a grid-style UI and scan snapshots:
  - `0023-cardputer-kb-scancode-calibrator/main/app_main.cpp`
- Skimmed FreeRTOS task patterns for “parallel producers”:
  - `0002-freertos-queue-demo/main/hello_world_main.c`

### Why
- The demo should feel native to this repo: reuse `cardputer_kb` and the existing on-screen console patterns, and mirror how other tutorials structure task loops and “dirty” redraws.

### What worked
- `components/cardputer_kb` cleanly separates “physical key identity” (x/y + keyNum) from “semantic actions” (bindings/chords).
- `0022` shows a practical, low-jitter UI loop that only redraws when needed (console dirty flag, periodic demo refresh).
- `0023` provides a ready-made visual diagnostic UI for pressed keys and can be used to validate scan correctness.

### What didn't work
- N/A (read-only exploration)

### What I learned
- `cardputer_kb::MatrixScanner` already implements the primary/alt IN0/IN1 autodetect and returns a stable `pressed_keynums` vector suited for posting as event payload.
- The repo already has a “token stream” UI (`ui_console`) which is a perfect target for “render incoming keyboard events”.

### What was tricky to build
- N/A (analysis only)

### What warrants a second pair of eyes
- The event-loop/threading choice: rendering inside an `esp_event` handler is only “safe” if that handler runs in the same task/context that owns the display SPI usage (or if display usage is carefully synchronized).

### What should be done in the future
- Prefer designing the demo so the UI task *is* the event loop dispatcher (via `esp_event_loop_run`), avoiding a separate event-loop task that might accidentally render concurrently with other UI work.

### Code review instructions
- Keyboard scan baseline: `components/cardputer_kb/REFERENCE.md`.
- Key-to-token UI baseline: `0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp`.
- End-to-end “keypress -> on-screen text” baseline: `0012-cardputer-typewriter/main/hello_world_main.cpp`.

### Technical details
- Commands used during exploration:
  - `rg -n "esp_event|event loop" ...`
  - `sed -n '...' <file>`

## Step 3: Review ESP-IDF `esp_event` APIs and dispatch model

Read the public API surface (`esp_event.h`) and the implementation (`esp_event.c`) to understand what the demo can safely do inside handlers, and what the performance/memory implications are when posting lots of events.

This clarified the main architectural fork for the demo: a dedicated event-loop task (handlers run on that task) vs. a “manual loop” (`task_name=NULL`) where the UI task calls `esp_event_loop_run()` and therefore owns both event dispatch and display rendering.

### What I did
- Read these ESP-IDF sources (v5.4.1 per `.envrc`):
  - `~/esp/esp-idf-5.4.1/components/esp_event/include/esp_event.h`
  - `~/esp/esp-idf-5.4.1/components/esp_event/include/esp_event_base.h`
  - `~/esp/esp-idf-5.4.1/components/esp_event/esp_event.c`
  - `~/esp/esp-idf-5.4.1/components/esp_event/default_event_loop.c`
- Focused on:
  - `esp_event_loop_create()` + the `task_name == NULL` behavior
  - `esp_event_loop_run()` dispatch guarantees
  - `esp_event_post_to()` queue + event_data lifetime rules

### Why
- The demo includes parallel producers and display rendering; `esp_event` handler execution context matters for correctness (display/SPI ownership) and for “smoothness” (avoiding long handler stalls on the loop task).

### What worked
- `esp_event_loop_create()` explicitly supports loops without a dedicated task, which can be dispatched by any “owner task” via `esp_event_loop_run()`.
- `esp_event_post_to()` copies `event_data` into heap storage (so it’s safe to post stack structs) and frees it after dispatch.

### What didn't work
- N/A

### What I learned
- Posting events allocates memory when `event_data_size > 0` (heap `calloc` + `memcpy` per post). For a “random event spammer” task, keep payload small and/or rate-limited to avoid heap churn.
- If a loop has a dedicated task, handlers run on that task (`esp_event_loop_run_task`). Rendering in handlers can be OK for a demo, but it risks stalling other event consumers and is easy to accidentally race with other drawing code.

### What was tricky to build
- The “right” threading model is non-obvious until you notice the `task_name == NULL` option and that `esp_event_loop_run()` sets `loop->running_task` to avoid deadlocks when a handler posts to the same loop.

### What warrants a second pair of eyes
- Confirm the recommended pattern for UI ownership: event-loop dispatch inside the UI task should eliminate concurrent display access, but it also means any long handler blocks the UI loop. We should keep handlers short (update state + set dirty) and render at a fixed cadence.

### What should be done in the future
- In the design doc, explicitly document the “manual loop in UI task” choice (or justify not choosing it), and add a small section on event payload sizing/rate limiting.

### Code review instructions
- API surface: `~/esp/esp-idf-5.4.1/components/esp_event/include/esp_event.h`.
- Key implementation behaviors:
  - `esp_event_loop_create()` and its `task_name != NULL` branch in `~/esp/esp-idf-5.4.1/components/esp_event/esp_event.c`
  - `esp_event_post_to()` data copy + queue semantics in `~/esp/esp-idf-5.4.1/components/esp_event/esp_event.c`

### Technical details
- Commands used:
  - `sed -n '...' ~/esp/esp-idf-5.4.1/components/esp_event/include/esp_event.h`
  - `sed -n '...' ~/esp/esp-idf-5.4.1/components/esp_event/esp_event.c`

## Step 4: Draft demo design (event taxonomy + task layout)

Converted the exploration notes into a concrete demo design: define a small event taxonomy, create a dedicated application event loop, have multiple producer tasks post into it, and have a UI task that both dispatches events and renders them.

The main decision is to dispatch the event loop from the UI task (`task_name=NULL` + `esp_event_loop_run()` in the UI loop). That keeps display ownership simple and makes the “receiver task renders keyboard events” requirement map cleanly onto the `esp_event` model.

### What I did
- Wrote the design doc with:
  - recommended event loop configuration (`esp_event_loop_create`, no dedicated task)
  - event base + IDs + payload struct shapes
  - responsibilities of keyboard producer, random producers, and UI consumer/renderer

### Why
- A demo should be safe by default: avoid rendering inside `sys_evt` (default loop task) and avoid implicit cross-task drawing.

### What worked
- The repo’s existing building blocks (keyboard scan + console/typewriter UI patterns) map cleanly onto an `esp_event` demo with minimal new concepts.

### What didn't work
- N/A

### What I learned
- `esp_event` is a good fit for “fan-in” from multiple tasks, but you still need to be explicit about *where* handlers run, especially when they touch hardware like the display.

### What was tricky to build
- Choosing a payload format that is informative on-screen without pulling in dynamic allocation (e.g., prefer `keynum` + modifiers over a heap string).

### What warrants a second pair of eyes
- Event-loop ownership and render cadence: verify that “handlers only mutate state + set dirty” is sufficient to keep the UI responsive even under random-event bursts.

### What should be done in the future
- Add a playbook doc once the firmware exists (build/flash + expected UI output + troubleshooting).

### Code review instructions
- Read the design doc: `ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/design-doc/01-esp-event-demo-architecture-event-taxonomy.md`.

### Technical details
- N/A (doc drafting)

## Step 5: Implement demo firmware (`0028-cardputer-esp-event-demo`)

Implemented the actual tutorial firmware project that matches the design: a custom application event loop (`esp_event_loop_create`), a keyboard producer task posting key events, a few random/heartbeat producers, and a UI task that dispatches events via `esp_event_loop_run()` and renders an on-screen event log.

The build succeeded under ESP-IDF 5.4.1 (same version referenced in the earlier analysis), which gives confidence the demo is ready to flash and iterate on.

### What I did
- Added a new ESP-IDF project directory: `0028-cardputer-esp-event-demo/`.
- Implemented:
  - app event base + event IDs (`CARDPUTER_DEMO_EVENT`, `CARDPUTER_DEMO_EVT_*`)
  - UI task that owns the display and calls `esp_event_loop_run()` with a tick budget
  - keyboard producer (`cardputer_kb::MatrixScanner`) posting key edges and chord actions
  - random producers posting `RAND#N` events
  - heartbeat task posting periodic heap/DMA stats
- Built the firmware locally.

### Why
- Move from “analysis/design” to a runnable demo firmware that showcases `esp_event` in a Cardputer-friendly way and avoids cross-task display access.

### What worked
- The project builds cleanly with M5GFX + `components/cardputer_kb` as extra components.
- The UI-dispatch model is straightforward: events fan-in from multiple tasks, but handlers run in the UI task context because dispatch happens there.

### What didn't work
- The first `idf.py set-target` run generated `dependencies.lock` automatically (expected for IDF component manager); no functional issue.
- Initial UI sprite wiring: `M5Canvas` needs a parent display (`M5Canvas screen(&display)`); fixed by giving `UiState` a constructor that initializes `screen` with `&display`.

### What I learned
- `esp_event_loop_run(portMAX_DELAY)` would effectively “own” the UI task and prevent periodic redraws; using a small tick budget is essential for a responsive render loop.

### What was tricky to build
- Keeping UI controls functional while the log is “paused” (controls are handled on receive side and set `dirty` even when log append is disabled).

### What warrants a second pair of eyes
- Event rate / heap churn: each `esp_event_post_to()` with a payload allocates/copies data; verify the default random producer rates don’t create unnecessary heap fragmentation over long runs.

### What should be done in the future
- Flash on real hardware and confirm keyboard scan + nav bindings behave as expected across Cardputer revisions (primary vs alt IN0/IN1 autodetect).

### Code review instructions
- Start at `0028-cardputer-esp-event-demo/main/app_main.cpp`:
  - `ui_task()` (event loop creation + dispatch + rendering)
  - `keyboard_task()` (scan + edge/action events)
  - `demo_event_handler()` (event-to-UI state + controls)
- Build with: `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0028-cardputer-esp-event-demo build`

### Technical details
- Commands run:
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0028-cardputer-esp-event-demo set-target esp32s3`
  - `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0028-cardputer-esp-event-demo build`
