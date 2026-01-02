# `cardputer_kb` component

Reusable Cardputer keyboard matrix scan + layout helpers.

## What it provides

- Matrix scan that returns a stable “physical key” representation:
  - `KeyPos{x,y}` (4×14 picture coordinate system)
  - vendor-style `keyNum` (`y*14 + (x+1)`)
- Optional alt IN0/IN1 autodetect (`{13,15,...}` → `{1,2,...}`).
- Static key legend table (same as vendor HAL) for UI/debug output.

## Why this exists

We have multiple firmwares that need a consistent definition of “which physical key is pressed”, especially for Fn-combos (arrow navigation etc). This component keeps that logic in one place.

