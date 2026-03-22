---
Title: PaperS3 touch draw demo detailed implementation plan
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
    - Path: ../../../../../../../M5PaperS3-UserDemo/main/hal/hal.cpp
      Note: Donor board initialization evidence
    - Path: 0075-papers3-touch-draw-demo/CMakeLists.txt
      Note: Donor component reuse strategy
    - Path: 0075-papers3-touch-draw-demo/main/app_main.cpp
      Note: Primary implementation target and runtime plan
    - Path: 0075-papers3-touch-draw-demo/sdkconfig.defaults
      Note: ESP-IDF 5.3.4 and USB Serial JTAG defaults
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T20:02:37.346693982-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 touch draw demo detailed implementation plan

## Executive Summary

Build a new small tutorial project at `0075-papers3-touch-draw-demo/` rather than editing the donor app in place. Reuse the donor’s already-vendored `M5Unified` and `M5GFX` components, keep the runtime architecture to one class in one file, and focus the demo on three user-visible behaviors: boot into a clean canvas, draw black strokes from finger motion, and clear the screen when the user taps `CLEAR`.

This plan optimizes for:

- low implementation risk
- no network dependency during build
- intern readability
- direct reuse of proven PaperS3 and GT911 plumbing

## Problem Statement

The user requested a demo program for the PaperS3 that must:

1. use ESP-IDF 5.3.4,
2. use `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo` as the hardware/display/touch starting point,
3. draw user touch input on the e-paper screen, and
4. provide a button that clears the drawing.

The donor project already proves board bring-up, touch, and e-paper operation, but it is much larger than needed. The implementation task is therefore not “make PaperS3 hardware work”; it is “extract the minimum reliable subset into a simpler tutorial-grade example.”

## Proposed Solution

Create a new project with this shape:

```text
0075-papers3-touch-draw-demo/
  CMakeLists.txt
  README.md
  dependencies.lock
  partitions.csv
  sdkconfig.defaults
  main/
    CMakeLists.txt
    app_main.cpp
```

Key design points:

- `CMakeLists.txt` points `EXTRA_COMPONENT_DIRS` at `../../M5PaperS3-UserDemo/components` so the new project can reuse the donor’s vendored libraries without copying them.
- `sdkconfig.defaults` pins `esp32s3`, 16 MB flash, PSRAM, and USB Serial/JTAG console.
- `app_main.cpp` contains one small application class, `PaperS3DrawDemo`, which owns layout, rendering, and touch gesture handling.
- Full-screen UI redraws use `epd_text`.
- incremental stroke updates use `epd_fast`.
- touch behavior is based on `M5.Touch.getCount()` and `M5.Touch.getDetail()`.

## Design Decisions

### Decision 1: New project instead of donor-app surgery

Reason:

- the donor app is a multi-app demo shell, not a tutorial project
- a small example is easier to document and easier for an intern to extend
- this avoids accidentally breaking unrelated donor functionality

### Decision 2: Reuse donor components through `EXTRA_COMPONENT_DIRS`

Reason:

- the components already exist locally
- the build succeeds offline
- there is no need to create duplicate copies of `M5GFX` or `M5Unified`

### Decision 3: One-file app architecture

Reason:

- the behavior is simple enough that additional abstraction would mostly be ceremony
- a single file is easier for a new intern to scan end-to-end
- the code still separates logic by methods: board init, layout, redraw, stroke rendering, touch handling

### Decision 4: Use `epd_text` for chrome and `epd_fast` for pen strokes

Reason:

- the button, frame, and header text benefit from cleaner rendering
- live pen drawing benefits from lower-latency partial refresh
- this matches the IT8951 update-mode split exposed by `Panel_IT8951::display(...)`

### Decision 5: Clear by redrawing the entire UI, not by erasing stored strokes

Reason:

- it keeps state minimal
- e-paper demos benefit from occasional full-screen refresh
- the requirement does not need undo, stroke replay, or persistence

## Alternatives Considered

### Alternative A: Modify `M5PaperS3-UserDemo` directly

Rejected because:

- the donor project contains unrelated demo apps
- it is harder to explain which parts are still essential
- the resulting tutorial would still inherit a larger framework than needed

### Alternative B: Copy donor components into `0075`

Rejected because:

- it duplicates a large amount of third-party code
- it increases maintenance cost
- it was unnecessary once `EXTRA_COMPONENT_DIRS` worked

### Alternative C: Store every stroke and repaint from memory on every update

Rejected because:

- it adds complexity not required for the requested behavior
- it would require a stroke model, replay path, and redraw invalidation logic
- the e-paper partial-update path already supports direct incremental writes

### Alternative D: Use a sprite/framebuffer abstraction on top of the panel

Rejected because:

- it obscures the direct panel behavior this tutorial is supposed to teach
- the feature set does not justify the extra layer

## Implementation Plan

### Phase 1: Capture donor constraints

Steps:

1. Read donor bring-up code and confirm the minimum board initialization sequence.
2. Confirm touch access pattern:
   - `M5.Touch.getCount()`
   - `M5.Touch.getDetail()`
3. Confirm EPD write behavior:
   - `startWrite()` and `endWrite()` commit drawing on EPD targets.

Deliverable:

- line-backed donor references for initialization, touch, and display modes

### Phase 2: Create the standalone project skeleton

Steps:

1. Add a new numbered directory `0075-papers3-touch-draw-demo/`.
2. Add root `CMakeLists.txt`.
3. Add `main/CMakeLists.txt`.
4. Add `sdkconfig.defaults`, `partitions.csv`, `dependencies.lock`, and `README.md`.

Deliverable:

- an IDF project that configures cleanly under 5.3.4

### Phase 3: Wire donor components

Steps:

1. Set `EXTRA_COMPONENT_DIRS` to `../../M5PaperS3-UserDemo/components`.
2. Run `idf.py build`.
3. If CMake reports the component directory cannot be found, correct the relative path and rebuild.

Deliverable:

- working component discovery without fetching dependencies

### Phase 4: Implement the rendering model

Steps:

1. Define layout constants for header, button, and canvas.
2. Draw the static UI:
   - background
   - title
   - subtitle
   - clear button
   - canvas border
3. Use `epd_text` for the full redraw path.

Deliverable:

- a boot screen that clearly communicates the interaction model

### Phase 5: Implement touch gesture routing

Steps:

1. Track whether a finger is down.
2. On first contact:
   - if inside clear button: arm a clear gesture
   - else if inside canvas: start a stroke
3. While finger remains down:
   - update `last_touch_`
   - if stroke active: draw from previous point to current point
4. On release:
   - if clear gesture is still inside the button, redraw the UI

Deliverable:

- deterministic routing between draw and clear gestures

### Phase 6: Implement stroke rendering

Steps:

1. Clamp points to the canvas bounds.
2. Switch to `epd_fast`.
3. Wrap the stroke write inside `startWrite()` / `endWrite()`.
4. Apply a clip rect so the stroke cannot bleed into the header.
5. Stamp filled circles along the segment to create a thick, continuous line.

Deliverable:

- visible, smooth-enough finger strokes on the e-paper surface

### Phase 7: Validate and document

Steps:

1. Build with:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py build
```

2. Record the exact successful build result.
3. Write ticket docs.
4. Run `docmgr doctor`.
5. Upload the bundle to reMarkable.

Deliverable:

- shippable firmware plus ticket documentation

## Open Questions

1. Real-device touch feel has not been checked in this session, so stroke latency and palm-rejection behavior remain unverified.
2. Repeated `epd_fast` drawing may accumulate ghosting over longer sessions; if that becomes visible, the next revision should store stroke data and periodically force a cleaner full-screen replay.
3. The current gesture model is single-finger only. Multi-touch is available in the API but intentionally ignored.

## References

- New firmware: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo/main/app_main.cpp`
- New project config: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo/CMakeLists.txt`
- New SDK defaults: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo/sdkconfig.defaults`
- Donor init: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/main/hal/hal.cpp`
- Donor touch wrapper: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/main/hal/hal.h`
- M5Unified touch API: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5Unified/src/utility/Touch_Class.hpp`
- M5GFX EPD write semantics: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
- IT8951 update-mode mapping: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp`
