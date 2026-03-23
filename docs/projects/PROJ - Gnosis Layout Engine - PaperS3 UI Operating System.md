---
title: Gnosis Layout Engine for PaperS3 - A UI Operating System on E-Ink
aliases:
  - Gnosis Layout Engine
  - PaperS3 Layout OS
  - Gnosis UI Engine
  - E-Ink Layout Engine
tags:
  - project
  - esp32-s3
  - papers3
  - firmware
  - ui-engine
  - layout
  - e-ink
  - display
status: active
type: project
created: 2026-03-22
repo: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
firmware: 0078-papers3-gnosis-layout
---

# Gnosis Layout Engine for PaperS3 -- A UI Operating System on E-Ink

This project implements a tree-based UI layout engine on the M5Paper S3, a 960x540 e-ink tablet running ESP-IDF on an ESP32-S3. The engine takes a declarative description of a screen -- boxes, labels, gauges, grids, progress bars -- and automatically computes positions, renders widgets, tracks changes, and issues the minimum possible set of e-ink partial refreshes. Screen presets can be switched live over a USB serial console.

The design is inspired by the Gnosis DSL, a JSON-based screen description language prototyped in a React/Canvas web app. The C++ port compiles DSL trees directly into struct initializer lists, skipping JSON parsing entirely and paying zero runtime cost for the layout description itself.

> [!summary]
> The engine has four stages that run every frame:
> 1. **Layout**: a recursive tree walk computes the bounding rectangle of every node
> 2. **Update**: application logic mutates node values (clock, gauges) and marks them dirty
> 3. **Collect + Merge**: dirty leaf rectangles are gathered and merged to minimize EPD calls
> 4. **Render + Refresh**: only the merged dirty regions are redrawn and pushed to the e-ink panel
>
> Seven preset screens demonstrate the system: dashboard with live clock and animated gauges, calendar with event grid, boot sequence with progress bar, telemetry full-screen gauges, book reader layout, widget gallery, and standby mode.

## Why build a layout engine on a microcontroller?

The previous PaperS3 firmwares (0075 through 0077) all followed the same pattern: a class with a `BuildLayout()` method that manually computes pixel positions for every UI element, and a `RenderFullUi()` method that redraws everything to the screen. Adding a button meant changing coordinates in four places. Moving a panel meant cascading arithmetic through a dozen variables. The code worked, but it did not compose.

The deeper problem is that e-ink displays are expensive to update. A full-screen refresh on the PaperS3 takes 300ms to over a second and causes a visible full-screen flash. The only way to make the UI feel responsive is partial refresh: updating small rectangular regions of the display using faster waveforms. But partial refresh means you need to know *exactly which pixels changed*. In the old firmwares, this was done ad-hoc -- specific methods knew their own screen regions and issued targeted refreshes. That approach does not scale to a system with multiple screens, shared widgets, and dynamic data.

A layout engine solves both problems at once. The tree structure defines positions declaratively, so changing a layout means changing the tree, not doing pixel arithmetic. And the dirty-rect tracker gives you precise, automatic knowledge of what changed, so the refresh logic is generic.

## The hardware constraints that shaped the design

The M5Paper S3 is not a smartphone. Understanding its constraints is necessary to understand why the engine works the way it does.

**Display**: 960x540 pixels, electrophoretic (e-ink). Pixels are physical ink particles moved by electric fields. The display is bistable -- it holds its image with zero power. But changing pixels is slow and has visible artifacts. There are multiple "waveform" modes trading speed for quality:

| Mode | EPD enum | Typical latency | Ghosting | When to use |
|---|---|---|---|---|
| Quality | `epd_quality` | 800ms+ | None, full flash | Screen changes, periodic deghosting |
| Text | `epd_text` | ~200ms | Minimal | Partial UI updates with text |
| Fast | `epd_fast` | ~80ms | Noticeable | Live interaction, drawing |
| Fastest | `epd_fastest` | ~50ms | Heavy | Real-time tracking |

The engine uses `epd_text` for partial dirty-rect refreshes during normal operation, and `epd_quality` for periodic full-screen refreshes that clean up accumulated ghosting. That distinction matters -- `epd_text` redraws a region efficiently but does not do the full voltage sweep needed to completely reset the ink particles. Over many partial refreshes, ghost images accumulate. The full-quality refresh fixes this.

**MCU**: ESP32-S3, dual-core 240 MHz, 512 KB SRAM, 8 MB PSRAM. Memory is not the constraint -- PSRAM is abundant. CPU is not the constraint either; the layout pass for 60 nodes takes microseconds. The bottleneck is always the display.

**Touch**: Capacitive touch overlay (GT911 controller), polled via `M5.Touch.getDetail()`. Single-touch only. Adequate for button presses but not for gestures.

## Architecture

The system has four layers:

```
Screen definitions (C++ struct trees)
         |
    Layout engine (recursive position computation)
         |
    Dirty tracker (change detection + rect merging)
         |
    Widget renderer (M5GFX drawing + EPD partial refresh)
```

These layers are cleanly separated. The screen definitions know nothing about pixels. The layout engine knows nothing about e-ink. The dirty tracker knows nothing about widget types. The renderer knows nothing about tree structure beyond "draw this node at this rectangle."

### The node tree

Everything in the UI is a `Node`. A node has a type, optional children, a computed bounding rectangle, and type-specific properties packed into a small array. The struct is about 140 bytes:

```cpp
struct Node {
    NodeType type;          // VBOX, HBOX, FIXED, LABEL, BAR, GAUGE, ...
    Waveform waveform;      // FAST, PART, FULL
    bool dirty;             // Needs redraw?
    Rect rect;              // Computed by layout pass
    Node* children[16];     // Inline child array (no heap allocation)
    uint8_t n_children;
    int16_t props[4];       // Type-specific packed properties
    int16_t explicit_w, explicit_h;  // 0 = flexible
    int16_t offset_x, offset_y;     // For FIXED children
    char text[64];          // For LABEL, BADGE, TEXT_BLOCK
    // ... flags, list data, grid data
};
```

There are only three layout types. Everything else is a leaf:

- **VBOX**: stacks children vertically. Children with `explicit_h` get that exact height; the rest share the remaining space equally.
- **HBOX**: arranges children horizontally. Same flex logic on the width axis, plus a special "split" mode that divides the box into two panes with a 1px divider.
- **FIXED**: places children at explicit `(offset_x, offset_y)` positions relative to the parent. No flow. Used for overlapping elements like crosshairs on top of circles.

Leaf types include LABEL, BAR, GAUGE, LIST, GRID, CIRCLE, CROSS, SEP, DOT, BADGE, ICON, TEXT_BLOCK, and SPACER. Each is rendered by a dedicated draw function.

### Static allocation

On an embedded system you want to avoid heap fragmentation. All nodes come from a `NodePool` -- a flat array of 192 pre-allocated nodes with a bump allocator:

```cpp
class NodePool {
    Node nodes_[192];
    size_t count_ = 0;
public:
    Node* Alloc() {
        if (count_ >= 192) return nullptr;
        Node* n = &nodes_[count_++];
        *n = Node{};
        return n;
    }
    void Reset() { count_ = 0; }
};
```

Switching screens calls `Reset()` and rebuilds from zero. No fragmentation, no leaks, deterministic memory usage. A full dashboard screen uses about 50-60 nodes, or roughly 8 KB.

## The layout algorithm

The layout algorithm is a single recursive tree walk. It runs in O(N) time where N is the number of nodes. For a typical screen with 60 nodes, it completes in under a millisecond -- negligible compared to e-ink refresh times.

### Entry point

A `Screen` has three zones: bar (top), body (middle), nav (bottom). The bar and nav have fixed heights; the body gets whatever remains:

```
LAYOUT-SCREEN(screen, W=960, H=540):
    bar_h  = screen.bar.explicit_h       // e.g. 32
    nav_h  = screen.nav.explicit_h       // e.g. 32
    body_h = 540 - 32 - 32 = 476

    LAYOUT-NODE(bar,  0, 0,   960, 32)
    LAYOUT-NODE(body, 0, 32,  960, 476)
    LAYOUT-NODE(nav,  0, 508, 960, 32)
```

### VBOX: the two-pass flex algorithm

VBOX is the workhorse layout. It runs two passes:

**Pass 1** counts fixed-height children and flexible children:

```
fixed_total = 0
flex_count  = 0
for each child:
    if child.explicit_h > 0:
        fixed_total += child.explicit_h
    else:
        flex_count += 1
```

**Pass 2** distributes remaining space and assigns positions:

```
remaining = h - fixed_total
flex_h = remaining / flex_count    // integer division

cursor_y = y
for each child:
    child_h = child.explicit_h > 0 ? child.explicit_h : flex_h
    LAYOUT-NODE(child, x, cursor_y, w, child_h)
    cursor_y += child_h
```

This is the same algorithm used by CSS flexbox in its simplest form: fixed children get their declared size, flexible children share the remainder equally. No weights, no min/max constraints -- those are unnecessary complexity for an e-ink UI with a known resolution.

### HBOX: flex plus split

HBOX follows the same two-pass pattern on the horizontal axis, but adds a special case. When `props[0]` is non-zero, the box uses a two-pane split:

```
if node.props[0] > 0:    // split mode
    split_w = props[0]
    LAYOUT-NODE(children[0], x, y, split_w, h)
    LAYOUT-NODE(children[1], x + split_w + 1, y, w - split_w - 1, h)
    return
```

The 1px gap between panes is rendered as a vertical divider line. This split pattern appears in almost every Gnosis screen: the dashboard splits clock/compass from telemetry, the calendar splits the grid from the agenda, the reader splits the library sidebar from the reading pane.

For the general case, HBOX also handles SPACER nodes -- invisible flex items that absorb remaining width, pushing other elements apart. A typical status bar uses spacers:

```
[LABEL "GNOSIS//3.1"] [SPACER] [LABEL "SIG:97%"] [LABEL "PWR:EINK" w=100] [DOT w=24]
```

The label widths are computed from text length times glyph width. The spacer absorbs everything between the left-aligned title and the right-aligned status indicators.

### FIXED: escape hatch for free positioning

FIXED containers do no flow calculation. Each child carries explicit offsets and is placed relative to the parent's origin:

```
for each child:
    cx = parent.x + child.offset_x
    cy = parent.y + child.offset_y
    cw = child.explicit_w or (parent.w - child.offset_x)
    ch = child.explicit_h or (parent.h - child.offset_y)
    LAYOUT-NODE(child, cx, cy, cw, ch)
```

FIXED is used inside split panes for precise placement of labels, gauges, circles, and separators. The dashboard's left pane, for example, is a FIXED container with a clock label at (16, 44), compass circles at (240, 360), and a section header at (16, 210).

## Widget rendering

Each leaf node type has a dedicated render function. The renderer walks the tree, culls nodes that don't intersect the current dirty region, and calls the appropriate draw function. A few examples illustrate the approach.

### Bitmap font

The engine includes a custom 5x7 pixel bitmap font for the authentic Gnosis aesthetic. Each character is stored as seven bytes, each byte encoding a 5-bit row:

```cpp
// 'A' in the 5x7 font
{ 0b01110,    //  ###
  0b10001,    // #   #
  0b10001,    // #   #
  0b11111,    // #####
  0b10001,    // #   #
  0b10001,    // #   #
  0b10001 }   // #   #
```

The renderer supports size multipliers (1x, 2x, 4x). At size 1, each bit maps to one pixel. At size 4, each bit becomes a 4x4 block, producing large chunky text like a clock display. The font covers ASCII 0x20 through 0x60 (space through backtick), with lowercase mapped to uppercase bitmaps.

Drawing a character at size 1:

```cpp
for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
        if (bitmap[row] & (1 << (4 - col))) {
            display.drawPixel(gx + col, y + row, color);
        }
    }
}
```

At size 4, each pixel becomes a `fillRect(gx + col*4, y + row*4, 4, 4, color)`. This is not the fastest possible approach -- a scanline buffer would be faster -- but on this hardware the bottleneck is always the e-ink refresh, never the CPU rendering.

### Gauge widget

The GAUGE is a compact compound widget that packs a label, a numeric value, and a progress bar into a single row:

```
R 015 [========        ]
```

The rendering sequence:

```cpp
void DrawGauge(M5GFX& display, Node* node) {
    // Label character (e.g. "R") in dim color
    DrawBitmapText(display, node->text, node->rect.x, node->rect.y, 1, COLOR_MID);

    // Zero-padded value
    char buf[8];
    snprintf(buf, sizeof(buf), "%03d", node->props[1]);
    DrawBitmapText(display, buf, node->rect.x + 2*GLYPH_W, node->rect.y, 1, COLOR_FG);

    // Progress bar
    int bar_x = node->rect.x + 7 * GLYPH_W;
    int bar_w = node->rect.w - 7 * GLYPH_W - 4;
    display.fillRect(bar_x, node->rect.y + 3, bar_w, 3, COLOR_LIGHT);  // track
    int fill_w = bar_w * min(cur, max) / max;
    display.fillRect(bar_x, node->rect.y + 3, fill_w, 3, COLOR_FG);    // fill
}
```

Six of these stacked vertically in a FIXED container produce the telemetry panel from the Gnosis dashboard. Each gauge node is about 140 bytes. Changing a gauge value is a single assignment to `props[1]` followed by `MarkDirty(node)`.

### Grid widget (calendar)

The GRID renders a table of cells, used for the calendar "Temporal Map" screen. It supports a highlighted "today" cell (inverted colors) and event dots:

```cpp
for (int i = 0; i < count; i++) {
    int col = i % cols;
    int row = i / cols;
    int gx = rect.x + col * cell_w;
    int gy = rect.y + row * cell_h;

    display.drawRect(gx, gy, cell_w, cell_h, COLOR_LIGHT);

    if (i == grid_today) {
        display.fillRect(gx+1, gy+1, cell_w-2, cell_h-2, COLOR_FG);
        DrawBitmapText(display, dayStr, gx + cell_w - 18, gy + 3, 1, COLOR_BG);
    } else {
        DrawBitmapText(display, dayStr, gx + cell_w - 18, gy + 3, 1, COLOR_FG);
    }

    if (grid_events & (1u << i)) {
        display.fillRect(gx + cell_w/2 - 1, gy + cell_h - 4, 3, 3, COLOR_FG);
    }
}
```

Event days are stored as a 32-bit bitmask, which is compact enough to fit in a node's existing fields. The "today" index is a single `int8_t`.

## Dirty region tracking

This is the subsystem that makes the engine practical on e-ink. Without it, every data update would trigger a full-screen refresh.

### The problem

An e-ink partial refresh typically takes 50-200ms depending on the waveform and the size of the region. A full-screen refresh takes 300ms to over a second. If you update six gauges every second and do a full refresh each time, the display spends its entire time flashing.

The solution: track exactly which nodes changed, compute the minimal set of screen rectangles that cover the changes, and refresh only those regions.

### Marking dirty

When application logic changes a node's data, it calls `MarkDirty(node)`. This sets a single boolean flag:

```cpp
void MarkDirty(Node* node) {
    node->dirty = true;
}
```

That's it. No propagation to parents, no event queue, no observer pattern. The flag is checked later during collection.

### Collecting dirty rectangles

The collector walks the tree looking for dirty leaf nodes:

```cpp
void DirtyCollector::Collect(Node* node) {
    if (node->dirty) {
        if (node->n_children == 0) {
            // Leaf: record its bounding rectangle
            rects[count] = node->rect;
            waveforms[count] = node->waveform;
            count++;
            node->dirty = false;
        } else {
            // Container: recurse into children
            for (auto* child : node->children)
                Collect(child);
            node->dirty = false;
        }
    } else {
        // Check for dirty children under a clean parent
        for (auto* child : node->children)
            if (child->dirty) Collect(child);
    }
}
```

The key insight is the third case: a clean parent can have dirty children. This happens when a FIXED container's label changes but its sibling circle does not. The collector still finds the dirty label without forcing the entire container to be redrawn.

### Merging rectangles

If two gauges 30 pixels apart both change, it's cheaper to refresh one 200-pixel-wide rectangle than to issue two separate 60-pixel EPD refreshes. The merger uses a greedy algorithm:

```
repeat:
    for each pair (i, j) of dirty rects:
        union = union_rect(rects[i], rects[j])
        waste = union.area - rects[i].area - rects[j].area
        if waste < THRESHOLD (1024 px^2):
            merge them
            restart
until no merges possible
```

The threshold of 1024 square pixels prevents merging distant small regions into one huge rectangle. For two 10x10 labels 50 pixels apart, the waste would be about 500 pixels -- below threshold, so they merge. For a label in the top-left and a gauge in the bottom-right, the waste would be hundreds of thousands of pixels -- above threshold, so they stay separate.

The waveform for a merged rectangle is the *worst* (slowest, highest quality) waveform of any constituent rectangle. This is conservative -- it prevents ghosting artifacts by never using a fast waveform where a slow one was requested.

### Refresh dispatch

After merging, the engine issues one EPD partial refresh per merged rectangle:

```cpp
for each merged rect r:
    M5.Display.setEpdMode(waveform_to_epd_mode(r.waveform));
    M5.Display.startWrite();
    M5.Display.setClipRect(r.x, r.y, r.w, r.h);
    M5.Display.fillRect(r.x, r.y, r.w, r.h, COLOR_BG);  // clear region

    RenderSubtree(screen.bar, r);   // draw only intersecting nodes
    RenderSubtree(screen.body, r);
    RenderSubtree(screen.nav, r);

    M5.Display.clearClipRect();
    M5.Display.endWrite();
```

The `setClipRect` call is critical. It tells M5GFX to ignore any drawing outside the rectangle, which both prevents visual artifacts and lets the EPD controller optimize the refresh to only the clipped region.

### Periodic deghosting

After 60 partial refreshes, the engine does a full-quality refresh:

```cpp
M5.Display.setEpdMode(epd_mode_t::epd_quality);
M5.Display.startWrite();
M5.Display.fillScreen(COLOR_BG);
// ... render entire tree ...
M5.Display.endWrite();
M5.Display.waitDisplay();
```

The `epd_quality` waveform does a true full-flash cycle -- the screen briefly inverts to black and back -- which resets all ink particles and eliminates accumulated ghosting. This is visually disruptive, which is why it only happens periodically rather than on every update.

## Screen definitions as C++ struct trees

The original Gnosis engine used a JSON DSL:

```json
{
  "type": "hbox",
  "split": 200,
  "items": [
    { "type": "label", "label": "14:37", "size": 4 },
    { "type": "gauge", "label": "R", "value": 15, "max": 360 }
  ]
}
```

Parsing JSON on an ESP32 is possible but wasteful -- it requires a heap-allocated parse tree, string comparisons for type dispatch, and careful error handling. Since the screens are static, we can do better.

The C++ port uses builder functions that read like the DSL but compile to direct struct initialization:

```cpp
Screen BuildDashboard(NodePool& pool) {
    Screen s;

    s.bar = HBox(pool, {
        Label(pool, "GNOSIS//3.1"),
        Spacer(pool),
        Label(pool, "SIG:97%", 1, /*color=mid*/ 1),
        Label(pool, "PWR:EINK", 1, 1, 0, 0, 100),
        Dot(pool, 24),
    }, /*h=*/ 32);
    s.bar->border_b = true;

    Node* left = Fixed(pool, {
        Label(pool, "CHRONO", 1, 2, 16, 12),
        Label(pool, "14:37", 4, 0, 16, 44),
        // ...
    });

    Node* right = Fixed(pool, {
        Label(pool, "TELEMETRY", 1, 2, 16, 12),
        Gauge(pool, "R", 15, 360, 16, 44, 420),
        Gauge(pool, "P", 34, 360, 16, 72, 420),
        // ...
    });

    s.body = HBoxSplit(pool, 480, left, right);
    s.nav = BuildNavBar(pool);
    return s;
}
```

The `initializer_list`-based API makes adding new screens mechanical: describe the tree, call the builder, return the screen. The seven presets in the firmware were each written in under a minute once the builder helpers existed.

## Console REPL for live preset switching

The firmware runs an `esp_console` REPL on the USB Serial/JTAG port. This lets you switch screens, inspect engine state, and poke gauge values without reflashing:

```
gnosis> list
available presets:
  [0] dashboard (active)
  [1] calendar
  [2] boot
  [3] gallery
  [4] telemetry
  [5] reader
  [6] minimal
nodes used: 58 / 192

gnosis> switch calendar
gnosis: switching to preset 'calendar'

gnosis> gauge R 270
gnosis: gauge R = 270

gnosis> refresh
gnosis: full refresh done
```

The console runs on a dedicated FreeRTOS task so it does not block the display update loop. Commands mutate the shared `GnosisApp` state, and the main loop picks up changes on the next iteration.

Short aliases (`list`, `switch`, `refresh`) are registered so you don't have to type `gnosis` every time. The `switch` command accepts both names and numeric indices.

## The main loop

The application loop follows the same pattern as all previous PaperS3 firmwares:

```cpp
void GnosisApp::Run() {
    InitBoard();           // M5.begin(), rotation, font
    BuildCurrentScreen();  // Allocate nodes, run layout
    FullRefresh();         // First render to EPD

    while (true) {
        M5.update();            // Poll touch + buttons
        HandleTouch();          // Nav bar icon -> switch screen
        UpdateData();           // Animate clock, gauges (1 Hz)
        ProcessDirtyRefresh();  // Collect dirty rects, render, EPD refresh
        M5.delay(16);           // ~60 Hz loop
    }
}
```

The loop runs at 60 Hz but the display update rate is much lower -- limited by EPD refresh latency. `ProcessDirtyRefresh()` is a no-op when nothing is dirty, so idle power is minimal.

`UpdateData()` runs once per second. It updates the dashboard clock from the system uptime, increments gauge values to create a rotating animation, and advances the boot progress bar. Each changed node gets a `MarkDirty()` call. On the next loop iteration, `ProcessDirtyRefresh()` picks up those dirty flags and issues targeted partial refreshes.

Touch handling is simple: if the user taps a nav bar icon, the corresponding shape index maps to a preset. If they tap the "AUTO" badge, it cycles to the next preset. Screen switches call `pool_.Reset()`, rebuild the tree, and do a full-quality refresh.

## Important files

The implementation lives in `0078-papers3-gnosis-layout/main/`:

| File | Purpose |
|---|---|
| `gnosis_types.h` | `Rect`, `Node`, `NodePool`, `Screen`, `NodeType` enum |
| `layout_engine.cpp` | Recursive VBOX/HBOX/FIXED layout, `LayoutScreen()` |
| `bitmap_font.cpp` | 5x7 pixel font data and `DrawBitmapText()` |
| `widget_renderer.cpp` | Draw functions for all 16 widget types |
| `dirty_tracker.cpp` | `DirtyCollector`, `MarkDirty()`, `MergeRects()` |
| `node_builder.h` | `VBox()`, `HBox()`, `Label()`, `Gauge()`, etc. |
| `screens.cpp` | Seven preset screen builders + preset registry |
| `gnosis_app.cpp` | Main loop, touch handling, data animation |
| `gnosis_console.cpp` | `esp_console` REPL with `list`, `switch`, `refresh`, `gauge` |
| `app_main.cpp` | Entry point: starts console task + UI task on core 1 |

## What this enables next

The layout engine is a foundation, not an endpoint. With the tree structure and dirty tracking in place, several extensions become straightforward:

- **Data binding**: instead of manually calling `MarkDirty()`, nodes could reference named variables. When a variable changes, all bound nodes are automatically marked dirty.
- **Touch-aware widgets**: buttons and list items could detect taps and fire callbacks, enabling interactive multi-screen applications.
- **Screen transitions**: switching screens could cross-fade or slide, using the fast waveform for intermediate frames.
- **JSON loading over serial**: accepting Gnosis JSON over the console and parsing it into the node tree would allow live prototyping without reflashing.
- **WASM scripting**: combining this engine with a WebAssembly runtime (see `0079-papers3-wamr-assemblyscript-console`) could allow scripted screen definitions and data sources.

The core insight -- that a UI on e-ink must be change-aware to avoid expensive full refreshes -- will remain relevant regardless of which direction the project takes.

## Relationship to prior work

This firmware builds directly on the patterns established in:

- `0075-papers3-touch-draw-demo`: proved touch input + live EPD fast-mode drawing
- `0076-papers3-protractor-trainer`: established card-based UI layout with manual coordinates. See [[PROJ - Glyph Protractor Algorithm - PaperS3 Handwriting Recognition]]
- `0077-papers3-alphabet-graffiti`: introduced deferred rendering, mode switching, and the Lain/Navi visual theme

The Gnosis layout engine generalizes the manual coordinate approach from 0076/0077 into a reusable, declarative system. The rendering code, EPD mode management, and touch handling patterns are direct descendants of those earlier projects.
