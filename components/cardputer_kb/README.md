# `cardputer_kb` component

Reusable Cardputer keyboard scan + layout helpers (supports both Cardputer GPIO matrix and Cardputer-ADV TCA8418).

## What it provides

- A unified scanner API (`cardputer_kb::UnifiedScanner`) that returns a stable “physical key” representation:
  - `KeyPos{x,y}` (4×14 picture coordinate system)
  - vendor-style `keyNum` (`y*14 + (x+1)`)
- Optional alt IN0/IN1 autodetect (`{13,15,...}` → `{1,2,...}`).
- Cardputer-ADV TCA8418 support (I2C event FIFO → picture-space remap → `pressed_keynums` snapshot).
- Static key legend table (same as vendor HAL) for UI/debug output.
- Optional semantic binding decoder (actions → required chords), with captured examples.

## Why this exists

We have multiple firmwares that need a consistent definition of “which physical key is pressed”, especially for Fn-combos (arrow navigation etc). This component keeps that logic in one place.

## Developer docs

- Reference: `components/cardputer_kb/REFERENCE.md`
- Tutorial: `components/cardputer_kb/TUTORIAL.md`
