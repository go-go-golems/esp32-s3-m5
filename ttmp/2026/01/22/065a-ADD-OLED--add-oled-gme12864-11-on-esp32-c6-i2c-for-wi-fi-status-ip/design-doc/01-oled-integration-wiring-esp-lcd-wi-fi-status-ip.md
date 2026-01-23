---
Title: 'OLED integration: wiring + esp_lcd + Wi-Fi status + IP'
Ticket: 065a-ADD-OLED
Status: active
Topics:
    - esp-idf
    - esp32c6
    - xiao
    - i2c
    - oled
    - display
    - wifi
    - ui
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_driver_i2c/include/driver/i2c_master.h
      Note: New I2C master driver used for bus init and address probe
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_lcd/include/esp_lcd_io_i2c.h
      Note: ESP-IDF I2C panel IO config used for SSD1306
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_lcd/src/esp_lcd_panel_ssd1306.c
      Note: ESP-IDF SSD1306 driver; defines framebuffer expectations and init
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_lcd/test_apps/i2c_lcd/main/test_i2c_lcd_panel.c
      Note: Reference SSD1306 I2C bring-up and draw_bitmap usage
    - Path: 0065-xiao-esp32c6-gpio-web-server/README.md
      Note: Target firmware to extend with OLED status display
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/app_main.c
      Note: Integration point for OLED init/task + existing Wi-Fi bring-up
    - Path: components/wifi_mgr/include/wifi_mgr.h
      Note: Wi-Fi status snapshot API used to render state/IP
    - Path: components/wifi_mgr/wifi_mgr.c
      Note: Wi-Fi event handling and status fields (state/ssid/ip4)
    - Path: ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/scripts/idf_inventory_ssd1306.sh
      Note: Re-runnable inventory of ESP-IDF SSD1306 support
    - Path: ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/scripts/lookup_gme12864_11_sources.py
      Note: Re-runnable breadcrumbs for module naming/sources
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-22T20:49:01.167688787-05:00
WhatFor: ""
WhenToUse: ""
---


# OLED integration: wiring + esp_lcd + Wi-Fi status + IP

## Executive Summary

Add a small 128×64 monochrome OLED module (GME12864-11) to the Seeed Studio XIAO ESP32C6 firmware `0065-xiao-esp32c6-gpio-web-server` over I2C, using ESP-IDF 5.4.1’s built-in `esp_lcd` SSD1306 panel driver. The OLED renders a minimal “status HUD”: Wi‑Fi state (IDLE/CONNECTING/CONNECTED), SSID, and the STA IPv4 address when available.

The project already has the needed software building blocks:
- `components/wifi_mgr` maintains a thread-safe snapshot of Wi‑Fi state + IP (`wifi_mgr_get_status()`).
- ESP-IDF 5.4.1 provides `esp_lcd_panel_ssd1306` and an I2C panel IO implementation (`esp_lcd_new_panel_io_i2c(...)`).

The missing pieces are: wiring documentation, an I2C bring-up checklist, and a small 1‑bit text renderer that draws into the SSD1306 page-oriented framebuffer.

## Problem Statement

The existing firmware `0065-xiao-esp32c6-gpio-web-server` is controllable via:
- an interactive `esp_console` REPL over USB Serial/JTAG (Wi‑Fi configuration)
- a small HTTP server once the station has an IP

However, when this device is installed in a “headless” setting (no computer attached and no easy way to discover the new IP), there is no local feedback to answer:
- “Is Wi‑Fi connected?”
- “What SSID am I on?”
- “What IP address did DHCP assign?”

Adding a tiny OLED status display provides immediate, at-a-glance observability without changing the existing REPL/UI.

## Proposed Solution

### 1) Hardware / wiring (XIAO ESP32C6 + I2C OLED)

This ticket assumes an I2C variant of the GME12864-11 128×64 OLED module (often SSD1306 based). Many 0.96" 128×64 OLEDs exist in both SPI and I2C variants; therefore the bring-up checklist includes verifying the interface and address.

Wire the OLED to the XIAO ESP32C6 as follows:

- **GND → GND**
- **VCC → 3V3** (preferred; avoid 5V unless the module explicitly supports it and has level shifting)
- **SDA → XIAO “pin 4” (GPIO22)** (per Seeed Studio ESP32C6 pinout: SDA = GPIO22)
- **SCL → XIAO “pin 5” (GPIO23)** (per Seeed Studio ESP32C6 pinout: SCL = GPIO23)
- **RST (optional)**:
  - If your OLED board exposes `RST`, connect it to a spare GPIO so the firmware can hard-reset the panel.
  - If unconnected, the SSD1306 driver can run with `.reset_gpio_num = -1` (software init only).

I2C electrical notes (important in practice):
- I2C requires pull-ups. Many OLED breakout boards include pull-ups on SDA/SCL; if not, add external ~4.7kΩ pull-ups to 3V3.
- ESP32 internal pull-ups exist but are weak; they may work at 100kHz on short wires but are not a substitute for real pull-ups, especially at 400kHz or with longer cables.
- Keep wires short and routed away from high-current/fast-edge lines.

### 2) ESP-IDF software you can reuse (ESP-IDF 5.4.1)

ESP-IDF 5.4.1 already contains:

1. **SSD1306 panel driver**
   - `components/esp_lcd/src/esp_lcd_panel_ssd1306.c`
   - API: `esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle)`

2. **I2C panel IO driver used by `esp_lcd`**
   - `components/esp_lcd/include/esp_lcd_io_i2c.h`
   - API: `esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle)`

3. **New I2C master driver**
   - `components/esp_driver_i2c/include/driver/i2c_master.h`
   - API: `i2c_new_master_bus(...)`, `i2c_master_probe(...)` (useful for “scan”)

4. **Reference test implementation**
   - `components/esp_lcd/test_apps/i2c_lcd/main/test_i2c_lcd_panel.c`
   - Shows the correct `control_phase_bytes` and `dc_bit_offset` settings for SSD1306 I2C.

This repo already has:
- `components/wifi_mgr` and `components/wifi_console`, already used by `0065-xiao-esp32c6-gpio-web-server`.

### 3) Display driver architecture

Implement the OLED as a small module, e.g. `main/oled_status.{c,h}`, responsible for:
- initializing I2C bus + SSD1306 panel (one-time)
- maintaining a 128×64 1bpp framebuffer (1024 bytes)
- providing simple drawing primitives for 1bpp:
  - clear framebuffer
  - draw a glyph / draw text
  - optional draw box / separator line
- pushing the framebuffer to the panel via `esp_lcd_panel_draw_bitmap(...)`

The module should be “best effort”:
- If no device responds on the configured I2C address, log once and keep running without OLED.
- Avoid spamming logs (the console is shared with USB Serial/JTAG REPL).

### 3a) Concrete initialization skeleton (I2C master v2 + esp_lcd SSD1306)

Below is a minimal “shape” of the initialization code for ESP-IDF 5.4.1, based on the `esp_lcd` test app:

```c
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

static i2c_master_bus_handle_t s_i2c_bus;
static esp_lcd_panel_io_handle_t s_lcd_io;
static esp_lcd_panel_handle_t s_panel;

static esp_err_t oled_init(gpio_num_t sda, gpio_num_t scl, uint8_t i2c_addr_7bit)
{
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .sda_io_num = sda,
        .scl_io_num = scl,
        // .flags.enable_internal_pullup = 1, // optional; prefer external pull-ups
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_i2c_bus), TAG, "i2c_new_master_bus");

    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr = i2c_addr_7bit,   // e.g. 0x3C
        .scl_speed_hz = 400000,      // try 100000 if wiring is marginal
        .control_phase_bytes = 1,    // per SSD1306 datasheet
        .dc_bit_offset = 6,          // per SSD1306 datasheet (control byte D/C# bit)
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_cfg, &s_lcd_io), TAG, "new_panel_io_i2c");

    esp_lcd_panel_ssd1306_config_t ssd1306_cfg = {
        .height = 64,
    };
    esp_lcd_panel_dev_config_t panel_cfg = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,           // set if you wire RST
        .vendor_config = &ssd1306_cfg,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ssd1306(s_lcd_io, &panel_cfg, &s_panel), TAG, "new_panel_ssd1306");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "panel_reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel_init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "disp_on");

    return ESP_OK;
}
```

Important notes:
- `control_phase_bytes=1` and `dc_bit_offset=6` are SSD1306 I2C specifics; if these are wrong you’ll often see “ACK but blank screen”.
- If you wire `RST`, set `.reset_gpio_num` and consider calling `esp_lcd_panel_reset()` unconditionally at boot for robustness.

### 4) SSD1306 framebuffer format (what `draw_bitmap` expects)

In ESP-IDF’s SSD1306 implementation, `esp_lcd_panel_draw_bitmap(panel, x0, y0, x1, y1, data)`:
- sets `COLUMN_RANGE` and `PAGE_RANGE`
- sends raw bytes using `tx_color(...)`
- computes `len = (y1 - y0) * (x1 - x0) * bpp / 8`

For a full-screen update on a 128×64 panel at 1bpp:
- `len = 64 * 128 / 8 = 1024 bytes`

The byte layout matches the SSD1306 page concept:
- the screen is split into 8 “pages” of 8 vertical pixels each
- within a page, each byte represents a column (x) of 8 pixels (y within that page)
- a typical mapping is:
  - `page = y / 8`
  - `index = page * width + x`
  - `bit = 1 << (y % 8)`

This mapping is the foundation for font rendering: if you can set/clear a pixel in this buffer, you can render any monochrome font.

### 5) Font rendering strategy (minimal, robust, no extra dependencies)

Goal: render 2–6 short lines of ASCII text (Wi‑Fi state, SSID, IP) reliably, without LVGL and without large external libraries.

Recommended approach: fixed-width bitmap font embedded as a C header.

Minimum viable font:
- **6×8** or **5×7** glyphs for ASCII 0x20–0x7E
- store glyphs as “column bytes” that already match SSD1306 page layout for an 8‑pixel-high glyph
- draw_char implementation:
  - for each glyph column, OR/overwrite into framebuffer byte at `(page*width + x)`
  - optionally support inverse text by XOR with 0xFF on the glyph column

Why this approach works well here:
- The SSD1306 memory model is page-oriented (8-pixel tall units); 8‑pixel-tall glyphs map perfectly.
- Wi‑Fi status UI is tiny; we don’t need proportional fonts, kerning, or anti-aliasing.
- No heap allocations; deterministic, small, easy to test.

Optional font pipeline (if you want “nicer” fonts later):
- Author a font as BDF/PCF or use a known bitmap font.
- Add a small conversion script that emits a `const uint8_t font[][GLYPH_W]` array in SSD1306 column format.
- Keep the runtime renderer unchanged; only swap the font table.

### 6) Rendering the Wi‑Fi state + IP

The firmware already tracks everything we need via `wifi_mgr_get_status()`:
- `state` = UNINIT/IDLE/CONNECTING/CONNECTED
- `ssid` = last configured SSID
- `ip4` = host-order IPv4 (0 means “none”)

Two viable update models:

1) **Polling task (simplest integration)**
   - Create a FreeRTOS task `oled_task` that runs every 250–1000 ms:
     - call `wifi_mgr_get_status(&st)`
     - if `st` differs from previous snapshot, redraw framebuffer and flush to OLED
   - Benefits: does not require touching Wi‑Fi event registration; decoupled and robust.
   - Cost: periodic wakeups (still trivial at 1 Hz).

2) **Event-driven (more “correct”, slightly more plumbing)**
   - Register handlers on the default event loop for `WIFI_EVENT` and `IP_EVENT`.
   - On relevant events, post a notification to the OLED task to redraw.
   - Benefits: no polling; instantaneous updates.
   - Cost: more moving pieces; still needs a task or lock to avoid drawing inside event context.

Given the firmware is a tutorial-like project, polling is the recommended baseline.

### 7) Console + logging constraints

This repo prefers the **USB Serial/JTAG** console (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`), and `0065` already uses it. Keep it that way:
- OLED updates should not print every frame.
- Only log init failures once (e.g., “no device ACK at 0x3C”).

## Design Decisions

1) **Use `esp_lcd` SSD1306 instead of a third-party library**
   - Rationale: ESP-IDF 5.4.1 already ships it, and it matches the target display class.

2) **Use a 1bpp framebuffer + full-screen flush**
   - Rationale: 128×64×1bpp is only 1024 bytes, easy to manage, and makes text composition straightforward.

3) **Use a fixed 8-pixel-tall bitmap font**
   - Rationale: aligns with SSD1306 page layout; simplest correct implementation.

4) **Use a polling-based status refresh loop**
   - Rationale: avoids extra event plumbing and is easy to reason about; redraw only on change.

5) **Treat OLED as optional / best-effort**
   - Rationale: the primary UX is still the USB console + web UI; OLED should never block boot or Wi‑Fi.

## Alternatives Considered

1) **LVGL (`lvgl` + `esp_lvgl_port`)**
   - Pros: rich widgets, fonts, layout.
   - Cons: heavier dependency footprint than needed for a 2-line status HUD; adds complexity to a small tutorial firmware.

2) **u8g2 / Adafruit SSD1306**
   - Pros: widely used, high-level APIs.
   - Cons: extra third-party dependency management; duplicates ESP-IDF functionality available in 5.4.1.

3) **Draw directly using raw I2C writes (no `esp_lcd`)**
   - Pros: minimal code, fully custom.
   - Cons: reinventing a working driver; harder to maintain; more likely to get SSD1306 init sequencing wrong.

## Implementation Plan

### Step A: Verify the OLED variant (I2C vs SPI) and the I2C address

Before writing code, confirm:
- the board is an I2C OLED breakout (typically 4 pins: VCC/GND/SCL/SDA)
- the I2C address (often `0x3C` or `0x3D`)

Plan for a simple address probe (“scan”) using `i2c_master_probe(...)` from `driver/i2c_master.h`.

### Step B: Add OLED configuration knobs

Add a `Kconfig.projbuild` section under `0065-xiao-esp32c6-gpio-web-server/main/` (or a new component) for:
- enable/disable OLED
- SDA GPIO (default 22)
- SCL GPIO (default 23)
- I2C address (default 0x3C)
- I2C speed (default 400000; consider 100000 first if bring-up is flaky)
- optional RESET GPIO (default -1 / disabled)
- refresh interval (ms) and/or “redraw only on change”

### Step C: Add `oled_status` module and wire it into `app_main`

In `0065-xiao-esp32c6-gpio-web-server/main/`:
- Add `oled_status.c`/`.h`
- In `app_main.c`:
  - initialize OLED early (after log init; before Wi‑Fi is fine)
  - start OLED task after `wifi_mgr_start()` so status is meaningful

### Step D: Implement font + rendering

1) Create a small font table header, e.g. `oled_font_6x8.h`
2) Implement primitives:
   - `oled_fb_clear(fb)`
   - `oled_fb_draw_char(fb, x, y, ch)`
   - `oled_fb_draw_text(fb, x, y, "text")`
3) Render lines:
   - `WiFi: IDLE|CONNECTING|OK`
   - `SSID: <ssid or ->`
   - `IP: <ip or ->`

### Step E: Flush strategy and update triggers

Use a task:
- cache last `wifi_mgr_status_t` (or a minimal subset: state + ip4 + ssid)
- if changed: rebuild framebuffer and call `esp_lcd_panel_draw_bitmap(panel, 0, 0, 128, 64, fb)`

### Step F: Validate on hardware

- Confirm OLED init (panel turns on, stable, no flicker)
- Confirm screen updates:
  - on boot: IDLE, IP "-"
  - when connecting: CONNECTING
  - after IP: CONNECTED + actual IP
  - on disconnect: IDLE and IP "-"

## Open Questions

1) What is the exact controller / interface on the specific “GME12864-11” module in hand?
   - Many listings indicate SSD1306; some vendor pages mention SPI variants.
2) What is the I2C address on the module (0x3C vs 0x3D)?
3) Does the breakout include pull-ups, and are they sized appropriately for the chosen bus speed?
4) Should the OLED show additional information (RSSI, disconnect reason, uptime)?
5) Is an OLED reset pin accessible and worth wiring for improved robustness?

## References

- Firmware: `0065-xiao-esp32c6-gpio-web-server/README.md`
- Firmware OLED implementation:
  - `0065-xiao-esp32c6-gpio-web-server/main/oled_status.c`
  - `0065-xiao-esp32c6-gpio-web-server/main/oled_font_5x7.h`
  - `0065-xiao-esp32c6-gpio-web-server/main/Kconfig.projbuild`
- Wi‑Fi status snapshot API: `components/wifi_mgr/include/wifi_mgr.h` and `components/wifi_mgr/wifi_mgr.c`
- SSD1306 panel: `/home/manuel/esp/esp-idf-5.4.1/components/esp_lcd/src/esp_lcd_panel_ssd1306.c`
- I2C panel IO: `/home/manuel/esp/esp-idf-5.4.1/components/esp_lcd/include/esp_lcd_io_i2c.h`
- I2C master driver: `/home/manuel/esp/esp-idf-5.4.1/components/esp_driver_i2c/include/driver/i2c_master.h`
- SSD1306 bring-up example: `/home/manuel/esp/esp-idf-5.4.1/components/esp_lcd/test_apps/i2c_lcd/main/test_i2c_lcd_panel.c`
- Ticket scripts:
  - `scripts/idf_inventory_ssd1306.sh`
  - `scripts/lookup_gme12864_11_sources.py`
