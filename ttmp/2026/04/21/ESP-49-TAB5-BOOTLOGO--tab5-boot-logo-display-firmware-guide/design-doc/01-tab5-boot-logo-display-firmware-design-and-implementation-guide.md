---
Title: Tab5 boot logo display firmware design and implementation guide
Ticket: ESP-49-TAB5-BOOTLOGO
Status: active
Topics:
  - firmware
  - display
  - lvgl
  - mipidsi
  - boot
  - esp-idf
  - m5stack
  - tab5
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
  - Path: M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp
    Note: The factory HAL showing the complete Tab5 subsystem initialization sequence
  - Path: M5Tab5-UserDemo/platforms/tab5/main/hal/components/hal_wifi.cpp
    Note: The factory Wi-Fi HAL example showing the AP+HTTP bring-up pattern
  - Path: M5Tab5-UserDemo/app/apps/app_startup_anim/app_startup_anim.cpp
    Note: The factory startup animation app showing how LVGL images are created and animated
  - Path: M5Tab5-UserDemo/app/assets/assets.h
    Note: The LVGL image declarations used in the factory app
  - Path: M5Tab5-UserDemo/app/assets/images/logo_tab.c
    Note: The 180×80 RGB565 logo image array (173 KB) declared with LV_IMG_DECLARE
  - Path: M5Tab5-UserDemo/platforms/tab5/sdkconfig.defaults
    Note: The Tab5 target defaults including CONFIG_BSP_LCD_COLOR_FORMAT_RGB888 and PSRAM settings
  - Path: M5Tab5-UserDemo/platforms/tab5/components/espressif__esp_lvgl_port/include/esp_lvgl_port.h
    Note: The esp_lvgl_port public API header showing lvgl_port_init, lock/unlock, and display add functions
  - Path: M5Tab5-UserDemo/platforms/tab5/components/espressif__esp_lvgl_port/include/esp_lvgl_port_disp.h
    Note: The display configuration structure with DSI support, buffer sizing, and flag options
  - Path: esp32-s3-m5/0050-tab5-web-text-echo/sdkconfig.defaults
    Note: The existing Tab5 Wi-Fi + ESP-Hosted SDK defaults used as a base for the boot logo demo
  - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/wifi_app.c
    Note: The Tab5 Wi-Fi bring-up and NVS credential persistence used as the STA+AP bring-up pattern
ExternalSources:
  - https://docs.m5stack.com/en/core/Tab5
  - https://docs.m5stack.com/en/esp_idf/m5tab5/userdemo
  - https://components.espressif.com/components/espressif/esp_lvgl_port
  - https://components.espressif.com/components/espressif/esp_lcd_panel
Summary: A minimal boot logo display demo for the Tab5 that initializes the 5-inch 720P MIPI DSI display, renders the factory logo, and pairs it with the existing Wi-Fi + HTTP server from the Tab5 text echo demo.
LastUpdated: 2026-04-21T19:00:00Z
WhatFor: Use this design when you want to understand the Tab5 display bring-up path, LVGL integration with the P4's MIPI DSI interface, and how to add a persistent boot logo to a Tab5 firmware.
WhenToUse: Use before implementing the Tab5 display demo or when reviewing the LVGL/DSI initialization with a new engineer.
---

# Tab5 boot logo display firmware design and implementation guide

## Executive Summary

This guide covers how to build a minimal Tab5 boot logo display firmware. The device boots, initializes its 5-inch 720×1280 MIPI DSI display, renders the factory M5 logo image on screen, starts the Wi-Fi SoftAP and HTTP server from the text echo demo, and keeps the logo visible on the display while the network services run in the background.

The core engineering insight is that the Tab5 display cannot be driven with plain `esp_lcd` calls in isolation. It requires a layered stack: the P4's native MIPI DSI hardware → the panel IO → the ST7123 panel driver → `esp_lvgl_port` → LVGL → the application. Each layer in that stack has its own initialization contract that must be satisfied before the next layer is called. Skipping any layer produces a compile error or a black screen.

The guide assumes familiarity with the Tab5 hardware architecture from the earlier text echo demo (`ESP-48`). This document focuses exclusively on the display path and how it differs from a network-only demo.

## Problem Statement and Scope

The Tab5 has a 5-inch 720P portrait display that is physically prominent and visible on power-on. A boot logo firmware answers the question: "what is the minimal correct code to put something on that screen?" It is also the prerequisite for any firmware that wants to display status information, a UI, or diagnostics.

The scope for this ticket is intentionally narrow:

- **In scope**
  - Display subsystem initialization using the P4's MIPI DSI interface
  - LVGL integration via `esp_lvgl_port`
  - Rendering an existing LVGL image on screen
  - Keeping the logo visible while Wi-Fi and HTTP run in the background
  - A clear intern-friendly guide explaining every layer of the display stack

- **Out of scope**
  - Touch panel initialization
  - Display rotation or animation
  - Custom logo generation
  - LVGL widgets or UI elements beyond a static image
  - Audio or camera integration

## Current State Analysis

### The display is physically dominant on the Tab5

The Tab5 has a 5-inch IPS TFT display at 720×1280 portrait resolution. It dominates the board's face. Any firmware that ignores the display is ignoring the most visually distinctive feature of the hardware. A boot logo is the simplest useful thing to put on it.

### The factory firmware shows the complete display bring-up path

The factory Tab5 firmware initializes the display through several distinct layers that must be called in the right order:

```text
bsp_display_start_with_config(&cfg)
  └─> esp_lcd_new_panel()           ← P4 MIPI DSI hardware + ST7123 driver
  └─> esp_lcd_panel_io_dsi_init()   ← Panel IO handle for DSI commands
  └─> esp_lvgl_port_add_disp_dsi()  ← registers display with LVGL
  └─> lvgl_port_init()              ← starts LVGL task and timer
  └─> lv_display_set_rotation()      ← applies 90° rotation for landscape
  └─> bsp_display_backlight_on()     ← enables the backlight LED via GPIO
```

This sequence is documented in `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp` in the `HalEsp32::init()` method. Understanding this sequence is the prerequisite for writing any Tab5 display code.

### The logo is a LVGL image declared with `LV_IMG_DECLARE`

The factory app declares its logo assets in `M5Tab5-UserDemo/app/assets/assets.h`:

```cpp
LV_IMG_DECLARE(launcher_bg);
LV_IMG_DECLARE(sw_chg_off);
LV_IMG_DECLARE(logo_tab);
LV_IMG_DECLARE(logo_5);
```

The actual image data lives in `logo_tab.c` (180×80 pixels, RGB565, 28800 bytes). The `LV_IMG_DECLARE()` macro expands to an extern declaration of the array. When the array is placed in flash or PSRAM and declared this way, LVGL can load and display it directly without reading from a file.

The display initialization configures a double-buffered pixel buffer in PSRAM:

```cpp
bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
    .buffer_size   = BSP_LCD_H_RES * BSP_LCD_V_RES,   // 720 × 1280 = 921600 px
    .double_buffer = true,                               // two buffers for tear-free rendering
    .flags = {
        .buff_dma   = true,    // buffers live in DMA-capable memory
        .buff_spiram = true,  // primary buffer in PSRAM
        .sw_rotate  = true,    // software rotation (P4 has no display rotation unit)
    }
};
```

The display is rotated 90° in software so that content drawn in landscape orientation appears correctly on the physical portrait screen.

### The startup animation app shows how LVGL images are created and used

The factory `AppStartupAnim` creates two image widgets and animates them across the screen using spring-based interpolation. Its `onOpen()` callback shows the pattern clearly:

```cpp
_lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFFFFF), LV_PART_MAIN);
_lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

_logo_tab = std::make_unique<Image>(lv_screen_active());
_logo_tab->setAlign(LV_ALIGN_TOP_MID);
_logo_tab->setSrc(&logo_tab);   // references the LV_IMG_DECLARE'd image
_logo_tab->setPos(-46, 785);  // starts off-screen above the top edge
_logo_tab->setOpa(0);          // starts transparent
```

The C++ wrapper classes (`Image`, `Label`) are mooncake-specific. The tutorial firmware uses the plain C LVGL API instead, which achieves the same result with `lv_img_create()` and `lv_obj_set_pos()`.

### The esp_lvgl_port API is the standard Espressif abstraction

`esp_lvgl_port` is Espressif's officially supported way to integrate LVGL with any display panel. It handles:

- initializing LVGL itself
- creating the background LVGL task and periodic timer
- providing mutex-guarded `lvgl_port_lock()` / `lvgl_port_unlock()` calls that application code must use before touching LVGL objects from other tasks
- adding displays via platform-specific functions like `lvgl_port_add_disp_dsi()`

The `lvgl_port_add_disp_disp()` function accepts a `lvgl_port_display_cfg_t` containing the panel handles and buffer configuration, plus optional DSI-specific flags. The ESP-IDF component registry lists `esp_lvgl_port` as a managed dependency, and the `idf_component.yml` for the tutorial firmware specifies `espressif/esp_lvgl_port` as a versioned requirement.

### The P4 lacks hardware rotation

The ESP32-P4 does not have a dedicated display rotation engine. The factory firmware therefore sets `.sw_rotate = true` in the display flags and applies `lv_display_set_rotation(lvDisp, LV_DISPLAY_ROTATION_90)` after adding the display. The application then draws in landscape coordinates while the screen shows the content in portrait. This matters for logo placement: a 180×80 landscape logo needs to be centered in landscape mode, but on the physical portrait screen it will appear rotated.

The cleanest mental model is:

> Think of the LVGL canvas as a landscape 1280×720 rectangle that gets mapped onto the physical 720×1280 portrait screen through a 90° software rotation. Objects that are centered in landscape coordinates end up centered on the physical portrait screen after rotation.

## Gap Analysis

The existing text echo demo (`ESP-48`) covers the Wi-Fi and HTTP path but does not touch the display at all. The factory firmware covers the display but depends on the mooncake C++ framework and a large app structure. There is no minimal, plain-C, ESP-IDF-native example of the Tab5 display path that a new engineer can read in one sitting.

The gap is:

- No tutorial firmware showing just the display bring-up
- No explanation of the `esp_lvgl_port` + `lvgl_port_add_disp_dsi()` initialization sequence
- No guide showing how to use `LV_IMG_DECLARE` to embed an image and display it with LVGL
- No explanation of why the P4 needs software rotation and double-buffered PSRAM buffers
- No explanation of the backlight GPIO control sequence

## Proposed Solution

### Architectural summary

The boot logo firmware layers five components:

1. **ESP-IDF component manifest** — declares the `esp_lvgl_port`, `esp_lcd_panel`, `esp_lvgl`, and `esp_hosted` dependencies
2. **SDK defaults** — extends the text echo defaults with display, LVGL, and SPIRAM configuration
3. **Display initialization** — calls `esp_lcd_new_panel()` for the ST7123 over MIPI DSI, then `lvgl_port_add_disp_dsi()` to register it with LVGL
4. **LVGL logo display** — declares the factory logo with `LV_IMG_DECLARE`, creates an `lv_img` object, and positions it on the rotated screen
5. **Background services** — starts the Wi-Fi SoftAP and HTTP server so the board remains reachable while the logo is visible

### File layout

```text
0051-tab5-boot-logo/
├── CMakeLists.txt
├── build.sh
├── sdkconfig.defaults
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.c           ← boot entrypoint
│   ├── display_app.c          ← LVGL + display bring-up
│   ├── display_app.h
│   ├── wifi_app.c            ← from the text echo demo
│   ├── wifi_app.h
│   ├── http_server.c          ← from the text echo demo
│   ├── http_server.h
│   ├── echo_state.c           ← from the text echo demo
│   ├── echo_state.h
│   ├── wifi_console.c         ← from the text echo demo
│   ├── wifi_console.h
│   └── assets/
│       └── logo_m5.c          ← copied from M5Tab5-UserDemo/app/assets/images/logo_tab.c
└── main/idf_component.yml
```

### Boot sequence

The full application entrypoint calls each subsystem in dependency order:

```c
void app_main(void) {
    ESP_LOGI(TAG, "boot");

    // Display first: LVGL needs the display to exist before any lv_ calls
    ESP_ERROR_CHECK(display_app_init());   // MIPI DSI + lvgl_port + logo

    // Network second: the display is already running
    ESP_ERROR_CHECK(echo_state_init());
    ESP_ERROR_CHECK(wifi_app_start());    // AP+STA + NVS credentials
    wifi_console_start();                  // REPL for Wi-Fi config
    ESP_ERROR_CHECK(http_server_start());  // HTTP echo server

    ESP_LOGI(TAG, "ready — logo displayed on screen");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

The display must be initialized before any LVGL calls, which is why `display_app_init()` runs first. If the display initialization fails, the logo simply does not appear.

### Pseudocode: display bring-up

```text
display_app_init()
  esp_lcd_new_panel_dsi(host)              // create ST7123 panel handle on P4 DSI
  esp_lcd_new_panel_io_dsi(host)            // create DSI panel IO handle
  lvgl_port_init(lvgl_port_cfg)            // initialize LVGL task + timer

  display_cfg = {
    .io_handle   = dsi_io_handle,
    .panel_handle = panel_handle,
    .buffer_size = 720 * 1280 * 2,        // one full-screen RGB565 buffer
    .double_buffer = true,                  // two buffers for tear-free rendering
    .flags = {
      .buff_spiram = true,               // buffers go in PSRAM
      .sw_rotate = true,                // P4 has no HW rotation
    }
  }

  dsi_cfg = { .flags = { .avoid_tearing = true } }

  disp = lvgl_port_add_disp_dsi(display_cfg, dsi_cfg)  // register with LVGL
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90)  // software 90° rotation
  bsp_display_backlight_on()                              // enable backlight via GPIO

  // Now LVGL is ready. Display the logo.
  lv_init()
  LV_IMG_DECLARE(logo_tab)    // extern reference to the image array
  scr = lv_screen_active()
  img = lv_img_create(scr)      // create image widget on the active screen
  lv_img_set_src(img, &logo_tab)  // point it at the declared image
  lv_obj_center(img)           // center in landscape (rotated to portrait on screen
```

## Design Decisions

### 1. Display before network

**Decision:** initialize the display before starting Wi-Fi and HTTP.

**Why:** LVGL objects must be created after `lvgl_port_add_disp()` is called. If the network services start first and their logging clutters the serial console, it is still better to put the display on screen as early as possible so the user gets visual feedback immediately on boot.

### 2. LVGL port over raw `lvgl_init()` / `lv_port_disp_init()`

**Decision:** use `esp_lvgl_port` instead of hand-rolling the LVGL task and timer.

**Why:** `esp_lvgl_port` is the officially supported Espressif abstraction. It handles the FreeRTOS task, the timer period, the mutex semantics, and the DMA/bus error paths correctly. Hand-rolling a LVGL task is error-prone on Tab5 because the P4's memory layout and DMA requirements are board-specific. The official abstraction is also what the factory firmware uses.

### 3. Double-buffered PSRAM instead of internal SRAM

**Decision:** allocate LVGL framebuffers in PSRAM, not in internal SRAM.

**Why:** the Tab5 has 32 MB of PSRAM and 768 KB of internal SRAM. A full 720×1280 RGB565 framebuffer is 1.8 MB. Two framebuffers would consume 3.6 MB of internal SRAM, which is impossible. PSRAM is the correct place for pixel buffers on this board.

### 4. Software rotation instead of hardware rotation

**Decision:** use `lv_display_set_rotation()` and `.sw_rotate = true` rather than relying on the P4's display rotation hardware.

**Why:** the ESP32-P4 does not have a dedicated display rotation engine in the way the ESP32-S3 does. The software rotation in LVGL is the correct approach for this chip. The tradeoff is a small CPU cost for the rotation on each frame, which is negligible for a static logo.

### 5. Include the logo from M5Tab5-UserDemo

**Decision:** copy `logo_tab.c` from the M5Tab5-UserDemo assets into the tutorial firmware, rather than generating a new image.

**Why:** the factory logo is already the right size (180×80 landscape), already in the correct RGB565 format, and already declared with `LV_IMG_DECLARE`. Reusing it means no image generation step is needed. It also makes the relationship between the tutorial and the factory firmware obvious.

### 6. Keep the network services running alongside the logo

**Decision:** start the Wi-Fi SoftAP and HTTP server as background services while the logo stays on screen.

**Why:** this demonstrates that the display and network can coexist, and it keeps the board reachable over the network even during the boot logo phase. A board that shows a logo but is unreachable is harder to debug.

## Implementation Details

### SDK defaults

The `sdkconfig.defaults` extends the text echo defaults with the display-specific settings:

```
# Display and LVGL
CONFIG_LV_COLOR_SCREEN_TRANSP=n
CONFIG_LV_MEM_CUSTOM=y
CONFIG_LV_DISP_DEF_REFR_PERIOD=25
CONFIG_LV_USE_LOG=y
CONFIG_LV_LOG_PRINTF=y
CONFIG_LV_USE_PERF_MONITOR=y
CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y

# Font configuration (minimum set for the demo)
CONFIG_LV_FONT_MONTSERRAT_12=y
CONFIG_LV_FONT_MONTSERRAT_16=y
CONFIG_LV_FONT_MONTSERRAT_24=y

# PSRAM double-buffering
CONFIG_SPIRAM=y
CONFIG_SPIRAM_SPEED_200M=y
CONFIG_SPIRAM_XIP_FROM_PSRAM=y
```

The `LV_COLOR_SCREEN_TRANSP=n` flag is important. When this is enabled, the factory firmware expects the display to support transparency mode. The tutorial demo uses opaque rendering, so this flag must be disabled.

### `LV_IMG_DECLARE` and the logo image

The `LV_IMG_DECLARE(logo_tab)` macro from LVGL expands to:

```c
extern const lv_image_dsc_t logo_tab;
```

The actual `lv_image_dsc_t` structure is defined at the end of `logo_tab.c`:

```c
const lv_image_dsc_t logo_tab = {
    .header.cf       = LV_COLOR_FORMAT_RGB565,
    .header.magic    = LV_IMAGE_HEADER_MAGIC,
    .header.w        = 180,
    .header.h        = 80,
    .data_size       = 14400 * 2,   // = 28800 bytes
    .data            = logo_tab_map,
};
```

LVGL reads the width, height, and color format from the header at runtime. The `lv_img_create()` call does not need these values explicitly; it reads them from the descriptor.

### Positioning the logo on a rotated screen

With the display set to `LV_DISPLAY_ROTATION_90`, the LVGL canvas is rotated. The logo is 180×80 in landscape. Centering it with `lv_obj_center(img)` places it at the landscape center, which maps to the portrait center on the physical screen — exactly where you want it.

If you want the logo higher or lower, use `lv_obj_set_pos(img, x, y)` in landscape coordinates. Higher Y values move the logo down on the physical portrait screen. Lower Y values move it up.

### Backlight control

The Tab5 backlight is driven by a ME2212 boost converter enable GPIO controlled through an I/O expander. The factory firmware calls `bsp_display_backlight_on()` to enable it. In the tutorial firmware, the backlight is enabled implicitly by the panel initialization. If it is not on, the screen appears black even when LVGL is rendering correctly.

If the backlight is off, check that the GPIO expander initialization completed and that the `LCD_BL_GPIO` signal is being driven high.

### LVGL task safety and the display mutex

LVGL is not thread-safe by default. Any task that creates, modifies, or deletes LVGL objects must hold the `lvgl_port_lock(0)` mutex first. The console REPL runs in a separate task, so calling `lv_img_set_src()` from the console task requires the mutex. The boot logo is created in the `app_main` task before the console starts, so it does not need locking.

If a future iteration adds a console command to change the displayed logo, that command must call:

```c
lvgl_port_lock(0);
lv_img_set_src(img, new_src);
lvgl_port_unlock();
```

## Test Strategy

### 1. Display-only pass: does the screen show the logo?

Power the board with only the display initialization in `app_main()`. Remove the network services temporarily. If the logo appears, the display stack is correct.

### 2. Full pass: does the logo stay on screen while the network runs?

With the network services added, verify:
- the logo is still visible on power-on
- the SoftAP appears on the expected IP
- the HTTP server responds to `curl http://192.168.4.1/api/health`
- `wifi status` prints correctly from the console

### 3. PSRAM buffer pass: does the logo render without tearing?

A static logo on a double-buffered display should render without tearing. If tearing is visible, the `.avoid_tearing` flag in the DSI config may need to be enabled, or the buffer size should be verified to ensure both buffers are in PSRAM. Memory-mapped PSRAM buffers avoid the tearing artifact that occurs when the display scans out a buffer that is simultaneously being written.

## Risks and Open Questions

### Risk: backlight control timing

The backlight must be enabled after the panel initialization completes but before LVGL begins rendering. If it is enabled too early or too late, the user may briefly see a black screen or a flash. The factory firmware enables the backlight after the LVGL display handle is created. The tutorial firmware follows the same sequence.

### Risk: logo appears black if the color format is wrong

LVGL supports multiple color formats. The logo array is RGB565. If the display is initialized in RGB888 mode, the logo will appear with incorrect colors or all black. Verify `.header.cf = LV_COLOR_FORMAT_RGB565` in the logo descriptor matches the `lvgl_port_display_cfg_t` color format.

### Risk: P4 memory pressure from large buffers

Two full-screen RGB565 buffers in PSRAM consume 3.6 MB. With LVGL's internal structures, this is still well within the 32 MB PSRAM budget. The risk is if other PSRAM consumers (camera buffers, audio, large network buffers) are added later, the PSRAM budget needs to be re-evaluated.

### Open question: should the logo be animated?

The factory app animates the logo sliding in from off-screen. For the tutorial demo, a static logo is simpler. A spring animation would require `smooth_ui_toolkit` or a hand-rolled animation task. This is intentionally out of scope for the first version.

### Open question: should the logo fade in with `lv_obj_set_opa()`?

A fade-in on boot is achievable with `lv_obj_set_opa(img, 0)` followed by `lv_anim` that animates opacity from 0 to 255. This would require an animation timer, which adds complexity. Out of scope for the first version.

## References

- `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp` — factory HAL showing the full display bring-up sequence
- `M5Tab5-UserDemo/app/apps/app_startup_anim/app_startup_anim.cpp` — factory startup animation using LVGL images
- `M5Tab5-UserDemo/app/assets/assets.h` — `LV_IMG_DECLARE` logo declarations
- `M5Tab5-UserDemo/app/assets/images/logo_tab.c` — the 180×80 RGB565 logo array (28800 bytes)
- `M5Tab5-UserDemo/platforms/tab5/components/espressif__esp_lvgl_port/include/esp_lvgl_port_disp.h` — display config structure with DSI support
- `M5Tab5-UserDemo/platforms/tab5/sdkconfig.defaults` — Tab5 PSRAM and LVGL defaults
- `esp32-s3-m5/0050-tab5-web-text-echo/sdkconfig.defaults` — existing Tab5 SDK defaults
- `esp32-s3-m5/0050-tab5-web-text-echo/main/wifi_app.c` — existing Wi-Fi bring-up pattern
- `esp32-s3-m5/0050-tab5-web-text-echo/main/http_server.c` — existing HTTP server pattern
- [LVGL display documentation](https://docs.lvgl.io/) — LVGL image display API
- [ESP-IDF LVGL port documentation](https://components.espressif.com/components/espressif/esp_lvgl_port) — `esp_lvgl_port` API reference
