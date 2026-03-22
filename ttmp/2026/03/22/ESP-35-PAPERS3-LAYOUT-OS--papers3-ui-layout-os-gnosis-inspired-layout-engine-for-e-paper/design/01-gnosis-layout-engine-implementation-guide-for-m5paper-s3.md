---
Title: Gnosis Layout Engine - Implementation Guide for M5Paper S3
Ticket: ESP-35-PAPERS3-LAYOUT-OS
Status: active
Topics:
    - papers3
    - display
    - esp-idf
    - esp32s3
    - ui
    - layout-engine
    - e-paper
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti/main/alphabet_app.h:Reference for Rect struct, app lifecycle, M5GFX API usage"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti/main/alphabet_app.cpp:Reference for BuildLayout, rendering, EPD refresh modes, touch handling"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0076-papers3-protractor-trainer/main/trainer_app.cpp:Reference for protractor UI layout, card-based rendering"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti/CMakeLists.txt:Build system reference"
ExternalSources: []
Summary: "Complete implementation guide for porting the Gnosis DSL-driven layout engine to the M5Paper S3 (960x540 EPD) as an ESP-IDF 5.3.4 C++ firmware, including tree-based layout, widget rendering, dirty-rect tracking, and partial EPD refresh."
LastUpdated: 2026-03-22T10:15:04.208374258-04:00
WhatFor: "Intern onboarding document: explains every subsystem of the Gnosis layout engine in detail so a new developer can understand, build, and extend it."
WhenToUse: "When implementing or extending the PaperS3 Layout OS firmware."
---

# Gnosis Layout Engine -- Implementation Guide for M5Paper S3

## Table of Contents

1. [What Is This Project?](#1-what-is-this-project)
2. [Hardware Platform](#2-hardware-platform)
3. [Software Stack](#3-software-stack)
4. [Architecture Overview](#4-architecture-overview)
5. [Data Structures](#5-data-structures)
6. [The Layout Algorithm](#6-the-layout-algorithm)
7. [Widget Rendering](#7-widget-rendering)
8. [Dirty Region Tracking and EPD Refresh](#8-dirty-region-tracking-and-epd-refresh)
9. [Touch Input and Event Handling](#9-touch-input-and-event-handling)
10. [Screen Definitions (Hardcoded DSL)](#10-screen-definitions-hardcoded-dsl)
11. [The Main Application Loop](#11-the-main-application-loop)
12. [File Structure and Build System](#12-file-structure-and-build-system)
13. [Step-by-Step Implementation Plan](#13-step-by-step-implementation-plan)
14. [API Quick Reference](#14-api-quick-reference)
15. [Appendix: Pseudocode Listings](#15-appendix-pseudocode-listings)

---

## 1. What Is This Project?

This project implements a **UI layout operating system** for the M5Paper S3 e-ink tablet. The core idea comes from the *Gnosis Layout Engine*, a DSL-driven approach to building e-ink user interfaces that was originally prototyped as a React/Canvas web application (see `sources/gnosis-engine.jsx`) and specified as a formal algorithm (see `sources/gnosis-layout-algorithm.md`).

The Gnosis engine takes a **tree of layout nodes** -- boxes, labels, bars, grids, lists, separators, circles, and other widgets -- and performs a recursive layout pass to compute the screen position of every element. It then renders only the regions that have changed ("dirty rectangles") using the e-ink display's partial refresh capability, which is critical because e-ink refreshes are slow (50-300ms) and refreshing the entire screen causes visible flicker.

### Why a layout engine on an embedded device?

Traditional embedded UIs are drawn imperatively: you call `fillRect`, `drawString`, etc. at hardcoded pixel positions. This works for simple screens but becomes unmanageable when:

- You have multiple screens (dashboard, calendar, mail, reader, debug)
- Widgets need to resize based on content
- You want partial updates -- only redrawing what changed
- You want to experiment with different layouts quickly

The Gnosis approach solves this by separating **what** to display (the node tree / DSL) from **how** to display it (the layout + render engine). On this embedded device, we skip JSON parsing entirely and define screens as C++ struct initializer lists, which compile down to static data with zero runtime overhead.

### What already exists?

The previous PaperS3 firmware projects (0075-0077) established:

- The M5Paper S3 hardware initialization pattern
- Touch input handling via M5Unified
- EPD refresh mode management (text, fast, quality modes)
- A card-based UI layout approach with hardcoded coordinates
- A `Rect` struct with hit-testing

This project generalizes those patterns into a reusable layout engine.

---

## 2. Hardware Platform

### M5Paper S3

| Property | Value |
|---|---|
| MCU | ESP32-S3 (dual-core Xtensa LX7, 240 MHz) |
| RAM | 512 KB SRAM + 8 MB PSRAM (octal SPI) |
| Flash | 16 MB |
| Display | 4.7" e-ink (EPD), 960 x 540 pixels, 1-bit (black/white) with 16-level grayscale |
| Touch | Capacitive touch panel overlay (GT911 controller) |
| Framework | ESP-IDF 5.3.4 |
| Display Library | M5GFX (LovyanGFX fork) via M5Unified |

### Display Characteristics

The e-ink display is fundamentally different from LCD/OLED:

- **No backlight**: Reflective, like paper. Reads well in sunlight.
- **Bistable**: Pixels hold their state without power. Only costs energy to *change*.
- **Slow refresh**: A full-screen refresh takes 300-1000ms with visible flashing.
- **Partial refresh**: Updating a small region (e.g. 200x50 pixels) can be done in 50-150ms with no flash.
- **Ghosting**: Partial refreshes leave faint residue of previous content. Periodic full refreshes clean this up.
- **Waveforms**: The display controller supports multiple refresh "waveforms" trading speed vs. quality:

| Mode | ESP enum | Speed | Quality | Use case |
|---|---|---|---|---|
| Quality | `epd_quality` | Slowest | Best, no ghosting | Full screen redraws, images |
| Text | `epd_text` | Medium | Good | UI redraws with text |
| Fast | `epd_fast` | Fast | Some ghosting | Live interaction (drawing, scrolling) |
| Fastest | `epd_fastest` | Fastest | Most ghosting | Real-time feedback |

### Memory Budget

Understanding memory is critical on this device:

```
Framebuffer (960 x 540, 1-bit):  64,800 bytes (63 KB)
   -- Actually 4-bit grayscale: 259,200 bytes (253 KB)
   -- M5GFX manages this in PSRAM automatically

Node tree (64 nodes x ~40 bytes):   2,560 bytes (2.5 KB)
Dirty rect buffer (32 x 8 bytes):     256 bytes
Stack + locals:                      ~8 KB
────────────────────────────────────────────
Application overhead:               ~11 KB SRAM
Framebuffer:                        ~253 KB PSRAM
```

PSRAM is abundant (8 MB). SRAM is more constrained but 512 KB is plenty for this use case. The layout engine itself is designed to be extremely memory-efficient.

---

## 3. Software Stack

### Layer Diagram

```
┌─────────────────────────────────────────────────┐
│              Application Layer                   │
│  Screen definitions, touch handlers, app logic   │
├─────────────────────────────────────────────────┤
│              Gnosis Layout Engine                 │
│  Node tree, layout algorithm, dirty tracking     │
├─────────────────────────────────────────────────┤
│              Widget Renderer                      │
│  Label, Bar, List, Grid, Circle, Sep, etc.       │
├─────────────────────────────────────────────────┤
│              M5GFX / M5Unified                    │
│  Display driver, touch driver, system init       │
├─────────────────────────────────────────────────┤
│              ESP-IDF 5.3.4                        │
│  FreeRTOS, SPI, I2C, PSRAM, flash               │
├─────────────────────────────────────────────────┤
│              Hardware                             │
│  ESP32-S3 + IT8951 EPD controller + GT911 touch  │
└─────────────────────────────────────────────────┘
```

### Key Dependencies

| Component | Header | Purpose |
|---|---|---|
| M5Unified | `<M5Unified.hpp>` | Board init, display, touch, buttons |
| M5GFX | (included by M5Unified) | Drawing primitives, font rendering, EPD modes |
| ESP-IDF | `<esp_log.h>`, `<freertos/FreeRTOS.h>` | Logging, delays, task management |

### How M5GFX Works

M5GFX is a graphics library that provides a `M5GFX` display object (accessible as `M5.Display`) with these key APIs:

```cpp
// Drawing primitives
M5.Display.fillRect(x, y, w, h, color);        // Filled rectangle
M5.Display.drawRect(x, y, w, h, color);        // Outlined rectangle
M5.Display.drawFastHLine(x, y, w, color);       // Horizontal line
M5.Display.drawFastVLine(x, y, h, color);       // Vertical line
M5.Display.drawCircle(cx, cy, r, color);        // Circle outline
M5.Display.fillCircle(cx, cy, r, color);        // Filled circle

// Text
M5.Display.setTextFont(2);                      // Built-in font (2 = small)
M5.Display.setTextColor(fg, bg);                // Foreground + background
M5.Display.drawString("text", x, y);            // Draw text at position
M5.Display.setTextDatum(top_left);              // Alignment anchor

// EPD control
M5.Display.setEpdMode(epd_mode_t::epd_text);   // Set refresh waveform
M5.Display.startWrite();                         // Begin batch drawing
M5.Display.endWrite();                           // End batch, trigger refresh
M5.Display.waitDisplay();                        // Wait for refresh complete
M5.Display.setClipRect(x, y, w, h);            // Clip drawing to region
M5.Display.clearClipRect();                      // Remove clip

// Screen info
int w = M5.Display.width();                      // 960 (in rotation 1)
int h = M5.Display.height();                     // 540 (in rotation 1)
```

The display is initialized in landscape mode via `M5.Display.setRotation(1)`, giving us a 960-wide by 540-tall canvas.

---

## 4. Architecture Overview

The system has four major subsystems that work together in a pipeline:

```
                ┌──────────────┐
                │ Screen Def   │  Static node tree (C++ structs)
                │ (DSL data)   │
                └──────┬───────┘
                       │
                       v
                ┌──────────────┐
                │   LAYOUT     │  Recursive tree walk
                │   ENGINE     │  Computes Rect for every node
                └──────┬───────┘
                       │
                       v
                ┌──────────────┐
                │   DIRTY      │  Tracks which nodes changed
                │   TRACKER    │  Merges overlapping rects
                └──────┬───────┘
                       │
                       v
                ┌──────────────┐
                │   RENDERER   │  Draws widgets into framebuffer
                │              │  Issues EPD partial refresh
                └──────────────┘
```

### Data Flow for a Typical Frame

1. **Startup**: The screen definition (node tree) is loaded. `LayoutScreen()` computes positions for all nodes.
2. **Event**: User touches a button, or a timer fires, or sensor data arrives.
3. **Update**: Application logic updates bound values (e.g., clock text, sensor readings). Affected nodes are marked `dirty`.
4. **Collect**: `CollectDirtyRects()` walks the tree and gathers bounding rectangles of all dirty leaf nodes.
5. **Merge**: `MergeRects()` combines overlapping or nearby rectangles to reduce the number of EPD refresh calls.
6. **Render**: For each merged rectangle, `RenderRect()` walks the tree, clips to the dirty region, and draws only the intersecting widgets.
7. **Refresh**: `EPD_PartialRefresh()` pushes each dirty rectangle to the e-ink display using the appropriate waveform.

This is much more efficient than redrawing the entire screen. For example, updating a clock label only refreshes a ~100x30 pixel region instead of all 960x540 pixels.

---

## 5. Data Structures

### 5.1 The Rect Struct

This is the fundamental building block, already used in firmwares 0076/0077:

```cpp
struct Rect {
    int16_t x = 0;     // Top-left X coordinate
    int16_t y = 0;     // Top-left Y coordinate
    int16_t w = 0;     // Width in pixels
    int16_t h = 0;     // Height in pixels

    bool Contains(int16_t px, int16_t py) const {
        return px >= x && px < (x + w) && py >= y && py < (y + h);
    }

    bool Intersects(const Rect& other) const {
        return !(other.x >= x + w || other.x + other.w <= x ||
                 other.y >= y + h || other.y + other.h <= y);
    }

    Rect Intersection(const Rect& other) const {
        int16_t ix = std::max(x, other.x);
        int16_t iy = std::max(y, other.y);
        int16_t ix2 = std::min(x + w, other.x + other.w);
        int16_t iy2 = std::min(y + h, other.y + other.h);
        if (ix2 <= ix || iy2 <= iy) return {0, 0, 0, 0};
        return {ix, iy, static_cast<int16_t>(ix2 - ix), static_cast<int16_t>(iy2 - iy)};
    }

    Rect Union(const Rect& other) const {
        int16_t ux = std::min(x, other.x);
        int16_t uy = std::min(y, other.y);
        int16_t ux2 = std::max(x + w, other.x + other.w);
        int16_t uy2 = std::max(y + h, other.y + other.h);
        return {ux, uy, static_cast<int16_t>(ux2 - ux), static_cast<int16_t>(uy2 - uy)};
    }

    int32_t Area() const { return static_cast<int32_t>(w) * h; }
};
```

### 5.2 Node Types

Each node in the layout tree has a type that determines both how it is laid out and how it is rendered:

```cpp
enum class NodeType : uint8_t {
    VBOX,       // Vertical box: children stacked top-to-bottom
    HBOX,       // Horizontal box: children laid out left-to-right
    FIXED,      // Absolute positioning: children placed at explicit offsets
    LABEL,      // Text label (bitmap font, sizeable)
    BAR,        // Progress/gauge bar
    LIST,       // Scrollable list of rows
    GRID,       // Grid of cells (e.g., calendar)
    CIRCLE,     // Circle outline (Bresenham)
    CROSS,      // Crosshair
    SEP,        // Horizontal separator line
    BTN,        // Button (outlined rect, child label)
    GAUGE,      // Labeled gauge: "R 015 [====    ]"
    BADGE,      // Inverted text chip
    ICON,       // Small shape icon (square, circle, diamond, triangle)
    DOT,        // Status dot
    TEXT_BLOCK, // Multi-line text block
    SPACER,     // Flexible space (absorbs remaining width/height)
};
```

### 5.3 Waveform Hint

Each node can specify what EPD refresh quality it needs:

```cpp
enum class Waveform : uint8_t {
    FAST,       // Fastest refresh, some ghosting OK
    PART,       // Partial refresh, balanced
    FULL,       // Full refresh needed (for images, large changes)
};
```

### 5.4 The Node Struct

This is the core data structure. It must be kept small because we may have 60+ nodes in a screen:

```cpp
// Forward declaration
struct Node;

// Maximum children per node (keeps memory bounded)
static constexpr size_t kMaxChildren = 16;
// Maximum text content length for labels
static constexpr size_t kMaxTextLen = 48;

struct Node {
    NodeType type = NodeType::VBOX;
    Waveform waveform = Waveform::PART;
    bool dirty = true;  // Starts dirty so first render draws everything

    // Computed bounding box (set by layout pass)
    Rect rect = {};

    // Children (inline array to avoid heap allocation)
    Node* children[kMaxChildren] = {};
    uint8_t n_children = 0;

    // Type-specific properties (packed to save space)
    //   LABEL:  props[0]=size(1/2/4), props[1]=color_index, props[2]=max_visible_chars
    //   BAR:    props[0]=max_value, props[1]=current_value
    //   LIST:   props[0]=row_h, props[1]=max_items, props[2]=scroll_offset, props[3]=selected
    //   GRID:   props[0]=cols, props[1]=cell_w, props[2]=cell_h, props[3]=count
    //   CIRCLE: props[0]=cx_offset, props[1]=cy_offset, props[2]=radius
    //   CROSS:  props[0]=cx_offset, props[1]=cy_offset, props[2]=arm_length
    //   GAUGE:  props[0]=max_value, props[1]=current_value
    //   HBOX:   props[0]=split_width (if non-zero, use two-pane split)
    //   VBOX/FIXED: unused
    //   BADGE:  (uses text)
    //   ICON:   props[0]=shape (0=square,1=circle,2=diamond,3=triangle)
    int16_t props[4] = {};

    // Explicit dimensions (0 = flexible/auto)
    int16_t explicit_w = 0;  // If non-zero, use this width instead of flex
    int16_t explicit_h = 0;  // If non-zero, use this height instead of flex

    // Position offsets for FIXED children
    int16_t offset_x = 0;
    int16_t offset_y = 0;

    // Text content (for LABEL, BADGE, TEXT_BLOCK, GAUGE)
    char text[kMaxTextLen] = {};

    // Padding (for labels)
    int8_t pad_x = 0;
    int8_t pad_y = 0;

    // Visual flags
    bool invert = false;     // Inverted colors (for badges, selected items)
    bool border_t = false;   // Draw top border line
    bool border_b = false;   // Draw bottom border line
};
```

**Memory analysis**: Each `Node` is approximately 140 bytes. A screen with 64 nodes uses ~9 KB. This fits easily in SRAM or PSRAM.

### 5.5 The Screen Struct

A screen is the top-level container with three fixed zones:

```cpp
struct Screen {
    Node* bar = nullptr;    // Top status bar (fixed height)
    Node* body = nullptr;   // Main content area (fills remaining space)
    Node* nav = nullptr;    // Bottom navigation bar (fixed height)
};
```

This three-zone split is a convention from the Gnosis DSL, where every screen has:
- A top bar showing system status (title, signal, power, etc.)
- A body area with the actual content
- A bottom nav bar with action icons/badges

### 5.6 Node Pool (Static Allocation)

To avoid heap fragmentation on an embedded system, all nodes are allocated from a static pool:

```cpp
static constexpr size_t kMaxNodes = 128;

class NodePool {
public:
    Node* Alloc() {
        if (count_ >= kMaxNodes) return nullptr;
        Node* n = &nodes_[count_++];
        *n = Node{};  // Zero-initialize
        return n;
    }

    void Reset() {
        count_ = 0;
    }

    size_t Used() const { return count_; }

private:
    Node nodes_[kMaxNodes];
    size_t count_ = 0;
};
```

---

## 6. The Layout Algorithm

The layout algorithm is the heart of the engine. It takes a tree of nodes and computes the `rect` (position + size) for every node. The algorithm is recursive and runs in a single pass (O(N) where N is the number of nodes).

### 6.1 Entry Point: LayoutScreen

```
procedure LAYOUT-SCREEN(screen, W=960, H=540)
    bar_h  = screen.bar->explicit_h        (e.g., 16px)
    nav_h  = screen.nav->explicit_h        (e.g., 16px)
    body_h = H - bar_h - nav_h             (e.g., 508px)

    LAYOUT-NODE(screen.bar,  0, 0,          W, bar_h)
    LAYOUT-NODE(screen.body, 0, bar_h,      W, body_h)
    LAYOUT-NODE(screen.nav,  0, bar_h+body_h, W, nav_h)
```

The screen is divided into three horizontal bands. The bar and nav have fixed heights; the body gets whatever remains.

### 6.2 Recursive Dispatch: LayoutNode

```
procedure LAYOUT-NODE(node, x, y, w, h)
    node.rect = {x, y, w, h}

    switch node.type:
        VBOX  -> LAYOUT-VBOX(node, x, y, w, h)
        HBOX  -> LAYOUT-HBOX(node, x, y, w, h)
        FIXED -> LAYOUT-FIXED(node, x, y, w, h)
        *     -> LAYOUT-LEAF(node, x, y, w, h)
```

There are only three layout modes (VBOX, HBOX, FIXED). Everything else is a leaf node that simply records its bounding box.

### 6.3 Vertical Box Layout (VBOX)

A VBOX stacks its children vertically. Children with an explicit height get that exact height; remaining space is divided equally among flexible children.

```
procedure LAYOUT-VBOX(node, x, y, w, h)
    // Pass 1: measure fixed children, count flexible ones
    fixed_total = 0
    flex_count  = 0
    for each child in node.children:
        if child.explicit_h > 0:
            fixed_total += child.explicit_h
        else:
            flex_count += 1

    // Pass 2: compute flexible height and assign positions
    remaining = h - fixed_total
    flex_h = (flex_count > 0) ? remaining / flex_count : 0

    cursor_y = y
    for each child in node.children:
        child_h = child.explicit_h > 0 ? child.explicit_h : flex_h
        LAYOUT-NODE(child, x, cursor_y, w, child_h)
        cursor_y += child_h
```

**Visual example** for a screen body VBOX with children [h=16, flex, h=16]:

```
┌────────────────────────────┐ y=16
│  Status bar (h=16)         │
├────────────────────────────┤ y=32
│                            │
│  Flexible content          │
│  (h = 540-16-16-16-16 =   │
│       476 pixels)          │
│                            │
├────────────────────────────┤ y=508
│  Bottom nav (h=16)         │
└────────────────────────────┘ y=524
```

### 6.4 Horizontal Box Layout (HBOX)

HBOX is identical to VBOX but operates on the horizontal axis. It has one special case: the **split** mode.

**Split mode** (when `props[0] > 0`):

```
procedure LAYOUT-HBOX-SPLIT(node, x, y, w, h)
    split_w = node.props[0]        // Left pane width (e.g., 200)
    right_w = w - split_w - 1      // Right pane width (1px for divider)

    LAYOUT-NODE(children[0], x, y, split_w, h)
    LAYOUT-NODE(children[1], x + split_w + 1, y, right_w, h)
```

**General HBOX** (without split):

```
procedure LAYOUT-HBOX(node, x, y, w, h)
    // Pass 1: measure
    fixed_total = 0, flex_count = 0
    for each child:
        if child.explicit_w > 0:
            fixed_total += child.explicit_w
        else if child.type == SPACER:
            flex_count += 1
        else if child is a LABEL:
            fixed_total += strlen(child.text) * GLYPH_W * size + padding

    // Pass 2: assign
    flex_w = (flex_count > 0) ? (w - fixed_total) / flex_count : 0
    cursor_x = x
    for each child:
        child_w = ... (explicit, flex, or text-measured)
        LAYOUT-NODE(child, cursor_x, y, child_w, h)
        cursor_x += child_w
```

**Visual example** for a status bar HBOX: `[LABEL "GNOSIS//3.1"] [SPACER] [LABEL "SIG:97%"] [LABEL "PWR:EINK" w=64] [DOT w=12]`

```
┌─────────────┬──────────────────────────┬──────────┬────────────┬──┐
│ GNOSIS//3.1 │        (spacer)          │ SIG:97%  │  PWR:EINK  │. │
│  (auto-w)   │   (fills remaining)      │ (auto-w) │  (w=64)    │12│
└─────────────┴──────────────────────────┴──────────┴────────────┴──┘
```

### 6.5 Fixed (Absolute) Positioning

Children carry explicit offsets relative to the parent. No flow calculation.

```
procedure LAYOUT-FIXED(node, x, y, w, h)
    for each child:
        cx = x + child.offset_x
        cy = y + child.offset_y
        cw = child.explicit_w > 0 ? child.explicit_w : (w - child.offset_x)
        ch = child.explicit_h > 0 ? child.explicit_h : (h - child.offset_y)
        LAYOUT-NODE(child, cx, cy, cw, ch)
```

FIXED mode is used for overlapping elements (e.g., crosshairs over circles) and for precise positioning inside a panel (e.g., telemetry gauges at specific Y offsets).

### 6.6 Leaf Node Measurement

Leaf nodes don't have children. Their rect is already set by the parent layout. However, some leaf types compute additional properties:

```
procedure LAYOUT-LEAF(node, x, y, w, h)
    node.rect = {x, y, w, h}

    switch node.type:
        LABEL:
            size = max(1, node.props[0])
            node.props[2] = w / (GLYPH_W * size)   // max visible chars

        LIST:
            row_h = node.props[0]
            max_items = node.props[1]
            node.props[3] = min(max_items, h / row_h)  // visible rows

        GRID:
            cols = node.props[0]
            cell_w = node.props[1] > 0 ? node.props[1] : w / cols
            cell_h = node.props[2]
            node.props[1] = cell_w  // write back computed cell width
            node.props[3] = h / cell_h  // visible rows
```

### 6.7 Text Width Measurement

For HBOX layout, we need to know how wide a label will be:

```
function TEXT-WIDTH(node) -> int16_t
    GLYPH_W = 6     // pixels per glyph (5 pixel wide + 1px gap)
    GLYPH_H = 8     // pixels tall (7 pixel rows + 1px gap)
    size = max(1, node.props[0])
    n_chars = strlen(node.text)
    return n_chars * GLYPH_W * size
```

The Gnosis reference uses a 5x7 pixel bitmap font with 6x8 cell spacing. On M5GFX we can use the built-in fonts, but for authentic Gnosis aesthetics we should implement a bitmap font renderer.

---

## 7. Widget Rendering

The renderer walks the node tree and draws each widget into the M5GFX framebuffer. Drawing is clipped to the current dirty region for efficiency.

### 7.1 Rendering Pipeline

```
procedure RENDER-RECT(screen, clip_rect)
    M5.Display.setClipRect(clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h)

    // Clear the clip region to background
    M5.Display.fillRect(clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h, COLOR_BG)

    // Render each section
    RENDER-SUBTREE(screen.bar,  clip_rect)
    RENDER-SUBTREE(screen.body, clip_rect)
    RENDER-SUBTREE(screen.nav,  clip_rect)

    M5.Display.clearClipRect()
```

```
procedure RENDER-SUBTREE(node, clip_rect)
    if not node.rect.Intersects(clip_rect):
        return    // Early cull -- don't draw nodes outside dirty region

    if node.n_children == 0:
        DRAW-WIDGET(node, clip_rect)
    else:
        // Draw container decorations (borders)
        if node.border_b:
            M5.Display.drawFastHLine(node.rect.x, node.rect.y + node.rect.h - 1,
                                      node.rect.w, COLOR_MID)
        if node.border_t:
            M5.Display.drawFastHLine(node.rect.x, node.rect.y,
                                      node.rect.w, COLOR_MID)
        // HBOX split: draw divider
        if node.type == HBOX && node.props[0] > 0:
            M5.Display.drawFastVLine(node.rect.x + node.props[0],
                                      node.rect.y, node.rect.h, COLOR_MID)

        for each child in node.children:
            RENDER-SUBTREE(child, clip_rect)
```

### 7.2 Widget Drawing Functions

Each leaf widget type has its own drawing function. Here is the implementation for each:

#### LABEL

```cpp
void DrawLabel(M5GFX& display, const Node& node) {
    const char* text = node.text;
    int size = std::max(1, static_cast<int>(node.props[0]));
    int max_chars = node.props[2];

    if (node.invert) {
        // Inverted label: black background, white text
        int text_w = strlen(text) * GLYPH_W * size + 6;
        int text_h = GLYPH_H * size + 4;
        display.fillRect(node.rect.x, node.rect.y, text_w, text_h, COLOR_FG);
        DrawBitmapText(display, text, node.rect.x + 3, node.rect.y + 2,
                       size, COLOR_BG, max_chars);
    } else {
        uint32_t color = ResolveColor(node.props[1]);
        DrawBitmapText(display, text, node.rect.x + node.pad_x,
                       node.rect.y + node.pad_y, size, color, max_chars);
    }
}
```

#### BAR (Progress Bar)

```cpp
void DrawBar(M5GFX& display, const Node& node) {
    int16_t max_val = node.props[0];
    int16_t cur_val = node.props[1];
    int16_t fill_w = node.rect.w * std::min(cur_val, max_val) / max_val;

    // Track (background)
    display.fillRect(node.rect.x, node.rect.y, node.rect.w, node.rect.h, COLOR_LIGHT);
    // Fill (foreground)
    display.fillRect(node.rect.x, node.rect.y, fill_w, node.rect.h, COLOR_FG);
}
```

#### GAUGE (Label + Value + Bar)

A gauge is a compound widget: `"R 015 [====    ]"`

```cpp
void DrawGauge(M5GFX& display, const Node& node) {
    int16_t max_val = node.props[0];
    int16_t cur_val = node.props[1];

    // Label (1 char)
    DrawBitmapText(display, node.text, node.rect.x, node.rect.y, 1, COLOR_MID);
    // Value (3 digits, zero-padded)
    char buf[8];
    snprintf(buf, sizeof(buf), "%03d", cur_val);
    DrawBitmapText(display, buf, node.rect.x + 18, node.rect.y, 1, COLOR_FG);
    // Bar
    int16_t bar_x = node.rect.x + 50;
    int16_t bar_w = node.rect.w - 58;
    display.fillRect(bar_x, node.rect.y + 3, bar_w, 3, COLOR_LIGHT);
    int16_t fill_w = bar_w * std::min(cur_val, max_val) / max_val;
    display.fillRect(bar_x, node.rect.y + 3, fill_w, 3, COLOR_FG);
}
```

#### LIST (Scrollable Rows)

```cpp
void DrawList(M5GFX& display, const Node& node, const ListData& data) {
    int16_t row_h = node.props[0];
    int16_t visible = node.props[3];
    int16_t scroll = node.props[2];
    int16_t selected = node.props[3];  // or separate field

    for (int i = 0; i < visible; i++) {
        int data_i = scroll + i;
        int16_t ry = node.rect.y + i * row_h;

        // Selection highlight
        if (data_i == selected) {
            display.fillRect(node.rect.x, ry, node.rect.w, row_h, COLOR_LIGHT);
        }

        // Draw row content (columns with text)
        DrawListRow(display, data, data_i, node.rect.x + 2, ry + 2);
    }
}
```

#### GRID (Calendar-style)

```cpp
void DrawGrid(M5GFX& display, const Node& node, const GridData& data) {
    int16_t cols = node.props[0];
    int16_t cell_w = node.props[1];
    int16_t cell_h = node.props[2];
    int16_t count = node.props[3];  // total cells

    for (int i = 0; i < count; i++) {
        int col = i % cols;
        int row = i / cols;
        int16_t gx = node.rect.x + col * cell_w;
        int16_t gy = node.rect.y + row * cell_h;

        // Cell border
        display.drawRect(gx, gy, cell_w, cell_h, COLOR_LIGHT);

        // Cell content (e.g., day number)
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", (i % 31) + 1);
        DrawBitmapText(display, buf, gx + cell_w - 18, gy + 3, 1, COLOR_FG);

        // Highlight today
        if (data.today == i) {
            display.fillRect(gx + 1, gy + 1, cell_w - 2, cell_h - 2, COLOR_FG);
            DrawBitmapText(display, buf, gx + cell_w - 18, gy + 3, 1, COLOR_BG);
        }

        // Event dot
        if (data.HasEvent(i)) {
            display.fillRect(gx + cell_w/2 - 1, gy + cell_h - 4, 3, 3, COLOR_FG);
        }
    }
}
```

#### CIRCLE, CROSS, SEP, DOT

These are simple geometric primitives:

```cpp
void DrawCircle(M5GFX& display, const Node& node) {
    int16_t cx = node.rect.x + node.props[0];
    int16_t cy = node.rect.y + node.props[1];
    int16_t r  = node.props[2];
    display.drawCircle(cx, cy, r, COLOR_MID);
}

void DrawCross(M5GFX& display, const Node& node) {
    int16_t cx = node.rect.x + node.props[0];
    int16_t cy = node.rect.y + node.props[1];
    int16_t len = node.props[2];
    display.drawFastHLine(cx - len, cy, 2 * len, COLOR_FG);
    display.drawFastVLine(cx, cy - len, 2 * len, COLOR_FG);
}

void DrawSep(M5GFX& display, const Node& node) {
    display.drawFastHLine(node.rect.x, node.rect.y, node.rect.w, COLOR_MID);
}

void DrawDot(M5GFX& display, const Node& node) {
    display.fillRect(node.rect.x + 2, node.rect.y + node.rect.h/2 - 2, 4, 4, COLOR_FG);
}
```

#### BADGE (Inverted Chip)

```cpp
void DrawBadge(M5GFX& display, const Node& node) {
    int text_w = strlen(node.text) * GLYPH_W + 8;
    display.fillRect(node.rect.x, node.rect.y + 2, text_w, GLYPH_H + 4, COLOR_FG);
    DrawBitmapText(display, node.text, node.rect.x + 4, node.rect.y + 4, 1, COLOR_BG);
}
```

#### ICON (Small Shape)

```cpp
void DrawIcon(M5GFX& display, const Node& node) {
    int sz = 8;
    int ix = node.rect.x + node.rect.w/2 - sz/2;
    int iy = node.rect.y + node.rect.h/2 - sz/2;

    switch (node.props[0]) {
        case 0: display.drawRect(ix, iy, sz, sz, COLOR_MID); break;  // square
        case 1: display.drawCircle(ix+sz/2, iy+sz/2, sz/2, COLOR_MID); break;  // circle
        case 2: // diamond
            display.drawLine(ix+sz/2, iy, ix+sz, iy+sz/2, COLOR_MID);
            display.drawLine(ix+sz, iy+sz/2, ix+sz/2, iy+sz, COLOR_MID);
            display.drawLine(ix+sz/2, iy+sz, ix, iy+sz/2, COLOR_MID);
            display.drawLine(ix, iy+sz/2, ix+sz/2, iy, COLOR_MID);
            break;
        case 3: // triangle
            display.drawTriangle(ix+sz/2, iy, ix+sz, iy+sz, ix, iy+sz, COLOR_MID);
            break;
    }
}
```

### 7.3 The Bitmap Font

For the authentic Gnosis pixel aesthetic, we implement a simple 5x7 bitmap font. Each character is stored as 7 rows of 5-bit bitmasks:

```cpp
static constexpr uint8_t FONT_5X7[][7] = {
    // 'A'
    { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 },
    // 'B'
    { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 },
    // ... (full ASCII set from gnosis-engine.jsx BITMAPS table)
};

static constexpr int GLYPH_W = 6;  // 5px glyph + 1px gap
static constexpr int GLYPH_H = 8;  // 7px glyph + 1px gap

void DrawBitmapText(M5GFX& display, const char* text, int x, int y,
                     int size, uint32_t color, int max_chars = -1) {
    int len = strlen(text);
    if (max_chars >= 0 && len > max_chars) len = max_chars;

    for (int i = 0; i < len; i++) {
        const uint8_t* bm = GetGlyphBitmap(text[i]);
        if (!bm) continue;
        int gx = x + i * GLYPH_W * size;
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (bm[row] & (1 << (4 - col))) {
                    if (size == 1) {
                        // Single pixel -- use fillRect for 1x1
                        display.drawPixel(gx + col, y + row, color);
                    } else {
                        display.fillRect(gx + col * size, y + row * size,
                                         size, size, color);
                    }
                }
            }
        }
    }
}
```

**Performance note**: For size=1, drawing individual pixels is slow. A faster approach is to build a small buffer (5x7 bytes) and blit it. However, for the demo this is adequate. The M5GFX built-in fonts (setTextFont(2)) can also be used as a fallback -- they look different but are faster.

### 7.4 Color Palette

The Gnosis engine uses an e-ink-appropriate palette:

```cpp
// E-ink palette -- grayscale values for the EPD
static constexpr uint32_t COLOR_BG    = 0xFFFFFF;  // Paper white
static constexpr uint32_t COLOR_FG    = 0x000000;  // Ink black
static constexpr uint32_t COLOR_MID   = 0x808080;  // Medium gray (borders, secondary)
static constexpr uint32_t COLOR_LIGHT = 0xC0C0C0;  // Light gray (tracks, highlights)
static constexpr uint32_t COLOR_GHOST = 0xAAAAAA;  // Ghost text (disabled, hints)
static constexpr uint32_t COLOR_DIM   = 0x666666;  // Dim text (timestamps, labels)

// Color index lookup (from node.props[1])
uint32_t ResolveColor(int16_t index) {
    switch (index) {
        case 0: return COLOR_FG;
        case 1: return COLOR_MID;
        case 2: return COLOR_GHOST;
        case 3: return COLOR_DIM;
        case 4: return COLOR_LIGHT;
        default: return COLOR_FG;
    }
}
```

---

## 8. Dirty Region Tracking and EPD Refresh

This is the most important optimization for e-ink. Without it, every update would require a full-screen refresh (300ms+ with flashing). With dirty tracking, we only refresh the small regions that actually changed.

### 8.1 Marking Nodes Dirty

When a value changes (e.g., clock updates, sensor reading arrives), the corresponding node is marked dirty:

```cpp
void MarkDirty(Node* node) {
    node->dirty = true;
}
```

### 8.2 Collecting Dirty Rectangles

After marking dirty nodes, we walk the tree to collect their bounding boxes:

```cpp
static constexpr size_t kMaxDirtyRects = 32;

struct DirtyCollector {
    Rect rects[kMaxDirtyRects];
    Waveform waveforms[kMaxDirtyRects];
    size_t count = 0;

    void Collect(Node* node) {
        if (!node) return;

        if (node->dirty) {
            if (node->n_children == 0) {
                // Leaf node: add its rect
                if (count < kMaxDirtyRects) {
                    rects[count] = node->rect;
                    waveforms[count] = node->waveform;
                    count++;
                }
                node->dirty = false;
            } else {
                // Container: recurse into children
                for (int i = 0; i < node->n_children; i++) {
                    Collect(node->children[i]);
                }
                node->dirty = false;
            }
        } else {
            // Check if any child is dirty (dirty child under clean parent)
            for (int i = 0; i < node->n_children; i++) {
                if (node->children[i]->dirty) {
                    Collect(node->children[i]);
                }
            }
        }
    }
};
```

### 8.3 Merging Overlapping Rectangles

If multiple adjacent labels change, their dirty rects might overlap or be very close. It's more efficient to merge them into one larger rectangle than to issue multiple small EPD refreshes:

```cpp
void MergeDirtyRects(DirtyCollector& dc) {
    static constexpr int32_t THRESHOLD = 512;  // pixels^2 waste tolerance

    bool merged_any;
    do {
        merged_any = false;
        for (size_t i = 0; i < dc.count && !merged_any; i++) {
            for (size_t j = i + 1; j < dc.count && !merged_any; j++) {
                Rect u = dc.rects[i].Union(dc.rects[j]);
                int32_t waste = u.Area() - dc.rects[i].Area() - dc.rects[j].Area();
                if (waste < THRESHOLD) {
                    dc.rects[i] = u;
                    // Use worst waveform
                    if (dc.waveforms[j] > dc.waveforms[i]) {
                        dc.waveforms[i] = dc.waveforms[j];
                    }
                    // Remove j
                    dc.rects[j] = dc.rects[dc.count - 1];
                    dc.waveforms[j] = dc.waveforms[dc.count - 1];
                    dc.count--;
                    merged_any = true;
                }
            }
        }
    } while (merged_any);
}
```

**Why 512 pixels^2?** This is the "waste budget": the maximum area of unnecessary pixels we're willing to refresh to avoid an extra EPD call. Since each EPD partial refresh has overhead (~10ms setup), merging two 20x20 rects with 400px^2 waste is cheaper than two separate refreshes.

### 8.4 EPD Refresh Dispatch

```cpp
epd_mode_t WaveformToEpdMode(Waveform wf) {
    switch (wf) {
        case Waveform::FAST: return epd_mode_t::epd_fast;
        case Waveform::PART: return epd_mode_t::epd_text;
        case Waveform::FULL: return epd_mode_t::epd_quality;
    }
    return epd_mode_t::epd_text;
}

void RefreshDisplay(Screen& screen, DirtyCollector& dc) {
    if (dc.count == 0) return;

    MergeDirtyRects(dc);

    for (size_t i = 0; i < dc.count; i++) {
        Rect& r = dc.rects[i];

        // Set EPD mode for this region
        M5.Display.setEpdMode(WaveformToEpdMode(dc.waveforms[i]));

        M5.Display.startWrite();

        // Render into the dirty region
        M5.Display.setClipRect(r.x, r.y, r.w, r.h);
        M5.Display.fillRect(r.x, r.y, r.w, r.h, COLOR_BG);  // Clear region

        RenderSubtree(screen.bar, r);
        RenderSubtree(screen.body, r);
        RenderSubtree(screen.nav, r);

        M5.Display.clearClipRect();
        M5.Display.endWrite();
    }
}
```

### 8.5 Full Screen Refresh

Periodically (e.g., on screen change or every N partial updates), a full refresh clears ghosting:

```cpp
void FullRefresh(Screen& screen) {
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    M5.Display.startWrite();
    M5.Display.fillScreen(COLOR_BG);

    RenderSubtree(screen.bar, screen.bar->rect);
    RenderSubtree(screen.body, screen.body->rect);
    RenderSubtree(screen.nav, screen.nav->rect);

    M5.Display.endWrite();
    M5.Display.waitDisplay();
}
```

---

## 9. Touch Input and Event Handling

### 9.1 Touch State Machine

The M5Paper S3 has a capacitive touch overlay. Touch input is polled via `M5.update()` and read from `M5.Touch`:

```cpp
void HandleTouch() {
    const bool has_touch = M5.Touch.getCount() > 0;

    if (has_touch) {
        const auto& detail = M5.Touch.getDetail();
        int16_t tx = detail.x;
        int16_t ty = detail.y;

        if (!touch_down_) {
            // Finger just touched -- this is a "press" event
            touch_down_ = true;
            OnTouchDown(tx, ty);
        } else {
            // Finger is dragging
            OnTouchMove(tx, ty);
        }
    } else if (touch_down_) {
        // Finger lifted -- this is a "release" event
        touch_down_ = false;
        OnTouchUp();
    }
}
```

### 9.2 Hit Testing

To determine which node was touched, walk the tree and find the deepest (most specific) node containing the touch point:

```cpp
Node* HitTest(Node* node, int16_t tx, int16_t ty) {
    if (!node || !node->rect.Contains(tx, ty)) return nullptr;

    // Check children in reverse order (last drawn = on top)
    for (int i = node->n_children - 1; i >= 0; i--) {
        Node* hit = HitTest(node->children[i], tx, ty);
        if (hit) return hit;
    }

    // No child hit, return this node if it's interactive
    return node;
}
```

### 9.3 Button Handling Pattern

From 0077-alphabet-graffiti, the established pattern is:

1. On touch-down: record which button was pressed (by position)
2. On touch-up: verify the touch is still over the same button, then execute the action

This prevents accidental activations when the user drags away from a button.

---

## 10. Screen Definitions (Hardcoded DSL)

Instead of parsing JSON at runtime, we define screens as C++ functions that build node trees using the node pool. This gives us the same expressiveness as the Gnosis JSON DSL with zero parsing overhead.

### 10.1 Builder Helper Functions

```cpp
// Create a VBOX node with children
Node* VBox(NodePool& pool, std::initializer_list<Node*> children, int16_t h = 0) {
    Node* n = pool.Alloc();
    n->type = NodeType::VBOX;
    n->explicit_h = h;
    for (auto* child : children) {
        if (n->n_children < kMaxChildren) {
            n->children[n->n_children++] = child;
        }
    }
    return n;
}

// Create an HBOX node
Node* HBox(NodePool& pool, std::initializer_list<Node*> children, int16_t h = 0) {
    Node* n = pool.Alloc();
    n->type = NodeType::HBOX;
    n->explicit_h = h;
    for (auto* child : children) {
        if (n->n_children < kMaxChildren) {
            n->children[n->n_children++] = child;
        }
    }
    return n;
}

// Create an HBOX with split
Node* HBoxSplit(NodePool& pool, int16_t split_w, Node* left, Node* right) {
    Node* n = pool.Alloc();
    n->type = NodeType::HBOX;
    n->props[0] = split_w;
    n->children[0] = left;
    n->children[1] = right;
    n->n_children = 2;
    return n;
}

// Create a FIXED container
Node* Fixed(NodePool& pool, std::initializer_list<Node*> children) {
    Node* n = pool.Alloc();
    n->type = NodeType::FIXED;
    for (auto* child : children) {
        if (n->n_children < kMaxChildren) {
            n->children[n->n_children++] = child;
        }
    }
    return n;
}

// Create a LABEL
Node* Label(NodePool& pool, const char* text, int size = 1, int color = 0,
            int16_t x = 0, int16_t y = 0, int16_t w = 0) {
    Node* n = pool.Alloc();
    n->type = NodeType::LABEL;
    n->props[0] = size;
    n->props[1] = color;
    n->offset_x = x;
    n->offset_y = y;
    n->explicit_w = w;
    strncpy(n->text, text, kMaxTextLen - 1);
    return n;
}

// Create a SPACER
Node* Spacer(NodePool& pool) {
    Node* n = pool.Alloc();
    n->type = NodeType::SPACER;
    return n;
}

// ... similar helpers for BAR, GAUGE, LIST, GRID, CIRCLE, CROSS, SEP, DOT, BADGE, ICON
```

### 10.2 Example: Dashboard Screen

This recreates the "main: Dashboard" preset from the Gnosis JSX reference, adapted for 960x540:

```cpp
Screen BuildDashboardScreen(NodePool& pool) {
    // Note: all coordinates are scaled from the 400x280 Gnosis reference
    // to our 960x540 display (factor of ~2.4x horizontal, ~1.93x vertical)
    // We use the wider space to add more detail.

    Screen screen;

    // Top bar (16px tall status strip)
    screen.bar = HBox(pool, {
        Label(pool, "GNOSIS//3.1"),
        Spacer(pool),
        Label(pool, "SIG:97%", 1, /*color=mid*/ 1),
        Label(pool, "PWR:EINK", 1, 1, 0, 0, 154),
        Dot(pool, 24),
    }, /*h=*/ 32);
    screen.bar->border_b = true;

    // Body: split layout with left panel (clock/compass) and right panel (telemetry/log)
    Node* left_panel = Fixed(pool, {
        Label(pool, "CHRONO", 1, 2, 16, 12),
        Label(pool, "14:37", 4, 0, 16, 44),
        Label(pool, "2026.03.22", 1, 1, 16, 116),
        Label(pool, "SEC 42", 1, 2, 16, 144),
        Sep(pool, 0, 192, 480),
        Label(pool, "ORIENTATION", 1, 2, 16, 208),
        Circle(pool, 240, 368, 120),
        Circle(pool, 240, 368, 90),
        Circle(pool, 240, 368, 56),
        Cross(pool, 240, 368, 20),
    });

    Node* right_panel = Fixed(pool, {
        Label(pool, "TELEMETRY", 1, 2, 16, 12),
        Gauge(pool, "R", 15, 360, 16, 44, 360),
        Gauge(pool, "P", 34, 360, 16, 72, 360),
        Gauge(pool, "Y", 127, 360, 16, 100, 360),
        Gauge(pool, "T", 291, 360, 16, 128, 360),
        Gauge(pool, "V", 3, 360, 16, 156, 360),
        Gauge(pool, "A", 188, 360, 16, 184, 360),
        Sep(pool, 0, 224, 470),
        Label(pool, "SYS.LOG", 1, 2, 16, 240),
        // List node would go here with log entries
    });

    screen.body = HBoxSplit(pool, 480, left_panel, right_panel);

    // Bottom nav bar
    screen.nav = HBox(pool, {
        Icon(pool, 0, 48),   // square
        Icon(pool, 1, 48),   // circle
        Icon(pool, 2, 48),   // diamond
        Icon(pool, 3, 48),   // triangle
        Spacer(pool),
        Badge(pool, "AUTO", 84),
        Spacer(pool),
        Label(pool, "PIXEL MONOSPACED", 1, 2),
    }, /*h=*/ 32);
    screen.nav->border_t = true;

    return screen;
}
```

### 10.3 Other Screens

The Gnosis JSX reference includes 7 preset screens. For the demo, we implement at least:

1. **Dashboard** (clock, compass, telemetry gauges, system log)
2. **Calendar/Temporal Map** (grid calendar, agenda list)
3. **Boot/Startup Sequence** (circles, progress bar, branding text)
4. **Widget Gallery/Debug** (all widget types for testing)

Each is a function that returns a `Screen` struct built from the node pool.

---

## 11. The Main Application Loop

### 11.1 Application Class

Following the pattern established in 0076/0077:

```cpp
class GnosisApp {
public:
    void Run();

private:
    void InitBoard();
    void BuildCurrentScreen();
    void HandleTouch();
    void UpdateData();
    void ProcessRefresh();

    NodePool pool_;
    Screen current_screen_;
    int current_screen_index_ = 0;
    bool touch_down_ = false;
    uint32_t last_data_update_ms_ = 0;
    uint32_t full_refresh_counter_ = 0;
};
```

### 11.2 Main Loop

```cpp
void GnosisApp::Run() {
    // Phase 1: Hardware init
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);  // Landscape 960x540

    // Phase 2: Build initial screen
    BuildCurrentScreen();

    // Phase 3: Initial full render
    LayoutScreen(current_screen_, 960, 540);
    FullRefresh(current_screen_);

    // Phase 4: Main loop
    while (true) {
        M5.update();          // Poll hardware (touch, buttons)
        HandleTouch();        // Process input events
        UpdateData();         // Update dynamic values (clock, sensors)
        ProcessRefresh();     // Render dirty regions to EPD
        M5.delay(12);         // ~83 Hz loop (12ms per iteration)
    }
}
```

### 11.3 Data Update

Dynamic values (clock, simulated telemetry) update on a timer:

```cpp
void GnosisApp::UpdateData() {
    uint32_t now = millis();
    if (now - last_data_update_ms_ < 1000) return;  // Update every 1 second
    last_data_update_ms_ = now;

    // Update clock label
    Node* clock_label = FindNodeById("clock");
    if (clock_label) {
        // Get current time
        struct tm timeinfo;
        time(&now_time);
        localtime_r(&now_time, &timeinfo);
        snprintf(clock_label->text, kMaxTextLen, "%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min);
        MarkDirty(clock_label);
    }

    // Update simulated telemetry gauges
    Node* gauge_r = FindNodeById("gauge_r");
    if (gauge_r) {
        gauge_r->props[1] = (gauge_r->props[1] + 3) % 360;
        MarkDirty(gauge_r);
    }
    // ... similar for other gauges
}
```

### 11.4 Screen Switching

Touch on nav icons switches between screens:

```cpp
void GnosisApp::SwitchScreen(int index) {
    current_screen_index_ = index;
    pool_.Reset();  // Free all nodes
    BuildCurrentScreen();
    LayoutScreen(current_screen_, 960, 540);
    FullRefresh(current_screen_);  // Full refresh on screen change
}
```

---

## 12. File Structure and Build System

### 12.1 Project Directory Layout

```
0078-papers3-gnosis-layout/
├── CMakeLists.txt               # Project-level CMake
├── sdkconfig.defaults           # ESP-IDF config overrides
├── main/
│   ├── CMakeLists.txt           # Component-level CMake
│   ├── app_main.cpp             # Entry point (calls GnosisApp::Run)
│   ├── gnosis_app.h             # Application class
│   ├── gnosis_app.cpp           # Application implementation
│   ├── layout_engine.h          # Node, Screen, NodePool, layout functions
│   ├── layout_engine.cpp        # Layout algorithm implementation
│   ├── widget_renderer.h        # DrawWidget, DrawBitmapText, etc.
│   ├── widget_renderer.cpp      # Widget rendering implementation
│   ├── dirty_tracker.h          # DirtyCollector, MergeDirtyRects
│   ├── dirty_tracker.cpp        # Dirty tracking implementation
│   ├── bitmap_font.h            # 5x7 bitmap font data + DrawBitmapText
│   ├── bitmap_font.cpp          # Font rendering
│   ├── screen_dashboard.cpp     # Dashboard screen builder
│   ├── screen_calendar.cpp      # Calendar screen builder
│   ├── screen_boot.cpp          # Boot screen builder
│   ├── screen_debug.cpp         # Widget gallery screen builder
│   └── screens.h                # Screen builder function declarations
```

### 12.2 Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "../../M5PaperS3-UserDemo/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_gnosis_layout)
```

### 12.3 main/CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "app_main.cpp"
        "gnosis_app.cpp"
        "layout_engine.cpp"
        "widget_renderer.cpp"
        "dirty_tracker.cpp"
        "bitmap_font.cpp"
        "screen_dashboard.cpp"
        "screen_calendar.cpp"
        "screen_boot.cpp"
        "screen_debug.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        M5Unified
)
```

### 12.4 Building and Flashing

```bash
# Set up ESP-IDF environment (version 5.3.4)
source ~/esp/esp-idf-v5.3.4/export.sh

# Configure (first time)
cd 0078-papers3-gnosis-layout
idf.py set-target esp32s3
idf.py menuconfig   # Enable PSRAM: Component config > ESP PSRAM > Octal

# Build
idf.py build

# Flash (connect M5Paper S3 via USB-C)
idf.py -p /dev/ttyACM0 flash monitor
```

---

## 13. Step-by-Step Implementation Plan

### Phase 1: Skeleton (Day 1)

1. Create project directory `0078-papers3-gnosis-layout/`
2. Copy CMakeLists.txt from 0077 as template
3. Create `app_main.cpp` with minimal GnosisApp class
4. Implement `InitBoard()` (copy from 0077)
5. Verify it compiles and boots showing a white screen

### Phase 2: Data Structures + Layout (Day 2)

1. Implement `Rect` struct with `Contains`, `Intersects`, `Union`, `Intersection`
2. Implement `Node` struct, `NodeType` enum, `NodePool`
3. Implement `LayoutScreen`, `LayoutNode`, `LayoutVBox`, `LayoutHBox`, `LayoutFixed`, `LayoutLeaf`
4. Write a unit test (on host, not device) that creates a simple tree and verifies computed rects
5. Implement builder helpers (VBox, HBox, Label, etc.)

### Phase 3: Widget Rendering (Day 3)

1. Implement `DrawBitmapText` with the 5x7 font
2. Implement `DrawWidget` dispatch for LABEL, SEP, DOT, BAR, CIRCLE, CROSS
3. Implement `RenderSubtree` with intersection culling
4. Create the Boot screen (simplest -- just circles, cross, labels, progress bar)
5. Full-render the boot screen to the actual EPD -- first visual output!

### Phase 4: Dirty Tracking (Day 4)

1. Implement `DirtyCollector` (collect + merge)
2. Implement `RefreshDisplay` with partial EPD refresh
3. Add a timer that updates the boot progress bar value every second
4. Verify that only the bar region refreshes, not the whole screen

### Phase 5: Dashboard Screen (Day 5)

1. Build the Dashboard screen with clock, gauges, compass circles, log list
2. Implement GAUGE widget rendering
3. Implement LIST widget rendering (simple row display, no scrolling yet)
4. Add simulated data updates (clock ticks, gauge animation)
5. Verify partial refresh works for multiple independent widgets

### Phase 6: Touch + Navigation (Day 6)

1. Implement touch handling state machine
2. Implement HitTest tree walk
3. Add nav bar icons that switch between screens
4. Build Calendar and Debug screens
5. Test full workflow: boot -> dashboard -> calendar -> debug -> dashboard

### Phase 7: Polish (Day 7)

1. Add GRID widget for calendar
2. Add BADGE, ICON widgets
3. Add TEXT_BLOCK widget
4. Periodic full refresh to clear ghosting (every 30 partial refreshes)
5. Final testing and cleanup

---

## 14. API Quick Reference

### M5GFX Drawing Primitives

| Method | Description |
|---|---|
| `fillRect(x,y,w,h,c)` | Fill rectangle |
| `drawRect(x,y,w,h,c)` | Outline rectangle |
| `drawFastHLine(x,y,w,c)` | Horizontal line (fast) |
| `drawFastVLine(x,y,h,c)` | Vertical line (fast) |
| `drawCircle(cx,cy,r,c)` | Circle outline |
| `fillCircle(cx,cy,r,c)` | Filled circle |
| `drawLine(x0,y0,x1,y1,c)` | Arbitrary line |
| `drawTriangle(x0,y0,x1,y1,x2,y2,c)` | Triangle outline |
| `drawPixel(x,y,c)` | Single pixel |
| `drawString(str,x,y)` | Text (uses current font) |
| `fillScreen(c)` | Clear entire framebuffer |

### M5GFX EPD Control

| Method | Description |
|---|---|
| `setEpdMode(mode)` | Set refresh waveform |
| `startWrite()` | Begin batch drawing |
| `endWrite()` | End batch, trigger refresh |
| `waitDisplay()` | Block until refresh complete |
| `setClipRect(x,y,w,h)` | Set clipping region |
| `clearClipRect()` | Remove clipping |
| `setRotation(1)` | Landscape mode (960x540) |
| `width()` / `height()` | Get display dimensions |

### M5Unified Touch

| Method | Description |
|---|---|
| `M5.update()` | Poll all inputs |
| `M5.Touch.getCount()` | Number of active touches (0 or 1) |
| `M5.Touch.getDetail()` | Touch detail (x, y, state) |

### Key Constants

| Constant | Value | Description |
|---|---|---|
| `SCREEN_W` | 960 | Display width (landscape) |
| `SCREEN_H` | 540 | Display height (landscape) |
| `GLYPH_W` | 6 | Bitmap font cell width |
| `GLYPH_H` | 8 | Bitmap font cell height |
| `kMaxNodes` | 128 | Maximum nodes in pool |
| `kMaxChildren` | 16 | Max children per node |
| `kMaxDirtyRects` | 32 | Max dirty regions |
| `kMergeThreshold` | 512 | Dirty rect merge waste budget (px^2) |
| `kLoopDelayMs` | 12 | Main loop period (~83 Hz) |

---

## 15. Appendix: Pseudocode Listings

### Complete Layout Pipeline

```
GNOSIS-FRAME-UPDATE(screen, W, H):
    // 1. Layout (only needed on screen change)
    LAYOUT-SCREEN(screen, W, H)

    // 2. Collect dirty regions
    collector = new DirtyCollector
    collector.Collect(screen.bar)
    collector.Collect(screen.body)
    collector.Collect(screen.nav)

    if collector.count == 0:
        return   // Nothing changed

    // 3. Merge nearby rects
    MERGE-DIRTY-RECTS(collector)

    // 4. Render + refresh each dirty region
    for i = 0 to collector.count - 1:
        r = collector.rects[i]
        wf = collector.waveforms[i]

        SET-EPD-MODE(wf)
        START-WRITE()
        SET-CLIP-RECT(r)
        FILL-RECT(r, BG)

        RENDER-SUBTREE(screen.bar, r)
        RENDER-SUBTREE(screen.body, r)
        RENDER-SUBTREE(screen.nav, r)

        CLEAR-CLIP-RECT()
        END-WRITE()
```

### Node Tree for Dashboard (ASCII Diagram)

```
Screen
├── bar (HBOX h=32)
│   ├── LABEL "GNOSIS//3.1"
│   ├── SPACER
│   ├── LABEL "SIG:97%" color=mid
│   ├── LABEL "PWR:EINK" color=mid w=154
│   └── DOT w=24
├── body (HBOX split=480)
│   ├── left (FIXED)
│   │   ├── LABEL "CHRONO" @(16,12) ghost
│   │   ├── LABEL "14:37" @(16,44) size=4
│   │   ├── LABEL "2026.03.22" @(16,116) mid
│   │   ├── LABEL "SEC 42" @(16,144) ghost
│   │   ├── SEP @(0,192)
│   │   ├── LABEL "ORIENTATION" @(16,208) ghost
│   │   ├── CIRCLE @(240,368) r=120
│   │   ├── CIRCLE @(240,368) r=90
│   │   ├── CIRCLE @(240,368) r=56
│   │   └── CROSS @(240,368) len=20
│   └── right (FIXED)
│       ├── LABEL "TELEMETRY" @(16,12) ghost
│       ├── GAUGE "R" 15/360 @(16,44)
│       ├── GAUGE "P" 34/360 @(16,72)
│       ├── GAUGE "Y" 127/360 @(16,100)
│       ├── GAUGE "T" 291/360 @(16,128)
│       ├── GAUGE "V" 3/360 @(16,156)
│       ├── GAUGE "A" 188/360 @(16,184)
│       ├── SEP @(0,224)
│       ├── LABEL "SYS.LOG" @(16,240) ghost
│       └── LIST @(16,272) rows=7
└── nav (HBOX h=32)
    ├── ICON square w=48
    ├── ICON circle w=48
    ├── ICON diamond w=48
    ├── ICON triangle w=48
    ├── SPACER
    ├── BADGE "AUTO" w=84
    ├── SPACER
    └── LABEL "PIXEL MONOSPACED" ghost
```

### Memory Layout Diagram

```
SRAM (512 KB)
┌──────────────────────────┐ 0x0000
│  Stack (8 KB)             │
├──────────────────────────┤
│  NodePool (128 x 140B     │
│    = 17.5 KB)             │
├──────────────────────────┤
│  DirtyCollector (288B)    │
├──────────────────────────┤
│  App state (~1 KB)        │
├──────────────────────────┤
│  ... free (~484 KB) ...   │
└──────────────────────────┘

PSRAM (8 MB)
┌──────────────────────────┐
│  Framebuffer (~253 KB)    │  Managed by M5GFX
├──────────────────────────┤
│  ... free (~7.75 MB) ...  │
└──────────────────────────┘
```

---

## Glossary

| Term | Definition |
|---|---|
| **EPD** | Electrophoretic Display (e-ink). Pixels are physical particles moved by electric fields. |
| **Partial refresh** | Updating only a sub-region of the EPD, much faster than full refresh. |
| **Ghosting** | Faint residue of previous image content after a partial refresh. |
| **Waveform** | The voltage sequence used to transition EPD pixels. Different waveforms trade speed for quality. |
| **Dirty rect** | A rectangular region of the screen that needs to be redrawn because its content changed. |
| **Node tree** | The hierarchical data structure describing the UI layout. |
| **VBOX/HBOX** | Vertical/horizontal box layout -- children arranged in a line with flexible sizing. |
| **FIXED** | Absolute positioning layout -- children placed at explicit pixel offsets. |
| **Node pool** | Pre-allocated array of Node structs to avoid heap allocation. |
| **DSL** | Domain-Specific Language -- the Gnosis JSON format for describing screens. Here, compiled to C++ structs. |
| **M5GFX** | LovyanGFX-based graphics library for M5Stack devices. |
| **M5Unified** | M5Stack's unified hardware abstraction layer (display, touch, IMU, power). |
