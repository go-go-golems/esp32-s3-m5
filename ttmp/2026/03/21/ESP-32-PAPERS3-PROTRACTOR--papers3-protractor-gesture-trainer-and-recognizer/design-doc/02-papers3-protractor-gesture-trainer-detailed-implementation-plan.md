---
Title: PaperS3 protractor gesture trainer detailed implementation plan
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
    - Path: 0076-papers3-protractor-trainer/CMakeLists.txt
      Note: Donor component reuse
    - Path: 0076-papers3-protractor-trainer/sdkconfig.defaults
      Note: IDF target and USB Serial/JTAG console policy
    - Path: 0076-papers3-protractor-trainer/main/protractor_math.cpp
      Note: Protractor resample, vectorize, and scoring implementation
    - Path: 0076-papers3-protractor-trainer/main/trainer_app.cpp
      Note: UI layout, touch routing, and rendering plan
ExternalSources:
    - local:protractor_gesture_recognizer_demo.html
Summary: ""
LastUpdated: 2026-03-21T20:26:40.648766125-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 protractor gesture trainer detailed implementation plan

## Executive Summary

This plan describes how to deliver a self-contained PaperS3 gesture trainer that uses the Protractor recognizer, stores templates locally in memory, displays recognition scores on-device, and remains understandable to a new engineer. The key architectural decision is to separate the algorithm from the product shell:

- `protractor_math.*` owns gesture preprocessing and similarity math
- `trainer_app.*` owns layout, touch routing, state, and e-paper rendering
- `app_main.cpp` only boots the app

This split makes the code easier to review and makes the intern guide much cleaner.

## Problem Statement

The imported HTML demo proves that Protractor can:

- resample a stroke to fixed point count
- rotate and normalize it into a vector
- compare it against saved templates with optimal cosine distance

That demo is not directly usable on PaperS3 because it depends on:

- HTML and CSS layout
- a browser canvas
- pointer events
- a keyboard-backed text input for template names

The PaperS3 version must solve the same recognition problem inside a very different system:

- touch arrives through GT911 and `M5Unified`
- rendering goes through `M5GFX` and the IT8951 e-paper controller
- live drawing must minimize full-screen refreshes
- the UI must remain usable without a text keyboard

## Proposed Solution

Build a new tutorial app, `0076-papers3-protractor-trainer`, with the following behavior:

1. The board boots using the donor `M5Unified` and `M5GFX` component stack.
2. A single-screen UI shows:
   - a large gesture canvas
   - eight template slots labeled `A` through `H`
   - actions to save, delete, clear, and reset
   - preprocessing metrics
   - ranked recognition results
3. A finger stroke on the canvas is collected as raw points.
4. On release, the app:
   - resamples the stroke to 16 points
   - computes centroid, indicative angle, rotation delta, and normalized vector
   - compares the vector against recorded templates
   - displays top results as bars
5. Saving stores the selected slot’s vector in RAM.
6. Clearing removes only the current stroke.
7. Reset clears both the stroke and all templates.

## Scope

In scope:

- one-screen PaperS3 trainer
- fixed in-memory templates
- top-match highlighting
- visually explainable preprocessing overlays
- build verification with `ESP-IDF 5.3.4`
- documentation sufficient for intern onboarding

Out of scope:

- persistent storage
- user-entered template names
- multi-stroke gestures
- pressure sensitivity
- undo/redo
- offline export/import of template libraries
- hardware validation beyond successful build

## Architecture Plan

### Phase 1: Establish the project shell

Files:

- `0076-papers3-protractor-trainer/CMakeLists.txt`
- `0076-papers3-protractor-trainer/main/CMakeLists.txt`
- `0076-papers3-protractor-trainer/sdkconfig.defaults`
- `0076-papers3-protractor-trainer/README.md`
- `0076-papers3-protractor-trainer/partitions.csv`

Tasks:

- create the new numbered tutorial project
- point `EXTRA_COMPONENT_DIRS` at `../../M5PaperS3-UserDemo/components`
- lock the project to `esp32s3`
- preserve the USB Serial/JTAG console default
- document the exact 5.3.4 build command

Acceptance criteria:

- CMake config resolves donor components
- `idf.py set-target esp32s3` works
- the project can enter the compile phase without missing component errors

### Phase 2: Port the algorithm into a standalone math module

Files:

- `0076-papers3-protractor-trainer/main/protractor_math.h`
- `0076-papers3-protractor-trainer/main/protractor_math.cpp`

Tasks:

- define `PointF` and `VectorizedGesture`
- port `pathLength`, `centroid`, `resample`, `vectorize`, and `optCosDistance`
- return a structured result from vectorization so the UI can display centroid and angles
- preserve the browser demo’s 16-point resampling strategy

Acceptance criteria:

- the algorithm compiles independently of UI code
- math code has no display dependencies
- vector output and scoring logic mirror the imported HTML behavior

### Phase 3: Design a PaperS3-native training UI

Files:

- `0076-papers3-protractor-trainer/main/trainer_app.h`
- `0076-papers3-protractor-trainer/main/trainer_app.cpp`

Tasks:

- define screen regions for header, canvas, slots, controls, metrics, and results
- replace HTML “named templates” with touch-selectable slots
- add rendering helpers for cards and buttons
- keep the interface grayscale and e-paper friendly

Design decisions:

- use fixed slots instead of free-form names because the device has no text-entry scope
- use one selected slot at a time to keep the interaction deterministic
- keep metrics visible so the app doubles as a teaching aid

Acceptance criteria:

- all major states are visible without opening menus
- a new user can understand the training loop from the screen alone
- the layout fits in landscape mode without overlap

### Phase 4: Implement touch routing and live drawing

Tasks:

- poll touch inside the main loop using `M5.update()`
- start a stroke only when touch begins inside the canvas
- treat slots and action buttons as tap targets
- use `epd_fast` for live stroke stamping
- switch to `epd_text` for full UI redraws

Pseudo-code:

```text
loop:
  M5.update()
  if touch exists:
    if touch just began:
      if point in canvas: begin stroke
      else if point in slot: arm slot selection
      else if point in button: arm button action
    else if drawing: extend stroke
  else if touch just ended:
    if drawing: finish stroke and classify
    else if slot tap confirmed: select slot
    else if button tap confirmed: run action
```

Acceptance criteria:

- drawing does not accidentally trigger buttons
- buttons do not leave marks in the canvas
- stroke feedback appears during movement, not only on release

### Phase 5: Implement training and recognition state transitions

Tasks:

- analyze the current stroke on release
- store vectors in the selected slot
- delete a slot without affecting others
- recompute recognition when templates change
- highlight the best-matching slot in the UI

Data structures:

- `slots_`: array of eight template slots
- `selected_slot_`: where the next save will go
- `raw_points_`: current finger path
- `resampled_points_`: normalized point sample
- `current_gesture_`: centroid, angles, normalized vector
- `recognition_scores_`: ranked results for display

Acceptance criteria:

- save is enabled only when a valid stroke exists
- delete is enabled only when the selected slot is populated
- recognition results update after stroke release and after template edits

### Phase 6: Validate and document

Tasks:

- build with `/home/manuel/esp/esp-idf-5.3.4/export.sh`
- record real commands and outcomes in the diary
- write the guide for a new intern
- relate the important source files to the ticket
- validate the ticket with `docmgr doctor`
- upload the bundle to reMarkable

Acceptance criteria:

- build passes
- diary reflects actual implementation sequence
- docs explain both the product and the underlying system
- ticket bundle can be reviewed without opening the chat transcript

## Design Decisions

### Decision: split algorithm and UI into separate modules

Why:

- math is easier to validate in isolation
- UI code stays focused on device behavior
- documentation can explain the algorithm once and then show where it is called

### Decision: fixed template slots instead of template names

Why:

- the browser demo relied on a text field and button controls
- PaperS3 has touch but no text-entry workflow in scope
- slot labels `A` through `H` are enough for demo/training purposes

### Decision: use cosine similarity display instead of raw inverse-distance score

Why:

- the browser demo internally sorted by `1 / distance`
- for on-device display, `cos(distance)` is more interpretable and naturally bounded
- the best match still sorts to the top

### Decision: use `epd_fast` for live strokes and `epd_text` for full redraws

Why:

- live pen feel is better with a faster update mode
- chrome and text remain clearer with a text-optimized full redraw
- this aligns with the IT8951 update mapping in the donor display stack

## Alternatives Considered

### Alternative: keep everything in one source file

Rejected because:

- the algorithm would be buried inside UI code
- the intern guide would have to jump around too much
- future testing or reuse would be harder

### Alternative: store templates in NVS immediately

Rejected because:

- not required for the demo
- persistence adds lifecycle and serialization complexity
- the first deliverable benefits more from clarity than from durability

### Alternative: replicate the HTML layout exactly

Rejected because:

- the browser layout assumes color, richer typography, and abundant width
- the PaperS3 interface needs fewer simultaneous controls
- exact fidelity would create a weaker device experience

### Alternative: use dynamic handwritten template names

Rejected because:

- adds input UX complexity unrelated to Protractor itself
- fixed slots communicate state more clearly on a touch-only device

## Implementation Plan

1. Create the ticket and import the local HTML reference.
2. Inspect the browser implementation for algorithm and layout primitives.
3. Create a new numbered PaperS3 project that reuses donor components.
4. Port Protractor math into a separate module.
5. Build a `TrainerApp` shell for layout, rendering, and input state.
6. Implement live stroke drawing and release-time recognition.
7. Implement template save/delete/reset behavior.
8. Compile with `ESP-IDF 5.3.4` and fix any integration issues.
9. Write the plan, guide, and diary with concrete file and API references.
10. Validate the ticket and upload the bundle.

## Risks And Mitigations

- Risk: live drawing ghosts badly on e-paper.
  Mitigation: use fast partial updates only for the canvas area and reserve full redraws for stable UI.

- Risk: touch taps and drawing gestures interfere with each other.
  Mitigation: route by gesture start region and only confirm taps on release.

- Risk: math and UI drift from the imported reference.
  Mitigation: keep the resample/vectorize/distance formulas structurally aligned with the HTML source.

- Risk: intern confusion about where GT911 logic lives.
  Mitigation: explicitly document the chain from `Touch_GT911.cpp` to `Touch_Class.hpp` to `TrainerApp::HandleTouch()`.

## Acceptance Checklist

- [x] New app project exists
- [x] Imports donor display/touch stack
- [x] Protractor math is implemented
- [x] Templates can be trained on-device
- [x] Results can be viewed on-device
- [x] Stroke can be cleared independently of templates
- [x] All templates can be reset
- [x] Build succeeds with `ESP-IDF 5.3.4`
- [x] Ticket includes detailed handoff documentation
- [ ] Physical hardware run completed
- [ ] reMarkable upload verified

## Open Questions

Open items outside this implementation pass:

- Should templates persist across reboot using NVS or a file?
- Should the next version allow more than eight templates?
- Is orientation-sensitive Protractor mode useful for PaperS3-specific gesture sets?
- Should a future revision add miniature slot previews instead of only labels and match highlighting?

## References

- `sources/local/protractor_gesture_recognizer_demo.html`
- `design-doc/01-papers3-protractor-gesture-trainer-analysis-design-and-implementation-guide.md`
- `reference/01-investigation-diary.md`
