---
Title: Test matrix (cable mapping + contention diagnosis)
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
      Note: Command list used to execute the matrix (`mode`, `pin`, `tx`, `rx`, `status`)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: Defines the exact electrical meaning of `mode idle` and how mode changes reset pins
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/playbook/02-debug-grove-uart-using-0016-signal-tester.md
      Note: Narrative procedure version of this matrix (more context, less table)
ExternalSources: []
Summary: "Copy/paste test matrices to answer (1) are G1/G2 swapped? and (2) is the RX low-level lifted by contention/bias? using the AtomS3R 0016 signal tester."
LastUpdated: 2025-12-25T22:38:17.129449894-05:00
WhatFor: "Make scope/logic-analyzer sessions systematic: run a small finite set of tests and record results instead of iterating randomly."
WhenToUse: "When GROVE UART seems dead or signals look wrong; before touching UART-peripheral or esp_console-over-UART assumptions."
---

# Test matrix (cable mapping + contention diagnosis)

## Goal

Provide a small set of **repeatable tests** (with a recordable matrix) that turns “maybe swapped” / “maybe contention” into concrete evidence.

## Context

- This matrix is designed to be executed with AtomS3R running `0016-atoms3r-grove-gpio-signal-tester`.
- Treat the peer as external (Cardputer, USB↔UART adapter, function generator, etc.).
- The most important baseline is `mode idle`, which resets **both** GPIO1 and GPIO2 to input with pulls/interrupts off.

## Quick Reference

## Matrix A — Cable / label mapping (G1/G2 swap discovery)

### Setup

- Probe the peer device side (or the cable end) so you can see which labeled pin has the square wave.
- Use an easy frequency like 1 kHz (visible on scope and logic analyzer).

### Run

```text
sig> pin 1
sig> mode tx
sig> tx square 1000
```

Record where the wave appears on the peer: **peer G1**, **peer G2**, both, or neither.

Repeat:

```text
sig> pin 2
sig> tx square 1000
```

### Record sheet

Fill this in:

```text
Atom drives:      Peer observes square wave on:
TX GPIO1 (G1)  ->  [ ] peer G1   [ ] peer G2   [ ] neither
TX GPIO2 (G2)  ->  [ ] peer G1   [ ] peer G2   [ ] neither
```

### Interpretation

- **Expected for pin-to-pin, labels consistent**: GPIO1 shows on peer G1, GPIO2 shows on peer G2.
- **Swapped somewhere**: GPIO1 shows on peer G2 and/or GPIO2 shows on peer G1.
- **Neither**: wiring/ground/probe point is wrong, or the peer side is not what you think you’re probing.

## Matrix B — Contention / bias diagnosis (“low lifts to ~1.5V”)

This directly targets the field symptom: peer TX low no longer reaches ~0V when connected to AtomS3R.

### Setup

- Ensure the peer is transmitting a UART waveform (or any clean toggling signal) on the wire you believe is RX.
- Probe voltage at the AtomS3R side (or as close as possible to the Atom’s GROVE pins).

### Run (one pin at a time)

Pick the suspect wire:

```text
sig> pin 2
```

Now run these configurations and observe low-level voltage:

| Atom config | Commands | Expected if line is healthy |
|---|---|---|
| True baseline (Hi‑Z + no ISR) | `mode idle` | peer waveform should look like unloaded (0–3.3V) |
| Hi‑Z input *with* ISR | `mode rx` + `rx pull none` | peer waveform should still reach ~0V on lows |
| Pulled input (UP) | `rx pull up` | idle goes high; TX lows should still reach ~0V if peer drive is strong |
| Pulled input (DOWN) | `rx pull down` | idle goes low; highs should still reach ~3.3V if peer drive is strong |
| Forced contention (drive high) | `mode tx` + `tx high` | peer lows may lift (reproduce ~1.5V symptom if contention is the cause) |
| Forced contention (drive low) | `tx low` | peer highs may sag (confirms two drivers are fighting) |

### Interpretation

- If the waveform is distorted even in `mode idle`, the bias is likely external (wiring/cable/peer/board circuitry).
- If the waveform is clean in `mode idle` but distorts in `mode tx` or `mode rx`, you have strong evidence the Atom configuration is influencing the line.

## Matrix C — “Does Atom observe edges?” (GPIO ISR counter)

This validates *software sees electrical activity* at the GPIO layer.

```text
sig> pin 2
sig> mode rx
sig> rx edges both
sig> rx pull up
sig> rx reset
sig> status
```

While the peer is transmitting, run `status` repeatedly and confirm `rx_edges_total` increases.

## Usage Examples

### Minimal record template for a scope session

Copy this into a notes file and fill it live:

```text
DATE:
SETUP:
- AtomS3R firmware: 0016
- Peer device:
- Cable:
- Probe points:

Matrix A results:
- TX GPIO1 (G1) observed on: ...
- TX GPIO2 (G2) observed on: ...

Matrix B results (pin 1):
- idle low: ...
- rx pull none low: ...
- tx high low: ...

Matrix B results (pin 2):
- idle low: ...
- rx pull none low: ...
- tx high low: ...

Matrix C results:
- rx_edges_total increments? ...
```

## Related

- Ticket spec: `../analysis/01-spec-grove-gpio-signal-tester.md`
- Wiring/pin mapping: `03-wiring-pin-mapping-atoms3r-cardputer-grove-g1-g2.md`
- CLI contract: `02-cli-contract-0016-signal-tester.md`
- Operational playbook: `../playbook/02-debug-grove-uart-using-0016-signal-tester.md`
