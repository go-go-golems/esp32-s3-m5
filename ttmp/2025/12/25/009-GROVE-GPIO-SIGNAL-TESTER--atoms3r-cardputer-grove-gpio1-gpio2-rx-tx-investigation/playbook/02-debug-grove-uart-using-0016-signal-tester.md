---
Title: "Playbook: debug GROVE UART RX/TX using the 0016 GPIO signal tester (AtomS3R)"
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
  - esp32s3
  - esp-idf
  - atoms3r
  - gpio
  - uart
  - serial
  - usb-serial-jtag
  - debugging
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/README.md
    Note: Build/flash instructions + REPL command list
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
    Note: The “truth” of what modes actually do to GPIO1/GPIO2 (safe reset + apply)
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
    Note: TX patterns (high/low/square/pulse) implemented with esp_timer
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_rx.cpp
    Note: RX edge counter ISR + pull configuration
ExternalSources: []
Summary: "How to use the AtomS3R-only 0016 signal tester firmware to debug GROVE G1/G2 (GPIO1/GPIO2) UART wiring, pin swaps, and electrical contention—before touching esp_console/UART VFS."
LastUpdated: 2025-12-25T00:00:00.000000000+00:00
WhatFor: "Turn ambiguous 'TX works but RX doesn't' into crisp yes/no results at the GPIO layer."
WhenToUse: "When you see UART TX on a scope but the receiver appears to see no RX activity, or when you suspect G1/G2 mapping/cable swap/contestion."
---

## Executive summary

This playbook is how I would use the `0016-atoms3r-grove-gpio-signal-tester` firmware to debug your GROVE serial problem **without involving `esp_console` on the GROVE pins**. The key idea is to treat GPIO1/GPIO2 as “just wires” first and answer three questions in order:

1. **Is the cable/pin mapping correct?** (Does a generated signal on Atom GPIO1/2 show up where you think it does?)
2. **Is the RX line electrically healthy?** (Does “low” go near 0V, or does it float at ~1.5V implying contention/bias?)
3. **Does the Atom physically observe edges on the pin?** (GPIO ISR edge counter increments when the peer transmits)

Only after those are proven should we go back up to UART-peripheral and `esp_console`/VFS/linenoise assumptions.

## What the 0016 firmware actually does to GPIO1/GPIO2 (important)

The behavior of the tool is defined by a single-writer state machine in `0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp`:

- On every mode change, it **stops TX and disables RX**, then **resets both GPIO1 and GPIO2 into a safe input state** (input, pulls off, interrupts off).
- Then it configures **only the active pin** according to the selected mode:
  - TX: sets pin to output and drives/pulses it (`gpio_tx_apply()` in `gpio_tx.cpp`)
  - RX: sets pin as input, applies pull, attaches GPIO ISR (`gpio_rx_configure()` in `gpio_rx.cpp`)
  - IDLE: leaves both pins in safe input state (your “Hi‑Z baseline”)

This “reset both pins first” design is exactly what you want when diagnosing contention: it lets you deliberately put the Atom into known electrical states.

## Setup

### Hardware

- AtomS3R running `0016`
- A scope or logic analyzer probing GROVE pins
- Optional peer device (Cardputer, USB↔UART dongle, function generator)
- Shared ground between Atom and the peer (if connected)

### Software

- Connect to the Atom via **USB Serial/JTAG** and use the REPL prompt (`sig> `).
- Command set is documented in `0016-atoms3r-grove-gpio-signal-tester/README.md`.

## Workflow A: Baseline “is Atom biasing the line?” (Hi‑Z vs pull vs drive)

This is the most direct way to interpret the observed “low lifts to ~1.5V when connected” symptom.

### Step A1: Put Atom pins into true baseline (Hi‑Z)

On the Atom REPL:

- `mode idle`

What this means electrically (from `signal_state.cpp`):

- GPIO1 and GPIO2 are input
- pull-up and pull-down disabled
- interrupts disabled

**What to look for on scope:**

- With peer disconnected: do pins float high? low? noisy? (this reveals board-level pulls)
- With peer connected and transmitting: does the peer TX waveform remain ~0–3.3V?
  - If yes: Atom isn’t strongly biasing/driving in idle; contention likely occurs only in some other mode.
  - If no (low lifts even in idle): look for *external* contention (cable/wiring, peer side, board circuitry).

### Step A2: Enable RX mode on the suspect pin, with pull control

On the Atom:

- `pin 2` (to select GPIO2)
- `mode rx`
- `rx pull none` (repeat with `up`, then `down`)

**Interpretation guide:**

- If `rx pull up` makes the idle level go high (expected), but a peer TX low still reaches ~0V: that’s normal.
- If merely enabling `mode rx` (even with `pull none`) causes the peer TX low to jump toward ~1.5V: that suggests the line is being fought or heavily biased somewhere (not a “line endings” issue).

### Step A3: Force contention intentionally (prove your scope is telling the truth)

On the Atom (still `pin 2`):

- `mode tx`
- `tx high` (Atom drives high)
- while peer is transmitting “low” start bits, observe whether the measured low becomes mid-level (~1.5V)

Then:

- `tx low` (Atom drives low)

This gives you a calibration:

- If forcing TX high produces the exact “~1.5V low” symptom you saw earlier, you have strong evidence that the earlier symptom was **TX↔TX contention** (or at least “something is driving high when it shouldn’t”).

## Workflow B: Pin mapping / cable swap discovery (the 2×2 matrix)

This answers: “Are G1/G2 swapped across devices/cable/labels?”

### Step B1: Generate a visible square wave on Atom GPIO1 or GPIO2

On Atom:

- `pin 1`
- `mode tx`
- `tx square 1000`

Probe both GROVE pins on the peer side and record where the 1 kHz wave appears.

Repeat with:

- `pin 2` → `tx square 1000`

### The matrix to fill

Record “where the signal appears” as a truth table:

```text
Atom drives:      Peer observes square wave on:
TX GPIO1 (G1)  ->  ? (peer G1?)  ? (peer G2?)
TX GPIO2 (G2)  ->  ? (peer G1?)  ? (peer G2?)
```

**Interpretation:**

- If the wave always appears on the same labeled pin, mapping is consistent.
- If TX on Atom GPIO1 appears on peer’s “G2”, your cable/labeling is swapped somewhere.
- If neither appears: wiring/ground is wrong, or you’re probing the wrong physical points.

## Workflow C: “Does Atom observe RX edges?” (GPIO ISR counter)

This is the “software sees electrical activity” proof at the GPIO layer.

### Step C1: Configure RX edge counting on the pin you believe is RX

On Atom:

- `pin 2`
- `mode rx`
- `rx edges both`
- `rx pull up` (UART idle is high; pull-up helps if the line floats)
- `rx reset`
- `status` (baseline)

Now, make the peer transmit (UART bytes, or just a toggling signal).

Run:

- `status` repeatedly

You’re looking at:

- `rx_edges_total`, `rises`, `falls`

**Interpretation:**

- If counts increase: the Atom is physically seeing edges on that pin; the wiring/pin mapping is likely correct.
- If counts stay at 0 while the scope clearly shows transitions at the Atom header: something is wrong on the Atom side (pin not really the one you think, or not configured as input as expected).

## Common “gotchas” this tool helps you rule in/out

- **You’re typing into the wrong console**: `0016` uses USB Serial/JTAG for the REPL. GROVE pins are not used for stdin/stdout here; that’s deliberate.
- **The pin is not high‑Z when you think it is**: `mode idle` is your truth baseline; if the peer TX waveform is still distorted then, look for external contention.
- **The cable isn’t pin-to-pin**: the square-wave mapping matrix makes this unambiguous.
- **Pull-ups are being mistaken for contention**: internal pulls usually don’t prevent a real UART TX from pulling low; mid-level lows usually mean “two drivers”.

## What I would look for next (if GPIO-level tests pass)

If the matrix and edge counters prove the signal is present and clean on the right pin, but `esp_console` over GROVE UART still won’t accept input, then the next step is **UART-peripheral-level testing**, not more guessing:

- Add (or use) a “UART echo” mode (configure UART1 RX/TX on GPIO1/2 and echo bytes) to isolate:
  - UART peripheral routing
  - driver install
  - line endings
  - REPL expectations

That’s explicitly Milestone E in `tasks.md` and is only worth doing after this playbook yields “GPIO layer is clean”.


