---
Title: PaperS3 alphabet graffiti analysis design and implementation guide
Ticket: ESP-33-PAPERS3-ALPHABET-GRAFFITI
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
    - storage
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/main/app_main.cpp
      Note: Application entrypoint
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.h
      Note: Main application state, UI layout, and mode definitions
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.cpp
      Note: Touch routing, UI drawing, and runtime behavior
    - Path: 0077-papers3-alphabet-graffiti/main/glyph_store.h
      Note: Persistent glyph template storage interface
    - Path: 0077-papers3-alphabet-graffiti/main/glyph_store.cpp
      Note: SPIFFS-backed template serialization and loading
    - Path: 0077-papers3-alphabet-graffiti/main/protractor_math.h
      Note: Public gesture math API
    - Path: 0077-papers3-alphabet-graffiti/main/protractor_math.cpp
      Note: Protractor-style preprocessing and distance calculation
    - Path: 0077-papers3-alphabet-graffiti/partitions.csv
      Note: Partition table including the SPIFFS storage partition
    - Path: 0077-papers3-alphabet-graffiti/sdkconfig.defaults
      Note: ESP-IDF 5.3.4 target and USB Serial/JTAG defaults
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5Unified/src/utility/Touch_Class.hpp
      Note: Touch abstraction used by the app
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/touch/Touch_GT911.cpp
      Note: GT911 driver path beneath M5Unified
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp
      Note: PaperS3 e-paper update mode behavior
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T21:28:34.615631557-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 alphabet graffiti analysis design and implementation guide

## Executive Summary

This document explains the full system behind `0077-papers3-alphabet-graffiti` for a new intern. The app has two user-facing modes:

- `TRAIN`: the user selects a glyph from `A-Z` or `0-9`, draws a stroke, and saves that stroke as the template for that glyph
- `WRITE`: the user draws graffiti-style single-stroke input and the app recognizes the stroke against the saved templates, automatically appending strong matches to a text buffer

The app is intentionally built as a thin device UI around one shared recognition pipeline. There is not a separate recognizer for training and writing. Both modes use the same raw touch points, resampling, vectorization, and matching. The only difference is the post-recognition action:

- in `TRAIN`, recognition is informational and the user can save the current vector into the selected glyph slot
- in `WRITE`, recognition drives live text entry through automatic append plus correction controls

That design is the main architectural idea to preserve.

## Problem Statement

The earlier PaperS3 demos proved three separate things:

- the donor `M5PaperS3-UserDemo` stack already gives us the correct display and GT911 touch foundation
- Protractor-style gesture matching can be ported cleanly from a browser demo into ESP-IDF C++
- the PaperS3 screen has enough space for a useful handwriting interface if the layout is carefully partitioned

What the earlier demos did not provide was an alphabet-scale workflow. A realistic graffiti-like recognizer needs all of these properties:

- support for more than a handful of templates
- persistence across reboot, otherwise training is not useful
- a mode where partially trained alphabets are still usable
- a UI that lets the user switch between template authoring and live writing without reflashing or changing apps

The `0077` app solves that by combining:

- a 36-glyph template table
- SPIFFS-based persistent storage
- a `TRAIN` mode with paged glyph selection
- a `WRITE` mode with a live text buffer and editing controls

This is not a general-purpose handwriting recognizer. It is a user-trained single-stroke recognizer. That distinction matters when you evaluate design choices and test results.

## Proposed Solution

The solution is a single firmware image with one runtime class, `alphabet_graffiti::AlphabetApp`, that owns:

- board initialization
- layout geometry
- touch routing
- temporary stroke data
- the in-memory template table
- the persistent storage bridge
- the mode-specific UI
- the text buffer for writing mode

The top-level architecture looks like this:

```text
Finger / stylus
  -> GT911 touch controller
  -> M5GFX GT911 driver
  -> M5Unified touch facade
  -> AlphabetApp::HandleTouch()
  -> raw stroke points
  -> Protractor preprocessing
     - Resample()
     - Vectorize()
     - OptimalCosineDistance()
  -> ranked matches
  -> mode-specific action
     - TRAIN: save/delete/reload template
     - WRITE: append glyph / edit text buffer
  -> PaperS3 e-paper UI redraw
```

The file/module split is:

```text
app_main.cpp
  minimal entrypoint, instantiates AlphabetApp

alphabet_app.h / alphabet_app.cpp
  application state, touch routing, layout, mode branching, rendering

glyph_store.h / glyph_store.cpp
  SPIFFS mount + load/save of the glyph table

protractor_math.h / protractor_math.cpp
  resampling, centroid normalization, vectorization, similarity

partitions.csv
  adds the "storage" SPIFFS partition

sdkconfig.defaults
  ESP32-S3 target settings and USB Serial/JTAG console defaults
```

### Why one runtime class is acceptable here

For a large product, one large runtime class might be too much. For this app it is a reasonable tradeoff because:

- the UI is still single-screen
- the state machine is simple enough to inspect in one place
- all event routing is touch-first and board-local
- the algorithm has already been factored into its own module

The main risk is eventual growth. If punctuation, multiple templates per glyph, or settings screens are added, `AlphabetApp` should probably be split into:

- `InputController`
- `LayoutModel`
- `TrainingModeView`
- `WritingModeView`
- `RecognitionEngineFacade`

That refactor is not required yet.

## System Inventory

### 1. Board and donor stack

The project starts from the PaperS3 donor environment instead of building board support from scratch. That means:

- `M5Unified` handles board bring-up and touch access
- `M5GFX` handles PaperS3 display and e-paper mode integration
- the donor component tree is reused through `EXTRA_COMPONENT_DIRS`

Why this matters:

- the app code does not need to know low-level GT911 register behavior
- display refresh modes are already exposed through the donor stack
- the application can stay focused on UI and recognition

Relevant files:

- `0077-papers3-alphabet-graffiti/CMakeLists.txt`
- `M5PaperS3-UserDemo/components/M5Unified/src/utility/Touch_Class.hpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/touch/Touch_GT911.cpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp`

### 2. Entry point

`app_main.cpp` is deliberately minimal:

```cpp
extern "C" void app_main(void)
{
    alphabet_graffiti::AlphabetApp app;
    app.Run();
}
```

This is good for debugging because all app behavior starts in one obvious place. If the app fails after boot, the next file to inspect is always `alphabet_app.cpp`.

### 3. Main runtime state

The most important state in `AlphabetApp` falls into four groups.

UI/layout state:

- screen rectangles for the header, canvas, cards, buttons, and glyph slots
- mode tabs for `TRAIN` and `WRITE`

Recognition state:

- `raw_points_`
- `resampled_points_`
- `current_gesture_`
- `recognition_scores_`
- `matched_index_`

Template state:

- `templates_`
- `selected_index_`
- `current_page_`
- `storage_ready_`
- `storage_status_`

Writing state:

- `write_buffer_`
- `write_status_`
- `kWriteAcceptanceThreshold`

This grouping is important when debugging. A bug is usually one of:

- the touch path did not collect the right raw points
- the recognition path produced bad matches
- the persistence path loaded or saved the wrong template
- the UI state is drawing the wrong branch for the current mode

### 4. Glyph template table

The app supports exactly 36 glyphs:

- `A` through `Z`
- `0` through `9`

This mapping lives in `GlyphStore`:

- `GlyphStore::kGlyphCount`
- `GlyphStore::GlyphForIndex()`
- `GlyphStore::IndexForGlyph()`

The app does not dynamically allocate labels or ask the user to name templates. That is a deliberate simplification for a touch-only device UI.

### 5. Persistent storage

Templates are stored on SPIFFS in:

```text
/spiffs/glyph_templates.txt
```

The partition is declared in `partitions.csv`:

```text
storage,  data, spiffs,  ,        512K,
```

That partition is mounted using `esp_vfs_spiffs_register()` in `glyph_store.cpp`.

The file format is plain text:

```text
glyph-store-v1
A 32 0.123456 -0.054321 ...
B 32 ...
7 32 ...
```

Each non-header line is:

- glyph label
- vector length
- vector floats

This format is intentionally simple.

Advantages:

- easy to inspect by hand during debugging
- easy to extend with a new version header later
- deterministic and easy to regenerate

Limitations:

- no compression
- no metadata beyond the vector itself
- no multiple templates per glyph

### 6. Recognition engine

Recognition lives in `protractor_math.cpp`. The pipeline is:

1. collect raw points in screen coordinates
2. resample to a fixed number of points
3. compute the normalized vector representation
4. compare the current vector to every recorded template using optimal cosine distance
5. sort matches by descending cosine score

The app currently resamples to 16 points, which yields a 32-float vector.

Key exported API:

- `Resample(const std::vector<PointF>&, std::size_t)`
- `Vectorize(const std::vector<PointF>&, bool)`
- `OptimalCosineDistance(const std::vector<float>&, const std::vector<float>&)`

## End-to-End Runtime Flow

### Boot flow

```text
app_main()
  -> AlphabetApp::Run()
     -> InitBoard()
     -> BuildLayout()
     -> LoadTemplatesFromDisk()
     -> RenderFullUi()
     -> loop:
          M5.update()
          HandleTouch()
          delay
```

### Touch flow

The app uses polling through `M5.update()` and `M5.Touch`.

The routing rule is simple:

- if touch begins inside the canvas, start a stroke
- if touch begins on a button, remember the pending action
- if touch begins on a glyph slot in `TRAIN`, remember the pending slot
- on release, commit the action only if the touch is still inside the same interactive region

This is a good pattern on embedded touch UIs because it avoids accidental triggers during movement.

### Stroke lifecycle

Pseudocode:

```text
BeginStroke(point):
  clear previous stroke analysis
  clear canvas interior
  clamp point to canvas
  push point into raw_points_
  draw initial dot

ExtendStroke(point):
  clamp point
  append to raw_points_
  draw segment from last point

FinishStroke():
  AnalyzeStroke()
  if mode == WRITE:
    TryAppendRecognizedGlyph()
  RenderFullUi()
```

### Recognition lifecycle

Pseudocode:

```text
AnalyzeStroke():
  resampled_points = Resample(raw_points, 16)
  current_gesture = Vectorize(resampled_points)
  recognition_scores = []
  for each template in templates:
    if template.recorded:
      distance = OptimalCosineDistance(template.vector, current_gesture.vector)
      score = cos(distance)
      recognition_scores.push(template, score)
  sort descending by score
  matched_index = top match or -1
```

Important detail:

- unrecorded glyphs are skipped
- that is what allows `WRITE` mode to work even with only a partial alphabet trained

## Mode-Specific Behavior

### TRAIN mode

`TRAIN` mode answers the question: "Which glyph am I currently defining?"

The right-side column shows:

- a paged glyph picker
- training actions
- status metrics
- storage and recognition feedback

Actions:

- `SAVE GLYPH`
- `CLEAR STROKE`
- `DELETE GLYPH`
- `RELOAD DISK`

The glyph picker is paged `4 x 3`, so 12 glyphs are visible at a time. This is the compromise between:

- target size large enough for touch
- showing a meaningful chunk of the alphabet
- leaving a large canvas for stroke capture

### WRITE mode

`WRITE` mode answers the question: "What text has the user produced so far?"

The right-side column changes to:

- a writing buffer card
- writing actions
- writing metrics
- recognition feedback

Actions:

- `SPACE`
- `BACKSPACE`
- `CLEAR TEXT`
- `CLEAR STROKE`

When a stroke ends:

- the app computes recognition normally
- if the top score is greater than or equal to `0.82`, the glyph is appended to `write_buffer_`
- otherwise, the buffer is unchanged and the user gets a status message

This auto-append threshold is an important product choice. It keeps write mode fast, but it may need tuning on real hardware.

## UI Layout

The screen is intentionally asymmetric:

```text
+---------------------------------------------------------------+
| Header: title + mode tabs                                     |
+-------------------------------+-------------------------------+
| Canvas card                   | TRAIN: glyph picker           |
|                               | WRITE: writing buffer         |
|   large drawing area          +-------------------------------+
|                               | actions                       |
|                               +-------------------------------+
|                               | metrics                       |
|                               +-------------------------------+
|                               | results / status              |
+-------------------------------+-------------------------------+
```

Why this layout works:

- the canvas stays stable across both modes
- the right column is allowed to branch by mode
- the user never loses the mental model of "draw on the left, inspect/control on the right"

## Rendering Model

The display is e-paper, so rendering strategy matters.

The app uses two update styles:

- fast partial drawing for live stroke ink
- slower full redraws for stable text and panels

Implementation pattern:

```text
During live drawing:
  setEpdMode(epd_fast)
  draw only inside the canvas clip rect

During full UI refresh:
  setEpdMode(epd_text)
  fill screen and redraw all cards
```

This avoids unnecessary ghosting and keeps the interactive feeling acceptable on PaperS3.

API references worth reading:

- `M5.Display.setEpdMode(...)`
- `M5.Display.startWrite()`
- `M5.Display.endWrite()`
- `M5.Display.setClipRect(...)`
- `M5.Display.clearClipRect()`

## Detailed Storage Design

`GlyphStore` has three responsibilities:

1. initialize the in-memory glyph table with labels and blank vectors
2. mount SPIFFS
3. load/save the table from/to a text file

### Load path

Pseudocode:

```text
Load(templates):
  if not mounted:
    Mount()
  open /spiffs/glyph_templates.txt
  if file missing:
    return success with "no file yet" status
  validate header == glyph-store-v1
  reinitialize templates to blank
  for each line:
    parse glyph label
    parse vector length
    parse N floats
    if parse succeeds:
      mark template as recorded
      copy vector into template
```

### Save path

Pseudocode:

```text
Save(templates):
  if not mounted:
    Mount()
  open /spiffs/glyph_templates.txt for write
  write glyph-store-v1 header
  for each recorded template:
    write label, vector size, and vector floats
```

### Why the load path starts from a blank table

This is subtle but important. `Load()` first resets the table to the canonical 36 labels before applying file contents. That means:

- deleted templates do not accidentally persist in RAM after reload
- corrupted or partial files only affect the lines they actually load
- the app always preserves the fixed label ordering

## Key Design Decisions

### Decision 1: one template per glyph

Chosen because:

- simplest UI
- simplest storage format
- easiest explanation for a new user and a new developer

Rejected alternative:

- multiple templates per glyph

Why rejected for now:

- complicates storage schema
- complicates training UI
- requires a policy for template averaging or best-match aggregation

### Decision 2: fixed alphabet + digits only

Chosen because:

- avoids on-device naming
- matches the user request
- keeps the glyph table predictable

Rejected alternative:

- arbitrary label strings or punctuation now

Why rejected for now:

- needs keyboard or a much more complex template-management UI

### Decision 3: partial alphabet support in write mode

Chosen because:

- matches the user requirement
- makes the app useful earlier
- recognition naturally supports it because only recorded templates are matched

### Decision 4: automatic append with threshold

Chosen because:

- creates a real writing workflow instead of "recognition preview only"
- avoids needing an extra confirm button for every stroke

Tradeoff:

- false positives are possible if the threshold is too low
- false negatives are possible if the threshold is too high

## Alternatives Considered

### Alternative A: separate training and writing apps

Pros:

- cleaner per-app code
- smaller runtime state in each binary

Cons:

- awkward user workflow
- harder to switch immediately from training to writing
- duplicated board/UI code

Rejected because the user explicitly wanted quick mode switching.

### Alternative B: dynamic filesystem objects per glyph

Pros:

- one file per template
- easier manual replacement of a single glyph

Cons:

- more filesystem operations
- more error cases
- more complex reload logic

Rejected because one compact file is easier to reason about initially.

### Alternative C: browser-style canvas layering on device

Pros:

- potentially more flexible overlays

Cons:

- much more complexity on e-paper
- unnecessary for the current app size

Rejected because the current card-based layout is sufficient.

## Implementation Plan

If a new intern had to rebuild this app from scratch, the recommended sequence is:

1. Build the skeleton app and confirm donor PaperS3 display/touch wiring.
2. Port the Protractor math into a reusable `protractor_math` module.
3. Add a fixed glyph table for `A-Z` and `0-9`.
4. Add `TRAIN` mode UI with save/delete/clear behavior.
5. Add SPIFFS persistence and verify load/save.
6. Add `WRITE` mode using the exact same recognition pipeline.
7. Add text editing controls and confidence-threshold behavior.
8. Tune layout and verify on physical hardware.

## Extension Guide For Interns

### Safe changes

These are low-risk modifications:

- adjust the write threshold
- adjust card titles or button labels
- change page size from 12 to 9 or 8
- improve the text preview formatting

### Medium-risk changes

- add punctuation glyphs
- store multiple templates per glyph
- add a settings panel
- change recognition scoring heuristics

### High-risk changes

- rewrite touch routing
- change e-paper update mode usage
- change the storage format without versioning
- change the normalization math without retesting trained templates

## Suggested Debugging Checklist

If something is broken, debug in this order:

1. Does `M5.update()` run and do touches appear?
2. Does `raw_points_` grow while drawing?
3. Does `Resample()` produce the expected number of points?
4. Are any templates recorded in memory?
5. Did SPIFFS mount successfully?
6. Does `/spiffs/glyph_templates.txt` contain the expected lines?
7. Is `recognition_scores_` sorted and non-empty?
8. Is the mode-specific branch drawing the UI you think it is?

## Open Questions

- Should the write threshold become user-configurable?
- Should space eventually be inferred from timing or gesture gaps?
- Should the app support more than one template per glyph for improved robustness?
- Should long write buffers scroll instead of only showing the trailing preview window?

## References

Primary project files:

- `0077-papers3-alphabet-graffiti/main/app_main.cpp`
- `0077-papers3-alphabet-graffiti/main/alphabet_app.h`
- `0077-papers3-alphabet-graffiti/main/alphabet_app.cpp`
- `0077-papers3-alphabet-graffiti/main/glyph_store.h`
- `0077-papers3-alphabet-graffiti/main/glyph_store.cpp`
- `0077-papers3-alphabet-graffiti/main/protractor_math.h`
- `0077-papers3-alphabet-graffiti/main/protractor_math.cpp`
- `0077-papers3-alphabet-graffiti/partitions.csv`
- `0077-papers3-alphabet-graffiti/sdkconfig.defaults`

Donor and framework references:

- `M5PaperS3-UserDemo/components/M5Unified/src/utility/Touch_Class.hpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/touch/Touch_GT911.cpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp`

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
