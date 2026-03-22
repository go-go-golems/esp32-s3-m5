---
Title: PaperS3 protractor gesture trainer analysis design and implementation guide
Ticket: ESP-32-PAPERS3-PROTRACTOR
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
      Note: |-
        startWrite and endWrite semantics for EPD
        EPD startWrite/endWrite semantics
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp
      Note: |-
        EPD mode mapping
        IT8951 update mode mapping for epd_fast and epd_text
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/touch/Touch_GT911.cpp
      Note: |-
        GT911 device driver path
        Underlying GT911 touch driver implementation
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5Unified/src/utility/Touch_Class.hpp
      Note: |-
        Touch event and gesture API
        M5Unified touch abstraction used by the trainer
    - Path: 0076-papers3-protractor-trainer/CMakeLists.txt
      Note: Donor component reuse
    - Path: 0076-papers3-protractor-trainer/main/protractor_math.cpp
      Note: |-
        Protractor math implementation
        Detailed algorithm walkthrough and source-of-truth implementation
    - Path: 0076-papers3-protractor-trainer/main/protractor_math.h
      Note: Public algorithm API
    - Path: 0076-papers3-protractor-trainer/main/trainer_app.cpp
      Note: |-
        UI, input routing, and rendering
        Detailed runtime
    - Path: 0076-papers3-protractor-trainer/main/trainer_app.h
      Note: Application state and layout structures
    - Path: 0076-papers3-protractor-trainer/sdkconfig.defaults
      Note: Board and console configuration
ExternalSources:
    - local:protractor_gesture_recognizer_demo.html
Summary: ""
LastUpdated: 2026-03-21T20:26:40.608550353-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 protractor gesture trainer analysis design and implementation guide

## Executive Summary

This guide explains the full system behind `0076-papers3-protractor-trainer` for a new intern. It covers the hardware path, donor software stack, Protractor algorithm, PaperS3 UI design, runtime state machine, and the exact files that implement each part.

The most important idea is that this app is two systems joined together:

- a board-facing system that gets touch points from GT911 and draws onto an e-paper screen
- an algorithm-facing system that turns a stroke into a normalized vector and compares it to saved templates

The final product looks simple, but it only works if both halves are understood.

## Problem Statement

The imported browser demo already showed a useful interaction loop:

- draw a gesture
- record templates
- recognize the next stroke against those templates
- inspect preprocessing and similarity results

The problem was to recreate that loop on a `PaperS3` device without a browser runtime, while still using the donor display and touch stack from `M5PaperS3-UserDemo`. That means replacing:

- browser canvas drawing with `M5GFX`
- pointer events with `M5Unified` touch polling
- HTML/CSS layout with device-side geometry and drawing code
- text-entry template naming with a touch-first alternative

The solution chosen here is a fixed-slot trainer. Instead of naming templates with a keyboard, the user picks slot `A` through `H`, draws, saves, and then compares future gestures against the saved slot vectors.

## Proposed Solution

The implementation uses the following structure:

```text
Finger on screen
  -> GT911 controller
  -> M5GFX GT911 driver
  -> M5Unified Touch_Class facade
  -> TrainerApp::HandleTouch()
  -> raw stroke points
  -> Resample() + Vectorize()
  -> OptimalCosineDistance() against saved templates
  -> ranked results rendered on PaperS3
```

The code is divided so that each layer has one job:

```text
app_main.cpp
  boots the app

trainer_app.h / trainer_app.cpp
  owns layout, state, touch routing, actions, and rendering

protractor_math.h / protractor_math.cpp
  owns resampling, centroid math, vectorization, and similarity scoring

CMakeLists.txt / sdkconfig.defaults
  wire in donor components and target-specific configuration
```

This is a deliberate teaching-oriented design. It is easier to explain than a highly compact implementation.

## System Inventory

### 1. Hardware-facing touch path

At the lowest practical level we care about, the touch controller is GT911. The donor graphics stack already includes a GT911 implementation in `Touch_GT911.cpp`, which initializes the controller over I2C and fetches raw points during updates.

Relevant references:

- `Touch_GT911::init()` in `M5GFX/src/lgfx/v1/touch/Touch_GT911.cpp:51-89`
- `_update_data()` in `M5GFX/src/lgfx/v1/touch/Touch_GT911.cpp:139-167`
- `getTouchRaw()` in `M5GFX/src/lgfx/v1/touch/Touch_GT911.cpp:169-208`

What this means in practice:

- the app does not talk to GT911 directly
- donor components already know how to bring the controller up
- the app can stay at the `M5.Touch` abstraction layer

### 2. Touch abstraction used by the app

The app uses `M5Unified` rather than raw driver calls. `Touch_Class.hpp` defines the user-facing touch API:

- `getCount()` tells us how many touch points are active
- `getDetail()` returns the touch details for a point
- `touch_detail_t` exposes `x`, `y`, previous position, base position, and gesture-state helpers

Relevant references:

- touch states in `M5Unified/src/utility/Touch_Class.hpp:13-35`
- `touch_detail_t` in `M5Unified/src/utility/Touch_Class.hpp:49-91`
- `getCount()` and `getDetail()` in `M5Unified/src/utility/Touch_Class.hpp:94-111`

This app intentionally uses only a subset:

- `getCount()`
- `getDetail().x`
- `getDetail().y`

That is enough because the gesture routing policy is simple: route by where the finger starts, then track until release.

### 3. E-paper rendering path

The PaperS3 display pipeline comes from donor `M5GFX`. One of the most important facts for an intern is in `LGFXBase.hpp`: for EPD targets, drawing performed after `startWrite()` is reflected when `endWrite()` is called.

Relevant reference:

- `LGFXBase.hpp:131-139`

This matters because the app uses two drawing styles:

- live partial updates for drawing ink
- full-screen redraws for stable UI panels and text

The IT8951 panel driver maps e-paper modes like this:

- `epd_fastest -> DU4`
- `epd_fast -> DU`
- `epd_text -> GL16`
- default -> `GC16`

Relevant reference:

- `Panel_IT8951.cpp:448-476`

That is why the implementation chooses:

- `epd_fast` during live stroke stamping
- `epd_text` for full UI redraws

### 4. Donor component reuse

The new tutorial project does not vendor a second copy of the donor libraries. Instead, it points at the existing donor component tree:

- `0076-papers3-protractor-trainer/CMakeLists.txt:1-6`

This is a strong choice for both engineering and documentation:

- one source of truth for board support
- no dependency download step
- tutorial app stays small

### 5. Target configuration

The project pins itself to `esp32s3`, sets the custom partition table, enables PSRAM, and prefers USB Serial/JTAG for the console:

- `0076-papers3-protractor-trainer/sdkconfig.defaults:1-22`

This follows the local repo policy for ESP32-S3 console selection and prevents conflicts with peripheral UART usage.

## Current App Walkthrough

### `app_main.cpp`

`app_main.cpp` is intentionally minimal:

- create `TrainerApp`
- call `Run()`

Reference:

- `0076-papers3-protractor-trainer/main/app_main.cpp:1-6`

This is good design for an embedded tutorial. The entrypoint is not where the complexity lives.

### `trainer_app.h`

`trainer_app.h` defines the long-lived state of the product:

- geometry rectangles through `Rect`
- template slot storage through `TemplateSlot`
- ranked results through `RecognitionScore`
- action routing through `ActionButton`
- app state in `TrainerApp`

Relevant references:

- `Rect` in `trainer_app.h:14-24`
- slot/result/action definitions in `trainer_app.h:31-52`
- runtime methods in `trainer_app.h:54-86`
- state members in `trainer_app.h:88-117`

An intern should pause here and understand the state model before reading the implementation. Almost every runtime behavior is a mutation of these fields.

### `protractor_math.h`

The algorithm API is deliberately small:

- `PathLength`
- `Centroid`
- `Resample`
- `Vectorize`
- `OptimalCosineDistance`

Reference:

- `protractor_math.h:8-25`

This small API boundary is intentional. The UI does not need to know internal math details.

## Imported Browser Demo Analysis

The imported HTML source is important because it was the direct algorithm and layout reference for this app.

### Algorithm primitives copied structurally

The browser demo implements:

- `resample()` at `sources/local/protractor_gesture_recognizer_demo.html:134-154`
- `vectorize()` at `sources/local/protractor_gesture_recognizer_demo.html:156-176`
- `optCosDistance()` at `sources/local/protractor_gesture_recognizer_demo.html:178-187`
- `recognize()` at `sources/local/protractor_gesture_recognizer_demo.html:189-198`

The C++ port in `protractor_math.cpp` stays structurally aligned:

- `Resample()` at `protractor_math.cpp:49-104`
- `Vectorize()` at `protractor_math.cpp:106-154`
- `OptimalCosineDistance()` at `protractor_math.cpp:156-172`

This is valuable for debugging. If an intern suspects a recognition bug, they can compare the C++ flow against the HTML source almost line by line.

### Layout ideas adapted from the browser version

The browser demo has:

- template chips and a template count
- record / clear / reset actions
- preprocessing stats
- recognition score bars

Relevant references:

- vector preview bars in `sources/local/protractor_gesture_recognizer_demo.html:320-328`
- score rows in `sources/local/protractor_gesture_recognizer_demo.html:331-343`
- template chips in `sources/local/protractor_gesture_recognizer_demo.html:345-359`
- record / clear / reset handlers in `sources/local/protractor_gesture_recognizer_demo.html:361-383`

The PaperS3 app preserves the same conceptual panels, but changes the input model:

- browser “record with name” becomes PaperS3 “save into selected slot”
- browser chip click-to-remove becomes slot select + explicit `DELETE SLOT`
- browser score list becomes recognition bars in the right-hand card

This is a faithful adaptation rather than a literal port.

## Data Model

The key runtime data structures are:

- `raw_points_`
  - the exact finger path as sampled on-device
- `resampled_points_`
  - 16 evenly spaced points derived from the raw path
- `current_gesture_`
  - centroid, indicative angle, rotation delta, normalized vector, and validity
- `slots_`
  - fixed-size array of trained template vectors
- `recognition_scores_`
  - sorted match results for display

The life of a stroke is:

```text
touch move samples
  -> raw_points_
  -> FinishStroke()
  -> AnalyzeStroke()
  -> Resample(raw_points_, 16)
  -> Vectorize(resampled_points_)
  -> RecognizeCurrentStroke()
  -> recognition_scores_
```

Relevant references:

- stroke analysis in `trainer_app.cpp:223-239`
- recognition loop in `trainer_app.cpp:259-275`

## Detailed Algorithm Explanation

### Step 1: Path length

`PathLength()` sums Euclidean distance across adjacent raw points:

- `protractor_math.cpp:19-28`

Why it matters:

- resampling needs the total path length
- fixed-count samples require a consistent interval size

### Step 2: Resampling

`Resample()` transforms an arbitrary-length stroke into exactly 16 points:

- `protractor_math.cpp:49-104`

Conceptually:

```text
interval = total_path_length / (N - 1)
walk segment by segment
when accumulated length crosses interval:
  interpolate a new point
  insert it
repeat until N points exist
```

Pseudocode:

```text
function resample(points, target_count):
  if no points: return []
  if one point: repeat it target_count times
  work = copy(points)
  output = [work[0]]
  interval = path_length(work) / (target_count - 1)
  D = 0
  i = 1
  while i < len(work):
    d = distance(work[i-1], work[i])
    if d == 0:
      i += 1
      continue
    if D + d >= interval:
      q = interpolate(work[i-1], work[i], (interval - D) / d)
      output.push(q)
      insert q into work at i
      D = 0
    else:
      D += d
      i += 1
  pad with last point until target_count
  return output[:target_count]
```

Why an intern should care:

- recognition quality depends heavily on this step
- if the resampler is wrong, every later metric becomes misleading

### Step 3: Centroid and centering

`Centroid()` computes the arithmetic mean of all point coordinates:

- `protractor_math.cpp:30-47`

`Vectorize()` then shifts every point so the centroid becomes the origin:

- `protractor_math.cpp:113-119`

This removes translation from the recognition problem. The system should not care where in the canvas the user drew the shape.

### Step 4: Indicative angle and rotation delta

`Vectorize()` computes the indicative angle from the first centered point:

- `protractor_math.cpp:121`

Then it chooses the rotation delta:

- orientation-sensitive mode snaps to the nearest 45-degree bucket
- orientation-invariant mode simply negates the indicative angle

Relevant references:

- `protractor_math.cpp:121-128`

This app currently uses the orientation-invariant default because it is better for a general-purpose trainer UI.

### Step 5: Normalize into a vector

After rotating each centered point, `Vectorize()` flattens them into:

```text
[x1, y1, x2, y2, ..., x16, y16]
```

Then it divides by magnitude to normalize the vector:

- `protractor_math.cpp:130-153`

The result is stored in `VectorizedGesture.vector`.

### Step 6: Compare with optimal cosine distance

`OptimalCosineDistance()` calculates the best rotational alignment between two normalized vectors and returns an angular distance:

- `protractor_math.cpp:156-172`

The trainer app then turns that distance into a display-friendly cosine score:

- `trainer_app.cpp:263-268`

Lower distance is better, and `cos(distance)` closer to `1.0` is better.

## Runtime Flow

### App startup

`TrainerApp::Run()` does exactly three important things before entering the loop:

1. `InitBoard()`
2. `BuildLayout()`
3. `RenderFullUi()`

Reference:

- `trainer_app.cpp:52-60`

### Board initialization

`InitBoard()`:

- gets the default config
- enables display clearing
- calls `M5.begin(cfg)`
- rotates the display into landscape
- configures text defaults
- initializes slot labels `A` through `H`

Reference:

- `trainer_app.cpp:62-75`

### Layout construction

`BuildLayout()` creates all rectangles for:

- header
- canvas card
- canvas interior
- templates card
- controls card
- metrics card
- results card
- each slot chip
- each action button

Reference:

- `trainer_app.cpp:77-116`

An intern should note that layout is static, not responsive. That is appropriate here because the target hardware resolution is fixed.

### Touch handling

The touch state machine lives in `HandleTouch()`:

- `trainer_app.cpp:323-412`

Behavior:

- if a touch begins in the canvas, start a stroke
- if it begins on a slot, arm a slot selection
- if it begins on a button, arm that action
- while drawing, append points and stamp ink
- on release, either finish the stroke or confirm the tap action

Diagram:

```text
touch begin
  |
  +-- inside canvas -> drawing_ = true -> ExtendStroke() on move -> FinishStroke() on release
  |
  +-- inside slot   -> remember pressed_slot_ -> select slot on release
  |
  +-- inside button -> remember pressed_action_ -> run action on release
```

This “arm on press, confirm on release” pattern is what prevents accidental action triggers during finger movement.

### Stroke lifecycle

Key methods:

- `BeginStroke()` at `trainer_app.cpp:193-208`
- `ExtendStroke()` at `trainer_app.cpp:210-221`
- `FinishStroke()` at `trainer_app.cpp:223-228`
- `AnalyzeStroke()` at `trainer_app.cpp:230-239`

The app resets recognition state when a new stroke begins, then rebuilds recognition state when the stroke ends.

### Template lifecycle

Key methods:

- `SaveToSelectedSlot()` at `trainer_app.cpp:277-289`
- `DeleteSelectedSlot()` at `trainer_app.cpp:291-301`
- `ClearStroke()` at `trainer_app.cpp:303-312`
- `ResetAll()` at `trainer_app.cpp:314-321`

Important distinction:

- `ClearStroke()` preserves template training data
- `ResetAll()` clears everything

## Rendering Model

### Full UI redraw

`RenderFullUi()` switches to `epd_text`, fills the full screen, and redraws all cards:

- `trainer_app.cpp:414-430`

This redraw is heavier but produces a stable, readable screen.

### Live drawing

`DrawLiveStrokeSegment()` switches to `epd_fast`, clips to the canvas, and stamps circles along the line segment:

- `trainer_app.cpp:161-191`

This is one of the most important implementation details in the whole app. The intern should understand why circles are stamped instead of drawing only a single line:

- touch sampling can be sparse relative to finger speed
- stamping fills gaps and makes strokes look continuous
- clip rect prevents accidental drawing outside the canvas

### Overlay rendering

`DrawGestureOverlay()` renders:

- the raw stroke
- resampled points
- centroid indicator
- indicative angle line

Reference:

- `trainer_app.cpp:583-620`

This overlay is pedagogically important. It turns the app into an explainer, not only a recognizer.

## UI Design Decisions

### Why the UI uses cards

The PaperS3 display is large enough for side-by-side structure. The card layout makes responsibilities obvious:

- left: gesture entry
- right upper: training controls
- right middle: preprocessing metrics
- right lower: recognition results

This echoes the HTML demo’s panel organization while remaining device-native.

### Why slots are labeled with `*`

In `DrawTemplatesCard()`, recorded slots are shown as `A*`, `B*`, and so on:

- `trainer_app.cpp:466-489`

That gives the user fast visibility into which slots have been trained.

### Why the best match is highlighted in two places

The best match is expressed by:

- `matched_slot_` for the slot border
- the first recognition bar being darker

This reduces the chance that the user misses the result.

## File-by-File Reading Order For A New Intern

Read the code in this order:

1. `0076-papers3-protractor-trainer/README.md`
   - learn the product and build commands
2. `0076-papers3-protractor-trainer/main/protractor_math.h`
   - learn the algorithm API surface
3. `0076-papers3-protractor-trainer/main/protractor_math.cpp`
   - understand the algorithm port
4. `0076-papers3-protractor-trainer/main/trainer_app.h`
   - understand state and structure
5. `0076-papers3-protractor-trainer/main/trainer_app.cpp`
   - understand runtime behavior
6. imported `sources/local/protractor_gesture_recognizer_demo.html`
   - compare the original browser version
7. donor `Touch_Class.hpp`, `LGFXBase.hpp`, `Panel_IT8951.cpp`, `Touch_GT911.cpp`
   - understand the supporting hardware abstractions

## Debugging Guide

### Symptom: drawing appears but recognition never matches

Check:

- is `raw_points_` large enough?
- is `resampled_points_` length 16?
- is `current_gesture_.valid` true?
- are template slots actually recorded?

Likely files:

- `trainer_app.cpp`
- `protractor_math.cpp`

### Symptom: button taps leave marks on the canvas

Check:

- gesture start routing in `HandleTouch()`
- whether `BeginStroke()` is only called when touch begins inside `canvas_`

Likely file:

- `trainer_app.cpp:323-412`

### Symptom: screen updates are slow or dirty

Check:

- whether live updates are still using `epd_fast`
- whether full-screen UI redraws are bounded to moments when the app state changes
- whether clip rects are being cleared correctly

Likely files:

- `trainer_app.cpp`
- donor `LGFXBase.hpp`
- donor `Panel_IT8951.cpp`

## Design Decisions

Key decisions and rationale:

- Use fixed slots rather than free-form names.
  Reason: fits the device and avoids unrelated text-input work.

- Use a separate math module.
  Reason: preserves algorithm clarity and reuse.

- Expose preprocessing visually.
  Reason: the app doubles as a teaching/demo tool.

- Prefer partial fast updates only for the canvas.
  Reason: better e-paper behavior and more readable UI.

## Alternatives Considered

The important rejected alternatives are:

- exact HTML UI clone
  - rejected as device-inappropriate
- persistence first
  - rejected as scope creep
- single-file code
  - rejected as poor teaching structure

## Implementation Plan

See the dedicated detailed plan in:

- `design-doc/02-papers3-protractor-gesture-trainer-detailed-implementation-plan.md`

## Open Questions

Questions for later iterations:

- Should slots display a tiny sketch preview of the saved template?
- Should templates persist across reboots?
- Should the trainer expose orientation-sensitive mode as a toggle?
- Should there be a confidence threshold below which “no match” is shown?

## References

- `sources/local/protractor_gesture_recognizer_demo.html`
- `reference/01-investigation-diary.md`
- `0076-papers3-protractor-trainer/README.md`
