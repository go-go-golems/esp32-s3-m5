---
Title: High-Frequency GPIO Pattern VM Brainstorm
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
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
      Note: Existing timer-based waveform generator for baseline patterns
    - Path: esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_console.c
      Note: Pattern sequencing model for on/off timing
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/analysis/01-existing-firmware-analysis-and-reuse-map.md
      Note: Prior reuse map used to ground the VM design
ExternalSources: []
Summary: Brainstorm and design notes for a small VM that encodes high-speed GPIO patterns and bus waveforms.
LastUpdated: 2026-01-11T19:07:30.144150608-05:00
WhatFor: Explore a VM-based approach for precise, programmable GPIO waveforms beyond JS timing limits.
WhenToUse: Use when specifying high-frequency output and protocol emulation features for the JS GPIO exercizer.
---


# High-Frequency GPIO Pattern VM Brainstorm

## Executive Summary

We need a way to emit high-speed, deterministic GPIO patterns that JS cannot time accurately. The proposed solution is a small, purpose-built VM that executes a compact instruction stream describing pin transitions, delays, and loops. The VM should target multiple backends (RMT, gptimer + ISR, or tight CPU loop) and be driven by JS as a configuration language, not as the timing engine.

This document includes a panel-style debate among experts and a recommended hybrid design: a minimal instruction set plus two execution backends (RMT for high precision, timer-driven for lower rates), with a shared assembler and simulator.

## Problem Statement

MicroQuickJS is not suitable for microsecond-level timing loops. To test logic analyzer decoders and emulate simple bus protocols, we need deterministic waveforms with sub-microsecond resolution and repeatability. A tiny VM with a fixed timing base can encode complex signal sequences and run independently of JS execution jitter.

## Proposed Solution

Create a high-frequency GPIO pattern VM with:

- A compact instruction set to express pin changes, delays, loops, and I/O reads.
- A fixed timing base in ticks (e.g., 12.5 ns, 25 ns, 50 ns) depending on backend.
- Two execution backends:
  - **RMT backend**: compile instruction stream into RMT items for precise waveforms.
  - **Timer backend**: use gptimer ISR to advance VM for lower-rate patterns.
- A JS-facing API that assembles and submits programs, but does not perform timing.

### Minimal instruction set (draft)

```
NOP
SET mask, value        ; apply new pin levels for mask
WAIT ticks             ; delay for N ticks
PULSE mask, value, ticks ; convenience: set -> wait -> clear
JMP addr
JNZ reg, addr          ; loop counter
LOAD reg, imm
STORE reg, addr
HALT
```

### Example: I2C-like waveform (conceptual)

```
; SDA on bit0, SCL on bit1
LOAD R0, 8           ; byte length
LOOP:
  SET 0b11, 0b11     ; idle high
  WAIT 10
  SET 0b10, 0b10     ; SDA low, SCL high (start)
  WAIT 10
  ...                ; shift bits by toggling SCL
  JNZ R0, LOOP
HALT
```

### JS API sketch

```js
const vm = PatternVM({ pins: ["GPIO4", "GPIO5"], tickNs: 50 });
vm.emit("SET", 0b11, 0b11);
vm.emit("WAIT", 10);
vm.emit("PULSE", 0b01, 1, 4);
vm.loop("R0", 8, () => { /* bit loop */ });
vm.submit({ backend: "rmt", repeat: true });
```

## Expert Panel Debate (design rationale)

**Panel**
- **Avery (Real-time systems)**: Focused on determinism and ISR safety
- **Ben (ESP-IDF peripherals)**: Focused on RMT, gptimer, and hardware constraints
- **Chi (VM/Compiler designer)**: Focused on instruction set and code density
- **Dee (Protocol engineer)**: Focused on bus emulation use cases
- **Eli (Test/QA)**: Focused on observability and verification

### Debate excerpts (condensed)

**Avery**: "We must keep the VM deterministic. Any heap allocation or JS callback during execution is a non-starter. The VM should run in IRAM or a dedicated task, with precomputed timing."

**Ben**: "RMT already solves the high-precision timing problem. The VM should compile to RMT items for the fast path. A timer-driven VM is still useful for slower patterns or when RMT channels are unavailable."

**Chi**: "The instruction set should be minimal. Keep a small number of opcodes with fixed-size encoding to simplify compilation from JS. Loops are essential for bus patterns."

**Dee**: "We need to express start/stop conditions, clock edges, and inter-bit delays. A simple SET+WAIT model can do it, but convenience macros (PULSE) will reduce program size."

**Eli**: "We need a simulator that can dump a timeline for logic analyzer validation. Without visibility into the waveform, debugging will be painful."

### Points of contention

- **RMT-only vs dual backend**
  - RMT-only is precise but less flexible (channel limits, item count limits).
  - Dual backend adds complexity but provides a fallback.

- **Instruction richness**
  - Rich instructions reduce program length but increase implementation complexity.
  - Minimal instructions keep the VM simple but may bloat programs.

### Consensus summary

- Use a minimal instruction set with a macro layer that expands into core ops.
- Support both RMT compile and timer-driven execution.
- Provide a simulator for debugging and for generating test waveforms.

## Design Decisions

- **Minimal core opcodes**: implement SET, WAIT, JMP, JNZ, LOAD, HALT; add macros like PULSE in the assembler layer.
- **Fixed tick duration**: VM operates on ticks; backend chooses tick resolution.
- **No dynamic memory in execution**: program buffer is fixed and built before run.
- **Two backends**: RMT for high frequency, gptimer ISR for moderate patterns.
- **JS as control plane**: JS only assembles and submits, never times.

## Alternatives Considered

- **Pure RMT item builder (no VM)**: simplest but hard to express loops and conditional patterns.
- **CPU busy-loop toggling**: precise only at high CPU cost, sensitive to interrupts.
- **LEDC-only PWM**: limited to PWM patterns, not arbitrary bus traffic.

## Implementation Plan

1) Define VM instruction encoding and assembler API (C + JS bindings).
2) Implement a simulator that outputs timestamps and pin states.
3) Implement RMT backend: compile VM program into RMT items.
4) Implement gptimer backend: VM executes on each tick ISR.
5) Add JS bindings to build and submit programs.
6) Create examples for UART-like, I2C-like, and custom pulse trains.

## Open Questions

- What is the maximum program length we need (item count, loops)?
- Do we need input sampling instructions (READ pin) for bidirectional protocols?
- Should we add a "trace" buffer to capture actual output timing for validation?

## How to use the VM (draft usage guidance)

- Build a program from JS or C (assembler layer).
- Choose backend and tick resolution based on target frequency.
- Submit once, optionally loop or repeat.
- Use simulator output to validate logic analyzer decoding before hardware.
