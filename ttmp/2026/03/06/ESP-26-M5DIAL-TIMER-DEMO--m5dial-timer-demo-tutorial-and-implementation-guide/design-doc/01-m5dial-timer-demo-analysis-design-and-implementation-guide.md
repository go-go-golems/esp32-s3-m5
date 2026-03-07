---
Title: M5Dial timer demo analysis, design, and implementation guide
Ticket: ESP-26-M5DIAL-TIMER-DEMO
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - timer
    - ui
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Dial-UserDemo/main/hal/display/hal_display.hpp
      Note: M5Dial display and backlight wiring reference
    - Path: ../../../../../../../M5Dial-UserDemo/main/hal/hal.cpp
      Note: Primary M5Dial board initialization reference
    - Path: ../../../../../../../M5Dial-UserDemo/main/hal/tp/hal_tp.hpp
      Note: M5Dial touch controller reference
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: Minimal LVGL application wiring reference
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: LVGL display port reference
    - Path: 0071-cardputer-adv-photo-timer/main/app_main.cpp
      Note: Timer UI structure reference
    - Path: 0071-cardputer-adv-photo-timer/main/timer_engine.cpp
      Note: Timer engine reference
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-06T19:14:43.069676618-05:00
WhatFor: ""
WhenToUse: ""
---


# M5Dial timer demo analysis, design, and implementation guide

## Executive Summary

This document explains how to create a new M5Dial tutorial project in `esp32-s3-m5` that behaves like a polished timer product instead of a generic firmware sample. The target outcome is a directory such as `esp32-s3-m5/0072-m5dial-timer-demo/` with a clean ESP-IDF structure, a dedicated M5Dial hardware abstraction layer, a small timer state machine, and an LVGL user interface that looks deliberate on the 240x240 round screen.

The key architectural recommendation is to avoid building directly on the old `M5Dial-UserDemo` application stack. That project contains correct board-specific knowledge for the M5Dial display, touch controller, encoder pins, buzzer, and power-hold wiring, but its runtime is organized as a launcher plus many apps, and LVGL is disabled in its default `HAL` path. For a new intern, that is too much surface area. A better approach is to copy only the board-specific hardware knowledge from `M5Dial-UserDemo` and then combine it with the simpler tutorial conventions already used in `esp32-s3-m5`, especially the display and LVGL bring-up patterns in `0025-cardputer-lvgl-demo` and the timer engine patterns in `0071-cardputer-adv-photo-timer`.

The resulting demo should use the encoder as the main control surface:

- rotate to change the timer value or navigate controls
- press to start, pause, or confirm
- optionally use touch for shortcuts such as reset or preset selection
- optionally beep on start, pause, and completion

The user interface should be intentionally designed for the circular display:

- a large center time value
- a circular progress arc or ring
- small labels around the edge for actions
- restrained color usage with one accent color
- minimal clutter

## Problem Statement

The repository already contains two kinds of relevant firmware:

1. `M5Dial-UserDemo`, which proves the M5Dial hardware can be brought up successfully on ESP-IDF 5.4.1 after the compatibility fixes we just made.
2. `esp32-s3-m5`, which contains many smaller, more didactic tutorial projects with clearer project boundaries and more modern patterns.

The problem is that these two strengths are split across different codebases. `M5Dial-UserDemo` knows the board but is not a good onboarding example. `esp32-s3-m5` has good tutorial structure but currently has no dedicated M5Dial tutorial. A new intern asked to build a simple M5Dial timer demo would have to discover:

- how the M5Dial screen is wired
- how the touch controller is initialized
- how the encoder is read
- how to keep the power-hold pin asserted
- how to set up LVGL on top of the display stack
- how to structure a timer engine without blocking the UI
- how to keep the project small enough to fit and remain understandable

This guide closes that gap. It is deliberately detailed because the intended reader is not yet fluent in this codebase or in the ESP-IDF + M5GFX/LVGL split.

## Scope

In scope:

- define the recommended folder layout for a new M5Dial tutorial under `esp32-s3-m5`
- explain the hardware-facing pieces that matter on M5Dial
- explain how to wire M5GFX or LovyanGFX, LVGL, encoder input, touch input, and a timer engine together
- propose a visual design that looks good on the round screen
- provide file-level implementation guidance
- provide pseudocode and API sketches
- provide validation guidance

Out of scope for the first version:

- Wi-Fi
- preset storage
- HTTP API
- screenshot/export automation
- background audio themes or advanced sound synthesis
- a full launcher or app framework

## Current-State Analysis

### What `esp32-s3-m5` already does well

The existing tutorials in `esp32-s3-m5` show a clear pattern: each tutorial is self-contained, has a root `CMakeLists.txt`, a `main/` component, a local `sdkconfig.defaults`, and often a local `partitions.csv`. For example:

- `0015-cardputer-serial-terminal/CMakeLists.txt` reuses vendored `M5GFX` through `EXTRA_COMPONENT_DIRS` instead of copying the library into the tutorial folder.
- `0025-cardputer-lvgl-demo/main/CMakeLists.txt` keeps the main component explicit and small.
- `0025-cardputer-lvgl-demo/sdkconfig.defaults` pins flash size, partition layout, CPU frequency, and LVGL memory defaults.

This matters because the tutorial should be teachable. The intern should be able to answer a simple question such as "where does the display come from?" by reading one or two files, not fifty.

### What `M5Dial-UserDemo` already proves

The old M5Dial firmware contains concrete, board-specific facts that we should trust because they were exercised on hardware:

- `M5Dial-UserDemo/main/hal/hal.cpp:123` shows the board initialization order: power hold, encoder init, I2C init, display init, buzzer init, and optionally LVGL init.
- `M5Dial-UserDemo/main/hal/hal.cpp:152` shows that M5Dial requires an explicit power-hold GPIO to stay on.
- `M5Dial-UserDemo/main/hal/display/hal_display.hpp:20` through `:27` defines the display and backlight pins.
- `M5Dial-UserDemo/main/hal/display/hal_display.hpp:64` and `:65` confirm the panel size is `240x240`.
- `M5Dial-UserDemo/main/hal/hal_common_define.h:14` through `:27` defines the power, encoder, touch I2C, and buzzer pins.
- `M5Dial-UserDemo/main/hal/tp/hal_tp.hpp:18` and `:103` show the touch controller is accessed over legacy I2C at address `0x38`.
- `M5Dial-UserDemo/main/hal/hal.cpp:95` through `:110` shows the encoder uses GPIO `41` and `40` and the center button uses GPIO `42`.

This old code is valuable as a hardware notebook, even if it is not the right runtime architecture for a new tutorial.

### Where the old M5Dial app becomes a poor tutorial base

`M5Dial-UserDemo` is organized as a larger application shell:

- `M5Dial-UserDemo/main/main.cpp:23` constructs a `HAL` object, then launches either factory test or a multi-app launcher loop.
- `M5Dial-UserDemo/main/apps/app.h:22` defines a reusable app lifecycle abstraction with create/resume/running/pause/destroy hooks.
- `M5Dial-UserDemo/main/apps/launcher/launcher.cpp:18` through `:54` builds a circular menu system and app selector.
- `M5Dial-UserDemo/main/CMakeLists.txt:3` and `:12` use recursive source globs to compile the full HAL and all apps.

Those choices are fine for a shipped demo image, but they are a poor starting point for a first M5Dial tutorial because they force the reader to understand:

- a custom app framework
- a menu framework
- a sprite-based rendering model
- several unrelated apps
- legacy compatibility workarounds

### Where the modern tutorial patterns are stronger

The newer tutorials contain cleaner examples of the subsystems we actually want:

- `0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp:60` shows a small, direct LVGL display port that allocates draw buffers and registers a flush callback.
- `0025-cardputer-lvgl-demo/main/app_main.cpp:39` through `:67` shows straightforward LVGL bring-up in `app_main`.
- `0071-cardputer-adv-photo-timer/main/photo_timer_types.h:8` through `:44` defines a clear timer domain model.
- `0071-cardputer-adv-photo-timer/main/timer_engine.cpp:42` through `:216` shows a timer engine that is non-blocking, `esp_timer`-based, and safe to poll from a UI loop.
- `0071-cardputer-adv-photo-timer/main/app_main.cpp:359` through `:400` shows how to build an LVGL screen that is small, navigable, and regularly refreshed.

The strongest design is therefore hybrid:

- M5Dial hardware facts from `M5Dial-UserDemo`
- tutorial project structure from `esp32-s3-m5`
- LVGL port pattern from `0025`
- timer engine pattern from `0071`

## Gap Analysis

There is currently no tutorial in `esp32-s3-m5` that teaches a new engineer how to build for M5Dial specifically. That leaves several concrete gaps.

### Gap 1: no M5Dial board layer in the tutorial repo

`esp32-s3-m5` currently contains board-specific logic for Cardputer and AtomS3R, but not a reusable M5Dial board layer. An intern would otherwise have to discover the right GPIO values from the older project by hand.

### Gap 2: no small timer demo tailored to rotary + round screen interaction

The `0071` photo timer has the right timing ideas, but it targets Cardputer-ADV and uses a different encoder device and UI layout. The M5Dial needs a circular UI and direct rotary interaction, not a list-heavy rectangular workflow.

### Gap 3: old M5Dial code mixes hardware bring-up and product logic

The existing M5Dial firmware entangles board init, app lifecycle, launcher behavior, buzzer feedback, and app logic. A tutorial should isolate those concerns.

### Gap 4: touch and encoder semantics are not normalized

The old M5Dial code uses direct polling:

- encoder motion in `launcher.cpp:105`
- center button in `launcher.cpp:115`
- touch in `launcher.cpp:140`

That works, but a tutorial should define a clear input adapter so that UI code does not need to know where a delta or click came from.

## Proposed Solution

### Recommendation summary

Create a new tutorial project at:

```text
esp32-s3-m5/0072-m5dial-timer-demo/
```

Use this project shape:

```text
0072-m5dial-timer-demo/
├── CMakeLists.txt
├── README.md
├── sdkconfig.defaults
├── partitions.csv
└── main/
    ├── CMakeLists.txt
    ├── app_main.cpp
    ├── m5dial_board.h
    ├── m5dial_board.cpp
    ├── lvgl_port_m5dial.h
    ├── lvgl_port_m5dial.cpp
    ├── timer_model.h
    ├── timer_model.cpp
    ├── timer_controller.h
    ├── timer_controller.cpp
    ├── ui_timer_screen.h
    └── ui_timer_screen.cpp
```

The goal is to keep each file answering one question:

- `m5dial_board.*`: how the physical board is initialized
- `lvgl_port_m5dial.*`: how LVGL talks to the display and inputs
- `timer_model.*`: what the timer state machine is
- `timer_controller.*`: how physical inputs map to domain actions
- `ui_timer_screen.*`: how the timer is rendered
- `app_main.cpp`: how everything is wired together

### High-level architecture

```text
                   +----------------------+
                   |     app_main()       |
                   | boot + wire modules  |
                   +----------+-----------+
                              |
          +-------------------+-------------------+
          |                                       |
          v                                       v
 +--------------------+                 +--------------------+
 |   M5DialBoard      |                 |   TimerModel       |
 | display, touch,    |                 | countdown state,   |
 | encoder, buzzer    |                 | elapsed/remaining  |
 +---------+----------+                 +---------+----------+
           |                                      ^
           v                                      |
 +--------------------+                 +---------+----------+
 | LVGL Port          |<----------------| TimerController    |
 | flush, tick,       | input events    | maps encoder/touch |
 | indev adapters     |                 | to model actions   |
 +---------+----------+                 +---------+----------+
           |                                      |
           +------------------+-------------------+
                              v
                    +--------------------+
                    | UI Timer Screen    |
                    | ring, labels,      |
                    | buttons, states    |
                    +--------------------+
```

### Runtime ownership model

The safest rule is: one task owns LVGL and screen updates.

That is the same core idea used in `0030-cardputer-console-eventbus/main/app_main.cpp:8` through `:10`, where the event loop is designed so the UI task remains the display owner. Even though the M5Dial timer demo can be simpler, the same rule should apply:

- the main loop owns LVGL
- the main loop polls encoder and touch
- the main loop updates the timer model
- the main loop calls `lv_timer_handler()`
- no background task directly modifies LVGL objects

This rule prevents the most common beginner failure: cross-thread UI mutation.

## Design Decisions

### Decision 1: use a new tutorial project, not `M5Dial-UserDemo` directly

Rationale:

- the older project has correct hardware information but excessive runtime complexity
- tutorials in `esp32-s3-m5` are easier to maintain and teach
- a fresh project avoids dragging in unrelated menu and app-framework code

Consequence:

- we will copy board knowledge, not application architecture

### Decision 2: prefer explicit M5Dial board config over autodetect for v1

`0025-cardputer-lvgl-demo/main/app_main.cpp:43` uses `m5gfx::M5GFX display; display.init();`, which is fine when the target board is already supported and autodetect is reliable. For M5Dial, the most evidence-backed path in this repository is still the explicit LovyanGFX panel wiring from `M5Dial-UserDemo/main/hal/display/hal_display.hpp`.

Rationale:

- explicit configuration is easier for an intern to inspect
- the working pin values are already known
- the M5Dial-specific touch and power-hold behavior also need explicit setup anyway

Consequence:

- the tutorial should build a tiny `LGFX_M5Dial` class or equivalent board display wrapper rather than depending on undocumented autodetect behavior

### Decision 3: use LVGL for UI composition, not raw sprite rendering

The old M5Dial demo renders directly to a `LGFX_Sprite` canvas in `M5Dial-UserDemo/main/hal/hal.cpp:46` and its app GUIs. That approach can look good, but it increases the amount of custom drawing code the intern has to own.

LVGL is a better tutorial target because:

- widgets, labels, buttons, and arcs are already implemented
- encoder focus behavior is a first-class concept
- state-driven screen updates are clearer than manual full-screen redrawing
- the tutorial repo already contains a clean LVGL port example in `0025`

Consequence:

- the M5Dial timer demo should use LVGL
- the visual identity should come from layout, colors, and typography choices, not from a hand-built scene graph

### Decision 4: keep the timer engine separate from LVGL

The `0071` photo timer is a strong reference because its `TimerEngine` does not know anything about the display library. `TimerSnapshot` is a domain data structure, not a widget tree. That separation should be preserved in the M5Dial tutorial.

Rationale:

- makes the timer logic testable
- avoids coupling business logic to widget callbacks
- lets future features such as presets or REST APIs reuse the same engine

### Decision 5: design for the round screen from the start

The 240x240 circular display changes layout rules. A rectangular list transplanted onto a round screen usually wastes space and looks accidental. The M5Dial timer should instead treat the device as a dial:

- large center value
- circular progress indicator
- controls placed near the bottom edge or as touch targets around the perimeter

## Proposed UX and Visual Design

### User story

The user should be able to do this without reading instructions:

1. power on
2. see a large default duration such as `05:00`
3. rotate the dial to adjust time
4. press the dial to start
5. rotate during pause to adjust again
6. press again to pause or resume
7. touch a visible reset affordance if they want to clear

### Visual direction

The UI should feel like a physical instrument, not a desktop app squeezed onto a circle.

Recommended style:

- dark charcoal or near-black background
- one saturated accent color, for example amber or warm green
- large monospaced or geometric timer digits
- subtle outer tick marks
- soft arc progress with thick stroke
- small status text at top and minimal action labels at bottom

Suggested screen composition:

```text
          .------------------------.
       .-'                          '-.
     .'   preset / mode label          '.
    /                                    \
   |        12 small tick markers         |
   |                                      |
   |          [ progress arc ]            |
   |                                      |
   |              05:00                   |
   |            READY / RUN               |
   |                                      |
   |    Reset            Start/Pause      |
    \                                    /
     '.                                .'
       '-.                        .-'
          '----------------------'
```

### Interaction rules

- Encoder rotate:
  - idle or paused: adjust time in 5-second or 15-second increments
  - running: optionally disabled, or adjust only when long-press is implemented later
- Encoder press:
  - idle: start
  - running: pause
  - paused: resume
  - complete: reset to idle
- Touch:
  - reset button: reset timer
  - optional quick presets: 1 min, 3 min, 5 min, 10 min

### Sound rules

Sound should be optional in the first implementation. The buzzer path exists and works in the M5Dial firmware after the LEDC fix, but the timer demo should not depend on sound to be usable.

If enabled:

- short beep on start
- lower beep on pause
- double beep on completion

## Detailed Architecture

### Module 1: `m5dial_board.*`

This module hides the board-specific setup:

- assert power hold
- initialize the display and backlight
- initialize touch I2C and the FT3267 controller
- initialize the quadrature encoder
- initialize the center push button
- initialize the buzzer

This should be based on the following old sources:

- `M5Dial-UserDemo/main/hal/hal.cpp`
- `M5Dial-UserDemo/main/hal/display/hal_display.hpp`
- `M5Dial-UserDemo/main/hal/tp/hal_tp.hpp`
- `M5Dial-UserDemo/main/hal/hal_common_define.h`

Recommended board API:

```cpp
struct M5DialBoardConfig {
  int backlight = 255;
};

class M5DialBoard {
 public:
  bool init(const M5DialBoardConfig& cfg = {});

  lgfx::LGFX_Device& display();
  bool touch_read(int* x, int* y, bool* pressed);
  int encoder_take_delta();
  bool encoder_take_press();
  void buzzer_beep(uint32_t hz, uint32_t ms);
};
```

The API should return normalized values. UI code should not need to know which GPIOs are used.

### Module 2: `lvgl_port_m5dial.*`

This module should closely follow `0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp`.

Responsibilities:

- call `lv_init()`
- allocate one or two LVGL draw buffers
- register a display flush callback that pushes pixels to the M5Dial display
- register an encoder input device
- optionally register a touch pointer input device
- start an `esp_timer` tick source for `lv_tick_inc`

Recommended API:

```cpp
struct LvglM5DialConfig {
  int buffer_lines = 40;
  bool double_buffer = true;
  bool swap_bytes = false;
  int tick_ms = 2;
};

bool lvgl_port_m5dial_init(M5DialBoard& board, const LvglM5DialConfig& cfg);
lv_indev_t* lvgl_port_m5dial_encoder_indev();
lv_indev_t* lvgl_port_m5dial_touch_indev();
```

Important design note: keep the flush callback dumb. It should copy the pixels and return. Do not bury business logic inside it.

### Module 3: `timer_model.*`

This should be a simplified version of the `0071` timer engine, because the first M5Dial demo does not need preset chains. A single countdown is enough.

Suggested domain model:

```cpp
enum class TimerState {
  kIdle,
  kRunning,
  kPaused,
  kComplete,
};

struct TimerSnapshot {
  TimerState state = TimerState::kIdle;
  uint32_t total_ms = 0;
  uint32_t remaining_ms = 0;
};

class TimerModel {
 public:
  void set_duration_ms(uint32_t ms);
  void start();
  void pause();
  void resume();
  void toggle();
  void reset();
  void tick(uint64_t now_us);
  TimerSnapshot snapshot() const;
};
```

The `0071` code is valuable reference because:

- `timer_engine.cpp:42` shows clean public methods
- `timer_engine.cpp:111` shows non-blocking update via wall-clock time
- `timer_engine.cpp:118` shows snapshot generation for UI consumption

For a beginner, this structure is easier to reason about than storing `start_ms`, `pause_ms`, and `elapsed_ms` ad hoc across UI callbacks.

### Module 4: `timer_controller.*`

This module translates physical inputs into domain actions.

Responsibilities:

- consume encoder deltas
- convert delta into duration changes when appropriate
- consume button presses
- route touch events to reset or preset actions
- decide when beeps should play

The controller should own policy such as:

- how many seconds each encoder step changes
- whether changes are allowed while running
- whether touch and encoder can trigger the same action

Suggested interface:

```cpp
struct ControllerUpdateResult {
  bool ui_dirty = false;
  bool model_changed = false;
};

ControllerUpdateResult timer_controller_update(
    M5DialBoard& board,
    TimerModel& model,
    UiTimerScreen& ui);
```

### Module 5: `ui_timer_screen.*`

This module owns LVGL objects and nothing else.

It should:

- create the arc, main label, status label, and buttons
- expose a `render(snapshot)` or `update(snapshot)` method
- manage styles centrally
- never read GPIOs directly
- never compute timing directly

Recommended screen API:

```cpp
class UiTimerScreen {
 public:
  void create(lv_obj_t* parent, lv_indev_t* encoder, lv_indev_t* touch);
  void update(const TimerSnapshot& snap);
  bool consume_reset_requested();
  bool consume_preset_minutes(uint32_t* out_minutes);
};
```

## API References and Source Anchors

Use these files as the primary references when implementing:

- Board init order: `M5Dial-UserDemo/main/hal/hal.cpp:123`
- Power hold: `M5Dial-UserDemo/main/hal/hal.cpp:152`
- Display SPI and panel wiring: `M5Dial-UserDemo/main/hal/display/hal_display.hpp:20`
- Backlight PWM config: `M5Dial-UserDemo/main/hal/display/hal_display.hpp:81`
- Encoder/button pins: `M5Dial-UserDemo/main/hal/hal_common_define.h:15`
- Touch I2C pins: `M5Dial-UserDemo/main/hal/hal_common_define.h:20`
- Touch controller class: `M5Dial-UserDemo/main/hal/tp/hal_tp.hpp:108`
- Legacy buzzer wrapper: `M5Dial-UserDemo/main/hal/buzzer/hal_buzzer.hpp:18`
- LVGL display port example: `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp:60`
- LVGL boot wiring example: `esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp:39`
- Timer types example: `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/photo_timer_types.h:8`
- Timer engine example: `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/timer_engine.cpp:42`
- LVGL timer screen example: `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/app_main.cpp:359`

## Pseudocode and Key Flows

### Boot flow pseudocode

```cpp
extern "C" void app_main(void) {
  M5DialBoard board;
  if (!board.init()) {
    log_error_and_return();
  }

  LvglM5DialConfig lv_cfg = {
    .buffer_lines = 40,
    .double_buffer = true,
    .swap_bytes = false,
    .tick_ms = 2,
  };
  if (!lvgl_port_m5dial_init(board, lv_cfg)) {
    log_error_and_return();
  }

  TimerModel model;
  model.set_duration_ms(5 * 60 * 1000);

  UiTimerScreen ui;
  ui.create(lv_scr_act(),
            lvgl_port_m5dial_encoder_indev(),
            lvgl_port_m5dial_touch_indev());
  ui.update(model.snapshot());

  while (true) {
    const uint64_t now_us = esp_timer_get_time();

    timer_controller_update(board, model, ui);
    model.tick(now_us);
    ui.update(model.snapshot());

    lv_timer_handler();
    vTaskDelay(1);
  }
}
```

### Encoder flow pseudocode

```cpp
delta = board.encoder_take_delta();
if (delta != 0) {
  if (model.state() == Idle || model.state() == Paused) {
    duration_ms += delta * 5000;
    duration_ms = clamp(duration_ms, 5s, 99m59s);
    model.set_duration_ms(duration_ms);
  }
}

if (board.encoder_take_press()) {
  model.toggle();
  maybe_beep_for_state(model.snapshot().state);
}
```

### Touch flow pseudocode

```cpp
if (ui.consume_reset_requested()) {
  model.reset();
}

uint32_t preset_minutes = 0;
if (ui.consume_preset_minutes(&preset_minutes)) {
  model.set_duration_ms(preset_minutes * 60 * 1000);
}
```

### UI refresh flow pseudocode

```cpp
void UiTimerScreen::update(const TimerSnapshot& snap) {
  pct = (snap.total_ms == 0) ? 0 : ((snap.total_ms - snap.remaining_ms) * 100) / snap.total_ms;
  lv_arc_set_value(progress_arc_, pct);

  lv_label_set_text(timer_label_, format_mm_ss(snap.remaining_ms).c_str());
  lv_label_set_text(status_label_, state_to_text(snap.state));

  if (snap.state == TimerState::kRunning) {
    apply_running_palette();
  } else if (snap.state == TimerState::kPaused) {
    apply_paused_palette();
  } else if (snap.state == TimerState::kComplete) {
    apply_complete_palette();
  } else {
    apply_idle_palette();
  }
}
```

### Threading and timing flow

```text
main loop
  -> poll inputs
  -> update timer model from esp_timer_get_time()
  -> write snapshot into UI widgets
  -> call lv_timer_handler()
  -> sleep 1 tick
```

This design is intentionally boring. Boring is good for embedded UI reliability.

## Concrete File-by-File Implementation Plan

### Phase 1: scaffold the tutorial directory

Create these files:

- `esp32-s3-m5/0072-m5dial-timer-demo/CMakeLists.txt`
- `esp32-s3-m5/0072-m5dial-timer-demo/README.md`
- `esp32-s3-m5/0072-m5dial-timer-demo/sdkconfig.defaults`
- `esp32-s3-m5/0072-m5dial-timer-demo/partitions.csv`
- `esp32-s3-m5/0072-m5dial-timer-demo/main/CMakeLists.txt`

Use these as direct templates:

- root `CMakeLists.txt`: `0015-cardputer-serial-terminal/CMakeLists.txt`
- local partition layout: `0025-cardputer-lvgl-demo/partitions.csv`
- local defaults: `0025-cardputer-lvgl-demo/sdkconfig.defaults`

Recommended root `CMakeLists.txt` direction:

- reuse vendored graphics component through `EXTRA_COMPONENT_DIRS`
- if you vendor `LovyanGFX` or `M5GFX` specifically for M5Dial, point at that explicit location
- keep the project root minimal

### Phase 2: implement `m5dial_board.*`

Start with the M5Dial hardware constants from:

- `M5Dial-UserDemo/main/hal/hal_common_define.h`
- `M5Dial-UserDemo/main/hal/display/hal_display.hpp`

Implement:

- power hold GPIO
- display init
- touch init
- encoder attach
- button attach
- buzzer init

Important details:

- initialize power hold first, before expensive setup
- initialize I2C before touch
- initialize display before touch if they share reset behavior, matching `hal.cpp:28`
- keep buzzer optional if it complicates first boot

### Phase 3: implement LVGL port

Base this on `0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp`.

Tasks:

- write display flush callback
- allocate DMA-capable draw buffers when available
- register encoder indev
- register touch indev
- create LVGL tick timer with `esp_timer`

Do not reuse the old `M5Dial-UserDemo/main/hal/lvgl/porting/lv_port_indev.cpp` as-is. It is useful as a semantic reference, but the new tutorial should have a smaller, clearer adapter that exposes only the M5Dial inputs you actually need.

### Phase 4: implement `timer_model.*`

Start smaller than `0071`.

Required states:

- idle
- running
- paused
- complete

Required operations:

- set duration
- start
- pause
- resume
- toggle
- reset
- tick
- snapshot

Guardrails:

- never block on countdown using `vTaskDelay(duration)`
- never compute the remaining time from frame-to-frame deltas alone
- always derive remaining time from a stable timestamp and stored start time

### Phase 5: implement `ui_timer_screen.*`

Create one screen only.

Recommended widgets:

- `lv_arc` for the progress ring
- one large `lv_label` for `MM:SS`
- one smaller `lv_label` for status
- one or two `lv_btn` widgets for reset and presets

If the design starts looking rectangular or generic, simplify it. The center timer text and arc should dominate the screen.

### Phase 6: implement `timer_controller.*`

This is where device personality lives.

Examples:

- one encoder detent = five seconds
- if current duration is above ten minutes, one detent can become fifteen seconds
- button press toggles start/pause
- completion triggers a distinct beep pattern

Keep this policy code outside the UI module so it can change without reworking widget structure.

### Phase 7: write README and build instructions

The tutorial README should explicitly use the repo environment:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
source .envrc
cd 0072-m5dial-timer-demo
idf.py build
```

Reference:

- `esp32-s3-m5/.envrc:1`

## Recommended `sdkconfig.defaults`

Start from the same overall shape used in `0025-cardputer-lvgl-demo/sdkconfig.defaults`.

Recommended settings:

- `CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y`
- `CONFIG_ESPTOOLPY_FLASHSIZE="8MB"`
- `CONFIG_PARTITION_TABLE_CUSTOM=y`
- `CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"`
- `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y`
- `CONFIG_ESP_MAIN_TASK_STACK_SIZE=10000`
- `CONFIG_LV_MEM_SIZE_KILOBYTES=64`

Font guidance:

- enable one small font for labels
- enable one large font for the timer digits
- avoid enabling every LVGL font or example because that increases binary size

Memory guidance:

- keep LVGL examples disabled
- keep draw buffers modest, for example `40` lines with double buffering
- if memory is tight, reduce buffer lines before rewriting the architecture

## Implementation Sketches

### Example `main/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS
        "app_main.cpp"
        "m5dial_board.cpp"
        "lvgl_port_m5dial.cpp"
        "timer_model.cpp"
        "timer_controller.cpp"
        "ui_timer_screen.cpp"
    PRIV_REQUIRES esp_timer
    REQUIRES lvgl
    INCLUDE_DIRS "."
)
```

If your board layer depends on a vendored graphics component, add that requirement explicitly once you decide whether the tutorial should use `M5GFX` or `LovyanGFX` naming.

### Example state transition table

```text
State      Event           Next State   Notes
---------  --------------  -----------  ----------------------------
Idle       press           Running      countdown starts
Running    press           Paused       keep remaining time
Paused     press           Running      resume from remaining time
Running    reaches zero    Complete     fire completion beep
Complete   press           Idle         reset to configured duration
Idle       rotate          Idle         duration changes
Paused     rotate          Paused       duration changes
Running    rotate          Running      ignored in v1
```

### Example style palette

```text
Background:  #101416
Primary:     #E0B04A
Running:     #55D17A
Paused:      #F0C15A
Complete:    #FF7A59
Text:        #F6F1E8
Subtext:     #A8B2B0
```

## Alternatives Considered

### Alternative A: reuse `M5Dial-UserDemo` app framework directly

Rejected for the tutorial because:

- too much unrelated code
- harder for an intern to isolate the minimum required system
- recursive source globs and multiple apps make the build surface area larger than necessary

### Alternative B: avoid LVGL and draw everything manually with `LGFX_Sprite`

Rejected for the first tutorial because:

- more custom rendering code
- more manual hit-testing and focus logic
- less reusable if the demo later grows into settings or presets

This can still be a valid second project once the board layer is stable.

### Alternative C: use M5Unified autodetect for everything

Potentially attractive, but currently less evidence-backed in this repository for M5Dial than the explicit board configuration in the older project. It is reasonable to revisit later if a local M5Dial-specific M5Unified path is confirmed stable and simpler.

## Testing and Validation Strategy

### Build validation

Use the repo environment:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
source .envrc
cd 0072-m5dial-timer-demo
idf.py build
```

Check:

- no undefined references
- app fits chosen partition
- no accidental LVGL examples or demos in the build graph

### Flash and boot validation

```bash
idf.py -p /dev/ttyACM0 -b 115200 flash monitor
```

Check for:

- board boots without resets
- display initializes
- no I2C conflict or LEDC timer conflict
- timer screen appears immediately

### Interaction validation

Manually verify:

1. rotate changes duration while idle
2. press starts countdown
3. press pauses
4. press resumes
5. reaching zero changes state to complete
6. reset touch target works
7. optional beep patterns are distinct

### UI quality validation

Ask these concrete questions:

- is the timer legible at arm’s length?
- does the screen still look balanced when showing `00:05` and `59:59`?
- can the user infer that the encoder press starts the timer?
- do touch targets avoid the extreme edges of the round screen?

## Risks

### Risk 1: too much board code is copied from the old project

Mitigation:

- copy facts, not architecture
- rename files clearly
- document every imported constant

### Risk 2: LVGL input feels wrong with the encoder

Mitigation:

- keep direct controller logic for timer adjustments
- use LVGL focus mostly for buttons and optional presets
- do not force every interaction through generic widget navigation if it makes the dial feel worse

### Risk 3: buzzer or backlight PWM conflicts reappear

Mitigation:

- keep the backlight LEDC clock-source fix in mind
- treat sound as optional until the main interaction loop is proven stable

### Risk 4: the project grows into another monolith

Mitigation:

- stop at one screen for v1
- do not add Wi-Fi, storage, or HTTP until the timer demo is stable and understandable

## Open Questions

1. Should the tutorial vendor `LovyanGFX` directly for M5Dial, or introduce an `M5GFX` wrapper convention consistent with the rest of `esp32-s3-m5`?
2. Is touch required in v1, or should the first version be encoder-only with a single reset long-press?
3. Do we want one fixed default duration, or a row of quick presets such as `1m`, `3m`, `5m`, and `10m`?
4. Should sound ship in v1, given the recent LEDC compatibility work, or remain a follow-up enhancement?

## References

### M5Dial hardware references

- `M5Dial-UserDemo/main/main.cpp`
- `M5Dial-UserDemo/main/hal/hal.cpp`
- `M5Dial-UserDemo/main/hal/hal.h`
- `M5Dial-UserDemo/main/hal/hal_common_define.h`
- `M5Dial-UserDemo/main/hal/display/hal_display.hpp`
- `M5Dial-UserDemo/main/hal/tp/hal_tp.hpp`
- `M5Dial-UserDemo/main/hal/buzzer/hal_buzzer.hpp`
- `M5Dial-UserDemo/main/hal/arduino/Tone.cpp`
- `M5Dial-UserDemo/partitions.csv`

### Tutorial structure and LVGL references

- `esp32-s3-m5/.envrc`
- `esp32-s3-m5/0015-cardputer-serial-terminal/CMakeLists.txt`
- `esp32-s3-m5/0025-cardputer-lvgl-demo/CMakeLists.txt`
- `esp32-s3-m5/0025-cardputer-lvgl-demo/sdkconfig.defaults`
- `esp32-s3-m5/0025-cardputer-lvgl-demo/partitions.csv`
- `esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp`
- `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp`
- `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.h`

### Timer architecture references

- `esp32-s3-m5/0071-cardputer-adv-photo-timer/README.md`
- `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/photo_timer_types.h`
- `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/timer_engine.cpp`
- `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/app_main.cpp`

## Problem Statement

<!-- Describe the problem this design addresses -->

## Proposed Solution

<!-- Describe the proposed solution in detail -->

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
