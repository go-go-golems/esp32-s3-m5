---
Title: Diary
Ticket: MO-01-JS-GPIO-EXERCIZER
Status: active
Topics:
    - esp32s3
    - cardputer
    - gpio
    - serial
    - console
    - esp-idf
    - tooling
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp
      Note: Persist control plane + engines after app_main (commit 520973b)
    - Path: esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/spiffs_image/.gitkeep
      Note: Track SPIFFS image dir so builds succeed on clean clones (commit 520973b)
ExternalSources: []
Summary: Implementation diary for the Cardputer-ADV MicroQuickJS GPIO exercizer.
LastUpdated: 2026-01-11T20:36:14-05:00
WhatFor: Track implementation progress, failures, and validation steps for the JS GPIO exercizer firmware.
WhenToUse: ""
---


# Diary

## Goal

Capture the research, analysis, and design brainstorming steps for the Cardputer-ADV MicroQuickJS GPIO/protocol exercizer.

## Step 1: Create ticket workspace and scaffolding

I set up the docmgr ticket workspace and created the core documents (analysis, design doc, diary). This establishes a stable place to capture analysis, decisions, and future implementation steps without mixing them into unrelated tickets.

This step also seeded initial tasks so the work stays trackable as the firmware plan evolves.

### What I did
- Created ticket `MO-01-JS-GPIO-EXERCIZER` with docmgr
- Added analysis, design-doc, and diary documents
- Added initial tasks to `tasks.md`

### Why
- Create a clean, navigable workspace for the firmware analysis and design effort

### What worked
- Ticket creation and doc scaffolding succeeded via docmgr

### What didn't work
- N/A

### What I learned
- The repo docmgr root is `esp32-s3-m5/ttmp`, so ticket paths land under that tree

### What was tricky to build
- Keeping docmgr tasks tidy (removed the default placeholder task)

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Start with `esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/index.md`
- Verify the doc list exists and the ticket has analysis/design/diary files

### Technical details
- Commands run:
  - `docmgr ticket create-ticket --ticket MO-01-JS-GPIO-EXERCIZER ...`
  - `docmgr doc add --ticket MO-01-JS-GPIO-EXERCIZER --doc-type analysis/design-doc/reference`
  - `docmgr task add --ticket MO-01-JS-GPIO-EXERCIZER --text "..."`

## Step 2: Survey existing firmware examples

I reviewed relevant firmware examples under `esp32-s3-m5/` to locate reusable patterns for REPL I/O, GPIO waveform generation, UART/I2C handling, and scheduling. The result is a mapped set of files and symbols that directly match the needs of a JS-driven signal exercizer.

I focused on the MicroQuickJS REPL base and the existing GPIO signal tester because they already implement the core mechanics we need.

### What I did
- Searched and opened firmware files in `esp32-s3-m5/` (MicroQuickJS REPL, GPIO signal tester, I2C sample, esp_console REPL)
- Captured reusable patterns and added them to the analysis doc

### Why
- Reduce implementation risk by reusing proven patterns and constraints

### What worked
- Identified direct reuse targets for REPL, control-plane queues, GPIO TX/RX, UART test loops, and I2C task scheduling

### What didn't work
- N/A

### What I learned
- The existing MicroQuickJS REPL already supports mode switching and SPIFFS autoload
- The GPIO signal tester uses esp_timer for square/pulse generation with a queue-based control plane

### What was tricky to build
- Distilling the most relevant files without inflating RelatedFiles too much

### What warrants a second pair of eyes
- Validate that the reuse map covers all protocol-related firmware examples that matter for Cardputer-ADV

### What should be done in the future
- Extend the reuse map with any additional RMT/LEDC protocol samples if we choose high-frequency waveforms

### Code review instructions
- Start in `esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/analysis/01-existing-firmware-analysis-and-reuse-map.md`
- Spot-check referenced files for correctness of the symbol names and flow descriptions

### Technical details
- Files reviewed include:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
  - `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp`
  - `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/uart_tester.cpp`
  - `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c`
  - `esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/console_repl.cpp`

## Step 3: Brainstorm JS API + runtime architecture

I drafted a design brainstorm that explores several JS API shapes and runtime architectures, with explicit weighting for real-time accuracy and ergonomics. The result is a proposed hybrid approach where JS compiles plans and dedicated signal engines execute them.

This step captures pros/cons for each option and documents how the real-time constraints of MicroQuickJS influence the design.

### What I did
- Wrote a design brainstorm covering API options and runtime architectures
- Added pros/cons and a weighted scoring table
- Drafted a hybrid proposed solution and implementation plan

### Why
- Avoid a premature API design that would break real-time expectations or overuse JS for timing

### What worked
- The analysis converged on a hybrid queue + signal-engine approach

### What didn't work
- N/A

### What I learned
- The best-fitting architecture blends declarative signal plans with protocol-specific engines

### What was tricky to build
- Balancing API ergonomics with strict real-time safety

### What warrants a second pair of eyes
- Confirm the scoring weights reflect the actual priorities for logic-analyzer validation work

### What should be done in the future
- Convert the brainstorm into a concrete implementation plan with per-module milestones

### Code review instructions
- Start in `esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/01-js-api-and-runtime-brainstorm.md`
- Verify the JS API sketches align with MicroQuickJS constraints and the reuse map

### Technical details
- Architectural sketch:
  - JS REPL task -> command queue -> GPIO/UART/I2C engines

## Step 4: Design high-frequency GPIO pattern VM

I drafted a new design brainstorm focused on a small VM that can encode high-speed GPIO patterns and bus waveforms. The doc frames the VM as a deterministic timing engine that JS configures, not one it drives directly.

I structured the brainstorm with a panel-style debate to surface tradeoffs between RMT-only approaches, timer-driven VMs, and richer instruction sets. The outcome favors a minimal opcode set with multiple backends and a simulator for validation.

### What I did
- Wrote the high-frequency GPIO pattern VM brainstorm design doc
- Added expert panel debate notes and a minimal instruction set sketch
- Documented design decisions, alternatives, and an implementation plan

### Why
- Provide a concrete path for high-frequency protocol emulation beyond JS timing limits

### What worked
- The VM proposal aligns with existing constraints about JS jitter and real-time safety

### What didn't work
- N/A

### What I learned
- A minimal opcode set plus a macro assembler keeps the VM simple while enabling complex bus patterns

### What was tricky to build
- Balancing RMT precision benefits against the need for a fallback backend and simulator

### What warrants a second pair of eyes
- Validate that the VM instruction set is sufficient for UART/I2C/SPI-style waveforms without overcomplicating execution

### What should be done in the future
- Turn the VM proposal into concrete tasks once backend requirements are finalized

### Code review instructions
- Start in `esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/02-high-frequency-gpio-pattern-vm-brainstorm.md`
- Confirm the expert debate aligns with the desired bus-emulation priorities

### Technical details
- Proposed VM backends: RMT compilation + gptimer ISR execution

## Step 5: Draft concrete implementation plan (GPIO + I2C engines)

I wrote a concrete implementation design doc that maps code modules, reuse targets, and pseudocode for the GPIO and I2C engines. The doc outlines a control-plane queue, engine tasks, and JS bindings, keeping MicroQuickJS as the control layer and engines as the timing layer.

This step adds actionable engineering detail without yet modifying firmware code. It also seeds new implementation tasks aligned with the design doc.

### What I did
- Authored a detailed implementation plan for GPIO + I2C engines
- Added explicit reuse references and pseudocode for each module
- Added new implementation tasks to the ticket

### Why
- Provide a concrete blueprint before starting firmware changes

### What worked
- The plan aligns with existing REPL, GPIO, and I2C patterns in the repo

### What didn't work
- N/A

### What I learned
- A clear module split (control-plane, GPIO engine, I2C engine, JS bindings) makes reuse and testing easier

### What was tricky to build
- Keeping the plan concrete without locking in pin mappings prematurely

### What warrants a second pair of eyes
- Confirm the proposed control-plane event schema is sufficient for both GPIO and I2C needs

### What should be done in the future
- Convert the implementation plan into actual firmware changes once pin defaults are decided

### Code review instructions
- Start in `esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/03-concrete-implementation-plan-js-gpio-exercizer-gpio-i2c.md`
- Review the pseudocode and reuse mapping for correctness against the source files

### Technical details
- Proposed modules: `control_plane`, `engine_gpio`, `engine_i2c`, `js_bindings`, `app_main`

## Step 6: Update implementation plan to use C++ components

I updated the concrete implementation design to model the control plane and engines as standalone C++ components, matching how `LineEditor` and `ReplLoop` are structured. This makes the plan consistent with existing reusable REPL pieces and clarifies how we will package the engines for reuse across firmware targets.

I also aligned the task list with the component-first approach so implementation work explicitly starts with component extraction before wiring the firmware entry point.

### What I did
- Updated the design doc to introduce `ControlPlane`, `GpioEngine`, and `I2cEngine` components
- Adjusted the code layout and pseudocode to use component classes
- Updated tasks to reflect component creation work

### Why
- Keep the engine/control-plane design aligned with existing reusable C++ components

### What worked
- The revised plan is clearer about reuse boundaries and dependencies

### What didn't work
- N/A

### What I learned
- Presenting engines as components simplifies future reuse beyond this firmware target

### What was tricky to build
- Updating multiple sections (architecture, layout, pseudocode) without missing any references

### What warrants a second pair of eyes
- Confirm the component layout names and boundaries fit existing repo conventions

### What should be done in the future
- Ensure the component CMake integration is captured once we start implementation

### Code review instructions
- Start in `esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/design-doc/03-concrete-implementation-plan-js-gpio-exercizer-gpio-i2c.md`
- Verify the component layout and updated pseudocode align with the task list

### Technical details
- Proposed components: `exercizer_control`, `exercizer_gpio`, `exercizer_i2c`

## Step 7: Implement ControlPlane + GPIO/I2C engines and new exercizer firmware

I implemented the C++ components for the control plane and the GPIO/I2C engines, then created the new firmware target that wires them into the MicroQuickJS REPL. The engines reuse the existing timer-based GPIO logic and the I2C worker-task pattern, while JS bindings are exposed through the MicroQuickJS stdlib generator.

This step also regenerated the MicroQuickJS stdlib header and atom table so the new JS APIs (`gpio.*`, `i2c.*`) are available in the REPL without runtime registration hacks.

### What I did
- Added `exercizer_control`, `exercizer_gpio`, and `exercizer_i2c` components
- Implemented GPIO waveform handling and I2C transaction queue + worker task
- Created `0039-cardputer-adv-js-gpio-exercizer` firmware and replaced app_main with exercizer wiring
- Added JS bindings in the stdlib generator/runtime and regenerated stdlib + atom headers

### Why
- Provide a reusable engine/control-plane layer and a concrete firmware target to flash on the Cardputer-ADV

### What worked
- Component scaffolding compiled cleanly (control plane + engines)
- Stdlib regeneration produced updated `esp32_stdlib.h` and `mquickjs_atom.h`

### What didn't work
- Stdlib generator warned: `Too many properties, consider increasing ATOM_ALIGN`

### What I learned
- Adding new JS bindings requires regenerating both the stdlib header and atom table to keep parsing stable

### What was tricky to build
- Keeping the JS binding definitions in sync between generator input and runtime implementation

### What warrants a second pair of eyes
- Confirm the ATOM_ALIGN warning is safe to ignore or needs a follow-up change

### What should be done in the future
- Evaluate whether to increase ATOM_ALIGN if more bindings are added

### Code review instructions
- Start in `esp32-s3-m5/components/exercizer_control/` then `esp32-s3-m5/components/exercizer_gpio/` and `esp32-s3-m5/components/exercizer_i2c/`
- Review `esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp` for wiring
- Check stdlib binding changes in `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`

### Technical details
- Commands run:
  - `cc -O2 -o tools/esp_stdlib_gen/esp_stdlib_gen tools/esp_stdlib_gen/esp_stdlib.c components/mquickjs/mquickjs_build.c components/mquickjs/cutils.c components/mquickjs/dtoa.c -Icomponents/mquickjs -lm`
  - `tools/gen_esp32_stdlib.sh`

## Step 8: Fix control-plane lifetime and boot-time esp_timer init

I stabilized the JS exercizer runtime by making the control plane and engines static and by handling `esp_timer_init()` returning "already initialized." This addresses the boot-time abort and the JS "control plane not ready" errors that appeared once `app_main` returned while the REPL continued.

With the control plane kept alive for the lifetime of the firmware, JS commands now target a valid queue, and the REPL can drive GPIO without racing a freed stack object. The spiffs image directory is also tracked explicitly so fresh clones build without manual setup.

**Commit (code):** 520973b — "Add JS GPIO exercizer firmware and engines"

### What I did
- Made the control plane and engines static in `app_main.cpp` so they survive after `app_main` returns
- Allowed `esp_timer_init()` to return `ESP_ERR_INVALID_STATE` without aborting
- Added a tracked SPIFFS image directory placeholder for reproducible builds
- Rebuilt the firmware (`idf.py build`)

### Why
- Prevent boot aborts and REPL-side failures caused by timer re-init and dangling control-plane pointers

### What worked
- Build completed successfully with the updated `app_main.cpp`
- REPL prompt appears after boot and no longer aborts on `esp_timer_init()`

### What didn't work
- `gpio.square(3,1000)` previously failed with `TypeError: gpio.square: control plane not ready`
- Boot abort when `ESP_ERROR_CHECK(esp_timer_init())` hit `ESP_ERR_INVALID_STATE`:
  - `ESP_ERROR_CHECK failed: esp_err_t 0x103 (ESP_ERR_INVALID_STATE) ... expression: esp_timer_init()`

### What I learned
- The REPL outlives `app_main`, so any control-plane state it depends on must be static or heap-owned
- `esp_timer_init()` is already called by IDF, so it must tolerate `ESP_ERR_INVALID_STATE`

### What was tricky to build
- Ensuring control-plane lifetime remains correct after `app_main` returns and tasks continue to run

### What warrants a second pair of eyes
- Confirm the static control-plane/engine objects are safe under concurrent task use and do not require explicit teardown

### What should be done in the future
- Consider a more explicit init log instead of the `esp_timer` warning if it becomes noisy

### Code review instructions
- Start in `esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp` and review the static control-plane wiring
- Validate with `idf.py build` and a hardware REPL test like `gpio.square(3, 1000)`

### Technical details
- Errors captured:
  - `E (296) esp_timer: Task is already initialized`
  - `TypeError: gpio.square: control plane not ready`
