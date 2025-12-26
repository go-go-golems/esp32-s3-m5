---
Title: "Spec: GROVE GPIO1/GPIO2 signal tester (USB Serial/JTAG esp_console control + LCD UI)"
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - cardputer
    - gpio
    - uart
    - usb-serial-jtag
    - debugging
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "A deterministic test harness to generate and observe signals on GROVE GPIO1/GPIO2, controlled via esp_console over USB Serial/JTAG and showing live status on the device LCD."
LastUpdated: 2025-12-25T00:00:00.000000000+00:00
WhatFor: "Precisely diagnose whether GROVE TX/RX failures are due to wiring/pin mapping, electrical reachability, UART pin routing, or REPL-level issues."
WhenToUse: "When a UART transmitter is confirmed on a scope but the receiver reports no activity, or when the GROVE labels G1/G2 are suspect across devices."
---

## Executive summary

We will build a small firmware “instrument” for **AtomS3R** that can act as either a **signal source** (TX) or **signal sink** (RX) on two specific pins: **GPIO1 and GPIO2** (the pins typically exposed as GROVE `G1` and `G2`).

The firmware has two user-facing interfaces:

- **Control plane**: an interactive `esp_console` REPL over **USB Serial/JTAG** (stable, easy to use, doesn’t depend on the GROVE UART we’re debugging).
- **Status plane**: a minimal **LCD text UI** showing the current mode (TX/RX), selected pin, output pattern or RX stats, and any warnings.

The result is a tool you can flash onto the AtomS3R and use to answer, in minutes:

- “Is the signal actually present at the receiving header?”
- “Are G1/G2 swapped between these boards or the cable?”
- “Is the issue electrical (no edges) or software binding (edges exist but UART doesn’t parse)?”

## Motivation (why we need a dedicated tool)

Our current investigation mixes layers:

- We can confirm Cardputer TX on a scope (electrical).
- AtomS3R application reports “no RX edges” or “no input”.
- `esp_console` adds additional assumptions (line endings, echo, task scheduling).

When the observed symptom is “no input”, it’s easy to spend hours changing UART numbers, console transports, or line endings without realizing the signal never reached the RX pin in the first place (or reached the wrong pin).

This tool is designed to break that loop by producing binary answers at the GPIO layer.

## Key field observation (scope) and what it implies

During the investigation we observed a very strong electrical hint:

- **Unloaded Cardputer TX**: UART waveform is normal **0–3.3V**.
- **Cardputer TX connected to AtomS3R**: the same waveform becomes approximately **1.5V–3.3V** (the “low” level no longer reaches ~0V).

This observation matters because a UART start bit *requires* a real low level. A “low” stuck around ~1.5V can be interpreted as neither a valid low nor a clean edge by the receiver, and it strongly suggests the issue is **below** the REPL layer.

### Most likely interpretations (hypotheses)

The tool we’re building should help confirm which of these is true:

- **TX↔TX contention (very likely)**:
  - Cardputer TX is connected (directly or effectively) to an AtomS3R pin that is being driven high (e.g., AtomS3R TX),
    so when Cardputer drives low, the two outputs fight and the line settles at an intermediate voltage.
  - This aligns well with “low rises to ~1.5V only when the other board is connected”.

- **Strong pull-up / external circuitry on the AtomS3R pin**:
  - The AtomS3R-side pin might have an external pull-up or other circuitry connected (not a pure high‑Z input),
    so the Cardputer TX can’t pull low enough.
  - Internal ESP32 pull-ups are typically weak; an intermediate low more often points to *stronger* bias or contention.

- **Ground/reference problem**:
  - If the reference/ground path is not solid, the apparent “low level” can float.
  - This is less likely when the waveform timing looks clean, but should still be tested explicitly.

- **Wrong pin mapping (G1/G2 swapped)**:
  - The signal might be physically reaching the GROVE connector, but landing on a different GPIO than assumed,
    so “RX on GPIO2” is simply the wrong pin on the AtomS3R side.

### What the signal tester must support because of this

Given this symptom, a “good” v1 tool must include:

- a **true high‑impedance (Hi‑Z) input mode** (pulls off) so we can see if the peer TX returns to 0–3.3V when AtomS3R stops influencing the line
- a **strong-drive test mode** (drive low, drive high) so we can detect if the pin is being fought
- a **pin-swap workflow** (switch between GPIO1 and GPIO2 quickly, with clear on-device UI) to diagnose G1/G2 ambiguity

## Acceptance criteria (current symptom captured explicitly)

The v1 signal tester is “good enough” when it can make the following symptom **deterministic and explainable**:

- **Unloaded peer TX**: waveform is normal ~0–3.3V.
- **Peer TX connected to AtomS3R**: low level lifts to ~**1.5V** (suspected contention/bias).

To diagnose this deterministically, the tester must provide the following electrical states (without reflashing):

- **Hi‑Z baseline**: `mode idle` (both pins input, pulls off, interrupts off)
- **Pulled input**: `mode rx` + `rx pull none|up|down`
- **Driven output**: `mode tx` + `tx high|low` (and patterns like square/pulse)

## Scope

### In scope

- GPIO-level TX/RX tests for **GPIO1** and **GPIO2**.
- A small REPL for changing modes at runtime.
- Live status on LCD.
- A repeatable “pin mapping discovery” workflow across AtomS3R and a peer device/cable (often Cardputer, but not limited to it).

### Out of scope (for v1)

- Full protocol decoding (UART byte capture, framing analysis).
- High-resolution logic analyzer features.
- Automated host-side tooling (optional later).

## System overview (high-level architecture)

The firmware is structured as:

```text
USB Serial/JTAG
   |
   v
esp_console REPL  --->  Control State (mode, pin, pattern, pulls)
   |
   +--> apply_config(): config GPIO direction/pulls/peripheral setup
   |
LCD UI task  <---  read Control State + RX Stats (counters, timestamps)
   |
   v
Draw text: mode, pin, freq, edges, warnings

GPIO ISR (RX edge counter)  --->  RX Stats (atomic counters)

TX generator task (optional) ---> toggles GPIO at requested freq/pattern
```

This separation matters:

- REPL is the only writer of “what should we do?”
- GPIO ISR and TX generator do minimal work and update stats
- LCD task is pure presentation

## User stories

- **As a developer**, I can set “TX square wave on GPIO2 at 1 kHz” and verify on the scope where that signal appears on the other board/cable.
- **As a developer**, I can set “RX edge counter on GPIO2” and see whether the other board’s TX reaches this pin.
- **As a developer**, I can swap pins quickly (`pin 1` → `pin 2`) without reflashing.
- **As a developer**, I can see a clear on-device indicator of “current mode” so I don’t confuse which board is acting as TX vs RX.

## CLI (esp_console) command set (proposed)

We’ll keep commands boring and explicit.

Note: the authoritative “stable contract” lives in `reference/02-cli-contract-0016-signal-tester.md`. This section is a readable summary; update both if the CLI surface changes.

### Core commands

- `mode tx|rx|idle`
- `pin 1|2` (maps to GPIO1/GPIO2)
- `status` (print current mode/pin/pattern and rx stats)
- `help`

### TX commands

- `tx high` / `tx low`
- `tx square <hz>` (e.g., `tx square 1000`)
- `tx pulse <width_us> <period_ms>` (useful for triggers)
- `tx stop`

### RX commands

- `rx edges rising|falling|both`
- `rx pull none|up|down`
- `rx reset`
- `rx stats` (edges, delta/sec, last timestamp)

### UART (optional, later)

- `uart enable <uart_num> <baud> <tx_gpio> <rx_gpio>`
- `uart echo on|off`

## LCD UI (spec)

The LCD should render a single screen of text, refreshed at ~5–10 Hz:

- **Header**: board name + build id
- **Mode line**: `MODE: TX` or `MODE: RX`
- **Pin line**: `PIN: GPIO2 (G2)` (and also show “logical name” G1/G2 if we decide)
- **TX status**:
  - `TX: square 1000 Hz` or `TX: HIGH`
- **RX status**:
  - `RX: edges=12345 (+67/s) last_us=... pull=UP`
- **Warning line** (only when relevant):
  - `WARN: RX edges=0 while peer TX active -> wiring/pin mismatch?`

## Implementation approach (no code yet; design choices)

### Timing sources

We have two ways to generate TX patterns:

- **Software toggle task** (simplest; adequate for “is the pin mapping correct?”)
- **RMT-based output** (more precise; optional if needed)

For v1, software toggle is enough because we care about reachability and mapping, not perfect duty cycle.

### RX edge counting

Use a GPIO ISR and atomic counters:

- increment counters on ISR
- record last edge time using `esp_timer_get_time()` (careful in ISR; if needed, only store a coarse timestamp)

### Avoiding measurement interference

We explicitly avoid using the same pins for:

- keyboard scanning (Cardputer has known GPIO1/GPIO2 conflicts in some revisions)
- UART console (we will use USB Serial/JTAG for control plane)

## “Why not just use esp_console on GROVE UART?”

Because it conflates the debugging tool with the thing being debugged.

If GROVE RX is broken or swapped, binding the REPL to GROVE makes the tool unusable. USB Serial/JTAG gives us a stable control plane so we can change GPIO mode/pin live, even if GROVE wiring is wrong.

## Test plan (what success looks like)

### Test 1: cable mapping (pin-to-pin sanity)

1. Flash Board A: set `TX square 1000 Hz` on GPIO2.
2. Flash Board B: set `RX` on GPIO2, count edges.
3. Expected:
   - If GROVE is pin-to-pin and mapping is consistent: edges increase.
   - If edges stay 0: try RX on GPIO1 (pin swap detection).

### Test 2: swapped label detection

Repeat Test 1 but systematically permute:

- TX on GPIO1 vs GPIO2
- RX on GPIO1 vs GPIO2

Record the matrix:

```text
          Board B RX
         GPIO1   GPIO2
TX GPIO1   ?       ?
TX GPIO2   ?       ?
```

Exactly one cell “should” show strong edge counts if the cable and labels are consistent.

Reference: `reference/04-test-matrix-cable-mapping-contention-diagnosis.md` (copy/paste-ready recording sheet).

### Test 3: receiver-only electrical reachability

If TX is visible on Board A scope but not visible on Board B header:

- the root cause is physical (cable/pin/ground), not software.

### Test 4: detect contention vs. high‑Z input

This test is directly motivated by the ~1.5V low-level symptom.

1. Peer device transmits UART on its TX pin (confirmed 0–3.3V when unloaded).
2. AtomS3R sets the corresponding pin to **Hi‑Z input** (no pull).
3. Observe on scope at AtomS3R pin:
   - if the low level returns to ~0V: AtomS3R was previously biasing/driving the line
   - if low remains ~1.5V: the bias is elsewhere (cable, peer output strength, external circuitry)

We then repeat the same test with AtomS3R configured as:

- input pull-up
- input pull-down
- output drive-high
- output drive-low

The patterns of resulting voltages identify whether we’re dealing with contention, pull-up bias, or grounding/reference issues.

## Risks and mitigations

- **Risk: Cardputer GPIO1/GPIO2 are used by keyboard autodetect on some revisions**
  - Mitigation: add Kconfig option to disable that feature in the signal tester build, or avoid those pins for keyboard scanning entirely.

- **Risk: ISR/logging can cause stack/latency issues**
  - Mitigation: avoid logging in ISRs; log periodic summaries from a dedicated task with sufficient stack.

## Next steps

Implementation is not part of this ticket creation step.

The next actionable items are in `tasks.md`.


