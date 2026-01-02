---
Title: 'Project analysis: System monitor + sparklines (heap/FPS/Wi-Fi)'
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
    - Path: 0005-wifi-event-loop/main/hello_world_main.c
      Note: ESP-IDF Wi-Fi event loop and state transitions; useful for “Wi-Fi status” panel
    - Path: 0022-cardputer-m5gfx-demo-suite/main/demo_b2_perf.cpp
      Note: Existing perf visualization patterns (bars, timing breakdown)
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_hud.h
      Note: Existing perf/memory snapshot model (EMA) and header/footer overlays (non-LVGL)
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: Provides loop counter + LVGL handler time metrics used by SysMon
    - Path: 0025-cardputer-lvgl-demo/main/demo_basics.cpp
      Note: Existing periodic status label (heap/dma/last_key) and timer cleanup pattern
    - Path: 0025-cardputer-lvgl-demo/main/demo_manager.cpp
      Note: Where a new “System Monitor” screen would be integrated
    - Path: 0025-cardputer-lvgl-demo/main/demo_system_monitor.cpp
      Note: Implements SysMon screen (charts + sampling timer)
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: Display flush path; informs what “FPS” actually measures
    - Path: THE_BOOK.md
      Note: Repo-wide discussion of perf/memory and “keep loop responsive” patterns
ExternalSources: []
Summary: Analyzes how to build a reusable system monitor screen (heap/DMA/FPS/Wi‑Fi) with tiny sparklines on Cardputer LVGL, reusing perf snapshot patterns from the M5GFX demo suite.
LastUpdated: 2026-01-02T09:39:37.682048256-05:00
WhatFor: ""
WhenToUse: ""
---


# Project analysis: System monitor + sparklines (heap/FPS/Wi‑Fi)

This document analyzes how to add a “system monitor” screen to the LVGL demo catalog. The goal is a moderately challenging project that unlocks reusable functionality:

- a consistent way to measure and display **performance** (FPS, frame time),
- visibility into **memory** (heap + DMA heap),
- optional **Wi‑Fi** state and signal health,
- small time-series charts (“sparklines”) suitable for **240×135**.

It should feel like a tiny “top” / “status dashboard” you can keep around while building everything else.

---

## Why this project is high-leverage

Every other demo will eventually benefit from these primitives:

- “Is the UI loop responsive?” (FPS + frame time)
- “Are we leaking memory?” (heap trends)
- “Are we starving DMA heap?” (DMA free)
- “Did Wi‑Fi reconnect?” (state transitions / RSSI)

Also, the act of building this screen forces good habits:

- time budget discipline (don’t block the UI loop to gather metrics)
- correct LVGL lifecycle management (timers must be deleted on screen destruction)
- a data model that can be consumed by both LVGL UI and serial logs/commands later

---

## What already exists in the repo (relevant files + concepts)

### 1) Periodic “status label” pattern in LVGL (already in 0025)

`0025-cardputer-lvgl-demo/main/demo_basics.cpp` already does the simplest form of monitoring:

- gathers memory values:
  - `esp_get_free_heap_size()`
  - `heap_caps_get_free_size(MALLOC_CAP_DMA)`
- updates an LVGL label in a timer callback every 250ms:
  - `lv_timer_create(status_timer_cb, 250, st);`
- cleans up the timer in `LV_EVENT_DELETE`:
  - `lv_timer_del(st->status_timer);`

This is a minimal but *correct* baseline for “screen-local monitoring logic”.

### 2) A more complete perf snapshot model (non-LVGL) in the M5GFX demo suite

`0022-cardputer-m5gfx-demo-suite/main/ui_hud.h` + `ui_hud.cpp` include:

- `FrameTimingsUs`:
  - `update_us`, `render_us`, `present_us`, `total_us`
- `PerfSnapshot`:
  - last timings + heap/dma + EMA averages
- `PerfTracker`:
  - `on_frame_present(FrameTimingsUs)` uses EMA smoothing
  - `sample_memory(heap,dma)`

Even though it’s non-LVGL, the *data model* is exactly what we want:

- it separates measurement from rendering
- it provides smoothed values suitable for tiny displays

`0022-cardputer-m5gfx-demo-suite/main/demo_b2_perf.cpp` shows how to visualize timing breakdown (bars + text) in a small UI.

### 3) Wi‑Fi state transitions via event loop (tutorial 0005)

`0005-wifi-event-loop/main/hello_world_main.c` shows:

- how to register Wi‑Fi and IP event handlers
- how to track connection state and retry behavior

For a system monitor screen, this matters because Wi‑Fi state is best represented as:

- a small state machine updated by events, not by polling

---

## “What to measure” on Cardputer LVGL (practical metrics)

### Memory (must-have)

- Total free heap:
  - `uint32_t esp_get_free_heap_size(void);`
- DMA-capable free heap:
  - `size_t heap_caps_get_free_size(uint32_t caps);` (use `MALLOC_CAP_DMA`)

Why DMA heap matters here:

- LVGL draw buffers attempt DMA allocations (`lvgl_port_m5gfx.cpp` prefers DMA caps)
- M5GFX SPI DMA paths need DMA-capable buffers

If DMA free trends down, you’ll see rendering failures before “heap free” hits zero.

### “FPS” and frame time (must-have, but define it correctly)

In LVGL, there isn’t a single “frame present” hook because rendering is “flush as needed”.

For our purposes, define a frame as:

- one `lv_timer_handler()` iteration that results in at least one `flush_cb` call, or
- simply “one UI loop iteration” (less accurate, but stable and cheap)

We want something that correlates with perceived responsiveness, not a perfect compositor metric.

Pragmatic approach:

- measure wall time around `lv_timer_handler()` and your input/queue processing:

```c++
int64_t t0 = esp_timer_get_time();
// poll keyboard + feed LVGL
// drain control plane
lv_timer_handler();
int64_t t1 = esp_timer_get_time();
uint32_t loop_us = (uint32_t)(t1 - t0);
```

- maintain an EMA of `loop_us` and compute “fps-ish” as `1e6 / avg_loop_us`

This captures:

- UI load (LVGL handler time)
- any blocking you accidentally introduce in the loop

### Wi‑Fi state and RSSI (optional but valuable)

At minimum:

- state: `DISCONNECTED`, `CONNECTING`, `GOT_IP`
- retry count / last disconnect reason

If connected as STA, you can optionally show:

- RSSI via `esp_wifi_sta_get_ap_info(&wifi_ap_record_t)`

Note: Cardputer demos might not be Wi‑Fi-enabled by default. This is still valuable scaffolding because it becomes immediately useful once any demo adds networking.

---

## Proposed architecture: `SystemStats` + “monitor screen”

The reusable part should be a small “stats service” decoupled from any particular screen:

### 1) A snapshot struct

```c++
struct SystemStatsSnapshot {
  uint32_t heap_free;
  uint32_t dma_free;

  uint32_t loop_us_last;
  uint32_t loop_us_avg;   // EMA
  int fps_estimate;       // derived from avg

  // Optional Wi‑Fi
  bool wifi_connected;
  int wifi_rssi;
  int wifi_disconnect_reason;
};
```

### 2) A ring buffer for sparklines

Sparklines are tiny, but they benefit from a stable buffer length:

```c++
static constexpr int kPoints = 48; // ~10–30 seconds depending on sample period
struct Series {
  int head;
  int values[kPoints];
};
```

Keep series for:

- heap_free (scaled to KiB)
- dma_free (scaled)
- loop_us_avg (scaled)

### 3) A sampling cadence

Recommended:

- sample stats every 200–500ms
- update LVGL widgets at the same cadence (or slower)

Why:

- `esp_get_free_heap_size()` is cheap
- LVGL redraw cost is the real budget; don’t update charts every 10ms

### 4) UI thread vs background task

Two safe approaches:

- **All on UI thread** (simplest):
  - `lv_timer` on the system monitor screen calls “sample” and “update UI”
  - no cross-thread anything
- **Background sampler** (more advanced):
  - a FreeRTOS task samples stats and pushes snapshots to a queue
  - UI thread drains queue and updates widgets

Given this repo’s philosophy (“keep it simple unless needed”), start with all-on-UI-thread.

---

## LVGL implementation: sparklines on a tiny display

Two workable ways:

### Option A: `lv_chart` (recommended)

LVGL API shape (conceptual):

```c
lv_obj_t* chart = lv_chart_create(parent);
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
lv_chart_set_point_count(chart, kPoints);
lv_chart_series_t* s = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);

// each sample:
lv_chart_set_next_value(chart, s, value);
```

Why it’s good:

- minimal custom drawing
- handles scrolling/point indexing for you

### Option B: `lv_canvas` (more control, more work)

If we want very custom visuals (tiny 1px sparklines), use:

- `lv_canvas_set_buffer`
- draw polyline points manually

This is more work and adds memory constraints (canvas buffer), so `lv_chart` is a better default.

---

## Screen layout suggestion (fits 240×135)

Example layout:

```
┌──────────────────────────────┐
│ System Monitor               │
│ heap  178k  dma  44k  fps 35 │  ← compact header text
│──────────────────────────────│
│ heap sparkline               │
│ [chart]                      │
│ dma sparkline                │
│ [chart]                      │
│ loop-us sparkline            │
│ [chart]                      │
│ Wi‑Fi: DISCONNECTED          │  ← optional footer
└──────────────────────────────┘
```

This should be keyboard-navigable but doesn’t *need* focusable widgets. It can handle keys at the root:

- `Esc` returns to menu (already global)
- optional: `R` reset chart buffers
- optional: `Space` toggle Wi‑Fi view panel

---

## For a new developer: what the numbers mean

### Heap vs DMA heap

- “heap free” is general-purpose RAM available to `malloc`.
- “DMA free” is a subset of RAM that is safe for DMA transfers (important for display and some peripherals).

Symptoms:

- DMA free exhausted first → graphics becomes unstable (buffer alloc failures, flush issues).
- heap free exhausted first → random `malloc` failures in UI or subsystems.

### Why EMA smoothing (and why 1/8)

Raw `loop_us` can spike (interrupts, flash cache, logging). EMA makes a small display readable.

The repo’s existing `PerfTracker` uses:

- shift=3 → 1/8 smoothing

That’s a good default:

- responsive enough to reflect changes
- stable enough to read

### “FPS” on LVGL isn’t a GPU FPS

It’s a proxy for responsiveness based on loop timing. What matters is:

- whether your input feels laggy
- whether LVGL and flushes keep up under load

Treat it as a regression signal, not a literal frame compositor metric.

---

## v0 metrics + UI (implemented)

This ticket’s v0 System Monitor intentionally stays “UI-thread only” (all sampling and widget updates occur in an `lv_timer` owned by the System Monitor screen) to preserve LVGL’s single-thread constraint.

### Metrics sampled (every ~250ms)

- `heap_free` (KiB): `esp_get_free_heap_size()`
- `dma_free` (KiB): `heap_caps_get_free_size(MALLOC_CAP_DMA)`
- `lvgl_handler_us_last` / `lvgl_handler_us_avg`:
  - measured around `lv_timer_handler()` in the main loop
  - EMA smoothing uses `shift=3` (~1/8)
- `loop_hz` (smoothed):
  - derived from the delta of an `app_main` loop counter over the sample interval
- `fps_est`:
  - `1e6 / lvgl_handler_us_avg` (a responsiveness signal, not literal display FPS)

### Visuals (240×135)

- Header line: compact text showing heap/dma/loop Hz/LVGL handler us/fps
- Three `lv_chart` sparklines:
  - heap KiB (range `0..512`)
  - dma KiB (range `0..128`)
  - fps estimate (range `0..60`)

### Control plane

- The SysMon screen is reachable via:
  - menu entry `SysMon`
  - host REPL command `sysmon` (enqueues a `CtrlEvent`)
  - command palette action “Open System Monitor”

## Implementation checklist (pragmatic)

1. Add a new `DemoId::SystemMonitor` and a new `demo_system_monitor.cpp`.
2. Implement a `SystemMonitorState` with:
   - labels for heap/dma/fps
   - 2–3 `lv_chart` widgets
   - an `lv_timer_t` sampling timer
3. Sample heap/dma and update labels + charts every 250ms.
4. Add optional Wi‑Fi state fields (compile-time optional if Wi‑Fi not enabled).
5. Ensure `LV_EVENT_DELETE` deletes the sampling timer (per lifecycle doc).
