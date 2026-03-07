---
Title: Investigation diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Dial-UserDemo/main/hal/hal.cpp
      Note: M5Dial board init evidence gathered during research
    - Path: esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: LVGL port reference used during research
    - Path: esp32-s3-m5/0071-cardputer-adv-photo-timer/main/timer_engine.cpp
      Note: Timer model reference used during research
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/CMakeLists.txt
      Note: |-
        Root project wiring for the new M5Dial timer tutorial (commit b39be1f)
        Project-level LovyanGFX legacy-I2C define restored for Step 5
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/README.md
      Note: User-facing control docs updated for touch-swipe theme cycling in Step 5
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/CMakeLists.txt
      Note: Build wiring updated to compile reused encoder and button support sources in Step 4
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp
      Note: |-
        Stage-1 hardware smoke test screen and polling loop (commit b39be1f)
        Single-task LVGL loop and runtime watchdog fixes (commit f91abc2)
        Restored timer UI after raw graphics-test diagnosis in Step 4
        Two-task I/O-to-UI split and LVGL buffer tuning from Steps 6 and 7
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h
      Note: Queue-backed input event schema introduced in Step 6
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/lvgl_port_m5dial.cpp
      Note: LVGL display flush port added for the M5Dial timer demo (commit f91abc2)
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp
      Note: |-
        Initial M5Dial board bring-up and normalized hardware access (commit b39be1f)
        Display wrapper stabilization and PCNT-backed encoder migration recorded in Step 4
        FT3267 initialization
        Hybrid ISR/task button path and event production
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h
      Note: |-
        Board state updated for hardware-backed encoder ownership in Step 4
        Touch and swipe board APIs extended in Step 5
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp
      Note: |-
        Encoder and button control mapping for the timer demo (commit f91abc2)
        Swipe gestures mapped into theme cycling in Step 5
        Consumes normalized input events from the queue
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/timer_model.cpp
      Note: Non-blocking countdown model implemented for the tutorial (commit f91abc2)
    - Path: esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp
      Note: |-
        Round-screen timer UI implementation and styling (commit f91abc2)
        Multiple theme palettes and swipe-visible styling added in Step 5
        View-state caching and redraw quantization to reduce visible tearing
    - Path: esp32-s3-m5/ttmp/2026/03/06/ESP-26-M5DIAL-TIMER-DEMO--m5dial-timer-demo-tutorial-and-implementation-guide/design-doc/01-m5dial-timer-demo-analysis-design-and-implementation-guide.md
      Note: Primary deliverable recorded in the diary
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-06T19:14:43.05478957-05:00
WhatFor: ""
WhenToUse: ""
---







# Investigation diary

## Goal

Record how the M5Dial timer demo guide was assembled, which source files were used as evidence, which design direction was chosen, and how to continue the work later without repeating the discovery phase.

## Context

This ticket did not implement the new tutorial firmware yet. Instead, it created a research-backed design and implementation guide for a future `esp32-s3-m5` tutorial project that targets M5Dial hardware. The design work happened immediately after the M5Dial build-and-boot repair work in `M5Dial-UserDemo`, so the codebase context was fresh and the M5Dial hardware paths had recently been validated on real hardware.

## Step 1: Create the ticket and write the M5Dial tutorial guide

The first step was to treat the request as a documentation-and-design task, not a coding task. The main work was to identify which existing projects in the repository provide the right building blocks for a new M5Dial tutorial and then turn that into a concrete, intern-friendly implementation guide.

The key design decision was to split responsibilities across repositories: use `M5Dial-UserDemo` only as the trusted hardware reference for the M5Dial board, and use `esp32-s3-m5` as the place where the new tutorial should live. This avoided dragging the older launcher/app framework into the new guide.

### Prompt Context

**User prompt (verbatim):** "cool. Now let's create a docmgr ticket and create a guide to create a simple m5 dial demo in our esp32-s3-m5 folder. It should allow me to control a timer, and look good.

Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new docmgr ticket in `esp32-s3-m5`, write a detailed architecture and implementation guide for a future M5Dial timer tutorial, keep the writing suitable for a new intern, and publish the resulting docs to reMarkable.

**Inferred user intent:** Preserve the recent M5Dial debugging knowledge in a reusable form and turn it into a high-quality onboarding document that makes the next implementation step much easier.

**Commit (code):** N/A

### What I did
- Created ticket `ESP-26-M5DIAL-TIMER-DEMO` in `esp32-s3-m5/ttmp`.
- Added a primary design doc and a diary document to the ticket.
- Inspected docmgr ticket conventions and available vocabulary before choosing the ticket topics.
- Gathered evidence from `M5Dial-UserDemo` for:
  - board power-hold
  - display panel wiring
  - touch controller wiring and protocol
  - encoder and button pins
  - buzzer path
- Gathered evidence from `esp32-s3-m5` tutorials for:
  - tutorial folder structure
  - local `sdkconfig.defaults` conventions
  - LVGL porting pattern
  - timer engine pattern
- Wrote the design doc with:
  - executive summary
  - current-state analysis
  - gap analysis
  - proposed architecture
  - module boundaries
  - pseudocode
  - diagrams
  - file-level implementation plan
  - validation strategy
  - risks and open questions

### Why
- The older M5Dial app is too broad to serve as a first tutorial.
- The tutorial repo already contains better examples of project structure and LVGL wiring.
- The user asked for a document that an intern can actually follow, which requires stronger structure and more explanation than a normal engineering note.

### What worked
- `docmgr ticket create-ticket` and `docmgr doc add` created the ticket structure quickly.
- The existing repository provided enough evidence to avoid speculative recommendations.
- `0025-cardputer-lvgl-demo` was the cleanest reference for the LVGL port.
- `0071-cardputer-adv-photo-timer` was the cleanest reference for the timer engine and UI refresh model.
- `M5Dial-UserDemo` had all the M5Dial pin and board-init facts needed for the future tutorial.

### What didn't work
- There is no existing M5Dial tutorial under `esp32-s3-m5`, so the new guide had to propose a future directory and file layout rather than documenting an existing implementation.
- The older `M5Dial-UserDemo` LVGL code was not a good direct reference for the new tutorial because `LVGL_ENABLE` is disabled by default in `main/hal/hal.h`, and the overall project architecture is much larger than necessary for a teaching example.

### What I learned
- The best path is not "port the whole M5Dial demo," but "extract the M5Dial board facts and combine them with the cleaner `esp32-s3-m5` tutorial patterns."
- The timer domain model from `0071` is reusable even though the hardware target differs.
- The recent M5Dial repair work materially improved confidence in the recommended board-level design because the power, I2C, display, and PWM issues were all exercised on current ESP-IDF.

### What was tricky to build
- The tricky part was deciding what to inherit from which project. The repository contains several valid display and UI approaches, but they serve different goals. The old M5Dial firmware is the most hardware-accurate source, while the newer `esp32-s3-m5` tutorials are the most pedagogically useful. Mixing those two without creating an incoherent guide required keeping a strict separation:
  - hardware bring-up comes from `M5Dial-UserDemo`
  - tutorial structure comes from `esp32-s3-m5`
  - LVGL porting comes from `0025`
  - timer logic comes from `0071`

### What warrants a second pair of eyes
- Whether the new tutorial should expose `M5GFX` naming or direct `LovyanGFX` naming for the M5Dial board layer.
- Whether touch should be included in v1 or deferred to keep the first implementation smaller.
- Whether the future implementation should vendor a M5Dial-specific graphics component inside `esp32-s3-m5` or point at an existing vendored copy elsewhere in the mono-workspace.

### What should be done in the future
- Implement `esp32-s3-m5/0072-m5dial-timer-demo/` following the guide.
- Decide the graphics component strategy before coding.
- Build and hardware-validate the first implementation.
- Add screenshots once the actual firmware exists.

### Code review instructions
- Start with the design doc:
  - `design-doc/01-m5dial-timer-demo-analysis-design-and-implementation-guide.md`
- Then verify the evidence sources mentioned in that doc:
  - `M5Dial-UserDemo/main/hal/hal.cpp`
  - `M5Dial-UserDemo/main/hal/display/hal_display.hpp`
  - `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp`
  - `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/timer_engine.cpp`
- Validate ticket hygiene with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
docmgr doctor --ticket ESP-26-M5DIAL-TIMER-DEMO --stale-after 30
```

### Technical details

Commands used during the investigation:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
docmgr status --summary-only
docmgr ticket list
docmgr ticket create-ticket --ticket ESP-26-M5DIAL-TIMER-DEMO --title "M5Dial timer demo tutorial and implementation guide" --topics esp32-s3,esp32s3,firmware,m5stack,m5gfx,timer,ui
docmgr doc add --ticket ESP-26-M5DIAL-TIMER-DEMO --doc-type design-doc --title "M5Dial timer demo analysis, design, and implementation guide"
docmgr doc add --ticket ESP-26-M5DIAL-TIMER-DEMO --doc-type reference --title "Investigation diary"
```

Repository evidence commands:

```bash
rg --files esp32-s3-m5 M5Dial-UserDemo
rg -n "app_main|lv_init|lv_timer_handler|M5GFX|LovyanGFX|timer|encoder|touch" esp32-s3-m5 M5Dial-UserDemo -S
nl -ba M5Dial-UserDemo/main/hal/hal.cpp | sed -n '1,260p'
nl -ba M5Dial-UserDemo/main/hal/display/hal_display.hpp | sed -n '1,220p'
nl -ba esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp | sed -n '1,260p'
nl -ba esp32-s3-m5/0071-cardputer-adv-photo-timer/main/timer_engine.cpp | sed -n '1,260p'
```

## Quick Reference

Recommended future tutorial directory:

```text
esp32-s3-m5/0072-m5dial-timer-demo/
```

Recommended evidence sources:

- M5Dial hardware: `M5Dial-UserDemo/main/hal/*`
- LVGL port structure: `esp32-s3-m5/0025-cardputer-lvgl-demo/main/*`
- timer engine structure: `esp32-s3-m5/0071-cardputer-adv-photo-timer/main/*`

## Usage Examples

Use this diary when continuing the ticket:

1. Reopen the design doc and choose the graphics component strategy.
2. Scaffold the future `0072-m5dial-timer-demo` tutorial.
3. Copy only the required M5Dial board constants and initialization logic.
4. Reuse the LVGL and timer patterns identified here.
5. Update this diary with implementation and hardware validation steps.

## Related

- `../design-doc/01-m5dial-timer-demo-analysis-design-and-implementation-guide.md`

## Step 2: Scaffold the 0072 project and prove the first clean build

This step converted the ticket from design-only work into a real firmware project. The goal was not to build the final timer UI yet, but to create a minimal, correct M5Dial project under `esp32-s3-m5`, commit the board bring-up foundation, and prove that the project can build cleanly on the current `esp-idf-5.4.1` environment.

The first milestone intentionally stopped at a hardware smoke test. That kept the scope tight enough to validate the project structure, graphics-component strategy, flash layout, and board-level pin mapping before introducing LVGL ownership, timer-state logic, and UI behavior. It also created a clean commit boundary so later runtime fixes can be reviewed independently.

### Prompt Context

**User prompt (verbatim):** "ok now build  the system step by step, checking off and committing appropriately, keeping a detailed diary. device is on /dev/ttyACM0"

**Assistant interpretation:** Start implementing the new tutorial firmware incrementally, keep the work broken into reviewable milestones, commit each completed milestone, and record the technical details in the ticket diary as the build progresses.

**Inferred user intent:** Turn the design ticket into a working M5Dial tutorial project without losing traceability, so another engineer can understand exactly what was built, what failed, and how to continue.

**Commit (code):** b39be1f426f55969788b7e13b746e9fdaa93a1af — "Scaffold M5Dial timer demo"

### What I did
- Chose direct `LovyanGFX` reuse from `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/components/LovyanGFX` instead of trying to start from an older `M5GFX` or `M5Unified` copy.
- Created `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/` with:
  - `CMakeLists.txt`
  - `README.md`
  - `sdkconfig.defaults`
  - `partitions.csv`
  - `dependencies.lock`
  - `main/CMakeLists.txt`
  - `main/idf_component.yml`
  - `main/m5dial_board.h`
  - `main/m5dial_board.cpp`
  - `main/app_main.cpp`
- Implemented an initial `M5DialBoard` abstraction that owns:
  - power hold
  - panel and backlight setup
  - touch controller setup
  - encoder delta normalization
  - button press normalization
- Implemented a stage-1 smoke-test `app_main.cpp` that draws a simple status screen and polls encoder, button, and touch input in a loop.
- Ran the first builds under the shared `esp32-s3-m5/.envrc` environment.
- Corrected the target-selection issue with an explicit `idf.py set-target esp32s3`.
- Added the missing `driver/i2c.h` include needed by the board-layer code on ESP-IDF 5.4.
- Built the project successfully and recorded the resulting size and partition headroom.

### Why
- The ticket needed a real code base before the design could be validated.
- A board-layer smoke test is the fastest way to confirm that the project structure and hardware constants are coherent.
- Reusing the already-repaired `LovyanGFX` copy reduces unknowns compared with reviving a second, older graphics stack.
- Stopping before LVGL keeps the first implementation milestone narrow and easy to review.

### What worked
- The direct `LovyanGFX` strategy integrated cleanly with a new ESP-IDF project under `esp32-s3-m5`.
- The custom `partitions.csv` and `sdkconfig.defaults` produced a large app slot immediately, avoiding the earlier single-app size trap seen in `M5Dial-UserDemo`.
- After setting the correct target, the project built cleanly on ESP-IDF `v5.4.1`.
- Final build result:
  - binary size: `0x4c170`
  - smallest app partition: `0x400000`
  - free headroom: `0x3b3e90` bytes (`93%`)

### What didn't work
- The first build inherited the wrong target because `.envrc` now only exports the toolchain and no longer runs `idf.py set-target esp32s3`.
- Exact failing command:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py build
```

- Exact failure symptoms from that incorrect-target build included:
  - `'GPIO_NUM_46' was not declared in this scope`
  - `'GPIO_NUM_42' was not declared in this scope`
  - `'GPIO_NUM_41' was not declared in this scope`
  - `'GPIO_NUM_40' was not declared in this scope`
- Before adding the missing header, the board-layer code also lacked the ESP-IDF I2C driver declaration needed for `I2C_NUM_0`.

### What I learned
- New tutorial projects in this workspace must explicitly set their chip target at least once because the shared `.envrc` no longer does that automatically.
- The simplest reliable starting point is a small board wrapper plus a smoke-test loop, not immediate LVGL integration.
- The hardware constants extracted from `M5Dial-UserDemo` are internally consistent enough to support a clean standalone project.

### What was tricky to build
- The tricky part was separating "toolchain environment is loaded" from "project target is configured." Those used to be coupled when `.envrc` ran `idf.py set-target esp32s3`, but they are now separate. The symptom was misleading board-pin compile errors, because the project was compiling as plain `esp32` rather than `esp32s3`. The fix was to treat target selection as an explicit one-time project bootstrap step:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py set-target esp32s3
idf.py build
```

- The other sharp edge was that the board wrapper uses the legacy ESP-IDF I2C driver types directly, so the translation unit needed `#include <driver/i2c.h>` explicitly rather than relying on transitive includes.

### What warrants a second pair of eyes
- The exact touch-controller GPIO choice should be hardware-validated on the real M5Dial before building the pointer input path on top of it.
- The current encoder decoder is intentionally simple and may need edge-case tuning if the physical dial produces noisy transitions.
- The graphics-component path points outside the `0072` directory tree, which is practical for now but may become a portability question later if the tutorial needs to stand alone.

### What should be done in the future
- Flash this stage-1 image to `/dev/ttyACM0` and verify:
  - boot
  - display output
  - encoder delta
  - button press detection
  - touch coordinates
- If the smoke test passes, replace the raw polling screen with the LVGL port, timer model, and controller/UI layers.
- Decide whether the final tutorial should keep referencing the shared vendored `LovyanGFX` path or copy the dependency closer to the tutorial.

### Code review instructions
- Start with the project scaffold:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/sdkconfig.defaults`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/partitions.csv`
- Then review the board layer:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
- Then review the current application entrypoint:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
source ../.envrc
idf.py set-target esp32s3
idf.py build
```

### Technical details

Commands used during this step:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation
rg --files M5Dial-UserDemo esp32-s3-m5
git -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5 status --short
lsof /dev/ttyACM0
```

Build commands:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py build
idf.py set-target esp32s3
idf.py build
```

Committed files:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/CMakeLists.txt`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/README.md`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/dependencies.lock`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/CMakeLists.txt`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/idf_component.yml`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/partitions.csv`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/sdkconfig.defaults`

## Step 3: Replace the smoke test with the first usable timer demo

This step turned the scaffolding into a real application. The code now boots into a styled round LVGL timer screen, uses the M5Dial encoder and center button as its control surface, and keeps all UI state mutation inside a single dedicated task. That is the first point where the project became a usable tutorial instead of only a board bring-up reference.

The step also flushed out two runtime problems that were easy to misdiagnose. The first was the shape of the task model: leaving a forever-loop in `app_main()` triggered the task watchdog. The second was subtler: even after moving the loop into its own task, `pdMS_TO_TICKS(5)` produced a zero-tick delay on the default FreeRTOS tick rate, which meant the app still starved the idle task. Both had to be fixed before the LVGL UI could be treated as stable.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Continue implementing the tutorial as a sequence of real milestones, commit each milestone, and preserve the exact technical decisions and failures in the diary.

**Inferred user intent:** End up with a working, reviewable M5Dial timer demo and a high-signal implementation record that another engineer can follow without rediscovering the same problems.

**Commit (code):** f91abc235bcf5c894f3b01c799c85ee08bbae43c — "Implement M5Dial timer demo UI"

### What I did
- Replaced the stage-1 smoke-test UI with a real timer stack:
  - `main/lvgl_port_m5dial.h`
  - `main/lvgl_port_m5dial.cpp`
  - `main/timer_model.h`
  - `main/timer_model.cpp`
  - `main/timer_controller.h`
  - `main/timer_controller.cpp`
  - `main/ui_timer_screen.h`
  - `main/ui_timer_screen.cpp`
- Reworked `main/app_main.cpp` to:
  - initialize the board
  - initialize LVGL
  - create the timer screen
  - run a single-task loop that polls input, updates the timer model, renders the UI, and calls `lv_timer_handler()`
- Extended the board layer with long-press detection so the timer has a reset gesture in addition to short-press start/pause.
- Updated the tutorial `README.md` with:
  - exact build commands
  - explicit `idf.py set-target esp32s3`
  - current control mapping
  - flash/monitor instructions
- Built, flashed, and monitored the full timer firmware on `/dev/ttyACM0`.

### Why
- A timer tutorial is not complete without an interaction model that works directly on the hardware.
- LVGL needed to be introduced once the board layer was proven so the tutorial could demonstrate a realistic UI loop.
- The demo needed one obvious control for reset; long-press on the center button is the least surprising option on this device.
- The watchdog issues had to be solved at this stage because they reflect core event-loop structure, not cosmetic polish.

### What worked
- The new timer firmware builds cleanly on ESP-IDF `v5.4.1`.
- The application binary remains comfortably within the custom app partition:
  - binary size: `0x9b940`
  - smallest app partition: `0x400000`
  - free headroom: `0x3646c0` bytes (`85%`)
- Hardware flash and boot succeeded on `/dev/ttyACM0`.
- The monitored boot did not reproduce the earlier `task_wdt`, `i2c: CONFLICT!`, or LEDC timer-conflict errors.
- The timer model now supports:
  - `idle`
  - `running`
  - `paused`
  - `complete`
- The control scheme is coherent for v1:
  - rotate to adjust duration
  - short press to start/pause/resume
  - long press to reset

### What didn't work
- The first attempt left the smoke-test loop inside `app_main()`, which caused the task watchdog to fire on CPU0 after boot:

```text
E (5496) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (5496) task_wdt:  - IDLE0 (CPU 0)
E (5496) task_wdt: Tasks currently running:
E (5496) task_wdt: CPU 0: main
```

- After moving the loop into a dedicated task, the watchdog still fired because `pdMS_TO_TICKS(5)` evaluated to `0` with the default FreeRTOS tick rate, so the loop was effectively busy-spinning on CPU1:

```text
E (5506) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (5506) task_wdt:  - IDLE1 (CPU 1)
E (5506) task_wdt: Tasks currently running:
E (5506) task_wdt: CPU 0: IDLE0
E (5506) task_wdt: CPU 1: m5dial_smoke
```

- The first timer-UI build also had a compile failure in `ui_timer_screen.cpp` because of an invalid `printf` format string:

```text
error: '+' flag used with '%u' gnu_printf format
```

### What I learned
- The right ownership model for this tutorial is:
  - `app_main()` creates the UI task and returns
  - the UI task owns board polling, timer updates, LVGL updates, and rendering
- On ESP-IDF, short millisecond sleeps are not safe to assume unless you know the FreeRTOS tick rate. `pdMS_TO_TICKS(5)` can become `0`, which is a real runtime bug, not a style issue.
- A small dedicated controller layer keeps the input mapping simple and reviewable instead of smearing control logic across the board layer and UI code.
- A complete v1 does not need touch or sound yet; encoder plus center-button control is enough to make the demo coherent.

### What was tricky to build
- The hardest part was distinguishing UI problems from scheduler problems. When the first LVGL-based firmware hit the watchdog, it would have been easy to blame display flushes or LVGL itself. The backtraces showed something else: first the main task shape was wrong, and after that the scheduler still was not getting a real blocking delay because `pdMS_TO_TICKS(5)` collapsed to zero. The fix sequence was:
  - move the forever-loop out of `app_main()`
  - run the app logic in its own task
  - increase the polling interval to `20 ms`
  - guard the task delay so it never becomes `0` ticks
- The other tricky point was deciding what not to implement yet. Touch and sound both exist in the hardware references, but adding them at this stage would have increased the runtime surface area before the base UI loop was stable.

### What warrants a second pair of eyes
- The timer UI layout should be looked at on real hardware to confirm the typography and spacing actually read well on the round screen.
- The chosen adjustment-step thresholds (`5s`, `15s`, `30s`, `60s`) are sensible defaults, but they are a product choice, not a hardware constraint.
- The long-press threshold may need tuning based on how the physical center button feels in use.
- The current v1 does not use LVGL encoder indev registration; it uses a local controller loop instead. That is intentional for simplicity, but it diverges from one possible reading of the original task list.

### What should be done in the future
- Verify the on-screen visuals and control feel directly on the device.
- Decide whether v2 should add:
  - touch shortcuts
  - buzzer feedback
  - LVGL encoder indev/group navigation
- Capture one or more photos or screenshots of the running timer UI.
- Update the design doc if the final teaching story should emphasize the direct controller loop over LVGL input-device registration.

### Code review instructions
- Start with the runtime loop and task ownership:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
- Then review the model/controller boundary:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_model.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_model.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp`
- Then review the LVGL wiring and screen composition:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/lvgl_port_m5dial.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp`
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
source ../.envrc
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
timeout 15s idf.py -p /dev/ttyACM0 monitor
```

### Technical details

Build and flash commands used:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
timeout 15s idf.py -p /dev/ttyACM0 monitor
```

Key runtime fixes captured in code:

- `app_main()` now returns immediately after spawning the app task.
- The UI loop uses a guaranteed non-zero task delay.
- Long-press reset is implemented in the board layer rather than being inferred in the UI.

Files added in this step:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/lvgl_port_m5dial.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/lvgl_port_m5dial.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_model.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_model.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.h`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp`

## Step 4: Stabilize the display path and move the encoder onto hardware counting

The first LVGL timer build booted cleanly and logged normal initialization, but the screen was still blank. At that point the right move was to stop treating it as a UI problem and reduce the system back down to a raw graphics test so the panel path could be isolated from LVGL, timer state, and input logic.

That display diagnosis exposed a board-layer mistake rather than an LVGL bug. Once the panel path was aligned with the known-working M5Dial configuration, the timer UI became visible. The remaining control issue was encoder feel: the original sampled GPIO decoder was too coarse and jittery, so it was replaced with the same PCNT-backed `ESP32Encoder` approach already proven in `M5Dial-UserDemo`.

### Prompt Context

**User prompt (verbatim):** "the encoder doesn't really work well, glitchy and slow."

**Assistant interpretation:** Stabilize the remaining hardware-facing parts of the tutorial firmware now that the timer UI boots, with emphasis on display correctness and usable encoder control.

**Inferred user intent:** Get the tutorial onto a state where it is no longer just technically booting, but is actually credible to use and demo on the real M5Dial hardware.

**Commit (code):** d04e6e0f087fbd9e9750008aa879801f7d252fef — "Stabilize M5Dial display and encoder"

### What I did
- Swapped the timer UI out temporarily for a raw graphics test so the display path could be verified independently from LVGL widget composition.
- Compared the local `LGFX_M5Dial` configuration against the known-good panel setup in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/display/hal_display.hpp`.
- Removed the attached `Touch_FT5x06` device from the display wrapper and set `bus_shared = true` in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`.
- Simplified `read_touch()` to return an empty touch state for v1 so the touch path cannot interfere with panel bring-up.
- Reflashed the graphics test to `/dev/ttyACM0` and confirmed the user could see the display output, which proved the panel and backlight path were working.
- Restored the timer UI after the display fix.
- Replaced the software quadrature decoder in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp` with a PCNT-backed `ESP32Encoder`.
- Updated `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/CMakeLists.txt` to compile the reused `ESP32Encoder.cpp` and `Button.cpp` sources from `M5Dial-UserDemo`.
- Set the encoder filter to `1023`, cleared the initial count on boot, reflashed the firmware, and rechecked the interaction on hardware.

### Why
- A blank screen after a healthy boot log usually means the display pipeline needs to be isolated before changing application logic.
- Reusing the known-good M5Dial display shape was lower risk than continuing to debug a custom hybrid configuration.
- A rotary encoder should feel stable at the hardware boundary; sampling GPIO transitions in a UI loop is the wrong place to absorb mechanical jitter.

### What worked
- The raw graphics test made the display issue visible immediately and avoided guessing about LVGL behavior.
- Matching the panel configuration to the working M5Dial reference fixed the invisible-screen problem.
- Removing touch from the display wrapper reduced the initialization surface area and made the board layer easier to reason about.
- The PCNT-backed encoder path materially improved interaction quality; the user reported the result as "much better."
- The rebuilt timer UI now renders and is usable on the real device.

### What didn't work
- The initial timer-UI firmware produced a blank screen even though the device booted and the backlight was on.
- The first custom `LovyanGFX` board wrapper attached a touch device during panel bring-up, which was not how the known-working M5Dial display class was structured.
- The original software quadrature decoder felt glitchy and slow because counts were inferred from a sampled transition table instead of accumulated by hardware between polls.

### What I learned
- For M5Dial, the display path should be treated as proven only when it matches the working panel configuration closely; small board-wrapper differences are enough to break real output.
- A simple raw-render diagnostic is worth keeping around because it cuts through LVGL ambiguity quickly.
- PCNT-backed rotary decoding is the right baseline for this hardware tutorial, even if the long-term API later migrates from the legacy `ESP32Encoder` wrapper to the newer IDF pulse-count driver.

### What was tricky to build
- The tricky part was separating "nothing is on screen" from "the app is drawing but not presenting." Boot logs and backlight state said the firmware was alive, but that still left too many possibilities: bad panel config, touch interference, LVGL flush bugs, or a drawing loop that never reached the panel. The solution was to reduce the problem aggressively:
  - replace the timer UI with a raw graphics test
  - align the panel wrapper with the known-good M5Dial display class
  - prove visible output first
  - then restore the timer UI
- The encoder issue had a similar shape. The first implementation was functionally correct, but because it depended on a periodic polling loop, the feel was bounded by the loop rate and by mechanical bounce. Moving the count accumulation into the ESP32 PCNT hardware removed most of that sensitivity without complicating the timer controller.

### What warrants a second pair of eyes
- The reused `ESP32Encoder` path still depends on the deprecated legacy PCNT driver, so it is stable for now but should eventually be reviewed against the IDF 5.4 `pulse_cnt` API.
- Touch is currently deferred in the board layer rather than fixed. That is the right scope cut for v1, but it means the project is not yet a complete M5Dial input reference.
- The timer UI is now visible and usable, but it should still be reviewed for polish and perceived responsiveness on the physical dial.

### What should be done in the future
- Decide whether v2 should restore touch input or keep the tutorial intentionally encoder-only.
- Capture a photo or screenshot of the running timer UI now that the panel path is stable.
- Consider replacing the reused legacy `ESP32Encoder` dependency with a tutorial-local wrapper on top of modern IDF pulse counting.
- Tune encoder step sizing and acceleration only after more physical use, not preemptively.

### Code review instructions
- Start with the board-layer changes:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h`
- Then review the build wiring for the reused encoder sources:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/CMakeLists.txt`
- Finally confirm the app still uses the restored timer UI rather than the temporary graphics test:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
source ../.envrc
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
timeout 15s idf.py -p /dev/ttyACM0 monitor
```

### Technical details

Key display diagnosis sequence:

```text
1. Timer UI boots but screen is blank.
2. Replace LVGL UI with direct graphics test.
3. Compare board wrapper against known-working M5Dial panel config.
4. Remove touch from the display object and set bus_shared = true.
5. Reflash and confirm visible graphics.
6. Restore timer UI on top of the corrected board layer.
```

Key encoder migration sequence:

```text
1. Remove software quadrature transition-table decoder.
2. Add ESP32Encoder source to the component build.
3. Attach PCNT-backed half-quadrature on pins 41/40.
4. Enable the maximum filter value (1023).
5. Read count deltas in poll() and consume them via take_encoder_delta().
```

Commands used during this step:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
timeout 12s idf.py -p /dev/ttyACM0 monitor
```

## Step 5: Add touch-swipe theme switching and restore the legacy I2C balance

Once the timer screen and encoder path were stable, touch could be added back in a narrower way than the original display-wrapper attempt. Instead of reattaching touch to `LovyanGFX`, this step added a board-owned FT3267 reader, turned raw touch movement into discrete swipe events, and used those events to cycle the timer screen through a small set of distinct visual themes.

The first boot with touch enabled failed immediately, but the failure was useful: it proved the earlier `driver_ng` versus legacy I2C fix in `M5Dial-UserDemo` had not been copied into `0072`. Adding the same global `LGFX_FORCE_LEGACY_I2C_DRIVER` define restored the expected balance, and the follow-up flash booted cleanly with `touch controller ready` in the log.

### Prompt Context

**User prompt (verbatim):** "Ok, now add touch swipes as well, to change the background color style or something."

**Assistant interpretation:** Reintroduce touch in a controlled way and give it a clear visible behavior that improves the demo without complicating the core timer interaction model.

**Inferred user intent:** Make the M5Dial demo feel more like a touch-capable product demo rather than a purely encoder-driven utility, while keeping the interaction simple and visually rewarding.

**Commit (code):** 1a9e0069a7e8b99722b94f570c13fe364f0fe729 — "Add touch swipe themes to M5Dial demo"

### What I did
- Added a legacy-I2C FT3267 reader directly inside `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp` using the known M5Dial touch pins (`11`/`12`) and the FT3267 register sequence from `M5Dial-UserDemo`.
- Extended `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h` with `SwipeDirection`, persistent touch state, and a `take_swipe()` API.
- Implemented simple swipe detection in the board poll loop by tracking touch-down, touch-release, displacement, and dominant axis.
- Updated `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp` so left/up swipes advance the theme and right/down swipes reverse it.
- Expanded `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp` with four named color palettes and theme-aware background/text styling.
- Updated `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/README.md` to document the new touch-swipe controls.
- Reproduced a boot-time failure after the first touch integration attempt:

```text
E (318) i2c: CONFLICT! driver_ng is not allowed to be used with this old driver
abort() was called at PC 0x4203ce13 on core 0
```

- Fixed that by adding `-DLGFX_FORCE_LEGACY_I2C_DRIVER` to `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/CMakeLists.txt`, matching the repaired `M5Dial-UserDemo` project.
- Rebuilt, reflashed, and re-monitored the device until the boot log showed:
  - `m5dial_board: touch controller ready`
  - `m5dial_board: board init complete`
  - `m5dial_timer_0072: timer demo started`

### Why
- Swipe-to-theme-change is a low-risk touch interaction that is easy to explain, easy to notice visually, and does not compete with the encoder’s primary timer role.
- Keeping touch as a board-owned input source avoids repeating the earlier display-wrapper coupling mistake.
- The `LGFX_FORCE_LEGACY_I2C_DRIVER` define is required because this tutorial mixes ESP-IDF’s legacy `driver/i2c.h` path with the vendored `LovyanGFX` component.

### What worked
- The FT3267 came up on the expected M5Dial pins and reported a clean initialization on hardware.
- Swipe events fit naturally into the existing controller pattern; no LVGL indev plumbing was needed.
- Theme changes are obvious on the round display because they affect the root background, orb background, supporting text colors, and hint text.
- The follow-up boot after restoring the LovyanGFX legacy-I2C define was clean.
- Flash, build, and monitor all succeeded after the fix.

### What didn't work
- The first touch-enabled flash immediately reintroduced the old/new I2C driver conflict:

```text
E (318) i2c: CONFLICT! driver_ng is not allowed to be used with this old driver
abort() was called at PC 0x4203ce13 on core 0
```

- The first non-interactive monitor attempt also hit the usual tooling limitation:

```text
Error: Monitor requires standard input to be attached to TTY. Try using a different terminal.
```

### What I learned
- The repaired `M5Dial-UserDemo` behavior depends not only on source changes inside `LovyanGFX`, but also on a project-level compile definition. Reusing the component without reusing that define is enough to regress at boot.
- Touch does not need to be fully LVGL-native to be useful in a small tutorial; board-level gestures are sufficient for a polished v1 interaction.
- Theme changes are a good first touch feature on a circular display because they reward a gesture immediately without creating layout complexity.

### What was tricky to build
- The tricky part was that the first failure looked like a new touch bug, but the backtrace pointed somewhere older and lower-level: `do_global_ctors` and the I2C conflict checker. That meant the crash was happening before the tutorial board code had a chance to run, which narrowed the root cause to component initialization and compile-time configuration. The fix sequence was:
  - add touch using the legacy I2C driver
  - flash and observe the immediate `driver_ng` conflict
  - compare the project-level build flags against the repaired `M5Dial-UserDemo`
  - restore `LGFX_FORCE_LEGACY_I2C_DRIVER`
  - rebuild, reflash, and verify the clean boot log
- Swipe detection itself also needed restraint. The goal was not a full gesture engine, just a robust enough horizontal/vertical classifier for a round timer UI. That is why the implementation uses a threshold plus axis dominance instead of trying to interpret every drag.

### What warrants a second pair of eyes
- The swipe thresholds (`36 px` threshold, `14 px` axis lead) are reasonable starting values, but they should still be judged on real fingers, not just code review.
- Raw touch orientation has only been validated indirectly through gesture handling, not through a dedicated coordinate test page.
- The project still intentionally mixes a deprecated legacy PCNT path and a deprecated legacy I2C path because that is the shortest stable route on IDF `5.4.1`; a longer-term cleanup pass should revisit both.

### What should be done in the future
- Confirm with human testing that swipe direction matches user expectation on the physical device.
- Decide whether to keep touch gesture handling local to the board/controller layer or eventually expose an LVGL pointer indev too.
- Add a tiny touch diagnostic screen if deeper coordinate debugging becomes necessary later.
- Consider moving from the legacy FT3267 + `driver/i2c.h` path to the newer IDF I2C API only when the graphics stack is migrated with it.

### Code review instructions
- Start with the board input layer and touch/I2C changes:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h`
- Then inspect the build fix that keeps LovyanGFX on the legacy I2C path:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/CMakeLists.txt`
- Then review the gesture-to-UI wiring:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp`
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
source ../.envrc
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
idf.py -p /dev/ttyACM0 monitor
```

### Technical details

Swipe mapping used in this step:

```text
left swipe  -> next theme
up swipe    -> next theme
right swipe -> previous theme
down swipe  -> previous theme
```

Key boot-log lines after the fix:

```text
W (...) i2c: This driver is an old driver, please migrate your application code to adapt `driver/i2c_master.h`
I (...) m5dial_board: touch controller ready
I (...) m5dial_board: board init complete
I (...) m5dial_timer_0072: timer demo started
```

Commands used during this step:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
idf.py -p /dev/ttyACM0 monitor
```

## Step 6: Split I/O from UI with a queue-backed event bus and ISR-assisted button wakeups

Once the display and touch paths were functional, the next hardware problem was interaction quality. The encoder theme-swiping demo was alive, but the center button still felt unreliable and the input path was too tightly coupled to the UI loop. That made the whole firmware feel softer and more erratic than it needed to be, even though the individual peripherals were technically working.

This step turned the app into a clearer two-task system: one task owns hardware polling and event production, the other owns LVGL and event consumption. The result is simpler timing behavior, fewer chances to miss short button transitions, and a safer ownership boundary around LVGL.

### Prompt Context

**User prompt (verbatim):** "i don't get anything, also the button press is not reliable, can we use a proper event bus between the io and the display tasks?"

**Assistant interpretation:** Replace the ad-hoc shared-state polling path with an explicit event queue between hardware I/O and the display/UI task, and improve button handling in the same pass.

**Inferred user intent:** Make the dial feel dependable on hardware by decoupling fast, noisy inputs from the LVGL update cadence and by giving button edges a more robust capture path.

**Commit (code):** 2a4ddba608b214d6276e2bbb4bc570179f918092 — "Refactor M5Dial input handling around IO event queue"

### What I did
- Added `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h` with a normalized `InputEvent` payload and explicit event types for encoder delta, short press, long press, and swipe.
- Reworked `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp` into:
  - an I/O task pinned to core `0`
  - a UI/LVGL task pinned to core `1`
  - a FreeRTOS queue connecting them
- Changed `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h` and `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp` so the board layer posts immutable `InputEvent` values instead of exposing multiple `take_*()` latches.
- Added a GPIO ISR for the center button that does only one thing: wake the I/O task with `vTaskNotifyGiveFromISR`. Debounce, long-press timing, and event creation remain in task context.
- Tightened swipe handling so a valid swipe can fire while the finger is still down, instead of waiting only for touch release.
- Updated `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp` to consume the new queue events directly.
- Rebuilt, reflashed, and rechecked the boot path on `/dev/ttyACM0`.

### Why
- LVGL should stay single-owner. A queue makes that ownership explicit and avoids subtle future breakage if more hardware behaviors get added.
- Hardware inputs and UI rendering do not want the same cadence. The queue lets the I/O side run cheaply and frequently, while the UI side drains batched events at its own rate.
- A hybrid button approach is the right trade-off on ESP32: ISR for wakeup/edge capture, task context for debounce and long-press semantics.

### What worked
- Button presses became materially more reliable on the physical device.
- Swipes and encoder steps fit naturally into the same queue model; the controller code became easier to reason about.
- LVGL ownership stayed isolated to the UI task.
- The device still built, flashed, and booted cleanly after the refactor.

### What didn't work
- The previous latch-based input path was technically functional but not good enough on hardware. Short interactions could feel delayed or get lost in the UI loop cadence.
- This step did not solve visual tearing by itself; it only stabilized input delivery.

### What I learned
- The firmware already had the right conceptual split, but not the right boundary. Once every hardware action became a queue event, the code read more like a real embedded UI and less like a demo loop.
- The button did not need fully ISR-driven behavior. It only needed ISR-driven wakeups so the I/O task could sample and debounce promptly.
- Swipes are more responsive when they fire on a decisive in-progress drag rather than only after release.

### What was tricky to build
- The tricky part was resisting the temptation to do too much in the ISR. The user explicitly asked whether the button logic could be ISR-driven, but putting debounce and long-press timing inside the interrupt would have made the design worse, not better. The clean sequence was:
  - switch the button GPIO to `GPIO_INTR_ANYEDGE`
  - install an ISR that only notifies the I/O task
  - keep raw-state sampling, debounce, short-press detection, and long-press detection in `poll()`
  - let the I/O task post a stable `InputEvent` to the queue
- The second tricky part was preserving the LVGL single-owner rule while still making touch and button input feel immediate. The two-task split solved that without introducing shared mutable UI state.

### What warrants a second pair of eyes
- Queue depth is currently `32`, which is reasonable for this small demo but still worth revisiting if more event sources are added later.
- The button debounce and long-press timings (`30 ms` and `700 ms`) are hardware-tuned constants, not formally justified values.
- The project still uses legacy IDF PCNT and I2C APIs because they are the shortest stable path for this board on IDF `5.4.1`.

### What should be done in the future
- Consider migrating the reused `ESP32Encoder` dependency to modern `driver/pulse_cnt.h` if the tutorial becomes a longer-lived reference.
- If touch grows beyond theme swipes, move from a gesture-only board path to a more explicit input-abstraction layer with richer event payloads.
- Add an event-drop counter if queue overflow ever becomes a concern during later feature growth.

### Code review instructions
- Start with the event schema and task wiring:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
- Then review the hybrid ISR/task button path and event posting:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h`
- Then confirm the controller remains a pure consumer of normalized events:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp`
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
source ../.envrc
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
```

### Technical details

Event flow introduced in this step:

```text
encoder / button / touch
          |
          v
      io_task poll()
          |
          v
   FreeRTOS input queue
          |
          v
        ui_task
          |
          v
TimerController -> TimerModel -> TimerScreen -> LVGL
```

Button handling shape introduced in this step:

```text
GPIO edge ISR
  -> notify io_task
io_task wakes
  -> sample GPIO
  -> debounce
  -> detect short/long press
  -> post InputEvent
ui_task drains queue
  -> handle event
```

Commands used during this step:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
```

## Step 7: Reduce visible tearing by shrinking redraw churn and enabling LVGL double buffering

Once the interaction path was stable enough to use comfortably, the remaining hardware quality issue was visual. The UI was working, but the countdown screen was being restyled and relabeled too aggressively, and the LVGL port was still running in the cheapest single-buffer partial-update mode. On a round SPI panel that combination is a reliable way to make redraw seams visible.

This step focused on perception rather than correctness. The goal was not to invent true VSync on hardware that does not expose it here, but to cut the amount of panel work enough that the UI feels materially cleaner during normal use.

### Prompt Context

**User prompt (verbatim):** "working! buttons and swipes. can we avoid tearing on the UI?"

**Assistant interpretation:** Reduce visible redraw artifacts on the M5Dial timer screen by changing the LVGL buffering strategy and by avoiding unnecessary invalidation in the screen code.

**Inferred user intent:** Make the demo look polished on real hardware now that the core interactions are working.

**Commit (code):** a1c6dc32ce10b9341a52c6c33686dd9b30032246 — "Reduce M5Dial timer UI tearing"

### What I did
- Enabled LVGL double buffering in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`.
- Increased the LVGL stripe buffer from `40` lines to `120` lines so the panel sees fewer flush segments per update.
- Added a cached view-state struct in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.h`.
- Reworked `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp` so it only updates:
  - theme-dependent styles when the theme changes
  - state-dependent accent/hint text when the state changes
  - the duration/step lines when the configured duration changes
  - the large time label at `1 s` granularity
  - the progress arc at `100 ms` granularity
- Rebuilt successfully, then reflashed `/dev/ttyACM0`.
- The first flash retry failed because the serial port was busy:

```text
A fatal error occurred: Could not open /dev/ttyACM0, the port is busy or doesn't exist.
([Errno 11] Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable)
```

- Identified the owner with `lsof /dev/ttyACM0`, found a stale `esp_idf_monitor` process, stopped it, and reflashed successfully.

### Why
- Double buffering and larger draw stripes reduce the number of visible partial panel transfers.
- The more important fix is behavioral: if the screen code reissues style and label updates every UI tick, LVGL has no chance to be quiet, and the panel stays busy even when nothing meaningful changed.
- A timer UI does not need `10 ms` label updates. Quantizing the visible state is a user-experience improvement, not a compromise.

### What worked
- The firmware still built cleanly after the redraw optimization.
- Flash completed successfully after the stale monitor was removed.
- The M5Dial now has a rendering path that is materially less wasteful: most screen elements only change when a human could actually perceive a change.
- Binary size remained comfortable: `0x9e5e0` into a `0x400000` app partition.

### What didn't work
- The first flash attempt for this step failed because `/dev/ttyACM0` was locked by another process:

```text
A fatal error occurred: Could not open /dev/ttyACM0, the port is busy or doesn't exist.
([Errno 11] Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable)
```

- A non-TTY `idf.py monitor` attempt still hit the usual wrapper limitation:

```text
Error: Monitor requires standard input to be attached to TTY. Try using a different terminal.
```

- The fallback serial-read attempt with system Python also failed because `pyserial` is not installed there:

```text
ModuleNotFoundError: No module named 'serial'
```

### What I learned
- On this demo, unnecessary invalidation was the easier and more actionable source of tearing than the transport buffer alone.
- The right question was not "how do we draw faster?" but "why are we redrawing this at all?"
- Quantizing visible progress is a good fit for a countdown timer: it reduces work without making the UI feel unresponsive.

### What was tricky to build
- The tricky part was separating "true tearing elimination" from "practical visible improvement." Without a synchronized tear-effect signal path, the realistic goal on this hardware/software stack is to reduce flush frequency and repaint scope enough that artifacts stop being distracting. The implementation sequence was:
  - inspect the LVGL config and confirm the app was still using `double_buffer = false`
  - inspect the screen code and confirm it was restyling and relabeling almost everything every apply cycle
  - enable double buffering and enlarge the draw stripes
  - cache the last visible state so the screen only mutates LVGL objects when something human-visible changed
  - quantize label and arc updates to sensible timer-specific intervals
  - rebuild, reflash, and prepare for user-side visual confirmation
- The other sharp edge was port ownership. Because a stale monitor had the serial device open, the first flash failure looked like an infrastructure hiccup rather than a code issue. That had to be cleared before hardware validation could continue.

### What warrants a second pair of eyes
- The chosen quantization values (`1 s` for the large label, `100 ms` for the arc) are practical defaults, but they are still taste decisions.
- If artifacts remain obvious, the next step may need to move deeper into the LVGL port rather than the screen code, for example experimenting with full-screen buffers or a different flush strategy.
- This step improves visible behavior, but it does not prove the hardware path is truly synchronized to the panel refresh cycle.

### What should be done in the future
- Confirm on-device whether the new redraw behavior is "good enough" or whether the arc should update more slowly or more quickly.
- If tearing is still objectionable, evaluate full-screen double buffering and memory cost explicitly instead of guessing.
- Capture a photo or short video once the visual quality is judged acceptable.

### Code review instructions
- Start with the LVGL configuration change:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
- Then inspect the view-state caching and update quantization:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp`
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
source ../.envrc
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
```

### Technical details

Redraw policy after this step:

```text
theme change      -> restyle palette-dependent objects
state change      -> update accent color, status, hint
duration change   -> update arc range, duration label, step label
countdown label   -> update at 1 second granularity
progress arc      -> update at 100 millisecond granularity
```

Validation commands and outcomes:

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo
idf.py build
# succeeded; binary size 0x9e5e0
idf.py -p /dev/ttyACM0 -b 115200 flash
# first attempt blocked by stale esp_idf_monitor, second attempt succeeded
```
