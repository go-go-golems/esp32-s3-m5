---
Title: Diary
Ticket: M5PS3-EPD-DRAWING
Status: active
Topics:
    - epd
    - display
    - m5gfx
    - m5unified
    - esp-idf
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-30T22:49:07.9477837-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Capture the research trail for how drawing/refresh on the M5PaperS3 EPD works in this codebase: which library is used, how the draw calls flow to the panel driver, and what the underlying refresh/update algorithm does.

## Step 1: Create ticket + orient in repo

This step established the docmgr ticket workspace and did a first-pass scan of the repo for anything EPD/e-ink related. The intent was to anchor all subsequent findings to concrete files and avoid drifting into guesswork about which display stack is actually used.

The key outcome is that the app code draws via `M5Unified` / `M5GFX` (`M5.Display`), not via a bespoke app-level EPD driver. That narrows the investigation to the display library internals, especially board autodetection for `board_M5PaperS3`.

**Commit (code):** N/A

### What I did
- Read `~/.cursor/commands/docmgr.md` and `~/.cursor/commands/diary.md` for ticket + diary rules.
- Created ticket `M5PS3-EPD-DRAWING` and added two docs: a diary (this file) and an analysis doc.
- Grepped this repo for `EPD`, `epdiy`, `M5GFX`, and `setEpdMode`.

### Why
- I need a durable place to store the final analysis and the intermediate "how did we learn this" trail.
- Early broad search prevents missing obvious "it’s actually X" wiring.

### What worked
- Display usage is centralized in `GetHAL().display` (alias of `M5.Display`).
- The repo vendors `components/M5GFX` and `components/M5Unified` (see `repos.json`), so the EPD implementation is likely in-tree, not in ESP-IDF.

### What didn't work
- Searching for an in-repo `epdiy` implementation returned only `M5GFX` docs and conditional compilation stubs; no `epdiy.h` is present in this workspace.

### What I learned
- This app is a “consumer” of the M5 stack; the interesting EPD behavior is almost certainly inside `M5GFX`’s PaperS3 panel implementation.

### What was tricky to build
- Avoiding confusion between “EPDiy (external library)” and “Panel_EPD (in-tree panel driver)”: both exist as concepts in `M5GFX`, but only one is actually used here.

### What warrants a second pair of eyes
- Confirm which panel backend is selected at runtime/compile time for PaperS3 in this repo’s specific `M5GFX` version (it matters because `Panel_EPDiy` is gated by `__has_include(<epdiy.h>)`).

### What should be done in the future
- After running a real build, confirm the compiled-in panel type for PaperS3 (e.g., map symbols / verbose logs / compile output) to eliminate any lingering ambiguity.

### Code review instructions
- Start in `main/hal/hal.h` (the `display` alias) and `main/main.cpp` (where drawing/refresh is orchestrated).
- Then inspect `components/M5GFX/src/M5GFX.cpp` for PaperS3 board detection and panel selection.

### Technical details
- Ticket docs created:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/reference/01-diary.md`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/analysis/01-epd-drawing-pipeline-library-deep-dive.md`

## Step 2: Trace app-level draw/refresh flow (HAL + apps)

This step mapped how the application triggers display updates and how it chooses “EPD modes” (quality/text/fast/fastest). The goal was to understand whether the app does full refreshes, partial refreshes, or relies on the display library’s auto-refresh behavior.

The key outcome is that app code uses normal `M5GFX` drawing APIs (`fillRect`, `drawString`, `drawPng`) and switches behavior primarily by calling `setEpdMode(...)` and by batching via `startWrite()`/`endWrite()`.

**Commit (code):** N/A

### What I did
- Read `main/hal/hal.h` and `main/main.cpp`.
- Scanned all apps in `main/apps/*.cpp` for `GetHAL().display.*` calls and refresh logic.

### Why
- The EPD panel code typically behaves very differently depending on whether updates are explicit (`display()`) or implicit (auto-display on `endWrite()`), and depending on whether updates are partial (rects) or full-screen.

### What worked
- Found the app’s “full background refresh every 15s” in `main/main.cpp` via `check_full_display_refresh_request()` which draws a full-screen PNG background and sets an app-level refresh flag.
- Found repeated partial updates in apps (RTC time/date, WiFi scan state, etc.) switching to `epd_fastest` or `epd_text` as appropriate.
- Found explicit batching in `draw_gray_scale_bars()` using `startWrite()` and `endWrite()`.

### What didn't work
- There is no custom “flush rectangle to EPD” helper in the app; if partial refresh behavior exists, it’s library-side.

### What I learned
- The app-level contract is:
  - Set mode (`setEpdMode(epd_mode_t::...)`).
  - Draw with normal primitives.
  - Optionally batch multiple draws using `startWrite()`/`endWrite()` (important for EPD because `endWrite()` can trigger the actual refresh).

### What was tricky to build
- “Refresh requested” in `Hal` is an app-level boolean, not the same as “EPD hardware refresh”; it just tells apps to redraw their regions after a full background redraw.

### What warrants a second pair of eyes
- Whether `drawPng(...)` or other primitives internally call `startWrite()`/`endWrite()` in ways that might implicitly trigger EPD updates even without explicit batching.

### What should be done in the future
- Add a minimal instrumentation pass (e.g., enable `ESP_LOG*` in `M5GFX` or wrap calls) to verify exactly when `display(...)` is invoked during typical app redraw sequences.

### Code review instructions
- Start in `main/main.cpp` and follow `check_full_display_refresh_request()` and the `startWrite()`/`endWrite()` usage.
- Spot-check a representative partial update in `main/apps/app_rtc.cpp`.

### Technical details
- Key files:
  - `main/hal/hal.h`
  - `main/main.cpp`
  - `main/apps/app_rtc.cpp`

## Step 3: Identify the actual EPD library/backend used by PaperS3

This step focused on which library and which panel backend (driver) actually handles EPD updates for PaperS3. The suspicion going in was “EPDiy”, but the repo already vendors `M5GFX`, so it was important to prove whether this build uses EPDiy at all.

The key outcome is that this codebase uses `M5GFX`’s in-tree PaperS3 driver `lgfx::Panel_EPD` (and `lgfx::Bus_EPD`) rather than EPDiy. `Panel_EPDiy` exists in-tree, but it’s gated by `__has_include(<epdiy.h>)`, and `epdiy.h` is not present here.

**Commit (code):** N/A

### What I did
- Read the PaperS3 initialization path in `components/M5GFX/src/M5GFX.cpp` (board autodetect and panel instantiation).
- Read `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp` / `.hpp` and `Bus_EPD.*`.
- Read `components/M5GFX/src/lgfx/v1/panel/Panel_EPDiy.*` and `components/M5GFX/docs/M5PaperS3.md`.

### Why
- “Which library is used” is the core question: if it’s `Panel_EPD`, then we need to understand its LUT/step-based update system; if it’s EPDiy, we’d need to find the external dependency and study that.

### What worked
- `components/M5GFX/src/M5GFX.cpp` selects `lgfx::Panel_EPD()` for `board_M5PaperS3` and wires it to `Bus_EPD` with explicit pin mapping and panel geometry (960x540).
- `components/M5GFX/docs/M5PaperS3.md` explicitly says: “EPDiy is no longer used from v0.2.7.” This repo uses `M5GFX` branch `0.2.15` (`repos.json`), consistent with “no EPDiy”.

### What didn't work
- Looking for `epdiy` sources locally: there is no `epdiy.h` anywhere in this repo (and I didn’t yet locate an EPDiy checkout under `/home/manuel/esp/esp-5.3.4` either).

### What I learned
- The actual backend in this repo is:
  - Library: `M5GFX` (LovyanGFX-derived).
  - EPD panel driver: `lgfx::Panel_EPD` (`components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`).
  - Hardware bus driver: `lgfx::Bus_EPD` (`components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp`) using ESP-IDF `esp_lcd_i80` APIs.
- EPD mode selection is via `epd_mode_t` (`components/M5GFX/src/lgfx/v1/misc/enum.hpp`) and is passed through `M5.Display.setEpdMode(...)`.

### What was tricky to build
- `Panel_EPD` performs refresh asynchronously in a background FreeRTOS task (`task_update`) fed by a queue from `display(...)`. That means app-visible timing (`waitDisplay`, `displayBusy`) is decoupled from draw calls and depends on queue occupancy and task behavior.

### What warrants a second pair of eyes
- The PSRAM/cache coherency note in `Panel_EPD::init_intenal()` (“PSRAM cache sync needed if task core differs”) is easy to miss and can cause “randomly stale framebuffer” symptoms.

### What should be done in the future
- Verify runtime behavior with a real `idf.py build`/`flash` session and, if needed, add temporary logging around `Panel_EPD::display(...)` and `task_update(...)` to correlate app draws to update queue activity.

### Code review instructions
- Start in `components/M5GFX/src/M5GFX.cpp` around the PaperS3 block (panel + bus wiring).
- Then read `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp` focusing on:
  - `_buf` / `_step_framebuf` roles
  - `display(...)` queueing
  - `task_update(...)` application of LUT steps and scanline output
- Finally read `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp` for the actual I80 bus + scanline handshake.

### Technical details
- The high-level pipeline inferred from code:
  - App draws primitives into `Panel_EPD::_buf` (4bpp packed, 2 pixels/byte, stored in PSRAM).
  - `endWrite()` triggers `display(0,0,0,0)` when `_auto_display == true` (`components/M5GFX/src/lgfx/v1/Panel.hpp`), which queues an `update_data_t` rectangle to `_update_queue_handle`.
  - Background `task_update`:
    - Updates `_step_framebuf` entries for the queued rectangle, encoding per-pixel “which LUT step remains” + target grayscale.
    - Emits scanlines via `Bus_EPD::writeScanLine(...)`, using `blit_dmabuf(...)` and the precomputed `_lut_2pixel` table to map step state to drive waveforms.
    - Powers off the EPD (`Bus_EPD::powerControl(false)`) when no steps remain.

### What I'd do differently next time
- Start by checking `components/M5GFX/docs/M5PaperS3.md` earlier; it short-circuits a lot of “is it EPDiy?” uncertainty.

## Step 4: Build the firmware to confirm compiled-in display backend

This step validated the earlier source-level conclusions by doing a real `idf.py build` and confirming which source files were compiled into the firmware. The intent was to eliminate “maybe the code path isn’t actually used” doubts and confirm the ESP-IDF version/toolchain assumptions.

The key outcome is that the firmware builds successfully with ESP-IDF `v5.3.4`, and the build includes `Panel_EPD.cpp` and `Bus_EPD.cpp` (the in-tree EPD backend). `Panel_EPDiy.cpp` is also compiled, but it is guarded by `__has_include(<epdiy.h>)` and no `epdiy.h` include path appears in the build.

**Commit (code):** N/A

### What I did
- Ran `idf.py build` from the repo root.
- Checked `build/compile_commands.json` to confirm which panel sources compiled and whether `epdiy.h` appeared anywhere in include paths.

### Why
- With embedded stacks, the “source-of-truth” is sometimes build configuration; compiling once gives high confidence about the active backend.

### What worked
- Build completed and produced `build/papers3.elf` and `build/papers3.bin`.
- `build/compile_commands.json` contains compile entries for:
  - `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`
  - `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp`
- No `epdiy.h` mention appears in `build/compile_commands.json`.

### What didn't work
- N/A (build succeeded).

### What I learned
- The EPD backend used here is not “something hidden in ESP-IDF”; it’s compiled from the vendored `components/M5GFX` sources, with ESP-IDF providing the `esp_lcd_i80` infrastructure.

### What was tricky to build
- The build produced a lot of unrelated warnings (ADC deprecation + missing field initializers) which can drown out signal; they’re unrelated to the EPD path, but worth filtering when debugging.

### What warrants a second pair of eyes
- Verify whether `Panel_EPDiy.cpp` being compiled (as an empty unit when `epdiy.h` is missing) has any side effects on binary size or symbol resolution. It probably doesn’t, but it’s a subtle “compiled but inert” case.

### What should be done in the future
- If we need to prove runtime selection, capture serial logs on a real device during `M5.begin()` / board autodetect and confirm it logs `board_M5PaperS3`.

### Code review instructions
- Run `idf.py build` and then inspect `build/compile_commands.json` for `Panel_EPD.cpp` and `epdiy.h`.

### Technical details
- Build artifacts:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/build/papers3.elf`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/build/papers3.bin`

## Step 5: Understand how drawing becomes EPD waveforms (Panel_EPD internals)

This step drilled into how generic draw calls (rect fills, images, strings) are stored in memory and then converted into actual EPD driving waveforms. The purpose was to answer “how does it draw” in concrete terms: buffer formats, dithering, update queueing, and scanline emission.

The key outcome is that `Panel_EPD` is a buffered grayscale panel with an asynchronous update engine: draw calls modify a packed 4bpp framebuffer in PSRAM (`_buf`), then `display(...)` enqueues a rectangle update, and a background task (`task_update`) advances a per-pixel “refresh step” state machine (`_step_framebuf`) while sending scanlines through `Bus_EPD` using a precomputed LUT table (`_lut_2pixel`).

**Commit (code):** N/A

### What I did
- Read the draw-write path in `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`:
  - `writeFillRectPreclipped(...)`
  - `writeImage(...)`
  - `writePixels(...)`
  - `_draw_pixels(...)`
  - `display(...)`
  - `task_update(...)`
- Read the bus path in `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp`.

### Why
- EPD behavior is dominated by how partial updates are accumulated and how the panel is driven (waveforms/LUT steps). Understanding this is necessary for any future performance tuning or bugfixing (ghosting, incomplete refresh, etc.).

### What worked
- Identified framebuffer formats:
  - `_buf`: packed 4bpp grayscale (2 pixels per byte) in PSRAM; drawing writes here.
  - `_step_framebuf`: 16-bit-per-pixel-pair * 2 buffers that encode “target grayscale + LUT step progress” for ongoing refresh sequences.
- Identified dithering differences by mode:
  - In “fast” modes, pixels are thresholded to `0x0` or `0xF` using a Bayer matrix (binary-ish update).
  - In “quality/text”, pixels are quantized to 16 grayscale levels with Bayer dithering.
- Identified update triggering:
  - Modified regions update `_range_mod`.
  - `Panel.hpp`’s `endWrite()` triggers `display(0,0,0,0)` when `_auto_display == true`, which is set in `Panel_EPD`’s constructor.

### What didn't work
- N/A (reading code only).

### What I learned
- `Panel_EPD` is essentially a software compositor + waveform engine:
  - Draw operations are “cheap” memory writes to `_buf`.
  - The expensive part is the periodic scanline emission loop in `task_update`, which keeps running until all pixels in `_step_framebuf` have completed their LUT sequences (`remain == false`).

### What was tricky to build
- There are multiple coordinate spaces in play:
  - Panel coordinates (panel width/height) vs memory coordinates (`memory_width`/`memory_height`) and potential vertical magnification (`magni_h`).
  - Internal rotation (`_internal_rotation`) affects how pixel writes map to `_buf`, and `display(...)` expands rectangles to even pixel boundaries.

### What warrants a second pair of eyes
- Correctness of cache synchronization and cross-core behavior:
  - `Panel_EPD::display(...)` calls `cacheWriteBack(...)` for PSRAM buffer regions, but the comment warns about cache sync when task core differs.
  - Any “stale redraw” or “partial updates not applying” bugs likely live here.

### What should be done in the future
- Document (and possibly test) the invariants for rectangle alignment:
  - `xs/xe` are aligned to even pixels in `display(...)`, while the task consumes `new_data.w >> 1` bytes.
- If ghosting is observed, focus on LUT selection per mode and the “eraser” insertion behavior in `task_update(...)`.

### Code review instructions
- Start in `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`:
  - Read `_draw_pixels(...)` to understand grayscale quantization + packing.
  - Read `display(...)` and `task_update(...)` to understand the async update engine.
- Then read `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp` for the timing/handshake of scanline transfer.

### Technical details
- Draw packing (simplified):
  - Each byte in `_buf` stores two 4-bit pixels: high nibble for even x, low nibble for odd x (depending on rotation path).
- Update data flow:
  - `display(...)` queues `{x,y,w,h,mode}` to `_update_queue_handle` and sets `_display_busy`.
  - `task_update(...)` incorporates queued rectangles into `_step_framebuf` and then repeatedly:
    - `blit_dmabuf(...)` converts step state into scanline bytes using `_lut_2pixel`.
    - `Bus_EPD::writeScanLine(...)` sends scanline via `esp_lcd_panel_io_tx_color(...)`.
    - powers off when no pixels remain in transition.

## Step 6: Deep dive Panel_EPD timing + pin map reconciliation

This step expanded the earlier overview into a code-reading guide for `Panel_EPD.cpp`: what each buffer means, how the LUT stepper (`blit_dmabuf`) advances time, and what the “timing” delays in `Bus_EPD.cpp` correspond to. The goal was to make the file comprehensible without needing to treat it as a black box.

It also reconciled the screenshot pin map naming (DB0..DB7, XSTL, XLE, SPV, CKV, PWR) with M5GFX’s `Bus_EPD` signal names and PaperS3’s pin assignment in `M5GFX.cpp`, calling out the `PWR`/`XOE` vs `pin_pwr` naming mismatch.

**Commit (code):** N/A

### What I did
- Read the readable (non-assembly) implementation of `blit_dmabuf` in `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp` to understand the state machine precisely.
- Read `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.h` and `Bus_EPD.cpp` to understand signal naming and per-scanline handshake.
- Transcribed the screenshot pin mapping into the analysis doc and mapped it against the PaperS3 bus config in `components/M5GFX/src/M5GFX.cpp`.
- Updated the analysis doc with an in-depth “how to read Panel_EPD.cpp” section.

### Why
- The confusing part of e-ink isn’t drawing pixels; it’s the time-domain waveform sequencing. Understanding `blit_dmabuf` and the per-pixel-pair double buffering is the key to reasoning about ghosting, speed, and partial updates.

### What worked
- The C fallback `blit_dmabuf` makes the workflow unambiguous: current slot advances by `+256` per scanout step, swaps in the reserved slot when LUT returns `0`, and uses bit15 (signed negative) to mean “inactive”.
- `Bus_EPD.h` explicitly annotates XSTL/XOE/XLE/XCL naming, which matches the screenshot’s XSTL/XLE and helps explain the wiring.

### What didn't work
- I could not load the attached image via filesystem tools (it isn’t present as a local file in this workspace), so the pin map is captured as a transcribed table instead.

### What I learned
- The “timing” is mostly implemented by repeated scanouts (each scanout is one LUT step), not by long sleeps; the microsecond delays are mostly minimum pulse-width/power sequencing constraints on the control pins.

### What was tricky to build
- The meaning of `0x8000` is easy to misread unless you notice that `blit_dmabuf` loads **even slots** as signed values; negative means “skip”. Updates are scheduled by making the **even slot** non-negative and using the odd slot as a pending/next value.

### What warrants a second pair of eyes
- The pin-map mismatch for “PWR” (screenshot says G45) vs `Bus_EPD` having both `pin_oe` (G45) and `pin_pwr` (G46). This likely reflects different naming conventions across docs vs code; confirming against the panel/module schematic would remove ambiguity.

### What should be done in the future
- If panel-level timing issues occur, get the ED047TC1 datasheet and verify the `beginTransaction()` pulse sequence and `notify_line_done()` latch order against the required waveforms.

### Code review instructions
- Read the new “Panel_EPD.cpp deep dive” section in the analysis doc first, then jump into `Panel_EPD.cpp` and follow along with:
  - `_step_framebuf` layout
  - `blit_dmabuf` C fallback
  - `task_update`
  - `Bus_EPD::writeScanLine` and `notify_line_done`
