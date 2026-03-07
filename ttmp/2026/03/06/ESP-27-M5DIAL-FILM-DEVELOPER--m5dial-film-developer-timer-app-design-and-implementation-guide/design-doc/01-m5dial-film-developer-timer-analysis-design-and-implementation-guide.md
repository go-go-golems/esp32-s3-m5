---
Title: M5Dial film developer timer analysis, design, and implementation guide
Ticket: ESP-27-M5DIAL-FILM-DEVELOPER
Status: active
Topics:
    - esp32s3
    - m5stack
    - firmware
    - photo-development
    - ui
    - timer
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0072-m5dial-timer-demo/main/app_main.cpp
      Note: Working M5Dial task and event-bus architecture to clone for the new app
    - Path: 0072-m5dial-timer-demo/main/input_events.h
      Note: Normalized input event contract to preserve in the new app
    - Path: 0072-m5dial-timer-demo/main/m5dial_board.cpp
      Note: Working board wrapper and touch/encoder/button implementation
    - Path: 0072-m5dial-timer-demo/main/ui_timer_screen.cpp
      Note: Reference for round-screen visual ambition and LVGL composition
    - Path: 0073-m5dial-film-developer-timer/CMakeLists.txt
      Note: Scaffolded app now exists and uses its own project name
    - Path: 0073-m5dial-film-developer-timer/README.md
      Note: Scaffolded app README for the future film developer timer
    - Path: 0073-m5dial-film-developer-timer/main/film_catalog.cpp
      Note: Runtime catalog initialization path added in the first implementation step
    - Path: 0073-m5dial-film-developer-timer/main/film_catalog.h
      Note: Runtime catalog contract added in the first implementation step
    - Path: 0073-m5dial-film-developer-timer/main/generated_film_catalog.cpp
      Note: Generated curated runtime recipe catalog for v1
    - Path: film_dev_times.json
      Note: Raw film-development source data that must be curated before runtime use
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py
      Note: Ticket-local script summarizing the source dataset
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/generate_film_catalog_cpp.py
      Note: Generator script that produces the runtime C++ catalog
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py
      Note: Ticket-local script evaluating the starter-scope subset
ExternalSources: []
Summary: Detailed intern-focused plan for a new M5Dial film developer timer app built by copying 0072, adding a curated film/developer catalog derived from film_dev_times.json, and redesigning the UI around film, temperature, push/pull, and optional developer selection.
LastUpdated: 2026-03-06T21:02:48.638166201-05:00
WhatFor: Use this document to understand the current M5Dial timer architecture, the constraints of film_dev_times.json, and the proposed design for a film developer timer app.
WhenToUse: Use when implementing, reviewing, or onboarding onto the proposed 0073 M5Dial film developer timer application.
---




# M5Dial film developer timer analysis, design, and implementation guide

## Executive Summary

This document proposes a new M5Dial application, tentatively named `0073-m5dial-film-developer-timer`, built by copying the existing `0072-m5dial-timer-demo` project and changing its domain from a generic countdown timer to a film development timer. The new app should let the user select a film, temperature, push or pull adjustment, and, when more than one choice exists, a developer and dilution. After selection, the app should present a polished countdown screen tuned for the actual development time from the dataset.

The important technical conclusion is that the raw dataset at `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json` should not be treated as the runtime UI database. It is large, heterogeneous, and not optimized for rotary-selector use on a 240x240 round screen. The recommended approach is to derive a curated starter catalog from that file and check that smaller result into the app, either as a generated compact JSON or a generated C++ source artifact.

For v1, the guide recommends a deliberately constrained catalog:

- common black-and-white developers only
- color negative / C-41 support only where the source file actually contains explicit usable rows
- a selector flow that always feels fast on-device
- reuse of the working board, event-bus, and LVGL scaffolding already proven in `0072`

This makes the first version achievable and stable, while leaving room for a later “full library browser” once the data pipeline is stronger.

## Problem Statement

The current M5Dial demo at `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo` proves that the hardware path works: the dial display is initialized correctly, the encoder and center button are usable, touch swipes work, and the app already has a clean I/O-task to UI-task event boundary. However, its domain model is intentionally tiny. It only knows about one duration value and one timer state machine. It cannot answer a more realistic workflow like “I shot HP5+ at EI 800, I want Rodinal at 20 C, what time should I run?”

The source film-development data exists, but it is not ready for direct use as an on-device menu database. The current JSON file is very large and internally uneven:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json` is about `7,911,909` bytes and `324,602` lines
- it contains `14,293` entries and `340` films
- the dominant category is black-and-white (`13,160` entries)
- temperature is mostly normalized to `{celsius, fahrenheit}`, but some rows use only `{"raw": "20"}`
- push/pull values are numerous and irregular (`86` distinct `push_pull_type` labels)
- explicit color-negative / C-41 coverage is sparse in the current file

That means the core design problem is not just “build a new screen.” It is:

1. decide what the runtime catalog should look like,
2. decide how the selector state should work on a round, encoder-driven UI,
3. decide what to do when the source data is incomplete or inconsistent,
4. preserve the already-working M5Dial hardware architecture instead of re-solving the board layer.

## Scope

### In Scope

- A new app cloned from `0072`
- Film selection
- Temperature selection
- Push or pull selection
- Optional developer selection when the catalog has more than one developer for the current film
- Optional dilution display or selection when multiple dilutions exist
- Countdown timer screen based on the selected development time
- Detailed documentation for implementation and onboarding
- Starter scope limited to:
  - common B/W developers
  - whatever explicit C-41-like rows are present and usable in the source data

### Out of Scope for v1

- Full browsing of all `14k+` raw source rows on-device
- User notes display for every source record
- Agitation programs
- Multi-step process orchestration (pre-wash, stop, fix, wash) as separate timers
- A perfect or exhaustive C-41 catalog if the source data does not already provide it
- Touch-heavy UI paradigms that compete with the encoder-first interaction model

## Current State Architecture

This section is evidence-based. It maps the parts of `0072` that should be reused rather than redesigned.

### Runtime Task Model

The app entrypoint in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp` already uses a healthy split:

- an I/O task polls hardware and emits normalized events
- a UI task owns LVGL and consumes those events
- a FreeRTOS queue connects the two

Relevant lines:

- `AppContext` with board + queue: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp:26`
- `io_task()`: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp:31`
- `ui_task()`: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp:42`
- queue creation and task startup: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp:87`

This is the correct base for the film app. The new app should keep this shape.

### Input Event Abstraction

The current event contract is small and useful:

- encoder delta
- short button press
- long button press
- swipe direction

See `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h:7`.

This means the film app already has a transport for:

- moving a selector
- confirming a choice
- going back or resetting
- changing a visual mode or switching top-level panels

The event layer should be extended only if needed. It should not be replaced.

### Board Layer

The working M5Dial board wrapper is in:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h`

Important responsibilities it already handles:

- power hold
- LCD configuration
- backlight
- encoder setup through `ESP32Encoder`
- debounced center button with ISR-assisted wakeups
- FT3267 touch bring-up and swipe reporting

Important evidence:

- LCD pins and panel setup: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp:20`
- board init and display bring-up: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp:119`
- button debounce and long press logic: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp:181`
- touch initialization: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp:219`
- swipe detection: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp:267`

The film app should reuse this board layer almost unchanged.

### Timer Domain Model

The current timer model is intentionally generic:

- state enum
- duration
- remaining time
- tick/update logic
- start/pause/reset

See `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_model.h:7`.

This is a good base, but the film app will need more domain data above it:

- a selected recipe
- metadata about film, developer, temperature, push/pull, and dilution
- one authoritative chosen development time

The recommendation is to keep a simple countdown engine, but wrap it inside a richer “recipe selection + timer session” model.

### UI Layer

The current screen in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp` proves that:

- LVGL works on the round panel
- the app can render a stylized, non-default screen
- state-dependent color and copy updates are already in place

Evidence:

- root/orb/arc layout creation: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp:78`
- labels and hierarchy: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp:115`
- apply path: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp:156`

The film app should not reuse this exact screen structure, but it should reuse its visual ambition: large center focus, intentional typography, and a circular composition that fits the hardware.

## Evidence From film_dev_times.json

The source dataset is at:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json`

The ticket-local scripts created during this investigation are:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py`

### What the Source File Tells Us

- File size is large enough that direct runtime parsing is unattractive on the dial.
- Most data is B/W.
- The source categories are:
  - `bw`
  - `cine`
  - `infrared`
  - `color_negative`
  - `color_reversal`
- The dominant temperature is `20.0 C`.
- Push/pull labels are numerous and irregular.
- Temperature format is not perfectly normalized.

The analysis scripts showed:

```text
entries 14293
films 340
categories [('bw', 13160), ('cine', 482), ('infrared', 331), ('color_negative', 292), ('color_reversal', 28)]
top temperatures [(20.0, 12163), (24.0, 1050), (21.0, 415), (22.0, 339), ...]
distinct push types 86
temperature key variants [('celsius', 'fahrenheit'), ('raw',)]
```

### What This Means for Design

This data is valuable as source material, but it is not yet a user-ready catalog. A compact on-device selector wants:

- normalized temperature labels
- normalized push/pull labels
- a small number of decisions per screen
- deterministic ordering
- duplicate collapse rules

The raw file has none of those guarantees.

## Recommended Product Shape

The app should feel like a “recipe chooser and execution timer,” not a spreadsheet browser.

### Core User Flow

1. Choose film
2. Choose developer if needed
3. Choose dilution if needed
4. Choose temperature
5. Choose push/pull
6. Review recipe
7. Start timer
8. Pause/resume/reset as needed

This is the simplest flow that matches the requested feature set.

### Why Not Show Everything At Once?

The M5Dial screen is round and small. Trying to place film, developer, dilution, temperature, push/pull, and time all on one screen would create one of two bad outcomes:

- unreadable cramped text
- a fake “dashboard” that looks impressive in code but is poor in use

The device wants a staged interaction model. Encoder-driven devices are good at:

- focus
- stepping
- confirming
- revisiting previous choices

They are not good at giant option matrices.

## Proposed Architecture

The recommended project path is:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer`

The fastest implementation path is to copy `0072` and then replace the domain modules.

### High-Level Module Diagram

```text
film_dev_times.json
        |
        v
curation / normalization script
        |
        v
generated compact catalog
        |
        v
FilmCatalog
        |
        v
RecipeSelectorModel <-> FilmTimerModel
        |
        v
FilmTimerController
        |
        v
FilmSelectorScreen / FilmRunScreen
        |
        v
LVGL UI task

M5DialBoard -> InputEvent queue -> UI task
```

### Reuse vs Replace

Keep from `0072`:

- board layer
- LVGL port
- I/O task and UI task split
- input event queue
- build setup, partitions, and flashing workflow

Replace or add:

- timer domain model
- controller logic
- screen layout
- data loading and catalog normalization
- README and app naming

### Proposed Runtime Modules

#### 1. `FilmCatalog`

Purpose:

- own the curated runtime dataset
- expose sorted option lists for the current selection state
- return a selected recipe row with the final development time

Suggested files:

- `main/film_catalog.h`
- `main/film_catalog.cpp`
- optionally `main/generated_film_catalog.c` or `main/generated_film_catalog.json`

Suggested API sketch:

```cpp
struct FilmRecipeId {
  uint16_t index;
};

struct FilmRecipe {
  std::string_view film;
  std::string_view developer;
  std::string_view dilution;
  float temperature_c;
  std::string_view push_pull_type;
  float push_pull_stops;
  uint16_t time_seconds;
  bool has_120;
  bool has_35mm;
};

class FilmCatalog {
 public:
  bool init();

  std::span<const std::string_view> films() const;
  std::span<const std::string_view> developers_for(std::string_view film) const;
  std::span<const std::string_view> dilutions_for(std::string_view film, std::string_view developer) const;
  std::span<const float> temperatures_for(std::string_view film, std::string_view developer, std::string_view dilution) const;
  std::span<const std::string_view> push_pull_for(...) const;

  bool resolve_recipe(...selection..., FilmRecipe *out) const;
};
```

Intern guidance:

- do not let the UI query raw rows directly
- centralize sorting and filtering here
- make the rest of the app talk in terms of options, not JSON internals

#### 2. `RecipeSelectorModel`

Purpose:

- hold the currently highlighted field and chosen value
- know which fields are visible, optional, or auto-selected
- produce a “resolved recipe” when possible

Suggested files:

- `main/recipe_selector_model.h`
- `main/recipe_selector_model.cpp`

Suggested responsibilities:

- current focus row
- current scroll index within that row
- selected film/developer/dilution/temperature/push-pull
- validation state
- whether a row is locked, hidden, or auto-filled

This should be a pure state machine, not an LVGL class.

#### 3. `FilmTimerModel`

Purpose:

- wrap the generic countdown engine with recipe metadata
- expose both the selected recipe and the timer session state

Suggested files:

- `main/film_timer_model.h`
- `main/film_timer_model.cpp`

Suggested API sketch:

```cpp
enum class FilmTimerState {
  kSelecting,
  kReady,
  kRunning,
  kPaused,
  kComplete,
};

struct FilmTimerSnapshot {
  FilmTimerState state;
  bool recipe_ready;
  FilmRecipe recipe;
  uint32_t total_ms;
  uint32_t remaining_ms;
};
```

Important note:

The old `TimerModel` in `0072` can be partially reused, but do not stretch it until it becomes confusing. The film app has richer semantics and should name them clearly.

#### 4. `FilmTimerController`

Purpose:

- map `InputEvent` values into selector changes or timer actions

Suggested files:

- `main/film_timer_controller.h`
- `main/film_timer_controller.cpp`

Behavior sketch:

- encoder:
  - in selection mode: change current option
  - in run mode: no effect or maybe theme change only
- short press:
  - in selection mode: confirm / advance
  - in ready mode: start timer
  - in running mode: pause
  - in paused mode: resume
  - in complete mode: restart current recipe
- long press:
  - in selection mode: go back or reset selection
  - in run mode: reset to recipe review
- swipe:
  - switch theme
  - optionally move between tabs if later added

#### 5. `FilmSelectorScreen` and `FilmRunScreen`

Purpose:

- split the UI into two conceptual states instead of one overloaded screen

Suggested files:

- `main/ui_film_selector_screen.h`
- `main/ui_film_selector_screen.cpp`
- `main/ui_film_run_screen.h`
- `main/ui_film_run_screen.cpp`

Rationale:

- selection mode and countdown mode want different layout priorities
- separating them keeps LVGL code readable

## Data Strategy

This is the most important architecture decision in the whole design.

### Recommendation

Do not parse the full raw `film_dev_times.json` on every boot and do not browse it directly in the UI.

Instead:

1. create a ticket-local or app-local preprocessing script
2. normalize and filter the raw rows
3. generate a compact runtime catalog for the app

### Why

- `7.9 MB` raw JSON is expensive for a tiny device UI
- duplicate and irregular labels need cleanup
- the app needs stable sort order and predictable option lists
- the user explicitly wants a limited initial scope anyway

### Recommended v1 Catalog Rules

Start with:

- `film_category == bw` and developer in:
  - `D-76`
  - `HC-110`
  - `ID-11`
  - `Ilfosol 3`
  - `Ilfotec DD-X`
  - `Rodinal`
  - `TMax Dev`
  - `Xtol`
- plus any explicit `color_negative` rows that use:
  - `C-41`
  - or another explicitly whitelisted color developer if enough rows exist later

Important caveat:

The current source file appears to contain only a tiny amount of explicit C-41-like data. The guide therefore recommends that v1:

- include whatever explicit C-41 rows exist
- document the limitation clearly
- keep the architecture ready for a richer color catalog later

### Normalization Rules

Normalize temperature:

- if `temperature.celsius` exists, use it
- else if `temperature.raw` is parseable as a number, use that as Celsius
- else drop the row from the curated runtime catalog

Normalize time:

- convert `time_35mm` to seconds for the default runtime time
- optionally carry `time_120` too, but do not expose format selection unless the UI explicitly supports it

Normalize push/pull:

- keep original `push_pull_type`
- also compute a display label such as:
  - `Box`
  - `Push +1`
  - `Pull -1`
  - `Push +0.3`

Normalize developer/dilution:

- preserve source string for traceability
- trim whitespace
- collapse exact duplicates

### Suggested Preprocessing Pseudocode

```text
load raw json
for each raw entry:
  if category not in allowed starter categories:
    skip

  if developer not in allowed starter developers:
    skip

  temp_c = normalize_temperature(entry.temperature)
  if temp_c is invalid:
    skip

  time_min = choose_default_time(entry)
  if time_min is missing:
    skip

  emit normalized row:
    film
    developer
    dilution
    temp_c
    push_pull_type
    push_pull_stops
    time_seconds
    film_category

sort normalized rows by:
  film, developer, dilution, temp_c, push_pull_stops

write compact output
```

## UI Design

The app should look more like a premium recipe dial than a utilitarian menu list.

### Visual Direction

Suggested mood:

- dark lab / safelight inspired
- warm amber or red accent for active selection
- muted gray text for secondary metadata
- large circular timer ring for the run screen
- tactile stepped selector rows on the configuration screen

Avoid:

- rectangular settings-page layouts pasted into a round screen
- tiny text tables
- too many visible controls at once

### Screen 1: Selector

Purpose:

- choose the recipe

Layout idea:

```text
  FILM LAB

   HP5+                <- focused row
  Rodinal
  1+25
  20 C
  Push +1

  11:00
  press confirm
```

Design notes:

- one focused row should dominate
- other rows should remain visible for context
- the computed final time should always be visible
- if a row has only one valid option, show it dimmed and locked

### Screen 2: Review / Ready

Purpose:

- show the final chosen recipe before running

Layout idea:

```text
   TRI-X 400
  Rodinal 1+25
   20 C  Push +1

    13:30

  press start
  hold back
```

### Screen 3: Running Timer

Purpose:

- countdown and reinforce the chosen chemistry

Layout idea:

```text
     TRI-X
  Rodinal 1+25

     09:12

  20 C  Push +1
  press pause
  hold reset
```

The run screen should reuse the circular timer-ring language from `0072`, but the metadata beneath or above the time should describe the recipe, not generic timer labels.

### Suggested Interaction Rules

- encoder rotates current field values
- short press advances to the next selectable field
- long press moves back one field or resets the selection stack
- swipe changes theme only

Why this is good:

- encoder and button already work well on the hardware
- touch is nice for theme switching but should not carry the main information architecture

## Detailed Implementation Plan

This section is written for a new intern. It assumes the app does not exist yet.

### Phase 1: Clone the Existing App

Create a new project:

- copy `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo`
- new path:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer`

Rename project-facing strings:

- `README.md`
- project name in `CMakeLists.txt`
- log tags
- binary/app name if desired

At the end of this phase, the cloned app should still build unchanged.

### Phase 2: Add the Data Pipeline

Create a preprocessing script in the new app’s `scripts/` directory, for example:

- `0073-m5dial-film-developer-timer/scripts/build_curated_film_catalog.py`

Responsibilities:

- read `../film_dev_times.json`
- apply starter-scope filtering
- normalize rows
- emit compact output

Choose one runtime packaging strategy:

1. Generated JSON asset
   - easier to inspect
   - easier to regenerate
2. Generated C/C++ array
   - simpler runtime loading
   - no JSON parser needed in firmware

Recommendation for v1:

- generated C/C++ static array

Why:

- smallest runtime complexity
- predictable memory behavior
- easier to keep the UI responsive on boot

### Phase 3: Build `FilmCatalog`

Implement:

- `main/film_catalog.h`
- `main/film_catalog.cpp`

Tasks:

- load the generated compact catalog
- build deduplicated option lists
- expose queries by current selection state
- expose final resolved recipe lookup

Validation:

- add simple host-side or component-level tests if practical
- at minimum log the counts during boot

### Phase 4: Replace the Simple Timer Model

Implement:

- `main/recipe_selector_model.h`
- `main/recipe_selector_model.cpp`
- `main/film_timer_model.h`
- `main/film_timer_model.cpp`

The selector model should know:

- what the current step is
- which options exist for that step
- whether a step is auto-selected
- whether the current selection is complete

The timer model should know:

- selected recipe metadata
- total time
- remaining time
- countdown state

### Phase 5: Replace the UI

Create:

- `main/ui_film_selector_screen.*`
- `main/ui_film_run_screen.*`

Use the existing `0072` screen as a style reference, not a structural template.

Important UI guidelines for the intern:

- keep the current focus visually obvious
- keep line lengths short
- abbreviate safely when needed
- always display the computed time
- do not show fields that have not become relevant yet

### Phase 6: Replace the Controller

Implement:

- `main/film_timer_controller.h`
- `main/film_timer_controller.cpp`

Pseudo-flow:

```text
if state == selecting:
  encoder -> selector.rotate_current_option()
  short press -> selector.confirm_or_advance()
  long press -> selector.go_back_or_reset()

if selector becomes complete:
  timer_model.load_recipe(selector.resolved_recipe())

if state == ready:
  short press -> timer_model.start()
  long press -> selector.return_to_last_field()

if state == running:
  short press -> timer_model.pause()
  long press -> timer_model.reset_to_ready()

if state == paused:
  short press -> timer_model.resume()
  long press -> timer_model.reset_to_ready()
```

### Phase 7: Wire `app_main.cpp`

Keep the same high-level skeleton as `0072`.

New responsibilities in `app_main.cpp`:

- initialize `FilmCatalog`
- initialize selector and timer models
- initialize both screens
- route events to the new controller
- choose which screen is active based on model state

Do not:

- let the I/O task manipulate LVGL
- let the UI directly parse raw JSON

### Phase 8: Polish and Validation

On hardware, validate:

- encoder behavior on long option lists
- button confirm/back flow
- touch theme changes still work
- startup time is acceptable
- displayed time matches source rows
- edge cases with only one developer or only one temperature behave gracefully

## API and File Reference Map

These are the most important reference files for the future implementation.

Existing reusable files:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/lvgl_port_m5dial.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/lvgl_port_m5dial.cpp`

Source data and analysis tooling:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py`

Planned new files:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/recipe_selector_model.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/recipe_selector_model.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_model.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_model.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_controller.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_controller.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/ui_film_selector_screen.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/ui_film_selector_screen.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/ui_film_run_screen.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/ui_film_run_screen.cpp`

## Risks and Design Decisions

### Decision: Curated Catalog Instead of Full Raw Browse

Chosen because:

- the raw file is large
- the UI is small
- the source data is uneven
- the user explicitly asked to start smaller

Risk:

- some users may expect films or developers not present in v1

Mitigation:

- document the starter scope clearly
- make the catalog-generation pipeline easy to extend

### Decision: Encoder-First Navigation

Chosen because:

- it matches the working hardware path
- it simplifies the LVGL input story
- it is better for precise menu control than swipe-only navigation

Risk:

- long lists can feel slow

Mitigation:

- group options
- auto-skip single-choice fields
- optionally add accelerated scrolling later

### Decision: Separate Selector and Run Screens

Chosen because:

- selection and countdown have different information density needs
- this keeps the code legible

Risk:

- more LVGL code files

Mitigation:

- clearer responsibilities and simpler update logic

## Alternatives Considered

### Alternative 1: Parse `film_dev_times.json` directly on-device

Rejected because:

- large source file
- inconsistent schema details
- expensive and awkward for a small rotary UI

### Alternative 2: Keep the generic timer and only add presets

Rejected because:

- it hides the key domain concept, which is recipe selection
- it would turn the app into a timer with a confusing preset loader

### Alternative 3: Touch-first UI with multiple tabs

Rejected because:

- touch is currently a secondary-quality input on this app
- the encoder and button already provide the strongest reliable control path

## Validation Strategy

### Data Validation

- run the preprocessing script and print counts
- ensure no output rows have missing time, film, developer, or temperature
- inspect a few chosen recipes manually against the raw file

### Functional Validation

- app boots to selector screen
- selecting a recipe always yields a valid countdown time
- single-option fields auto-fill correctly
- timer start/pause/reset behavior still matches `0072`
- long press never loses the user in the selector flow

### UX Validation

- does rotating the encoder through film options feel too slow?
- do users understand when developer selection is optional versus required?
- is the review screen clear enough before start?
- are B/W and color recipes visually distinguishable if desired?

### Hardware Validation

- build with `idf.py build`
- flash with `idf.py -p /dev/ttyACM0 -b 115200 flash`
- confirm screen, encoder, button, and swipes all work

## Open Questions

These should remain visible during implementation.

1. Should v1 include only explicit `developer == C-41` color-negative rows, or should it also include a hand-curated expansion from nearby color-negative records?
2. Should the app expose film format (`35mm`, `120`, `sheet`) in v1, or just default to `35mm` time when present?
3. Should “developer selection” be hidden entirely when only one developer exists, or still shown as a locked row for clarity?
4. Should push/pull values be displayed exactly as source labels or normalized to friendlier labels only?
5. Should there be an audible completion tone, given the current sound path is not yet a focus of the cloned app?

## References

- Current working timer app:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo`
- Current dataset:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json`
- Ticket-local analysis scripts:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py`
