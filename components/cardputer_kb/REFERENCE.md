# `cardputer_kb` — Reference

## Purpose

Provide a stable, reusable “physical keyboard” layer for the M5 Cardputer:

- Scan the GPIO matrix and return pressed **physical keys** as:
  - `KeyPos{x,y}` in the vendor “picture” coordinate system (4×14)
  - vendor-style `keyNum` (1..56) where `keyNum = y*14 + (x+1)`
- Offer a small semantic binding/decoder layer for mapping actions to chords (`required_keynums`).

## Files / Headers

- Scanner API: `components/cardputer_kb/include/cardputer_kb/scanner.h`
- Layout helpers + legend: `components/cardputer_kb/include/cardputer_kb/layout.h`
- Binding decoder: `components/cardputer_kb/include/cardputer_kb/bindings.h`
- Captured example bindings (M5Cardputer): `components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h`

## Matrix and pins (Cardputer)

The scanner implements the vendor scan wiring:

- Outputs: `{8, 9, 11}` (3-bit select)
- Inputs (primary): `{13, 15, 3, 4, 5, 6, 7}`
- Inputs (alt IN0/IN1): `{1, 2, 3, 4, 5, 6, 7}`

The scanner can switch to the alt pinset if it observes activity on the alt IN0/IN1 pins while primary is idle.

## Scanner API

### Types

- `cardputer_kb::KeyPos` — physical key position
- `cardputer_kb::ScanSnapshot`
  - `pressed`: `std::vector<KeyPos>` (deduped, sorted)
  - `pressed_keynums`: `std::vector<uint8_t>` (1..56)
  - `use_alt_in01`: `bool` (which pinset is active)

### Functions

- `cardputer_kb::MatrixScanner::init()`
  - configures GPIO directions + pullups and sets outputs low
- `cardputer_kb::MatrixScanner::scan() -> ScanSnapshot`
  - scans `scan_state=0..7` and collects all pressed keys

### Contracts / gotchas

- This is **not** character decoding. It returns physical key identity.
- `pressed_keynums` may contain multiple keys (chords), including `fn` (keyNum `29`).
- The scan loop uses a small settle delay (`~10us`) after setting outputs.
- Autodetect only switches on observed activity; “no key pressed” looks identical on both pinsets.

## Layout / legend helpers

- `cardputer_kb::keynum_from_xy(x,y) -> uint8_t`
- `cardputer_kb::xy_from_keynum(keynum, &x, &y)`
- `cardputer_kb::legend_for_xy(x,y) -> const char*`

The legend is for UI/debug only. Do not treat it as a semantic mapping for navigation.

## Binding decoder

### Why bindings exist

Actions like “Up/Down/Left/Right” are **semantic**, and on Cardputer they may be implemented as:

- Fn + key chords, or
- punctuation keys (depending on UX conventions)

The binding decoder maps:

- pressed set of `keyNum`s → a matching `Binding` (action + required chord)

### API

- `cardputer_kb::Binding` — `{action, n, keynums[4]}`
- `cardputer_kb::decode_best(pressed_keynums, bindings, bindings_count) -> const Binding*`
  - A binding matches if all `required keynums` are currently pressed (subset test).
  - “Most specific wins”: the binding with the largest chord length `n` wins.

### Captured bindings

Example captured nav bindings are provided as:

- `cardputer_kb::kCapturedBindingsM5Cardputer` in `bindings_m5cardputer_captured.h`

These are intended as a starting point and may be regenerated using the 0023 calibrator.

