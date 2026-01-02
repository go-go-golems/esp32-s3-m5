---
Title: 'Implementation guide: Status bar + HUD overlay (A1)'
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - ui
    - keyboard
    - esp32s3
    - esp-idf
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.hpp
      Note: Sprite creation and pushSprite layering
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Multi-sprite layout inspiration
ExternalSources: []
Summary: Intern guide for implementing a layered status bar/HUD overlay (dirty redraw, layered sprites, stable chrome) for all demos.
LastUpdated: 2026-01-01T20:09:15.58903029-05:00
WhatFor: ""
WhenToUse: ""
---



# Implementation guide: Status bar + HUD overlay (A1)

This guide describes how to implement a **persistent status bar + HUD overlay** that can be reused by all demos in the demo firmware. This is “real firmware UI”: almost every device UI needs a top bar and/or small on-screen indicators.

The primary outcome is an intern-readable recipe for:

- drawing a status bar efficiently (only when values change)
- layering it on top of a content screen
- keeping it consistent across demos (same layout, same controls)

## Outcome (acceptance criteria)

- A header bar is always visible showing:
  - demo name (or screen name)
  - frame-time or FPS
  - heap + DMA free
  - optional battery indicator (if available)
- The header updates without flicker and without forcing full-screen redraw.
- The header can be disabled/toggled from any demo (e.g., key `H` toggles “HUD”).

## Where to look (existing patterns)

- Multi-sprite layout inspiration:
  - `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`
- A robust redraw-on-dirty pattern:
  - `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`
- Sprite present pattern with `waitDMA()`:
  - `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`

## Rendering model (recommended)

Use layered sprites:

- `content` sprite: demo content area (usually full screen or below the header)
- `header` sprite: fixed-height top bar (e.g., 240×16)
- `footer` sprite: optional keys/help hints (e.g., 240×16)
- `overlay` sprite: transient overlays (help, modal, toast) drawn last

Present order (each frame or when dirty):

```text
1) content.pushSprite(0, content_y)
2) header.pushSprite(0, 0)
3) footer.pushSprite(0, 135 - footer_h) (optional)
4) overlay.pushSprite(0, 0, colorkey) (optional)
5) display.waitDMA() if full-screen DMA present is used
```

This keeps chrome separate so most demos only touch `content`.

## Key APIs to use

Sprites:

```cpp
void* createSprite(int32_t w, int32_t h);
void pushSprite(int32_t x, int32_t y);
template<typename T> void pushSprite(int32_t x, int32_t y, const T& transparent);
uint32_t bufferLength(void) const;
```

Text and shapes:

```cpp
void setTextDatum(textdatum_t datum);
void setTextColor(T fg, T bg);
void setTextSize(float sx, float sy);
size_t drawString(const char* s, int32_t x, int32_t y);

void fillRect(int32_t x, int32_t y, int32_t w, int32_t h);
void drawFastHLine(int32_t x, int32_t y, int32_t w);
void drawRect(int32_t x, int32_t y, int32_t w, int32_t h);
```

Metrics and system info:

- `esp_get_free_heap_size()` (ESP-IDF)
- `heap_caps_get_free_size(MALLOC_CAP_DMA)` (ESP-IDF)

## Data model (what state the status bar needs)

Define a struct that contains all values that influence drawing. Update it periodically, and redraw only when it changes.

```cpp
struct HudState {
  const char* screen_name = "Home";
  int fps = 0;
  int frame_ms = 0;          // optional; display one metric
  uint32_t heap_free = 0;
  uint32_t dma_free = 0;
  int battery_pct = -1;      // -1 means unknown
  bool wifi = false;         // optional
};
```

Keep a `HudState last_drawn;` and compare field-by-field.

## Layout (keep it simple and readable)

Suggested header layout (240×16):

```text
[ScreenName........] [fps:60] [H:123k D:45k] [bat:85%]
```

Rules:

- use a single small font size everywhere
- avoid dynamic text jitter: set fixed column widths where possible
- align numbers right so they don’t “dance” as digits change

## Drawing routine (intern-friendly, deterministic)

Pseudocode:

```cpp
void draw_header(LGFX_Sprite& header, const HudState& s) {
  header.fillRect(0, 0, header.width(), header.height(), TFT_DARKGREY);
  header.drawFastHLine(0, header.height() - 1, header.width(), TFT_BLACK);

  header.setTextSize(1, 1);
  header.setTextDatum(lgfx::textdatum_t::middle_left);
  header.setTextColor(TFT_WHITE, TFT_DARKGREY);

  header.drawString(s.screen_name, 4, header.height() / 2);

  // right-aligned stats block
  header.setTextDatum(lgfx::textdatum_t::middle_right);
  char buf[64];
  snprintf(buf, sizeof(buf), "%d fps  H:%uk D:%uk", s.fps, (unsigned)(s.heap_free / 1024), (unsigned)(s.dma_free / 1024));
  header.drawString(buf, 236, header.height() / 2);
}
```

## “Dirty” strategy (don’t redraw every frame)

Recommended update intervals:

- FPS/frame time: 200–500ms
- heap/dma free: 500–1000ms
- battery: 2–10s

Implementation sketch:

```cpp
bool hud_dirty = false;
if (now_ms - last_hud_update_ms > 250) {
  auto new_state = sample_hud_state();
  if (new_state != last_drawn) hud_dirty = true;
  hud_state = new_state;
}
if (hud_dirty) {
  draw_header(header, hud_state);
  header.pushSprite(0, 0);
  hud_dirty = false;
  last_drawn = hud_state;
}
```

## Optional: icons (battery, wifi)

If you have 1-bit icon bitmaps, render them with:

```cpp
drawXBitmap(x, y, icon_bits, w, h, color);
```

That’s usually better than shipping PNGs for tiny icons.

## Integration checklist

- Decide the header height (16 is a good start).
- Ensure every demo uses consistent `content_y` offset (header height).
- Ensure help overlay doesn’t overlap the header unless intentionally designed.

## Common pitfalls

- Redrawing the header every frame: wastes time and can cause visible flicker when combined with full-screen animation.
- Mixing direct-to-display drawing with sprite overlay without a plan: you’ll get “overdraw ordering” bugs.
- Putting too much text in 16px height: readability suffers; prioritize the name + one metric.
