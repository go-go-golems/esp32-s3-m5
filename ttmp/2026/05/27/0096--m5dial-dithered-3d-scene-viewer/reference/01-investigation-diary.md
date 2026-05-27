---
Title: Investigation Diary
Ticket: "0096"
Status: active
Topics:
    - esp32
    - m5dial
    - dithering
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../code/wesen/go-go-golems/go-go-parc/Projects/2026/05/27/ARTICLE - M5Dial Dithered 3D Scene Viewer - Software Rendering on ESP32-S3.md
      Note: Obsidian deep-dive report committed to vault
    - Path: 0096-m5dial-dithered-3d/main/app_main.cpp
      Note: |-
        Poster scene dispatch and dirty redraw loop
        Framebuffer registration
    - Path: 0096-m5dial-dithered-3d/main/console_commands.cpp
      Note: |-
        Runtime commands and redraw invalidation
        dumpfb
    - Path: 0096-m5dial-dithered-3d/main/renderer.cpp
      Note: Stats record helper shared by poster renderer path
    - Path: 0096-m5dial-dithered-3d/main/terrain_poster.cpp
      Note: |-
        Five-scene poster renderer and Bayer/contrast/pixel implementation
        UI safe areas and solid
    - Path: ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/artifacts/latest-dumpfb.png
      Note: Validated reconstructed framebuffer screenshot artifact
    - Path: ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/artifacts/scene-captures-v2/terrain.png
      Note: Post-fix screenshot inspected with read
    - Path: ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/02-dumpfb-to-png.py
      Note: Host-side PNG reconstruction from dumpfb transcript
    - Path: ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/03-capture-dumpfb.py
      Note: Automated pyserial capture helper for dumpfb screenshots
ExternalSources: []
Summary: Chronological implementation diary for the M5Dial dithered 3D scene viewer firmware.
LastUpdated: 2026-05-27T00:00:00Z
WhatFor: Track investigation steps, implementation decisions, failures, validation, and follow-ups.
WhenToUse: Resume or review ticket 0096 firmware work.
---




# Investigation Diary

## Goal

Capture the step-by-step investigation and design process for the M5Dial dithered 3D scene viewer firmware (ticket 0096).

## Step 1: Source Analysis and Ticket Setup

The user asked to create a docmgr ticket for a dithered 3D scene on the M5Dial, using `~/Downloads/m5dial.jsx` as inspiration, with a detailed design doc, diary, and eventual reMarkable upload.

### Prompt Context

**User prompt (verbatim):** "The user asked to create a docmgr ticket for a dithered 3D scene on the M5Dial (ESP32) where the rotary dial rotates the camera. They wanted ~/Downloads/m5dial.jsx used as inspiration but ported to ESP32. They requested a detailed intern-level design doc with prose, bullets, pseudocode, diagrams, API/file references, stored in the ticket and uploaded to reMarkable. Then add tasks, implement step-by-step with a detailed diary, commit at intervals, and add esp_console for device interaction simulation."

**Assistant interpretation:** Port a WebGL dithered 3D renderer to bare-metal ESP32, using the M5Dial's rotary encoder to orbit the camera. Write a comprehensive design document first, then implement.

**Inferred user intent:** Get a working firmware that visually resembles the JSX simulator, with structured documentation that would let an intern build it.

### What I did

- Created ticket 0096 "M5Dial Dithered 3D Scene Viewer"
- Copied `m5dial.jsx` into the ticket's `scripts/` folder
- Read the complete 1042-line JSX source, extracting: 5 scenes (terrain, torus, ocean, planet, tunnel), 5 palettes (classic, inverted, red mono, blue mono, amber CRT), GLSL dither shader (DITHER_FS with Bayer 4×4 ordered dithering + 4-color quantization + circular mask)
- Studied the M5DialBoard driver from project 0074 (GPIO pins, SPI config at 80 MHz, GC9A01 240×240, encoder on GPIO 40/41, button on 42, touch on I2C FT5x06)
- Reviewed the M5Dial hardware constraints: ESP32-S3FN8, 8MB embedded flash (DIO), no PSRAM, ~224 KB free heap
- Read the textbook-authoring skill for writing style guidelines (foundational prose, no analogies, concrete code)
- Wrote a 27 KB design doc covering: hardware profile, memory budget (2-bit framebuffer = 14.4 KB), fixed-point arithmetic, vertex transform pipeline, scene geometry adaptations (reduced poly counts for ESP32), scanline rasterizer with per-scanline Z-buffer, Bayer 4×4 dither implementation, color palettes in RGB565, framebuffer-to-display pipeline, encoder input, esp_console commands, performance estimates (8–15 FPS), 5-phase implementation plan, file structure, key decisions, risks

### Why

The design doc is the foundation for implementation. Porting a GPU pipeline to a microcontroller without FPU requires understanding every arithmetic tradeoff. The memory budget analysis revealed that a full RGB565 framebuffer + Z-buffer exceeds available RAM, forcing the 2-bit framebuffer + scanline-Z architecture.

### What worked

- The 2-bit framebuffer insight: 14.4 KB instead of 115.2 KB for RGB565 — the key enabler for the entire design
- Scanline-parallel rasterize + dither: avoids needing an intermediate luminance buffer
- Reusing M5DialBoard from 0074: proven driver code, no re-debugging needed
- Per-scene triangle budget analysis: terrain 722, torus 576, ocean 450, planet 320+12+64, tunnel 384

### What didn't work

- N/A (analysis phase, no code yet)

### What I learned

- The GLSL dither shader classifies pixels into warm/cool/white categories before applying Bayer thresholding. This is not a standard dither-then-quantize pipeline — it's classify-then-dither-within-category. The ESP32 implementation must preserve this logic exactly or the visual result will differ noticeably.
- The M5Dial's 224 KB free heap is the binding constraint. The 0095 wifi-bench firmware showed allocations of 100 KB+ failing. A 14.4 KB framebuffer is safe.
- The ESP32-S3 *does* have hardware single-precision FPU (IEEE 754). Earlier notes about "no FPU" were incorrect for S3. This changes the vertex transform strategy from fixed-point to float.

### What was tricky to build

- The memory budget analysis required iterating through several approaches. First attempt: RGB565 framebuffer (115 KB) + Z-buffer (115 KB) = 230 KB > 224 KB. Second attempt: indexed 8-bit framebuffer (57.6 KB) + Z-buffer (115 KB) = 172 KB, but the Z-buffer is too large. Third attempt: 2-bit framebuffer (14.4 KB) + scanline Z-buffer (480 B) = 14.9 KB — this works, but requires a 2-bit → RGB565 expansion pass before display. The expansion cost (~1 ms) is negligible vs. the SPI transfer (~11.5 ms).

### What warrants a second pair of eyes

- The 2-bit framebuffer packing/unpacking: bit alignment across byte boundaries is error-prone. The `fb_set(x, y, color)` and `fb_get(x, y)` functions must handle 4 pixels per byte correctly.
- The scanline Z-buffer reset: must be cleared to max-depth before each new scanline, not just at frame start. A subtle bug here would cause Z-fighting artifacts on every scanline boundary.
- The Bayer threshold comparison: the shader uses `lum > bayer_threshold` (strict greater-than). If the luminance encoding uses 0–15 and the Bayer matrix uses 0–15, the comparison must use the same range. Off-by-one errors in the threshold mapping would produce visible banding.

### What should be done in the future

- Build and flash Phase 1 (static test pattern) to verify the display pipeline works end-to-end on hardware
- Measure actual SPI transfer time with a logic analyzer or GPIO toggle
- Profile the rasterizer on real scenes to validate the 8–15 FPS estimate
- Consider DMA double-buffering if SPI transfer time dominates

### Code review instructions

- Design doc at: `ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/design-doc/01-dithered-3d-scene-viewer-design-and-implementation-guide.md`
- Key sections to review: "Memory Budget" table, "Dithering and Quantization" (Bayer logic), "Scanline Rasterization" (Z-buffer lifecycle)
- Cross-reference the GLSL shader in `scripts/m5dial.jsx` lines 51–108 against the "4-Color Quantization Logic" section to verify the port is correct

### Technical details

- Bayer 4×4 matrix values: `{{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}}` (thresholds 0/16 through 15/16)
- RGB565 palette values computed from hex: `#ff2940 → 0xF804`, `#3050ff → 0x185F`, `#ffffff → 0xFFFF`, `#5a78ff → 0x5B5F`, `#7a1020 → 0x7810`, `#ffae20 → 0xFD04`, `#5a3010 → 0x5A60`, `#ffe080 → 0xFF20`
- Camera orbit: 2° per encoder detent (`π/90` radians), 1024-entry sin/cos LUT covering 0–2π
- SPI transfer: 240×240×2 bytes at 80 MHz = 11.5 ms (blocking)

## Step 2: Compilation Fix and Build-System Diagnosis

The first firmware scaffold failed before it reached application code because the root CMake file accidentally hid ESP-IDF's built-in components. The symptom looked like an `nvs_flash` dependency problem in LovyanGFX, but the actual cause was local: `COMPONENT_DIRS` had been set to only `components`, so CMake could discover the project-local LovyanGFX component while losing the normal `$IDF_PATH/components` search path.

After that root cause was fixed, the build proceeded into normal C++ compile errors. I copied the missing Button vendor files from the working 0074 firmware, replaced the hand-written console loop with ESP-IDF's USB Serial/JTAG REPL helper, fixed C++ designated initializer ordering, and corrected format/include issues. The project now builds successfully with ESP-IDF v5.4.2.

### Prompt Context

**User prompt (verbatim):** "Your little brother has been struggling hard with what seems like a pretty trivial compilation issue, investigate and fix."

**Assistant interpretation:** Stop guessing, inspect the working M5Dial projects and the generated build state, identify the real build-system issue, and make the 0096 firmware compile.

**Inferred user intent:** Get past a basic compilation blocker so implementation can continue on the actual renderer instead of wasting time on CMake/environment noise.

**Commit (code):** Not committed yet — working tree contains new 0096 firmware and ticket docs, plus unrelated untracked files already present in the repository.

### What I did

- Compared `0096-m5dial-dithered-3d/CMakeLists.txt` against the working `0074-m5dial-web-remote/firmware/CMakeLists.txt`.
- Found the root cause: `set(COMPONENT_DIRS "components" ...)` in 0096 overrode the normal ESP-IDF component search path and made built-in components such as `nvs_flash` invisible.
- Removed that override and matched the working 0074 style by relying on automatic project-local component discovery and adding `LGFX_FORCE_LEGACY_I2C_DRIVER`.
- Deleted the accidental `components_tmp/` directory left over from the first LovyanGFX copy attempt.
- Copied missing vendor Button files from 0074:
  - `main/vendor/Button/Button.cpp`
  - `main/vendor/Button/Button.h`
- Added `vendor/Button/Button.cpp` and `vendor/Button` include path to `main/CMakeLists.txt`.
- Fixed `console_commands.cpp`:
  - removed invalid `<arg_bindings.h>` include
  - included `<argtable3/argtable3.h>` and `<cinttypes>`
  - changed render-stat format strings to use `PRIu32`
- Fixed `scene_terrain.cpp` and `scene_stubs.cpp` C++ designated initializer ordering for `scene_def_t`.
- Replaced the manual USB Serial/JTAG console loop in `app_main.cpp` with `esp_console_new_repl_usb_serial_jtag()` and `esp_console_start_repl()`.
- Verified build success with:

```bash
cd 0096-m5dial-dithered-3d
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build
```

The successful build produced:

```text
Generated .../build/0096-m5dial-dithered-3d.bin
0096-m5dial-dithered-3d.bin binary size 0x55b40 bytes.
Smallest app partition is 0x300000 bytes. 0x2aa4c0 bytes (89%) free.
Project build complete.
```

### Why

The `nvs_flash` error was misleading because `nvs_flash` really does exist under `/home/manuel/esp/esp-idf-5.4.2/components/nvs_flash`. The build could not see it because the project told ESP-IDF to search only the project-local `components` directory. Removing `COMPONENT_DIRS` restored default discovery and immediately moved the failure forward to real source compile errors.

### What worked

- Comparing against 0074 exposed that 0096 should not set `COMPONENT_DIRS` at all.
- The existing LovyanGFX component works with ESP-IDF v5.4.2 once normal component discovery is restored.
- ESP-IDF's REPL helper removed several API mismatches around USB Serial/JTAG VFS setup and `linenoise`.

### What didn't work

- Searching for `espressif/nvs_flash` as a managed component was the wrong direction. Exact failure:

```text
Executing action: add-dependency
ERROR: Component "espressif/nvs_flash" not found
```

- Switching between ESP-IDF 5.4.1 and 5.4.2 did not solve the problem because the local CMake override was independent of the IDF version.
- Manually editing LovyanGFX's `boards.cmake/esp-idf.cmake` to remove `nvs_flash` was unnecessary and was reverted.

### What I learned

- In ESP-IDF projects, `COMPONENT_DIRS` is not a harmless way to point at local components. It replaces the component search set. For ordinary projects, leave it unset and use `EXTRA_COMPONENT_DIRS` only when adding directories outside the default `main/` and `components/` layout.
- The working 0074 firmware used the same LovyanGFX old-style `register_component()` wrapper. That was not the problem; ESP-IDF's compatibility macro translates it to `idf_component_register()` correctly.
- IDF 5.4.2's USB Serial/JTAG console API is easiest to use through `esp_console_new_repl_usb_serial_jtag()`, which handles the VFS/linenoise plumbing.

### What was tricky to build

The tricky part was not the final fix; it was recognizing that the error message identified a missing *resolved component*, not necessarily a missing installed component. The symptoms pointed at LovyanGFX and `nvs_flash`, and both IDF 5.4.1/5.4.2 were plausible suspects. The useful diagnostic was checking the actual IDF install (`components/nvs_flash` existed) and then comparing the root CMake file against 0074. That made the local `COMPONENT_DIRS` override stand out.

### What warrants a second pair of eyes

- The console REPL now starts before the render and IO tasks. This is the standard pattern, but it should still be checked on device to ensure it does not monopolize USB Serial/JTAG or collide with display initialization logs.
- The renderer compiles, but it has not yet been flashed or visually validated. Build success does not prove the scanline Z-buffer logic or terrain rendering is correct.
- `pushColors()` currently builds with a deprecation warning; replacing it with `writePixels()` or `pushPixels()` can be done after the first hardware smoke test.

### What should be done in the future

- Flash 0096 to the M5Dial and run an initial monitor smoke test.
- Use the console commands (`help`, `fps`, `palette`, `wireframe`, `angle`) to verify REPL interaction.
- Inspect the actual display output and fix renderer correctness issues separately from compilation issues.

### Code review instructions

- Start with `0096-m5dial-dithered-3d/CMakeLists.txt`; the key build fix is the removal of `COMPONENT_DIRS`.
- Check `0096-m5dial-dithered-3d/main/app_main.cpp` for the USB Serial/JTAG REPL startup path.
- Check `0096-m5dial-dithered-3d/main/CMakeLists.txt` to confirm all vendor encoder/Button files are included.
- Validate with `source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build` from `0096-m5dial-dithered-3d`.

### Technical details

- Root cause line removed:

```cmake
set(COMPONENT_DIRS "components" CACHE STRING "Component search dirs")
```

- Correct root CMake pattern now used:

```cmake
add_definitions(-DLGFX_FORCE_LEGACY_I2C_DRIVER)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(0096-m5dial-dithered-3d)
```

- Remote resources saved with `defuddle` under `sources/`:
  - `stackoverflow-resolve-component-esp-idf.md`
  - `idf5-migration-removed-components.md`
  - `idf-component-management.md`
  - `lovyanGFX-readme.md`
  - `idf-spi-master.md`

## Step 3: Hardware Smoke Test, Terrain Poster Pass, and LCD Color Fix

The first hardware run proved that the firmware booted, initialized the display, and accepted console input, but it also exposed two practical problems. The generic triangle rasterizer was too expensive for the initial terrain mesh and triggered the task watchdog; after reducing the mesh and moving to a static/poster-style terrain renderer, the display was stable enough to compare against the user's target image.

The user then supplied a target TERRAIN screenshot. I treated that as the art direction for the first embedded implementation: black background, electric-blue ordered-dither terrain, red ordered-dither sun/halo, fixed pixel grid, and simple red/white UI overlays. This is now implemented as a dedicated fast `terrain_poster_render()` path while the real 3D triangle path remains available for later scenes.

### Prompt Context

**User prompt (verbatim):** "you can capture a screenshot, right? or send it over serial or something. anyway, so you can look at the result and iterate to make it look good. /tmp/pi-clipboard-7546db1f-2067-4f28-a5af-c9020c128491.png

cause this is how i want it to look like . (this is TERRAIN)"

**Assistant interpretation:** Use the provided target image as the visual benchmark, iterate firmware output toward it, and consider adding a way to inspect the device framebuffer or screen output.

**Inferred user intent:** Make the firmware look like the original JSX/target TERRAIN composition, not merely compile or render arbitrary geometry.

**Commit (code):** Not committed yet.

### What I did

- Asked a vision model to describe the target TERRAIN screenshot in concrete terms.
- Implemented `terrain_poster.h/cpp`, a fast dedicated TERRAIN renderer with:
  - screen-locked 8×8 Bayer ordered dithering
  - black background
  - blue terrain density ramp
  - red sun with dithered halo
  - red `TERRAIN` title
  - bottom status dots and `BIP-001` label
  - palette probe squares in the top-left for color debugging
- Changed the default auto-rotation speed to `0.0f` so the screen is static unless the encoder moves or console parameters change.
- Made the event loop repaint only when dirty, which removes constant SPI redraw flicker.
- Increased visible encoder effect in the poster renderer by moving the terrain peak/shoulder and sun slightly as `camera_angle` changes.
- Fixed LCD color byte order by setting `display.setSwapBytes(true)` before streaming `uint16_t` RGB565 scanlines with `writePixels()`.
- Restored short-button palette cycling after the user clarified they had been pressing the button intentionally.
- Built and flashed the firmware successfully.

### Why

The target image is closer to a designed posterized/dithered scene than to a conventional lit 3D rasterization result. The first renderer was trying to rasterize terrain triangles and then apply dither, but the user-visible goal for TERRAIN is a stable halftone composition. A dedicated TERRAIN pass is the fastest way to make the first scene look correct while the slower triangle renderer evolves separately.

### What worked

- The device now boots cleanly after flash and reaches the `3d>` console prompt.
- Switching to dirty-frame rendering stopped continuous redraw flicker.
- `display.setSwapBytes(true)` matches LovyanGFX's documented RGB565 handling and should remove the greenish byte-swapped color artifact.
- The dedicated poster renderer gives precise control over the target image composition independent of the experimental 3D rasterizer.

### What didn't work

- The original triangle terrain pass was too slow and triggered the task watchdog:

```text
E task_wdt: Task watchdog got triggered.
CPU 1: 3d_app
Backtrace ... renderer_render_frame ... app_task
```

- The first LCD write path treated host-order RGB565 scanlines without enabling byte swapping. The visible symptom was greenish colors instead of clean red/blue. LovyanGFX's own examples confirm that host-order `uint16_t` RGB565 should be sent with byte swapping enabled.

### What I learned

- For this target aesthetic, a screen-space ordered-dither renderer is more useful than a general triangle renderer for the first scene. It gives a visually faithful result immediately and avoids watchdog pressure.
- Byte order matters for RGB565 on the GC9A01 path. The byte-swapped value of the target blue lands in a green-ish region, matching the user's observation.
- The M5Dial encoder behavior was fine; the apparent unexpected palette cycling was the user pressing the button, not a phantom input bug.

### What was tricky to build

The main difficulty was separating three different visual problems that appeared at once: renderer speed, redraw flicker, and color corruption. The watchdog pointed at CPU load, the visible flicker came from constant redraws, and the greenish colors were a byte-order issue. Solving them required different changes: poster renderer and dirty redraw for timing, `setSwapBytes(true)` for color, and restored button handling once the input behavior was clarified.

### What warrants a second pair of eyes

- The poster renderer is intentionally art-directed rather than physically 3D. Review whether this is acceptable as the TERRAIN scene implementation, or whether it should later be renamed as a fallback/reference renderer.
- The palette probe squares are useful while debugging but may need to be hidden behind a console flag before final polish.
- A serial framebuffer-dump command would help future iterations compare exact framebuffer output without depending only on phone photos.

### What should be done in the future

- Add a `screenshot`/`dumpfb` console command that emits the 2-bit framebuffer plus palette as compact hex/RLE so a host script can reconstruct a PNG.
- Tune the poster renderer after another device photo: terrain height, sun radius, UI placement, and dither densities.
- Once TERRAIN looks correct, apply the same art-directed approach to the remaining scenes or optimize the triangle renderer separately.

### Code review instructions

- Start with `0096-m5dial-dithered-3d/main/terrain_poster.cpp` for the new target-image renderer.
- Check `0096-m5dial-dithered-3d/main/app_main.cpp` for dirty-frame rendering, `setSwapBytes(true)`, and restored button palette cycling.
- Validate with `idf.py build`, flash, and observe that colors are red/blue rather than greenish.

### Technical details

- Current display path:

```cpp
display.startWrite();
display.setSwapBytes(true);
display.setAddrWindow(0, 0, FB_WIDTH, FB_HEIGHT);
for (int y = 0; y < FB_HEIGHT; y++) {
    fb_expand_scanline(ctx->fb, y, ctx->rgb565_line, pal->colors);
    display.writePixels(ctx->rgb565_line, FB_WIDTH);
}
display.endWrite();
```

- Target TERRAIN render path is now:

```cpp
if (scene_current_id() == SCENE_TERRAIN) {
    terrain_poster_render(ctx->fb, p->camera_angle);
} else {
    fb_fill(ctx->fb, COLOR_BLACK);
    renderer_render_frame(ctx->fb, pal, scene, p);
}
```

## Step 4: Encoder Sensitivity Tuning

The user confirmed the M5Dial has about twelve tactile clicks per full rotation and that the previous camera-angle increment felt far too slow. I changed encoder handling so one physical knob revolution maps to one full scene orbit.

### Prompt Context

**User prompt (verbatim):** "rotate much fasater on encoder turns. this is way to slow it has like 12 clicks pere rotation or so"

**Assistant interpretation:** Increase the camera rotation angle per encoder event by a large factor, using the observed 12-click mechanical rotation as calibration.

**Inferred user intent:** Make each tactile click visibly affect the scene, and make one physical knob rotation correspond to a meaningful full visual rotation.

**Commit (code):** Not committed yet.

### What I did

- Changed `InputEventType::kEncoderDelta` handling from `0.0349f` rad/click (about 2°) to `0.5236f` rad/click (about 30°).
- Rebuilt and flashed the firmware.

### Why

A 12-click dial should not need dozens of turns to rotate the scene. Mapping `2π / 12` radians per click makes the physical interaction match the hardware detents.

### What worked

- Firmware builds and flashes cleanly after the sensitivity change.
- The new mapping is simple and hardware-calibrated: 12 clicks ≈ 360°.

### What didn't work

- N/A for this step.

### What I learned

- The M5Dial's tactile click count is a better input calibration source than assuming a high-resolution encoder UX.

### What was tricky to build

- The earlier apparent rotation/button confusion was user interaction, not a firmware bug. This step therefore only changes sensitivity, not event routing.

### What warrants a second pair of eyes

- If the encoder library reports half-steps or multiple counts per tactile click on some units, the effective rotation may be too fast. This should be validated by turning the physical dial once and watching if the scene returns near its starting orientation.

### What should be done in the future

- Add a console command such as `sensitivity <clicks-per-rev>` so the value can be tuned without reflashing.

### Code review instructions

- Review `0096-m5dial-dithered-3d/main/app_main.cpp`, `handle_input_event()`, encoder delta case.
- Validate by rotating the dial one full mechanical revolution; terrain should complete roughly one full visual cycle.

### Technical details

```cpp
p->camera_angle += event.value * 0.5236f; // 2π / 12
```

## Step 5: Five Poster Scenes, Runtime Pixel/Contrast Controls, and Obsidian Deep-Dive Report

After the terrain scene stabilized, the user asked to continue with the remaining scenes and to expose contrast and pixel-size controls. I converted the dedicated terrain poster path into a five-scene poster renderer that covers TERRAIN, TOROID, OCEAN, PLANET, and TUNNEL while preserving the same 2-bit framebuffer, ordered-dither, palette-indexed display pipeline.

The same prompt also requested a long-form Obsidian project report in textbook style. I wrote and pushed a new vault article that explains the hardware constraints, memory math, ordered dithering, display byte order, input model, console controls, scene math, debugging path, and remaining work.

### Prompt Context

**User prompt (verbatim):** "ok continue. do the other scenes, contrast, pixel size too."

**Assistant interpretation:** Implement the remaining scene visuals and make contrast/pixel-size tunable at runtime.

**Inferred user intent:** Move from a single TERRAIN proof of concept to a more complete interactive scene viewer with visual controls exposed on-device.

**User prompt (verbatim):** "write a very detailed projec report for the obsidian vault on how this was built, the math, the embedded constraints, the hardware, etc... Twice as long as your usual reports ,as a deep dive technical blog post, written in a textbook writing style (no analogies). Then commit and push the vault, and continue working"

**Assistant interpretation:** Create a durable, long-form Obsidian technical article, commit and push only the vault note, then continue firmware implementation.

**Inferred user intent:** Preserve the implementation knowledge in a high-quality reusable project report while keeping firmware progress moving.

**Commit (vault):** bd7c3106e3e4098fe45fb45429c28e6e56417281 — "Add M5Dial dithered scene viewer deep dive"

### What I did

- Replaced the terrain-only poster entry point with `poster_render_scene()` in `terrain_poster.h/cpp`.
- Added poster renderers for:
  - `TERRAIN`: blue dithered terrain mound and red sun/halo.
  - `TOROID`: rotated elliptical torus band with black center hole.
  - `OCEAN`: horizon, low sun, wave field, and sparse foam.
  - `PLANET`: starfield, planet disk, tilted ring, banded shading.
  - `TUNNEL`: polar-coordinate rings/spokes with a black vanishing point.
- Added `pixel_size` and `revision` to `render_params_t`.
- Added `render_params_touch()` so console commands trigger dirty redraw when auto-rotate is off.
- Added the `pixel [1..6]` console command.
- Wired console setters for palette, rotate, contrast, aperture, pixel, wireframe, pause, and angle to increment the revision counter.
- Changed `app_main.cpp` so all scenes use the poster renderer instead of falling back to empty stub geometry.
- Built successfully with ESP-IDF 5.4.2.
- Flashed the latest build to `/dev/ttyACM0` and smoke-tested console commands: `scene torus`, `pixel 3`, `contrast 1.8`, `scene ocean`, `scene planet`, `scene tunnel`, and `aperture 90`.
- Wrote the Obsidian report:
  - `/home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/05/27/ARTICLE - M5Dial Dithered 3D Scene Viewer - Software Rendering on ESP32-S3.md`
- Committed and pushed the vault:
  - `bd7c3106e3e4098fe45fb45429c28e6e56417281`

### Why

The original non-terrain scenes were stubs, so scene switching did not yet produce meaningful visuals. Implementing them as poster scenes fits the direction established by the target TERRAIN image: crisp silhouettes, ordered dithering, four semantic color indices, circular mask, and rotary-controlled motion.

The revision counter was needed because console commands mutate render state from the REPL task. With dirty-only rendering enabled, state changes must explicitly notify the app task that a repaint is needed.

### What worked

- The firmware builds cleanly after adding all five poster scenes and the new `pixel` command.
- The latest flashed build boots and reaches the `3d>` prompt.
- Scene-switch commands are accepted and log the selected scene.
- Pixel, contrast, and aperture commands are accepted and should repaint immediately because they call `render_params_touch()`.
- The Obsidian report was committed and pushed to the vault repo.

### What didn't work

- The current `fps` command still reports the old triangle-renderer stats, which are not meaningful for the poster renderer path.
- The scene tasks in docmgr still describe the original geometry-based implementation for terrain/torus/ocean/planet/tunnel. The firmware now has poster-scene implementations instead; the task text should be revised or separate poster-scene tasks should be added if this becomes the accepted direction.

### What I learned

- Once dirty redraw is enabled, every out-of-band state mutation needs an invalidation mechanism. A simple monotonically increasing revision counter is enough for this firmware.
- The poster renderer can cover the whole scene set with less code and less risk than reviving the triangle renderer immediately.
- Pixel-size control can be implemented without allocating a second low-resolution framebuffer by quantizing Bayer lookup coordinates and block-filling in the draw helper.

### What was tricky to build

The subtle issue was command-driven redraw. The console command handlers run independently from the render loop, so the app task would otherwise remain idle after commands such as `contrast 1.8` or `pixel 3`. The solution was to add `revision` to `render_params_t`, increment it from setters, and have the render loop compare it against a local `seen_revision`.

Pixel size also had to preserve the 2-bit memory budget. Allocating a separate low-resolution framebuffer would complicate the pipeline. The current block-fill approach changes the visual density pattern and apparent pixel scale while keeping the same packed framebuffer.

### What warrants a second pair of eyes

- The five scene implementations are art-directed poster scenes, not the original mesh-based scenes. Confirm that this is the desired product direction.
- The tunnel scene uses expensive per-pixel `atan2f`/`sqrtf`; if it feels slow on-device, it should be profiled and optimized with lookup tables or simplified math.
- `fps` should be updated for the poster path before using it for performance claims.

### What should be done in the future

- Add a framebuffer dump/screenshot command for serial PNG reconstruction.
- Add a `sensitivity` console command for encoder tuning.
- Add a `debug-ui` command to hide/show palette probe squares.
- Decide whether to update docmgr tasks to explicitly track poster-scene acceptance instead of original mesh-scene tasks.

### Code review instructions

- Start with `0096-m5dial-dithered-3d/main/terrain_poster.cpp` for all five scene renderers and the Bayer/contrast/pixel helpers.
- Review `0096-m5dial-dithered-3d/main/scene.h` and `scene.cpp` for the new `pixel_size` and `revision` fields.
- Review `0096-m5dial-dithered-3d/main/console_commands.cpp` for the `pixel` command and redraw invalidation calls.
- Review `0096-m5dial-dithered-3d/main/app_main.cpp` for `poster_render_scene()` integration and revision-based dirty redraw.
- Validate with:
  - `cd 0096-m5dial-dithered-3d`
  - `source /home/manuel/esp/esp-idf-5.4.2/export.sh`
  - `idf.py build`
  - `idf.py -p /dev/ttyACM0 flash monitor`
  - console smoke commands: `scene torus`, `pixel 3`, `contrast 1.8`, `scene ocean`, `scene planet`, `scene tunnel`, `aperture 90`.

### Technical details

Poster rendering now dispatches by scene id:

```cpp
poster_render_scene(ctx->fb, scene_current_id(), p);
```

Console redraw invalidation uses:

```cpp
void render_params_touch(void) {
    s_params.revision++;
}
```

and the app task observes:

```cpp
if (p->revision != seen_revision) {
    seen_revision = p->revision;
    dirty = true;
}
```

## Step 6: Poster Renderer Instrumentation, Screenshot Dump, Debug UI, and Encoder Sensitivity Command

The next implementation pass focused on making the firmware easier to tune from the real device. The poster renderer already displayed all five scenes, but the development loop still lacked three important tools: meaningful FPS stats for the poster path, a way to capture the framebuffer over serial, and runtime toggles for diagnostic overlays and encoder sensitivity.

I added those tools without changing the 2-bit framebuffer architecture. The firmware now records poster frame timing into the existing `fps` command, emits a complete packed-framebuffer dump through `dumpfb`, and has a host-side script that reconstructs the dump into a PNG without third-party Python dependencies.

### Prompt Context

**User prompt (verbatim):** "go ahead. keep a detailed diary as you work."

**Assistant interpretation:** Continue the firmware implementation and maintain chronological ticket documentation while working.

**Inferred user intent:** Keep progressing on the scene viewer while preserving enough detail for review, resumption, and future debugging.

**Commit (code):** Not committed yet.

### What I did

- Added `renderer_stats_record()` to `renderer.h/cpp` so the poster render path can update the existing stats object used by the `fps` command.
- Changed `app_main.cpp` to measure poster render + LCD scanline push time and record it as poster-mode frame time.
- Added `encoder_step` to `render_params_t`, preserving the current default of `2π / 12` radians per click.
- Changed encoder handling to use `p->encoder_step` instead of a hard-coded value.
- Added `debug_ui` to `render_params_t` and made palette probe squares conditional on that flag.
- Added console commands:
  - `sensitivity [clicks]` — sets clicks per full visual revolution, clamped to 2–96.
  - `debug [on|off]` — toggles diagnostic palette probes.
  - `dumpfb` — prints the current 2-bit framebuffer as 240 hex rows plus the active RGB565 palette.
- Registered the current framebuffer with the console command module using `console_commands_set_framebuffer()` after framebuffer allocation.
- Added host reconstruction script:
  - `ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/02-dumpfb-to-png.py`
- Rebuilt and flashed the firmware.
- Smoke-tested commands on-device through the tmux monitor:
  - `fps`
  - `sensitivity`
  - `sensitivity 24`
  - `debug off`
  - `debug on`
  - `dumpfb`
- Captured the `dumpfb` transcript and reconstructed a PNG artifact:
  - `ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/artifacts/latest-dumpfb.png`
- Asked a vision model to inspect the reconstructed PNG. It reported that the image plausibly matches the TERRAIN target: black circular field, red title/sun/UI, blue dithered terrain, white UI elements, no obvious bitplane or palette decoding errors.

### Why

The physical display is still the final authority, but a serial framebuffer dump gives a repeatable artifact that can be inspected, archived, compared, and converted to PNG. This is especially useful for debugging palette and bit-packing issues because it captures the exact framebuffer before LovyanGFX expands it to RGB565.

The `fps` command previously reported triangle-renderer stats, which were stale for the poster renderer. Recording poster frame time makes the console useful again for performance checks. Runtime sensitivity and debug toggles avoid reflashing for common tuning operations.

### What worked

- Build succeeded with ESP-IDF 5.4.2 after the instrumentation and console additions.
- Flash/monitor succeeded on `/dev/ttyACM0` after clearing the previous monitor process.
- `fps` now reports poster mode and a real frame time. In the smoke test it printed:

```text
Frame time: 31720 us (31.5 FPS)
Mode: poster
```

- `sensitivity` correctly reported the default and accepted a new value:

```text
Encoder sensitivity: 12.00 clicks/rev (0.5236 rad/click)
Encoder sensitivity: 24.00 clicks/rev (0.2618 rad/click)
```

- `debug off` and `debug on` both worked and trigger redraw through the revision mechanism.
- `dumpfb` emitted a full structured dump:

```text
DUMPFB_BEGIN width=240 height=240 bpp=2 bytes=14400 palette=CLASSIC
PALETTE 0000 F948 329F FFFF
ROW 000 ...
...
DUMPFB_END
```

- The host script produced a valid PNG:

```text
ttmp/.../artifacts/latest-dumpfb.png: PNG image data, 240 x 240, 8-bit/color RGB, non-interlaced
```

### What didn't work

- `dumpfb` is verbose: it prints 240 row lines plus headers. This is acceptable for occasional screenshots but not for high-frequency capture.
- The screenshot is a framebuffer capture, not a display readback. It validates the packed framebuffer and palette conversion, but it does not validate the GC9A01 physical transfer path after `setSwapBytes(true)`.

### What I learned

- The packed framebuffer format is easy to export because each row is exactly `FB_WIDTH / 4 = 60` bytes. A complete screenshot is only 14,400 bytes before hex encoding.
- A dependency-free PNG writer is small enough to keep in the ticket scripts. It avoids relying on Pillow or ImageMagick on whatever host later processes the dump.
- The reconstructed TERRAIN image is a strong validation point: the palette indices, bit order, circular mask, title, sun, terrain field, and bottom UI are all present in the pre-display framebuffer.

### What was tricky to build

The main design choice was where to put screenshot state. Console commands are registered independently from `AppContext`, so the command module had no direct pointer to the framebuffer. I added `console_commands_set_framebuffer(const uint8_t*)` and call it immediately after `fb_init()` in `app_main.cpp`. This avoids global ownership of the framebuffer allocation while still making the active buffer visible to `dumpfb`.

The other tricky part was keeping PNG reconstruction dependency-free. The script parses the firmware's text dump, unpacks four 2-bit pixels per byte using the same low-bit slot order as `fb_set()`, converts RGB565 palette entries to RGB888, and writes a minimal PNG by constructing `IHDR`, `IDAT`, and `IEND` chunks with CRCs and zlib compression.

### What warrants a second pair of eyes

- `dumpfb` reads the framebuffer without locking against the render task. With dirty rendering this is usually stable, but auto-rotate or encoder input during a dump could capture a partially updated frame. A future version should pause rendering or take a small copy before printing.
- The current `fps` measurement includes poster rendering plus LCD transfer. That is useful for user-visible frame cost, but if we need CPU-only render cost we should record render and display times separately.
- `debug_ui` currently controls palette probes, but some UI elements are still always visible. Decide later whether there should be a cleaner `ui off` mode.

### What should be done in the future

- Add a host helper that talks to the serial port directly, sends `dumpfb`, waits for `DUMPFB_END`, and writes a PNG in one command.
- Add optional RLE or base64 binary mode for faster screenshot transfer.
- Split `fps` into `render_us`, `display_us`, and `total_us` if performance tuning becomes important.
- Tune each poster scene using captured PNGs and physical photos.

### Code review instructions

- Review `0096-m5dial-dithered-3d/main/console_commands.cpp` for `dumpfb`, `sensitivity`, and `debug` command implementations.
- Review `0096-m5dial-dithered-3d/main/app_main.cpp` for framebuffer registration, encoder-step usage, and poster stats recording.
- Review `0096-m5dial-dithered-3d/main/scene.h` and `scene.cpp` for new render parameters.
- Review `0096-m5dial-dithered-3d/main/renderer.h/cpp` for `renderer_stats_record()`.
- Review `ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/02-dumpfb-to-png.py` for dump parsing and PNG encoding.
- Validate with:
  - `idf.py build`
  - `idf.py -p /dev/ttyACM0 flash monitor`
  - `fps`
  - `sensitivity 12`
  - `debug off`
  - `debug on`
  - `dumpfb`
  - `python3 scripts/02-dumpfb-to-png.py dump.txt screenshot.png`

### Technical details

Firmware dump format:

```text
DUMPFB_BEGIN width=240 height=240 bpp=2 bytes=14400 palette=CLASSIC
PALETTE 0000 F948 329F FFFF
ROW 000 <60 bytes as 120 hex chars>
...
ROW 239 <60 bytes as 120 hex chars>
DUMPFB_END
```

Host-side unpacking mirrors the firmware bit layout:

```python
packed = row[x >> 2]
color_index = (packed >> ((x & 3) * 2)) & 0x03
```

RGB565 expansion in the host script uses bit replication:

```python
r = (r5 << 3) | (r5 >> 2)
g = (g6 << 2) | (g6 >> 4)
b = (b5 << 3) | (b5 >> 2)
```

## Step 7: Automated Serial Capture, Scene Review, UI Safe Areas, and Solid Text

The next pass turned the one-off framebuffer dump into a repeatable capture workflow and used it to review all five scenes. The key improvement is `03-capture-dumpfb.py`: it opens `/dev/ttyACM0`, waits for the `3d>` console prompt, sends setup commands, runs `dumpfb`, extracts the framebuffer block, and writes PNG files through the existing dump decoder.

After reviewing the captured PNGs, I made the UI layer more explicit. Scene artwork remains dithered and pixel-blocked, but text is now drawn as solid pixels, independent of the poster pixel size. I also added black top and bottom safe areas before drawing titles and status UI so dense scene dithering does not destroy readability.

### Prompt Context

**User prompt (verbatim):** "do it"

**Assistant interpretation:** Implement the next step from the previous summary: a host helper that drives `dumpfb` and saves PNGs automatically.

**Inferred user intent:** Make framebuffer screenshots a repeatable workflow rather than a manual tmux capture and conversion process.

**User prompt (verbatim):** "you can use read() to look at the images too"

**Assistant interpretation:** Use the local image-capable `read` tool for direct inspection of generated PNG artifacts.

**Inferred user intent:** Inspect screenshots locally as part of the iteration loop instead of relying only on external vision checks.

**User prompt (verbatim):** "you shouldn't dither the text if that's possible. just the 3d rendering."

**Assistant interpretation:** Separate scene rendering effects from UI rendering: ordered dithering and pixel-size block rendering should apply to scene artwork only, not text or status UI.

**Inferred user intent:** Keep the UI readable and crisp while preserving the dithered aesthetic for the graphics.

**User prompt (verbatim):** "commit at appropriate intervals as well, btw and keep a detailed diary"

**Assistant interpretation:** Commit coherent milestones and continue recording detailed chronological work notes.

**Inferred user intent:** Avoid losing work and keep the project reviewable/resumable.

**Commit (code/docs):** Pending in this step until staging review.

### What I did

- Added `scripts/03-capture-dumpfb.py`, a pyserial-based host capture helper.
- First attempted a lower-level `termios`/raw-file-descriptor serial implementation. It opened the port but captured no console output after the board reset:

```text
TimeoutError: timed out waiting for b'DUMPFB_END'; captured 0 bytes
```

- Verified with a small pyserial smoke test that opening `/dev/ttyACM0` resets the USB Serial/JTAG path and produces boot logs; switched the helper to pyserial and prompt-based synchronization.
- Implemented all-scenes capture:

```bash
python3 ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/03-capture-dumpfb.py \
  --port /dev/ttyACM0 \
  --all-scenes \
  --setup "debug off" \
  --setup "pixel 2" \
  --setup "contrast 1.6" \
  --out-dir ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/artifacts/scene-captures
```

- Captured PNG/TXT pairs for terrain, torus, ocean, planet, and tunnel.
- Used `read` on the generated PNG files to inspect the images locally.
- Applied composition/readability fixes in `terrain_poster.cpp`:
  - added `clear_rect_visible()` for top title and bottom status safe areas
  - lowered the ocean horizon so the title has room
  - reduced tunnel outer radius so it keeps more black border
  - reduced terrain height so it does not collide as much with the UI
  - added `set_solid_if_visible()` and changed `draw_text()` to use it instead of pixel-blocked `set_if_visible()`
- Rebuilt and flashed the firmware after the UI/text changes.
- Re-captured all scenes to `artifacts/scene-captures-v2/` and inspected them with `read`.

### Why

The manual screenshot workflow worked, but it depended on tmux capture and post-processing by hand. A real host helper makes visual iteration reproducible: send setup commands, capture the framebuffer, write PNGs, inspect the result, tune code, repeat.

Text readability needed a separate rule from scene rendering. The scene should be dithered; the UI should communicate. Applying pixel-size block filling to glyphs made text less reliable, especially at `pixel 2` and above. Solid glyph drawing keeps titles and status labels legible even when the scene uses coarse pixels.

### What worked

- The pyserial capture helper works end-to-end. It captured all five scenes automatically and wrote PNGs.
- Opening the serial port may reset the board, but prompt synchronization handles that by waiting for `3d>` before sending commands.
- `read` can inspect local PNG artifacts directly, which makes fast visual checks possible inside the coding loop.
- Solid text and safe-area clearing improved readability. The second capture set shows readable titles/status UI for terrain, ocean, planet, and tunnel; torus title remains small/faint but is no longer dithered by the scene path.

### What didn't work

- The raw `termios` helper failed to receive output after the port open/reset sequence:

```text
TimeoutError: timed out waiting for b'DUMPFB_END'; captured 0 bytes
```

- The first all-scene capture showed composition problems:
  - bottom UI overlapped terrain/water/tunnel patterns
  - ocean and tunnel needed stronger safe areas
  - title placement needed consistent clearing
- The `dumpfb` capture still needs exclusive serial ownership. It cannot run while `idf.py monitor` owns `/dev/ttyACM0`.

### What I learned

- For USB Serial/JTAG, pyserial's normal `Serial(..., timeout=...)` path with explicit DTR/RTS levels behaved better than a minimal raw `termios` implementation.
- The screenshot workflow should expect a reset on port open and wait for the firmware prompt rather than assuming the console is already alive.
- UI drawing needs its own rendering policy. Dither and pixel-size controls are scene effects; titles and labels should be solid unless explicitly styled otherwise.

### What was tricky to build

Serial synchronization was the sharp edge. The host script must handle two states: an already-running console and a freshly reset board that emits boot logs before the prompt. The robust sequence is: open port, set DTR/RTS, send a newline, wait for `3d>`, then send setup commands one at a time and wait for the prompt after each command. `dumpfb` is the exception: it waits for `DUMPFB_END` first because the dump is long, then consumes the trailing prompt opportunistically.

The UI rendering fix also needed care because the existing `set_if_visible()` helper intentionally applies poster pixel size. Reusing it for text would keep the bug. The solution was a separate `set_solid_if_visible()` helper for text and UI glyph pixels.

### What warrants a second pair of eyes

- Torus title readability is still weaker than the other scenes. It may need a larger font, different y-position, or white highlight/shadow.
- The bottom UI safe rectangle is currently a black box. It is readable but visually blunt; later polish could make it a deliberate panel shape.
- The host capture helper depends on pyserial. This is acceptable on this machine, but the script should mention that dependency if moved elsewhere.

### What should be done in the future

- Add an optional `--no-reset` or lower-level mode only if needed; current reset-tolerant pyserial mode is simpler.
- Add a montage generator for all five captured PNGs.
- Tune titles and bottom UI per scene, especially torus.
- Decide whether to commit small PNG artifacts or keep them as local/generated evidence only.

### Code review instructions

- Review `ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/03-capture-dumpfb.py` for serial synchronization and all-scene capture.
- Review `0096-m5dial-dithered-3d/main/terrain_poster.cpp` for safe-area clearing and solid text drawing.
- Validate with:
  - `idf.py build`
  - `idf.py -p /dev/ttyACM0 flash`
  - `python3 .../scripts/03-capture-dumpfb.py --port /dev/ttyACM0 --all-scenes --setup "debug off" --setup "pixel 2" --setup "contrast 1.6" --out-dir .../artifacts/scene-captures-v2`
  - inspect generated PNGs with `read` or another image viewer.

### Technical details

The host capture script waits for the prompt before sending commands:

```python
con.wait_for_prompt(args.prompt_timeout)
```

and captures one scene by sending setup commands followed by `dumpfb`:

```python
for command in setup:
    con.run_command_wait_prompt(command, command_timeout)
con.write_command("dumpfb")
raw = con.read_until(b"DUMPFB_END", dump_timeout)
```

Text now bypasses poster pixel-size block filling:

```cpp
set_solid_if_visible(fb, cursor + gx * scale + xx,
                     y + gy * scale + yy, color, p);
```
