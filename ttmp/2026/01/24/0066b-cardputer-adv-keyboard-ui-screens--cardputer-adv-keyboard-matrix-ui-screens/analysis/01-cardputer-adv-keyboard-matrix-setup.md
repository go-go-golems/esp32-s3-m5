---
Title: Cardputer ADV keyboard matrix setup
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Example KeyEvent+modifier decoding
    - Path: 0023-cardputer-kb-scancode-calibrator/main/app_main.cpp
      Note: Reference UI for verifying matrix scan
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Example keyboard task + event posting
    - Path: components/cardputer_kb/REFERENCE.md
      Note: Pin mappings and binding decoder reference
    - Path: components/cardputer_kb/include/cardputer_kb/scanner.h
      Note: Scanner API used by firmwares
    - Path: components/cardputer_kb/matrix_scanner.cpp
      Note: Keyboard matrix scan implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T00:46:15.759649677-05:00
WhatFor: ""
WhenToUse: ""
---


# Cardputer‑ADV keyboard matrix setup (ESP‑IDF, `cardputer_kb`)

This document explains how to read the Cardputer / Cardputer‑ADV keyboard reliably in ESP‑IDF, using the repo’s reusable component `components/cardputer_kb`. It is written to be an implementation guide: it describes *wiring*, *software contracts*, and *integration patterns* (polling loop, edge detection, chord decoding), and it ends with concrete “how to wire it into a UI loop” pseudocode.

## 0) Where the truth lives (files to read first)

### Scanner and layout

- Scanner API (what you call): `components/cardputer_kb/include/cardputer_kb/scanner.h`
- Scanner implementation (what it actually does): `components/cardputer_kb/matrix_scanner.cpp`
- Layout helpers + legend table: `components/cardputer_kb/include/cardputer_kb/layout.h`

### Semantic binding decoder (chords → actions)

- Binding decoder: `components/cardputer_kb/include/cardputer_kb/bindings.h`
- Captured default bindings: `components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h`

### “Known good” usage examples in this repo

- Calibrator / visualizer: `0023-cardputer-kb-scancode-calibrator/main/app_main.cpp`
- Keyboard-to-events pattern: `0030-cardputer-console-eventbus/main/app_main.cpp` (dedicated task + edge detection)
- Lightweight “key event” helper: `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.{h,cpp}`

## 1) The physical model: a scanned matrix, not a UART keyboard

The Cardputer keyboard is not a character device. It is a *switch matrix*:

- You choose one of several “scan states” by driving a small number of *output select pins*.
- You then read a bank of *input pins* to see which switches connect through in that scan state.

This is why the scanner returns “physical identity” instead of ASCII:

- `(x,y)` coordinate in the vendor “picture” coordinate system (4 rows × 14 columns)
- `keyNum` in `[1..56]` where:
  - `keyNum = y*14 + (x+1)`

> **Callout: Why physical identity matters**
>
> In embedded UIs we often care about *chords* (Fn+something), navigation, and stable “button-like” semantics.
> Mapping directly to ASCII early makes chords harder (do you represent Fn?), and it hides the reality that the same physical key may mean “left” in one context and “,” in another.

## 2) Pin wiring: what `cardputer_kb` assumes

The scanner uses the vendor scan wiring (same across the repo’s Cardputer projects):

- Output select pins (3 bits): `{8, 9, 11}`
- Input pins (primary): `{13, 15, 3, 4, 5, 6, 7}`
- Input pins (alt IN0/IN1): `{1, 2, 3, 4, 5, 6, 7}`

The “alt IN0/IN1” is a practical accommodation: some setups use GPIO1/2 instead of GPIO13/15 for two inputs. The scanner can auto-switch if it observes activity on alt pins while the primary set reads idle.

Reference: `components/cardputer_kb/REFERENCE.md` and `components/cardputer_kb/matrix_scanner.cpp`.

### GPIO configuration choices (why it looks like it does)

In `matrix_scanner.cpp`, initialization does:

- outputs: reset pin, set output direction, set “pullup/pulldown” mode, drive low
- inputs: reset pin, set input direction, enable pullup

This yields active-low reads:

- `gpio_get_level(pin) == 0` means “pressed in this scan state”

## 3) Software contract: `MatrixScanner::scan()`

The scanner API is minimal:

- `MatrixScanner::init()`
- `MatrixScanner::scan() -> ScanSnapshot`

`ScanSnapshot` includes:

- `pressed`: `std::vector<KeyPos>` sorted and deduped
- `pressed_keynums`: `std::vector<uint8_t>` (1..56), derived from `pressed`
- `use_alt_in01`: whether the alt pinset is active

Important properties:

1) It scans *all* scan states (`scan_state=0..7`) and reports all pressed keys.
2) It does not do debouncing; it returns the instantaneous physical set.
3) It does not do character decoding; it returns physical identity.
4) It includes modifier keys as keynums just like any other key (e.g. Fn is `keyNum=29`).

## 4) From “snapshot” to “events”: edge detection and repeat policy

UIs do not want the full pressed set every frame; they want “events”:

- key down edges (`A` pressed now but wasn’t pressed last scan)
- semantic actions (NavUp, Back, Enter, …)
- optionally: key repeat for held keys (for sliders)

### Edge detection (minimal, deterministic)

Given:

- `prev_pressed_keynums`
- `down_keynums`

The set of new presses is:

- `down_keynums \ prev_pressed_keynums`

This is exactly the pattern used in:

- `0030-cardputer-console-eventbus/main/app_main.cpp` (keyboard task)
- `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` (poll helper)

### Repeat policy (a design choice)

For menus, “one press = one move” is fine.

For sliders, “hold arrow to adjust continuously” is useful. There are two common policies:

1) **UI-driven repeat**: generate only edges, but the UI repeats based on “is key still down”.
2) **Keyboard-driven repeat**: the keyboard layer itself emits repeated events after an initial delay.

This ticket’s UI spec (see imported UI mock) implies UI-driven repeat, because modifiers change step size and we want repeat to respect the current modifier state at each tick.

## 5) Semantic actions (Fn chords): `bindings.h` + captured bindings

`cardputer_kb` includes a small decoder that maps “pressed chord” → “action”.

Example actions:

- `NavUp`, `NavDown`, `NavLeft`, `NavRight`
- `Back`, `Del`, `Enter`, `Tab`, `Space`

The decoder:

- tests whether a binding chord is a subset of the current pressed set
- chooses the “most specific” binding (largest chord) when multiple match

In practice, this lets you define “Fn+;” as “Up”, etc, without baking that into UI code.

### Default captured bindings (what we ship today)

From `bindings_m5cardputer_captured.h` (these are physical keyNums, not ASCII):

- `NavUp` = `Fn(29) + 40`
- `NavDown` = `Fn(29) + 54`
- `NavLeft` = `Fn(29) + 53`
- `NavRight` = `Fn(29) + 55`
- `Back` = `Fn(29) + 1`
- `Del` = `Fn(29) + 14`
- `Enter` = `42`
- `Tab` = `15`
- `Space` = `56`

> **Callout: “Fn chords” are a UI convention, not hardware**
>
> Nothing in the hardware says “this is up”. “Up” is a *semantic contract* between your firmware UX and the physical keyboard.
> Keeping it in a binding table means you can evolve UX without rewriting the scanner.

## 6) Modifiers: representing Ctrl/Opt/Alt/Fn/Shift

The scanner gives physical keynums; a UI typically wants a normalized modifier state:

- `shift` (keyNum `30`)
- `fn` (keyNum `29`)
- `ctrl` (keyNum `43`)
- `opt` (keyNum `44`)
- `alt` (keyNum `45`)

The existing helper in `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` treats:

- `alt = alt || opt`

This matches the repo’s “modifier semantics” habit: Alt and Opt are both “bigger adjustment step”.

## 7) Integration patterns (where the keyboard scanner runs)

There are two stable patterns in this repo.

### Pattern A: scan inside the UI task

Pros:

- simplest (one loop does both render + input)
- easiest to reason about (single-threaded UI state)

Cons:

- scan frequency becomes tied to render cadence
- if rendering stalls, input stalls

### Pattern B: dedicated keyboard task + queue/event bus (recommended)

Pros:

- stable input sampling independent of render performance
- easy to insert debouncing or repeat scheduling
- UI loop consumes events and stays simple

Cons:

- introduces cross-task message passing (queue sizing, drops)

Example: `0030-cardputer-console-eventbus/main/app_main.cpp` runs a dedicated keyboard task and posts events.

For 0066, we will prefer Pattern B with a bounded FreeRTOS queue:

```
keyboard_task:
  scanner.init()
  prev_pressed = []
  loop every 10ms:
    snap = scanner.scan()
    events = diff(prev_pressed, snap.pressed_keynums)
    push events into ui_queue (best-effort)
    prev_pressed = snap.pressed_keynums

ui_task:
  loop:
    while queue has events:
      ui_state.handle(event)
    render(ui_state, sim_engine_frame)
    sleep(frame_ms)
```

## 8) Build integration: how to “enable the keyboard” in a firmware

At build time you need:

1) Ensure the keyboard component is available to the project.
2) Add it to your `main` component requirements.

For tutorial-style projects that manually list components via `EXTRA_COMPONENT_DIRS`, add:

- `${CMAKE_CURRENT_LIST_DIR}/../components/cardputer_kb`

Then in `main/CMakeLists.txt` add `cardputer_kb` to `REQUIRES`.

## 9) Debugging checklist (what to do when “no keys work”)

1) Run the calibrator firmware:
   - `0023-cardputer-kb-scancode-calibrator`
   - It draws a 4×14 grid and highlights pressed cells, printing keyNums to the console.
2) Watch `use_alt_in01`:
   - if your hardware uses the alt pins, the scanner will log an autodetect switch.
3) Confirm no other peripheral is driving the matrix pins.
4) Confirm you are not relying on UART console pins that overlap input pins:
   - This repo prefers USB Serial/JTAG console to avoid pin contention.

## 10) Practical recommendation for Cardputer‑ADV UI work

Use the scanner as a *physical truth* source, then build semantic layers above it:

- Physical scan → `pressed_keynums`
- `pressed_keynums` + binding table → nav actions
- `pressed_keynums` + key legend map → typed keys (optional)
- UI state machine consumes events and applies context-sensitive meaning

This separation keeps the system maintainable: changes to “what key means Up” do not require changes to “how to scan the matrix”.
