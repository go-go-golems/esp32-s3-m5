---
Title: 'Implementation guide: Terminal/log console (E1)'
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
      Note: scrollRect/scroll/textWidth/fontHeight APIs
    - Path: 0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Reference for keyboard scan
ExternalSources: []
Summary: Intern guide for implementing a reusable terminal/log console UI (scrollback, wrapping, dirty redraw, optional scrollRect fast path).
LastUpdated: 2026-01-01T20:09:20.486699761-05:00
WhatFor: ""
WhenToUse: ""
---



# Implementation guide: Terminal/log console (E1)

This guide explains how to implement a **terminal/log console** demo that is realistic enough to reuse in actual firmware (debug console, REPL output, chat logs, serial terminal).

On Cardputer, this is one of the highest-value demos because it exercises:

- scrolling content efficiently
- line wrapping
- keyboard input and editing basics
- incremental redraw patterns (dirty region, scrollRect)

## Outcome (acceptance criteria)

- Console shows an output area with scrollback.
- New lines append smoothly without full-screen flicker.
- Basic input line supports typing, backspace, and enter (local echo is fine).
- Up/Down scrolls history (at least within visible viewport).

## Where to look (existing reference implementation)

The closest thing to a usable terminal in this repo is tutorial 0015:

- `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`

It contains:

- keyboard scanning and key mapping
- line buffering (`std::deque<std::string>`)
- redraw-on-dirty pattern using `M5Canvas`

Use it as your primary reference for input handling and text layout.

## Two rendering approaches (pick one)

### Approach 1 (recommended first): redraw visible region into a sprite (stable)

Maintain a scrollback buffer in RAM and redraw the visible lines into the content sprite when:

- a new line is added
- the view scroll offset changes
- the input line changes

Pros:

- simplest and hardest to break
- easy to layer with header/footer chrome

Cons:

- redraw cost scales with number of visible rows (still fine)

### Approach 2 (advanced): scrollRect + draw one new line (fastest)

Use `setScrollRect()` + `scroll()` to shift the output region up by one line height, and draw only the new bottom line.

Pros:

- extremely fast and “real terminal” feeling

Cons:

- more careful about backgrounds and clipping
- trickier if you want arbitrary scrollback (not just auto-follow)

Start with Approach 1, then add Approach 2 as an optional “fast mode”.

## Key M5GFX/LGFX APIs

Text:

```cpp
void setTextSize(float sx, float sy);
void setTextDatum(textdatum_t datum);
void setTextColor(T fg, T bg);
int32_t fontHeight(void) const;
int32_t textWidth(const char* s);
size_t drawString(const char* s, int32_t x, int32_t y);
```

Scroll helpers (Approach 2):

```cpp
void setScrollRect(int32_t x, int32_t y, int32_t w, int32_t h);
void scroll(int_fast16_t dx, int_fast16_t dy = 0);
```

Sprites:

```cpp
void* createSprite(int32_t w, int32_t h);
void pushSprite(int32_t x, int32_t y);
```

## Data model

Keep the console state small and explicit:

```cpp
struct ConsoleState {
  std::deque<std::string> lines;     // scrollback lines
  std::string input;                // current input line
  size_t dropped_lines = 0;          // how many were trimmed from top

  int scrollback = 0;               // 0 = follow tail; >0 = user scrolled up by N lines
  bool dirty = true;
};
```

### Line limits (important)

Keep a hard cap to avoid unbounded memory growth:

- `max_lines = 256` (or configurable)

When exceeding, pop from front and increment `dropped_lines`.

## Layout (recommended)

Assume your content area excludes a header/footer:

- Output region: top part of content area
- Input region: one line at bottom of content area (with a prompt)

Example (content height ~119):

```text
| output lines...                   |
|                                   |
| > input line here_                |
```

Compute sizes using font metrics:

```cpp
int line_h = g.fontHeight() + 1;
int input_h = line_h + 2;
int output_h = content_h - input_h;
int output_rows = output_h / line_h;
```

## Wrapping strategy (keep it simple)

Do not rely on automatic wrap for a terminal; make it deterministic:

- define a max pixel width for text (`max_w`)
- when appending a line, wrap it into multiple `lines` entries

Greedy wrap pseudocode using `textWidth()`:

```cpp
std::vector<std::string> wrap_line(lgfx::LovyanGFX& g, std::string s, int max_w) {
  std::vector<std::string> out;
  while (!s.empty()) {
    size_t cut = s.size();
    while (cut > 0 && g.textWidth(s.substr(0, cut).c_str()) > max_w) cut--;
    if (cut == 0) cut = 1;
    out.push_back(s.substr(0, cut));
    s.erase(0, cut);
  }
  return out;
}
```

This is O(n^2) in worst case, but with short lines and small screen it’s acceptable. If needed later, optimize by measuring character widths via the font metrics API.

## Rendering algorithm (Approach 1: redraw-on-dirty)

Pseudocode:

```cpp
void render_console(LGFX_Sprite& c, ConsoleState& s) {
  if (!s.dirty) return;

  c.fillRect(0, 0, c.width(), c.height(), TFT_BLACK);
  c.setTextSize(1, 1);
  c.setTextDatum(lgfx::textdatum_t::top_left);

  int line_h = c.fontHeight() + 1;
  int input_h = line_h + 2;
  int output_h = c.height() - input_h;
  int output_rows = output_h / line_h;

  // Determine which lines are visible (tail-follow or scrolled)
  int total = (int)s.lines.size();
  int tail_start = std::max(0, total - output_rows);
  int start = std::max(0, tail_start - s.scrollback);

  // Draw output
  c.setTextColor(TFT_GREEN, TFT_BLACK);
  for (int i = 0; i < output_rows; i++) {
    int idx = start + i;
    if (idx >= total) break;
    c.drawString(s.lines[idx].c_str(), 2, i * line_h);
  }

  // Draw input separator
  c.drawFastHLine(0, output_h, c.width(), TFT_DARKGREY);

  // Draw prompt + input
  c.setTextColor(TFT_WHITE, TFT_BLACK);
  c.drawString("> ", 2, output_h + 1);
  c.drawString(s.input.c_str(), 18, output_h + 1);

  // Cursor (simple underscore at end)
  int cursor_x = 18 + c.textWidth(s.input.c_str());
  c.drawFastHLine(cursor_x, output_h + line_h, 8, TFT_WHITE);

  s.dirty = false;
}
```

## Input handling (minimum viable)

Borrow keyboard scan + key mapping from `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`.

Minimum behaviors:

- printable keys append to `input`
- backspace removes last character
- enter:
  - append `"> " + input` to lines
  - append a fake output line (or echo it)
  - clear input
- Up/Down:
  - adjust `scrollback` (clamp to available lines)

## Optional: “auto-follow vs scrollback” UX

When `scrollback > 0`, show a small indicator (e.g., “SCROLL”) in a corner of the content area.

Press `End` (or `Shift+Down`) to return to follow-tail (`scrollback = 0`).

## Validation checklist

- Append 200+ lines and confirm it stays responsive and memory stays bounded.
- Rapid typing doesn’t drop characters (no visible lag).
- Scrolling up stops auto-follow; new lines still append, but view stays.
- Returning to tail shows latest lines.

## Common pitfalls

- Not wrapping lines: long strings disappear off the right edge and make the demo look broken.
- Trying to be a “full terminal emulator”: keep it to a log console with good UX.
- Redrawing every frame: only redraw on input/appends/scroll events.
