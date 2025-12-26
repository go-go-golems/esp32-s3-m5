---
Title: 'Cable mapping: determine whether GROVE G1/G2 are swapped (using 0016)'
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
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
      Note: TX square wave and pulse generation used for mapping tests
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: Defines the exact electrical meaning of mode/pin changes (safe reset then apply active pin)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/reference/04-test-matrix-cable-mapping-contention-diagnosis.md
      Note: Copy/paste matrix for recording results (truth table)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/reference/03-wiring-pin-mapping-atoms3r-cardputer-grove-g1-g2.md
      Note: Label→GPIO mapping reference for interpreting results
ExternalSources: []
Summary: "Step-by-step procedure to determine whether a GROVE cable/device labeling swaps G1/G2 (GPIO1/GPIO2), using 0016 square wave output plus a 2×2 observation matrix."
LastUpdated: 2025-12-26T08:29:13.909426245-05:00
WhatFor: "Remove ambiguity about whether G1/G2 are swapped across boards/cables before debugging UART/peripheral behavior."
WhenToUse: "When RX appears dead, when labels differ (G1/G2 vs SDA/SCL), or when you suspect wiring/cable issues."
---

# Cable mapping: determine whether GROVE G1/G2 are swapped (using 0016)

## Purpose

Determine, with minimal ambiguity, whether a GROVE cable/device swaps the two signal wires:

- **G1 / GPIO1**
- **G2 / GPIO2**

This playbook treats the pins as “wires” first and uses a known, generated square wave from the AtomS3R (`0016`) to fill in a small observation matrix.

## Environment Assumptions

- AtomS3R flashed with `0016` and reachable on USB Serial/JTAG (see playbook `03-bring-up-...`).
- You have a scope or logic analyzer and can probe both signal pins at the peer side.
- You have a single GROVE cable/device path under test (avoid multiple adapters stacked if possible).

## Commands

```bash
# REPL is on USB Serial/JTAG. These are typed at the "sig> " prompt (not run in bash).
```

### Step 1: Put the tester in a safe baseline

```text
sig> mode idle
```

### Step 2: Drive a 1 kHz square wave on G1/GPIO1

```text
sig> pin 1
sig> mode tx
sig> tx square 1000
```

Probe the peer-side GROVE header and record where the square wave appears (peer G1 vs peer G2).

### Step 3: Drive a 1 kHz square wave on G2/GPIO2

```text
sig> pin 2
sig> tx square 1000
```

Probe again and record where the wave appears.

### Step 4: Record sheet

Fill this in:

```text
Atom drives:      Peer observes square wave on:
TX GPIO1 (G1)  ->  [ ] peer G1   [ ] peer G2   [ ] neither
TX GPIO2 (G2)  ->  [ ] peer G1   [ ] peer G2   [ ] neither
```

## Exit Criteria

- You can fill both rows of the matrix with confident observations.
- Interpretation is unambiguous:
  - **Not swapped**: GPIO1→peer G1 AND GPIO2→peer G2
  - **Swapped**: GPIO1→peer G2 AND/OR GPIO2→peer G1
  - **Neither row shows signal**: wiring/ground/probe point is wrong (fix before continuing)

## Notes

- Use `mode idle` between configurations if you suspect contention or if the peer device might also be driving the line.
- If the peer device is a Cardputer, ensure its firmware isn’t also manipulating GPIO1/GPIO2 (keyboard autodetect can conflict on some revisions).
- For a copy/paste-ready “matrix + contention” sheet, see `reference/04-test-matrix-cable-mapping-contention-diagnosis.md`.
