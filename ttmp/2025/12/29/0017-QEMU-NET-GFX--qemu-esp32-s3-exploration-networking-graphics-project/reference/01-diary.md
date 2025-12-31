---
Title: Diary
Ticket: 0017-QEMU-NET-GFX
Status: active
Topics:
    - esp-idf
    - esp32s3
    - qemu
    - networking
    - graphics
    - emulation
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:07:25.074732379-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Document the step-by-step process of creating a QEMU-first ESP32-S3 project focused on **networking + graphics exploration**, including commands run, failures, what we learned about QEMU, and how the repo’s existing QEMU artifacts informed decisions.

## Step 1: Create the ticket + seed it with repo QEMU knowledge

This step creates a dedicated documentation workspace for QEMU exploration (separate from the MicroQuickJS work) and consolidates the most reusable QEMU bring-up learnings already present in the repo into a playbook + analysis document.

### What I did

- Created docmgr ticket `0017-QEMU-NET-GFX`.
- Added:
  - a bring-up playbook for `idf.py qemu monitor`
  - an analysis doc that points to existing QEMU logs/scripts and lists known issues (tool install drift, partition mismatch, UART RX)
- Added an initial task list oriented around “prove or disprove networking + graphics feasibility under QEMU”.

### Why

- We already have real QEMU experience in this repo (tooling install, monitor port, log capture, a TCP client to `localhost:5555`, and known failure modes). A dedicated ticket lets us turn that into a repeatable workflow before building a new “net+gfx” QEMU project.

### What worked

- We found and reused existing repo artifacts that are QEMU-centric (not inherently JS-centric):
  - `imports/qemu_storage_repl.txt` (golden log)
  - `imports/test_storage_repl.py` (raw TCP client to the QEMU serial port)

### What didn’t work

- N/A (this step is documentation and setup).

### What I learned

- In this repo, the QEMU “serial interface” is typically exposed as a TCP server on port `5555`, and tools connect as `socket://localhost:5555`.

### What warrants a second pair of eyes

- Whether the proposed “graphics in QEMU” definitions are aligned with what ESP32-S3 QEMU can realistically emulate (likely we’ll need a host-side viewer approach).

### What should be done in the future

- Implement the actual QEMU-first "net+gfx probe" project and iterate based on observed capabilities/limitations.

## Step 2: Research M5GFX/LovyanGFX with QEMU and LCD drivers

This step systematically researched how M5GFX (built on LovyanGFX) graphics libraries can be integrated with QEMU emulation for ESP32-S3, focusing on LCD driver options, QEMU's graphics capabilities, and practical integration approaches.

**Commit (code):** N/A (research + documentation)

### What I did

- Searched codebase for M5GFX/LovyanGFX usage patterns and architecture
- Researched ESP-IDF LCD component (`esp_lcd`) and QEMU graphics support
- Investigated LovyanGFX framebuffer panel (`Panel_fb`) implementation
- Analyzed real hardware display initialization examples from existing ESP-IDF projects
- Created comprehensive analysis document: `analysis/02-m5gfx-and-lovyangfx-with-qemu-lcd-driver-integration-and-emulation-approaches.md`
- Related key source files to the analysis document

### Why

- Understanding graphics library integration with QEMU is critical for the "graphics probe" task
- Need to document available approaches before implementing QEMU graphics support
- Real hardware examples in the repo provide reference for comparison with QEMU approaches

### What worked

- Found that Espressif provides official `esp_lcd_qemu_rgb` component for QEMU virtual framebuffer
- Discovered LovyanGFX has framebuffer panel (`Panel_fb`) that could potentially work with QEMU
- Identified multiple integration approaches with different trade-offs
- Located real hardware examples showing M5GFX/LovyanGFX initialization patterns

### What didn't work

- Could not find definitive documentation on whether QEMU exposes `/dev/fb0` for framebuffer panel access
- No clear examples of M5GFX/LovyanGFX working directly with QEMU (requires further investigation)

### What I learned

- **M5GFX architecture**: M5GFX wraps LovyanGFX; LovyanGFX handles low-level LCD communication via SPI/I2C buses
- **QEMU graphics**: QEMU supports virtual RGB framebuffer via `esp_lcd_qemu_rgb` component (Espressif's QEMU fork v8.1.3+)
- **Integration challenge**: QEMU does not emulate SPI LCD controllers (GC9107, ST7789), so graphics must use virtual framebuffer
- **Multiple approaches**: Four viable approaches identified:
  1. ESP-IDF native (`esp_lcd_qemu_rgb`) - official but loses M5GFX API
  2. LovyanGFX framebuffer panel (`Panel_fb`) - maintains API but requires framebuffer device
  3. LovyanGFX SDL panel - alternative to QEMU for PC simulation
  4. Hybrid adapter layer - wraps `esp_lcd_qemu_rgb` with M5GFX interface

### What was tricky to build

- Understanding the relationship between M5GFX, LovyanGFX, and ESP-IDF's `esp_lcd` component
- Determining which QEMU graphics capabilities are actually available vs. theoretical
- Balancing between maintaining M5GFX API compatibility vs. using official QEMU components

### What warrants a second pair of eyes

- Whether QEMU's virtual framebuffer is accessible as `/dev/fb0` from within the emulated system (affects `Panel_fb` viability)
- Evaluation of the hybrid adapter approach feasibility and performance implications
- Comparison of QEMU graphics timing/behavior with real hardware

### What should be done in the future

- **Test `esp_lcd_qemu_rgb`**: Create minimal QEMU project to verify graphics output works
- **Investigate framebuffer device**: Check if QEMU exposes `/dev/fb0` or similar for `Panel_fb` access
- **Prototype adapter layer**: If M5GFX integration needed, implement proof-of-concept adapter
- **Document QEMU graphics workflow**: Create playbook based on chosen approach
- **Compare with real hardware**: Test same graphics code on real hardware to identify differences

### Code review instructions

- Review `analysis/02-m5gfx-and-lovyangfx-with-qemu-lcd-driver-integration-and-emulation-approaches.md` for comprehensive findings
- Check related files:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/framebuffer/Panel_fb.hpp` - framebuffer panel implementation
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp` - real hardware example

### Technical details

**Key findings:**

1. **QEMU component**: `espressif/esp_lcd_qemu_rgb^1.0.2` provides `esp_lcd`-compatible interface to QEMU's virtual RGB framebuffer

2. **LovyanGFX framebuffer support**: `Panel_fb` maps to Linux framebuffer devices (`/dev/fb0`), potentially usable if QEMU exposes such device

3. **Real hardware pattern**: Standard M5GFX initialization involves:
   - Configure SPI bus (`Bus_SPI`)
   - Configure panel (`Panel_GC9107` for AtomS3R)
   - Initialize display (`M5GFX::init()`)

4. **QEMU limitations**: No SPI LCD controller emulation, no GPIO emulation for LCD control pins, timing differences from real hardware

**Web research queries:**
- "ESP-IDF esp_lcd QEMU emulation display driver"
- "M5GFX LovyanGFX QEMU ESP32-S3 LCD emulation"
- "esp_lcd_qemu_rgb component ESP-IDF documentation usage example"
- "ESP-IDF QEMU graphics monitor framebuffer display emulation"
- "QEMU ESP32-S3 framebuffer device /dev/fb0 access from guest"
- "LovyanGFX Panel_fb QEMU Linux framebuffer integration"
- "ESP-IDF esp_lcd_qemu_rgb example code usage tutorial"
- "M5GFX ESP-IDF QEMU emulation graphics development"
- "ESP32-S3 QEMU graphics display limitations SPI LCD emulation"
- "M5Stack LVGL device emulator SDL framebuffer github"
- "ESP32-S3 QEMU virtual RGB panel framebuffer memory mapping"

**Additional findings from extended web research:**
- M5Stack provides `lv_m5_emulator` project (https://github.com/m5stack/lv_m5_emulator) using SDL for M5Stack device emulation
- QEMU can use SDL for graphics rendering (separate from virtual framebuffer approach)
- LovyanGFX SDL panel is a proven approach used by M5Stack's own emulator project
- Framebuffer emulation in QEMU requires configuration but is possible

## Step 3: Analyze M5Stack LVGL Emulator Architecture

This step cloned and analyzed the M5Stack LVGL emulator repository to understand how M5Stack implements graphics emulation using SDL and M5GFX, and how this relates to QEMU graphics development.

**Commit (code):** N/A (research + documentation)

### What I did

- Cloned M5Stack LVGL emulator repository: `imports/lv_m5_emulator`
- Studied source code structure and key files:
  - `src/main.cpp`: Entry point with setup()/loop() pattern
  - `src/utility/sdl_main.cpp`: SDL entry point using `Panel_sdl::main()`
  - `src/utility/lvgl_port_m5stack.cpp`: LVGL porting layer bridging LVGL with M5GFX
  - `platformio.ini`: PlatformIO configuration showing native vs ESP32 builds
- Analyzed LovyanGFX `Panel_sdl` implementation in codebase
- Created comprehensive analysis document: `analysis/03-m5stack-lvgl-emulator-architecture-analysis-and-integration-with-qemu-graphics-project.md`
- Related key source files to analysis document

### Why

- M5Stack LVGL emulator demonstrates a proven approach to graphics emulation
- Understanding its architecture helps evaluate QEMU graphics integration strategies
- Provides reference patterns for graphics library integration and platform abstraction

### What worked

- Successfully cloned and analyzed the repository
- Identified key architectural patterns:
  - SDL-based native PC emulation (not QEMU)
  - M5GFX auto-detection of SDL environment
  - LVGL porting layer pattern
  - Conditional compilation for multiple platforms
- Found valuable integration patterns applicable to QEMU

### What didn't work

- N/A (analysis completed successfully)

### What I learned

- **M5Stack emulator is SDL-based, not QEMU**: Uses PlatformIO `native` platform for direct PC execution
- **M5GFX auto-detection**: Automatically uses `Panel_sdl` when SDL headers available
- **LVGL porting layer**: Clean abstraction bridging LVGL with M5GFX display driver
- **Chunked pixel transmission**: Uses 8K pixel chunks to avoid SIMD optimization issues
- **Double buffering**: Uses aligned memory allocation (64-byte) for performance
- **Thread safety**: Uses SDL mutexes for multi-threaded LVGL operations
- **Platform abstraction**: Same code compiles for native PC and ESP32 hardware

### What was tricky to build

- Understanding the relationship between SDL, M5GFX, and LVGL layers
- Distinguishing between native PC execution (SDL) vs QEMU emulation
- Identifying which patterns are applicable to QEMU vs SDL-specific

### What warrants a second pair of eyes

- Evaluation of adapter layer approach (`Panel_QEMU_RGB`) feasibility
- Comparison of performance characteristics between SDL emulator and QEMU framebuffer
- Assessment of whether M5GFX features are needed in QEMU or if basic `esp_lcd` API is sufficient

### What should be done in the future

- **Prototype adapter layer**: Implement `Panel_QEMU_RGB` following M5Stack emulator patterns
- **Test QEMU framebuffer**: Create minimal project using `esp_lcd_qemu_rgb` to verify approach
- **Compare approaches**: Evaluate SDL emulator vs QEMU framebuffer for different use cases
- **Document integration**: Create playbook for QEMU graphics development based on chosen approach

### Code review instructions

- Review `analysis/03-m5stack-lvgl-emulator-architecture-analysis-and-integration-with-qemu-graphics-project.md` for comprehensive analysis
- Check related files:
  - `imports/lv_m5_emulator/src/main.cpp` - Entry point pattern
  - `imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp` - LVGL porting layer
  - `imports/lv_m5_emulator/src/utility/sdl_main.cpp` - SDL entry point
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/sdl/Panel_sdl.hpp` - SDL panel implementation

### Technical details

**Key architectural findings:**

1. **Execution model**: Native PC application using PlatformIO `native` platform, not QEMU emulation
2. **Graphics path**: M5GFX → LovyanGFX `Panel_sdl` → SDL window
3. **LVGL integration**: Custom porting layer (`lvgl_port_m5stack.cpp`) bridges LVGL with M5GFX
4. **Platform support**: Conditional compilation supports both native PC and ESP32 hardware
5. **Performance optimizations**: Chunked pixel transmission, double buffering, aligned memory

**Comparison with QEMU:**

| Aspect | M5Stack Emulator (SDL) | QEMU Approach |
|--------|------------------------|--------------|
| Platform | Native PC | QEMU emulated ESP32-S3 |
| Graphics | SDL window | Virtual RGB framebuffer |
| Panel | LovyanGFX `Panel_sdl` | ESP-IDF `esp_lcd_qemu_rgb` |
| Execution | Direct PC execution | Emulated ESP32 firmware |
| Performance | Native speed | Emulation overhead |

**Integration strategies identified:**

1. **Adapter layer**: Create `Panel_QEMU_RGB` similar to how M5GFX uses `Panel_sdl`
2. **Direct `esp_lcd`**: Use `esp_lcd_qemu_rgb` directly without M5GFX
3. **Hybrid approach**: Use M5GFX APIs with conditional compilation for QEMU vs hardware

**Codebase searches:**
- M5GFX/LovyanGFX architecture and usage patterns
- ESP-IDF LCD driver components
- LovyanGFX framebuffer panel implementation
- Real hardware display initialization examples
