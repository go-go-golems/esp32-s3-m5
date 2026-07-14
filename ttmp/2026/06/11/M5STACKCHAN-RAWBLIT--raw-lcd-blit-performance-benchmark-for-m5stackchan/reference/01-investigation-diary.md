---
Title: Investigation diary
Ticket: M5STACKCHAN-RAWBLIT
Status: active
Topics:
    - benchmark
    - display
    - esp32
    - performance
    - m5stackchan
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../../../tmp/stackchan-rawblit-40-monitor.log
      Note: 40 MHz raw blit serial measurements
    - Path: ../../../../../../../../../../../../tmp/stackchan-rawblit-60-monitor.log
      Note: 60 MHz requested raw blit serial measurements
    - Path: ../../../../../../../../../../../../tmp/stackchan-rawblit-80-monitor.log
      Note: 80 MHz raw blit serial measurements
    - Path: build/firmware/main/bench/raw_blit_benchmark_main.cpp
      Note: Implemented and measured raw LCD blit benchmark
    - Path: build/firmware/main/hal/board/stackchan.cc
      Note: Source evidence for raw blit display path
    - Path: build/firmware/main/hal/board/stackchan_display.cc
      Note: Source evidence for LVGL/display buffer interaction
ExternalSources: []
Summary: Chronological diary for the raw LCD blit performance benchmark ticket.
LastUpdated: 2026-06-11T20:50:48.513620393-04:00
WhatFor: Use to resume the raw blit benchmark implementation and understand what was tried, what failed, and what measurements were captured.
WhenToUse: Before continuing raw display throughput experiments or writing follow-up reports.
---



# Diary

## Goal

Measure raw LCD blit throughput on the M5StackChan/CoreS3 display path. The benchmark should quantify full-screen and partial-region RGB565 transfer performance through `esp_lcd_panel_draw_bitmap()`, then explore whether SPI/display parameters such as chunk height, queue depth, buffer placement, and pixel clock can improve throughput.

## Step 1: Ticket Setup, Source Evidence, and Initial Design Guide

I created a new docmgr ticket for raw LCD blit benchmarking and wrote the initial implementation guide. The guide is deliberately separate from the previous standalone LVGL benchmark because it answers a different question: not "how expensive is a small LVGL object update?" but "how fast can the display path move pixels when we bypass LVGL scene rendering?"

The design keeps the earlier benchmark's lessons: start with a stable baseline, avoid measuring benchmark-induced failures, keep metrics storage off small task stacks, and print structured serial summaries. It adds a new focus on raw `esp_lcd` panel transfers, full-screen versus partial-rectangle throughput, and safe SPI parameter experiments.

### Prompt Context

**User prompt (verbatim):** "THen create a new ticket to measure the raw blit performance, full screen and smaller blits, and tyring to optimize the SPI transfer and other parameters to see how far things can go.

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.


Then implement and measure and do all that too."

**Assistant interpretation:** Create a new ticket, write an intern-ready design/implementation guide for raw LCD blit benchmarking, upload it to reMarkable, then implement, build, flash, measure, and document the raw blit benchmark.

**Inferred user intent:** Move beyond estimated full-screen FPS and collect real hardware measurements for raw display transfer throughput and possible SPI/display optimizations.

### What I did

- Created ticket `M5STACKCHAN-RAWBLIT`.
- Added design document:
  - `design-doc/01-raw-blit-performance-analysis-design-and-implementation-guide.md`
- Added this diary:
  - `reference/01-investigation-diary.md`
- Added implementation tasks for evidence gathering, guide writing, upload, implementation, build/flash, serial measurement, and results documentation.
- Gathered source evidence from:
  - `main/hal/board/config.h` — display size and orientation.
  - `main/hal/board/stackchan.cc` — SPI3 and ILI9342/ILI9341-compatible panel setup.
  - `main/hal/board/stackchan_display.cc` — LVGL display buffer, RGB565 format, DMA buffer, LVGL port setup.
  - `main/bench/benchmark_main.cpp` — working standalone benchmark pattern and metric recorder lessons.
  - ESP-IDF `esp_lcd` headers — raw panel draw and panel IO completion callback APIs.
- Wrote the guide with:
  - problem statement and scope;
  - display architecture;
  - raw blit mental model;
  - benchmark matrix for full-screen and smaller rectangles;
  - chunk-height experiments;
  - buffer-mode experiments;
  - SPI pixel-clock experiments;
  - pseudocode for `run_case()`;
  - risks and validation plan;
  - interpretation rules.

### Why

The previous benchmark measured small LVGL object updates. It did not measure full-screen RGB565 transfer throughput. The raw blit benchmark is needed because the display bus has an estimated full-screen ceiling of about 32.5 FPS at 40 MHz SPI, but that estimate excludes driver overhead, command overhead, DMA queueing, and scheduling.

### What worked

- The source evidence clearly identifies the display path:
  - `SPI3_HOST`
  - MOSI GPIO37
  - SCLK GPIO36
  - CS GPIO3
  - DC GPIO35
  - SPI mode 2
  - 40 MHz pixel clock
  - 8-bit commands and parameters
  - RGB565/BGR pixel format
  - ILI9341-compatible driver for the ILI9342 display
- The current HAL bridge exposes the LVGL display handle, but not raw panel handles.
- The design therefore includes a narrow accessor plan through `StackChanAvatarDisplay` and `hal_bridge`.

### What didn't work

- No code has been built or flashed yet for this ticket.
- The raw panel handle is not currently exposed, so implementation will require a small HAL/display accessor change.

### What I learned

- Raw blit benchmarking needs to separate fill time, submit/block time, and transfer-completion time.
- `esp_lcd_panel_draw_bitmap()` may queue asynchronous SPI transactions, so a single call's duration may not equal physical transfer completion.
- `esp_lcd_panel_io_register_event_callbacks()` can provide color transfer completion callbacks, but using it after `esp_lvgl_port` setup may overwrite LVGL's callback. The raw benchmark should avoid LVGL updates after installing its callback, or fall back to batch timing.
- Full-screen and partial-region measurements must always print byte counts; FPS alone is misleading without knowing the transferred pixel area.

### What was tricky to build

The main design challenge is deciding how to access the initialized display. Reinitializing the panel from scratch would duplicate board bring-up and risk measuring a different configuration. Reusing `GetHAL().init()` preserves fidelity, but the raw `esp_lcd_panel_handle_t` and `esp_lcd_panel_io_handle_t` are private inside `StackChanAvatarDisplay`. The planned compromise is to add narrow getters and bridge functions for benchmark use.

### What warrants a second pair of eyes

- Whether callback-based completion timing can safely coexist with the LVGL port after `GetHAL().init()`.
- Whether the raw benchmark should disable or remove the LVGL display from `esp_lvgl_port` before taking over the panel IO callback.
- Whether SPI clocks above 40 MHz are electrically safe for this panel/cable/board combination.
- Whether benchmark-only raw display accessors should be wrapped in Kconfig guards.

### What should be done in the future

- Upload the initial guide and diary bundle to reMarkable.
- Implement `CONFIG_STACKCHAN_RAW_BLIT_BENCHMARK` and `bench/raw_blit_benchmark_main.cpp`.
- Add raw panel/io accessors.
- Build and flash the raw benchmark.
- Capture serial `RAWBLIT_SUMMARY` output for full-screen and partial regions.
- Update this diary and the guide with measured results.

### Code review instructions

Start with the guide:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/11/M5STACKCHAN-RAWBLIT--raw-lcd-blit-performance-benchmark-for-m5stackchan/design-doc/01-raw-blit-performance-analysis-design-and-implementation-guide.md
```

Then review these source files:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/config.h
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/stackchan.cc
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/stackchan_display.cc
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/benchmark_main.cpp
```

### Technical details

The estimated theoretical full-screen ceiling remains:

```text
320 × 240 × 2 bytes = 153,600 bytes/frame
40 MHz SPI = 5,000,000 bytes/s
5,000,000 / 153,600 ≈ 32.55 FPS
```

This ticket's purpose is to measure how close the real firmware stack can get to that number and how throughput changes as the invalidated rectangle gets smaller.

## Step 2: Raw Blit Benchmark Implementation and Hardware Measurements

This step implemented the raw LCD blit benchmark and captured the first full-screen and partial-region measurements on real hardware. The benchmark now bypasses LVGL scene rendering and sends RGB565 rectangles directly through the initialized `esp_lcd` panel handle. It also makes the LCD SPI pixel clock configurable so 40 MHz, 60 MHz, and 80 MHz builds can be compared.

The main result is that 80 MHz requested SPI clock substantially improves full-screen raw blit throughput, while 60 MHz behaves like the 40 MHz baseline. The best measured full-screen case so far is 36.42 FPS for 320×240 generated-pattern blits with 120-line chunks at 80 MHz.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Continue from the guide into implementation, build/flash, measurement, and documentation.

**Inferred user intent:** Produce real raw blit data, not just a design, and explore whether SPI/display parameters can improve throughput.

### What I did

- Added `CONFIG_STACKCHAN_RAW_BLIT_BENCHMARK` to `main/Kconfig.projbuild`.
- Added `CONFIG_STACKCHAN_LCD_PIXEL_CLOCK_HZ` to make display SPI pclk configurable.
- Updated `main/CMakeLists.txt` to select `bench/raw_blit_benchmark_main.cpp` when raw blit benchmark is enabled.
- Added narrow raw panel accessors:
  - `StackChanAvatarDisplay::GetPanelHandle()`
  - `StackChanAvatarDisplay::GetPanelIoHandle()`
  - `hal_bridge::display_get_panel_handle()`
  - `hal_bridge::display_get_panel_io_handle()`
- Updated `stackchan.cc` so `io_config.pclk_hz` uses `CONFIG_STACKCHAN_LCD_PIXEL_CLOCK_HZ`.
- Created `main/bench/raw_blit_benchmark_main.cpp`.
- Enabled the raw benchmark in `sdkconfig.defaults.local`.
- Built and flashed the raw benchmark.
- Captured serial logs for:
  - 40 MHz: `/tmp/stackchan-rawblit-40-monitor.log`
  - 60 MHz: `/tmp/stackchan-rawblit-60-monitor.log`
  - 80 MHz: `/tmp/stackchan-rawblit-80-monitor.log`

### Why

The previous benchmark measured small LVGL object updates. This benchmark measures raw pixel movement. It is the only way to tell whether full-screen animation is limited by the LCD/SPI path or by higher-level LVGL/Mooncake/launcher behavior.

### What worked

- The raw blit benchmark builds successfully.
- The binary remains small relative to the OTA partition:

```text
stack-chan.bin binary size 0x25a6f0 bytes. Smallest app partition is 0x4f0000 bytes. 0x295910 bytes (52%) free.
```

- The benchmark boots and emits `RAWBLIT_SUMMARY` lines.
- 40 MHz, 60 MHz, and 80 MHz requested clock builds all ran without serial panic/WDT/assert errors after the printf fix.
- 80 MHz requested clock produced a clear throughput improvement.

### What didn't work

The first raw benchmark monitor run crashed while printing the first summary:

```text
RAWBLIT_SUMMARY case=full_320x240_chunk80 ... elapsed_us=lu fps_x100Guru Meditation Error: Core  0 panic'ed (LoadProhibited). Exception was unhandled.
```

Cause: `%llu` formatting in the summary `printf` path. Fix: clamp elapsed time to `uint32_t` and print with `%lu`.

Also, the full-frame 240-line DMA buffer allocation failed on every run:

```text
RAWBLIT_ALLOC_FAIL case=full_320x240_chunk240 w=320 h=240 chunk_h=240 bytes_per_chunk=153600 heap_internal_free=231383
```

The heap had enough total free internal memory but not a contiguous DMA-capable block of 153,600 bytes after HAL initialization.

### What I learned

- At 40 MHz, the best generated-pattern full-screen case was 25.00 FPS using 120-line chunks.
- At 80 MHz, the best generated-pattern full-screen case was 36.42 FPS using 120-line chunks.
- 60 MHz requested pclk did not improve timings over 40 MHz, likely because the SPI clock divider quantizes it to an effective 40 MHz path.
- 20-line chunks, matching the current LVGL draw-buffer height, are much slower for full-screen raw blits than 80-line or 120-line chunks.
- Small rectangles have high FPS but low effective MB/s because command/transaction overhead dominates.

### What was tricky to build

The benchmark had to reuse the initialized display without reimplementing board bring-up. That required exposing raw panel handles from the display object. It also had to take over the `on_color_trans_done` callback from the LVGL port for completion timing. The benchmark avoids further LVGL object updates after taking over the callback.

The printf crash was another reminder that benchmark reporting code is part of the embedded system. The measurement can succeed and the firmware can still fail while formatting the result.

### What warrants a second pair of eyes

- Confirm 80 MHz visual quality on the physical display; serial logs show stability but do not prove absence of subtle corruption.
- Confirm actual SCLK frequencies with a logic analyzer, especially the 60 MHz requested case.
- Review whether overwriting the LVGL panel IO callback is acceptable for a benchmark-only entry point.
- Consider whether a larger LVGL buffer can be allocated safely and whether it improves production large-region animation.

### What should be done in the future

- Add a chart renderer for raw blit results.
- Add a production LVGL full-refresh benchmark with larger display buffers.
- Try allocating larger buffers earlier in boot or in PSRAM with DMA/bounce behavior documented.
- Test 80 MHz visually and decide whether it is safe enough for production experiments.
- Instrument production launcher invalidated area and compare it to raw partial-region results.

### Code review instructions

Review changed firmware files:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/Kconfig.projbuild
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/CMakeLists.txt
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/stackchan_display.h
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/hal_bridge.h
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/hal_bridge.cc
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/stackchan.cc
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/raw_blit_benchmark_main.cpp
```

Validation logs:

```text
/tmp/stackchan-rawblit-40-monitor.log
/tmp/stackchan-rawblit-60-monitor.log
/tmp/stackchan-rawblit-80-monitor.log
```

### Technical details

Best measured full-screen cases:

```text
40 MHz: full_320x240_chunk120 generated, 25.00 FPS, 3.84 MB/s
80 MHz: full_320x240_chunk120 generated, 36.42 FPS, 5.59 MB/s
```

The current local config after the final run is:

```text
CONFIG_STACKCHAN_STANDALONE_BENCHMARK=n
CONFIG_STACKCHAN_RAW_BLIT_BENCHMARK=y
CONFIG_STACKCHAN_LCD_PIXEL_CLOCK_HZ=80000000
```

## Step 3: Visual Validation Exposed Raw RGB565 Byte-Order Problem

After the first 40/60/80 MHz raw blit measurements, the user inspected the physical display and reported a suspicious visual pattern. The serial data showed stable benchmark completion at 80 MHz, but the screen showed rainbow-colored diagonal patterns. The user could still see the intended rectangle structure, which is an important distinction: the display address window and rectangle geometry were working, but the pixel payload looked wrong or at least too ambiguous to validate.

I inspected the user-provided blurry webcam photo at `/home/manuel/Downloads/webcam.jpg`. The image showed a correctly placed large rectangular display area filled with dense diagonal/rainbow texture. That matches two facts at once: the benchmark's generated pattern intentionally varies color with `x`, `y`, and `frame`, but the raw RGB565 path was also likely missing the byte swap that the production LVGL path applies.

### Prompt Context

**User prompt (verbatim):** "record this in detail in your diary, and then fix and continue. I'm pretty sure the 80mhz are fine though."

**Assistant interpretation:** Preserve the visual-validation finding in the ticket diary, then fix the raw benchmark's pixel-format/byte-order issue and continue the measurement workflow. Treat 80 MHz as likely electrically stable unless further evidence says otherwise.

**Inferred user intent:** Make the raw blit benchmark visually trustworthy, not just serial-log stable, so the measured 80 MHz throughput can be interpreted with confidence.

### What I did

- Read the user's photo:

```text
/home/manuel/Downloads/webcam.jpg
```

- Observed that the rectangle region appeared correctly positioned and sized.
- Observed that the colors appeared as dense rainbow/diagonal texture.
- Compared the raw benchmark pixel generation with the production LVGL display path.
- Found the key source evidence:

```cpp
// stackchan_display.cc
.flags = {
    .swap_bytes = 1,
}
```

- Confirmed the raw benchmark currently writes host-endian `uint16_t` RGB565 values directly to the buffer and calls:

```cpp
esp_lcd_panel_draw_bitmap(panel, x0, y0, x1, y1, buffer)
```

- Confirmed the ILI9341-compatible panel driver forwards `color_data` bytes as-is through `esp_lcd_panel_io_tx_color()`:

```cpp
size_t len = (x_end - x_start) * (y_end - y_start) * ili9341->fb_bits_per_pixel / 8;
esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);
```

### Why

Serial logs only prove that the benchmark did not crash and that the code path completed. They do not prove that the displayed pixels are semantically correct. A raw blit benchmark must visually validate byte order and color order before its throughput results can be used as a display-performance reference.

### What worked

- The photo suggests geometry/addressing is correct: the display region and rectangle structure are visible.
- The 80 MHz serial benchmark remained stable, supporting the user's belief that 80 MHz itself is likely fine.
- The production LVGL configuration clearly explains the likely discrepancy: LVGL swaps RGB565 bytes before the panel sees them, while the raw benchmark did not.

### What didn't work

The raw benchmark's generated visual pattern was too complex for first-pass validation. Because it intentionally varies RGB components with position and frame number, it can produce diagonal/rainbow patterns even when byte order is correct. That made it a poor diagnostic for distinguishing:

1. intentional generated color texture;
2. RGB-vs-BGR color ordering;
3. RGB565 byte-order swap mistakes;
4. genuine 80 MHz signal corruption.

The benchmark needs simple diagnostic frames first: solid red, green, blue, white, black, and color bars.

### What I learned

- The raw benchmark must match LVGL's `.swap_bytes = 1` behavior when sending `uint16_t` RGB565 buffers directly.
- The `esp_lcd` ILI9341 driver does not automatically byte-swap the pixel buffer; it sends the supplied bytes.
- Visual validation should begin with boring patterns, not animated procedural gradients.
- If rectangle geometry is correct but colors look wrong, the first suspects are byte order and RGB/BGR order, not SPI clock stability.

### What was tricky to build

The tricky part is that the benchmark's procedural pattern and the byte-order bug can produce similar-looking symptoms. The visible diagonal/rainbow texture was not enough by itself to prove corruption. The proof came from comparing raw-buffer generation against the production LVGL path and noticing that LVGL explicitly swaps bytes while the raw benchmark does not.

### What warrants a second pair of eyes

- Confirm visually after the fix that solid red, green, blue, white, and black screens look correct.
- If red and blue are swapped after byte swapping, inspect `panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR` and decide whether the benchmark should generate BGR565 instead of RGB565.
- Confirm 80 MHz with the simple color diagnostics. The user believes 80 MHz is fine; the fixed benchmark should make that easy to verify.

### What should be done in the future

- Patch the raw benchmark to byte-swap every RGB565 pixel before `draw_bitmap()`.
- Add a short startup diagnostic sequence of solid colors and color bars.
- Rebuild and flash at 80 MHz.
- Capture a new serial log and, ideally, a new photo or user confirmation that the diagnostic colors look correct.
- Update the guide with the visual-validation lesson.

### Code review instructions

Review the raw benchmark pixel path:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/raw_blit_benchmark_main.cpp
```

Check that:

- `make_rgb565()` still returns normal RGB565 values;
- values are byte-swapped exactly once before storage in the transmit buffer;
- solid diagnostic colors use the same byte-swap path;
- benchmark summaries still report the same transfer sizes and clock configuration.

### Technical details

Expected byte-swap helper:

```cpp
static uint16_t to_lcd_rgb565(uint16_t rgb565)
{
    return static_cast<uint16_t>((rgb565 >> 8) | (rgb565 << 8));
}
```

Expected diagnostic colors before byte swap:

```text
red   = 0xF800
green = 0x07E0
blue  = 0x001F
white = 0xFFFF
black = 0x0000
```

## Step 4: Byte-Swap Fix Visual Confirmation and VSYNC Question

After flashing the byte-swapped 80 MHz raw blit benchmark, the user reported that the colors looked better and that they saw the expected full-screen RGB diagnostic screens. This confirms the earlier diagnosis: the first raw benchmark had correct rectangle geometry but mismatched the RGB565 byte order expected by the LCD path. Matching LVGL's `.swap_bytes = 1` behavior made the raw output visually meaningful.

The user then asked whether there is a way to do VSYNC. This is the next important display-pipeline question because raw blit throughput alone does not guarantee tear-free animation. SPI LCD panels typically have their own internal GRAM and scan timing. The MCU writes rectangles into GRAM over SPI, while the panel scans that memory to the glass independently. Without a hardware tearing-effect signal wired to the MCU, the firmware can pace transfers but cannot know the exact vertical blank interval.

### Prompt Context

**User prompt (verbatim):** "is there a way to do vsync?

Colors I think look better now and I saw full screen rgb screens."

**Assistant interpretation:** The byte-swap fix likely worked visually; now evaluate whether the display pipeline can synchronize updates to panel refresh / vertical blanking.

**Inferred user intent:** Move from raw throughput measurement toward animation quality and tearing control.

### What I did

- Recorded user confirmation that the byte-swapped benchmark shows better colors and full-screen RGB diagnostic screens.
- Identified that true VSYNC would require a hardware signal from the LCD controller, typically the ILI9341/ILI9342 tearing-effect (TE) output, if the panel exposes it and the board wires it to an ESP32-S3 GPIO.
- Noted that the current StackChan display setup has known SPI pins for MOSI/SCLK/CS/DC and reset through board logic, but no TE/VSYNC GPIO appears in the firmware configuration.

### Why

A raw blit FPS number only says how fast the firmware can write pixels. It does not say whether writes land during panel vertical blanking. For tear-free animation, the firmware needs either hardware TE/VSYNC or a display interface with explicit frame timing. SPI command-mode LCDs often do not provide that to firmware unless the TE pin is wired.

### What worked

- Byte-swapped RGB565 output appears visually improved.
- Full-screen red/green/blue diagnostics are visible, which makes the raw benchmark trustworthy for further measurements.

### What didn't work

- A serial-monitor capture that was started after the byte-swap flash was interrupted by the user before results were summarized in this diary step.
- No evidence has yet been found that StackChan exposes the LCD TE signal to the ESP32-S3 firmware.

### What I learned

- The next display-quality axis is not just FPS, but synchronization and tearing behavior.
- `esp_lcd` color-transfer completion callbacks are DMA/SPI completion callbacks, not panel VSYNC callbacks.
- Software pacing to 30 FPS can reduce tearing probability and jitter, but it is not true VSYNC.

### What was tricky to build

The distinction between transfer completion and VSYNC is subtle but important. A completed SPI transfer only means the bytes reached the LCD controller's GRAM. It does not mean the panel has scanned those pixels to the visible glass, and it does not mean the write occurred during vertical blanking.

### What warrants a second pair of eyes

- Inspect StackChan/CoreS3 LCD schematic or board files for a TE/VSYNC pin.
- Confirm whether the ILI9342 variant used on the module exposes TE and whether the display flex/connector routes it.
- If TE is wired, add an experiment that enables the panel tearing-effect output and waits on a GPIO interrupt before starting full-screen blits.

### What should be done in the future

- If no TE pin exists, implement software-paced frame submission at 30 FPS and compare visual tearing.
- If TE exists, wire/enable it using the ILI9341/ILI9342 TE command and measure tear-free full-screen pacing.
- Add a diagonal moving bar test after color diagnostics; it will make tearing easier to see than random/generated patterns.

### Code review instructions

Check:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/config.h
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/hal/board/stackchan.cc
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/raw_blit_benchmark_main.cpp
```

Look for:

- any TE/VSYNC GPIO definition;
- panel IO callback usage versus true panel scan synchronization;
- whether the benchmark should add software-paced animation tests.

### Technical details

Current best understanding:

```text
Transfer completion callback = SPI/DMA transaction done
VSYNC/TE = LCD controller scan-position / tearing-effect signal
```

These are not equivalent.
