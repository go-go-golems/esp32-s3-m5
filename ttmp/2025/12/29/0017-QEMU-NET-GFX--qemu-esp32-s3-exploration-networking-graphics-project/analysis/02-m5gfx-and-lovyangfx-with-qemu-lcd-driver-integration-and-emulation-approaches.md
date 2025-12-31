---
Title: 'M5GFX and LovyanGFX with QEMU: LCD driver integration and emulation approaches'
Ticket: 0017-QEMU-NET-GFX
Status: active
Topics:
    - esp-idf
    - esp32s3
    - qemu
    - networking
    - graphics
    - emulation
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/framebuffer/Panel_fb.hpp
      Note: LovyanGFX framebuffer panel implementation for Linux framebuffer devices
    - Path: 0013-atoms3r-gif-console/main/display_hal.cpp
      Note: Real hardware example of M5GFX/LovyanGFX display initialization with GC9107 panel
    - Path: ttmp/2025/12/21/003-ANALYZE-ATOMS3R-USERDEMO--analyze-m5atoms3-userdemo-project/analysis/02-m5unified-m5gfx-deep-dive-display-stack-and-low-level-lcd-communication.md
      Note: Deep dive into M5GFX/LovyanGFX architecture and low-level LCD communication
ExternalSources: []
Summary: Comprehensive analysis of using M5GFX (LovyanGFX) graphics library with QEMU emulation for ESP32-S3, covering LCD driver options, QEMU framebuffer support, integration approaches, and practical implementation strategies
LastUpdated: 2025-12-29T15:26:16.802838787-05:00
WhatFor: Guide developers on integrating M5GFX/LovyanGFX graphics libraries with QEMU for ESP32-S3 emulation, documenting available LCD driver options, QEMU's virtual framebuffer capabilities, and practical approaches for graphics emulation
WhenToUse: When planning to use M5GFX or LovyanGFX graphics libraries in QEMU-emulated ESP32-S3 projects, or when evaluating graphics emulation strategies for embedded development
---


# M5GFX and LovyanGFX with QEMU: LCD driver integration and emulation approaches

## Executive Summary

This document analyzes the integration of **M5GFX** (built on **LovyanGFX**) graphics libraries with **QEMU** emulation for ESP32-S3 projects. The key findings are:

1. **M5GFX/LovyanGFX Architecture**: M5GFX wraps LovyanGFX and provides board-specific display initialization. LovyanGFX handles low-level LCD controller communication via SPI/I2C/parallel buses.

2. **QEMU Graphics Support**: QEMU for ESP32-S3 supports a **virtual RGB framebuffer** via the `espressif/esp_lcd_qemu_rgb` component, which provides an `esp_lcd`-compatible interface.

3. **Integration Approaches**: Three main paths exist:
   - Use `esp_lcd_qemu_rgb` component with ESP-IDF's `esp_lcd` API (native ESP-IDF approach)
   - Adapt LovyanGFX's framebuffer panel (`Panel_fb`) for QEMU's framebuffer device
   - Use LovyanGFX's SDL panel (`Panel_sdl`) for PC-based simulation (alternative to QEMU)

4. **Challenges**: QEMU does not emulate specific LCD controllers (e.g., GC9107, ST7789) or SPI buses used by M5GFX/LovyanGFX. Graphics must be routed through QEMU's virtual framebuffer or alternative rendering paths.

## M5GFX and LovyanGFX Architecture

### Library Stack Overview

The graphics stack consists of three layers:

1. **M5Unified**: Device abstraction layer that provides board detection, pin mapping, and global `M5` singleton. Calls `M5.Display.init_without_reset()` during `M5.begin()`.

2. **M5GFX**: Graphics library wrapper around LovyanGFX that:
   - Provides board-specific display initialization and auto-detection
   - Contains pin mapping tables for M5Stack devices (AtomS3, AtomS3R, etc.)
   - Exposes `M5.Display.*` drawing APIs (rects, fonts, PNG decoding)
   - Supports canvas/sprite system for double-buffered rendering

3. **LovyanGFX (`lgfx`)**: Low-level graphics library that:
   - Implements LCD controller command sequences (GC9107, ST7789, etc.)
   - Provides bus classes for transport (SPI, I2C, parallel)
   - Optimizes ESP32-S3 SPI writes with direct register programming and DMA
   - Supports multiple panel types and framebuffer backends

### Key Components in This Repository

From the codebase analysis:

- **M5GFX sources**: `M5Cardputer-UserDemo/components/M5GFX/`
- **LovyanGFX sources**: `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/`
- **Panel implementations**: `lgfx/v1/panel/Panel_*.hpp` (GC9107, ST7789, framebuffer, SDL)
- **Bus implementations**: `lgfx/v1/platforms/esp32/Bus_SPI.hpp`, `Bus_I2C.hpp`

### Real Hardware Example: AtomS3R Display Initialization

From existing ESP-IDF projects in this repo (e.g., `0013-atoms3r-gif-console`, `0017-atoms3r-web-ui`):

```cpp
// AtomS3R uses GC9107 panel via SPI3
static lgfx::Bus_SPI s_bus;
static lgfx::Panel_GC9107 s_panel;
static m5gfx::M5GFX s_display;

// Configure SPI bus
auto cfg = s_bus.config();
cfg.spi_host = SPI3_HOST;
cfg.spi_mode = 0;
cfg.spi_3wire = true;
cfg.freq_write = 40000000; // 40MHz
cfg.pin_sclk = 15;
cfg.pin_mosi = 21;
cfg.pin_dc = 42;
s_bus.config(cfg);

// Configure panel
s_panel.bus(&s_bus);
auto pcfg = s_panel.config();
pcfg.pin_cs = 14;
pcfg.pin_rst = 48;
pcfg.panel_width = 128;
pcfg.panel_height = 128;
pcfg.offset_y = 32; // GC9107 has 128x160 memory, visible is 128x128
s_panel.config(pcfg);

// Initialize display
s_display.init(&s_panel);
```

**Key insight**: Real hardware uses SPI3_HOST with specific GPIO pins and a GC9107 panel driver. QEMU cannot emulate this exact hardware path.

## QEMU Graphics Emulation Capabilities

### QEMU Virtual RGB Framebuffer

Espressif's QEMU fork (version 8.1.3-20231206 and later) includes a **virtual RGB panel** that provides:

- Color modes: ARGB8888, RGB565
- Dedicated framebuffer memory
- Display window that opens when running `idf.py qemu --graphics monitor`

### ESP-IDF Component: `esp_lcd_qemu_rgb`

The official component provides an `esp_lcd`-compatible driver for QEMU's virtual framebuffer:

**Installation:**
```bash
idf.py add-dependency "espressif/esp_lcd_qemu_rgb^1.0.2"
```

**Usage pattern** (from ESP-IDF documentation):
- Initialize using standard `esp_lcd` API
- Configure panel parameters (width, height, color depth)
- Write to framebuffer using `esp_lcd_panel_draw_bitmap()`
- QEMU displays the framebuffer in a separate window

**Limitations:**
- Only works within QEMU environment
- Does not emulate specific LCD controllers (GC9107, ST7789, etc.)
- No SPI bus emulation for LCD controllers
- Framebuffer is virtual, not connected to real SPI display hardware

### QEMU Graphics Command

To launch QEMU with graphics support:
```bash
idf.py qemu --graphics monitor
```

This opens a window displaying the virtual framebuffer contents.

## Integration Approaches

### Approach 1: ESP-IDF Native (`esp_lcd_qemu_rgb`)

**Pros:**
- Official Espressif component
- Clean ESP-IDF integration
- Works directly with QEMU's virtual framebuffer
- No external graphics library dependencies

**Cons:**
- Cannot use M5GFX/LovyanGFX directly (different API)
- Requires rewriting graphics code to use `esp_lcd` API
- Loses M5GFX features (PNG decoding, fonts, canvas system)

**Implementation pattern:**
```cpp
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_qemu_rgb.h"

// Initialize QEMU RGB panel
esp_lcd_panel_handle_t panel_handle;
esp_lcd_qemu_rgb_config_t qemu_config = {
    .width = 128,
    .height = 128,
    .color_mode = ESP_LCD_COLOR_SPACE_RGB565,
};
esp_lcd_new_panel_qemu_rgb(&qemu_config, &panel_handle);
esp_lcd_panel_init(panel_handle);

// Draw using esp_lcd API
uint16_t* framebuffer = ...;
esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 128, framebuffer);
```

### Approach 2: LovyanGFX Framebuffer Panel (`Panel_fb`)

LovyanGFX includes a Linux framebuffer panel (`Panel_fb`) that maps to `/dev/fb0` or named devices.

**Pros:**
- Can use LovyanGFX/M5GFX APIs directly
- Maintains all graphics library features
- Works if QEMU exposes a framebuffer device

**Cons:**
- Requires QEMU to expose `/dev/fb0` or similar device
- May need custom QEMU configuration
- Not officially supported by Espressif's QEMU fork

**Implementation pattern:**
```cpp
#include "lgfx/v1/platforms/framebuffer/Panel_fb.hpp"

lgfx::Panel_fb panel;
panel.setDeviceName("/dev/fb0"); // or QEMU's framebuffer device
m5gfx::M5GFX display;
display.init(&panel);

// Use M5GFX APIs normally
display.fillRect(0, 0, 128, 128, TFT_RED);
display.drawPng(...);
```

**Status**: Requires investigation whether QEMU's virtual framebuffer is accessible as a Linux framebuffer device from within the emulated system.

### Approach 3: LovyanGFX SDL Panel (`Panel_sdl`)

LovyanGFX includes an SDL-based panel for PC simulation. This approach is used by M5Stack's `lv_m5_emulator` project for running M5Stack applications in an emulated environment.

**Pros:**
- Full M5GFX/LovyanGFX API support
- Native PC development and testing
- Can run without QEMU
- Proven approach (used by M5Stack's own emulator project)
- SDL provides windowed graphics output on host system

**Cons:**
- Not QEMU-based (different emulation approach)
- Requires SDL2 library
- May have different behavior than QEMU emulation
- Does not test QEMU-specific code paths

**Use case**: Alternative to QEMU for graphics development and testing. Useful for rapid iteration without QEMU constraints, but not suitable for QEMU-specific testing or when QEMU emulation fidelity is required.

**Reference**: M5Stack's `lv_m5_emulator` project (https://github.com/m5stack/lv_m5_emulator) demonstrates this approach using PlatformIO and SDL.

### Approach 4: Hybrid: ESP-IDF `esp_lcd` + Custom M5GFX Adapter

Create an adapter layer that:
- Uses `esp_lcd_qemu_rgb` for QEMU framebuffer access
- Implements M5GFX/LovyanGFX panel interface that wraps `esp_lcd` calls
- Allows M5GFX APIs to work with QEMU framebuffer

**Pros:**
- Maintains M5GFX API compatibility
- Uses official QEMU component
- Can conditionally compile for QEMU vs. real hardware

**Cons:**
- Requires custom adapter implementation
- May have performance overhead
- Needs maintenance

**Implementation sketch:**
```cpp
// Custom panel adapter
class Panel_QEMU_RGB : public lgfx::Panel_Device {
    esp_lcd_panel_handle_t panel_handle;
public:
    bool init(bool use_reset) override {
        // Initialize esp_lcd_qemu_rgb
        esp_lcd_qemu_rgb_config_t cfg = {...};
        esp_lcd_new_panel_qemu_rgb(&cfg, &panel_handle);
        return true;
    }
    void writePixels(...) override {
        // Convert to esp_lcd_panel_draw_bitmap()
    }
};
```

## ESP-IDF LCD Component Overview

### `esp_lcd` Component

ESP-IDF provides a unified LCD component (`components/esp_lcd/`) that supports:

- Multiple panel drivers (ST7789, SSD1306, etc.)
- SPI and I2C bus interfaces
- RGB parallel interfaces
- Panel IO abstraction

**Panel drivers available:**
- `esp_lcd_panel_st7789`
- `esp_lcd_panel_ssd1306`
- `esp_lcd_panel_gc9107` (via component registry: `espressif/esp_lcd_gc9107`)

**Note**: GC9107 is not in ESP-IDF core but available via component manager:
```bash
idf.py add-dependency "espressif/esp_lcd_gc9107^2.0.0"
```

### QEMU-Specific LCD Support

The `esp_lcd_qemu_rgb` component is specifically designed for QEMU and:
- Provides virtual RGB panel interface
- Maps to QEMU's framebuffer memory
- Supports standard `esp_lcd` panel operations

## Practical Recommendations

### For QEMU Graphics Development

1. **Start with `esp_lcd_qemu_rgb`**: Use the official component for initial QEMU graphics testing.

2. **Evaluate M5GFX integration needs**:
   - If only basic graphics needed: use `esp_lcd` API directly
   - If M5GFX features required (PNG, fonts, canvas): consider Approach 4 (adapter layer)

3. **Test framebuffer device access**: Investigate whether QEMU exposes `/dev/fb0` that could work with `Panel_fb`.

4. **Consider SDL simulation**: For graphics development without QEMU constraints, use LovyanGFX's SDL panel.

### For Real Hardware + QEMU Compatibility

1. **Conditional compilation**: Use `#ifdef CONFIG_IDF_TARGET_QEMU` to switch between:
   - Real hardware: M5GFX with `Panel_GC9107` and SPI bus
   - QEMU: `esp_lcd_qemu_rgb` or adapter layer

2. **Abstraction layer**: Create a display HAL that abstracts panel initialization:
   ```cpp
   #ifdef CONFIG_IDF_TARGET_QEMU
       init_qemu_display();
   #else
       init_m5gfx_display();
   #endif
   ```

3. **Test both paths**: Ensure graphics code works in both QEMU and real hardware environments.

## Known Limitations and Challenges

### QEMU Limitations

1. **No SPI LCD controller emulation**: QEMU does not emulate SPI buses or LCD controllers (GC9107, ST7789). Graphics must use virtual framebuffer.

2. **No GPIO emulation for LCD pins**: GPIO pins used for LCD control (CS, DC, RST) are not emulated in a way that affects display output.

3. **Timing differences**: QEMU timing may differ from real hardware, affecting graphics rendering performance.

4. **Memory constraints**: QEMU may have different memory constraints than real hardware.

### M5GFX/LovyanGFX Limitations in QEMU

1. **Hardware-specific optimizations**: M5GFX/LovyanGFX use ESP32-S3-specific SPI optimizations that won't work in QEMU.

2. **Board auto-detection**: M5GFX's board detection relies on hardware-specific probes that may fail in QEMU.

3. **Backlight control**: Backlight control (PWM or I2C) is hardware-specific and not emulated.

## Research Findings Summary

### From Web Research

1. **QEMU graphics support**: Confirmed that Espressif's QEMU fork supports virtual RGB framebuffer via `esp_lcd_qemu_rgb` component.

2. **M5GFX ESP-IDF support**: M5GFX can be used with ESP-IDF via component manager (`m5stack/m5gfx^0.1.16`).

3. **Alternative approaches**: LVGL simulator and SDL-based simulation are alternatives to QEMU for graphics development.

4. **M5Stack LVGL Emulator**: M5Stack provides an `lv_m5_emulator` project (https://github.com/m5stack/lv_m5_emulator) that demonstrates running M5Stack applications in an emulated environment using SDL for display output. This project uses PlatformIO and SDL to simulate M5Stack devices on a host machine, providing insights into graphics library integration with emulated environments.

5. **QEMU SDL integration**: QEMU can utilize SDL (Simple DirectMedia Layer) for rendering graphics, allowing the emulated system's graphics to be displayed on the host machine. This is separate from but complementary to QEMU's virtual framebuffer approach.

6. **LovyanGFX SDL panel**: LovyanGFX includes an SDL panel implementation (`Panel_sdl`) that can render graphics in a windowed environment on the host system, facilitating development and testing without physical hardware. This is a native PC development approach, not QEMU-based.

### From Codebase Analysis

1. **LovyanGFX framebuffer support**: `Panel_fb` exists for Linux framebuffer devices, potentially usable with QEMU if framebuffer device is exposed.

2. **Real hardware patterns**: Multiple ESP-IDF projects in this repo use M5GFX with GC9107 panel via SPI3, providing reference implementation.

3. **Display initialization**: Standard pattern involves configuring SPI bus, panel, and display object, then calling `init()`.

## Next Steps

1. **Test `esp_lcd_qemu_rgb`**: Create a minimal QEMU project using `esp_lcd_qemu_rgb` to verify graphics output.

2. **Investigate framebuffer device**: Check if QEMU exposes `/dev/fb0` or similar that could work with `Panel_fb`.

3. **Prototype adapter layer**: If M5GFX integration is needed, implement a proof-of-concept adapter between M5GFX and `esp_lcd_qemu_rgb`.

4. **Document QEMU graphics workflow**: Create a playbook for QEMU graphics development based on chosen approach.

5. **Compare with real hardware**: Test same graphics code on real hardware to identify QEMU-specific differences.

## References

- ESP-IDF LCD Component: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/lcd.html
- `esp_lcd_qemu_rgb` Component: https://components.espressif.com/components/espressif/esp_lcd_qemu_rgb
- M5GFX GitHub: https://github.com/m5stack/M5GFX
- LovyanGFX GitHub: https://github.com/lovyan03/LovyanGFX
- LovyanGFX Documentation: https://lovyangfx.readthedocs.io/en/master/01_intro.html
- QEMU ESP32 Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/qemu.html
- M5Stack LVGL Emulator: https://github.com/m5stack/lv_m5_emulator (SDL-based emulation example)
