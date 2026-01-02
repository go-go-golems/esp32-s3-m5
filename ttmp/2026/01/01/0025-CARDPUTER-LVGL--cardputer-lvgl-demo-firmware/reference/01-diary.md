---
Title: Diary
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.cursor/commands/diary.md
      Note: Diary format and required sections
    - Path: ../../../../../../../../../../.cursor/commands/docmgr.md
      Note: Workflow for creating ticket/docs/tasks
    - Path: 0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Key event decoding decisions referenced in Step 1
    - Path: 0025-cardputer-lvgl-demo/build.sh
      Note: Step 3 flash workflow + tmux monitor guidance
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: Step 2 UI loop + stable UiState callbacks
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: Step 2 display flush + tick timer
    - Path: 0025-cardputer-lvgl-demo/sdkconfig.defaults
      Note: Step 2 LVGL Kconfig defaults (disable example build)
    - Path: imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp
      Note: Flush/tick/handler reference used in Step 1
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T22:49:10.13641006-05:00
WhatFor: ""
WhenToUse: ""
---




# Diary

## Goal

Capture the research and implementation journey for building a Cardputer LVGL demo firmware: what I looked at, what I changed, why, what failed, and what to validate.

## Context

This ticket builds an ESP-IDF firmware project for M5Stack Cardputer that integrates LVGL using M5GFX as the display backend and the `cardputer_kb` scanner (plus the existing `CardputerKeyboard` wrapper from `0022-cardputer-m5gfx-demo-suite`) as the input source.

## Step 1: Research the existing “known-good” Cardputer setup and LVGL port patterns

I started by identifying the smallest set of existing code in this repo that already solves the hard parts we need (Cardputer display bring-up, keyboard scanning, and an LVGL<->M5GFX flush path). The key decision from this step is to reuse the project/app scaffold from `0022-cardputer-m5gfx-demo-suite` and adapt the LVGL emulator port’s display flush approach to ESP-IDF.

The biggest uncertainty going in is the RGB565 byte order: LVGL and LovyanGFX both support swapped/non-swapped pixel order, and getting that wrong yields “working but wrong colors”. The plan is to implement the simplest correct flush path first, then adjust `LV_COLOR_16_SWAP` / `setSwapBytes()` if needed.

### What I did
- Read `~/.cursor/commands/docmgr.md` and `~/.cursor/commands/diary.md` to follow the ticket+diary workflow.
- Read the Cardputer demo suite scaffold (`0022-cardputer-m5gfx-demo-suite`) to copy its ESP-IDF project wiring patterns.
- Read the LVGL emulator port (`imports/lv_m5_emulator`) to reuse its flush/tick/handler patterns conceptually.
- Reviewed the shared Cardputer keyboard component (`components/cardputer_kb`) and how `CardputerKeyboard` turns scans into semantic events.

### Why
- Avoid reinventing bring-up code that already works in this repo.
- Minimize scope: build the smallest LVGL port that can render and accept keyboard input.

### What worked
- Found a clean “Cardputer baseline” in `0022-cardputer-m5gfx-demo-suite`:
  - `m5gfx::M5GFX display; display.init();`
  - `CardputerKeyboard keyboard; keyboard.init();`
- Found a directly relevant LVGL flush implementation that targets `M5GFX` in the emulator import.

### What didn't work
- N/A (research-only step)

### What I learned
- The repo already has a good separation of concerns for keyboard:
  - `components/cardputer_kb/*` = stable physical scan + action decoding
  - `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.*` = “event stream” suitable for app/UI layers

### What was tricky to build
- LVGL’s input model expects a polling callback (`read_cb`) while the Cardputer keyboard layer emits “edge events”. This requires a small queue/adapter between the two.

### What warrants a second pair of eyes
- The chosen mapping from `KeyEvent.key` strings to LVGL keycodes (to ensure we preserve normal text entry while enabling navigation).
- The display flush pixel format assumptions (RGB565 swap correctness).

### What should be done in the future
- Once the baseline works, consider adding a small note/playbook on how to debug color swap issues (photos + the specific knob that fixed it).

### Code review instructions
- Start with the analysis plan: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md`
- Then review the scaffold we’re reusing: `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`
- Then review the LVGL port pattern reference: `esp32-s3-m5/imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp`

### Technical details
- Key files referenced in this step:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/CMakeLists.txt`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
  - `esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp`
  - `esp32-s3-m5/imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp`

### What I'd do differently next time
- N/A

## Step 2: Scaffold the ESP-IDF project and implement a minimal LVGL UI loop

This step created the actual firmware project (`0025-cardputer-lvgl-demo`) and implemented the smallest LVGL “happy path”: bring up M5GFX, register an LVGL display driver that flushes to M5GFX, register a keyboard indev, and render a simple screen with a textarea and a button.

The key correctness fix in this step was ensuring that LVGL callback `user_data` points at a stable object. LVGL stores pointers verbatim; passing the address of a stack local in `create_demo_ui()` would have been a use-after-return bug. The current implementation uses a static `UiState` in `app_main` and passes its address into LVGL callbacks/timers.

### What I did
- Added a new ESP-IDF project at `esp32-s3-m5/0025-cardputer-lvgl-demo/` with the same “EXTRA_COMPONENT_DIRS” setup as `0022-cardputer-m5gfx-demo-suite` (M5GFX + cardputer_kb).
- Added an ESP-IDF Component Manager dependency on `lvgl/lvgl` via `main/idf_component.yml`.
- Implemented:
  - `lvgl_port_m5gfx` (draw buffers + flush callback + esp_timer tick)
  - `lvgl_port_cardputer_kb` (KEYPAD indev backed by a small key queue)
  - `app_main` UI loop with a textarea + “Clear” button + status label
- Ran a local build to confirm the toolchain and component dependency resolution.

### Why
- Establish a reusable baseline integration that future Cardputer UI work can build on.
- Make the first demo prove the integration end-to-end: draw + input + periodic updates.

### What worked
- `./build.sh build` successfully fetched and built LVGL and produced `build/cardputer_lvgl_demo.bin`.
- The LVGL managed component resolved to `lvgl/lvgl (8.4.0)` with the current semver constraint.

### What didn't work
- First build compiled a large set of LVGL examples (`CONFIG_LV_BUILD_EXAMPLES=y`), which massively increases compile time for a tiny demo. I added a default override in `sdkconfig.defaults` to disable it going forward.

### What I learned
- The ESP-IDF `lvgl/lvgl` managed component defaults to `CONFIG_LV_CONF_SKIP=y` (it builds without requiring a project-provided `lv_conf.h`).
- For LVGL integration, pointer lifetime is a real footgun: callback `user_data` needs a stable address for the lifetime of the objects/timers.

### What was tricky to build
- Choosing an input model that preserves normal typing while still providing navigation: the adapter uses “semantic” actions (`up/down/...`) when the bindings decoder emits them, and otherwise forwards single-character keys as ASCII.

### What warrants a second pair of eyes
- RGB565 swap correctness: the integration currently defaults `swap_bytes=false` and relies on `M5GFX::setSwapBytes(false)`. If colors are wrong on-device, we should flip the appropriate knob and document it.
- Memory choices for draw buffers (DMA heap vs normal heap) and whether PSRAM should be used.

### What should be done in the future
- Add a short “color swap debugging” note after the first on-device run (photo + setting used).

### Code review instructions
- Start in `esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp`
- Then review the display port in `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp`
- Then review key mapping/queue behavior in `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp`
- Build: `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh build`

### Technical details
- `sdkconfig.defaults` sets `# CONFIG_LV_BUILD_EXAMPLES is not set` and increases `CONFIG_LV_MEM_SIZE_KILOBYTES` to reduce compile time and avoid small-heap surprises.

## Step 3: Flash smoke test on hardware

This step validated the firmware on a real Cardputer by flashing it over the USB-Serial/JTAG port. The demo screen appears usable: text entry works in the textarea and the UI is responsive enough for an initial baseline.

I didn’t capture a full interactive `idf.py monitor` session from this CLI environment (it requires an attached TTY). For future debugging, the project’s `./build.sh tmux-flash-monitor` target is the intended workflow because it runs monitor in a proper terminal.

### What I did
- Flashed the firmware:
  - `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash`
- Verified the UI behavior directly on device (manual).

### Why
- Confirm the display flush, tick, and keyboard indev are working end-to-end on real hardware.

### What worked
- Flash succeeded and the demo appears to work well on device (user-reported).

### What didn't work
- Running `idf.py monitor` from this harness fails due to missing stdin TTY (“Monitor requires standard input to be attached to TTY”).

### What I learned
- This repo’s `build.sh tmux-flash-monitor` pattern is the right way to run a stable flash+monitor workflow for device work.

### What was tricky to build
- N/A (validation step)

### What warrants a second pair of eyes
- Confirm on-device colors are correct (RGB565 swap). If not, we should decide whether to change `swap_bytes` (M5GFX) or LVGL’s swap setting and document it.

### What should be done in the future
- Run a monitor session in a real terminal and capture the boot log + any LVGL warnings into the diary for future regressions.

### Code review instructions
- Flash+monitor (recommended): `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh tmux-flash-monitor`

## Quick Reference

### LVGL key mapping (proposed)

`KeyEvent.key` → `lv_indev_data_t.key`

- `"up"` → `LV_KEY_UP`
- `"down"` → `LV_KEY_DOWN`
- `"left"` → `LV_KEY_LEFT`
- `"right"` → `LV_KEY_RIGHT`
- `"enter"` → `LV_KEY_ENTER`
- `"tab"` → `LV_KEY_NEXT`
- `"esc"` → `LV_KEY_ESC`
- `"bksp"` → `LV_KEY_BACKSPACE`
- `"space"` → `' '`
- Single-character strings (e.g. `"a"`, `"A"`, `"/"`) → ASCII codepoint of the first char

### LVGL display flush (shape)

```c++
static void flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
  M5GFX& display = *static_cast<M5GFX*>(drv->user_data);
  int w = area->x2 - area->x1 + 1;
  int h = area->y2 - area->y1 + 1;
  uint32_t pixels = w * h;

  display.startWrite();
  display.setAddrWindow(area->x1, area->y1, w, h);
  display.writePixels(reinterpret_cast<lgfx::rgb565_t*>(color_p), pixels);
  display.endWrite();

  lv_disp_flush_ready(drv);
}
```

## Usage Examples

- Validate input quickly:
  - Boot the firmware, focus the textarea, press `a`, verify it appears.
  - Press `Enter` on the button, verify the bound action runs (e.g. clear textarea).

## Related

- Ticket index: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/index.md`
- Analysis plan: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md`
