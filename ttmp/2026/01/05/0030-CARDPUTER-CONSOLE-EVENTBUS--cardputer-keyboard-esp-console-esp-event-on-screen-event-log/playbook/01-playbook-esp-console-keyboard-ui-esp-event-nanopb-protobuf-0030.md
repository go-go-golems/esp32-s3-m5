---
Title: 'Playbook: esp_console + keyboard + UI + esp_event + nanopb protobuf (0030)'
Ticket: 0030-CARDPUTER-CONSOLE-EVENTBUS
Status: complete
Topics:
    - cardputer
    - keyboard
    - console
    - esp-idf
    - esp32s3
    - display
    - esp-event
    - ui
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0030-cardputer-console-eventbus/components/proto/CMakeLists.txt
      Note: nanopb FetchContent + codegen
    - Path: 0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto
      Note: Bus schema and nanopb bounds
    - Path: 0030-cardputer-console-eventbus/main/CMakeLists.txt
      Note: Main component wiring; REQUIRES proto
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: 'End-to-end demo: keyboard->esp_event'
    - Path: components/cardputer_kb/layout.h
      Note: Cardputer layout + semantic actions
    - Path: components/cardputer_kb/scanner.h
      Note: Keyboard scanning API used by keyboard_task
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T09:46:40.577082664-05:00
WhatFor: ""
WhenToUse: ""
---


# Playbook: esp_console + keyboard + UI + esp_event + nanopb protobuf (0030)

## Purpose

This playbook is a “from scratch to working firmware” guide for our Cardputer demo setup:

- **Keyboard input** (M5 Cardputer matrix) → typed events
- **`esp_event` bus** as the internal backbone
- **`esp_console` REPL** to post/control events interactively
- **UI task** (M5GFX) that owns the display and renders an on-screen event log
- **nanopb/protobuf** schema + build-time code generation + bounded embedded encoding for bus events

It is written to be onboarding-friendly for a new developer who needs to:

1) build and flash the demo firmware,
2) understand how all modules fit together and why the design choices exist, and
3) use the same patterns to build a new firmware (new event IDs, new UI, new console commands).

Scope:

- Embedded side only (ESP32-S3 / ESP-IDF).
- Protobuf is used as an **IDL + boundary serialization format**. Web/TypeScript decoding is explicitly out of scope in this ticket iteration.

## Environment Assumptions

- You have ESP-IDF installed and can run `idf.py` (we typically use IDF 5.4.1).
- You can connect an M5Stack Cardputer (ESP32-S3) over USB.
- You can access the serial device (often `/dev/ttyACM0` on Linux).
- This repo exists at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`

Key firmware project path:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus`

Key docs:

- Onboarding walkthrough: `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/01-onboarding-how-the-demo-fits-together.md`
- Protobuf design: `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/design-doc/01-design-protobuf-event-payloads-websocket-bridge-for-0030.md`

## Commands

This section is the “do this, get a working device” recipe. The rest of the document explains *why* these commands work and how the system is structured.

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5

# IDF environment (adjust path if your IDF is elsewhere)
source ~/esp/esp-idf-5.4.1/export.sh

# Build
idf.py -C 0030-cardputer-console-eventbus set-target esp32s3
idf.py -C 0030-cardputer-console-eventbus build

# Flash + monitor (Linux example port)
idf.py -C 0030-cardputer-console-eventbus -p /dev/ttyACM0 flash monitor
```

## Exit Criteria

- Device boots and you see the `bus>` prompt from `esp_console`.
- The screen shows an event log UI.
- You can post a console event and see it on the screen:
  - `evt post hello`
- You can enable the console monitor and see bus traffic:
  - `evt monitor on`
- You can enable protobuf capture and print an encoded event:
  - `evt pb on`
  - `evt post hi`
  - `evt pb last` prints `len=...` plus a hex dump

If you see frequent `drops=...`, the system is still working, but you’re saturating the event pipeline (see “Performance and backpressure” below).

## Notes

Two important behavioral notes:

1) **Display ownership is intentionally single-task.** Only the UI task touches M5GFX. This prevents “random corruption” that often happens when multiple tasks draw concurrently.

2) **Event handlers must stay fast.** The handler path formats short strings, enqueues to a monitor queue (non-blocking), and stores a protobuf “last event” snapshot behind a mutex. Anything heavier should be pushed into separate worker tasks.

---

# Repository Orientation

## Where the real firmware code lives

`0030` is a normal ESP-IDF project directory:

- `0030-cardputer-console-eventbus/CMakeLists.txt` — ESP-IDF project entry.
- `0030-cardputer-console-eventbus/main/` — application component (`app_main.cpp` is the demo).
- `0030-cardputer-console-eventbus/components/proto/` — local “proto component” used for nanopb integration and codegen.
- `components/cardputer_kb/` — shared Cardputer keyboard scanning/bindings component used by multiple demos in this repo.

Ticket docs live under:

- `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/`

## How the build is structured (ESP-IDF mental model)

ESP-IDF builds are CMake-based and component-centric:

- A “project” (like `0030-cardputer-console-eventbus/`) is a set of **components**.
- Each component has a `CMakeLists.txt` that calls `idf_component_register(...)`.
- Components declare dependencies:
  - `REQUIRES` / `PRIV_REQUIRES` for other components.
  - `INCLUDE_DIRS` for public headers.
  - `SRCS` for compilation units.

In `0030`, we rely on three big “functional” components:

- `main` (our app)
- `cardputer_kb` (keyboard scanning and semantic bindings)
- `proto` (nanopb + generated `.pb.c/.h` from our `.proto`)

---

# Architecture Overview (Big Picture)

At runtime, `0030` looks like this:

```
                 ┌──────────────────────────────┐
                 │          esp_console         │
                 │  cmd parser + REPL prompt    │
                 └───────────────┬──────────────┘
                                 │ posts events (evt post/clear/...)
                                 v
┌───────────────────────┐   ┌──────────────────────────────┐   ┌───────────────────────┐
│    keyboard_task       │   │        esp_event loop         │   │   rand/heartbeat tasks│
│ (cardputer_kb scanner) │──▶│   CARDPUTER_BUS_EVENT base    │◀──│  (background producers)│
└───────────────────────┘   │ BUS_EVT_* IDs + typed payloads │   └───────────────────────┘
                            └───────────────┬──────────────┘
                                            │ dispatched by UI task
                                            v
                                ┌──────────────────────────────┐
                                │            UI task            │
                                │ esp_event_loop_run()          │
                                │ demo_event_handler()          │
                                │ render on-screen event log    │
                                └───────────────┬──────────────┘
                                                │ optional
                                                v
                                ┌──────────────────────────────┐
                                │ monitor_task (console mirror) │
                                │ prints formatted lines         │
                                └──────────────────────────────┘

                                ┌──────────────────────────────┐
                                │ nanopb “last event” snapshot  │
                                │ evt pb last => encode + hex    │
                                └──────────────────────────────┘
```

Key design choice: we create an `esp_event` loop *without* its own task, and we run it from the UI task. This ensures event handlers that touch UI state run on the same thread that draws.

---

# Event Bus: IDs, Payloads, and Dispatch

## Event base and IDs (the spine)

In `0030-cardputer-console-eventbus/main/app_main.cpp`, we define:

- `ESP_EVENT_DEFINE_BASE(CARDPUTER_BUS_EVENT);`
- `BUS_EVT_*` numeric IDs (keyboard key, keyboard action, random, heartbeat, console post, console clear)

Think of this as the “API surface” between modules. Producers post events; consumers subscribe.

## Payload strategy: fixed-size POD structs

Producers post fixed-size structs:

- `demo_kb_key_event_t`
- `demo_kb_action_event_t`
- `demo_rand_event_t`
- `demo_heartbeat_event_t`
- `demo_console_post_event_t` (bounded `char msg[96]`)

Why fixed-size?

- `esp_event_post_to()` copies raw bytes; fixed-size makes it predictable.
- No heap allocation required for events.
- Safe to pass across tasks.

## Event loop model: UI task dispatches

We create a **user event loop**:

```c
esp_event_loop_args_t args = {
  .queue_size = 64,
  .task_name = NULL, // important
  .task_priority = 0,
  .task_stack_size = 0,
  .task_core_id = 0,
};
esp_event_loop_create(&args, &s_loop);
```

And later, the UI task periodically calls:

```c
esp_event_loop_run(s_loop, pdMS_TO_TICKS(5));
```

This has two consequences:

- Handlers execute in the UI task context (good for UI state updates).
- If the UI task stalls, the event queue can back up and producers can see `ESP_ERR_TIMEOUT` (drops).

## Handler: keep it short, push heavy work out

The handler (`demo_event_handler`) does *only* what’s needed:

- Format a short on-screen log line.
- Update counters/statistics.
- Optionally enqueue a console monitor line.
- Optionally update the protobuf snapshot.

It intentionally does not:

- block on IO,
- allocate large buffers,
- do expensive computations.

---

# Keyboard Pipeline (Cardputer)

The keyboard flow is:

1) Hardware scan (matrix) → pressed key numbers
2) Apply semantic bindings (“actions” like NavUp/NavDown)
3) Post bus events (key edges + actions)

The shared component is:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/`

The demo uses it from:

- `0030-cardputer-console-eventbus/main/app_main.cpp` (see `keyboard_task`)

## What is a “keynum”?

In this repo, Cardputer keys are represented by a vendor-style key number (`1..56`) used by `cardputer_kb`. These are not USB HID codes; they are “physical key indices”.

## Modifiers and semantic actions

`0030` computes a small “modifiers bitfield” from pressed keys:

- Shift, Ctrl, Alt, Fn

It then produces two event types:

- **Key edge** events: “key X pressed/released” (except pure modifiers are filtered)
- **Action** events: “NavUp”, “NavDown”, etc., derived from bindings (layout + Fn combos)

Pseudo-flow:

```c
snapshot = scanner.scan();
mods = modifiers_from_pressed(snapshot.pressed_keynums);

for each edge in snapshot.edges:
  if edge.keynum is modifier: continue;
  post BUS_EVT_KB_KEY { ts_us, keynum, mods }

for each semantic action resolved from bindings:
  post BUS_EVT_KB_ACTION { ts_us, action_id, mods }
```

## Why both key edges and actions?

Because they serve different audiences:

- UI navigation can be driven by actions (portable intent: “NavUp”).
- Debugging and mapping requires raw key edges (“keynum 17 pressed”).

---

# UI: On-Screen Event Log (M5GFX)

UI state is kept in a single struct (`UiState`) inside `app_main.cpp`:

- a deque of strings (log lines)
- counters (event counts, dropped lines)
- scrollback and pause state

The UI loop:

1) dispatches some events (`esp_event_loop_run`)
2) renders a canvas to the screen
3) applies keyboard actions for UI controls (scroll, pause, clear)

Important invariants:

- Only the UI task calls M5GFX drawing APIs.
- Event handlers that mutate UI state run in the UI task context (because UI dispatches the loop).

---

# esp_console: REPL Integration

The demo uses `esp_console` to provide an interactive prompt:

- prompt: `bus> `
- command: `evt ...`

The REPL is useful for:

- injecting events (`evt post ...`, `evt spam N`)
- controlling monitoring (`evt monitor on/off`)
- printing encoded protobuf (`evt pb last`)

## Console transport backends (and why it can be confusing)

ESP-IDF supports multiple console “IO backends”:

- USB Serial/JTAG (common on S3 boards)
- UART (external USB-UART)

The demo attempts to start the console on an available backend (configured via `sdkconfig.defaults` and menuconfig options). If you see warnings like:

- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; console not started`

…that’s a configuration mismatch: fix the Kconfig.

## Known sharp edge: `idf.py monitor` input

`idf.py monitor` is great for logs, but interactive input can be flaky depending on terminal/host.

Symptoms:

- “Writing to serial is timing out”
- prompt appears but input doesn’t reach device reliably

Workarounds:

- Use `tmux` and send keystrokes programmatically (handy for scripted validation).
- Use a dedicated serial terminal (`picocom`, `minicom`) if you need reliable REPL interaction.

---

# Console Commands (evt)

All commands are implemented in `cmd_evt(...)` in `0030-cardputer-console-eventbus/main/app_main.cpp`.

## Event injection

- `evt post <text>`: posts `BUS_EVT_CONSOLE_POST` with bounded message (96 bytes).
- `evt spam <n>`: posts N console events quickly (useful for testing backpressure/drops).
- `evt clear`: posts `BUS_EVT_CONSOLE_CLEAR` (UI clears the on-screen log).
- `evt status`: prints `drops=...` (counts event post failures/timeouts).

## Bus monitor (console mirror)

- `evt monitor on|off|status`

When enabled, the event handler enqueues formatted strings to a small FreeRTOS queue and `monitor_task` prints them.

Why a queue+task instead of printing in handler?

- Printing from handlers is slow and can block; it can also garble the REPL prompt.
- Enqueueing is non-blocking (`xQueueSend(..., 0)`), so handlers stay fast.

## Protobuf capture + encode (nanopb)

- `evt pb on|off|status|last`

Behavior:

- `on`: enables capture (store “last protobuf envelope” snapshot when events arrive)
- `last`: encodes snapshot into a fixed stack buffer and prints a hex dump

This is a validation harness, not yet a network bridge. It proves:

- schema/codegen are working,
- envelope mapping from bus payloads is correct,
- encoded output is bounded and deterministic.

---

# Protobuf + nanopb: Schema, Codegen, and Embedded Encoding

## Why nanopb (embedded)

Nanopb is a small protobuf implementation designed for embedded constraints:

- code size is small,
- it can generate fixed-size arrays for strings/repeated fields when configured,
- encoding can be done without heap allocation.

## Where the schema lives

Schema file:

- `0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto`

Conceptually, it mirrors `BUS_EVT_*` and payload structs from `app_main.cpp`.

It contains:

- an `EventId` enum (stable numeric IDs)
- a message for each payload type
- an `Event` envelope with:
  - `schema_version`
  - `id`
  - `oneof payload { ... }`

## How codegen is wired (CMake component)

The `proto` component is:

- `0030-cardputer-console-eventbus/components/proto/CMakeLists.txt`

It:

1) fetches nanopb via `FetchContent`,
2) configures `CMAKE_MODULE_PATH` so `find_package(Nanopb)` finds nanopb’s CMake helpers,
3) runs `nanopb_generate_cpp(...)` to generate `demo_bus.pb.c/.h`,
4) exposes a `proto` static library target to consumers.

### ESP-IDF sharp edge: “early expansion”

ESP-IDF runs component `CMakeLists.txt` in an “early expansion” pass to expand `CONFIG_*` and other build-time metadata.

If you call `idf_component_register()` and then add nanopb with `FetchContent_MakeAvailable(...)`, ESP-IDF’s build system can leak IDF link dependencies into nanopb’s targets, causing CMake export/install errors.

The proven fix (from `espressif/esp-idf#15693`) is to guard FetchContent + codegen behind:

```cmake
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
  # FetchContent + find_package + nanopb_generate_cpp
endif()
```

This is exactly how our `proto` component is written.

### Nanopb generator Python dependency

Nanopb’s generator is a Python plugin. If the Python `protobuf` package is missing in the environment used by the generator, you may see:

`ModuleNotFoundError: No module named 'google'`

Fix by installing into the ESP-IDF python env:

```bash
/home/manuel/.espressif/python_env/idf5.4_py3.11_env/bin/python -m pip install protobuf
```

## Embedded encoding strategy in 0030

`0030` keeps internal bus payloads as structs and encodes only at the boundary:

Pseudo-code:

```c
// event handler receives (base,id,data)
Event pb = map_to_proto(id, data);
store_last(pb); // guarded by mutex

// console command:
pb = load_last();
need = pb_get_encoded_size(pb);
buf[256];
pb_encode(buf, pb);
print_hex(buf, len);
```

This pattern scales naturally:

- Replace “print hex” with “send bytes over WebSocket”.
- Replace “last snapshot” with “queue of encoded frames”.

---

# Performance and Backpressure (Drops)

## What a “drop” means in this demo

Drops are counted when `esp_event_post_to(...)` fails (often `ESP_ERR_TIMEOUT`) because:

- the event loop queue is full, and
- the post uses a 0-tick timeout.

This is intentional: producers never block; instead, they drop and increment counters.

Tradeoff:

- Good: system remains responsive and avoids deadlocks.
- Bad: under heavy load, you can lose events.

Where to look:

- `evt status` prints the current drop count.
- Monitor output can show heartbeat `drops=...`.

How to reduce drops:

- Increase the event loop queue size (see creation args in `app_main.cpp`).
- Ensure the UI task dispatches frequently (`esp_event_loop_run` budget).
- Reduce event rate from producers (rand/heartbeat periods).

---

# Building Your Own Firmware From Scratch (Recommended Recipe)

This is the “use the same pattern, but for your firmware” checklist.

## 1) Start from a minimal ESP-IDF project directory

Create:

- `CMakeLists.txt` including `project.cmake`
- `main/CMakeLists.txt` with `idf_component_register`
- `main/app_main.cpp` with `extern "C" void app_main(void)`

Use `0030-cardputer-console-eventbus/` as a template.

## 2) Decide your bus contract first

Define:

- `ESP_EVENT_DEFINE_BASE(MY_BUS_EVENT)`
- event IDs
- fixed-size payload structs

Rule of thumb:

- If you can bound it, keep it fixed-size (strings, arrays, etc.).
- If you can’t bound it, move it out of the bus payload (store elsewhere; bus carries IDs/handles).

## 3) Create an event loop and pick a dispatch strategy

Two common strategies:

1) **UI dispatches loop** (what 0030 does):
   - Good when handlers update UI state.
2) **Dedicated event loop task**:
   - Good when UI shouldn’t be responsible for dispatch performance.

If you choose (2), ensure UI operations are queued to the UI task.

## 4) Add producers (tasks) and keep posting non-blocking

Use `esp_event_post_to(..., timeout=0)` in producers.

If you need guaranteed delivery, move to:

- higher queue size,
- a bounded producer-side queue,
- or backpressure semantics (drop oldest / drop newest / block).

## 5) Add `esp_console` for introspection and control

Minimum:

- `esp_console_init(...)`
- register commands with `esp_console_cmd_register(...)`
- start REPL (`esp_console_new_repl_*` + `esp_console_start_repl`)

Keep commands narrow:

- post events
- toggle modes
- dump stats

Avoid long-running work in the console handler; it blocks the REPL.

## 6) Add nanopb schema + codegen only after bus payloads stabilize

Add a `proto` component:

- Put `.proto` files under `components/proto/defs/`.
- Use the `CMAKE_BUILD_EARLY_EXPANSION` guard for `FetchContent` and `nanopb_generate_cpp`.
- Expose a static library target and add `REQUIRES proto` from `main`.

Then implement a boundary encoder:

- map bus payload structs → protobuf envelope
- encode into a bounded buffer
- send/store/print

---

# Troubleshooting

## “I see `bus>` but can’t type commands”

- Some terminals + `idf.py monitor` combinations don’t reliably send input.
- Try using `picocom` or a tmux session and send keys.

## “console not started” / backend warnings

- Check `sdkconfig.defaults` and Kconfig options:
  - USB Serial/JTAG console enablement
  - UART console settings if using UART

## Nanopb generator fails with ModuleNotFoundError

Install Python protobuf into the ESP-IDF python environment:

```bash
/home/manuel/.espressif/python_env/idf5.4_py3.11_env/bin/python -m pip install protobuf
```

## Protobuf event too large

`evt pb last` uses a fixed `uint8_t buf[256]`.

If you add larger messages:

- either increase this buffer,
- or move to a streaming/fragmented transport,
- or design payloads to stay bounded (preferred).

---

# API References (ESP-IDF)

Key ESP-IDF modules used in this demo:

- `esp_event` (user event loops, handler registration, posting)
  - headers: `esp_event.h`
  - typical APIs: `esp_event_loop_create`, `esp_event_handler_register_with`, `esp_event_post_to`, `esp_event_loop_run`
- `esp_console` (REPL, command registration)
  - headers: `esp_console.h`
  - typical APIs: `esp_console_init`, `esp_console_cmd_register`, `esp_console_new_repl_*`, `esp_console_start_repl`
- FreeRTOS tasks/queues/semaphores:
  - `xTaskCreate`, `xQueueCreate`, `xQueueSend`, `xSemaphoreCreateMutex`

Nanopb runtime:

- headers: `pb_encode.h` (and `pb_decode.h` if/when decoding is added)

---

# Appendix: “What to read first” (for a new dev)

If you’re joining the team and want to quickly understand the system:

1) Read `0030-cardputer-console-eventbus/main/app_main.cpp` and locate these symbols:
   - `CARDPUTER_BUS_EVENT`, `BUS_EVT_*`
   - `keyboard_task`
   - `cmd_evt`
   - `demo_event_handler`
   - `monitor_task` + `evt monitor ...`
   - `evt pb ...` encoding path
2) Read the `.proto` schema:
   - `0030-cardputer-console-eventbus/components/proto/defs/demo_bus.proto`
3) Read the nanopb integration CMake:
   - `0030-cardputer-console-eventbus/components/proto/CMakeLists.txt`
4) Then read the higher-level docs:
   - onboarding analysis
   - protobuf design doc
   - pattern doc for “how to package this into reusable components”
