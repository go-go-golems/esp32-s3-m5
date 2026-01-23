---
Title: 'Deep Research: LED Pattern Engine + Cardputer GFX Rendering'
Ticket: 0066-cardputer-ledchain-gfx-sim
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - animation
    - esp32c6
    - ui
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: Cardputer M5GFX sprite rendering loop baseline
    - Path: 0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Cardputer UI scaffolding and multi-sprite present patterns
    - Path: 0049-xiao-esp32c6-mled-node/main/mled_effect_led.c
      Note: Adapter mapping MLED wire patterns into led_pattern_cfg_t
    - Path: components/mled_node/include/mled_protocol.h
      Note: MLED/1 wire format for pattern configs (20 bytes)
    - Path: components/ws281x/include/led_patterns.h
      Note: Pattern config/state structs and public API
    - Path: components/ws281x/src/led_patterns.c
      Note: Canonical pattern engine (rainbow/chase/breathing/sparkle/off)
    - Path: components/ws281x/src/led_ws281x.c
      Note: Brightness scaling + pixel buffer wire order (GRB/RGB)
ExternalSources: []
Summary: Map the repo’s WS281x pattern engine and Cardputer M5GFX rendering idioms; propose a faithful on-screen simulator architecture for a 50-LED chain.
LastUpdated: 2026-01-23T08:52:45.524022451-05:00
WhatFor: ""
WhenToUse: ""
---


# Deep Research: LED Pattern Engine + Cardputer GFX Rendering

## Executive Summary

We want a Cardputer (ESP32-S3) firmware that *draws* a simulated WS281x strip of 50 LEDs, representing the same patterns currently computed and driven on an ESP32-C6 (XIAO) setup. The “hard part” is not drawing 50 circles; it is preserving *semantics*: pattern parameters, timebase, and brightness scaling must match the existing firmware so the simulation is predictive and debuggable.

This repository already provides:

- A reusable “pattern engine” that computes per-LED RGB values for `off`, `rainbow`, `chase`, `breathing`, `sparkle`: `components/ws281x/src/led_patterns.c`.
- A standard per-LED buffer representation and brightness scaling behavior: `components/ws281x/src/led_ws281x.c`.
- A FreeRTOS task loop that calls the pattern engine each frame (and manages configuration): `components/ws281x/src/led_task.c` + `components/ws281x/include/led_task.h`.
- An MLED/1 protocol definition that encodes pattern configs in 20 bytes (and a node firmware that maps these onto the pattern engine): `components/mled_node/include/mled_protocol.h` + `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c`.
- Several Cardputer M5GFX examples showing reliable render loops, double-buffering, and DMA safety: `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`, `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`, `0030-cardputer-console-eventbus/main/app_main.cpp`.

The recommended approach for the simulator is: reuse the existing pattern engine unchanged, but replace the physical WS281x “show” step with a rendering step that draws each LED as a colored glyph (rect/circle) on an M5GFX-backed sprite (`M5Canvas`), then presents via `pushSprite()` (and optionally `display.waitDMA()`).

## Problem Statement

We currently generate patterns for a 50-LED WS281x chain on an ESP32-C6 and transmit those colors to real hardware. We want a Cardputer UI that displays that same chain state on-screen.

This creates three concrete requirements:

1. **Pattern equivalence:** “rainbow/chase/etc” must mean the same thing (parameter ranges, clamping behavior, time dependence).
2. **Color equivalence:** brightness scaling and color ordering must be preserved at the RGB8 level; the display will quantize to RGB565 (and might need optional gamma).
3. **Time equivalence:** patterns that depend on `now_ms` and/or `dt_ms` must be stepped with a consistent timebase and frame cadence to match expected motion.

Secondary requirements (engineering constraints):

- The Cardputer display loop must be stable (no tearing / minimal flicker) and reasonably efficient.
- The simulator should be able to accept pattern configs in a way that matches how the ESP32-C6 is currently controlled (local UI, console commands, or MLED/1 messages).

## Proposed Solution

### Proposed architecture (high level)

At the level of “what computes colors” vs “what displays colors”, we want a strict separation:

- **Pattern engine:** produces `N=50` RGB values per frame.
- **Renderer:** maps those RGB values to pixels on the Cardputer screen.
- **Controller / input:** chooses pattern type and parameters (and possibly synchronizes time with the ESP32-C6 controller).

One minimal implementation is:

1. Maintain `led_patterns_t patterns` (`components/ws281x/include/led_patterns.h`) and a *virtual strip buffer* that matches the same per-LED RGB semantics used for WS281x output.
2. On each frame:
   - Compute `now_ms` from a monotonic clock (e.g., `esp_timer_get_time()/1000`).
   - Call `led_patterns_render_to_ws281x(&patterns, now_ms, &virtual_strip)`.
   - Draw 50 LED glyphs on an `M5Canvas` sprite using the virtual strip’s pixel buffer as the source of truth.
   - Present via `canvas.pushSprite(0,0)` and optionally `display.waitDMA()` to avoid overlap/tearing.

### Why reuse the WS281x pattern engine verbatim?

Because it already encodes the semantics we care about:

- Parameter clamping and defaults (e.g., “global brightness 0 becomes 1”, “speed capped at 20”).
- The precise time dependence per pattern.
- State variables (e.g., chase position in Q16.16; sparkle per-LED brightness decay array).

Rewriting this logic “for display” would introduce semantic drift and make future debugging harder.

### Simulator inputs (three viable options)

**Option A — Local pattern selection (standalone simulator).**

- Cardputer locally sets `led_pattern_cfg_t` and steps patterns itself.
- Best for rapid iteration; does not prove “mirror the C6” unless you manually match configs.

**Option B — Mirror MLED/1 pattern configs (wire compatible).**

- Cardputer listens on UDP multicast and accepts `mled_pattern_config_t` (20 bytes) as defined in `components/mled_node/include/mled_protocol.h`.
- Reuse the existing mapping logic from `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c` (or factor it into a shared component).
- Best path if “currently controlling from the esp32c6” means “we already have an MLED controller that configures the C6 node”.

**Option C — Direct mirror of a specific ESP32-C6 node’s status.**

- If the C6 node already reports its active pattern/params (e.g. via MLED/1 PONG-like status), Cardputer can subscribe and display what the node is actually executing.
- More complex, but best for “ground truth mirroring” when time sync (`execute_at_ms`) matters.

> **Callout (Fundamental): pattern config vs pattern frame**
>
> A “pattern config” is compact and stable (type + parameters). A “pattern frame” is the result of evaluating that config at a specific time `t`, producing 50 RGB triplets. The simulator can be accurate if it shares the same config and the same notion of time as the physical node.

## Design Decisions

### Decision 1: Canonical source for pattern semantics is `components/ws281x`

There are multiple copies of `led_patterns.c` across tutorials, but the reusable component is:

- `components/ws281x/include/led_patterns.h`
- `components/ws281x/src/led_patterns.c`

Tutorial projects `0044-xiao-esp32c6-ws281x-patterns-console` and `0046-xiao-esp32c6-led-patterns-webui` also include their own local copies, but the existence of `components/ws281x` (plus `led_task.c`, `led_ws281x.c`) indicates that `components/ws281x` is the intended “truth” for reusable behavior.

### Decision 2: Treat the virtual strip buffer as the simulator’s ground truth

The renderer should not “recompute” colors. It should only *interpret* a buffer that is produced by the pattern engine.

This reduces the simulator to two testable layers:

- “Does pattern engine produce expected per-LED RGB?”
- “Does renderer draw those RGB values faithfully?”

### Decision 3: Use M5GFX sprite-based rendering (M5Canvas) as baseline

Cardputer code in this repo repeatedly demonstrates the stable idiom:

- `m5gfx::M5GFX display; display.init(); display.setColorDepth(16);`
- `M5Canvas canvas(&display); canvas.setColorDepth(16); canvas.createSprite(w,h);`
- Draw into the sprite, then `canvas.pushSprite(0,0);`
- Optional `display.waitDMA();` after present (used in `0011` and `0030`) to avoid overlapping DMA transfers.

This should be the default for the LED simulator since it naturally supports full-frame redraw of 50 glyphs.

## Alternatives Considered

### Alternative: Render directly to `display` without a sprite

Pros: lower RAM usage; no sprite buffer allocation.

Cons: higher risk of flicker/tearing when drawing 50 primitives repeatedly; more complex redraw logic; harder to maintain.

Given existing repo patterns prefer `M5Canvas`, the sprite approach is a better baseline.

### Alternative: Implement a new pattern engine in “display space”

Pros: could tailor to the display and avoid an intermediate buffer.

Cons: semantic drift from the physical WS281x behavior; duplicated logic; harder to verify equivalence; loses benefits of already-shipped semantics.

This is rejected unless we find that `components/ws281x` cannot be built/used on the Cardputer target for some reason.

## Implementation Plan

### Phase 0: Freeze semantics (research deliverable)

- Confirm which implementation is canonical for patterns, brightness scaling, and time stepping.
- Capture a “semantic contract” for each pattern type (params, ranges, clamping, time dependence).

### Phase 1: Minimal simulator (standalone)

- Create a new Cardputer project that:
  - Brings up M5GFX display via autodetect.
  - Allocates an `M5Canvas` sprite.
  - Runs a frame loop at a fixed `frame_ms` and draws 50 LED glyphs.
  - Allows selecting `off/rainbow/chase/breathing/sparkle` with simple keyboard controls.

### Phase 2: Mirror the ESP32-C6 control plane

- Reuse `mled_pattern_config_t` as the interchange format.
- Implement a receiver on Cardputer that ingests pattern configs and updates the local pattern engine.
- (Optional) implement timebase sync if cues are scheduled for future execution (`execute_at_ms`).

### Phase 3: Polish

- Add on-screen debug overlays (FPS, active pattern summary, brightness, `now_ms`).
- Add a small “per-LED inspector” mode to view indices and raw RGB values for debugging mismatches.

## Open Questions

1. Should the Cardputer simulator:
   - Compute patterns locally from a config (best for development), or
   - Mirror the live ESP32-C6 node(s) (best for operational truth)?
2. Is gamma correction desired? If “looks like the strip” is a requirement, we may need optional gamma (the physical strip + human perception differs from a raw RGB565 LCD).
3. What is the desired visualization topology?
   - A literal 1×50 strip,
   - A wrapped strip (e.g. 2×25),
   - A grid (5×10), or
   - A “snake” layout with indices labeled?
4. Do we need to preserve WS281x color order (GRB vs RGB) in the simulator view, or should the simulator show “logical RGB” as users think about it?

---

## Codebase Map (Deep Research)

This section answers two practical questions:

1. Where is the *pattern math* (rainbow/chase/etc) implemented?
2. Where is the *Cardputer graphics stack* implemented (and which idioms should we reuse)?

### Pattern computation: `components/ws281x`

**Primary files**

- `components/ws281x/include/led_patterns.h`
  - Defines `led_pattern_cfg_t` (type + global brightness + per-pattern params).
  - Defines `led_patterns_t` (cfg + state, including chase position and sparkle arrays).
- `components/ws281x/src/led_patterns.c`
  - Implements:
    - `led_patterns_init`, `led_patterns_set_cfg`
    - `led_patterns_render_to_ws281x(...)` which dispatches to:
      - `render_rainbow`, `render_chase`, `render_breathing`, `render_sparkle`, `render_off`.

**Support files**

- `components/ws281x/include/led_ws281x.h` + `components/ws281x/src/led_ws281x.c`
  - Defines the strip buffer (`pixels`), the color ordering, and per-pixel brightness scaling.
- `components/ws281x/include/led_task.h` + `components/ws281x/src/led_task.c`
  - Defines a queue-driven task that calls the pattern engine each frame and updates a status snapshot.

#### Key file pointers (line anchors)

These are “jump points” for future implementation and debugging:

- `components/ws281x/src/led_patterns.c:120` — `led_patterns_init` (defaults + sparkle array allocation)
- `components/ws281x/src/led_patterns.c:171` — `led_patterns_set_cfg` (global brightness floor + state reset)
- `components/ws281x/src/led_patterns.c:189` — `render_rainbow` (absolute-time hue)
- `components/ws281x/src/led_patterns.c:224` — `chase_update_pos` (dt-driven head position; Q16.16)
- `components/ws281x/src/led_patterns.c:351` — `render_breathing` (absolute-time phase + curves)
- `components/ws281x/src/led_patterns.c:378` — `render_sparkle` (dt-driven fade + spawn accumulator)
- `components/ws281x/src/led_patterns.c:480` — `led_patterns_render_to_ws281x` (dispatcher + frame++ invariant)
- `components/ws281x/src/led_ws281x.c:100` — `led_ws281x_set_pixel_rgb` (brightness scaling + GRB/RGB wire order)
- `components/ws281x/src/led_task.c:161` — `led_task_main` (frame loop timebase + `led_patterns_render_to_ws281x`)

> **Callout (Fundamental): what exactly does the pattern engine output?**
>
> The pattern engine writes via `led_ws281x_set_pixel_rgb(strip, idx, rgb, brightness_pct)`.
> In the current implementation (`components/ws281x/src/led_ws281x.c`), that function:
> 1) scales each channel by `brightness_pct` (0..100),
> 2) writes the scaled bytes into `strip->pixels` in either GRB or RGB order (wire-order),
> 3) does *not* do gamma correction.
>
> For the simulator, a “virtual strip” can simply maintain an RGB array (logical order), but if we want byte-identical behavior with the WS281x sender we should treat the wire-order buffer as the ground truth and convert it back to RGB for display.

### Pattern computation: tutorial-local copies (non-canonical but informative)

The following projects contain local versions of the same pattern engine. These are useful for historical context, but should not be treated as the reusable base unless `components/ws281x` is unavailable:

- `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c`
- `0046-xiao-esp32c6-led-patterns-webui/main/led_patterns.c`

### ESP32-C6 “control plane”: MLED/1 protocol and mapping

The repo contains a compact, fixed-size pattern config representation used on the wire:

- `components/mled_node/include/mled_protocol.h`
  - `mled_pattern_config_t` is 20 bytes: `pattern_type`, `brightness_pct`, `flags`, `seed`, `data[12]`.
  - Enumerates wire pattern types `MLED_PATTERN_*` aligned with the pattern engine’s set.

There is an explicit adapter from wire pattern config to `led_pattern_cfg_t`:

- `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c`
  - `map_pattern_cfg(out, mled_pattern_config_t*)` translates wire `data[]` into per-pattern structs.
  - `mled_effect_led_apply_pattern()` sends `LED_MSG_SET_PATTERN_CFG` into `led_task`.

Key pointers:

- `components/mled_node/include/mled_protocol.h:67` — `mled_pattern_config_t` (wire layout)
- `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c:31` — `map_pattern_cfg` (wire → engine mapping)

This means the Cardputer simulator can plausibly “speak the same language” as the ESP32-C6 node: if it can receive `mled_pattern_config_t`, it can map it and reuse the same engine.

#### MLED/1 timing and “cue” model (why `execute_at_ms` matters)

MLED/1 is not just a “set pattern now” protocol; it also supports scheduled execution.

- The message header (`mled_header_t`) includes `epoch_id` and `execute_at_ms` (see `components/mled_node/include/mled_protocol.h:52` and pack/unpack in `components/mled_node/src/mled_protocol.c:35`).
- The node implementation maintains a notion of **show time** (`show_ms()`), applying an offset to local time to track a controller-defined clock. (`components/mled_node/src/mled_node.c:136`).
- Pattern changes flow through a two-phase cue mechanism:
  - `CUE_PREPARE` carries the pattern config and stores it.
  - `CUE_FIRE` schedules “apply prepared pattern at `execute_at_ms`” (node stores this in an internal “fires” list and applies when due).

> **Callout (Fundamental): why a simulator can be “correct” but still not match a live node**
>
> If the Cardputer uses *local* time and applies changes immediately, but the ESP32-C6 node applies changes at a controller-scheduled `execute_at_ms` in show time, the two will drift even if the pattern math is identical. For faithful mirroring, the simulator must either (a) share the same show-time sync or (b) render the node’s reported current pattern as executed (not merely the last requested config).

### Cardputer display rendering: M5GFX idioms in this repo

**Display bring-up + sprite loop (minimal, proven)**

- `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
  - `display.init(); display.setColorDepth(16);`
  - `M5Canvas canvas(&display); canvas.createSprite(w,h);`
  - Draw by writing into `canvas.getBuffer()` (RGB565) then `canvas.pushSprite(0,0);`
  - Optional `display.waitDMA();` after present.

**Larger UI scaffolding (menus, overlays, perf tracking)**

- `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`
  - Multi-sprite layout: header/footer/body as separate canvases.
  - Includes a loop that conditionally redraws and presents dirty sprites.
  - Uses `display.waitDMA()` after present for safety.
- `0022-cardputer-m5gfx-demo-suite/main/demo_primitives.cpp`
  - Examples of the primitives we need for LEDs: `fillRect`, `drawRect`, `drawCircle`, `fillTriangle`, etc.

**Dirty-flag full-screen redraw pattern**

- `0030-cardputer-console-eventbus/main/app_main.cpp`
  - `M5Canvas screen;` draw only when dirty; present with `pushSprite`; wait DMA.

Key pointers:

- `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp:107` — `display.init()` via autodetect + `setColorDepth(16)`
- `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp:130` — `M5Canvas` sprite allocation + `pushSprite` + optional `waitDMA()`
- `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:157` — Cardputer display init + multi-sprite layout
- `0022-cardputer-m5gfx-demo-suite/main/demo_primitives.cpp:6` — basic draw primitives API usage

> **Callout (Fundamental): why do we see `waitDMA()` in demos?**
>
> When `pushSprite()` uses DMA, starting a new transfer before the previous finishes can cause visible “flutter” (tearing/flicker) or other timing pathologies. Waiting after each present is conservative and often correct for simple animation loops, especially when drawing full frames continuously.

---

## Pattern Semantics (What the Engine Actually Does)

This section describes behavior as implemented in `components/ws281x/src/led_patterns.c` and should be treated as the simulator’s semantic contract.

### Shared semantics across patterns

- `led_patterns_set_cfg()` forces `global_brightness_pct` to be at least 1 (a requested 0 becomes 1).
- `led_patterns_render_to_ws281x()` returns early if `p->led_count != strip->cfg.led_count` (count mismatch is a hard “no output”).
- The engine maintains state (`p->st.frame`, `p->st.last_step_ms`, and others) and increments `frame` after each render call.

### Rainbow

Parameters: `speed` (0..20 RPM), `saturation` (0..100), `spread_x10` (1..50, where 10 means “full 360° across strip”).

Key semantics:

- The rainbow is evaluated from absolute `now_ms`:
  - `hue_offset = (now_ms * speed_rpm * 360 / 60000) % 360`
  - For LED index `i`, `hue = hue_offset + i*(hue_range/led_count)`.
- Saturation is clamped to 100.
- Speed > 20 is capped at 20.
- Brightness is applied per LED via `led_ws281x_set_pixel_rgb(..., global_brightness_pct)`.

### Chase

Parameters: `speed` (LEDs/sec), `tail_len`, `gap_len`, `trains`, `fg`, `bg`, `dir`, `fade_tail`.

Key semantics:

- Chase uses *delta time* tracked by `p->st.last_step_ms`:
  - The head position is advanced by `delta_q16 = speed_lps * dt_ms * 65536 / 1000`.
  - Bounce mode clamps at endpoints and flips `p->st.chase_dir`.
  - Non-bounce wraps modulo `led_count` (in Q16.16).
- Each LED chooses the “best” distance to any train head; LEDs within `tail_len` use `fg`, otherwise `bg`.
- Optional tail fading scales `fg` by a distance-derived factor.

### Breathing

Parameters: `speed` (0..20 breaths/min), `color`, `min_bri`, `max_bri`, `curve`.

Key semantics:

- Uses absolute `now_ms` modulated by derived `period_ms = 60000/bpm`.
- Curve is evaluated in a 0..255 phase domain; three curves exist:
  - sine (table),
  - linear triangle,
  - ease-in/out (quadratic-ish).
- `min_bri/max_bri` are swapped if misordered; intensity interpolates between them; `color` is scaled by intensity.

### Sparkle

Parameters: `speed` (0..20), `color`, `density_pct` (0..100), `fade_speed` (>=1), `color_mode`, `background`.

Key semantics:

- Maintains `sparkle_bri[led_count]`, a per-LED brightness that decays over time.
- `fade_amount` scales with `dt_ms` relative to a baseline 25ms step.
- Spawning is time-based with an accumulator:
  - speed 0..20 maps to 0..4 sparkles/sec (in tenths).
  - Caps concurrent sparkles at `density_pct%` of LEDs.
  - Chooses random indices using an LCG RNG state (`p->st.rng`).
- `color_mode` affects per-LED sparkle color selection:
  - fixed: `cfg->color`
  - random: computes a hue dependent on `(i, frame)`
  - rainbow: computes hue based on index `i`

> **Callout (Fundamental): “random” in sparkle is deterministic given state**
>
> The sparkle pattern uses an explicit RNG (`p->st.rng`) updated by an LCG. If we want repeatable sequences across devices (C6 vs Cardputer), we must ensure the seed/state initialization and frame stepping are aligned (or explicitly accept divergence and show “qualitatively similar” sparkles).

---

## Rendering the 50 LEDs on Cardputer (Practical GFX Notes)

### The simplest renderer: draw 50 rectangles (or circles) each frame

Recommended baseline:

- Choose a layout (e.g. `5×10` grid).
- For each LED index `i`, compute `(x,y)` cell position.
- Convert RGB8 → RGB565 and draw a filled rect (with a border).

Pseudo-render loop:

```text
init():
  display.init(); display.setColorDepth(16)
  canvas.createSprite(w, h)
  patterns.init(led_count=50)
  cfg = default pattern config

loop(frame_ms):
  now_ms = esp_timer_get_time() / 1000
  led_patterns_render_to_ws281x(patterns, now_ms, virtual_strip)

  canvas.fillScreen(TFT_BLACK)
  for i in 0..49:
    rgb = get_led_rgb_from_virtual_strip(i)
    color565 = pack_rgb565(rgb)
    (x, y) = layout(i)
    canvas.fillRoundRect(x, y, cell_w, cell_h, r, color565)
    canvas.drawRoundRect(x, y, cell_w, cell_h, r, TFT_DARKGREY)

  canvas.pushSprite(0, 0)
  display.waitDMA()
  vTaskDelay(frame_ms)
```

### Color conversion guidance (RGB8 → RGB565)

M5GFX uses RGB565 for many fast paths; converting 8-bit channels to 565 is typical:

```text
rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
```

This conversion is already used in `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp` (see its `rgb565()` helper).

### Choosing a topology (index mapping)

For debugging, it helps if the layout makes the index ordering obvious. A good default is a “snake” grid:

- Row 0: indices 0..9 left→right
- Row 1: indices 19..10 right→left
- Row 2: indices 20..29 left→right
- ...

This preserves adjacency visually while using a compact 5×10 or 10×5 grid.

---

## References (Repo-local)

- Pattern engine: `components/ws281x/include/led_patterns.h`, `components/ws281x/src/led_patterns.c`
- Strip buffer + brightness scaling: `components/ws281x/include/led_ws281x.h`, `components/ws281x/src/led_ws281x.c`
- Pattern task loop: `components/ws281x/include/led_task.h`, `components/ws281x/src/led_task.c`
- MLED/1 protocol wire format: `components/mled_node/include/mled_protocol.h`
- Wire→engine mapping: `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c`
- Cardputer M5GFX bring-up + sprite loop: `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
- Cardputer UI scaffolding + primitives: `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`, `0022-cardputer-m5gfx-demo-suite/main/demo_primitives.cpp`

## References

<!-- Link to related documents, RFCs, or external resources -->
