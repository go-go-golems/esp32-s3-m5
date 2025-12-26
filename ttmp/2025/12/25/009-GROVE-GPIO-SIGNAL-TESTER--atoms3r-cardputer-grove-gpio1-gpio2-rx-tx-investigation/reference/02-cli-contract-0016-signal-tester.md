---
Title: CLI contract (0016 signal tester)
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - cardputer
    - gpio
    - uart
    - serial
    - usb-serial-jtag
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/README.md
      Note: Canonical build/flash instructions + the high-level REPL command list
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/console_repl.cpp
      Note: Command parsing, argument validation, and usage text (truth of the CLI surface)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: Command semantics (how mode/pin/tx/rx changes actually apply to GPIO1/GPIO2)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/playbook/02-debug-grove-uart-using-0016-signal-tester.md
      Note: End-to-end “how to use it” procedure (operationalizes this contract)
ExternalSources: []
Summary: "Stable, copy/paste-ready REPL command contract for `0016-atoms3r-grove-gpio-signal-tester` (USB Serial/JTAG esp_console control plane)."
LastUpdated: 2025-12-25T22:38:16.854580906-05:00
WhatFor: "Make the signal tester controllable in a deterministic way (and keep docs/tests aligned with the actual command surface)."
WhenToUse: "When operating the 0016 tester, writing playbooks, or updating firmware code that changes command names/args/output."
---

# CLI contract (0016 signal tester)

## Goal

Define the **stable REPL interface** (commands, arguments, semantics, and expected output) for controlling `0016-atoms3r-grove-gpio-signal-tester` over **USB Serial/JTAG**.

## Context

- The REPL is `esp_console` running over **USB Serial/JTAG** (it does **not** depend on GROVE UART pins).
- Commands map to `CtrlEvent` messages; a **single-writer** state machine applies changes to GPIO1/GPIO2.
- On every apply, the state machine **stops TX and disables RX**, then resets both GPIO1 and GPIO2 into a safe input baseline, then configures only the active pin (see `main/signal_state.cpp`).

Update: `0016` now uses a **manual line-based REPL** (no `esp_console`). The command surface is intentionally kept compatible (`help`, `mode`, `pin`, `tx`, `rx`, `status`), but UX features like history/tab completion are gone by design.

## Quick Reference

### Transport + prompt

- **Transport**: USB Serial/JTAG (connect to the AtomS3R device’s `ttyACM*` on Linux)
- **Prompt**: `sig> ` (config: `CONFIG_TUTORIAL_0016_CONSOLE_PROMPT`)

### Pin naming contract (physical intent)

- `pin 1` selects **GROVE G1 / GPIO1**
- `pin 2` selects **GROVE G2 / GPIO2**

Direction (`RX` vs `TX`) is a wiring convention; the tester treats pins as “wires” first.

### Mode contract

- `mode idle`: true baseline (both pins input, pulls off, interrupts off)
- `mode tx`: active pin is GPIO-driven output (TX generator)
- `mode rx`: active pin is GPIO-owned input with pull + ISR edge counter

### Commands

#### `help`

- **Purpose**: show built-in help (from `esp_console`)
- **Args**: none

#### `mode tx|rx|idle`

- **Purpose**: set top-level tester mode
- **Notes**: changing mode triggers a full apply (safe reset of both pins, then configure active pin)

#### `pin 1|2`

- **Purpose**: select the “active” pin (GPIO1 or GPIO2)
- **Notes**: switching pins triggers a full apply (safe reset then configure new active pin)

#### `tx high|low|stop`

- **Purpose**: drive active pin high/low, or stop TX and return to baseline
- **Semantics**:
  - `tx high` forces mode `TX` and drives output high
  - `tx low` forces mode `TX` and drives output low
  - `tx stop` stops timers and returns to `IDLE`

#### `tx square <hz>`

- **Purpose**: generate a square wave on active pin using `esp_timer`
- **Args**:
  - `<hz>`: integer frequency in Hz
- **Validation** (enforced in `console_repl.cpp`):
  - allowed range: **1..20000**

#### `tx pulse <width_us> <period_ms>`

- **Purpose**: periodic pulse train useful for triggering scope/logic analyzer
- **Args**:
  - `<width_us>`: positive integer (microseconds)
  - `<period_ms>`: positive integer (milliseconds)
- **Validation**: both must be `> 0`

#### `rx edges rising|falling|both`

- **Purpose**: choose which edges count in RX mode
- **Semantics**:
  - takes effect immediately if currently in `mode rx`
  - otherwise, it updates config and will take effect on next `mode rx`

#### `rx pull none|up|down`

- **Purpose**: configure internal pull while in RX mode
- **Semantics**: same “apply immediately if in RX mode” behavior as `rx edges`
- **Electrical note**:
  - `rx pull none` is the closest “high‑Z input while still counting edges” configuration.
  - `mode idle` is the strongest baseline (high‑Z + interrupts off).

#### `rx reset`

- **Purpose**: reset RX counters (edges/rises/falls + last tick/level)

#### `status`

- **Purpose**: print current mode/pin/tx settings + rx stats
- **Output**: one-line key/value-ish summary (exact formatting defined in `signal_state.cpp`)

Example (shape):

```text
mode=IDLE pin=2 (gpio=2) tx=STOP rx_edges=BOTH rx_pull=UP rx_edges_total=0 rises=0 falls=0 last_tick=0 last_level=0
```

## Usage Examples

### Example: prove “high‑Z baseline” vs contention

```text
sig> mode idle
sig> pin 2
sig> status
sig> mode tx
sig> tx high
sig> status
sig> tx low
sig> status
sig> tx stop
sig> mode idle
```

### Example: cable mapping (G1/G2 swap detection)

```text
sig> pin 1
sig> mode tx
sig> tx square 1000
sig> status
sig> pin 2
sig> tx square 1000
sig> status
```

### Example: RX edge counter sanity check

```text
sig> pin 2
sig> mode rx
sig> rx edges both
sig> rx pull up
sig> rx reset
sig> status
```

## Related

- Ticket spec: `../analysis/01-spec-grove-gpio-signal-tester.md`
- Wiring/pin mapping: `03-wiring-pin-mapping-atoms3r-cardputer-grove-g1-g2.md`
- Test matrix: `04-test-matrix-cable-mapping-contention-diagnosis.md`
- Operational playbook: `../playbook/02-debug-grove-uart-using-0016-signal-tester.md`
