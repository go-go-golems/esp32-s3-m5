---
Title: 'Implementation guide: Frame-time + perf overlay (B2)'
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
    - Path: 0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: waitDMA usage affects present timing
    - Path: 0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Existing perf-related logging patterns
ExternalSources: []
Summary: Intern guide for implementing a low-overhead perf overlay (frame/update/render/present timings, heap/DMA stats) to debug all demos.
LastUpdated: 2026-01-01T20:09:18.006860773-05:00
WhatFor: ""
WhenToUse: ""
---



# Implementation guide: Frame-time + perf overlay (B2)

This guide describes how to build the **frame-time + performance overlay** that makes every other demo easier to debug. This is not just a “cool counter”; it is the fastest way to answer “why is this stuttering?”.

## Outcome (acceptance criteria)

- Overlay shows:
  - total frame time (ms) and/or FPS
  - update time, render time, present time (optional but recommended)
  - heap free and DMA free
- Overlay updates at a controlled rate (e.g., 4 Hz) to avoid becoming the performance problem.
- Overlay can be toggled globally (e.g., key `H` or `F`).

## Where to look

- Example of structured logging + UI redraw loops:
  - `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`
- M5GFX DMA present and why `waitDMA()` matters:
  - `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`

## Instrumentation points (what to measure)

Measure these phases:

1. **Input + update**: processing key events, updating demo state
2. **Render**: drawing into content sprite/canvas
3. **Present**: pushing sprites to display and waiting for DMA (if used)

Use microsecond resolution via ESP-IDF:

- `esp_timer_get_time()` (returns microseconds)

Also collect memory:

- `esp_get_free_heap_size()`
- `heap_caps_get_free_size(MALLOC_CAP_DMA)`

## Core data structures

Use a small rolling window so the numbers are stable. Keep it simple.

```cpp
struct FrameTimingsUs {
  uint32_t update_us = 0;
  uint32_t render_us = 0;
  uint32_t present_us = 0;
  uint32_t total_us = 0;
};

struct PerfSnapshot {
  FrameTimingsUs last;
  uint32_t heap_free = 0;
  uint32_t dma_free = 0;

  // smoothed values (rolling average)
  uint32_t avg_total_us = 0;
  uint32_t avg_render_us = 0;
};
```

Rolling average update (fixed alpha, no floats):

```cpp
uint32_t ema(uint32_t prev, uint32_t sample, uint8_t shift /* e.g. 3 => 1/8 */) {
  return prev + ((sample - prev) >> shift);
}
```

## Rendering strategy

Treat perf overlay as a tiny, separate sprite:

- `perf_overlay` sprite (e.g., 240×16 or 240×24)
- redraw it at 4 Hz or when toggled
- push it last (so it’s always on top)

This avoids messing with demo content rendering.

## Implementation skeleton (what to code)

### 1) Timing helper

Pseudocode:

```cpp
struct ScopedTimer {
  int64_t start_us = 0;
  uint32_t* out = nullptr;
  explicit ScopedTimer(uint32_t* out_) : start_us(esp_timer_get_time()), out(out_) {}
  ~ScopedTimer() { *out = (uint32_t)(esp_timer_get_time() - start_us); }
};
```

### 2) Frame loop integration

Pseudocode:

```cpp
FrameTimingsUs t = {};
{
  ScopedTimer timer(&t.update_us);
  poll_input();
  demo.update(...);
}
{
  ScopedTimer timer(&t.render_us);
  demo.render(...);
}
{
  ScopedTimer timer(&t.present_us);
  present_layers();
  if (demo.needs_wait_dma()) display.waitDMA();
}
t.total_us = t.update_us + t.render_us + t.present_us;
```

### 3) Sampling + smoothing

Update overlay state at a slower cadence:

```cpp
if (now_ms - last_overlay_update_ms >= 250) {
  snapshot.heap_free = esp_get_free_heap_size();
  snapshot.dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA);

  snapshot.avg_total_us = ema(snapshot.avg_total_us, t.total_us, 3);
  snapshot.avg_render_us = ema(snapshot.avg_render_us, t.render_us, 3);
  overlay_dirty = true;
  last_overlay_update_ms = now_ms;
}
```

### 4) Overlay drawing

Keep it compact and stable:

```cpp
void draw_perf_overlay(LGFX_Sprite& s, const PerfSnapshot& p) {
  s.fillRect(0, 0, s.width(), s.height(), TFT_BLACK);
  s.drawFastHLine(0, s.height() - 1, s.width(), TFT_DARKGREY);

  s.setTextSize(1, 1);
  s.setTextDatum(lgfx::textdatum_t::middle_left);
  s.setTextColor(TFT_WHITE, TFT_BLACK);

  int fps = (p.avg_total_us > 0) ? (int)(1000000 / p.avg_total_us) : 0;

  char buf[96];
  snprintf(buf, sizeof(buf),
           "fps:%d  total:%ums  render:%ums  H:%uk D:%uk",
           fps,
           (unsigned)(p.avg_total_us / 1000),
           (unsigned)(p.avg_render_us / 1000),
           (unsigned)(p.heap_free / 1024),
           (unsigned)(p.dma_free / 1024));
  s.drawString(buf, 4, s.height() / 2);
}
```

## What to show (recommended fields)

Start with:

- FPS + total frame ms
- render ms
- heap/dma free

Then optionally add:

- present ms (useful when DMA dominates)
- “dirty redraw count” (useful in UI screens)

## Validation checklist

- Overlay displays sensible numbers and doesn’t flicker.
- When you intentionally add a slow loop, overlay shows higher frame ms and lower fps.
- When you enable/disable `waitDMA()`, you see present time change (if your demo uses DMA).

## Common pitfalls

- Updating overlay every frame: the overlay becomes the bottleneck and pollutes your timings.
- Using floats in hot loops: unnecessary overhead on embedded.
- Measuring `present` incorrectly: `waitDMA()` should be included if it’s part of your pipeline.
