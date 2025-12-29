---
Title: Why does tx low on GPIO2 create a spike on GPIO1+GPIO2? (0016 signal tester + hardware coupling)
Ticket: 0013-GROVE-SERIAL-DEBUG
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - gpio
    - uart
    - debugging
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-26T17:01:31.131517666-05:00
WhatFor: "Explain why a small transient can appear on both GPIO1 and GPIO2 when commanding `tx low` on GPIO2 in the 0016 signal tester; provide ranked hypotheses and quick experiments to confirm."
WhenToUse: "Use when interpreting scope/logic-analyzer traces during GROVE UART debugging and you see unexpected spikes/edges that may be caused by firmware reconfiguration, physical coupling, or measurement artifacts."
---

## Observation (what we’re explaining)

Reported behavior while running `0016-atoms3r-grove-gpio-signal-tester` on AtomS3R:

```text
sig> pin 2
sig> mode tx
sig> tx low
```

Even though the active TX pin is GPIO2, a small transient “spike” is observed on **both GPIO2 and GPIO1**.

This document explains why that is plausible (and, in fact, expected) given the current tester semantics and typical wiring/probing physics.

## Key firmware facts (high-signal)

### Fact A: every apply resets BOTH pins (even if only one is “active”)

In `0016`, command handling flows:

- `manual_repl.cpp`: parses `tx low` → sends `CtrlType::TxLow`
- `signal_state.cpp`: `tester_state_apply_event()` sets `mode=TX`, `tx.mode=LOW` then calls `apply_config_snapshot()`

Critically, `apply_config_snapshot()` does:

- `gpio_tx_stop(); gpio_rx_disable();`
- `safe_reset_pin(GPIO_NUM_1); safe_reset_pin(GPIO_NUM_2);`
- configure only the active pin for TX/RX

So even if you “only drive GPIO2”, GPIO1 is being actively reconfigured into a floating/safe baseline on that apply.

### Fact A.1: your log shows the exact reset sequence (GPIO1 once, GPIO2 twice)

Your `tx low` output:

```text
sig> I (470296) atoms3r_gpio_sig_tester: tx stopped: gpio=2

I (470296) gpio: GPIO[1]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (470296) gpio: GPIO[2]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (470296) gpio: GPIO[2]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0

I (470296) atoms3r_gpio_sig_tester: tx low: gpio=2
```

This matches the current control flow:

- `apply_config_snapshot()` resets **GPIO1** and **GPIO2** once each (`safe_reset_pin(GPIO_NUM_1)` and `safe_reset_pin(GPIO_NUM_2)`).
- Then `gpio_tx_apply(GPIO2, ...)` calls `gpio_reset_pin(GPIO2)` again (so GPIO2 appears a second time).

So a single `tx low` is not “just set level low”; it includes a stop + full reapply and it touches both wires.

### Fact B: TX apply enables output direction before preloading output level

In `main/gpio_tx.cpp`, `gpio_tx_apply()` currently sequences:

- `gpio_reset_pin(pin);`
- `gpio_set_direction(pin, GPIO_MODE_OUTPUT);`
- then later, if mode is LOW: `gpio_set_level(pin, 0);`

This ordering can create a short “output enable” transient if the pad’s latched output value is not already the desired value at the moment output is enabled.

### Fact C: `gpio_reset_pin()` appears to enable internal pull-up briefly (visible in the dump)

In the log lines above, `Pullup: 1` is shown immediately after reset. Even if later code disables pulls, that brief pull-up enable (plus output-enable transitions) can produce a small transient on the active wire and a coupled transient on the adjacent wire.

## Why you can see a spike on BOTH GPIO1 and GPIO2 (ranked hypotheses)

### 1) Output-enable glitch on the active pin (GPIO2) due to sequencing (most likely)

If the output latch was previously high (e.g., you were running `tx high` or `tx square` earlier), then switching the pin to output may briefly drive high before the subsequent `gpio_set_level(..., 0)` takes effect. That shows up as a narrow pulse/spike on GPIO2.

Even if the latch is “usually 0”, the safest deterministic pattern is:

- write output level first (while still input),
- then enable output direction.

### 2) Capacitive coupling onto the inactive wire (GPIO1) because it is floating during/after reset (very likely)

Because `apply_config_snapshot()` resets GPIO1 to a safe input baseline every time, GPIO1 is typically left as a high-impedance node. A fast edge (even a tiny one) on GPIO2 couples to GPIO1 via:

- adjacency in the GROVE cable (two signal conductors running together),
- parallel PCB traces near the connector,
- input capacitance on the GPIO pad + probe capacitance.

Result: the inactive pin “shows the edge” even though it is not being driven.

### 3) Shared ground bounce / probe artifact (common)

When the active pin changes drive state, the instantaneous current and return path can cause the local ground reference (at the board/probe) to bounce. On a scope, that can look like a simultaneous spike on multiple channels, especially with long ground leads.

Tell-tale sign: both channels show a very similar spike at exactly the same time, and the spike magnitude changes dramatically with probe grounding (spring ground vs long clip lead).

### 4) External circuitry / pull-ups / peer device bias (possible)

AtomS3R’s GROVE pins are also used as “External I2C / Port.A” labels (SCL=GPIO1, SDA=GPIO2). Depending on board revision/add-ons, there may be external pull-ups or attached peripherals that change the RC environment.

Also, if a peer device is connected (Cardputer, USB-UART adapter, etc.), its own pulls/ESD structures can mirror or distort transients.

## Quick experiments to distinguish the causes

### Experiment 1: repeat with nothing connected to GROVE (open cable)

- If GPIO1 still shows a small spike that scales with probe setup, coupling/ground-bounce is dominant.
- If the spike disappears when the neighbor isn’t probed, probe capacitance is part of the story.

### Experiment 2: strap the inactive pin (GPIO1) with a resistor

With GPIO2 active, attach (example) a 10k resistor:

- GPIO1 → GND (or GPIO1 → 3V3)

If the GPIO1 spike collapses significantly, it was mainly capacitive coupling into a floating node (not “real drive”).

### Experiment 3: compare “first tx low after idle” vs “tx low after tx high”

- If the spike is larger when coming from `tx high`/`tx square`, that supports the “output latch / output-enable glitch” hypothesis.

## Mitigation ideas (if we decide the spike is confusing)

These are optional changes to improve determinism/interpretability of the tester as an instrument:

### Mitigation A: preload output level before enabling output direction (recommended)

In `gpio_tx_apply()`:

- disable pulls
- set output latch to the intended level (via `gpio_set_level`)
- then set direction to output

This commonly eliminates the “enable glitch” on many MCUs/SoCs.

Practical `0016`-specific guidance:

- If you want to minimize the “output-enable glitch”, reorder the TX setup to:
  - `gpio_reset_pin(pin)` (if you still want reset semantics),
  - **`gpio_set_level(pin, desired_level)`** (preload latch),
  - then `gpio_set_direction(pin, GPIO_MODE_OUTPUT)`.

### Mitigation B: consider whether full reset of BOTH pins is needed on every apply

The current “always reset both pins” behavior is a feature for contention diagnosis, but it increases the likelihood that the inactive wire is floating and shows coupled transients.

If this becomes problematic, consider:

- only resetting the inactive pin on mode changes or active-pin changes, not on every TX-level change,
- or explicitly biasing the inactive pin in a known state during TX mode (documented), accepting that it’s no longer “pure baseline”.

## References / related work

- Prior ticket diary (context + 0016 semantics): `009-GROVE-GPIO-SIGNAL-TESTER` → `reference/01-diary.md`
- 0016 CLI contract: `009.../reference/02-cli-contract-0016-signal-tester.md` (notes “reset both pins on every apply”)
- 0016 implementation: `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp`, `main/gpio_tx.cpp`, `main/manual_repl.cpp`

External references on “safe GPIO configuration” / glitches:

- Espressif ESP-IDF GPIO docs (ESP32-S3): includes a GPIO glitch filter API (useful for RX edge counting if you want to ignore very short pulses): [`gpio` peripheral API reference](https://docs.espressif.com/projects/esp-idf/en/v5.2.1/esp32s3/api-reference/peripherals/gpio.html)
- STMicroelectronics application note on safe GPIO port configuration (general principle: define level before enabling output): [`AN2710 Safe GPIO port configuration`](https://www.st.com/resource/en/application_note/an2710-safe-gpio-port-configuration-in-str7xx-devices-stmicroelectronics.pdf)

