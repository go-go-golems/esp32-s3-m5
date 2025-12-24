---
Title: Cardputer graphics with M5GFX (text, sprites, UI patterns)
Ticket: 005-CARDPUTER-SYNTH
Status: active
Topics:
    - audio
    - speaker
    - keyboard
    - cardputer
    - esp32-s3
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Display init and canvas sprite layout (M5GFX + LGFX_Sprite composition)
    - Path: M5Cardputer-UserDemo/main/hal/hal.h
      Note: HAL display/canvas ownership and the pushSprite()-based update API
    - Path: M5Cardputer-UserDemo/main/apps/utils/theme/theme_define.h
      Note: Font and layout constants used throughout UI (FONT_BASIC/FONT_REPL, glyph size assumptions)
    - Path: M5Cardputer-UserDemo/main/apps/launcher/views/system_bar/system_bar.cpp
      Note: Text + icon rendering patterns (fillSmoothRoundRect, setFont, drawCenterString, pushImage)
    - Path: M5Cardputer-UserDemo/main/apps/launcher/views/menu/menu_render_callback.hpp
      Note: Sprite-based UI composition and centered text rendering for menu icons
    - Path: M5Cardputer-UserDemo/main/apps/app_texteditor/app_texteditor.cpp
      Note: Text-mode UI using cursor APIs, text scrolling, and manual monospace layout
    - Path: M5Cardputer-UserDemo/main/apps/app_wifi_scan/app_wifi_scan.cpp
      Note: Mixed printf-style text rendering with per-line color changes
ExternalSources: []
Summary: "How M5Cardputer-UserDemo uses M5GFX/LovyanGFX to build a sprite-based UI on Cardputer, with emphasis on text rendering (fonts, cursor layout, drawCenterString, printf/print) and practical patterns for building apps."
LastUpdated: 2025-12-22T22:29:49.756353189-05:00
WhatFor: "Help you build Cardputer graphical apps using M5GFX with a clear mental model: display init, sprites as canvases, and the text rendering APIs that the existing apps rely on."
WhenToUse: "When creating a new Cardputer UI/app (menus, status bars, text UIs) and you want to follow the established patterns for M5GFX sprites, fonts, and text layout."
---

## Executive summary

`M5Cardputer-UserDemo` uses **M5GFX** (a packaging of LovyanGFX for M5Stack devices) as its display driver, but it almost never renders “directly” to the hardware display. Instead it builds a simple compositor:

- One `LGFX_Device` (`M5GFX`) is the physical display.
- Three `LGFX_Sprite` objects act as “offscreen canvases”:
  - main content canvas (`_canvas`)
  - system bar (`_canvas_system_bar`)
  - keyboard bar (`_canvas_keyboard_bar`)
- Each canvas is drawn into independently and then pushed into the correct position on the real display with `pushSprite()`.

This architecture has two big benefits for application code:

- **Flicker-free updates**: draw into a sprite buffer, then blit once.
- **Simple UI layering**: you can update only the system bar or only the main canvas without redrawing everything.

Text rendering is a mix of:

- **cursor-style printing** (`setCursor()`, `print()`, `printf()`) for “terminal-ish” UIs (REPL, text editor, wifi scan list)
- **centered string helpers** (`drawCenterString()`) for menu labels and status bar time display

## Architecture overview: display + canvases

This is the system-level picture you should keep in your head before reading the individual apps.

### Display init (Cardputer HAL)

The Cardputer HAL’s display init is short and intentionally avoids board-specific LovyanGFX boilerplate:

- **File**: `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`
- **Symbol**: `HAL::HalCardputer::_display_init()`

Key steps:

- `_display = new M5GFX; _display->init();`
- `_canvas = new LGFX_Sprite(_display); _canvas->createSprite(204, 109);`
- `_canvas_keyboard_bar = new LGFX_Sprite(_display); ... createSprite( _display->width()-_canvas->width(), _display->height());`
- `_canvas_system_bar = new LGFX_Sprite(_display); ... createSprite(_canvas->width(), _display->height()-_canvas->height());`

That geometry implies a layout like:

- A vertical “keyboard bar” sprite on the left
- A horizontal “system bar” sprite at the bottom (to the right of the keyboard bar)
- The main canvas in the remaining upper-right area

### “Push” API (where rendering actually hits the screen)

Rather than having every app call `pushSprite(...)` with coordinates, the HAL exposes tiny helpers:

- **File**: `M5Cardputer-UserDemo/main/hal/hal.h`

Key helpers:

- `canvas_keyboard_bar_update()` → `keyboard_bar_sprite->pushSprite(0, 0)`
- `canvas_system_bar_update()` → `system_bar_sprite->pushSprite(keyboard_bar_width, 0)`
- `canvas_update()` → `main_canvas->pushSprite(keyboard_bar_width, system_bar_height)`

This is a good pattern to copy: apps render into the right sprite, then call the one correct “present” function.

## Fonts + text sizing conventions (what the UI code assumes)

Almost every app shares a single theme header:

- **File**: `M5Cardputer-UserDemo/main/apps/utils/theme/theme_define.h`

Important conventions:

- `FONT_BASIC` and `FONT_REPL` are both `&fonts::efontCN_16` (a 16px font shipped by LovyanGFX).
- `FONT_HEIGHT` is hard-coded to `16`.
- “Monospace-ish” sizing for REPL/text-editor layout is hard-coded:
  - `FONT_REPL_WIDTH = 8`
  - `FONT_REPL_HEIGHT = 16`

That means some apps treat the font as a fixed-width 8×16 grid even though not all fonts truly are. If you change `FONT_REPL`, revisit any code that manually computes cursor movement and line wrapping.

## Text rendering patterns (what you should copy)

### Pattern A: centered labels (menus, status bar)

This is used for menu icon captions and the system bar time display:

- Set font once (`setFont(FONT_BASIC)`)
- Set text color (usually with background)
- Use `drawCenterString(text, center_x, center_y)`

Examples:

- `LauncherRender_CB_t::renderCallback()` in `.../views/menu/menu_render_callback.hpp`
- `Launcher::_update_system_bar()` in `.../views/system_bar/system_bar.cpp`

Why this pattern is nice:

- You avoid manual cursor math for centering.
- You can redraw the entire widget into a sprite each frame and blit once.

### Pattern B: “terminal” output using cursor APIs (lists, logs, editors)

This is the simplest way to output text:

- `setFont(FONT_REPL)`
- `setTextSize(1)` (or another scale)
- `setTextColor(fg, bg)`
- `setCursor(x, y)`
- `print(...)` / `printf(...)`

Examples:

- WiFi scan list: `M5Cardputer-UserDemo/main/apps/app_wifi_scan/app_wifi_scan.cpp`
  - Per-row color changes (`setTextColor(...)` before `printf`)
- Text editor: `.../apps/app_texteditor/app_texteditor.cpp`
  - Enables scroll: `setTextScroll(true)`
  - Uses `setBaseColor(bg)` to keep scroll behavior consistent

## Practical gotchas (things the existing code already learned)

### Text scrolling needs base color

The text editor explicitly sets:

- `setTextScroll(true)`
- `setBaseColor(THEME_COLOR_BG)`

Without a base color, scroll clearing can leave visual artifacts depending on the font/background usage.

### Manual cursor management assumes fixed glyph size

In the text editor, delete/backspace behavior manually moves the cursor using `FONT_REPL_WIDTH`/`FONT_REPL_HEIGHT` constants and prints `"  "` to erase.

That’s fine for a controlled font choice, but it’s fragile if you switch fonts or text size.

## “Quick recipes” for new apps (copy/paste mental model)

### Render a simple screen of text to the main canvas

Pseudo-usage:

```cpp
auto c = hal->canvas();
c->fillScreen(THEME_COLOR_BG);
c->setFont(FONT_REPL);
c->setTextSize(1);
c->setTextColor(TFT_WHITE, THEME_COLOR_BG);
c->setCursor(0, 0);
c->printf("Hello Cardputer!\nRSSI: %d\n", rssi);
hal->canvas_update();
```

### Render a status bar “pill” with centered text

Pseudo-usage (mirrors `system_bar.cpp`):

```cpp
auto sb = hal->canvas_system_bar();
sb->fillScreen(THEME_COLOR_BG);
sb->fillSmoothRoundRect(5, 4, sb->width()-10, sb->height()-8, (sb->height()-8)/2, THEME_COLOR_SYSTEM_BAR);
sb->setFont(FONT_BASIC);
sb->setTextColor(THEME_COLOR_SYSTEM_BAR_TEXT);
sb->drawCenterString("12:34", sb->width()/2, sb->height()/2 - FONT_HEIGHT/2);
hal->canvas_system_bar_update();
```

## External references

- M5GFX (component used by this repo): see `M5Cardputer-UserDemo/components/M5GFX/`
- LovyanGFX docs and examples: `https://github.com/lovyan03/LovyanGFX`

