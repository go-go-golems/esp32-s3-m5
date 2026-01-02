---
Title: 'Demo firmware: real-world scenario catalog'
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
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Multi-sprite layout inspiration for header/content/footer
    - Path: 0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: Baseline pattern for full-screen canvas + waitDMA
    - Path: 0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Terminal-style UI reference for scrolling/log console
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/analysis/01-m5gfx-functionality-inventory-demo-plan.md
      Note: API and capability inventory this scenario catalog builds upon
ExternalSources: []
Summary: Scenario-driven catalog of real-world demos to include in the Cardputer M5GFX demo firmware (UI patterns, tools, charts, media, productivity), with implementation sketches and ordering.
LastUpdated: 2026-01-01T20:00:12.107184658-05:00
WhatFor: ""
WhenToUse: ""
---



# Demo firmware: real-world scenario catalog

This document proposes an exhaustive set of **real-world, copy/paste-relevant demos** for a Cardputer firmware that showcases M5GFX/LovyanGFX. The emphasis is on “things you will actually build” (UIs, dashboards, tools, viewers) rather than a tour of individual APIs.

Where this fits in the ticket:

- The underlying capability and API inventory lives in `analysis/01-m5gfx-functionality-inventory-demo-plan.md`.
- This document picks demos based on **practical scenarios**, and then maps each scenario to the smallest set of graphics techniques needed to implement it well.

## Goals

- Provide a firmware that a developer can browse on-device and immediately say: “This is exactly how I build X”.
- Prefer demos that are reusable as patterns: status bars, lists, graphs, image viewers, consoles, overlays.
- Make each demo self-explanatory on-device: show a description, controls, and a “what to steal” note.
- Keep everything keyboard-first (Cardputer) and stable under ESP-IDF.

## Constraints and assumptions

- Device: M5 Cardputer (ESP32-S3, 240×135 landscape visible region, keyboard input, typically no touch).
- Display init should be via M5GFX autodetect (`m5gfx::M5GFX display; display.init();`) to inherit correct offsets, rotation, and backlight PWM.
- Each demo must be able to run offline; “network-enabled” demos are optional extras.
- Rendering should be flicker-free and predictable: prefer a canvas/sprite present loop for complex screens.

## Non-goals

- Not an app framework. The demo suite can have a minimal scene manager, but it should not become a general UI toolkit.
- Not a full port of `M5Cardputer-UserDemo` apps; we can learn from them, but this is a smaller, documentation-driven showcase firmware.

## Design principles (what makes these demos useful)

1. **Scenario-first**: start from a user story (“I need a live dashboard”) and show a complete screen.
2. **Small, stealable patterns**: each demo should contain one or two ideas worth copying, not ten.
3. **Instrumentation built-in**: each demo should show frame time, memory, and whether it’s using a full-screen sprite.
4. **Input conventions**: consistent keys across demos (Back, Help, Next/Prev, Pause).
5. **Visual correctness matters**: show boundaries, anchors, clipping areas, and layout grids when relevant.

## Firmware UX (navigation and interaction)

The firmware should behave like a “catalog app” with predictable navigation.

### Top-level structure

```text
Home (categories) -> Demo list -> Demo details (help/controls) -> Running demo
                                       |
                                       +-> "Copy/paste notes" page (the key takeaways)
```

### On-screen chrome (recommended)

- **Header bar**: demo name, category, current FPS/frame-time, heap/dma free, battery (optional).
- **Footer bar**: key bindings (contextual): `Esc Back`, `Tab Next`, `Shift+Tab Prev`, `H Help`, `Space Pause`.
- **Help overlay**: a semi-transparent panel describing:
  - scenario
  - what the demo teaches
  - controls

### Input mapping (keyboard-only)

Keep this consistent everywhere:

- Navigation: `Up/Down` select, `Enter` open, `Esc` back
- Demo switching: `Tab` next demo, `Shift+Tab` previous demo
- Pause: `Space`
- Help: `H`
- Screenshot: `P` (prints PNG to serial; optional save to storage later)

## Demo catalog (scenario-driven, exhaustive)

Each demo below includes:

- **Scenario**: what real project this maps to
- **User story**: the “why”
- **What it demonstrates**: concrete techniques you’d reuse
- **Implementation sketch**: how to build it in a stable way
- **Notes**: risks/perf/asset strategy

### Category A: Daily-driver UI building blocks

These are the demos you reference constantly when building actual UIs.

#### A1) Status bar + HUD overlay (battery / connectivity / time)

Scenario: building any “tool-like” firmware that must show persistent state (battery, wifi, recording status).

User story: “I want a consistent top bar across screens without redrawing the entire UI.”

What it demonstrates:

- Multi-sprite layout: separate sprites for status bar and content
- Iconography with 1-bit bitmaps (tiny assets)
- Minimal redraw: only update the bar when values change

Implementation sketch:

```text
Sprite: status_bar (240x16)
Sprite: content     (240x119)
On change: redraw status_bar only and push it.
```

Pseudocode:

```cpp
if (status_dirty) {
  status.fillSprite(TFT_BLACK);
  draw_battery_icon(status, level);
  status.drawString(ssid_or_mode, 60, 8);
  status.pushSprite(0, 0);
}
```

Notes:

- This is the pattern used in spirit by `M5Cardputer-UserDemo`’s multi-sprite HAL.
- Even if the main screen is full-screen animated, you can “overlay” a status bar sprite last.

#### A2) List view + selection (menus, settings)

Scenario: settings pages, app launchers, file pickers.

User story: “I need a fast, readable list that scrolls smoothly and highlights selection.”

What it demonstrates:

- Scroll rect + `scroll(dy)` to move list content cheaply
- Clip rect for partial redraw
- Selection highlight and focus handling

Implementation sketch:

```text
Viewport: list area
Use scrollRect to scroll by one row height when selection moves past edges.
```

Pseudocode:

```cpp
display.setScrollRect(x, y, w, h);
if (move_down) {
  display.scroll(0, -row_h);
  draw_row_at_bottom(new_item);
}
```

Notes:

- This demo should explicitly compare two approaches:
  - full redraw of the list area via sprite
  - scrollRect+incremental row redraw

#### A3) Modal dialog + confirmation (dangerous action)

Scenario: “Delete config?”, “Overwrite file?”, “Factory reset?”.

User story: “I need a modal that blocks input to the underlying screen and is visually obvious.”

What it demonstrates:

- Dimmed overlay (alpha-like effect simulated by drawing a translucent-looking rectangle pattern, since we may not have true alpha in base primitives)
- Focus trapping (keys only affect the dialog)
- Button-like affordances (drawn manually, or using `LGFX_Button` with keyboard simulation)

Implementation sketch:

```text
Render underlying screen to sprite once -> keep it.
When modal opens: draw overlay + dialog box to another sprite and push.
```

Pseudocode:

```cpp
canvas.pushSprite(0, 0);           // base UI
modal.draw_to(canvas);             // overlay + dialog
canvas.pushSprite(0, 0);           // present combined
```

#### A4) Toast notifications (non-blocking)

Scenario: “Saved”, “Connected”, “Low battery”, “Copied to clipboard”.

User story: “I want a quick confirmation that doesn’t interrupt flow.”

What it demonstrates:

- Small overlay sprite with time-based fade-out (emulated via dithering or by shrinking)
- Z-ordering: draw toast last
- State machine for timed UI

Notes:

- A good trick: render a toast sprite once, then move it up/down for a “slide-in” effect.

#### A5) Progress + loading patterns (download, scan, compute)

Scenario: wifi scan, BLE scan, SD card indexing, long compute tasks.

User story: “I want the UI to remain responsive with a clear indicator of progress and liveness.”

What it demonstrates:

- `M5GFX::progressBar` (if we choose to showcase it)
- Indeterminate progress (spinner, barber pole)
- Cooperative update loop (don’t block rendering)

Implementation sketch:

```cpp
draw_progress_frame(pct, phase_name);
if (!pct_known) draw_spinner(t);
```

#### A6) Form entry (text input field, toggles, sliders)

Scenario: entering Wi-Fi password, device name, MQTT broker URL, calibration values.

User story: “I need a clean text field and a slider that feels good with keyboard input.”

What it demonstrates:

- Text field cursor rendering, selection highlighting (optional)
- Slider and numeric input patterns
- Layout grid to maintain consistent spacing

Notes:

- Even if we don’t implement a full text editor, we can show a minimal, reliable “single-line input”.

### Category B: Diagnostics and developer tools

These demos turn M5GFX into a debugging instrument.

#### B1) “Hello display” with calibration grid (bring-up sanity)

Scenario: first time you bring a display up, you need to validate mapping and offsets.

User story: “I need to confirm the visible region, rotation, and offsets are correct.”

What it demonstrates:

- Grid with labeled coordinates and margin markers
- Safe colors and readable text
- Shows the “safe UI area” rectangle used in other demos

Implementation sketch:

```text
Draw a 10px grid, label corners, draw a 1px border exactly at edges.
```

#### B2) Frame-time + perf overlay (when things get slow)

Scenario: animations stutter; you need to know if it’s draw time, DMA, or logic.

User story: “I want a minimal profiler: frame ms, present ms, heap/dma free, dirty count.”

What it demonstrates:

- Measuring frame phases (update/render/present)
- Displaying perf counters without making perf worse (small overlay)

Notes:

- This should be integrated into all demos, but also exists as a standalone demo that stresses primitives.

#### B3) Screenshot to serial (bug report generator)

Scenario: remote debugging without a camera; share a screenshot in an issue.

User story: “I need a reproducible way to capture what the device displayed.”

What it demonstrates:

- `createPng(...)` and streaming bytes over USB-Serial-JTAG
- A framing protocol for the host (start marker, length, end marker)

Pseudocode:

```cpp
size_t len = 0;
void* png = display.createPng(&len, 0, 0, display.width(), display.height());
print_png_to_serial(png, len);
free(png);
```

Notes:

- This is directly useful for the demo firmware itself (capture reference images for docs).

#### B4) Pixel inspector / magnifier (readback correctness)

Scenario: verifying color conversion and drawing correctness (especially with palettes).

User story: “I want to move a cursor and see the exact RGB565/RGB888 values.”

What it demonstrates:

- `readPixel(...)` and `readPixelRGB(...)`
- Cursor overlay and zoom box rendering

Implementation sketch:

```text
Small zoom window: readRect around cursor -> scale up (nearest neighbor) into sprite.
```

#### B5) Memory fragmentation / allocation stress (sprite lifecycle)

Scenario: long-running UIs that create/destroy sprites or decode images repeatedly.

User story: “I want to know if repeatedly decoding PNG or recreating sprites leaks or fragments memory.”

What it demonstrates:

- Create/destroy sprites repeatedly
- Decode PNG/QOI in a loop, optionally call `releasePngMemory` at defined times
- Heap/dma counters over time

Notes:

- This demo is not pretty; it’s a “burn-in” tool.

### Category C: Visualization patterns (dashboards and charts)

These demos show how to display information cleanly and efficiently.

#### C1) Live line chart (sensor history)

Scenario: temperature logger, battery discharge curve, signal strength over time.

User story: “I need a smooth scrolling chart with axes and a legend.”

What it demonstrates:

- Ring buffer of samples + incremental redraw
- Clip rect for chart area
- Optional scrollRect for “move left” effect

Implementation sketch:

```text
Maintain N samples.
Each tick: shift chart left by 1px, draw one new vertical column at right.
```

Pseudocode:

```cpp
display.setScrollRect(chart_x, chart_y, chart_w, chart_h);
display.scroll(-1, 0);
draw_new_sample_column(chart_x + chart_w - 1, sample);
```

#### C2) Bar chart + sparklines (quick comparisons)

Scenario: wifi RSSI per AP, task CPU usage, BLE device signal list.

User story: “I want an at-a-glance comparison with minimal clutter.”

What it demonstrates:

- Consistent scale mapping
- Highlight selected bar
- Small sparklines next to list items (mini history)

#### C3) Gauge / ring progress (status indicators)

Scenario: progress, volume, brightness, “health meters”, battery, timers.

User story: “I want a radial gauge that looks good and animates smoothly.”

What it demonstrates:

- `fillArc` and smooth shapes
- Animated transitions with easing

Notes:

- The “brightness” demo should use this gauge as the primary UI (not just printing a number).

#### C4) Heatmap / grid (keyboard matrix, occupancy map)

Scenario: visualizing which keys are pressed, scanning matrix debugging, or mapping a sensor array.

User story: “I need a clear visualization of a 2D matrix that updates quickly.”

What it demonstrates:

- Fast cell redraw using `fillRect`
- Palette-based coloring (optional)
- Stable layout with labels

### Category D: Media and asset handling (things you ship)

These demos show how to render the assets real firmware tends to contain.

#### D1) Image viewer (zoom/pan/fit, multiple formats)

Scenario: logo display, QR poster, showing diagrams or photos from storage, splash screens.

User story: “I need to render images reliably and place them correctly with scaling and cropping.”

What it demonstrates:

- BMP/JPG/PNG/QOI decode (embedded assets first)
- Fit modes: fit, fill, 1:1, center-crop
- Alpha compositing for PNG (over checkerboard)

Implementation sketch:

```text
Mode: Fit -> compute scale, compute offsets, call drawPng/drawJpg with maxWidth/maxHeight/offX/offY/scale.
```

Notes:

- Even if the final firmware uses SD card, embed at least one sample of each format so the demo suite works on day one.

#### D2) Icon pack demo (1-bit + palette-tinted)

Scenario: status icons, small UI glyphs, consistent style.

User story: “I want an icon system that is tiny and themeable.”

What it demonstrates:

- 1-bit bitmaps via `drawBitmap/drawXBitmap`
- Theme switching by changing foreground/background colors
- Optional paletted sprite for icon atlas caching

#### D3) Animated “splash” or “boot” screen (brand-quality)

Scenario: product-like firmware that needs a pleasant boot sequence.

User story: “I want a short animation that looks smooth and doesn’t tear.”

What it demonstrates:

- Full-screen sprite animation pattern with `waitDMA`
- Simple compositing (text + logo + gradient background)

Notes:

- Keep it tasteful: 1–2 seconds, then fade into the menu.

### Category E: Interaction and productivity (keyboard-first workflows)

These demos show how to build useful workflows on Cardputer specifically.

#### E1) Terminal/log console (scrollback, copy mode)

Scenario: serial terminal, debug console, chat log, REPL output.

User story: “I want a readable terminal with smooth scrollback and fast append.”

What it demonstrates:

- ScrollRect-based line scrolling
- Monospace font selection + line wrapping strategy
- Cursor and selection highlight (optional)

Implementation sketch:

```text
Text area is a scrollRect.
Append line: scroll up by line height, draw new line at bottom.
```

Notes:

- This is the “most real” demo in the suite; it should be robust and reusable.

#### E2) Command palette (fuzzy search + actions)

Scenario: power users need fast navigation (“Open Settings”, “Toggle Wi-Fi”, “Run Self-Test”).

User story: “I want a searchable action list like modern apps.”

What it demonstrates:

- Text input field + list view
- Incremental filtering and highlighting
- Keyboard shortcuts and state transitions

#### E3) Notes scratchpad (minimal editor)

Scenario: jotting quick notes, capturing logs, writing short messages.

User story: “I want a simple editor that feels stable and doesn’t lose text.”

What it demonstrates:

- Text layout, cursor navigation, insert/delete
- Partial redraw strategy (either sprite full redraw or scrollRect)

Notes:

- Keep it intentionally minimal: this is a demo of UI stability, not a full editor.

#### E4) “Keyboard visualizer” (typing trainer / keymap debug)

Scenario: verifying key mapping and scan reliability; helps developers and users.

User story: “I want to see keypresses as they happen, including modifiers.”

What it demonstrates:

- Matrix heatmap (ties into C4)
- Event log at bottom
- Modifier state display (shift/ctrl/alt)

### Category F: Device control and power (real firmware concerns)

#### F1) Brightness + sleep/wakeup (battery friendly UI)

Scenario: any handheld device needs brightness and power controls.

User story: “I want a brightness UI that feels good and a sleep toggle that is safe.”

What it demonstrates:

- `setBrightness(...)`, `sleep()`, `wakeup()`, optional `powerSave(...)`
- Ring gauge UI for brightness
- Persistent setting storage (optional; not required for the demo)

#### F2) “Always-on” clock face (low update rate)

Scenario: standby mode that updates once per second/minute.

User story: “I want a low-power display mode with minimal redraw.”

What it demonstrates:

- Dirty-region redraw only
- Large text rendering and centering (datum)
- Avoiding full-screen redraws

Notes:

- This is a concrete example of why “full-screen sprite redraw at 60 FPS” is not always appropriate.

### Category G (optional): Connectivity-driven scenarios that still teach graphics

These are optional; the key is that the value is still mostly a UI pattern, not networking.

#### G1) Wi-Fi scan UI (list + live updating RSSI bars)

Scenario: pairing or setup flows.

User story: “I need a list of networks with signal strength and a refresh indicator.”

What it demonstrates:

- List view (A2) + bar chart (C2) + progress pattern (A5)
- Live updates and consistent sorting

Notes:

- If this is too much scope for “graphics demo”, stub the data (fake AP list) but keep the UI identical.

#### G2) QR “share this device” (pairing code)

Scenario: pairing device with phone/PC.

User story: “I want to display a QR code with a URL/token and instructions.”

What it demonstrates:

- QR code rendering
- Text layout with a safe margin and clear instructions

## Cross-cutting implementation patterns (what every demo should reuse)

### Pattern 1: Stable full-screen present loop

Use a full-screen canvas for complex UIs:

```cpp
canvas.fillScreen(TFT_BLACK);
render_ui(canvas);
canvas.pushSprite(0, 0);
display.waitDMA();
```

### Pattern 2: Layered sprites (chrome + content)

Use separate sprites for header/footer so the content can be swapped without re-rendering chrome:

```text
Push order each frame:
  1) content
  2) header
  3) footer
  4) transient overlays (toast/help/modal)
```

### Pattern 3: “Teach mode” overlays

Every demo should be able to toggle a “teach mode” overlay that draws:

- bounding boxes
- anchor points
- clip rect outlines
- a note about the primary technique (e.g., “scrollRect incremental update”)

This dramatically increases real-world usefulness because it turns visuals into explanations.

## Assets strategy (practical defaults)

To keep demos reliable under ESP-IDF:

- Embed small assets (icons, sample images) as arrays in flash for the base build.
- If/when we add storage support, allow optional “load from SD” but keep embedded fallbacks.

Minimum embedded assets:

- 1-bit icon sheet for status bar (battery, wifi, warning, play/pause)
- One sample image for each decode path we intend to demo: BMP, JPG, PNG (with alpha), QOI

## What “done” looks like (acceptance criteria)

- The firmware launches into a menu and can be navigated entirely with the keyboard.
- Each demo has:
  - a one-paragraph scenario description
  - a help overlay showing controls
  - a stable render loop (no flicker/tearing for sprite-based demos)
  - visible evidence of the technique (e.g., scrolling region outline)
- The “developer tools” demos (screenshot, perf overlay, pixel inspector) work reliably, since they help debug everything else.

## Implementation order (to maximize utility early)

1. Demo shell: menu, help overlay, consistent key handling
2. A2 List view (because it powers navigation patterns)
3. A1 Status bar overlay (because it’s cross-cutting)
4. E1 Terminal console (most reusable real app pattern)
5. C1 Live chart (common “device dashboard” need)
6. D1 Image viewer (asset rendering patterns)
7. B3 Screenshot (so we can capture results easily)
