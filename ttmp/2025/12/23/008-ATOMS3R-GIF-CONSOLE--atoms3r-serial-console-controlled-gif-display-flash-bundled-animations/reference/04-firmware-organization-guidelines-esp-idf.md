---
Title: "Guidelines: organizing firmware projects in ESP-IDF (idioms, module boundaries, and a house style)"
Ticket: 008-ATOMS3R-GIF-CONSOLE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - serial
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp
      Note: Example “thin orchestrator” app_main after refactor
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp
      Note: Example of isolating esp_console command registration + REPL binding
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/control_plane.cpp
      Note: Example of ISR -> queue control plane separation
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp
      Note: Example of a small display HAL module with a stable API
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/uart_rx_heartbeat.cpp
      Note: Example of debug instrumentation that is Kconfig-gated
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/CMakeLists.txt
      Note: Example of listing module sources in idf_component_register()
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild
      Note: Example of project-level Kconfig for tutorial options and debug toggles
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/sdkconfig.defaults
      Note: Example of a committed default configuration for a tutorial
ExternalSources: []
Summary: "Practical, ESP-IDF-aligned guidelines for structuring firmware: how to split modules, when to create components, how to manage Kconfig/sdkconfig.defaults, and how to keep debug code from destabilizing production behavior."
LastUpdated: 2025-12-24T00:00:00.000000000-05:00
WhatFor: "A shared 'house style' that keeps ESP32 firmware readable, testable, and safe to debug—especially as projects grow beyond a single file."
WhenToUse: "When starting a new ESP-IDF firmware, refactoring a large main file, or reviewing PRs that add new subsystems/drivers/tasks."
---

## Executive summary

ESP-IDF doesn’t impose a single “one true” firmware layout, but it **strongly nudges you toward a component-oriented architecture**: each “piece” of functionality lives in a component with its own sources, dependencies, and (optionally) Kconfig options. At small scale, you can keep everything in `main/`. At medium scale, you usually start extracting reusable or high-churn code into `components/`.

In practice, the difference between a firmware that stays pleasant and one that becomes “a scary blob” comes down to a few habits:

- keep `app_main()` small and “script-like”
- keep module boundaries explicit (who owns the hardware pin? who owns the task? who owns the queue?)
- separate debug instrumentation from the production data path (and gate debug features with Kconfig)
- treat configuration (`sdkconfig.defaults`, project `Kconfig.projbuild`) as a first-class part of the firmware design

This document proposes a set of guidelines that are aligned with ESP-IDF’s build system and also fit this repository’s tutorial style.

## ESP-IDF idioms (the “shape” ESP-IDF wants)

Before proposing a house style, it helps to name the native ESP-IDF primitives you’re building with:

- **Project**
  - top-level `CMakeLists.txt`
  - `main/` component (your application)
  - optional `components/` directory (your reusable components)
  - `sdkconfig`, `sdkconfig.defaults`
- **Component**
  - a directory with a `CMakeLists.txt` containing `idf_component_register(...)`
  - a dependency list: `REQUIRES` and `PRIV_REQUIRES`
  - optional `Kconfig` / `Kconfig.projbuild` for menuconfig options

ESP-IDF’s build system doc is the canonical overview:

- `https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-guides/build-system.html`

The most important practical implication is:

> You don’t “import files” the way you would in Arduino; you add sources to a component and express dependencies so headers and libraries show up in the right places.

## Design goals (what “good organization” buys you)

You can judge a firmware layout by whether it enables these outcomes:

- **Local reasoning**: a module can be understood without reading 2,000 lines of unrelated code.
- **Safe change**: changing the console implementation shouldn’t risk breaking the display HAL.
- **Reusability**: if you build a good `gif_storage` module, you should be able to reuse it in another tutorial without copy/paste.
- **Debuggability**: you can enable instrumentation without changing core behavior.
- **Configurability**: pin mappings and “which transport?” choices are discoverable in `menuconfig` and have sane defaults.

## Recommended directory structure (project-scale)

At the project level, prefer a layout like:

```text
my-firmware/
  CMakeLists.txt
  main/
    CMakeLists.txt
    Kconfig.projbuild        # project-level options
    app_main.cpp             # thin orchestrator (or hello_world_main.cpp)
    ... internal modules ... # HAL/services that are project-specific
  components/
    my_component_a/
      CMakeLists.txt
      Kconfig                # component options
      include/               # public API headers
      src/                   # implementation
    my_component_b/
      ...
  sdkconfig.defaults
  partitions.csv
  README.md
```

**Rule of thumb**:

- Keep code in `main/` while it is clearly project-specific.
- Promote code to `components/` once you want reuse, independent versioning, or separate ownership.

## Recommended structure (module-scale within `main/`)

Within `main/`, the most readable pattern is a “thin orchestrator + modules” pattern:

```text
main/
  hello_world_main.cpp       # top-level "script": init everything, run main loop
  display_hal.{h,cpp}         # device-specific display bring-up + present
  backlight.{h,cpp}           # backlight control policies
  console_repl.{h,cpp}        # esp_console init + commands -> events
  control_plane.{h,cpp}       # queue + ISR -> events
  gif_registry.{h,cpp}        # list/lookup assets
  gif_player.{h,cpp}          # decode/render loop helpers
  uart_rx_heartbeat.{h,cpp}   # debug-only instrumentation (Kconfig gated)
```

We adopted exactly this pattern in `esp32-s3-m5/0013-atoms3r-gif-console/main/` as a concrete example.

## Module taxonomy (what kind of files should exist)

Organize modules by responsibility, not by “which chip peripheral they touch”.

- **HAL modules (`*_hal`)**: hardware abstraction for a specific board or wiring.
  - Example: `display_hal.{h,cpp}`
  - Characteristics:
    - stable API
    - owns the hardware instance (e.g. `display_get()`)
    - hides driver/library quirks

- **Service modules**: a higher-level subsystem with state and policy.
  - Example: `gif_player.{h,cpp}`
  - Characteristics:
    - own the internal state machine (e.g. “playing vs stopped”, frame delay policy)
    - provide “one step” APIs (`play_frame`, `reset`, `open`)

- **Integration modules**: glue between subsystems.
  - Example: `console_repl.{h,cpp}` turns CLI commands into control-plane events.

- **Debug/instrumentation modules**: tools to answer a specific debugging question.
  - Example: `uart_rx_heartbeat.{h,cpp}`
  - Characteristics:
    - gated behind Kconfig flags
    - must be designed to minimize side effects

## API boundaries (headers, ownership, and “who can call what”)

### Keep headers small and purposeful

- **One header per module** (`foo.h` for `foo.cpp`).
- Headers should describe:
  - what the module provides (public API)
  - what it owns (hardware resources, queues, tasks)
  - what invariants it expects (call order, thread/ISR context)

### Make ownership explicit

If a module owns a resource, encode it in the API shape:

- **Queues**: the module returns a handle and documents who consumes it.
  - Example: `QueueHandle_t ctrl_init(void);`
- **Tasks**: a module provides `*_start()` and holds its own task handle internally.
- **Pins**: the module’s header should state whether it configures pin mux, pullups, interrupts, etc.

### Prefer “commands enqueue events” over “commands directly mutate global state”

This is the most impactful habit for long-term sanity:

- console command handlers should be thin and non-blocking
- they should enqueue `CtrlEvent` to a central loop
- the central loop becomes the “single writer” of playback state

In pseudocode:

```c
// console_repl.cpp
int cmd_play(...) {
  ctrl_send({ .type = CtrlType::PlayIndex, .arg = idx });
}

// app_main loop
while (true) {
  if (xQueueReceive(ctrl_q, &ev, timeout)) {
    switch (ev.type) { ... mutate state ... }
  }
  if (playing) { ... render frame ... }
}
```

This avoids a class of bugs where multiple tasks write to the same state in subtle ways.

## Configuration guidelines (Kconfig + sdkconfig.defaults)

### Use `Kconfig.projbuild` for project-level options

Project-level knobs should live in `main/Kconfig.projbuild`:

- “Console binding: UART vs USB Serial/JTAG”
- “UART pins and baud”
- debug toggles like “Enable RX heartbeat”

Give options names that make collisions unlikely:

- `CONFIG_TUTORIAL_0013_*` rather than generic names like `CONFIG_UART_RX_GPIO`

### Use `sdkconfig.defaults` to define reproducible defaults

In tutorial-style repos, committing `sdkconfig.defaults` is extremely useful:

- it makes “fresh clone → build → flash” repeatable
- it documents the expected pin mappings and transport choices

**Guideline**:

- Commit `sdkconfig.defaults`.
- Treat `sdkconfig` as “stateful output” that may drift. If you do commit `sdkconfig` (as this repo sometimes does for tutorials), document why and keep it in sync intentionally.

## Debug instrumentation guidelines (how to avoid self-inflicted wounds)

Debug code is where firmware organization pays off the most, because “quick hacks” tend to become permanent.

- **Gate debug features in Kconfig**
  - default debug to off (or on only in tutorial contexts)
- **Avoid changing the production data path**
  - instrumentation should observe, not interfere
- **Avoid reconfiguring shared pins**
  - if UART owns RX, debug code must not `gpio_config()` that pin and steal the function mux
- **Prefer “log deltas” not “log every event”**
  - count edges/bytes and log periodically to avoid ISR storms and log flooding

The UART RX heartbeat in `0013` is a good example of what *not to do* (reconfigure the RX pin) and what to do instead (attach minimal observation).

## Build hygiene (CMake + dependencies)

### Keep `main/CMakeLists.txt` boring

Use a simple `idf_component_register(...)` listing the `SRCS` and dependencies.

Guidelines:

- Sort sources roughly by layer (app → services → HAL → utils) or alphabetically—just be consistent.
- Use `PRIV_REQUIRES` for internal dependencies; reserve `REQUIRES` for dependencies that leak into the public API.

### Watch out for header macro collisions

Embedded C/C++ libraries sometimes define “Arduino-isms” like `memcpy_P` macros.

Guideline:

- isolate third-party headers in a single `.cpp` module where possible
- if you must include them in multiple modules, document the include-order constraints

In `0013`, `AnimatedGIF.h` + LovyanGFX `pgmspace.h` can collide depending on include order; this is exactly the kind of “hidden footgun” a modular layout makes easier to contain.

## Review checklist (use this in PRs)

- **Structure**
  - [ ] `app_main()` reads like a script, not like a library
  - [ ] new subsystems live in their own module files
  - [ ] module boundaries match responsibilities (HAL vs service vs glue)
- **Concurrency**
  - [ ] ISRs are minimal; heavy work is deferred to tasks via queues
  - [ ] one “owner” task mutates shared state (single-writer pattern)
- **Config**
  - [ ] new knobs are in `Kconfig.projbuild` with clear help text
  - [ ] defaults are reflected in `sdkconfig.defaults` when appropriate
- **Debug**
  - [ ] debug features are gated and do not change pin mux / core behavior unintentionally
- **Build**
  - [ ] dependencies declared correctly (`PRIV_REQUIRES` vs `REQUIRES`)

## External resources

- **ESP-IDF build system overview**: `https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-guides/build-system.html`


