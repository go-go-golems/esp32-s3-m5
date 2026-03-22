---
Title: UX Improvement Diary
Ticket: ESP-33-PAPERS3-ALPHABET-GRAFFITI
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
    - ux
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.cpp
      Note: Main app file - complete layout and rendering rewrite for touch-friendly UX
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.h
      Note: Header - removed 3 card rects
ExternalSources: []
Summary: Step-by-step UX redesign diary for the PaperS3 alphabet graffiti app, tracking layout and interaction improvements for embedded touchscreen use.
LastUpdated: 2026-03-21T21:53:22.031347874-04:00
WhatFor: Record UX decisions, before/after comparisons, and rationale for the embedded UI redesign
WhenToUse: When reviewing or continuing UX work on the alphabet graffiti app
---


# UX Improvement Diary

## Goal

Track the complete UX redesign of the PaperS3 Alphabet Graffiti app from an engineer-oriented debug layout to a touch-friendly embedded interface suitable for finger interaction on the M5Paper S3's 960x540 e-ink touchscreen.

## Context

The M5Paper S3 has a 4.7" 960x540 e-ink touchscreen (landscape via rotation 1). E-ink is slow to refresh, so minimizing full-screen redraws matters. The app has two modes: **Train** (record glyph templates) and **Write** (draw to compose text via recognition).

---

## Step 1: Audit the existing UI — identify all UX problems

The original layout was designed for development, not for use. This step catalogues every problem before making changes.

### Prompt Context

**User prompt (verbatim):** "Let's work on docmgr ticket ESP-33-PAPERS3-ALPHABET-GRAFFITI and @0077-papers3-alphabet-graffiti/ and make the eUI more simple, more suitable for embedded device, because ight now the buttons are small and hard to reach and the UX is very engineery and it looks like ass."

**Assistant interpretation:** Redesign the UI to be touch-friendly, visually clean, and suitable for a non-engineer using it on an embedded e-ink device.

**Inferred user intent:** Make the app pleasant and usable for finger-driven interaction on e-ink hardware, removing developer-oriented debug cruft.

### What I did

Audited every UI element in the existing `BuildLayout()` and `Draw*()` methods.

### Problems found

| Element | Size | Problem |
|---------|------|---------|
| Mode buttons (TRAIN/WRITE) | 92x28, 86x28 | Far too small for finger touch (minimum 44px recommended) |
| Glyph picker slots | ~(rw-44)/4 x 28 | 28px tall, hard to hit accurately |
| Action buttons (SAVE/CLEAR/DELETE/RELOAD) | ~half-width x 26 | 26px tall, crammed in a 2x2 grid in an 84px card |
| Page nav (PREV/NEXT) | 56x24 | Tiny, easy to miss |
| Metrics card | 74px tall | Shows debug info (raw points, centroid) — useless to end user |
| Results card | 66px tall | Shows storage status and match scores — engineering detail |
| Canvas area | 410x420 | Decent size but crowded by 4 sidebar cards |
| Right sidebar | 4 stacked cards (172+84+74+66) | Too dense, information overload |
| Header subtitle | Full sentence | Wastes vertical space with wordy instructions |

### Layout problems

1. **Information overload**: 6 distinct card regions visible at once (header, canvas, palette, controls, metrics, results)
2. **Small touch targets**: Every interactive element is under 30px tall
3. **Mode-agnostic layout**: Same sidebar layout for both Train and Write modes, despite very different needs
4. **Debug info on screen**: Centroid coordinates, raw point counts, storage paths — developer detail shown to user
5. **Wasted space**: In Write mode, the glyph picker is repurposed as a text preview, but the controls/metrics/results cards stay the same size

### Design principles for the redesign

1. **Minimum 48px touch targets** — ideally 52px for all buttons
2. **Mode-specific layouts** — Train needs sidebar for glyph picker; Write needs full-width canvas
3. **Remove debug info** — no metrics card, no results card; fold essential status into existing areas
4. **Bottom action bar** — large buttons spanning full width, natural thumb zone
5. **Maximize canvas** — the primary interaction surface should be as large as possible
6. **Simple visual hierarchy** — header, one content area, one action bar

---

## Step 2: Implement the new touch-friendly layout

Rewrote the entire layout and rendering pipeline. Removed 3 cards (controls, metrics, results), replaced with a full-width bottom action bar and a write-mode text buffer bar. Made every touch target at least 44px tall, most 48-52px.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Implement the redesigned layout per the audit findings.

**Inferred user intent:** Ship a usable, non-ugly UI.

### What I did

- **`alphabet_app.h`**: Removed `controls_card_`, `metrics_card_`, `results_card_` rects. Added `text_buffer_bar_`. Removed `DrawControlsCard()`, `DrawMetricsCard()`, `DrawResultsCard()` declarations. Added `DrawBottomBar()`, `DrawTextBufferBar()`. Removed `FormatPoint()`, `ModeSubtitle()`, `WriteBufferPreview()`.
- **`alphabet_app.cpp`**: Complete rewrite of `BuildLayout()` with mode-specific logic. Rewrote `DrawHeader()` (removed subtitle, larger mode buttons). Simplified `DrawCanvasCard()`. Rewrote `DrawPaletteCard()` (bigger cells via font 4, status area replaces metrics/results cards). New `DrawBottomBar()` and `DrawTextBufferBar()`. Updated `HandleTouch()` to call `BuildLayout()` on mode switch. Removed `FormatPoint()`, `ModeSubtitle()`, `WriteBufferPreview()`.

### New layout — Train mode (960x540)

```
+------------------------------------------------------------------+
| Alphabet Graffiti                      [ TRAIN ]  [ WRITE ]      | 52px
+--------------------------------------+---------------------------+
|                                      | Select glyph              |
|                                      | [A*] [B ] [C*] [D ]      |
|                                      | [E ] [F*] [G ] [H*]      |
|        DRAW HERE                     | [I ] [J ] [K ] [L ]      |
|        (588 x ~370 canvas)           | [< PREV]     [NEXT >]    |
|                                      | 8 saved | page 1/3       |
|                                      | Selected: A (1/36)        |
|                                      | Match: A (0.95)           |
+----------+---------+---------+-------+---------------------------+
|   SAVE   |  CLEAR  | DELETE  | RELOAD                           | 52px
+----------+---------+---------+-----------------------------------+
```

### New layout — Write mode (960x540)

```
+------------------------------------------------------------------+
| Alphabet Graffiti                      [ TRAIN ]  [ WRITE ]      | 52px
+------------------------------------------------------------------+
| HELLO WORLD_                                                      | 72px
| Draw to write.                                                    |
+------------------------------------------------------------------+
|                                                                    |
|              DRAW HERE (928 x ~250 canvas)                        |
|                                                                    |
+--------------------+---------------------+-----------------------+
|      SPACE         |       BACK          |     CLEAR ALL         | 52px
+--------------------+---------------------+-----------------------+
```

### Touch target comparison (before → after)

| Element | Before | After | Height increase |
|---------|--------|-------|-----------------|
| Mode buttons | 92x28, 86x28 | 140x44 | +57% |
| Glyph slots | var x 28 | 76x52 | +86% |
| Action buttons | var x 26 (2x2 grid) | ~226x52 (full-width bar) | +100% |
| Page nav | 56x24 | 80x48 | +100% |

### What was removed

| Removed | Reason |
|---------|--------|
| Metrics card (raw points, centroid, text len) | Debug info, useless to user |
| Results card (storage path, match score detail) | Folded into palette sidebar status area (train) and text buffer bar (write) |
| Header subtitle ("Record templates for A-Z...") | Wordy, mode obvious from toggle |
| `FormatPoint()` | Only used by removed metrics card |
| `ModeSubtitle()` | Removed from header |
| `WriteBufferPreview()` (wrapping logic) | Replaced by inline display in `DrawTextBufferBar()` |

### Why

The original layout was a debug dashboard. For actual use, the user needs: a big canvas, easy glyph selection, and clear action buttons. The metrics card showing "Raw points: 47" and "Centroid: (234, 189)" is purely developer introspection. The results card showing storage paths is similar. All essential info (selected glyph, match result, storage count) is now in the palette sidebar's status area.

### What worked

- Build compiles cleanly with zero warnings on ESP-IDF 5.4.1
- Mode-specific layout via `BuildLayout()` re-invocation on mode switch works well
- Font 4 for glyph labels and bottom bar buttons gives much better readability
- Reusing `save_button_`/`clear_button_`/`delete_button_`/`reload_button_` rects for both modes avoids code duplication in touch handling

### What didn't work

- Initial IDF 5.3.4 build environment had a stale Python env. Had to use IDF 5.4.1 with `fullclean`.

### What I learned

- E-ink apps benefit enormously from mode-specific layouts. The train and write modes have fundamentally different space needs.
- Bottom-bar action placement is far more natural for touch devices than sidebar buttons — thumb reach is better.
- Font 4 in M5GFX is ~26px tall, which fits nicely in 44-52px buttons with good padding.

### What was tricky to build

The layout must be fully recalculated on mode switch because canvas position and size change (narrow+sidebar in train, full-width in write). This means `BuildLayout()` is called both at init and on mode toggle. The key insight was that slot_rects_ and palette_card_ become stale in write mode, but the existing `mode_ == Mode::train` guards in `HandleTouch()` prevent them from being hit. I also zero out unused rects (`palette_card_ = {0,0,0,0}`, `reload_button_ = {0,0,0,0}`) in write mode as a safety net.

### What warrants a second pair of eyes

- **E-ink refresh with larger canvas**: In write mode the canvas is now ~928px wide vs 410px before. The partial refresh (`epd_fast`) covers more pixels per stroke segment — may need timing adjustment if ghosting appears on hardware.
- **Palette zero-rect safety**: Touch handling relies on mode guards, not rect bounds. If mode state ever gets confused, stale rects could cause phantom touches.

### What should be done in the future

- Test on actual hardware with finger interaction and adjust sizes if needed
- Consider a large glyph preview character (80px font) in the sidebar
- Add swipe gestures for page navigation
- Consider visual press feedback (brief invert on touch-down before e-ink refresh)

### Code review instructions

- Start at `BuildLayout()` in `alphabet_app.cpp` — verify the pixel math for 960x540
- Check `DrawBottomBar()` and `DrawTextBufferBar()` — new rendering methods
- Verify `HandleTouch()` mode switch branches now call `BuildLayout()`
- Confirm no remaining references to removed methods (`DrawControlsCard`, `DrawMetricsCard`, `DrawResultsCard`, `FormatPoint`, `ModeSubtitle`, `WriteBufferPreview`)
- Run: `idf.py build` — should complete with 0 errors

### Technical details

Key constants changed:
```cpp
kScreenMargin: 18 → 16
kHeaderHeight: 64 → 52
kBottomBarHeight: 52 (new)
kSlotCellWidth: 76, kSlotCellHeight: 52 (new, was ~var x 28)
kButtonGap: 8, kCellGap: 8 (new)
// Removed: kCanvasCardWidth (410), kCanvasCardHeight (420) — now dynamic
```

---

## Step 3: Apply Serial Experiments Lain visual theme

Restyled the entire UI from a clean-but-generic look to a Navi/Copland OS aesthetic inspired by Serial Experiments Lain. The e-ink display's black/white/gray limitation actually suits the Lain aesthetic perfectly — high contrast, angular, technical.

### Prompt Context

**User prompt (verbatim):** "now make it look all serial experiments lain like, make sure things are committed before you move on."

**Assistant interpretation:** Apply a Serial Experiments Lain / Navi computer interface visual theme to the existing touch-friendly layout. Commit the layout work first.

**Inferred user intent:** Give the app a distinctive, atmospheric visual identity instead of a generic embedded UI look.

**Commit (code):** b2ac5db — "feat(papers3): apply Serial Experiments Lain visual theme"

### What I did

- **Color palette**: Replaced 9 soft gray/off-white colors with 5 high-contrast Lain colors (`kColorBg`, `kColorFg`, `kColorDim`, `kColorGhost`, `kColorRecorded`)
- **DrawRoundCard → DrawPanel**: New helper draws sharp-cornered panels with a black title strip at the top (white text on black bar), replacing the old rounded-corner cards
- **DrawButton**: Rewrote for inverted active style — active buttons are solid black with white text, inactive are outlined. Sharp corners throughout.
- **DrawCornerMarks**: New helper draws L-shaped targeting brackets at the four corners of the canvas — Navi crosshair aesthetic
- **DrawHeader**: Full black bar with "NAVI://graffiti" title, "LAYER:07" indicator, mode buttons that glow white when active on the dark bar
- **Panel titles**: Technical Lain labels — "INPUT_STREAM", "GLYPH_DB", "OUTPUT_BUFFER", "HANDWRITING_INPUT"
- **Button labels**: Terminal-style `>` prefix — ">SAVE", ">CLEAR", ">DELETE", ">RELOAD", ">SPACE", ">BACK"
- **Status messages**: Terse protocol-style — "> TRAIN protocol active.", "> APPEND: A [0.93]", "> Buffer cleared."
- **Glyph chips**: Selected = fully inverted (black), recorded = dark gray with white text, empty = outlined
- **Empty canvas placeholder**: "> input glyph here" / "> draw to append"

### Visual identity mapping (Lain → e-ink)

| Lain/Navi element | E-ink implementation |
|-------------------|---------------------|
| Dark terminal backgrounds | Black header bar, black panel title strips |
| Green/amber phosphor text | White text on black (e-ink equivalent of glow) |
| Angular Copland OS windows | Sharp-cornered panels with double borders |
| Navi targeting crosshairs | L-shaped corner marks on canvas |
| Protocol/layer numbering | "LAYER:07" indicator, "PROTOCOL:TRAIN" messages |
| System command prompts | `>` prefix on all button labels and status text |
| CRT scanline aesthetic | Horizontal title strips creating visual rhythm |

### Why

The Lain aesthetic is a perfect match for e-ink:
1. E-ink renders pure black and pure white crisply — the high-contrast palette looks sharp
2. The angular, technical look avoids the "rounded corners = friendly" cliche that feels wrong on embedded hardware
3. The dark title strips create strong visual hierarchy without needing color
4. The "NAVI://graffiti" framing gives the app personality and atmosphere

### What worked

- The inverted black header bar immediately establishes the Lain mood
- Corner targeting marks on the canvas are a subtle but effective Navi reference
- Build compiles cleanly — the color refactoring was clean (all old color names eliminated)
- The `>` prefix on buttons gives a command-line feel without being unreadable

### What didn't work

- IDF 5.3.4 python env was missing; had to use `IDF_PYTHON_ENV_PATH=$HOME/.espressif/python_env/idf5.3_py3.13_env` explicitly

### What I learned

- E-ink's limitation (no color) is actually a feature for the Lain aesthetic — it naturally produces the monochrome terminal look
- Inverted regions (white text on black fill) are the e-ink equivalent of "glowing" UI elements
- Sharp corners render better on e-ink than rounded ones — fewer antialiased gray pixels

### What was tricky to build

The header mode buttons sit on a black background, so the generic DrawButton (which makes active buttons black) would be invisible. Had to write a custom inline lambda `draw_mode_btn` that inverts the logic: active buttons are WHITE (bright) on the dark header, inactive ones blend in with just a dim border. This is the opposite of buttons on the white content area.

The `replace_all` for old color names missed multi-line wrapped references where `kColorInk` was on a continuation line. Had to manually find and fix 2 remaining instances in `DrawLiveStrokeSegment`.

### What warrants a second pair of eyes

- **Glyph chip readability**: `kColorRecorded = 0x404040` (dark gray) with white text — verify this has enough contrast on actual e-ink hardware. E-ink gray levels can be muddy.
- **Corner marks on partial refresh**: The corner marks are redrawn in `ClearCanvasForLiveStroke()` during fast partial refresh. Verify they don't ghost or smear.

### What should be done in the future

- Add a small Navi logo or Lain silhouette as a decorative element
- Consider a "boot sequence" animation on startup (text cascade)
- Add thin horizontal scanline rules between status text lines
- Try `kColorRecorded = 0x303030` if 0x404040 is too light on real hardware

### Code review instructions

- Check color constants — all old names (`kColorPaper`, `kColorInk`, etc.) should be gone
- Look at `DrawHeader()` — the mode button drawing has custom logic for dark-background buttons
- Check `DrawCornerMarks()` — new helper for canvas targeting aesthetic
- Verify `DrawPanel()` title strip height (24px) doesn't clip text
- Run: `idf.py build` — should complete with 0 errors

### Technical details

Color palette change:
```cpp
// Before (soft grays)
kColorPaper = 0xFFFFFF, kColorCard = 0xF3F3F1, kColorCardDark = 0xE1E1DE
kColorInk = 0x000000, kColorMuted = 0x5A5A5A, kColorLightInk = 0x888888
kColorAccent = 0x202020, kColorAccentFill = 0xD7D7D2, kColorDisabled = 0xCDCDC8

// After (high contrast Lain)
kColorBg = 0xFFFFFF, kColorFg = 0x000000, kColorDim = 0x666666
kColorGhost = 0xAAAAAA, kColorRecorded = 0x404040
```
