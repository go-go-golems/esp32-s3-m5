---
Title: 'LVGL on Cardputer (ESP-IDF + M5GFX): integration plan'
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: Concrete implementation of the plan (demo UI + main loop)
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp
      Note: Key mapping and LVGL indev queue implementation
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: Concrete LVGL->M5GFX flush and tick implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T22:49:06.947866841-05:00
WhatFor: ""
WhenToUse: ""
---


# LVGL on Cardputer (ESP-IDF + M5GFX): integration plan

This document is the “build it right the first time” plan for a small M5Stack Cardputer firmware that uses LVGL for UI and uses M5GFX as the display backend. The goal is not a full app framework; it’s a clean baseline integration that we can reuse in future Cardputer projects.

The starting point is the app/project setup used by `0022-cardputer-m5gfx-demo-suite` (ESP-IDF project layout, M5GFX autodetect bring-up, and the reusable Cardputer keyboard scanner).

## Goals

- Produce an ESP-IDF firmware project that boots on Cardputer and renders a simple LVGL screen.
- Provide a minimal-but-correct LVGL port:
  - tick source (`lv_tick_inc`)
  - periodic handler (`lv_timer_handler`)
  - RGB565 display flush to M5GFX
  - keyboard input as LVGL `KEYPAD` indev (nav + text entry)
- Keep the first demo screen small and obviously working (something you can type into, and a button you can activate).

## Non-goals (for this ticket)

- Touch input (Cardputer is keyboard-first).
- Full theming / custom fonts / large demo suite.
- Adding SDL-based emulator support for this particular firmware (we already have a separate emulator import; firmware should remain device-first).

## Reference implementation starting points (files to read first)

### Reuse the Cardputer ESP-IDF “app setup”

The project structure and “bring-up” should mirror this ticket’s scaffold patterns:

- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/CMakeLists.txt` (how Cardputer projects wire `M5GFX` + `cardputer_kb` via `EXTRA_COMPONENT_DIRS`)
- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/CMakeLists.txt` (explicit `REQUIRES` / `PRIV_REQUIRES` in ESP-IDF)
- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/sdkconfig.defaults` (sane Cardputer defaults: 8MB flash, CPU 240MHz, bigger main stack)
- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` (display autodetect + keyboard init + “single main loop” style)

### Reuse the keyboard scanner + decoding conventions

We should treat this as the canonical “what keys mean” layer:

- `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h` (scan API)
- `esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp` (pins, scan timing, physical key positions)
- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.h`
- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` (action chords → nav keys; raw edge events → text)

### LVGL + M5GFX “flush” patterns to adapt

The closest existing example in this repo is the PC emulator port; the device build needs the same conceptual porting but with ESP-IDF primitives:

- `esp32-s3-m5/imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.hpp`
- `esp32-s3-m5/imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp`
- `esp32-s3-m5/imports/lv_m5_emulator/src/user_app.cpp` (example screen/demos)

## Proposed firmware structure (new project)

Create a new ESP-IDF project directory:

- `esp32-s3-m5/0025-cardputer-lvgl-demo/`
  - `CMakeLists.txt` (same `EXTRA_COMPONENT_DIRS` pattern as `0022-*`)
  - `sdkconfig.defaults`, `partitions.csv`, `README.md`
  - `main/`
    - `app_main.cpp`
    - `lvgl_port_m5gfx.{h,cpp}` (display + tick + handler loop helpers)
    - `lvgl_port_cardputer_kb.{h,cpp}` (LVGL keypad indev fed by `CardputerKeyboard`)
    - `lv_conf.h` or a project-local config approach compatible with the chosen LVGL component
    - `main/idf_component.yml` (declare LVGL dependency via IDF Component Manager)

## Key design decisions

### LVGL version and API surface

- Prefer LVGL v8 for now (stable ecosystem; many existing ports and examples).
- Use the v8 driver structs (`lv_disp_drv_t`, `lv_disp_draw_buf_t`, `lv_indev_drv_t`).

### Color format

- Use RGB565 end-to-end:
  - M5GFX display is configured to 16-bit (`display.setColorDepth(16)`).
  - LVGL uses `LV_COLOR_DEPTH=16`.
- Risk: byte order / swap. Be prepared to toggle one of:
  - `LV_COLOR_16_SWAP` (LVGL-side)
  - `display.setSwapBytes(true/false)` (LovyanGFX-side)

### Timing model (simple + robust)

- Use `esp_timer` as a monotonic tick:
  - periodic callback: `lv_tick_inc(kTickMs)`
- Run LVGL on a single FreeRTOS task (avoid cross-thread LVGL calls).
- In that task:
  - poll keyboard and push key events into a small queue
  - call `lv_timer_handler()`
  - sleep ~5–10ms

### Display flushing (M5GFX)

Implement an LVGL flush callback that:

- Calls `display.startWrite()`
- Calls `display.setAddrWindow(x, y, w, h)`
- Writes pixel data in chunks via `display.writePixels((lgfx::rgb565_t*)px, pixels)`
- Calls `display.endWrite()`
- Calls `lv_disp_flush_ready(drv)`

Chunking is primarily a safety/perf tuning knob; the emulator port uses it to avoid a SIMD copy path on some platforms. Keeping it on-device is fine if it’s not overly complex.

### Keyboard input (LVGL keypad indev)

Use LVGL `LV_INDEV_TYPE_KEYPAD`:

- Map semantic navigation actions (from `CardputerKeyboard::poll()`):
  - `"up"|"down"|"left"|"right"` → `LV_KEY_UP/DOWN/LEFT/RIGHT`
  - `"enter"` → `LV_KEY_ENTER`
  - `"tab"` → `LV_KEY_NEXT` (or `'\t'` if that works better)
  - `"esc"` → `LV_KEY_ESC`
  - `"bksp"` → `LV_KEY_BACKSPACE`
  - `"space"` → `' '`
- For raw single-character keys:
  - send the ASCII code (e.g. `'a'`, `'A'`, `'/'`, etc.)

This allows:
- text entry into a focused widget (e.g. a `lv_textarea`)
- keyboard navigation between focusable widgets

## Minimal demo screen (first milestone)

The first UI should prove:
- display flushing works (visible widgets)
- tick/handler works (animations/cursor blink)
- keyboard works (text entry + button activation)

Proposed screen:

- Title bar label: “Cardputer LVGL Demo”
- `lv_textarea` focused by default (“Type here…”)
- A button “Clear” that clears the textarea (Enter activates)
- A small status label (heap free + last key code), updated periodically

## Pseudocode sketch

```c++
extern "C" void app_main() {
  m5gfx::M5GFX display;
  display.init();
  display.setColorDepth(16);

  CardputerKeyboard kb;
  kb.init();

  lvgl_port_init_display(display);         // registers flush, draw buffers
  lvgl_port_init_tick(2 /*ms*/);           // esp_timer -> lv_tick_inc
  lvgl_port_init_keyboard(kb);             // indev keypad -> queue

  create_demo_screen();                    // lv_label + lv_textarea + lv_btn

  while (true) {
    for (auto& ev : kb.poll()) {
      lvgl_kb_feed(ev);                    // map KeyEvent -> LVGL key code
    }
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
```

## Used APIs (by subsystem)

### ESP-IDF

- `esp_timer_create`, `esp_timer_start_periodic`
- `esp_timer_get_time` (optional for perf stats)
- FreeRTOS: `vTaskDelay`, `pdMS_TO_TICKS`, task creation if needed
- Memory alloc (optional): `heap_caps_malloc`, `heap_caps_get_free_size`

### M5GFX / LovyanGFX (ESP-IDF component)

- `m5gfx::M5GFX::init()`
- `m5gfx::M5GFX::setColorDepth(int)`
- `m5gfx::M5GFX::width()`, `height()`
- `m5gfx::M5GFX::startWrite()`, `endWrite()`
- `m5gfx::M5GFX::setAddrWindow(int x, int y, int w, int h)`
- `m5gfx::M5GFX::writePixels(const lgfx::rgb565_t* data, uint32_t len)`
- Possibly: `m5gfx::M5GFX::setSwapBytes(bool)` (if color swap is wrong)

### LVGL (v8)

- Core: `lv_init`, `lv_tick_inc`, `lv_timer_handler`
- Display: `lv_disp_draw_buf_init`, `lv_disp_drv_init`, `lv_disp_drv_register`, `lv_disp_flush_ready`
- Input: `lv_indev_drv_init`, `lv_indev_drv_register`, `LV_INDEV_TYPE_KEYPAD`
- Widgets: `lv_scr_act`, `lv_label_create`, `lv_textarea_create`, `lv_btn_create`
- Focus (optional): `lv_group_create`, `lv_group_set_default`, `lv_indev_set_group`

### Cardputer keyboard layer

- `CardputerKeyboard::init()`, `CardputerKeyboard::poll()` (from `0022-*`)
- `cardputer_kb::MatrixScanner` (low-level scan)
- `cardputer_kb::decode_best(...)` (action chords like arrows)

## Validation plan

- Build: `idf.py set-target esp32s3 && idf.py build`
- Flash: `idf.py flash monitor`
- On device:
  - LVGL widgets render with correct colors
  - typing shows up in the textarea
  - Enter triggers button action (clear)
  - Fn/chord navigation (if present) moves focus/cursor as expected

## Risks / sharp edges to watch

- Color swap / endianness (RGB565 vs panel expectations).
- LVGL memory usage: draw buffers + `LV_MEM_SIZE` need to fit.
- Flush performance: too-small buffers cause visible tearing / low FPS.
- Keyboard mapping: avoid stealing normal letters for navigation.
