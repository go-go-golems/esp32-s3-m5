---
Title: 'esp_event demo: architecture + event taxonomy'
Ticket: 0028-ESP-EVENT-DEMO
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - keyboard
    - display
    - ui
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_event/esp_event.c
      Note: esp_event dispatch + event_data copy semantics (manual loop vs loop task)
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_event/include/esp_event.h
      Note: esp_event public API surface (loop create/run/post)
    - Path: 0005-wifi-event-loop/main/hello_world_main.c
      Note: In-repo esp_event handler registration example
    - Path: 0012-cardputer-typewriter/main/hello_world_main.cpp
      Note: Key edge -> text UI baseline (dirty redraw pattern)
    - Path: 0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Existing CardputerKeyboard token/modifier/edge logic to reuse or mirror
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp
      Note: On-screen event console rendering pattern
    - Path: 0028-cardputer-esp-event-demo/main/app_main.cpp
      Note: Concrete implementation of the proposed esp_event demo design
    - Path: components/cardputer_kb/REFERENCE.md
      Note: Keyboard scan + keynum/chord reference used for event payload design
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-04T23:31:30.695094646-05:00
WhatFor: Design analysis for a Cardputer demo firmware showcasing ESP-IDF esp_event with keyboard input and parallel event producers.
WhenToUse: Use when implementing the 0028 esp_event demo firmware, or when reviewing how this demo maps existing keyboard/display code onto esp_event.
---



# esp_event demo: architecture + event taxonomy

## Executive Summary

Build a Cardputer firmware demo that treats keyboard input as `esp_event` events, alongside a few parallel tasks that generate random/periodic events. A dedicated UI task owns the display and *also* runs the event loop dispatcher (`esp_event_loop_run()`), so event handlers can update UI state without any cross-task display locking.

The demo’s UI is a simple “event console” (scrolling log) that renders incoming events in real time. This reuses existing patterns from this repo: `components/cardputer_kb` for keyboard scanning and the terminal/typewriter style rendering patterns already used by Cardputer demos.

## Problem Statement

We want a concrete, easy-to-understand demonstration of the ESP-IDF `esp_event` framework that:

- Shows how multiple producers (keyboard + background tasks) can publish events to a central event loop.
- Shows how a consumer can react to specific event IDs and render results on-screen.
- Avoids common pitfalls (rendering from the system default event loop task, races on display access, large/alloc-heavy event payloads).

## Proposed Solution

### High-level architecture

- **Event loop**: Create a dedicated *application* event loop using `esp_event_loop_create()` with `task_name = NULL`.
  - The loop has no internal task; it is dispatched by a **UI task** calling `esp_event_loop_run()` regularly.
- **Keyboard producer task**: Poll `cardputer_kb::MatrixScanner` (or reuse the `CardputerKeyboard` wrapper pattern) and post “key edge” events into the app event loop.
- **Random producer tasks (N)**: Each task periodically posts a random/periodic event (with a small payload and a per-task `producer_id`).
- **UI task / event consumer**:
  - Initializes display (`M5GFX`) and a simple console renderer/state.
  - Registers handlers for keyboard and random events on the app event loop.
  - Runs a loop: dispatch events (`esp_event_loop_run()`), update state, and render when dirty or at a fixed cadence.

This keeps display rendering in one task, while still demonstrating that events can be posted from multiple tasks.

### Event taxonomy

Use one application event base and a small set of event IDs.

**Event base**

```c
ESP_EVENT_DECLARE_BASE(CARDPUTER_DEMO_EVENT);
```

In one `.c/.cpp` translation unit:

```c
ESP_EVENT_DEFINE_BASE(CARDPUTER_DEMO_EVENT);
```

Notes:
- If this demo mixes C and C++, keep the declaration/definition in a shared header/translation unit compiled consistently (or wrap the declaration in `extern "C"` when included from C++).

**Event IDs**

- `CARDPUTER_DEMO_EVT_KB_KEY` — a single key edge (press) event
- `CARDPUTER_DEMO_EVT_RAND` — a background “random event”
- `CARDPUTER_DEMO_EVT_HEARTBEAT` — optional periodic status event (uptime, heap, etc.)

**Event payloads (POD, small)**

Keyboard payload should be stable and easy to interpret in the UI. Prefer physical identity (`keynum`) + modifier flags over dynamically allocated strings.

Example:

```c
typedef struct {
    int64_t ts_us;
    uint8_t keynum;     // vendor-style 1..56 (cardputer_kb)
    uint8_t modifiers;  // bitmask: shift/ctrl/alt/fn
} demo_kb_event_t;
```

Random payload example:

```c
typedef struct {
    int64_t ts_us;
    uint8_t producer_id;
    uint32_t value;
} demo_rand_event_t;
```

Notes:
- `esp_event_post_to()` copies `event_data` into heap storage and frees it after dispatch, so stack structs are safe.
- Keep payloads small; avoid high-rate producers with large payloads to reduce heap churn.

### Event loop configuration

Create an app loop sized for “bursty” input (keyboard + random tasks):

```c
esp_event_loop_handle_t app_loop = NULL;
esp_event_loop_args_t args = {
    .queue_size = 32,
    .task_name = NULL,          // no dedicated task
    .task_priority = 0,         // ignored
    .task_stack_size = 0,       // ignored
    .task_core_id = 0,          // ignored
};
ESP_ERROR_CHECK(esp_event_loop_create(&args, &app_loop));
```

### Handler model (UI owns the display)

Register handlers on `app_loop`, then call `esp_event_loop_run(app_loop, ticks)` from the UI task. Because handlers run inside that dispatch call, they execute in the UI task context.

Handler responsibilities:
- **Do not render immediately**; instead update UI state and set a `dirty` flag.
- Keep handlers short and deterministic.

### Keyboard producer details

Baseline options from this repo:
- `components/cardputer_kb` — stable scan snapshot (`pressed_keynums`) and optional semantic chord decoder.
- `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` — provides `KeyEvent` tokens + modifiers and does edge detection.

For the demo, treat a “key event” as a rising edge of a non-modifier key (or a semantic action from chord decoding). Post one `demo_kb_event_t` per edge.

### UI rendering

Reuse the “console” pattern:
- maintain a deque of strings (or a ring buffer of fixed-size lines)
- on each event, append a formatted line:
  - `"[123456us] KB keynum=42 token=enter mods=shift"`
  - `"[123789us] RAND#2 value=0xdeadbeef"`
- re-render when dirty (or at ~30fps with a max render budget)

If you want a more “visual” demo, add a grid overlay similar to `0023` (pressed keys) while still logging event lines in a footer.

## Design Decisions

### Run the app event loop inside the UI task (`task_name = NULL`)

Rationale:
- Avoids rendering from the system default event-loop task (`sys_evt`) or another dedicated loop task that might race display access.
- Keeps a single owner for display/SPI usage.
- Still demonstrates multi-producer concurrency, since other tasks post into the loop’s queue.

Tradeoff:
- Long handlers block the UI loop; handlers must stay short and defer heavy work to the main UI loop rendering path.

### Use `cardputer_kb` as the keyboard scan baseline

Rationale:
- Consolidates scan wiring and the primary/alt IN0/IN1 autodetect logic.
- Provides stable physical key identity (`keynum`) suitable for an event payload.

### Keep event payloads small and allocation-friendly

Rationale:
- `esp_event_post_to()` heap-allocates a copy when `event_data_size > 0`; a spammy producer can otherwise cause avoidable heap churn.

## Alternatives Considered

### Use the default event loop (`esp_event_loop_create_default()`)

Pros:
- Familiar from WiFi examples.

Cons:
- Handlers run on the system event task (`sys_evt`), which is a poor place to do display rendering or long-running work.
- Mixes demo events with system component events (WiFi, IP, etc.), which makes the demo harder to reason about.

### Create a dedicated app event-loop task and forward to UI via a queue

Pros:
- Keeps event dispatch separate from UI; handlers can be very short and forward messages.

Cons:
- Adds a second queue and more plumbing, which distracts from the “esp_event demo” goal.
- Still requires careful display ownership rules.

### Skip `esp_event` and use FreeRTOS queues directly

Rejected because the purpose of the demo is specifically to show `esp_event`’s registration and dispatch model.

## Implementation Plan

1. Create the tutorial firmware directory `0028-cardputer-esp-event-demo/` with an ESP-IDF app skeleton.
2. Define `CARDPUTER_DEMO_EVENT`, event IDs, and payload structs (currently in `0028-cardputer-esp-event-demo/main/app_main.cpp`).
3. Implement `ui_task`:
   - init `M5GFX` + a sprite/canvas backbuffer
   - create `app_loop` (`task_name=NULL`)
   - register handlers for keyboard and random events
   - run a loop: `esp_event_loop_run(app_loop, ...)` + render dirty console
4. Implement `kb_task` using `cardputer_kb::MatrixScanner` (or the `CardputerKeyboard` wrapper approach) and post `CARDPUTER_DEMO_EVT_KB_KEY`.
5. Implement `rand_task` instances (2–3) that post `CARDPUTER_DEMO_EVT_RAND` at random intervals.
6. Add basic `menuconfig` knobs (queue size, producer intervals, keyboard scan period).
7. Add a short playbook: build/flash, and “expected on-screen behavior” checklist.

## Open Questions

- Should keyboard events include both “down” and “up”, or just “down edge”?
- Should Fn-chords be emitted as semantic events (NavUp/Down/Left/Right) using the captured bindings, or should the demo only emit physical keynums?
- What is the best default queue size for smoothness without wasting RAM (e.g. 16 vs 32 vs 64)?
- Should the demo include an “event rate” overlay (events/sec per producer) to make concurrency obvious?

## References

- Keyboard scan baseline: `components/cardputer_kb/REFERENCE.md`
- Keyboard wrapper/token mapping: `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
- Console rendering pattern: `0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp`
- Typewriter UI baseline: `0012-cardputer-typewriter/main/hello_world_main.cpp`
- esp_event API/internal notes: `~/esp/esp-idf-5.4.1/components/esp_event/include/esp_event.h`
- Firmware implementation: `0028-cardputer-esp-event-demo/main/app_main.cpp`
