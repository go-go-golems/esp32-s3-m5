---
Title: JS API and Runtime Brainstorm
Ticket: MO-01-JS-GPIO-EXERCIZER
Status: active
Topics:
    - esp32s3
    - cardputer
    - gpio
    - serial
    - console
    - esp-idf
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/ESP32_MicroQuickJS_Playbook.md
      Note: MicroQuickJS constraints and REPL background
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/analysis/01-existing-firmware-analysis-and-reuse-map.md
      Note: Source of reuse findings and constraints
ExternalSources: []
Summary: Brainstormed JS APIs and runtime architecture for a MicroQuickJS-driven GPIO/protocol exercizer with real-time safety.
LastUpdated: 2026-01-11T18:35:26.715567162-05:00
WhatFor: Explore API shapes and execution models before implementing the Cardputer-ADV MicroJS GPIO exercizer.
WhenToUse: Use when defining JS bindings and task architecture for GPIO/UART/I2C signal generation.
---


# JS API and Runtime Brainstorm

## Executive Summary

We need a MicroQuickJS-based REPL that can accept JS snippets and generate reliable GPIO, UART, and I2C traffic. Real-time signal generation must live in dedicated firmware tasks; JS should only enqueue or configure signals. This doc proposes several JS API shapes and runtime architectures, ranks them against real-time safety, and sketches a hybrid approach that combines a command queue, hardware signal engines, and optional protocol simulators.

## Problem Statement

The firmware must let a user send JS over REPL to generate and schedule signals on Cardputer-ADV GPIOs. It must be accurate enough to validate logic-analyzer decoding and support multiple protocols (UART, I2C, PWM-style sequences). JS execution is not real-time and MicroQuickJS is single-threaded, so the runtime must avoid blocking and should not rely on JS to toggle pins at microsecond granularity.

## Constraints and assumptions

- MicroQuickJS runs in a single task; no cross-task JS calls.
- Precise timing must be handled by hardware peripherals or dedicated tasks (RMT, LEDC, esp_timer, or gptimer).
- USB Serial/JTAG is the default REPL transport for Cardputer-ADV in this repo.
- We should be able to run in a "dry-run" (simulate) mode to validate logic analyzer configs without toggling pins.

## Design process (how I brainstormed)

- Start from existing REPL and GPIO signal patterns in `imports/esp32-mqjs-repl` and `0016-atoms3r-grove-gpio-signal-tester`.
- Identify the smallest JS interface that can express: GPIO toggles, PWM-ish waves, UART payloads, I2C transactions, and scheduling.
- Evaluate architectures by real-time accuracy, JS ergonomics, and implementation complexity.
- Prefer architectures that do not require JS to run on tight timing loops.

## Brainstormed JS APIs (options + thoughts)

### Option A: Imperative low-level API

**Sketch**

```js
gpio.mode(1, "out");
gpio.write(1, 1);
gpio.square(1, { hz: 1000, duty: 0.5 });
gpio.pulse(1, { widthUs: 50, periodMs: 5 });

uart.open({ port: 1, tx: 2, rx: 1, baud: 115200 });
uart.write("Hello\r\n");

i2c.open({ port: 0, sda: 8, scl: 9, hz: 100000 });
i2c.tx(0x44, [0x2C, 0x06]);
let bytes = i2c.rx(0x44, 6);
```

**Pros**
- Simple to learn; mirrors ESP-IDF concepts.
- Easy to map to existing C drivers (`gpio_tx`, `uart_tester`, `i2c_master`).

**Cons**
- Hard to express multi-step waveforms or timed sequences without loops in JS.
- Tempts users to do timing in JS (bad for accuracy).

**Thoughts**
- This should exist as the "primitive" layer, but not the only interface.

### Option B: Declarative waveform/timeline API

**Sketch**

```js
const wave = Signal.timeline("gpio", 1)
  .atUs(0, 1)
  .atUs(50, 0)
  .atUs(200, 1)
  .repeat(1000);

wave.start();
```

**Pros**
- Explicit timing model; easy to offload to RMT/LEDC or timer task.
- Good fit for logic-analyzer validation patterns.

**Cons**
- More complex to implement; needs a compiler from JS to C-side signal plans.
- Memory overhead if sequences are large.

**Thoughts**
- This is the most accurate approach if we can map to RMT items or timer queues.

### Option C: Protocol-focused API modules

**Sketch**

```js
uart.sendFrame({ baud: 115200, data: [0x55, 0xAA] });

i2c.transaction({
  addr: 0x44,
  write: [0x2C, 0x06],
  read: 6,
});

pwm.sweep(1, { startHz: 500, endHz: 2000, stepHz: 50, holdMs: 50 });
```

**Pros**
- Easier for the user to ask for "a protocol" instead of a waveform.
- Allows the firmware to choose best peripheral (RMT/LEDC/gptimer).

**Cons**
- Less flexible for arbitrary edge cases or raw waveforms.
- Larger API surface and more bindings.

**Thoughts**
- Pair with the low-level API; focus on a few core protocols first (UART, I2C, PWM, raw GPIO).

### Option D: Task-based scripting API with scheduler

**Sketch**

```js
let task = Task.start(() => {
  gpio.write(1, 1);
  Task.sleepUs(50);
  gpio.write(1, 0);
});

Task.interval(1000, () => uart.write("tick"));
```

**Pros**
- Familiar to JS users; can express loops and delays.

**Cons**
- Dangerous for real-time accuracy: JS sleep and scheduling jitter will dominate.
- Hard to keep consistent with MicroQuickJS single-task constraint.

**Thoughts**
- Avoid except for coarse scheduling; use to trigger hardware tasks, not for timing loops.

## Runtime execution architecture (options + weighting)

### Criteria + weights

- Real-time accuracy (weight 5)
- JS ergonomics (weight 3)
- Implementation complexity (weight 3)
- Testability / observability (weight 2)

### Scoring (1-5, higher is better)

| Architecture | Accuracy | JS ergonomics | Complexity | Testability | Weighted score |
| --- | --- | --- | --- | --- | --- |
| A) JS does timing loops | 1 | 4 | 5 | 2 | 1*5 + 4*3 + 5*3 + 2*2 = 34 |
| B) JS compiles plan -> signal engine task | 5 | 4 | 3 | 4 | 5*5 + 4*3 + 3*3 + 4*2 = 50 |
| C) Dedicated protocol tasks (UART/I2C/PWM) | 4 | 4 | 4 | 4 | 4*5 + 4*3 + 4*3 + 4*2 = 48 |
| D) RMT-only waveform engine | 5 | 2 | 2 | 3 | 5*5 + 2*3 + 2*3 + 3*2 = 40 |

**Takeaway**: The best fit is a hybrid of B + C: JS builds a plan and submits it to dedicated signal engines per protocol.

## Proposed Solution (hybrid)

- **Core**: MicroQuickJS REPL task enqueues commands onto a control queue (reuse 0016 pattern).
- **Signal engines**: Dedicated tasks for GPIO waveforms, UART, and I2C. Each engine consumes a queue of "plans" generated by JS.
- **Timing**: GPIO waveforms use esp_timer for low-frequency signals and RMT/LEDC for precise high-frequency output.
- **Simulation**: A global `Signal.simulate(true)` toggle routes plans to a log sink instead of hardware.

### Architecture diagram

```text
[USB Serial/JTAG]
      |
      v
[MicroQuickJS REPL Task]
      |
      v
[Command Queue]
  |       |       |
  v       v       v
GPIO Engine  UART Engine  I2C Engine
  |           |           |
GPIO/RMT   UART driver   I2C master
```

## Design Decisions (tentative)

- Use a single JS runtime task; no JS calls from other tasks.
- Provide a low-level API plus a small set of protocol helpers.
- Prefer queue-based control planes over direct function calls.
- Add a simulation mode to validate decoder setups without touching GPIO.

## Alternatives Considered

- Pure JS timing loops (rejected: too much jitter).
- RMT-only waveform output (rejected: not enough for UART/I2C control).
- Full async JS runtime with promises (not supported by MicroQuickJS ES5.1).

## Implementation Plan (draft)

1. Reuse the MicroQuickJS REPL base (`imports/esp32-mqjs-repl`) and control-plane queue pattern (`0016`).
2. Define JS bindings for `gpio`, `uart`, and `i2c` primitives.
3. Implement GPIO engine: esp_timer for slow patterns, RMT/LEDC for precise PWM.
4. Implement UART engine: configure pins, enqueue payloads, optional periodic send.
5. Implement I2C engine: enqueue transactions to an I2C worker task.
6. Add `Signal.simulate()` and a log sink to mirror outbound plans.
7. Add status queries and stats to the REPL (`:stats` extension or `signal.status()`).

## Open questions

- Which protocols beyond UART/I2C/PWM matter most for logic analyzer validation (SPI? 1-wire? I2S)?
- Do we need a persistent JS script store (SPIFFS autoload) on day one?
- How much waveform resolution is required before RMT becomes mandatory?
