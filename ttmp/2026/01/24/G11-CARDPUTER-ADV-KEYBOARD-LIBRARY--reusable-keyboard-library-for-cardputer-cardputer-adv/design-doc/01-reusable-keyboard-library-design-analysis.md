---
Title: 'Reusable Keyboard Library: Design Analysis'
Ticket: G11-CARDPUTER-ADV-KEYBOARD-LIBRARY
Status: active
Topics:
    - cardputer
    - keyboard
    - esp-idf
    - esp32s3
    - console
    - usb-serial-jtag
    - ui
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp
      Note: Example of queue-based delivery to an app UI loop
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/tca8418.c
      Note: Minimal register-level TCA8418 driver used by ADV keyboard demo
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp
      Note: Cardputer-ADV TCA8418 keyboard init + remap + modifier/text decoding
    - Path: components/cardputer_kb/include/cardputer_kb/bindings.h
      Note: Semantic chord binding decoder (decode_best)
    - Path: components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h
      Note: Example captured navigation bindings (Fn chords)
    - Path: components/cardputer_kb/include/cardputer_kb/scanner.h
      Note: Cardputer physical scanner public API and keyNum model
    - Path: components/cardputer_kb/matrix_scanner.cpp
      Note: Cardputer GPIO matrix scan implementation + picture-space mapping
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T15:05:00-05:00
WhatFor: ""
WhenToUse: ""
---


# Reusable Keyboard Library: Design Analysis

## Executive Summary

We want a single, reusable keyboard library that works on both:

- **Cardputer** (keyboard is a GPIO-scanned matrix; see `components/cardputer_kb`), and
- **Cardputer-ADV** (keyboard is driven by a TCA8418 keypad controller over I2C; see `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp` + `tca8418.*`).

The central design idea is to treat “keyboard input” as a *pipeline*:

1. **Acquire** physical key changes from hardware (board-specific).
2. **Normalize** them into a stable, board-agnostic representation (press/release events in a canonical coordinate system, with debouncing and repeat).
3. **Decode** them (optional) into higher-level meanings: text, keycodes, and semantic actions (navigation, back, etc.).
4. **Deliver** them (pluggably) to different application styles: polling loops, FreeRTOS queues, `esp_event`, LVGL `indev`, console line editors, and scripting runtimes (e.g., QuickJS).

This design explicitly separates what varies (hardware wiring and UX conventions) from what should be shared (state tracking, modifiers, repeat, and event delivery contracts).

## Problem Statement

Today, the repository demonstrates two distinct patterns:

- `components/cardputer_kb` is a reusable *physical* layer for Cardputer: it scans the matrix and returns pressed physical keys as `(x,y)` and `keyNum` (1..56).
- Tutorial `0066` contains an *application-specific* Cardputer-ADV keyboard handler: it initializes a TCA8418, remaps its internal key numbers into the same 4×14 “picture” coordinates, then immediately converts presses into a UI-specific event (`ui_key_event_t`) containing modifiers and text.

Both are correct for their contexts, but they do not compose cleanly:

- Apps that want **text input** must reinvent modifier/caps logic (or copy/paste from `0066`).
- Apps that want **semantic actions** (Up/Down/Back, etc.) must either invent their own chord scheme or hardcode one.
- Apps that want **raw physical keys** (games, hotkeys, diagnostics) must bypass higher-level decoders, but no common delivery API exists.
- Cardputer and Cardputer-ADV have different hardware acquisition models:
  - Cardputer yields *snapshots* (pressed set) when polled.
  - Cardputer-ADV yields *events* (press/release) via the TCA’s event queue + INT line, with polling as a fallback.

We want a library that is reusable because it has:

- a stable intermediate representation,
- explicit contracts at each layer, and
- “adapters” so applications can consume keyboard input without being coupled to a specific UI framework or transport.

## Proposed Solution

### 1) Choose a Canonical Physical Representation

The repository already contains the most important unifying choice: the **vendor “picture” coordinate system**.

- It is a **4×14** grid.
- It matches the printed keyboard layout and the `kKeyLegend` table in `components/cardputer_kb/include/cardputer_kb/layout.h`.
- It can be used to derive a vendor-style `keyNum`:

```text
keyNum(x,y) = y*14 + (x+1)     where x∈[0,13], y∈[0,3], keyNum∈[1,56]
```

Cardputer already uses this, and `0066` explicitly remaps TCA8418 events so the ADV keys also land in the same picture coordinates.

So the library should standardize on **picture-space** as its “physical truth”:

- `KeyPos{x,y}` (picture space),
- `keyNum` derived from it (1..56), and
- *event time* (for repeat/idle decisions).

This representation is not the electrical matrix wiring and not necessarily the TCA’s internal row/col; it is a *semantic coordinate system for humans and UIs*. The library must document this clearly so it does not leak electrical details upward.

### 2) Define the Core Event Types (Raw → Normalized → Decoded)

Think like a compiler: lexical tokens (raw) become syntax trees (normalized state) become meaning (decoded events).

#### Raw physical change (lowest layer)

```c
typedef enum {
  KB_EDGE_DOWN = 1,
  KB_EDGE_UP   = 2,
} kb_edge_t;

typedef struct {
  uint32_t time_ms;
  uint8_t  keynum;    // 1..56 (picture-space)
  uint8_t  x;         // 0..13
  uint8_t  y;         // 0..3
  kb_edge_t edge;     // press/release
} kb_raw_event_t;
```

This is what hardware drivers emit. It is intentionally minimal: no characters, no modifiers, no actions. It answers only: “which physical key changed state, and when?”

#### Normalized key state (shared)

All decoding benefits from a canonical state:

- Which keys are down right now?
- Which modifiers are latched (caps lock) vs held?
- What is the repeat timing?

So we maintain a `kb_state_t` that is updated by raw events and can produce “higher-level” outputs on demand.

#### Decoded events (optional)

Different applications want different products:

- **Text**: printable UTF-8 (e.g., `"p"`, `"?"`, `"~"`).
- **Keycodes**: stable non-text keys (Enter, Tab, Backspace, arrows, F-keys, etc.).
- **Semantic actions**: UX-defined meanings such as `NavUp` that may be implemented as a chord (e.g., Fn+`;` on Cardputer).

Rather than forcing one format, provide multiple decoders operating over the same normalized state:

```c
typedef enum {
  KB_EVT_RAW_EDGE,
  KB_EVT_KEYCODE,   // press/release of a keycode
  KB_EVT_TEXT,      // a text "commit" (UTF-8)
  KB_EVT_ACTION,    // semantic action (NavUp, Back, ...)
} kb_event_kind_t;
```

The core library can support all three, but applications should be able to subscribe at the layer they want (raw edges, keycodes, text, actions).

### 3) Factor the Hardware Layer: One Interface, Two Implementations

We want the same downstream code to run on both devices. That requires a stable hardware abstraction:

```c
typedef struct kb_hw kb_hw_t;

typedef esp_err_t (*kb_hw_init_fn)(kb_hw_t *hw);
typedef esp_err_t (*kb_hw_poll_fn)(kb_hw_t *hw, kb_raw_event_t *out, size_t out_cap, size_t *out_n);

struct kb_hw {
  kb_hw_init_fn init;
  kb_hw_poll_fn poll;   // drain available edges into out[]
  void *impl;           // board-specific state
};
```

Key point: **`poll()` always produces edge events**, even if the underlying hardware naturally produces snapshots.

That means:

- Cardputer implementation:
  - Use the existing `cardputer_kb::MatrixScanner::scan()` snapshot.
  - Diff against the previous snapshot to produce down/up edges.
- Cardputer-ADV implementation:
  - Use the existing `tca8418_*` helpers to drain `KEY_EVENT_A`.
  - Use the `decode_tca_event()` remap logic (or an equivalent table) to convert TCA events to picture-space `(x,y)` and `keyNum`.

This “edges everywhere” decision simplifies everything downstream:

- debouncing operates on a stream of edges,
- repeat operates on held keys in state,
- delivery can be push-based (events) or pull-based (state snapshots) as an adapter choice.

#### Worked example: converting a snapshot into edges

Cardputer’s scanner returns a set of pressed keys (snapshot). To unify with an event-driven device, we emit edges by comparing *previous* vs *current* snapshots.

Let `prev` and `cur` be boolean arrays indexed by `keyNum` (1..56):

```text
for kn in 1..56:
  if cur[kn] && !prev[kn]: emit DOWN(kn)
  if !cur[kn] && prev[kn]: emit UP(kn)
prev = cur
```

This is O(56) per poll, which is a rounding error on an ESP32-S3 and is deterministic (good for debugging).

### 4) Share the Decoder Layer: Keymap + Modifiers + Actions

The code in `0066` shows a compact, app-specific decoder:

- track modifier booleans (`shift`, `ctrl`, `alt`, `opt`, `caps`),
- decide between base vs shifted glyph for each key,
- emit special keys as distinct kinds (Tab/Enter/Del/Space),
- send events into a queue.

That logic is broadly reusable, but should be parameterized rather than hardcoded:

#### Keymap

Create a `kb_keymap_t` table indexed by `keyNum` (1..56). Each entry describes the *outputs* that a key can generate:

```c
typedef enum {
  KB_ROLE_PRINTABLE,   // produces text (base/shifted)
  KB_ROLE_MODIFIER,    // modifies state (held)
  KB_ROLE_TOGGLE,      // toggles state (caps lock)
  KB_ROLE_SPECIAL,     // Enter/Tab/Backspace/Space/etc.
} kb_key_role_t;

typedef struct {
  kb_key_role_t role;
  uint8_t       modifier_bit;   // if role==MODIFIER
  uint8_t       toggle_bit;     // if role==TOGGLE
  uint16_t      keycode;        // if role==SPECIAL (or always, if you choose)
  const char   *text_base;      // if role==PRINTABLE
  const char   *text_shift;     // if role==PRINTABLE
} kb_keymap_entry_t;
```

This table is where device/locale/UX differences live:

- Cardputer has a dedicated `fn` key at `keyNum=29` (`layout.h` legend row2 col0).
- Cardputer-ADV appears to treat `opt` as “Fn-like” (see comment in `0066`), which corresponds to picture position row3 col1 → `keyNum=44`.

The library should not hardcode that “Opt means Fn”; it should accept a keymap where that decision is encoded.

#### Worked example: decoding to both “text” and “keycode”

Many applications want:

- **Text** for insertion into a buffer, and
- **Keycodes** for non-text controls and shortcuts.

One practical rule is:

1. Always emit `KB_EVT_KEYCODE` for non-printables (Enter/Tab/Backspace/Space/arrows).
2. Emit `KB_EVT_TEXT` only on key-down edges *and only* for keys whose keymap role is `PRINTABLE`.
3. Also allow a “shortcut layer” that sees the raw edge + current modifiers so it can match `Ctrl+S` without relying on text.

This produces a stream that works for both a text editor and a menu UI without special casing.

#### Action bindings (semantic chords)

`components/cardputer_kb/include/cardputer_kb/bindings.h` already implements a useful general idea:

- represent a semantic action as a set of required `keyNum`s,
- decode by subset matching,
- resolve conflicts by “most specific wins.”

This is exactly what we want, but generalized:

1. Allow bindings for both devices (and multiple UX profiles).
2. Trigger actions from the normalized state (on press edges or on “chord becomes true” transitions).

If we keep the existing model, the action decoder can be essentially:

```text
on each key-down edge:
  chord = current pressed set
  best = decode_best(chord, bindings)
  if best: emit KB_EVT_ACTION(best->action)
```

The value of the “actions” layer is that it gives UIs stable inputs (Up/Down/Back) without forcing a specific physical chord scheme on every app.

### 5) Deliver Events via Adapters, Not Through a Single Framework

Once we have a stream of normalized/decoded events, delivery becomes an engineering problem: how to connect producers to consumers.

Different applications have different expectations:

1. **FreeRTOS app loop**: wants a `QueueHandle_t` to receive events.
2. **`esp_event` applications**: want `esp_event_post()` into a loop task.
3. **LVGL applications**: want an `lv_indev_drv_t` that supplies `lv_key_t` and text.
4. **Console/REPL**: wants character input (like a UART) but may also want control keys.
5. **Scripting runtimes (QuickJS)**: want a JSON-ish event object (string + modifiers + time).

So the keyboard library should provide a small set of *delivery adapters*:

- `kb_sink_queue`: push `kb_event_t` into a FreeRTOS queue.
- `kb_sink_eventloop`: post typed events into an `esp_event_loop_handle_t`.
- `kb_sink_lvgl`: implement LVGL’s “read” callback over the normalized key state.
- `kb_sink_callback`: invoke a user callback in the keyboard task context (documented).
- `kb_sink_ringbuf`: write events into a lock-free ring buffer for very low overhead.

The simplest mental model: **the keyboard library is a publisher**; applications attach **subscribers** appropriate to their architecture.

#### Worked example: a JS/web-friendly event shape

For scripting runtimes and web UIs, an event should be self-describing and cheap to parse:

```json
{
  "t": 1234567,
  "edge": "down",
  "keyNum": 42,
  "pos": {"x": 13, "y": 2},
  "mods": ["shift","ctrl"],
  "keycode": "Enter",
  "text": ""
}
```

Whether you generate JSON directly or via a tiny struct-to-JSON adapter, this illustrates the central benefit of the layered approach: you can expose the same physical truth plus decoded meaning to very different consumers.

### 6) A Concrete Library Shape

At a component level, this naturally becomes:

```text
components/
  kb_core/            # state, debouncing, repeat, event bus
  kb_hw_cardputer/    # wraps cardputer_kb MatrixScanner -> edges
  kb_hw_cardputer_adv/# wraps tca8418 -> edges (+ remap)
  kb_decode/          # keymap + text/keycode/action decoders
  kb_sinks/           # queue/eventloop/lvgl/etc adapters
```

Or, if you want fewer components, a single `keyboard/` component with internal submodules and `Kconfig` to select the hardware backend.

The one strong recommendation is: keep `cardputer_kb` (the existing component) as a *leaf dependency* rather than rewriting it; treat it as the Cardputer hardware backend’s “scanner primitive.”

## Design Decisions

### Decision 1: Standardize on picture-space `(x,y)` and `keyNum`

**Why:** It already exists, matches human expectations, and both devices can express their keys in it.

**Consequence:** The hardware layer must be responsible for translating electrical details into picture-space. In return, UIs and decoders never need to reason about GPIO pins or TCA row/col.

### Decision 2: Represent hardware output as edge events (down/up)

**Why:** It is the natural abstraction for TCA8418 and the most convenient for debouncing, repeat, and dispatch.

**Consequence:** Snapshot-producing devices must diff snapshots to produce edges (easy, deterministic, and cheap for 56 keys).

### Decision 3: Treat decoding as pluggable (raw vs keycode vs text vs actions)

**Why:** A “keyboard library” that only produces text is unusable for games; a library that only produces raw keys forces every UI to reinvent text and navigation.

**Consequence:** The API surface is slightly larger, but it’s a controlled complexity: decoders are pure functions over `kb_state_t` + keymap/bindings.

### Decision 4: Provide delivery adapters instead of coupling to one framework

**Why:** In embedded systems, you pay dearly for coupling: you can’t reuse code if it only speaks LVGL or only speaks FreeRTOS queues.

**Consequence:** You define one internal event type and write small adapters. Each adapter stays simple and local.

## Alternatives Considered

### Alternative A: Keep the status quo (per-app keyboard logic)

**Pros:** No new component work.

**Cons:** Duplicated decoding logic, inconsistent modifier semantics, inconsistent navigation bindings, and higher bug risk as new apps proliferate.

### Alternative B: Make everything “text-only”

**Pros:** Very simple API (`on_text("a")`).

**Cons:** Loses information (press vs release, modifiers, raw keys), cannot express non-text keys cleanly, and makes it hard to build UIs that need navigation and shortcuts.

### Alternative C: Emulate Linux input subsystem (evdev-like)

**Pros:** Mature conceptual model.

**Cons:** Overkill for a microcontroller environment; introduces complexity (many event types, synchronization semantics) that doesn’t buy much for a 56-key device.

The proposed solution borrows the *good* part of evdev (edge events + key state) without the full subsystem.

## Implementation Plan

1. **Extract and stabilize the ADV hardware backend**
   - Move `tca8418.{h,c}` into a reusable component (or keep as a private module inside a `keyboard` component).
   - Move the TCA→picture-space remap (`decode_tca_event`) into a reusable function with a testable contract.
2. **Wrap Cardputer scan snapshots into edge events**
   - Build a tiny diffing layer around `cardputer_kb::MatrixScanner::scan()`.
3. **Implement shared state tracking**
   - Maintain a `pressed[57]` bitset or boolean array (index by `keyNum`).
   - Add debouncing and repeat (configurable).
4. **Implement a keymap-driven decoder**
   - Start by encoding the mapping present in `0066` (base/shifted pairs).
   - Provide two default keymaps:
     - Cardputer keymap: `fn` is `keyNum=29`.
     - Cardputer-ADV keymap: treat `opt` (`keyNum=44`) as `FN` (as in `0066`), unless a different UX is desired.
5. **Integrate action bindings**
   - Reuse `decode_best` semantics (or import it) for semantic actions.
   - Provide default binding tables (and a way to override at build time).
6. **Provide delivery adapters**
   - FreeRTOS queue sink (mirrors how `0066` already uses a queue in `sim_ui.cpp`).
   - `esp_event` sink (good for loosely-coupled apps).
   - LVGL indev adapter (for GUI apps).
   - Optional JSON sink for scripting/web bridges.
7. **Add tests (host or target)**
   - Remap test: TCA event → `(x,y,keyNum)` matches known expected coordinates.
   - Diff test: snapshot sequences produce correct edge streams.
   - Decoder test: keymap + modifiers produce expected text/keycodes.

## Open Questions

1. **What is the canonical “Fn-like” modifier on Cardputer-ADV?**
   - `0066` uses Opt as Fn-like. Is that universally true across Cardputer-ADV firmwares in this repo, or just for this tutorial?
2. **Do we want to standardize a single “navigation chord” convention across devices?**
   - Cardputer has captured bindings (via `0023`) using Fn+punctuation.
   - ADV may prefer arrows (if present) or different chords.
3. **Should the decoder emit both “text” and “keycode” for printable keys?**
   - Some apps want both (e.g., shortcut handling on keycodes and text entry on text).
4. **Repeat policy**
   - Repeat on held printable keys only?
   - Repeat on navigation actions?
   - Interaction with caps lock and Fn chords.

## References

- Cardputer physical scanner + keyNum contracts:
  - `components/cardputer_kb/REFERENCE.md`
  - `components/cardputer_kb/include/cardputer_kb/scanner.h`
  - `components/cardputer_kb/matrix_scanner.cpp`
- Captured semantic bindings model:
  - `components/cardputer_kb/include/cardputer_kb/bindings.h`
  - `components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h`
- Cardputer-ADV acquisition + UI-level decoding example:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/tca8418.c`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp` (queue-based delivery)

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
