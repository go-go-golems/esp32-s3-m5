---
Title: Investigation diary
Ticket: ESP32-P4-VISUAL-QUICKJS-REPL
Status: active
Topics:
    - esp32p4
    - quickjs
    - javascript
    - firmware
    - lcd
    - repl
    - picocalc
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0099-esp32-p4-picocalc-display-keyboard/main/app_main.c
      Note: LCD rendering evidence that shaped the row-based visual REPL design
    - Path: components/qjs_service/qjs_service.cpp
      Note: QuickJS service ownership evidence that shaped REPL eval integration
    - Path: ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/design-doc/01-visual-quickjs-repl-analysis-design-and-implementation-guide.md
      Note: Primary visual REPL design guide written in Step 1
ExternalSources: []
Summary: Chronological diary for the visual QuickJS REPL on ESP32-P4 PicoCalc LCD ticket.
LastUpdated: 2026-06-24T04:15:00-04:00
WhatFor: Use to resume the ESP32-P4 visual QuickJS REPL design and implementation work.
WhenToUse: Read before implementing 0102, extracting PicoCalc components, or changing the visual REPL architecture.
---


# Diary

## Goal

Capture the investigation, design, implementation, validation, and handoff work for a visual QuickJS REPL on the ESP32-P4 PicoCalc LCD. The target firmware should reuse the previously validated native QuickJS service and PicoCalc LCD/keyboard work to provide scrollback, colored output, and keyboard-driven JavaScript evaluation on the device display.

## Step 1: Create the visual QuickJS REPL ticket and write the intern design guide

Created a new ticket for the visual REPL direction. The design joins two proven pieces of the repository: project 0099's PicoCalc LCD/keyboard bring-up and project 0101's native full-QuickJS service. The guide recommends a new firmware target, `0102-esp32-p4-visual-quickjs-repl`, rather than modifying 0101 in place, so the known-good UART QuickJS console remains available as a regression target.

The primary deliverable is an intern-facing design and implementation guide. It explains the hardware baseline, the native QuickJS service model, the proposed text-cell screen model, scrollback, colored output records, keyboard event handling, rendering boundaries, task ownership, implementation phases, test strategy, and risks.

### Prompt Context

**User prompt (verbatim):** "Let's create a new docmgr ticket to now create a visual REPL using all the LCD work we have been doing previously.

the REPL should have scrollback, colored output, using quickjs. 

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.


Then create detailed phases and tasks in the ticket and then work on them one by one, keeping a detailed diary as you work, committing at appropriate intervals."

**Assistant interpretation:** Create a structured docmgr ticket for a PicoCalc LCD-backed QuickJS REPL, write a detailed intern-facing design guide, upload the initial guide bundle to reMarkable, then implement the project in phases with diary and commits.

**Inferred user intent:** Turn the separate LCD/keyboard and native QuickJS successes into a self-contained device UI where the PicoCalc can be used as a visual JavaScript REPL without depending on a host serial terminal.

### What I did

- Created docmgr ticket `ESP32-P4-VISUAL-QUICKJS-REPL` with topics `esp32p4`, `quickjs`, `javascript`, `firmware`, `lcd`, `repl`, and `picocalc`.
- Added the primary design document: `design-doc/01-visual-quickjs-repl-analysis-design-and-implementation-guide.md`.
- Added this diary document: `reference/01-investigation-diary.md`.
- Replaced the default task file with implementation phases T0 through T8.
- Gathered evidence from:
  - `0099-esp32-p4-picocalc-display-keyboard/README.md` for pin mapping, console, LCD commands, and measured LCD throughput.
  - `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c` for LCD SPI init, SPLL clock source, DMA buffers, row rendering, text benchmarks, and console command registration.
  - `0099-esp32-p4-picocalc-display-keyboard/main/picocalc_keyboard.c` and `.h` for keyboard I2C polling, FIFO status, event structure, and key names.
  - `0101-esp32-p4-native-quickjs/main/app_main.cpp` for the proven native QuickJS service startup configuration.
  - `0101-esp32-p4-native-quickjs/main/js_command.cpp` for eval/status/reset/bench console behavior and timeout choices.
  - `components/qjs_service/include/qjs_service.h` and `components/qjs_service/qjs_service.cpp` for the native QuickJS owner-task API and implementation.
  - `components/quickjs_native/README.md` for the vendored QuickJS source set and ESP-IDF compatibility notes.

### Why

- The project needs a design boundary before implementation because a direct merge of 0099 and 0101 would produce a large app with mixed responsibilities.
- The visual REPL should reuse proven code but should not inherit diagnostic-only structure from 0099 or UART-only assumptions from 0101.
- An intern needs to understand the hardware, rendering, input, QuickJS ownership, and validation sequence before coding.

### What worked

- The local repository already contains strong evidence for both halves of the target system.
- The 0099 LCD code has row-level rendering and transfer benchmarks that directly inform the visual terminal design.
- The 0101 QuickJS service already provides eval, reset, status, deadlines, output capture, and exception formatting, which are exactly the semantics needed by a REPL UI.

### What didn't work

- No implementation has been attempted yet. This step is ticket creation, evidence gathering, design writing, and task planning.

### What I learned

- The first implementation should extract reusable `picocalc_lcd` and `picocalc_keyboard` components before building the visual REPL. The 0099 app is too diagnostic-focused to copy wholesale.
- The first visual UI should be a text-cell model, not an ANSI terminal emulator. The requirement is scrollback and colored output, and a controlled model is easier to validate.
- Row repaint is a better initial renderer granularity than full-screen redraw or per-cell dirty rectangles. With 8×16 cells, one row is 10,240 bytes, which fits under the 32 KiB DMA transfer size already proven in 0099.

### What was tricky to build

- The design had to keep three ownership domains separate: QuickJS runtime ownership, UI model ownership, and LCD transfer ownership. The guide makes `qjs_service` the only path into QuickJS, the UI task the owner of scrollback/input state, and the LCD component the owner of SPI/pixel transfer details.
- The guide also had to avoid overcommitting to product features. Syntax highlighting, multi-line editing, ANSI parsing, and JavaScript display APIs are useful, but they belong after the basic visual eval loop works.

### What warrants a second pair of eyes

- Review the decision to create reusable `components/picocalc_lcd` and `components/picocalc_keyboard` before implementing 0102. This adds upfront refactoring but should reduce long-term duplication.
- Review the proposed row-based renderer and 8×16 cell geometry. It is grounded in 0099 benchmarks, but visual readability depends on the chosen bitmap font.
- Review whether the first UI task may block during `qjs_service_eval` or whether eval completion should be asynchronous from the beginning.

### What should be done in the future

- Complete T0.4 through T0.8: relate files, update changelog, run doctor, upload the initial bundle to reMarkable, and commit the initial ticket docs.
- Start Phase 1 by extracting `picocalc_keyboard` and the minimal `picocalc_lcd` component from 0099.

### Code review instructions

- Start with the design guide at `design-doc/01-visual-quickjs-repl-analysis-design-and-implementation-guide.md`.
- Then read the evidence files in this order:
  1. `0099-esp32-p4-picocalc-display-keyboard/README.md`.
  2. `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`.
  3. `0099-esp32-p4-picocalc-display-keyboard/main/picocalc_keyboard.c` and `.h`.
  4. `components/qjs_service/include/qjs_service.h` and `components/qjs_service/qjs_service.cpp`.
  5. `0101-esp32-p4-native-quickjs/main/app_main.cpp` and `main/js_command.cpp`.
- Validate docs with `docmgr doctor --ticket ESP32-P4-VISUAL-QUICKJS-REPL --stale-after 30` after file relations and changelog are updated.

### Technical details

- Ticket path: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd`.
- Proposed firmware path: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl`.
- Proposed reusable components: `components/picocalc_lcd`, `components/picocalc_keyboard`, and optionally `components/visual_repl`.
- Existing reusable QuickJS components: `components/quickjs_native` and `components/qjs_service`.

## Step 2: Validate initial ticket and upload the design bundle to reMarkable

Validated the initial ticket state and uploaded the design bundle to reMarkable. This creates a readable handoff artifact before implementation begins, so the architecture and task plan can be reviewed independently from code changes.

The uploaded bundle includes the ticket index, the design guide, this diary, the task checklist, and the changelog.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Finish the initial documentation deliverable and publish it before starting source implementation.

**Inferred user intent:** Make the design reviewable on reMarkable and keep implementation work anchored to an accepted ticket plan.

### What I did

- Ran `docmgr doctor --ticket ESP32-P4-VISUAL-QUICKJS-REPL --stale-after 30`.
- Ran `remarquee upload bundle --dry-run ... --non-interactive` for the initial bundle.
- Ran `remarquee upload bundle ... --non-interactive` for the actual upload.
- Marked T0.6 and T0.7 complete.

### Why

- The design guide is a substantial planning artifact and should be available for review before implementation changes start.
- Running doctor before upload catches structural metadata issues while the ticket is still small.

### What worked

- `docmgr doctor` passed with all checks green.
- The dry-run included the intended five documents.
- Upload succeeded to `/ai/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL`.

### What didn't work

- N/A

### What I learned

- The ticket structure is valid after the initial design, diary, task, changelog, and relation updates.

### What was tricky to build

- N/A

### What warrants a second pair of eyes

- Review the initial design before Phase 1 extraction so component boundaries are correct before code moves.

### What should be done in the future

- Commit the initial ticket docs.
- Start Phase 1 by extracting `picocalc_keyboard` and `picocalc_lcd` components from 0099.

### Code review instructions

- Review the uploaded design guide and the task file before approving Phase 1 extraction.
- Validate with `docmgr doctor --ticket ESP32-P4-VISUAL-QUICKJS-REPL --stale-after 30`.

### Technical details

```text
## Doctor Report (1 findings)

### ESP32-P4-VISUAL-QUICKJS-REPL

- ✅ All checks passed
```

```text
OK: uploaded ESP32-P4-VISUAL-QUICKJS-REPL - Visual QuickJS REPL Guide.pdf -> /ai/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL
```

- Completed tasks: T0.6, T0.7.

## Step 3: Extract PicoCalc components, create the 0102 skeleton, and verify hardware smoke

Implemented the first source checkpoint for the visual REPL ticket. I extracted reusable PicoCalc LCD and keyboard components from the 0099 diagnostic firmware, created the new `0102-esp32-p4-visual-quickjs-repl` firmware skeleton, wired it to the existing native QuickJS components, built it for ESP32-P4, flashed it, and validated LCD, keyboard, and QuickJS from the UART debug console.

This is not the visual REPL yet. It is the hardware/runtime skeleton that proves the extracted component boundaries are buildable and that 0102 can initialize all three required subsystems on the real board: LCD, keyboard, and native QuickJS.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Begin implementing the task list after the initial design upload by extracting reusable hardware components and creating the first buildable 0102 firmware.

**Inferred user intent:** Move from design to working firmware in small validated checkpoints, while keeping 0099 and 0101 as stable prior-art targets.

**Commit (code):** e014eb3 — "0102: extract PicoCalc components and add visual REPL skeleton"

### What I did

- Created `components/picocalc_keyboard`:
  - copied the proven 0099 `picocalc_keyboard.c` implementation;
  - moved the public header into `include/picocalc_keyboard.h`;
  - added `CMakeLists.txt` and `README.md`.
- Created `components/picocalc_lcd`:
  - added `include/picocalc_lcd.h`;
  - extracted LCD SPI/panel/fill/blit primitives into `picocalc_lcd.c`;
  - preserved the 0099 pin mapping, `SPI_CLK_SRC_SPLL`, 80 MHz default SCLK, 32 KiB maximum transfer size, RGB565 mode, reset sequence, and minimal panel commands;
  - added `CMakeLists.txt` and `README.md`.
- Created `0102-esp32-p4-visual-quickjs-repl`:
  - added top-level `CMakeLists.txt` with `EXTRA_COMPONENT_DIRS` for `quickjs_native`, `qjs_service`, `picocalc_lcd`, and `picocalc_keyboard`;
  - added `sdkconfig.defaults` with ESP32-P4 UART0 console, 32 MB flash, 200 MHz hex PSRAM, and custom 4 MB app partition;
  - added `partitions.csv`, `README.md`, `main/CMakeLists.txt`, and `main/app_main.cpp`.
- Added a temporary UART debug console with commands:
  - `status`
  - `lcd init`
  - `lcd fill <color>`
  - `kbd [limit]`
  - `js eval <source>` / `js status` / `js reset`
- Built with ESP-IDF 5.4.2.
- Flashed to `/dev/ttyACM0` with `idf.py -p /dev/ttyACM0 flash`.
- Captured monitor output in tmux and killed the monitor session afterward.

### Why

- The visual REPL needs reusable LCD and keyboard components. Keeping those primitives inside 0099's diagnostic `app_main.c` would make 0102 hard to maintain.
- A skeleton that initializes LCD, keyboard, and QuickJS gives a stable base before adding the visual renderer and UI model.

### What worked

- `idf.py build` passes for `0102-esp32-p4-visual-quickjs-repl`.
- The custom 4 MB app partition is active: final binary size is `0xd8900`, leaving `0x327700` bytes (79%) free.
- Flash succeeded on `/dev/ttyACM0`.
- LCD initialized at actual 80 MHz and filled the screen:

```text
I (1581) picocalc_lcd: LCD SPI device ready: clk_src=11 requested=80000000 Hz actual=80000 kHz
I (2161) picocalc_lcd: LCD panel initialized (320x320 RGB565)
0102>  lcd fill blue
lcd fill color=0x001f err=ESP_OK elapsed_ms=21 actual_khz=80000
```

- Keyboard initialized and produced events:

```text
I (2281) picocalc_kbd: initialized PicoCalc keyboard I2C: sda=50 scl=49 speed=10000 addr=0x1f
0102>  kbd 3
kbd: state=1(pressed) key=0xb4(left) valid=1
kbd: state=1(pressed) key=0x31() valid=1
kbd: state=3(released) key=0x31() valid=1
```

- QuickJS service initialized and evaluated JavaScript:

```text
I (2291) qjs_service: runtime init status=ESP_OK elapsed=6 ms
0102>  js eval "print(1+2)"
[eval] ok=1 timed_out=0 elapsed=2ms
3
0102>  js eval "throw new Error('boom')"
[eval] ok=0 timed_out=0 elapsed=0ms
error: Error: boom
```

### What didn't work

- First `idf.py build` used the default 1 MB app partition because `sdkconfig.defaults` did not yet enable the custom partition table. The binary still fit, but only with 15% of the default partition free:

```text
0102-esp32-p4-visual-quickjs-repl.bin binary size 0xd8900 bytes. Smallest app partition is 0x100000 bytes. 0x27700 bytes (15%) free.
```

- Fix: add these defaults and delete the generated `sdkconfig` before rebuilding:

```text
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"
```

- Rebuild then used the intended 4 MB partition.

### What I learned

- The LCD component extraction preserved the 0099 performance baseline: full-screen blue fill still measured about 21 ms at actual 80 MHz.
- The keyboard module is reusable with minimal changes; its public API already had the right component boundary.
- The 0102 skeleton can combine LCD, keyboard, and native QuickJS without immediate component conflicts.

### What was tricky to build

- The LCD extraction needed to keep hardware constants exactly aligned with 0099. The SPLL clock source, max transfer size, and panel init sequence are not incidental; they are part of the validated display path.
- The build initially gave a misleadingly acceptable result with the default partition. The binary fit, but the visual REPL will need room for renderer/font/UI code, so the custom 4 MB partition had to be enabled immediately.

### What warrants a second pair of eyes

- Review `components/picocalc_lcd/picocalc_lcd.c` against the 0099 source to ensure no required panel initialization step was dropped.
- Review the `picocalc_lcd_blit_rect()` API contract: it currently requires the rectangle to fit on screen rather than clipping caller-provided pixels.
- Review whether the temporary UART debug console should stay through all bring-up phases or be compiled behind a config flag later.

### What should be done in the future

- Start Phase 3: add the text renderer and static colored visual screen.
- Replace the temporary UART-only debug behavior with the LCD model/rendering path once visual rows are available.

### Code review instructions

- Start with `components/picocalc_lcd/include/picocalc_lcd.h` and `components/picocalc_lcd/picocalc_lcd.c`.
- Then review `components/picocalc_keyboard` to confirm it remains a faithful extraction from 0099.
- Review `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` for startup order and temporary debug commands.
- Validate with `cd 0102-esp32-p4-visual-quickjs-repl && source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build`, then flash/monitor on `/dev/ttyACM0`.

### Technical details

- Build command: `idf.py build` after sourcing ESP-IDF 5.4.2.
- Flash command: `idf.py -p /dev/ttyACM0 flash`.
- Monitor command: `idf.py -p /dev/ttyACM0 monitor` inside tmux session `qjs0102`.
- Completed tasks: T1.1, T1.2, T1.3, T1.4, T1.5, T1.6, T2.1, T2.2, T2.3, T2.4, T2.5, T2.6.
