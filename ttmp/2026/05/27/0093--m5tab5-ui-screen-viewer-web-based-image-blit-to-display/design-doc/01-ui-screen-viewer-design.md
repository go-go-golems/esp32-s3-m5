---
Title: ""
Ticket: ""
Status: ""
Topics: []
DocType: ""
Intent: ""
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp
      Note: Complex C++ HAL init (runs OK, but too heavy to fork for screen viewer)
    - Path: 0051-tab5-boot-logo/components/m5stack_tab5/include/bsp/display.h
      Note: 720x1280 RGB565 display specs
    - Path: 0051-tab5-boot-logo/main/app_main.c
      Note: Boot sequence to fork from
    - Path: 0051-tab5-boot-logo/main/display_app.c
      Note: Display init + LVGL pattern to reuse
    - Path: 0051-tab5-boot-logo/main/http_server.c
      Note: HTTP server pattern to extend with upload endpoint
    - Path: 0051-tab5-boot-logo/main/wifi_app.c
      Note: WiFi APSTA + NVS persistence to reuse
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---







# UI Screen Viewer Design

## Executive Summary

Build an ESP-IDF firmware for the M5Tab5 that runs a Wi-Fi webserver, accepts uploaded images via HTTP POST, and blits them onto the 720×1280 MIPI DSI display using LVGL. This lets designers/developers preview potential UI mockups directly on the physical hardware from a browser.

## Problem Statement

When designing UIs for the M5Tab5's 5-inch 720P display, there is no quick way to preview how a mockup looks on the actual hardware. Currently you must:
- Convert images to C arrays and reflash the firmware (0051-tab5-boot-logo approach)
- Or run the full M5Tab5-UserDemo factory app, which works but is a complex C++ codebase (mooncake framework, smooth_ui_toolkit, many peripheral HALs) — hard to modify for a focused screen viewer use case

We need a lightweight "screen viewer" firmware that:
1. Boots fast
2. Connects to Wi-Fi (with esp_console setup)
3. Serves a web page where you can drag-and-drop or select a PNG/JPG/BMP image
4. Renders the uploaded image fullscreen on the Tab5 display
5. Remains responsive — you can upload new images without reflashing

## Current-State Architecture (Evidence-Based)

### Existing Tab5 projects in esp32-s3-m5

| Project | Display | WiFi | HTTP | Console | Status |
|---------|---------|------|------|---------|--------|
| `0050-tab5-web-text-echo` | ❌ No display | ✅ APSTA + NVS | ✅ Full | ✅ wifi commands | Working |
| `0051-tab5-boot-logo` | ✅ BSP+LVGL logo | ✅ APSTA + NVS | ✅ Full | ✅ wifi commands | Working |
| `M5Tab5-UserDemo` | ✅ Full factory UI | ✅ AP only (hal_wifi.cpp) | ✅ Hello page | ❌ No console | Runs OK* |

### Key hardware facts

- **MCU**: ESP32-P4 (dual-core, 400MHz, no built-in WiFi)
- **WiFi**: ESP32-C6 slave via ESP-Hosted over SDIO (4-bit, 40MHz)
- **Display**: 720×1280 portrait, ST7123 MIPI DSI panel, 2 data lanes @ 730 Mbps
- **Color format**: RGB565 (16-bit), big-endian = 0
- **BSP**: `m5stack_tab5` component provides `bsp_display_start_with_config()`, I2C, IO expanders (PI4IOE), backlight control
- **SPIRAM**: Available (quad, 200MHz), used for full-screen LVGL buffers
- **Console**: USB Serial/JTAG (GPIO 43/44)

### M5Tab5-UserDemo notes

The factory UserDemo (`platforms/tab5/main/`) uses a complex C++ HAL initialization path in `HalEsp32::init()`:
- Camera OSC init, codec init, IMU init, INA226 power monitor, RX8130 RTC, USB host, HID, RS485
- The `mooncake` framework (app runtime) + `smooth_ui_toolkit` add further complexity
- It has been confirmed to run on the hardware. The earlier report of a crash/loop was unverified — no crash logs or reproduction steps were found.
- **Verdict**: The UserDemo runs, but it's a poor fork base because it's a large C++ codebase with many peripherals we don't need for a simple screen viewer.

\* _UserDemo status: runs on hardware. No console REPL, AP-only WiFi, and the codebase is too heavy to fork for this purpose._

### Reusable code from 0051-tab5-boot-logo

The `0051-tab5-boot-logo` project is the ideal starting point because it already has:
- **Display init** (`display_app.c`): I2C → PI4IOE → bsp_reset_tp() → bsp_display_start_with_config() → rotation 90° → backlight on
- **WiFi** (`wifi_app.c`): Full APSTA with NVS persistence, auto-reconnect, scan
- **Console** (`wifi_console.c`): USB Serial/JTAG REPL with `wifi status|scan|set|save|connect|clear`
- **HTTP server** (`http_server.c`): Chunked JSON response, embedded HTML/JS assets
- **SDK config**: ESP-Hosted SDIO, SPIRAM, LVGL RGB565, custom 2MB partition table
- **BSP component**: `m5stack_tab5` with ST7123 driver, esp_lvgl_port

## Proposed Solution

### Firmware: `0093-tab5-ui-screen-viewer`

Fork from `0051-tab5-boot-logo`. Replace the boot logo rendering with an image upload → display pipeline.

### Core flow

```
Browser → POST /api/upload (multipart, PNG/JPG/BMP) → Firmware
  → Decode image to RGB565 pixel buffer in SPIRAM
  → Create/update LVGL image from pixel buffer
  → Blit fullscreen on display
Browser → GET / → Web UI with drag-drop upload area
```

### HTTP API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `GET /` | GET | Web UI (drag-drop image upload) |
| `GET /app.js` | GET | Frontend JavaScript |
| `GET /api/health` | GET | Health check |
| `GET /api/state` | GET | Current state (dimensions, filename) |
| `POST /api/upload` | POST | Upload image (multipart/form-data) |
| `POST /api/upload` | POST | Upload image (raw binary body) |
| `GET /api/screen` | GET | Screen metadata (resolution, format) |

### Image pipeline options

**Option A: LVGL built-in image decode (recommended)**
- Use LVGL's built-in PNG/JPG decoders (`lv_img_set_src` with `LV_IMG_CF_TRUE_COLOR`)
- Pros: No extra library needed, LVGL handles the decode
- Cons: LVGL PNG decoder is slow for 720×1280; need to check if it handles large images in SPIRAM

**Option B: Raw RGB565 upload**
- Browser-side: use `<canvas>` to resize + convert uploaded image to RGB565 binary
- POST raw RGB565 pixel buffer (720×1280×2 = 1.8 MB)
- Firmware: `memcpy` into SPIRAM buffer, update LVGL image descriptor
- Pros: Zero decode overhead on ESP32, instant blit
- Cons: Larger upload, more JS complexity, fixed resolution

**Option C: lodepng / upng embedded decode**
- Embed a lightweight PNG decoder (lodepng)
- Accept PNG uploads, decode to RGB565 in SPIRAM
- Pros: Works with any PNG source, reasonable decode time
- Cons: Extra code space, still CPU-bound decode

**Recommended: Option B (browser-side RGB565) as primary, with Option A (PNG) as fallback.**
The browser can easily convert any image to RGB565 at the correct resolution using a canvas. This gives the fastest possible turnaround on the device side — just a memcpy into SPIRAM. For convenience, also accept raw PNG uploads with LVGL decode as a fallback path.

### LVGL image rendering approach

```c
// SPIRAM buffer for current screen image
static uint8_t *s_screen_buf = NULL;  // 720 * 1280 * 2 bytes

// LVGL image descriptor
static lv_image_dsc_t s_screen_dsc = {
    .header = {
        .cf = LV_IMG_CF_TRUE_COLOR,
        .w = 720,
        .h = 1280,
    },
    .data = s_screen_buf,
    .data_size = 720 * 1280 * 2,
};

// Create fullscreen image object
lv_obj_t *img = lv_img_create(lv_screen_active());
lv_img_set_src(img, &s_screen_dsc);
```

When a new image arrives:
1. Copy RGB565 pixel data into `s_screen_buf`
2. Call `lv_img_cache_invalidate_src(&s_screen_dsc)` (LVGL 9) or `lv_obj_invalidate(img)`
3. LVGL re-renders the image on next refresh cycle

### Landscape orientation

The Tab5's physical panel is portrait (720×1280), but the firmware rotates to landscape (1280×720) via `lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90)`. The web UI should indicate that the target resolution is **1280×720** in landscape orientation, and the browser should pre-rotate the image accordingly before converting to RGB565.

### Memory layout

- SPIRAM full-screen buffer: 1280×720×2 = **1,843,200 bytes** (1.76 MB)
- HTTP receive buffer: 2 MB in SPIRAM (for raw RGB565 uploads)
- LVGL double buffer: already configured in BSP (2× full-screen in SPIRAM)
- Total SPIRAM usage: ~7 MB for display buffers — well within ESP32-P4's SPIRAM capacity

### Wi-Fi + Console setup

Reuse the `wifi_app.c` + `wifi_console.c` from 0051 directly:
- SoftAP always up (`Tab5-UI-Viewer` / `tab5viewer`) for recovery/setup
- STA joins saved network with NVS persistence
- Console on USB Serial/JTAG: `wifi set "SSID" "pass" save && wifi connect`

### Web UI

Simple single-page app:
- Drag-and-drop zone (or file picker button)
- Resolution indicator: "Target: 1280×720 landscape (RGB565)"
- Upload progress indicator
- Current image info (filename, dimensions, size)
- Clear screen button (fill black)

## Implementation Plan

### Phase 1: Fork and strip down

1. Copy `0051-tab5-boot-logo` → `0093-tab5-ui-screen-viewer`
2. Remove boot logo assets, simplify `display_app.c` to just init display with black screen
3. Add SPIRAM screen buffer allocation
4. Verify build + flash + display shows black

### Phase 2: Image upload HTTP endpoint

1. Add `POST /api/upload` handler to `http_server.c`
2. Accept raw binary body (RGB565) — content-length max = 1,843,200
3. Copy into SPIRAM buffer, update LVGL image, invalidate cache
4. Test with `curl -X POST --data-binary @test_rgb565.bin http://<ip>/api/upload`

### Phase 3: Browser-side RGB565 conversion

1. Write `app.js` with canvas-based image conversion
2. Load uploaded image into `<canvas>`, resize to 1280×720
3. Read pixels via `getImageData()`, convert RGBA → RGB565
4. POST binary RGB565 to `/api/upload`
5. Test with PNG/JPG drag-and-drop

### Phase 4: Polish and convenience

1. Add PNG upload fallback (if body starts with PNG magic, try LVGL decode)
2. Add `GET /api/screen` endpoint (resolution, format, current state)
3. Add `POST /api/clear` endpoint (fill black)
4. Add image metadata in state response
5. Update web UI with progress, clear button, status display

### Phase 5: Testing and documentation

1. Test with various image sizes and formats
2. Test WiFi AP mode + STA mode
3. Test console wifi commands
4. Write README with build/flash/usage instructions

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| LVGL image update not thread-safe | Crash/garbled display | Use `lvgl_port_lock()` / `lvgl_port_unlock()` around image updates |
| HTTP body too large for stack | Stack overflow | Receive into SPIRAM heap buffer, not stack |
| RGB565 byte order mismatch | Colors wrong | Tab5 uses big-endian=0 (little-endian), verify with test pattern |
| PNG decode too slow on P4 | Poor UX | Browser-side conversion is primary path; PNG is fallback |
| Upload timeout on large images | Failed upload | Increase `HTTPD_DEFAULT_CONFIG()` recv timeout; limit image size in web UI |

## Key Files (Reference)

### From 0051-tab5-boot-logo (base)
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c` — Boot sequence
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c` — Display init + LVGL
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/main/wifi_app.c` — WiFi APSTA
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/main/wifi_console.c` — Console REPL
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/main/http_server.c` — HTTP server
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/sdkconfig.defaults` — ESP-Hosted + LVGL config

### From M5Tab5-UserDemo (reference)
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp` — Full HAL init (crash source)
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Tab5-UserDemo/platforms/tab5/main/hal/components/hal_wifi.cpp` — Simple AP mode wifi

### BSP component
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/include/bsp/display.h` — 720×1280, RGB565
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/include/bsp/m5stack_tab5.h` — BSP API
