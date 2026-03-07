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
    - Path: ../../../../../../../M5Dial-UserDemo/main/hal/hal.cpp
      Note: M5Dial board init evidence gathered during research
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: LVGL port reference used during research
    - Path: 0071-cardputer-adv-photo-timer/main/timer_engine.cpp
      Note: Timer model reference used during research
    - Path: 0072-m5dial-timer-demo/CMakeLists.txt
      Note: Root project wiring for the new M5Dial timer tutorial (commit b39be1f)
    - Path: 0072-m5dial-timer-demo/main/app_main.cpp
      Note: Stage-1 hardware smoke test screen and polling loop (commit b39be1f)
    - Path: 0072-m5dial-timer-demo/main/m5dial_board.cpp
      Note: Initial M5Dial board bring-up and normalized hardware access (commit b39be1f)
    - Path: ttmp/2026/03/06/ESP-26-M5DIAL-TIMER-DEMO--m5dial-timer-demo-tutorial-and-implementation-guide/design-doc/01-m5dial-timer-demo-analysis-design-and-implementation-guide.md
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
