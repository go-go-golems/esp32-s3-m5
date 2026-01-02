---
Title: 'Implementation guide: List view + selection (A2)'
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
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp
      Note: Text metrics and drawing APIs used for list view
    - Path: 0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: Sprite present + waitDMA baseline
    - Path: 0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Keyboard-driven redraw-on-dirty pattern
ExternalSources: []
Summary: Intern guide for implementing a reusable keyboard-first list view (selection, scrolling, truncation) for the demo firmware.
LastUpdated: 2026-01-01T20:09:12.9577352-05:00
WhatFor: ""
WhenToUse: ""
---



# Implementation guide: List view + selection (A2)

This guide is for an intern implementing the **List View + Selection** demo and (more importantly) the reusable list-view component that the demo firmware’s menu/launcher will use everywhere.

The goal is not “draw text in a loop”. The goal is a **fast, readable, keyboard-first list** that:

- supports selection highlight
- supports scrolling with a stable “viewport”
- supports long labels (truncate or scroll)
- can be reused for: demo launcher, settings pages, file picker, command palette results

## Outcome (acceptance criteria)

When this is done:

- Up/Down moves selection with a visible highlight.
- Selection can move through a list longer than the viewport; the viewport scrolls correctly.
- Rendering is stable and flicker-free.
- The component exposes a small API that other demos can reuse.

## Where to look (existing code to copy patterns from)

- Keyboard-driven UI + redraw-on-dirty approach:
  - `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`
- M5GFX bring-up + sprite present + `waitDMA()`:
  - `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
- M5GFX core API for text metrics and drawing:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`

## Recommended rendering strategy (pick one)

You can implement list view in two valid ways:

### Option 1 (recommended first): sprite redraw of list viewport

Render the list viewport into a `M5Canvas` (or a smaller `LGFX_Sprite`) and push it each time the list changes. On a 240×135 display, this is simple and fast enough.

Pros:

- easiest to get correct (no scroll rect edge cases)
- easy to layer with header/footer sprites

Cons:

- redraw cost scales with viewport size (still fine on Cardputer)

### Option 2 (advanced): scrollRect + incremental redraw

Use `setScrollRect()` and `scroll()` to shift pixels, then redraw only the newly exposed row.

Pros:

- extremely fast for long lists

Cons:

- more state/edge cases
- more complicated with overlays/sprites

This guide includes both, but implement Option 1 first.

## Key M5GFX/LGFX APIs to use

These come from `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`:

Text and metrics:

```cpp
void setTextSize(float sx, float sy);
void setTextDatum(textdatum_t datum);
void setTextColor(T fg, T bg);
int32_t fontHeight(void) const;
int32_t textWidth(const char *string);
size_t drawString(const char *string, int32_t x, int32_t y);
```

Drawing:

```cpp
void fillRect(int32_t x, int32_t y, int32_t w, int32_t h);
void drawRect(int32_t x, int32_t y, int32_t w, int32_t h);
void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r);
```

Sprites:

```cpp
void* createSprite(int32_t w, int32_t h);
void pushSprite(int32_t x, int32_t y);
```

Optional (Option 2):

```cpp
void setScrollRect(int32_t x, int32_t y, int32_t w, int32_t h);
void scroll(int_fast16_t dx, int_fast16_t dy = 0);
void setClipRect(int32_t x, int32_t y, int32_t w, int32_t h);
void clearClipRect(void);
```

## Suggested component API (what to implement)

Create a tiny “ListView” module with these responsibilities:

- store items (`std::vector<std::string>`)
- store selection index and scroll offset
- render into a target surface (sprite/canvas)
- handle key events (Up/Down/PageUp/PageDown/Enter)

Pseudocode interface:

```cpp
struct ListViewStyle {
  int padding_x = 6;
  int row_gap = 2;
  uint32_t bg = TFT_BLACK;
  uint32_t fg = TFT_WHITE;
  uint32_t sel_bg = TFT_BLUE;
  uint32_t sel_fg = TFT_WHITE;
  uint32_t border = TFT_DARKGREY;
};

class ListView {
public:
  void set_items(std::vector<std::string> items);
  void set_bounds(int x, int y, int w, int h);
  void set_style(ListViewStyle s);

  int selected_index() const;
  void set_selected_index(int idx);

  // returns true if selection or scroll changed
  bool on_key(int key);

  // render into the provided sprite/canvas
  void render(lgfx::LovyanGFX& g);
};
```

## Layout math (the part interns often get wrong)

Define the row height using the actual font metrics:

```cpp
const int line_h = g.fontHeight() + style.row_gap;
const int visible_rows = (bounds_h - 2 * padding_y) / line_h;
```

Scroll offset should represent the index of the top visible row (`scroll_top_index`).

Rule to keep selection visible:

```text
if selected < scroll_top: scroll_top = selected
if selected >= scroll_top + visible_rows: scroll_top = selected - visible_rows + 1
```

## String fitting (truncate with ellipsis)

On a small screen, long labels must be truncated.

Strategy:

- reserve space for `"..."` (or `".."`) and cut until `textWidth(cut + "...") <= max_w`

Pseudocode:

```cpp
std::string fit_label(lgfx::LovyanGFX& g, const std::string& s, int max_w) {
  if (g.textWidth(s.c_str()) <= max_w) return s;
  const char* ell = "...";
  int ell_w = g.textWidth(ell);
  std::string out = s;
  while (!out.empty() && g.textWidth(out.c_str()) + ell_w > max_w) {
    out.pop_back();
  }
  return out + ell;
}
```

## Rendering algorithm (Option 1: sprite redraw)

Render the entire viewport each time the list is dirty:

```cpp
void ListView::render(lgfx::LovyanGFX& g) {
  g.fillRect(x, y, w, h, style.bg);
  g.drawRect(x, y, w, h, style.border);

  g.setTextDatum(lgfx::textdatum_t::top_left);
  g.setTextSize(1, 1);

  int row_y = y + padding_y;
  for (int i = 0; i < visible_rows; i++) {
    int idx = scroll_top + i;
    if (idx >= items.size()) break;

    bool sel = (idx == selected);
    int row_h = line_h;

    if (sel) {
      g.fillRoundRect(x + 2, row_y, w - 4, row_h - 1, 4, style.sel_bg);
      g.setTextColor(style.sel_fg, style.sel_bg);
    } else {
      g.setTextColor(style.fg, style.bg);
    }

    auto label = fit_label(g, items[idx], w - 2 * padding_x);
    g.drawString(label.c_str(), x + padding_x, row_y + 1);

    row_y += line_h;
  }
}
```

## Rendering algorithm (Option 2: scrollRect incremental row draw)

Only redraw one row when scrolling.

Key idea: when moving selection down past bottom:

- scroll the list viewport up by one row height
- draw the next item at the bottom

Pseudocode:

```cpp
g.setScrollRect(x, y, w, h);
g.scroll(0, -line_h);
draw_row_at(y + h - line_h, idx_new_bottom);
```

This approach requires careful control of background clearing and selection redraw; use it after Option 1 is stable.

## Integration into the demo firmware

Minimum integration requirements:

- The demo launcher uses the same `ListView` component.
- Demo “List View + Selection” is a screen that:
  - shows a list of fake items (or real demo list)
  - shows a help overlay describing reuse cases
  - has a toggle to switch between Option 1 and Option 2 (if implemented)

## Validation checklist (what to test)

- A list of 5 items fits without scrolling.
- A list of 100 items scrolls correctly to the last item and back to the top.
- Rapid key-repeat doesn’t tear/flicker; the selected row remains readable.
- Long strings truncate cleanly without visual overflow.

## Common pitfalls

- Using a hard-coded row height instead of `fontHeight()`: leads to overlap with different fonts.
- Forgetting to update `scroll_top` when selection changes: selection disappears off-screen.
- Redrawing everything every frame unnecessarily: use a `dirty` flag and only redraw when input changes.
