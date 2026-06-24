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

## Step 4: Add the first visual terminal renderer and static demo screen

Added the first `visual_repl` component and wired it into 0102. This checkpoint turns the LCD from a fill-only diagnostic into a fixed-cell visual terminal surface: 40 columns by 20 rows, 8×16 pixels per cell, semantic row styles, a simple built-in bitmap font, a prompt row, and a static demo screen that exercises system, status, prompt, output, error, and input styles.

This still is not the interactive REPL. It is the renderer checkpoint that proves the firmware can construct terminal rows, convert them into RGB565 row buffers, and repaint the complete 320×320 viewport through the reusable LCD component.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Continue implementing the visual REPL phases after the 0102 skeleton by adding the first visible terminal model and renderer.

**Inferred user intent:** Make forward progress toward the on-device LCD REPL with small buildable and flashable checkpoints, while recording failures and measurements.

**Commit (code):** ae74cb2 — "0102: add visual REPL terminal renderer"

### What I did

- Added `components/visual_repl`:
  - `include/visual_repl.h`
  - `visual_repl.cpp`
  - `CMakeLists.txt`
  - `README.md`
- Implemented constants for the first terminal geometry:
  - 320×320 LCD target;
  - 8×16 pixel cells;
  - 40 columns × 20 rows;
  - 19 scrollback/output rows plus one input row.
- Implemented semantic row styles:
  - `system`
  - `prompt`
  - `input`
  - `output`
  - `error`
  - `status`
- Added a compact 5×7 bitmap glyph set scaled into 8×16 cells.
- Implemented row rendering into a static RGB565 row buffer sized for one 320×16 row.
- Added `visual_repl_demo_screen()` with representative system, status, prompt, output, error, and input rows.
- Wired `visual_repl` into `0102-esp32-p4-visual-quickjs-repl` via `EXTRA_COMPONENT_DIRS` and `main/CMakeLists.txt`.
- Added startup demo rendering after LCD initialization.
- Added UART debug command `screen demo`.
- Extended `status` output with visual renderer status.
- Built, flashed, and monitored 0102 on `/dev/ttyACM0`.

### Why

- The visual REPL needs a model/renderer boundary before keyboard editing and QuickJS output can be shown on screen.
- A row renderer is enough for the first terminal checkpoint and maps cleanly to the existing `picocalc_lcd_blit_row()` primitive.
- Using semantic styles now prevents output from becoming unstructured strings that are hard to recolor later.

### What worked

- The 0102 build passes with the new component.
- Binary size remains comfortable with the 4 MB app partition:

```text
0102-esp32-p4-visual-quickjs-repl.bin binary size 0xd9d40 bytes. Smallest app partition is 0x400000 bytes. 0x3262c0 bytes (79%) free.
```

- Flash succeeded on `/dev/ttyACM0`.
- Boot-time visual demo initialized and rendered:

```text
I (2162) visual_repl: visual REPL model initialized: 40x20 cells (8x16 pixels)
I (2192) 0102: visual demo render: ESP_OK
```

- UART `status` shows the visual model and render measurement:

```text
visual: initialized=1 grid=40x20 cell=8x16 history=7 renders=1 last_render_ms=31
```

- UART `screen demo` repaints the full viewport successfully:

```text
0102>  screen demo
I (5592) visual_repl: visual REPL model initialized: 40x20 cells (8x16 pixels)
screen demo: ESP_OK elapsed_ms=32 render_ms=31 grid=40x20
```

### What didn't work

- First renderer build failed because `snprintf(prompt_line, sizeof(prompt_line), "> %s", s_input)` could truncate a 160-byte input buffer into the 41-byte screen row. ESP-IDF treats this warning as an error:

```text
error: '%s' directive output may be truncated writing up to 160 bytes into a region of size 39 [-Werror=format-truncation=]
  242 |     std::snprintf(prompt_line, sizeof(prompt_line), "> %s", s_input);
```

- Fix: replace the formatted write with explicit bounded copying into the 40-column prompt row.

### What I learned

- Full 40×20 repaint via 20 row blits currently takes about 31 ms on the validated 80 MHz SPI LCD path.
- The component can keep a full one-row RGB565 buffer in static internal memory without pushing large framebuffers through the heap.
- The first font and row-style model are adequate for bring-up, but mixed spans will be needed once prompts, user input, and output share physical rows.

### What was tricky to build

- The prompt row has two length domains: the logical input buffer is longer than one screen row, but the renderer must emit exactly one 40-column physical row. The compiler caught the unsafe truncation path, and the fix was to make clipping explicit.
- The current style model is intentionally one style per physical row. This keeps the first renderer simple, but it means a future span model is needed for richer prompt/input styling.

### What warrants a second pair of eyes

- Review `components/visual_repl/visual_repl.cpp` for off-by-one errors in the scrollback ring and visible-row selection.
- Review whether the 5×7 scaled font is readable enough on the PicoCalc LCD. The serial log proves rendering completed, but not subjective readability.
- Review the use of a static row buffer in internal RAM. It is small enough for this checkpoint, but future dirty-region buffering should avoid hidden growth.

### What should be done in the future

- Perform a human/camera visual readability check and mark T3.6 only after confirming the LCD text and colors are legible.
- Start Phase 4: keyboard polling task, key translation, input buffer editing, and dirty input-row rendering.
- Evolve row styles into spans before adding syntax highlighting or mixed prompt/input/output rendering.

### Code review instructions

- Start with `components/visual_repl/include/visual_repl.h` for geometry, styles, and public API.
- Then review `components/visual_repl/visual_repl.cpp`, especially `visual_repl_render()`, `render_text_row()`, and prompt-row clipping.
- Review `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` for the `screen demo` command and startup demo call.
- Validate with:
  - `cd 0102-esp32-p4-visual-quickjs-repl && source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build`
  - `idf.py -p /dev/ttyACM0 flash`
  - `idf.py -p /dev/ttyACM0 monitor`
  - `status`
  - `screen demo`

### Technical details

- Full viewport redraw: ~31 ms.
- `screen demo` command elapsed time: ~32 ms.
- App binary after renderer: `0xd9d40` bytes.
- Free space in 4 MB app partition: `0x3262c0` bytes (79%).
- Completed tasks: T3.1, T3.2, T3.3, T3.4, T3.5, T3.7.
- Remaining Phase 3 task: T3.6 hardware visual readability check.

## Step 5: Fix clipped glyph geometry and add the first keyboard editor loop

Continued Phase 4 and corrected a visual renderer defect reported from the real LCD. The original bitmap renderer scaled each 5×7 glyph by 2× horizontally and vertically, which made glyphs 10 pixels wide inside an 8-pixel cell. That made right edges disappear on the PicoCalc screen. I added a host-side SVG preview tool to compare the bad and corrected geometry, then changed the firmware renderer to use 1× horizontal and 2× vertical scaling centered inside each 8×16 cell.

I also implemented the first keyboard editor loop for the visual REPL: a background keyboard task, printable insertion, Backspace/Delete, Left/Right/Home/End, Escape-to-clear, Enter-to-append-without-eval, and input-row-only repainting. The firmware builds and flashes, but the current hardware keyboard smoke is blocked by repeated keyboard I2C `ESP_ERR_INVALID_STATE` after flashing; the earlier skeleton validated the same keyboard component, so the next step is a PicoCalc keyboard/southbridge power-cycle or reset check rather than more blind firmware retries.

### Prompt Context

**User prompt (verbatim):** "the font renderering is a bit broken, it looks like letters are cut off from the right. Feel free to do host-site experiments for the renderer to be able to iterate faster"

**Assistant interpretation:** Diagnose the LCD font clipping using faster host-side iteration, then patch the firmware renderer instead of repeatedly flashing for each visual experiment.

**Inferred user intent:** Improve the visual readability of the LCD REPL and keep development efficient by separating renderer geometry experiments from device flashing.

**Commit (code):** pending — "0102: add keyboard editor and fix visual font geometry"

### What I did

- Added host-side renderer experiment:
  - `components/visual_repl/tools/render_preview.py`
  - generated `components/visual_repl/tools/render_preview.svg`
- Used the host preview to compare:
  - old geometry: 5×7 glyphs scaled 2×2, producing 10-pixel-wide glyphs in 8-pixel cells;
  - new geometry: 5×7 glyphs scaled 1×2, producing 5-pixel-wide glyphs centered in 8-pixel cells.
- Patched `components/visual_repl/visual_repl.cpp`:
  - replaced a single `scale = 2` with separate `x_scale = 1` and `y_scale = 2`;
  - centered the glyph inside the 8×16 cell.
- Extended `components/visual_repl`:
  - added `visual_repl_render_input()` so keypresses can repaint only the prompt row.
- Implemented Phase 4 editor code in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`:
  - background `keyboard_task`;
  - input buffer and cursor;
  - printable insertion;
  - Backspace/Delete;
  - Left/Right/Home/End;
  - Escape clears input;
  - Enter appends `> ...` to scrollback and adds a status row indicating QuickJS eval is Phase 5.
- Added keyboard polling backoff and suppressed noisy `i2c.master` driver logs, while keeping rate-limited application warnings.
- Built and flashed the firmware several times on `/dev/ttyACM0` using single-owner tmux/serial handling.

### Why

- A 10-pixel-wide glyph cannot fit in an 8-pixel-wide cell. This was a deterministic renderer geometry bug, not an LCD timing issue.
- Input-row-only rendering is necessary for keyboard editing. Full 40×20 redraws for every keypress are unnecessary and would make cursor movement feel sluggish.
- The keyboard task needs backoff because a non-acknowledging keyboard controller can otherwise flood the UART and obscure useful logs.

### What worked

- Host-side renderer preview generated successfully without requiring Pillow or other dependencies. It emits SVG directly.
- The corrected firmware builds:

```text
0102-esp32-p4-visual-quickjs-repl.bin binary size 0xda660 bytes. Smallest app partition is 0x400000 bytes. 0x3259a0 bytes (79%) free.
```

- Flash succeeded on `/dev/ttyACM0`.
- Boot still reaches all major services:

```text
I (2163) visual_repl: visual REPL model initialized: 40x20 cells (8x16 pixels)
I (2193) 0102: visual initial render: ESP_OK
I (2293) picocalc_kbd: initialized PicoCalc keyboard I2C: sda=50 scl=49 speed=10000 addr=0x1f
I (2293) 0102: keyboard init: ESP_OK
I (2293) 0102: visual keyboard editor task started
I (2293) 0102: keyboard editor task create: ok
I (2313) qjs_service: runtime init status=ESP_OK elapsed=6 ms
```

- UART is now readable even when keyboard polling fails, because the low-level I2C driver spam is suppressed and the app logs only rate-limited warnings.

### What didn't work

- The keyboard smoke is currently blocked. The background task and the explicit `kbd 1` UART command both see keyboard I2C failures after the latest flashes:

```text
W (3293) 0102: keyboard poll failed: ESP_ERR_INVALID_STATE consecutive_errors=1
0102>  kbd 1
kbd: err=ESP_ERR_INVALID_STATE
Command returned non-zero error code: 0x1 (ERROR)
```

- The same keyboard component worked earlier in the 0102 skeleton smoke (`kbd 3` returned valid events), so this is not yet enough evidence to redesign the keyboard component. The most likely next check is to power-cycle or reset the PicoCalc keyboard/southbridge side, because ESP32 flashing only resets the ESP32-P4 and may not reset the keyboard controller.

### What I learned

- The original visual clipping was caused by exact renderer math: `5 glyph columns × 2 horizontal scale = 10 pixels`, while the terminal cell is only 8 pixels wide.
- The fixed 40-column layout and the current 5×7 font require anisotropic scaling: narrow horizontally, tall vertically.
- Firmware-side keyboard code is now structurally in place, but hardware validation needs a clean keyboard-controller state.

### What was tricky to build

- The screen geometry has competing constraints: 40 columns require 8-pixel cells on a 320-pixel-wide LCD, but common 5×7 fonts do not support 2× horizontal scaling in that cell size. Keeping 40 columns means accepting a narrower glyph or switching to a different 4-pixel-wide font design later.
- The keyboard failure mode is noisy because the ESP-IDF I2C master logs before returning the error. Suppressing the driver tag is acceptable for this interactive bring-up firmware because the app still tracks error counts and reports status.

### What warrants a second pair of eyes

- Confirm on the actual LCD that the corrected 1×2 glyphs are no longer clipped and remain readable.
- Review whether 40 columns is worth the narrower font. If readability is too poor, consider a 32-column mode with 10-pixel cells or a custom 4×7 font scaled 2× horizontally.
- Review whether suppressing `i2c.master` logs should be permanent or converted into a debug flag after keyboard bring-up stabilizes.

### What should be done in the future

- Ask the operator to power-cycle/reset the PicoCalc device or keyboard controller and retry the Phase 4 input smoke.
- If keyboard I2C failures persist after a full power-cycle, add a lower-level I2C recovery path or inspect whether another task/command is racing the I2C master.
- Add a screenshot/photo-based visual confirmation step for T3.6 if subjective readability remains uncertain.

### Code review instructions

- Review `components/visual_repl/tools/render_preview.py` first to understand the renderer geometry experiment.
- Review `components/visual_repl/visual_repl.cpp`, especially `draw_cell()` and `visual_repl_render_input()`.
- Review `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`, especially `keyboard_task()`, `handle_editor_key()`, `submit_input_line()`, and the `i2c.master` log-level suppression.
- Validate build with:
  - `cd 0102-esp32-p4-visual-quickjs-repl && source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build`
- Validate hardware after keyboard controller reset/power-cycle with:
  - `idf.py -p /dev/ttyACM0 flash`
  - `idf.py -p /dev/ttyACM0 monitor`
  - type `abc`, Left, `X`, Enter on the physical PicoCalc keyboard.

### Technical details

- Host preview output: `components/visual_repl/tools/render_preview.svg`.
- Font fix: `x_scale=1`, `y_scale=2`, centered in `VISUAL_REPL_CELL_W=8`, `VISUAL_REPL_CELL_H=16`.
- Latest build size: `0xda660` bytes, 79% free in the 4 MB app partition.
- Completed tasks: T4.1, T4.2, T4.3, T4.4, T4.5, T4.7.
- Still open: T3.6 visual readability confirmation and T4.6 hardware input smoke.
