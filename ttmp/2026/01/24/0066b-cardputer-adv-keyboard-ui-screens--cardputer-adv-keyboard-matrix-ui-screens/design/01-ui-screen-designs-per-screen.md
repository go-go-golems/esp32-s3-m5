---
Title: UI screen designs (per-screen)
Ticket: 0066b-cardputer-adv-keyboard-ui-screens
Status: active
Topics:
    - cardputer
    - keyboard
    - ui
    - m5gfx
    - led-sim
    - console
    - web-ui
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp
      Note: Existing always-on LED preview rendering
    - Path: ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/sources/local/Cardputer UI mock.md
      Note: Primary on-device UI spec input
    - Path: ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/sources/local/Cardputer Web UI mock.md
      Note: Primary web UI spec input
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T00:46:16.189018083-05:00
WhatFor: ""
WhenToUse: ""
---


# UI screen designs (per-screen): Cardputer‑side overlays + Web UI pages

This document turns two “UI mock” specs into implementable designs:

- **On-device UI** (Cardputer screen + keyboard): always-on LED preview plus modal overlays.
- **Web UI** (browser): multi-screen dashboard/patterns/JS/GPIO/presets/system.

The goal is *not* to be artistically perfect; it is to be operationally precise:

- what each screen shows,
- what inputs it consumes,
- how state is stored,
- what side-effects it causes (pattern changes, JS eval, GPIO toggles),
- how the “always-on preview” remains stable.

Primary spec sources (imported into this ticket):

- `sources/local/Cardputer UI mock.md`
- `sources/local/Cardputer Web UI mock.md`

## 0) Unifying architecture: “Preview is the floor, overlays are the ceiling”

### The invariant

At all times, the 10×5 LED preview grid is visible (unless a fatal error screen replaces it). Overlays never own the whole screen; they draw **on top** of the preview and may dim it or reserve a region.

### The rendering model

One frame consists of:

1) Draw status bar (top)
2) Draw LED preview (middle)
3) Draw hint bar (bottom) — depends on active mode
4) Draw overlay panel(s) (if any)

In M5GFX terms, this is a single sprite (`M5Canvas`) drawn to LCD once per frame.

### The input model

The UI consumes `KeyEvent` objects (physical scan already interpreted into:

- semantic actions: up/down/left/right/enter/esc/tab/space/del
- raw key “letters” (e.g. `p`, `E`, `;`) when needed
- modifier booleans: ctrl/alt/fn/shift

> **Callout: keep input semantic, not ASCII**
>
> A UI wants “Back”, not “Fn + keyNum 1”. The keyboard layer exists to translate raw scan into these semantics.

## 1) On-device UI: global conventions

### Modes

We represent UI as a mode machine:

- `Live` (no overlay)
- `Menu`
- `PatternSelect`
- `ParamsEditor`
- `ColorEditor`
- `BrightnessSlider`
- `FrameSlider`
- `Presets`
- `JsLab`
- `Help`

### Navigation keys

- `up/down`: move selection
- `left/right`: adjust value (or change tab)
- `enter`: accept / open editor / apply
- `esc` (Back): close overlay / go back
- `tab`: open menu (or cycle focus)

### Modifier semantics (consistent everywhere)

From the spec:

- Default arrow step: **small**
- `Alt`/`Opt`: **medium**
- `Ctrl`: **large**
- `Fn`: **micro**

Implementation rule:

```
step = base
if fn:  step = micro
if alt: step = medium
if ctrl: step = large   // ctrl wins if combined
```

The concrete step sizes vary per parameter (documented per screen below).

### Apply model (MVP)

We choose **Immediate apply** (Option 1 in the spec):

- Any change updates the simulator engine immediately.
- Overlays are transient editors; they don’t create a draft config.

Rationale: it matches the current firmware structure in tutorial 0066 (UI reads `sim_engine_get_cfg()` each frame), and it yields fast feedback.

## 2) On-device screen designs (per-screen)

Each screen below is specified as:

- **Purpose**
- **Layout**
- **State**
- **Inputs**
- **Transitions**
- **Side effects**
- **Implementation notes**

### 2.1 Live Preview (default)

**Purpose**

- Show the preview grid continuously.
- Provide always-visible “what is running” information.

**Layout**

- Top status bar: `pattern | bri | frame_ms | (wifi/js indicators)`
- Middle: 10×5 LED grid
- Bottom hint strip: “soft shortcuts” for current mode

**State**

- none (derived from `sim_engine_get_cfg` + runtime flags)

**Inputs**

- `tab` → Menu
- `p` → PatternSelect
- `e` → ParamsEditor
- `b` → BrightnessSlider
- `f` → FrameSlider
- `j` → JsLab
- `fn+h` → Help

**Side effects**

- none

**Implementation notes**

- Live Preview is what `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp` already does; we layer bars + overlays on top.

### 2.2 Menu (TAB)

**Purpose**

- Launcher for all overlays, reducing “shortcut memorization”.

**Layout**

- Center-left panel with vertical list:
  - Pattern
  - Params
  - Brightness
  - Frame time
  - Presets
  - JS Lab
  - System (optional)
  - Help

**State**

- `selected_index`

**Inputs**

- up/down: move selection
- enter: open selected screen
- esc: close menu → Live

**Transitions**

- enter on an item → corresponding screen

**Side effects**

- none

**Implementation notes**

- Keep list static and short; allow future additions without breaking UX.

### 2.3 Pattern Select (P)

**Purpose**

- Choose the active pattern quickly.

**Layout**

- Panel with list of patterns
- Show current pattern

**State**

- `selected_pattern`

**Inputs**

- up/down: choose pattern
- enter: apply pattern, close (or remain open based on preference; MVP closes)
- esc: close

**Side effects**

- `sim_engine_set_cfg(... type = selected_pattern ...)`

### 2.4 Params Editor (E)

**Purpose**

- Edit parameters of the current pattern.

**Layout**

- List of parameter rows: `name  value`
- The list depends on the active pattern.

Example rows (chase):

- speed, tail, gap, trains, dir, fade, fg, bg

**State**

- `selected_field`

**Inputs**

- up/down: select field
- left/right: adjust numeric/enum/bool
- enter: open sub-editor for colors (ColorEditor) or for “enum pickers” if needed
- esc: close
- ctrl+r: reset selected field to default (optional; spec mentions Ctrl meta actions)

**Side effects**

- Updates pattern config immediately via `sim_engine_set_cfg`.

**Adjustment steps (MVP)**

These are intentionally simple and can be tuned later:

- speed (0..255):
  - micro 1, small 5, medium 15, large 40
- tail/gap/trains:
  - micro 1, small 1, medium 2, large 5
- brightness pct (1..100):
  - micro 1, small 1, medium 5, large 10

### 2.5 Color Editor (for fg/bg/etc)

**Purpose**

- Edit an RGB color with predictable operations.

**Layout**

- Show current `#RRGGBB`
- Show R/G/B values and bar meters
- Optionally accept hex typing

**State**

- `which_color` (fg/bg/color/etc)
- `active_channel` (R/G/B)
- `hex_buffer` (optional)
- `original_color` (for cancel)

**Inputs**

- tab: next channel
- up/down or left/right: adjust channel by step
- hex typing (0-9, A-F): append to buffer
- enter: accept
- esc: cancel (restore original)

**Side effects**

- Updates selected color in config.

### 2.6 Brightness Slider

**Purpose**

- Fast global brightness changes.

**Layout**

- Slider bar + numeric value

**State**

- none (reads current brightness, writes new brightness)

**Inputs**

- left/right: adjust brightness pct
- enter/esc: close

**Side effects**

- `cfg.global_brightness_pct = new_value`

### 2.7 Frame Time Slider

**Purpose**

- Adjust animation frame rate / CPU tradeoff.

**Layout**

- Slider bar + numeric `frame_ms`

**State**

- none (reads current frame_ms, writes new frame_ms)

**Inputs**

- left/right: adjust frame_ms
- enter/esc: close

**Side effects**

- `sim_engine_set_frame_ms(engine, frame_ms)`

### 2.8 Presets

**Purpose**

- Store/reload named or numbered configurations quickly.

**MVP scope**

- 6 slots in RAM (optional NVS later).
- Save current config to slot; load slot into engine.

**Layout**

- list of slots with status (empty / saved)

**State**

- `selected_slot`

**Inputs**

- up/down: select slot
- enter: load slot
- ctrl+s (or alt+enter): save current config to slot
- del: clear slot
- esc: close

### 2.9 JS Lab (device-side)

**Purpose**

- Run small JS snippets without leaving the device UI.

**MVP scope**

- No full text editor (hard on small keyboard UI without a text widget).
- Provide a list of examples; selecting one runs it and shows output.

**Layout**

- left panel: example list
- right/bottom: output log (last N lines)

**State**

- `selected_example`
- `last_output` (string buffer, bounded)

**Inputs**

- up/down: select example
- enter: run example
- esc: close

**Side effects**

- Call into `js_service_eval(...)` with example code.

### 2.10 Help (Fn+H)

**Purpose**

- Show shortcuts and modifier semantics.

**Layout**

- small cheat sheet listing:
  - TAB menu, P pattern, E params, B brightness, F frame, J JS, Fn+H help
  - modifiers: Fn micro, Alt medium, Ctrl large

**State**

- none

**Inputs**

- any of esc/tab/help toggles closes

## 3) Web UI pages (browser) — per-screen design

The imported “Web UI mock” describes a multi-screen browser UI. For firmware simplicity, we implement it as a *single-page app without a build system*:

- `index.html` with tab buttons
- one `app.js` that swaps sections and calls REST endpoints

### 3.1 Dashboard

**Purpose**

- Show current status and quick controls (pattern, brightness, frame).

**Data**

- `GET /api/status`
- `POST /api/apply` for pattern/bri/frame changes

### 3.2 Patterns

**Purpose**

- Provide pattern-specific editors (rainbow/chase/breathing/sparkle).

**Data**

- Reads from `GET /api/status` (includes pattern params)
- Writes via `POST /api/apply` with nested objects (already implemented in 0066)

### 3.3 JS Lab

**Purpose**

- Browser editor for JS; run on device; view output and events.

**Data**

- `POST /api/js/eval` (raw JS body)
- `/ws` (JSON frames)

### 3.4 GPIO

**Purpose**

- Quick toggles and status for G3/G4.

**MVP data path**

- Use JS eval to call `gpio.toggle("G3")` etc (since 0066 already exposes that in JS).

### 3.5 Presets

**Purpose**

- Store/restore pattern configs.

**MVP**

- Client-side presets in `localStorage` (no new firmware endpoints).

### 3.6 System

**Purpose**

- Wi‑Fi status, firmware info, memory stats.

**MVP**

- Reuse existing `wifi` console for configuration; the web UI can display status only.

## 4) Implementation sequencing recommendation

Implement in layers:

1) Keyboard scan integration + event queue
2) Menu + PatternSelect + Brightness + Frame (fast wins)
3) ParamsEditor (numeric/bool/enum)
4) ColorEditor
5) Presets (RAM-only) and JS Lab (examples-only)

This matches a “vertical slice” development style: every step is usable.
