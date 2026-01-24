Here’s a **Cardputer-side UI** that keeps your current “LED grid always visible” vibe, but adds **keyboard-driven overlays** for patterns/params/JS. It’s designed around the modifiers you mentioned: **Ctrl / Opt / Fn / Alt**.

---

## Core idea

* **Always-on Preview Screen**: the 10×5 LED grid stays on screen.
* A thin **Top Status Bar** shows current pattern + brightness + frame time + (optional) Wi-Fi/JS indicators.
* Press a key to bring up a **modal overlay** (Menu / Pattern / Params / JS / Presets / Help).
* Everything is navigable with arrows (or HJKL), and modifiers change “how much” you adjust.

---

## Screen layout

### Default: Live Preview (no overlay)

* **Top bar**: `PATTERN | bri | frame | indicators`
* **Main**: 10×5 LED grid (what you already have)
* **Bottom hint strip**: “soft shortcuts” for the current mode

**ASCII mock**

```text
┌──────────────────────────────────────────────┐
│ chase   bri 35%   16ms   WiFi:•   JS:•       │  <- status bar
├──────────────────────────────────────────────┤
│  [10×5 LED grid preview]                     │
│  ■■□■□■■□■■□■□■■□■■□■□■■□■■                  │
│  □■□■■□■■□■□■■□■■□■□■■□■■□■                  │
│  ...                                         │
├──────────────────────────────────────────────┤
│ TAB:Menu  P:Pattern  E:Edit  J:JS  Fn+H:Help │  <- hint strip
└──────────────────────────────────────────────┘
```

---

## UI modes (overlays)

### 1) Quick Menu (TAB)

A simple launcher so you don’t need to remember shortcuts.

```text
┌──────────── MENU ────────────┐
│ ▶ Pattern                    │
│   Params                     │
│   Brightness                 │
│   Frame time                 │
│   Presets                    │
│   JS Lab                     │
│   Wi-Fi / System             │
│   Help                       │
└──────────────────────────────┘
```

Navigation: ↑↓ / Enter / Esc

---

### 2) Pattern Select (P or Menu→Pattern)

```text
┌────────── PATTERN ───────────┐
│ Current: chase               │
│                              │
│ ▶ off                        │
│   rainbow                    │
│   chase                      │
│   breathing                  │
│   sparkle                    │
│                              │
│ Enter:Apply   Esc:Back       │
└──────────────────────────────┘
```

* **Enter** applies immediately (or marks dirty if you use “Apply/Cancel” model).

---

### 3) Params Editor (E or Menu→Params)

A “form list” that edits the current pattern’s params. Selecting an item opens the right editor (number/enum/color/toggle).

Example for **Chase**:

```text
┌────────── PARAMS: CHASE ──────────┐
│ ▶ speed        128                │
│   tail          40                │
│   gap            8                │
│   trains         2                │
│   dir         FWD                 │
│   fade        ON                  │
│   fg        #FF8800               │
│   bg        #001020               │
│                                  │
│ ←→:Adjust  Enter:Open  Ctrl+R:Def │
└──────────────────────────────────┘
```

---

### 4) Color Editor (auto when selecting fg/bg/color)

Two ways to edit:

* Quick adjust with channels
* Type hex

```text
┌──────── COLOR ────────┐
│ fg: #FF8800           │
│                       │
│ R: 255  [■■■■■■■■■■]   │
│ G: 136  [■■■■■□□□□□]   │
│ B:   0  [□□□□□□□□□□]   │
│                       │
│ Tab:next channel      │
│ ←→/↑↓ adjust           │
│ Type: 0-9 A-F hex     │
│ Enter:OK   Esc:Cancel │
└───────────────────────┘
```

---

### 5) Brightness / Frame “Quick Sliders”

These are single-purpose overlays for fast tuning.

Brightness:

```text
┌────── BRIGHTNESS ──────┐
│ bri: 35%               │
│ [■■■■□□□]              │
│ ←→ adjust  Enter:OK    │
└────────────────────────┘
```

Frame time:

```text
┌──────── FRAME ─────────┐
│ frame: 16 ms (~60fps)  │
│ [■■□□□]                │
│ ←→ adjust  Enter:OK    │
└────────────────────────┘
```

---

### 6) Presets (slots)

Keep it dead simple on-device: 6–10 slots, each stores `{pattern + params + bri + frame}`.

```text
┌──────── PRESETS ────────┐
│ ▶ 1: Warm Chase         │
│   2: Cyan Breath        │
│   3: Dense Sparkle      │
│   4: (empty)            │
│                         │
│ Enter:Load              │
│ Ctrl+S:Save slot        │
│ Del:Clear slot          │
│ Esc:Back                │
└─────────────────────────┘
```

---

### 7) JS Lab (device-side)

Two tiers:

**Tier A: JS Macros (fast + usable)**

* Pick a script slot and run it.
* Great for pattern experiments without a full editor.

```text
┌──────── JS LAB ───────────────┐
│ VM: OK  heap: 64k  timers: 2  │
│                                │
│ ▶ 1: pulse_brightness.js  [RUN] │
│   2: chase_random_fg.js   [RUN] │
│   3: gpio_blink_g3.js     [RUN] │
│   4: (empty)                    │
│                                │
│ Enter:Run   Ctrl+R:Reset VM     │
│ Ctrl+S:Save current line->slot  │
│ Esc:Back                         │
└────────────────────────────────┘
```

**Tier B: One-line REPL (lightweight)**

* Type a line, eval it, show result/errors.
* Enough for rapid iteration.

```text
┌──────── JS REPL ───────────────┐
│ > sim.setSparkle({density:70}) │
│                                │
│ out: ok                         │
│ err: -                          │
│                                │
│ Enter:Eval  Ctrl+Enter:Eval+Log │
│ Ctrl+R:Reset VM  Esc:Back       │
└────────────────────────────────┘
```

(If later you add FAT/NVS file storage, “slots” can map to stored `.js` files.)

---

### 8) Help overlay (Fn+H)

Always available, shows the current keymap.

---

## Keyboard navigation model

### Focus + actions

* **↑↓** move between items (lists)
* **←→** adjust values (or switch tabs)
* **Enter** selects / opens editor / confirms
* **Esc** backs out / closes overlay
* **Tab** opens Menu (or cycles fields inside an editor)

Support both:

* **Arrow keys**
* **Vim keys** as fallback: `H J K L`

---

## Modifier semantics (consistent everywhere)

### Adjustment steps

When an item is adjustable (number/slider/channel):

* **no modifier**: normal step
* **Opt**: fine step (÷10)
* **Alt**: coarse step (×10)
* **Fn**: “jump” step (min/max or big preset jumps)
* **Ctrl**: meta action (reset/copy/paste style)

Concrete examples (recommended):

* brightness: normal ±1, Opt ±1, Alt ±10, Fn min/max
* frame_ms: normal ±1, Opt ±1, Alt ±5 or ±10, Fn snap to {16,33,50,100}
* speed (0..255): normal ±1, Opt ±1, Alt ±10, Fn ±25
* density (0..100): normal ±1, Alt ±10

### Ctrl meta actions (global conventions)

* **Ctrl+R**: reset current thing to defaults (pattern defaults, color default, etc.)
* **Ctrl+S**: save preset (when in Presets) or save JS line to slot (in JS)
* **Ctrl+L**: load preset (from Presets)
* **Ctrl+Enter**: “apply + show confirmation” (good in Params/JS)
* **Ctrl+G**: toggle “Auto-apply” mode (optional)

### Fn shortcuts (jumping)

* **Fn+1..Fn+5**: switch to pattern (off/rainbow/chase/breathing/sparkle)
* **Fn+B** brightness overlay
* **Fn+F** frame overlay
* **Fn+P** pattern overlay
* **Fn+E** params overlay
* **Fn+J** JS overlay
* **Fn+H** help overlay

(If the keyboard layout doesn’t have easy number reach, map Fn+Q/W/E/R/T instead.)

---

## Device UI behavior: Apply model

Two workable options:

### Option 1: Immediate apply (fastest)

* Changing pattern/params sends `sim_engine_set_cfg()` immediately.
* Great for live tweaking; simplest code.

### Option 2: Draft + Apply (safer)

* UI maintains a `draft_cfg` copy.
* Shows `*` in status bar if dirty.
* **Enter** applies; **Esc** discards.

On tiny screens, I like: **immediate apply for sliders**, but **draft/apply for multi-field parameter editing**.

Status bar could show: `chase* bri 35% 16ms` when draft differs from engine.

---

## How this supports device-side JS experimentation

This UI makes JS usable without serial:

* **JS Slots** let you treat scripts like “macros”:

  * “cycle patterns every 2s”
  * “randomize chase fg”
  * “blink G3”
* **One-line REPL** lets you iterate on the `sim.*` API quickly.
* You can add a “JS controls pattern” mode later (if you expose `sim.setLed(i,r,g,b)` or `sim.present(buffer)`), and it still fits: it becomes just another “Pattern: js” entry with params “script slot / fps”.

---

## Implementation notes (so it fits your current firmware structure)

* Keep the current `sim_ui` task, but add:

  * an input poll (M5Unified/M5Cardputer keyboard)
  * a small UI state machine (`ui_mode`, `focus_index`, `draft_cfg`, `dirty`)
* Rendering stays via the same `M5Canvas` you already use.
* Apply changes using existing:

  * `sim_engine_set_cfg(engine, &cfg)`
  * `sim_engine_set_frame_ms(engine, ms)`
* JS overlay will call into whatever you use today for `js eval` (same internal entry point that the console command uses).

---

If you want, I can write the **exact keymap** tailored to the Cardputer’s real key layout (which keys are physically present for arrows, enter, esc, etc.). Just tell me whether you want **arrows** or **Vim-style** as the primary navigation (or both).

