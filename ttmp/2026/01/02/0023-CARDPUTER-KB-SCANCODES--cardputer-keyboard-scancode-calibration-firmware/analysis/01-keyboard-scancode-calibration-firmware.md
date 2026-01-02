---
Title: 'Keyboard scancode calibration firmware: analysis + implementation plan'
Ticket: 0023-CARDPUTER-KB-SCANCODES
Status: active
Topics:
    - cardputer
    - keyboard
    - scancodes
    - esp32s3
    - esp-idf
    - debugging
    - ui
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Raw matrix scan reference (scan_state/out bits + input bitmask)
    - Path: ../../../../../../0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Current scanner implementation we can reuse as a starting point
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h
      Note: Vendor key map and key numbering convention (getKeyNum)
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Vendor scan implementation (pins + scan order)
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/keyboard/keymap.h
      Note: HID constants and arrow key usage IDs
ExternalSources: []
Summary: Implementation-ready design for a standalone Cardputer firmware that discovers raw keyboard scan codes and generates a reusable decoder mapping (including Fn-combos for arrows).
LastUpdated: 2026-01-02T02:30:00Z
WhatFor: ""
WhenToUse: ""
---

# Keyboard scancode calibration firmware (Cardputer)

## Problem statement

Our current Cardputer projects (e.g. the M5GFX demo-suite) use convenience mappings like `W/S` or hardcoded labels from the vendor key map. This breaks down for:

- “arrow” navigation, which is typically a **Fn+key combo** on compact keyboards
- firmware-to-firmware drift (different assumptions about what a key “means”)
- hardware variation (alternate IN0/IN1 wiring has appeared in the wild)

We need a **ground-truth calibration firmware** that can (1) show raw scan evidence and (2) capture stable identifiers for keys and combos, then emit a decoder config we can reuse elsewhere.

## Goals

- Provide an interactive, on-device UX to discover:
  - raw scan behavior
  - which physical keys constitute “Up/Down/Left/Right”
  - how modifiers (Fn/Shift/Ctrl/Alt/Opt) behave in the matrix
- Capture a mapping from “logical action” → “physical signature”:
  - single keys (Enter/Tab/Del/Space…)
  - combos (Fn+something, Shift+something, Fn+Shift+something)
- Output a machine-readable config via serial (JSON) and a copy/paste C/C++ snippet to implement decoding.
- Produce verbose debug logs for:
  - scan_state and in_mask per scan iteration
  - derived `x,y` and vendor-style `keyNum` (1..56)
  - chord stability timing and debounce

## Non-goals

- Shipping a general-purpose keyboard library.
- Implementing a full BLE/USB keyboard stack.
- Making a long-term end-user UI; this is a developer tool firmware.

## Terminology (what we mean by “scancode” here)

Because this is a matrix-scanned keyboard, the most stable “physical identifier” is not a character, but a **matrix signature**.

We will represent a key press in three progressively-higher layers:

1. **Raw scan**:
   - `scan_state` (0..7): which output bits are driven
   - `in_mask` (7-bit): which input lines read low

2. **Derived physical key position**:
   - `KeyPos{x,y}` using the repo’s established “picture coordinate system” (4 rows × 14 cols)

3. **Vendor key number**:
   - `keyNum = y*14 + (x+1)` (1..56), matching `KEYBOARD::Keyboard::getKeyNum()` in vendor code

Combos are then a *set* of `KeyPos` values pressed simultaneously.

## Prior art in this repo (what to reuse)

### Raw scanning and logging

- `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c`
  - shows how to log `scan_state` + `in_mask` and derive `x,y`
  - includes “alt IN0/IN1” autodetect logic

### Current scanner used by demo-suite

- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
  - already implements edge events + modifier booleans (`fn`, `shift`, `ctrl`, `alt`)
  - assumes the vendor `4x14` key legend for labels, but does not discover “arrow” semantics

### Vendor “ground truth” for matrix pins and coordinate conventions

- `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h`
  - `KEYBOARD::output_list = {8,9,11}`
  - `KEYBOARD::input_list  = {13,15,3,4,5,6,7}` (with a commented alt variant `{1,2,...}`)
  - `KEYBOARD::_key_value_map[4][14]` (labels + HID usage codes for basic keys)

- `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp`
  - `_set_output()` and `_get_input()` show the canonical scan loop
  - `updateKeyList()` supports multiple simultaneous keys

### HID usage constants (for arrows)

- `M5Cardputer-UserDemo/main/hal/keyboard/keymap.h`
  - `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT` are defined as HID usages
  - this file is also the canonical list of key usage IDs we should emit in the config

## Firmware UX (implementation-ready)

The calibration app is a simple wizard with three always-visible panels:

1. **Prompt panel** (top):
   - “Press: Up Arrow (Fn+?)”
   - shows target action name and a short tip (“Press and hold for 500ms”)

2. **Live matrix panel** (center):
   - 4×14 grid of cells labeled with the vendor legend (optional)
   - highlight currently pressed keys
   - also show the raw `keyNum` for each pressed key

3. **Debug panel** (bottom):
   - raw scan evidence (recent scan_state/in_mask lines)
   - derived `KeyPos` set printed as `{(x,y), ...}`
   - chord stability timer state

### Calibration flow

For each logical binding:

1. show prompt: “Press <BindingName>”
2. wait until:
   - the pressed set is non-empty
   - the pressed set remains stable for N ms (e.g., 250–500ms)
3. show confirmation:
   - “Captured: fn + w (KeyPos: (2,2) + (0,2))”
4. allow:
   - `Enter` to accept
   - `Del` to redo
   - `Tab` to skip
5. proceed to next binding

Bindings to start with:

- `NavUp`, `NavDown`, `NavLeft`, `NavRight`
- `Back` / `Esc`
- `Enter`
- `Tab`
- `Space`

## Output format (decoder config)

### Canonical representation

Each binding is a set of “required physical keys”.

Represent required keys by vendor `keyNum` for compactness:

```json
{
  "device": "M5Cardputer",
  "matrix": {"rows": 4, "cols": 14},
  "pins": {"out": [8,9,11], "in": [13,15,3,4,5,6,7], "alt_in01": [1,2]},
  "bindings": [
    {"name":"NavUp", "required_keynums":[<fn_keynum>, <some_keynum>]},
    {"name":"NavDown", "required_keynums":[...]}
  ]
}
```

### C/C++ header snippet

Also print a snippet that can be pasted into another firmware:

```cpp
struct Binding { const char* name; uint8_t n; uint8_t keynums[4]; };
static const Binding kBindings[] = { ... };
```

## Decoder design (how other firmware will consume it)

Given:

- a pressed set of `keyNum`s (from the scanner)
- a list of binding rules (`required_keynums`)

Decoding is:

- A binding matches if every required keyNum is pressed (subset test).
- Prefer the “most specific” binding when multiple match (larger required set wins).

This is important for Fn-combos: `fn + w` should beat `w` if both exist.

## Debug logging requirements

The firmware should offer at least two verbosity levels:

### Level 1 (default)

- per change in pressed set:
  - list of pressed `KeyPos` and `keyNum`
  - inferred modifiers (presence of fn/shift/ctrl/alt keys)

### Level 2 (raw scan)

- for each scan_state with any activity:
  - `scan_state=<n> out=<n> in_mask=0x.. pinset=primary|alt_in01`

This is the fastest path to understanding “my physical key doesn’t show up where I expect”.

## Implementation plan (files + responsibilities)

Create a new ESP-IDF project:

`esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/`

Recommended file layout:

- `main/app_main.cpp`
  - display init (M5GFX autodetect)
  - main loop + UI rendering
  - calibration state machine
- `main/kb_scan.cpp` / `main/kb_scan.h`
  - raw matrix scan (multi-key)
  - alt IN0/IN1 autodetect
  - “pressed set” API returning `std::vector<KeyPos>` and `std::vector<uint8_t keyNum>`
  - optional debounce/stability helper
- `main/calibration.cpp` / `main/calibration.h`
  - list of bindings to capture
  - capture/redo/skip logic
  - export to JSON + header snippet

## Review-critical decisions (ask/confirm)

- **Key identity**: store bindings by vendor `keyNum` (stable for our codebase) rather than characters.
- **Combo matching**: subset test + “most specific wins”.
- **Fn semantics**: treat Fn as a physical key in the pressed set (do not special-case it into a separate modifier bit), since arrows are likely implemented as combos.
- **Output framing**: for the JSON blob, print `CFG_BEGIN <len>` / `CFG_END` framing similar to screenshot framing (optional, but recommended for robust capture).

