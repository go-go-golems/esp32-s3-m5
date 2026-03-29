---
Title: Cardputer ADV dynamic animation UI analysis, design, and implementation guide
Ticket: ESP-47-CARDPUTER-ADV-ANIMATION-UI
Status: active
Topics:
    - cardputer-adv
    - cardputer
    - ui
    - animation
    - keyboard
    - display
    - m5gfx
    - esp-idf
    - firmware
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Sprite composition and navigation integration reference
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp
      Note: Tail-relative scrollback rendering reference
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp
      Note: Selection and scroll visibility logic reference
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Event-driven scrollback handling reference
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.cpp
      Note: Optional background engine pattern and snapshot publishing reference
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp
      Note: Primary single-task UI loop and canvas rendering reference
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp
      Note: Primary ADV keyboard implementation to reuse
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp
      Note: Mode-machine and shortcut handling reference
    - Path: ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/imports/retro_macos_line_minimap.html
      Note: Browser donor showing scrollPos
ExternalSources: []
Summary: Detailed intern-focused guide for building a new Cardputer ADV animation UI with keyboard-controlled scroll motion, using the existing 0066/0022/0030 firmware patterns as the implementation base.
LastUpdated: 2026-03-29T16:05:00-04:00
WhatFor: Use this document to understand the current Cardputer ADV UI building blocks and to implement a new dynamic scroll-animation firmware without re-deriving the runtime architecture.
WhenToUse: Read this before starting a new Cardputer ADV UI firmware, before refactoring 0066 patterns, or before porting the imported donor HTML interaction model to embedded C++.
---


# Cardputer ADV dynamic animation UI analysis, design, and implementation guide

## Executive Summary

The shortest reliable path to a modern dynamic animation UI on Cardputer ADV is to build a new standalone firmware that combines three existing patterns that are already proven in this repository:

1. Use the `0066-cardputer-adv-ledchain-gfx-sim` keyboard path for Cardputer ADV compatibility, because it already uses `cardputer_kb::UnifiedScanner` and explicitly documents that it auto-detects Cardputer ADV's TCA8418 backend versus the original Cardputer GPIO matrix backend (`0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:44-52`, `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:125-138`).
2. Use the `0022-cardputer-m5gfx-demo-suite` display composition model for sprite-backed rendering, menu/list navigation, dirty redraws, and explicit `waitDMA()` after presenting (`0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:161-199`, `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:404-506`, `0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp:33-123`).
3. Use the `0030-cardputer-console-eventbus` notion of scrollback as mutable UI state and keep display ownership in one task, even if events originate elsewhere (`0030-cardputer-console-eventbus/main/app_main.cpp:1-10`, `0030-cardputer-console-eventbus/main/app_main.cpp:172-217`, `0030-cardputer-console-eventbus/main/app_main.cpp:402-426`, `0030-cardputer-console-eventbus/main/app_main.cpp:761-813`).

The imported donor HTML contributes the interaction idea, not the runtime architecture. Its key concept is a scalar viewport state composed of `scrollPos`, `scrollTarget`, and a render function that derives both the minimap and scrollbar thumb from that same state (`imports/retro_macos_line_minimap.html:380-465`). That concept ports cleanly to embedded firmware. The browser-specific parts, such as DOM mutation and `requestAnimationFrame`, do not.

The recommended implementation for a first version is intentionally simple:

- one input task that converts physical keys into semantic UI events
- one UI task that owns `M5GFX`, the animation model, and all drawing
- one scalar animated scroll position (`scroll_pos_px`) and one target (`scroll_target_px`)
- a sprite-backed full-frame redraw at a configurable frame interval

Do not start with a complex multi-task renderer, two-dimensional camera, or web stack. Those are optional later layers.

## Problem Statement And Scope

The requested outcome is not just "draw something animated on screen." The real goal is to create a Cardputer ADV UI that feels intentional and modern: keyboard input should trigger motion, that motion should move the scroll position smoothly, and the UI should expose that movement in a compact, stylized overview component similar to the imported retro Mac minimap prototype.

That implies several engineering requirements:

- The UI must run on Cardputer ADV hardware, not just on the original Cardputer.
- Keyboard control must feel predictable and low-latency.
- Scroll movement must be animated rather than instant, because the animation itself is part of the product.
- The architecture must remain understandable to a new intern who has not worked with `M5GFX`, `cardputer_kb`, or ESP-IDF task ownership before.
- The display path must avoid DMA overlap or ambiguous task ownership issues.

This document stays within that scope. It explains how to build the embedded UI and animation architecture. It does not attempt to define final visual art direction for every screen, and it does not attempt to fully specify a networked or JavaScript-driven application shell.

## Terms And Mental Model

Before looking at the code, it helps to define the moving pieces in plain language.

- **Viewport**: the current portion of a larger virtual document or scene that is visible on screen.
- **Scroll position**: a scalar coordinate that represents where the viewport is centered or anchored in that larger document.
- **Scroll target**: where the UI wants the viewport to end up after animation completes.
- **Animation tick**: one frame-step where the code nudges the current scroll position toward the target.
- **Semantic key event**: a normalized event such as `Left`, `Right`, `Enter`, `Back`, or text input, rather than a raw electrical key scan.
- **Display owner**: the one task that is allowed to touch `M5GFX` drawing APIs. This is an important invariant in this repository.

For the embedded port, think in terms of a small state machine rather than a browser page:

```text
keyboard scan task
    -> semantic key event queue
        -> UI task updates model
            -> UI task advances animation state
                -> UI task redraws canvas
                    -> UI task presents sprite to display
```

## Current-State Architecture: Evidence From Existing Firmwares

### 1. What `0066` already proves for Cardputer ADV

`0066-cardputer-adv-ledchain-gfx-sim` is the most valuable reference because it is already Cardputer ADV aware.

At boot, `app_main()` initializes a background engine, starts the UI task, then optionally starts Wi-Fi and console infrastructure (`0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp:43-69`). That means the project already uses a separation between background state generation and foreground UI.

The UI entrypoint is `sim_ui_start()`, which creates a dedicated UI task with an 8 KB stack because `M5GFX` init is stack hungry (`0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:143-146`). Inside that task:

- `m5gfx::M5GFX display; display.init();` brings up the display (`0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:44-48`)
- the code queries `display.width()` and `display.height()` at runtime instead of hardcoding geometry (`0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:49-52`)
- it creates a sprite-backed `M5Canvas`, disables PSRAM by default for better DMA behavior, and can optionally `waitDMA()` after present (`0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:53-64`, `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:134-137`, `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:17-27`)
- it consumes queued keyboard events and applies them before drawing overlay text (`0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:125-133`)

The most important lesson here is architectural, not cosmetic: the task that consumes input events is the same task that updates UI state and draws. That is exactly the correct ownership model for the new animation UI.

### 2. What `0066` already proves for Cardputer ADV keyboard input

`0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp` is effectively the best existing abstraction boundary for the requested feature.

Important facts:

- The keyboard implementation uses `cardputer_kb::UnifiedScanner`, which initializes once and then auto-detects the actual backend (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:118-138`).
- The Kconfig help text confirms the intended runtime behavior: the same firmware can run on Cardputer ADV or the original Cardputer without swapping keyboard drivers (`0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:44-52`).
- The code emits **edge events only**, which is important because it gives the UI a clean signal for "start an animation now" rather than flooding the UI with repeated state every scan tick (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:187-297`).
- It converts physical key chords into semantic events like `UI_KEY_LEFT`, `UI_KEY_RIGHT`, `UI_KEY_BACK`, `UI_KEY_ENTER`, and `UI_KEY_TEXT` (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.h:14-40`, `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:203-296`).

That semantic event boundary is exactly what the new firmware should reuse. The UI should not care whether ADV produced the event via TCA8418 or the original Cardputer produced it via GPIO matrix scanning.

### 3. What `0066` already proves about UI state machines

`ui_overlay.cpp` in `0066` shows how to structure keyboard-driven modes in a way that remains readable. It has:

- an explicit `ui_mode_t` enum (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.h:10-20`)
- a compact state object that keeps mode indexes, selection indexes, JS output, and color editor state in one place (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.h:33-53`)
- one `ui_overlay_handle()` function that interprets input events based on mode (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp:646-760`)
- one `ui_overlay_draw()` function that renders the visual state (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp:865-1196`, discovered by symbol scan)

This is useful because the requested UI is not just a render loop. It is a small interactive system with modes such as:

- live browse
- autoplay / demo mode
- direct jump / selection mode
- help / shortcut overlay

The new firmware should reuse this state-machine style, even if its visual design is very different from `0066`.

### 4. What `0022` already proves about scene composition and dirty redraws

`0022-cardputer-m5gfx-demo-suite` is the strongest general Cardputer UI reference.

It initializes one `M5GFX` display and three sprites: `header`, `footer`, and `body` (`0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:161-194`). The main loop tracks `body_dirty`, `header_dirty`, and `footer_dirty`, only re-renders sections that changed, then pushes the corresponding sprite and calls `display.waitDMA()` after any present (`0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:234-246`, `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:404-506`).

That is not mandatory for the new firmware, but it gives two good options:

1. **Simple MVP**: one full-screen sprite, redraw every frame.
2. **Refined version**: split the screen into top HUD, center content, and bottom status bar the same way `0022` does.

For an intern, start with option 1 and only add partial redraws when there is a concrete performance reason.

### 5. What `0022` already proves about keyboard-to-navigation mapping

`0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` shows a smaller, app-level keyboard adapter around `UnifiedScanner` (`0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp:73-155`). It does two important things:

- converts captured bindings into semantic keys like `up`, `down`, `left`, `right`, `enter`, `esc`, `tab`, and `space` (`0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp:56-68`, `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp:102-118`)
- emits text edges for ordinary keys (`0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp:122-150`)

Then `nav_key_from_event()` maps multiple physical patterns into generic navigation, including Vim-like text keys and page-style movement (`0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:103-134`).

This is directly useful for the requested UI because a small keyboard has limited dedicated navigation keys. The existing repo already solved that problem. Reuse it.

### 6. What `0022` and `0030` already prove about scroll behavior

The repo already has two scroll implementations worth copying:

- `0022` terminal view uses a `scrollback` integer and computes the first visible line from the tail of the buffer (`0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp:58-95`).
- `0030` event bus UI also keeps `scrollback` in state, updates it in response to navigation actions, and derives the rendered line window from that value (`0030-cardputer-console-eventbus/main/app_main.cpp:172-217`, `0030-cardputer-console-eventbus/main/app_main.cpp:229-282`, `0030-cardputer-console-eventbus/main/app_main.cpp:402-426`).

Those are not animated yet, but they prove the simpler underlying contract:

- scrolling is state, not immediate rendering
- key events mutate that state
- the render pass only reflects current state

The new UI simply extends that contract from integer jumps to animated interpolation.

## The Imported Donor Prototype: What To Copy And What To Leave Behind

The imported donor `imports/retro_macos_line_minimap.html` is valuable because it expresses the interaction in a small, readable form.

### What the donor actually is

Its structure is:

- a title bar and status area
- a minimap composed of repeated bars (`imports/retro_macos_line_minimap.html:315-321`)
- a scrollbar with left and right buttons plus a movable thumb (`imports/retro_macos_line_minimap.html:325-336`)
- numeric status fields for line count, position, and active line (`imports/retro_macos_line_minimap.html:340-348`)

The script defines a single motion model:

- `scrollPos` is the animated current position (`imports/retro_macos_line_minimap.html:381`)
- `scrollTarget` is the desired destination (`imports/retro_macos_line_minimap.html:455`)
- `animate()` eases the current position toward the target (`imports/retro_macos_line_minimap.html:458-465`)

Both the indicator and scrollbar thumb are derived from the same normalized percentage of `scrollPos` (`imports/retro_macos_line_minimap.html:438-453`). That is the central idea to preserve.

### What should be copied into the embedded design

- The idea of one canonical scroll state and one scroll target.
- The idea that both overview and detailed content should derive from the same normalized position.
- The idea of proximity-based visual emphasis around the current scroll head (`imports/retro_macos_line_minimap.html:396-435`).
- The idea of discrete step-based motion for key taps, plus optional autoplay (`imports/retro_macos_line_minimap.html:511-527`).

### What should not be copied directly

- DOM nodes and CSS transitions.
- browser hover behavior as the primary interaction.
- `requestAnimationFrame`.
- raw mouse drag assumptions.

The Cardputer ADV equivalent is a fixed-rate UI tick and keyboard-driven target changes.

## Gap Analysis

The existing firmware pieces already cover most of the engineering substrate, but not the actual requested product.

### What is already solved

- ADV-compatible keyboard scanning and backend detection
- semantic key event translation
- `M5GFX` display bring-up via autodetect
- sprite-backed rendering
- modal UI state machines
- integer-based scrollback and list navigation
- configurable frame intervals and safe `waitDMA()` present behavior

### What is not yet solved

- a generic animated scroll model with `current` and `target` state
- a reusable minimap widget that visually responds to scroll position
- a document or virtual scene abstraction larger than the screen
- an intern-oriented explanation of which firmware patterns to combine and which ones to avoid

### Core design conclusion

You do not need a brand-new low-level stack. You need a new application-specific model layer sitting between the proven input/render plumbing and a new visual composition.

## Proposed Architecture

### High-level design

Create a new firmware project, for example:

```text
0082-cardputer-adv-animation-ui/
```

Reuse the shape of `0066`, but simplify it:

- keep the ADV-capable keyboard task
- keep one UI owner task
- do not pull in Wi-Fi, HTTP, or JS for the MVP
- keep the animation model entirely inside the UI task

This yields a cleaner starting point than modifying `0066` directly, because the new app is about viewport motion and minimap presentation, not LED simulation.

### Runtime ownership

```text
+-------------------+        queue         +------------------------------+
| keyboard task     | ------------------> | UI task                      |
| UnifiedScanner    |                     | owns M5GFX + animation model |
| edge events only  |                     | updates state, draws, waits  |
+-------------------+                     +------------------------------+
```

That ownership model matters because this repo already shows the costs of mixing responsibilities. `0030` explicitly states that the event loop is dispatched by the UI task so display ownership stays in one place (`0030-cardputer-console-eventbus/main/app_main.cpp:8-10`).

### Recommended modules

For the new firmware, use modules like these:

```text
main/
  app_main.cpp
  ui_app.h/.cpp
  ui_input.h/.cpp
  ui_model.h/.cpp
  ui_render.h/.cpp
  ui_theme.h
  ui_layout.h
```

Responsibilities:

- `app_main.cpp`
  - boot log
  - display bring-up
  - start input task
  - start UI task
- `ui_input.*`
  - mostly adapted from `0066/main/ui_kb.*`
  - convert physical scan results into semantic events
- `ui_model.*`
  - own viewport position, target, easing, mode, selected item, autoplay state
- `ui_render.*`
  - draw minimap, scrollbar, content, status, and help overlays onto `M5Canvas`
- `ui_app.*`
  - top-level loop: drain queue, mutate model, tick animation, render, present

### Recommended model structs

The donor HTML uses only `scrollPos` and `scrollTarget`, but the embedded version should be slightly more explicit.

```cpp
enum class UiMode {
    Browse,
    Help,
    Autoplay,
};

struct ScrollModel {
    float pos_px = 0.0f;
    float target_px = 0.0f;
    float min_px = 0.0f;
    float max_px = 0.0f;
    float snap_epsilon_px = 0.25f;
    float easing = 0.18f;
    bool animating = false;
};

struct MinimapModel {
    int line_count = 40;
    int active_index = -1;
    int hovered_index = -1;     // optional if later mapped from a pointer or touch source
    int selected_index = 0;
};

struct ContentModel {
    int virtual_width_px = 0;
    int viewport_width_px = 0;
    int selected_line = 0;
};

struct UiState {
    UiMode mode = UiMode::Browse;
    bool autoplay = false;
    uint32_t autoplay_step_ms = 60;
    int64_t last_tick_us = 0;
    ScrollModel scroll;
    MinimapModel minimap;
    ContentModel content;
    bool dirty = true;
};
```

### Why this model is better than using integer scrollback directly

`scrollback` is enough when the UI instantly jumps between rows. It is not enough when the UI needs momentum-like motion or easing.

You need both:

- a precise integer or item-based logical selection for commands
- a floating-point or fixed-point visual position for motion

That split keeps behavior deterministic while letting the motion feel smooth.

## API References From Existing Code

These are the APIs and patterns worth treating as the current local standard.

### Display loop APIs

- `m5gfx::M5GFX display; display.init();`
  - proven in `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:44-48`
  - proven in `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:161-168`
- `M5Canvas canvas(&display); canvas.createSprite(w, h);`
  - proven in `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:53-64`
- `canvas.pushSprite(0, 0); display.waitDMA();`
  - proven in `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:134-137`
  - refined in `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:494-506`

### Keyboard APIs

- `cardputer_kb::UnifiedScanner`
  - proven in `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:118-138`
- `cardputer_kb::ScanSnapshot`
  - used in `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:151-159`
- `cardputer_kb::xy_from_keynum()`
  - used in `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:101-110`
  - used in `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp:32-40`
- semantic event enum `ui_key_kind_t`
  - defined in `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.h:14-40`

### Config and timing patterns

- frame time config in Kconfig: `CONFIG_TUTORIAL_0066_FRAME_MS`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:10-15`
- safe DMA defaults
  - `0066-cardputer-adv-ledchain-gfx-sim/sdkconfig.defaults:31-35`
- USB Serial/JTAG console preference
  - `0066-cardputer-adv-ledchain-gfx-sim/sdkconfig.defaults:22-26`

## Detailed Design Decisions

### Decision 1: Keep display ownership in one task

This is non-negotiable for the first version.

Reasoning:

- `0030` explicitly documents that its event loop is UI-dispatched to keep display ownership in one task (`0030-cardputer-console-eventbus/main/app_main.cpp:8-10`).
- `0066` already processes keyboard events inside the same task that draws (`0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:125-133`).
- That design removes a class of race conditions and avoids needing cross-task rendering locks.

Consequence:

- input task emits semantic events only
- UI task mutates the model and draws
- background worker tasks are optional and only needed if future content generation becomes expensive

### Decision 2: Reuse `0066` keyboard code almost verbatim

Reasoning:

- It already handles Cardputer ADV detection.
- It already emits semantic arrow/back/tab/text events.
- It already filters to key press edges, which is ideal for starting animations cleanly.

Consequence:

- avoid writing a new raw keyboard driver
- adapt naming if needed, but keep backend detection and key-number mapping

### Decision 3: Start with a single-axis scroll model

Reasoning:

- The donor prototype is fundamentally one-dimensional.
- The smallest shippable experience is a horizontal minimap and one animated viewport axis.
- A two-axis camera complicates layout, content model, and key mapping immediately.

Consequence:

- implement `scroll_pos_px` and `scroll_target_px`
- map left/right navigation to motion first
- add up/down or secondary modes only after the one-axis interaction feels correct

### Decision 4: Prefer a deterministic fixed-step UI tick over variable `requestAnimationFrame` semantics

Reasoning:

- Browser rendering uses `requestAnimationFrame`.
- Embedded firmware already has explicit frame periods in Kconfig (`0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:10-15`).
- Fixed-step loops are easier to profile and reason about on-device.

Consequence:

- define a target frame interval, for example `16 ms` or `20 ms`
- update animation state once per loop
- optionally clamp elapsed time if frame scheduling jitters

### Decision 5: Redraw the full screen first, optimize later

Reasoning:

- The screen is small enough that a full-screen sprite redraw is acceptable for a first version.
- Full redraw is easier for an intern to debug.
- The repo already shows a safe full-screen canvas pattern in `0066` (`0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:53-64`, `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:104-139`).

Consequence:

- first implementation uses one `M5Canvas`
- later versions may split HUD/body/footer if profiling shows benefit

## Mapping The Donor HTML To Embedded Firmware

The cleanest way to understand the port is to line up each donor concept with its embedded equivalent.

| Donor HTML concept | Embedded equivalent | Notes |
| --- | --- | --- |
| `scrollPos` | `state.scroll.pos_px` | current visual position |
| `scrollTarget` | `state.scroll.target_px` | desired destination after easing |
| `animate()` | `tick_animation(state, now_us)` | fixed-step UI update |
| `updateBars()` | `render_minimap(canvas, state)` | draw derived overview bars |
| `updateIndicator()` | `render_scrollbar(canvas, state)` | derive thumb/head from normalized scroll |
| click left/right buttons | semantic key events | `Left` and `Right` actions from keyboard |
| mouse drag | optional future feature | not needed for keyboard-first MVP |
| `setInterval()` autoplay | `autoplay_tick_if_due()` | optional attract mode |

### Embedded translation of the donor core

Browser version:

```js
scrollPos += (scrollTarget - scrollPos) * 0.18;
if (Math.abs(scrollPos - scrollTarget) < 0.1) scrollPos = scrollTarget;
```

Recommended embedded version:

```cpp
static void tick_scroll(ScrollModel& s) {
    const float delta = s.target_px - s.pos_px;
    s.pos_px += delta * s.easing;
    if (fabsf(delta) < s.snap_epsilon_px) {
        s.pos_px = s.target_px;
        s.animating = false;
    } else {
        s.animating = true;
    }
}
```

This is intentionally almost identical because the donor already chose a good minimal motion model.

## Rendering Design

### Recommended screen layout

Do not hardcode Cardputer ADV dimensions in logic. Query them from `display.width()` and `display.height()` as `0066` and `0022` already do.

A good first layout is:

```text
+------------------------------------------------------+
| title / mode / status                                |
+------------------------------------------------------+
| minimap strip                                        |
| scrollbar strip                                      |
+------------------------------------------------------+
| animated content viewport                            |
| virtual document, line labels, viewport head, etc.   |
+------------------------------------------------------+
| shortcuts / mode hint                                |
+------------------------------------------------------+
```

### Visual style recommendation

The donor has a retro monochrome Mac feel. The user asked for a modern dynamic animation UI, not a literal browser clone, so the recommended direction is:

- preserve the minimap / thumb / indicator composition
- preserve high-contrast geometry
- use embedded-friendly solid fills and 1 px lines instead of CSS effects
- add motion through animated offsets and emphasis rather than trying to imitate browser shadows exactly

### Minimap rendering algorithm

The donor bars are driven by:

- base height
- emphasis from proximity to the current scroll head
- optional secondary emphasis from hover

For embedded MVP, remove hover and keep the first two.

Pseudocode:

```cpp
for each bar i:
    center = i * line_step + line_width / 2
    target_h = base_h(i)
    dist = abs(scroll.pos_px - center)
    if dist < distance_limit:
        norm = 1 - dist / distance_limit
        boost = norm * norm * 18
        target_h += boost
    height = round(target_h)
    draw_bar(i, height, is_active(i), is_close_to_head(dist))
```

This is effectively the same idea as the donor (`imports/retro_macos_line_minimap.html:396-435`), but implemented as direct drawing.

### Scrollbar rendering algorithm

The indicator and thumb should both be derived from normalized position:

```cpp
float pct = (scroll.max_px <= 0) ? 0.0f : scroll.pos_px / scroll.max_px;
pct = clamp01(pct);
indicator_x = round(pct * (indicator_track_w - indicator_w));
thumb_x = round(pct * (scrollbar_track_w - thumb_w));
```

That mirrors the donor exactly (`imports/retro_macos_line_minimap.html:438-453`) and keeps every visual "where am I?" signal consistent.

## Input Design

### Physical-to-semantic mapping

Use `0066` key mapping as the base:

- `Fn + ,` -> `UI_KEY_LEFT`
- `Fn + /` -> `UI_KEY_RIGHT`
- `Fn + ;` -> `UI_KEY_UP`
- `Fn + .` -> `UI_KEY_DOWN`
- `Fn + \`` -> `UI_KEY_BACK`

Those mappings already exist in `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:203-247`.

For the animation UI:

- `Left` should move the target scroll position left by a small step
- `Right` should move it right by a small step
- `Up` and `Down` can either change mode or adjust a selected line index
- `Enter` should start or confirm a mode-specific action
- `Tab` should toggle a menu or help overlay, following the `0066` pattern (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp:650-668`)

### Step sizes

Borrow the idea from `step_for_mods()` in `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp:43-49`.

Recommended policy:

- plain left/right: small move
- `Fn` modifier: micro-adjust
- `Alt` modifier: medium jump
- `Ctrl` modifier: large jump / page jump

That gives the UI a compact but expressive control scheme without needing more buttons.

Example:

```cpp
int scroll_step_px(uint8_t mods) {
    if (mods & UI_MOD_CTRL) return 96;
    if (mods & UI_MOD_ALT)  return 48;
    if (mods & UI_MOD_FN)   return 8;
    return 24;
}
```

## Application Flow

### Main loop design

The UI loop should do five things in order:

1. drain all pending key events
2. mutate UI state and scroll targets
3. advance animation by one tick
4. redraw the canvas if needed
5. present and wait for DMA

Pseudocode:

```cpp
for (;;) {
    while (queue_receive(key_q, &ev)) {
        handle_event(state, ev);
    }

    tick_autoplay_if_due(state, now_us);
    tick_scroll(state.scroll);

    if (state.dirty || state.scroll.animating) {
        render_frame(canvas, state);
        canvas.pushSprite(0, 0);
        display.waitDMA();
        state.dirty = false;
    }

    vTaskDelay(pdMS_TO_TICKS(frame_ms));
}
```

### Event handling design

Keep event handlers tiny and deterministic.

Example:

```cpp
void handle_event(UiState& state, const ui_key_event_t& ev) {
    switch (state.mode) {
    case UiMode::Browse:
        if (ev.kind == UI_KEY_LEFT)  nudge_target(state, -scroll_step_px(ev.mods));
        if (ev.kind == UI_KEY_RIGHT) nudge_target(state, +scroll_step_px(ev.mods));
        if (ev.kind == UI_KEY_ENTER) state.autoplay = !state.autoplay;
        if (ev.kind == UI_KEY_TAB)   state.mode = UiMode::Help;
        break;
    case UiMode::Help:
        if (ev.kind == UI_KEY_TAB || ev.kind == UI_KEY_BACK) state.mode = UiMode::Browse;
        break;
    case UiMode::Autoplay:
        ...
    }
    state.dirty = true;
}
```

## Suggested File-Level Implementation Plan

### Phase 1: Scaffold the new firmware

Start from `0066` rather than from scratch.

Copy only the parts that directly support the new UI:

- display init and canvas bring-up pattern from `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:44-64`
- `ui_kb.h/.cpp` concept and ADV detection from `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.*`
- safe defaults from `0066-cardputer-adv-ledchain-gfx-sim/sdkconfig.defaults:19-35`

Do not copy:

- Wi-Fi manager
- HTTP server
- JS service
- LED simulation engine

Deliverable:

- new firmware boots
- display initializes
- keyboard task starts
- UI task draws a static skeleton screen

### Phase 2: Implement semantic animation state

Add:

- `UiState`
- `ScrollModel`
- `nudge_target()`
- `tick_scroll()`

Validation:

- pressing left/right updates the target
- the current position visibly eases toward the target

### Phase 3: Implement the minimap and scrollbar

Add:

- `render_minimap()`
- `render_scrollbar()`
- `render_status()`

Validation:

- thumb and indicator track the same normalized position
- emphasis around the scroll head is visibly centered on the current position

### Phase 4: Implement real content viewport motion

Add a virtual content area larger than the screen.

The easiest first content type is synthetic line content:

- numbered columns
- repeated labels
- highlighted active region
- maybe a simple waveform or timeline-like strip

Why synthetic first:

- it isolates viewport motion from application data concerns
- it makes debugging coordinate math easier

### Phase 5: Add mode transitions and autoplay

Borrow the mode-machine style from `0066`.

Add:

- browse mode
- help mode
- autoplay mode

Autoplay should simply nudge `scroll_target_px` at fixed intervals, echoing the donor's auto-demo (`imports/retro_macos_line_minimap.html:518-527`).

### Phase 6: Refine visuals and performance

Only after the interaction feels correct:

- split screen into header/body/footer if that improves readability
- add theme constants
- tune step sizes
- measure whether full redraw is still sufficient

## Diagrams

### Architecture diagram

```text
             +--------------------------------------+
             | app_main                             |
             | - init display config                |
             | - create event queue                 |
             | - start keyboard task                |
             | - start UI task                      |
             +-------------------+------------------+
                                 |
                                 v
                 +---------------+---------------+
                 | keyboard task                 |
                 | UnifiedScanner                |
                 | - detect ADV backend          |
                 | - scan pressed keys           |
                 | - emit semantic edge events   |
                 +---------------+---------------+
                                 |
                                 v
                 +---------------+---------------+
                 | UI task                        |
                 | - drain queue                  |
                 | - mutate target scroll         |
                 | - tick animation               |
                 | - render minimap/content       |
                 | - pushSprite + waitDMA         |
                 +-------------------------------+
```

### Animation state diagram

```text
Idle
  |
  | key event / autoplay step
  v
Target Updated
  |
  | tick_scroll()
  v
Animating
  |
  | abs(target - pos) <= epsilon
  v
Settled
  |
  +--> Idle
```

## Testing And Validation Strategy

### Unit-of-work validation

After each phase, validate one visible behavior.

1. Boot validation
   - display initializes
   - keyboard backend logs correctly
2. Input validation
   - left/right keys change target state
3. Animation validation
   - visible easing instead of immediate jumps
4. Rendering validation
   - minimap, indicator, and scrollbar stay in sync
5. Mode validation
   - help/menu/autoplay transitions are reversible

### On-device debug instrumentation

Follow the spirit of `0066`'s keyboard debug snapshot (`0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.h:49-75`) and `0030`'s on-screen state reporting (`0030-cardputer-console-eventbus/main/app_main.cpp:245-282`).

Recommended debug HUD fields:

- backend type
- current scroll position
- current scroll target
- last key event
- frame time / FPS
- current mode

### Performance checks

Use these questions:

- Does `waitDMA()` materially limit frame rate?
- Is full-screen redraw still smooth at the chosen frame interval?
- Does PSRAM disable DMA in a way that harms the experience?

`0066` already documents the default answer for the last question: keep PSRAM disabled for the canvas when you want the best display performance (`0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:17-27`).

## Risks, Alternatives, And Open Questions

### Main risks

- If the UI model becomes too mode-heavy, it can collapse into a hard-to-understand state machine.
- If content rendering gets expensive, a full redraw every tick may become too costly.
- If key repeat is handled naively, animations may constantly retarget and feel mushy instead of deliberate.

### Recommended mitigations

- keep the first version to 2-3 modes
- keep only one animated axis
- keep one place where target scroll is updated
- log semantic events early during bring-up

### Alternatives considered

#### Alternative A: Build on top of `0030`

Rejected because `0030` proves scrollback logic and event dispatch, but it is more focused on event-bus architecture and text logging than on reusable visual composition.

#### Alternative B: Modify `0066` in place

Rejected for the first version because the LED simulator, JS service, and Wi-Fi stack add noise to the mental model. It is better as a donor than as the final host.

#### Alternative C: Port the donor HTML literally

Rejected because browser DOM concepts do not map cleanly to `M5Canvas`. The interaction idea is portable; the implementation details are not.

### Open questions

- Should the first shipping UI represent a text document, a timeline, or an abstract visual strip?
- Should autoplay remain a visible product feature or only a demo mode?
- Does the final design want one-axis motion only, or should later versions introduce vertical browsing?

## Suggested Pseudocode For The New Firmware

```cpp
extern "C" void app_main(void) {
    init_logging();

    Display display;
    display.init();

    QueueHandle_t input_q = xQueueCreate(32, sizeof(ui_key_event_t));
    ui_input_start(input_q);
    ui_app_start(&display, input_q);
}
```

```cpp
void ui_app_task(void* arg) {
    UiState state = make_default_state(display.width(), display.height());
    M5Canvas canvas(&display);
    canvas.createSprite(display.width(), display.height());

    for (;;) {
        ui_key_event_t ev{};
        while (xQueueReceive(input_q, &ev, 0) == pdTRUE) {
            handle_event(state, ev);
        }

        tick_scroll(state.scroll);
        tick_autoplay(state);

        if (state.dirty || state.scroll.animating) {
            render_frame(canvas, state);
            canvas.pushSprite(0, 0);
            display.waitDMA();
            state.dirty = false;
        }

        vTaskDelay(pdMS_TO_TICKS(state.frame_ms));
    }
}
```

## Concrete Implementation Checklist For A New Intern

1. Copy the ADV-capable keyboard layer from `0066`.
2. Create a new project rather than editing `0066`.
3. Bring up a full-screen `M5Canvas`.
4. Draw a static version of the intended layout.
5. Add `scroll_pos_px` and `scroll_target_px`.
6. Map left/right keys to change only the target.
7. Add the easing tick.
8. Draw the minimap bars from the current scroll position.
9. Draw the scrollbar thumb from normalized position.
10. Add help/autoplay mode only after the basic movement feels correct.

## References

- `0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp:43-69`
- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp:40-146`
- `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.h:14-75`
- `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:121-349`
- `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.h:10-63`
- `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp:443-760`
- `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:10-27`
- `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild:44-52`
- `0066-cardputer-adv-ledchain-gfx-sim/sdkconfig.defaults:19-35`
- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.h:21-61`
- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.cpp:28-123`
- `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:103-134`
- `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:161-199`
- `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:404-506`
- `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp:56-155`
- `0022-cardputer-m5gfx-demo-suite/main/ui_list_view.h:8-57`
- `0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp:33-123`
- `0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp:58-95`
- `0030-cardputer-console-eventbus/main/app_main.cpp:1-10`
- `0030-cardputer-console-eventbus/main/app_main.cpp:172-217`
- `0030-cardputer-console-eventbus/main/app_main.cpp:229-282`
- `0030-cardputer-console-eventbus/main/app_main.cpp:402-426`
- `0030-cardputer-console-eventbus/main/app_main.cpp:761-813`
- `imports/retro_macos_line_minimap.html:315-527`
