---
Title: PaperS3 touch draw demo analysis design and implementation guide
Ticket: ESP-31-PAPERS3-DRAW-DEMO
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp
      Note: EPD write semantics
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp
      Note: IT8951 update mode mapping
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5Unified/src/utility/Touch_Class.hpp
      Note: M5Unified touch detail API
    - Path: ../../../../../../../M5PaperS3-UserDemo/main/hal/hal.h
      Note: Donor touch access pattern
    - Path: 0075-papers3-touch-draw-demo/main/app_main.cpp
      Note: Main application walkthrough and API references
    - Path: 0075-papers3-touch-draw-demo/sdkconfig.defaults
      Note: Console and target configuration
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T20:02:37.392908909-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 touch draw demo analysis design and implementation guide

## Executive Summary

This guide explains how the new `PaperS3` touch-draw demo works, why it is structured the way it is, and what a new intern must understand before changing it. The demo is intentionally small. It does not try to preserve the donor app’s multi-screen framework. Instead, it extracts the proven hardware path from `M5PaperS3-UserDemo` and wraps it in a single-purpose application that is easy to reason about.

At a high level, the system works like this:

1. `M5.begin()` initializes the PaperS3 board through `M5Unified`.
2. `M5.Display.setRotation(1)` rotates the e-paper panel into the intended landscape coordinate system.
3. `M5.Touch.getCount()` and `M5.Touch.getDetail()` expose GT911 touch points through `M5Unified`.
4. The app decides whether the finger gesture belongs to the canvas or the `CLEAR` button.
5. Canvas gestures draw black brush stamps using `epd_fast`.
6. `CLEAR` triggers a full UI redraw using `epd_text`.

That is the whole product. The simplicity is deliberate.

## Problem Statement And Scope

The requested outcome was a small demo for the PaperS3 that:

- uses ESP-IDF 5.3.4
- starts from the existing `M5PaperS3-UserDemo` for display and GT911 knowledge
- lets the user draw on the screen with touch
- gives the user a clear button
- is documented well enough for a new intern to understand and continue

The scope of this implementation is narrow on purpose:

- one screen
- one drawing surface
- one clear action
- one finger input path

Out of scope:

- stroke undo
- save/load
- pressure or thickness control
- gesture recognition beyond draw-vs-clear routing
- production UX polish beyond a stable demonstration

## Current-State Analysis

### What the donor app proves

The donor app already demonstrates the minimum board bring-up sequence needed for this task. In its HAL, `Hal::init()` calls `M5.begin()` and then sets display rotation to `1` before starting other subsystems. That is the key observation that let us keep the new app simple.

Evidence:

- donor initialization in `M5PaperS3-UserDemo/main/hal/hal.cpp:41-53`

The donor HAL also exposes touch in the exact style we need:

- `M5.Touch.getCount() > 0` for “is there touch input right now?”
- `M5.Touch.getDetail()` for the current touch point

Evidence:

- donor touch wrapper in `M5PaperS3-UserDemo/main/hal/hal.h:99-110`

The donor main loop confirms that the runtime model is “call `M5.update()` continuously, then consume input and update rendering”:

- `M5.update()` in `M5PaperS3-UserDemo/main/main.cpp:93-99`

### What `M5Unified` exposes for touch

The touch interface is richer than this demo strictly needs. `touch_detail_t` includes:

- current `x` and `y`
- previous `prev_x` and `prev_y`
- base position for gesture measurement
- press/hold/flick/drag state helpers

Evidence:

- touch state enum and helper methods in `M5Unified/src/utility/Touch_Class.hpp:13-35`
- `touch_detail_t` fields and helpers in `M5Unified/src/utility/Touch_Class.hpp:49-91`
- `getCount()` and `getDetail()` in `M5Unified/src/utility/Touch_Class.hpp:94-112`

The intern should understand an important design choice: the current demo only uses a subset of what is available. It reads the current position and routes a gesture based on where it starts. It does not yet use drag/flick/hold states explicitly.

### What `M5GFX` exposes for e-paper updates

The most important display fact is this line from `LGFXBase.hpp`:

- drawing after `startWrite()` is reflected on EPD targets by calling `endWrite()`

Evidence:

- `M5GFX/src/lgfx/v1/LGFXBase.hpp:131-139`

This gives the correct mental model:

- `startWrite()` begins a batch of draw commands
- each draw command marks dirty e-paper regions internally
- `endWrite()` causes the EPD update to happen

The IT8951 driver also shows how update modes are selected:

- `epd_fastest -> DU4`
- `epd_fast -> DU`
- `epd_text -> GL16`
- default/quality -> GC16`

Evidence:

- `M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp:448-476`

This is why the implementation uses one mode for full-screen chrome and another for live drawing.

## Gap Analysis

The donor project is not wrong, but it is not the right shape for the requested deliverable.

Observed gaps:

- It is a multi-app shell, not a focused demo.
- It carries RTC, power, SD card, buzzer, IMU, and Wi-Fi code that is irrelevant to touch drawing.
- It assumes a background refresh pattern for a more complex UI.
- It is harder for a new engineer to identify which code is essential.

The new project closes those gaps by:

- removing all nonessential subsystems
- keeping one screen and one state machine
- using local vendored components rather than adding new dependency machinery
- writing documentation that names each layer and its responsibility

## Proposed Architecture And APIs

### File-level architecture

```text
0075-papers3-touch-draw-demo/
├── CMakeLists.txt
├── README.md
├── dependencies.lock
├── partitions.csv
├── sdkconfig.defaults
└── main/
    ├── CMakeLists.txt
    └── app_main.cpp
```

Responsibilities:

- `CMakeLists.txt`
  - points the build to the donor components
  - defines the project name
- `sdkconfig.defaults`
  - pins target, flash, PSRAM, and console policy
- `README.md`
  - tells the next engineer how to build and flash with the exact 5.3.4 export path
- `main/app_main.cpp`
  - contains the demo runtime

### Runtime objects inside `app_main.cpp`

The app defines:

- `Rect`
  - small helper for hit testing
- `Point`
  - integer screen coordinate pair
- `PaperS3DrawDemo`
  - the main application object

`PaperS3DrawDemo` owns:

- `clear_button_`
- `canvas_`
- `touch_down_`
- `clear_gesture_armed_`
- `stroke_active_`
- `last_touch_`
- `last_stroke_point_`

This is enough state to answer all runtime questions:

- is a finger down?
- did the gesture begin on the clear button?
- are we currently drawing?
- where was the last touch point?

### Initialization flow

Relevant implementation:

- board init in `0075-papers3-touch-draw-demo/main/app_main.cpp:63-73`
- layout build in `0075-papers3-touch-draw-demo/main/app_main.cpp:75-93`
- initial full redraw in `0075-papers3-touch-draw-demo/main/app_main.cpp:95-118`
- main loop in `0075-papers3-touch-draw-demo/main/app_main.cpp:41-52`

The flow is:

```text
app_main
  -> PaperS3DrawDemo::run
    -> initBoard
      -> M5.config
      -> M5.begin
      -> M5.Display.setRotation(1)
    -> buildLayout
    -> redrawUi
    -> forever:
         M5.update
         handleTouch
         M5.delay(12)
```

### Gesture routing flow

Relevant implementation:

- `handleTouch()` in `0075-papers3-touch-draw-demo/main/app_main.cpp:188-233`

The routing rule is:

- first touch inside button: arm clear gesture
- first touch inside canvas: start stroke
- movement while stroke is active: draw segments
- release while clear gesture is still over button: clear screen

Pseudo-code:

```text
if touch exists:
  read current point

  if this is a new finger-down event:
    if point in clear_button:
      clear_gesture_armed = true
      stroke_active = false
    else if point in canvas:
      clear_gesture_armed = false
      stroke_active = true
      last_stroke_point = clamp(point)
      drawBrushStroke(last_stroke_point, last_stroke_point)
  else:
    if stroke_active:
      next = clamp(point)
      if next != last_stroke_point:
        drawBrushStroke(last_stroke_point, next)
        last_stroke_point = next
else if finger was previously down:
  if clear_gesture_armed and last_touch still inside clear_button:
    redrawUi()
  reset gesture flags
```

### Stroke rendering flow

Relevant implementation:

- `drawBrushStroke()` in `0075-papers3-touch-draw-demo/main/app_main.cpp:160-186`

The method performs six important steps:

1. clamp both endpoints to the canvas
2. compute `dx`, `dy`, and `steps`
3. switch to `epd_fast`
4. call `startWrite()`
5. set a clip rect to the interior of the canvas
6. stamp filled circles from `from` to `to`
7. clear clip rect
8. call `endWrite()`

Why circles instead of a one-pixel line:

- a single-pixel line can look sparse when touch sampling skips positions
- a filled-circle brush hides sample gaps
- the brush radius is small enough to stay responsive

### UI redraw flow

Relevant implementation:

- `redrawUi()` in `0075-papers3-touch-draw-demo/main/app_main.cpp:95-118`
- `drawClearButton()` in `0075-papers3-touch-draw-demo/main/app_main.cpp:120-134`
- `drawCanvasFrame()` in `0075-papers3-touch-draw-demo/main/app_main.cpp:136-141`

The redraw path intentionally resets the entire screen state:

- wait for any in-flight update
- switch to `epd_text`
- fill the background white
- redraw header
- redraw subtitle
- redraw clear button
- redraw canvas border
- commit with `endWrite()`
- wait for completion

This is exactly what we want for the `CLEAR` action.

## System Diagram

```text
Finger on panel
    |
    v
GT911 touch controller
    |
    v
M5GFX touch driver (Touch_GT911)
    |
    v
M5Unified Touch_Class
    |
    v
PaperS3DrawDemo::handleTouch()
    |                     |
    |                     +--> point starts in clear button -> redrawUi()
    |
    +--> point starts in canvas -> drawBrushStroke()
                                      |
                                      v
                           M5.Display.startWrite()
                                      |
                                      v
                           fillCircle stamps with clip rect
                                      |
                                      v
                           M5.Display.endWrite()
                                      |
                                      v
                            IT8951 partial EPD update
```

## Detailed File Walkthrough

### `0075-papers3-touch-draw-demo/CMakeLists.txt`

Evidence:

- `0075-papers3-touch-draw-demo/CMakeLists.txt:1-6`

Important points:

- The project is normal ESP-IDF CMake boilerplate.
- The only special line is `EXTRA_COMPONENT_DIRS`.
- The path must go up two levels because the new project lives inside `esp32-s3-m5/`, while the donor lives beside that repo directory.

This path mistake was the only real implementation bug during bring-up.

### `0075-papers3-touch-draw-demo/sdkconfig.defaults`

Evidence:

- `0075-papers3-touch-draw-demo/sdkconfig.defaults:1-22`

Important points:

- target is pinned to `esp32s3`
- flash size is pinned to 16 MB
- PSRAM is enabled
- console prefers USB Serial/JTAG

Why USB Serial/JTAG matters here:

- the workspace instructions explicitly prefer it for ESP32-S3 console work
- UART pins are often repurposed on M5Stack devices
- this prevents console output from competing with board-specific peripherals

### `0075-papers3-touch-draw-demo/main/app_main.cpp`

Evidence:

- constants and helpers: `:8-37`
- app lifecycle: `:39-52`
- board init: `:63-73`
- layout: `:75-93`
- UI redraw: `:95-118`
- stroke drawing: `:160-186`
- touch routing: `:188-233`

This file is intentionally the center of gravity for the tutorial. A new intern should read it top to bottom before touching anything else.

### `M5PaperS3-UserDemo/main/hal/hal.cpp`

Evidence:

- `M5PaperS3-UserDemo/main/hal/hal.cpp:41-53`

This is the donor evidence for the minimal initialization pattern. The new demo keeps that pattern and drops everything after it that is not needed for drawing.

### `M5Unified/src/utility/Touch_Class.hpp`

Evidence:

- `Touch_Class.hpp:49-99`

A new intern should learn two things here:

- touch points already carry previous-position data
- the API supports richer gesture classification than this demo uses

### `M5GFX` EPD write path

Evidence:

- `LGFXBase.hpp:131-139`
- `Panel_IT8951.cpp:448-476`

These files explain why the rendering code is structured around mode selection plus `startWrite()`/`endWrite()`.

## Pseudocode By Responsibility

### Boot and setup

```text
cfg = M5.config()
cfg.clear_display = true
M5.begin(cfg)
M5.Display.setRotation(1)
buildLayout()
redrawUi()
```

### Canvas drawing

```text
from = clampToCanvas(from)
to = clampToCanvas(to)
steps = max(abs(dx), abs(dy))
set epd mode fast
startWrite()
setClipRect(canvas interior)
for each interpolated point:
  fillCircle(point, brush_radius, black)
clearClipRect()
endWrite()
```

### Clear action

```text
waitDisplay()
set epd mode text
startWrite()
fillScreen(white)
draw header
draw clear button
draw canvas border
endWrite()
waitDisplay()
```

## Implementation Phases For An Intern

### Phase 0: Build literacy

Before editing code, the intern should be able to answer:

- where does the project get `M5Unified` from?
- which IDF version is required?
- which file controls the console transport?
- which file contains the entire app state machine?

If the intern cannot answer those four questions, they should not start modifying behavior yet.

### Phase 1: Safe cosmetic edits

Good first edits:

- change title text
- move the clear button
- change canvas border thickness
- change brush radius

Why first:

- these changes teach layout and drawing without risking bring-up

### Phase 2: Input behavior changes

Next edits:

- use `detail.prev_x` and `detail.prev_y` explicitly
- add minimum movement thresholds
- highlight the clear button while it is pressed

Why second:

- these changes teach touch routing and state transitions

### Phase 3: Architecture extensions

Later edits:

- store strokes in memory
- replay strokes after a periodic full refresh
- add save/load to SD card
- add multiple pen sizes

Why third:

- these changes require new state models and are meaningfully more complex

## Test Strategy

### Minimum verification already completed

Completed:

- `source /home/manuel/esp/esp-idf-5.3.4/export.sh`
- `idf.py build`

Result:

- build succeeded
- binary size was `0x667f0`
- smallest app partition was `0x400000`

### Hardware verification still recommended

On-device checklist:

1. flash firmware to a PaperS3
2. confirm boot screen appears in landscape orientation
3. confirm finger inside canvas draws black strokes
4. confirm finger in header area does not draw
5. confirm tapping `CLEAR` redraws a clean UI
6. confirm console output is visible over USB Serial/JTAG

### Failure modes to watch for

- no touch response:
  - likely board detection or touch init issue
- mirrored or rotated touch:
  - likely coordinate-space mismatch after rotation
- drawing leaks into header:
  - likely missing or incorrect clip rect
- clear button also draws:
  - gesture routing bug
- ghosting after long drawing sessions:
  - expected tradeoff from repeated fast EPD updates

## Risks, Alternatives, And Open Questions

### Risks

- Long drawing sessions may accumulate ghosting.
- Real hardware touch cadence may differ from expectations.
- Because the app redraws directly to the panel, advanced features like undo will need a separate stroke model.

### Good next alternative if the demo grows

If the demo becomes more than a demo, the next architecture should:

- store strokes in RAM
- separate model from rendering
- provide a full replay path after clear or periodic quality refresh

### Open questions

- Should a future version force a periodic `epd_text` or `epd_quality` replay to reduce ghosting?
- Should the clear button trigger on press-down instead of release-inside?
- Should we expose multiple pen widths for touch experimentation?

## References

- `0075-papers3-touch-draw-demo/main/app_main.cpp`
- `0075-papers3-touch-draw-demo/CMakeLists.txt`
- `0075-papers3-touch-draw-demo/sdkconfig.defaults`
- `0075-papers3-touch-draw-demo/README.md`
- `M5PaperS3-UserDemo/main/hal/hal.cpp`
- `M5PaperS3-UserDemo/main/hal/hal.h`
- `M5PaperS3-UserDemo/main/main.cpp`
- `M5PaperS3-UserDemo/components/M5Unified/src/utility/Touch_Class.hpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp`
