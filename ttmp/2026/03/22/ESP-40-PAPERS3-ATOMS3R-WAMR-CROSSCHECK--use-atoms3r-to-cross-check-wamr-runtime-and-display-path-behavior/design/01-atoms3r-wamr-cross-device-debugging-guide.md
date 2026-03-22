---
Title: AtomS3R WAMR cross-device debugging guide
Ticket: ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK
Status: active
Topics:
    - papers3
    - atoms3r
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0081-atoms3r-wamr-probe-console/main/app_main.cpp
      Note: Top-level AtomS3R probe wiring
    - Path: 0081-atoms3r-wamr-probe-console/main/atoms3r_canvas.cpp
      Note: AtomS3R display flush path used for the control experiment
    - Path: 0081-atoms3r-wamr-probe-console/main/wasm_host_api.cpp
      Note: Queued host import model for replay and Wasm execution
    - Path: 0081-atoms3r-wamr-probe-console/main/wasm_module_runner.cpp
      Note: Wasm load instantiate execute flow under comparison
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-22T16:19:17.104516655-04:00
WhatFor: ""
WhenToUse: ""
---


# AtomS3R WAMR Cross-Device Debugging Guide

## Goal

This ticket exists to answer a narrower question than the PaperS3 work: is the remaining instability fundamentally about WAMR on ESP32-S3, or is it specific to the PaperS3 e-ink display path and its driver stack? The AtomS3R is a useful control board because it keeps the same SoC family and external display complexity, but removes the PaperS3 EPD pipeline from the equation.

The intention is not to rebuild the full PaperS3 demo on a second board. The intention is to build the smallest possible probe project that can exercise the same WAMR lifecycle stages and the same style of host-call queueing while using an entirely different display backend. If the AtomS3R reproduces a similar crash boundary, the bug likely sits closer to runtime integration or task/thread assumptions. If it stays stable while PaperS3 still fails, that points back toward the PaperS3 canvas and M5GFX EPD path.

## Current Theory

The PaperS3 debugging so far has already separated several layers:

- The earlier `bytecodealliance` integration could execute simple Wasm, but later display replay crashed.
- The first-pass Espressif component migration regressed even earlier, during runtime instantiation, until two platform assumptions were patched back to our known-good behavior.
- After those fixes, `return-42` and `log-only` recovered, but `hello-frame` still crashed in the PaperS3 preflush/display path.

That means the strongest current theory is:

- WAMR itself can be made to instantiate and execute on this ESP32-S3 class.
- The unresolved failure may depend on the PaperS3 display stack, especially the e-ink panel write path.
- We still need a cross-device comparison before we claim the remaining problem is "PaperS3-only."

## Why AtomS3R

AtomS3R is the best available next board because it preserves several variables that matter:

- `ESP32-S3` architecture
- external display driven through M5GFX/LovyanGFX
- optional PSRAM-backed memory environments depending on the exact board SKU and config
- realistic embedded console/debug workflow

At the same time, it removes or changes the variables that made PaperS3 special:

- no e-ink refresh modes
- no `Panel_EPD` path
- no full-screen PaperS3 grayscale update sequencing

This makes it a practical control experiment.

## Comparison Matrix

The comparison should be staged, not all-at-once:

1. Runtime-only probes
   - `wasm status`
   - `wasm run return-42`
   - `wasm run log-only`
2. Host command queue without Wasm
   - `wasm replay hello-frame`
3. Wasm plus display queue
   - `wasm run hello-frame`
4. Optional worker-thread follow-up
   - run the same modules on a dedicated worker thread rather than directly on the console REPL task

Interpretation rules:

- If runtime-only probes fail on AtomS3R, the problem is broader than PaperS3 display.
- If runtime-only probes pass and replay-only passes, but `wasm run hello-frame` fails, the issue is likely in the combination of Wasm call context and display-side effects.
- If all three pass on AtomS3R, the remaining suspicion falls heavily on the PaperS3 EPD implementation.

## Project Shape

The new project should be intentionally small and derive from two known-good sources:

- `0013-atoms3r-gif-console`
  - provides stable AtomS3R display and backlight bring-up
  - already has `esp_console`
- `0079-papers3-wamr-assemblyscript-console`
  - provides the current WAMR runtime, module registry, command structure, and host-call queueing model

The project should not inherit unnecessary baggage from either side:

- do not carry GIF storage, playback, or registry code from `0013`
- do not carry PaperS3-specific EPD abstractions from `0079`
- do carry the minimum set of Wasm assets and commands needed to reproduce the comparison matrix

## Proposed Files

Expected ownership in the new project:

- `CMakeLists.txt`
  - project identity and component search path
- `sdkconfig.defaults`
  - AtomS3R board defaults, USB Serial/JTAG console, WAMR feature selection
- `main/app_main.cpp`
  - board bring-up, status text, runtime init, host API init, REPL start
- `main/display_hal.cpp`
  - AtomS3R panel bring-up based on the known-good `0013` GC9107 path
- `main/backlight.cpp`
  - AtomS3R backlight gating and brightness device sequencing
- `main/atoms3r_canvas.cpp`
  - draw operations and present path for the AtomS3R control experiment
- `main/wasm_runtime_service.cpp`
  - runtime init and status reporting
- `main/wasm_host_api.cpp`
  - queued host commands and native symbol registration
- `main/wasm_module_runner.cpp`
  - runtime load / instantiate / execute / flush flow
- `main/wasm_command.cpp`
  - CLI surface used for the comparison matrix
- `main/wasm-assets/*.wasm`
  - embedded probe binaries

## Minimal Architecture

```text
USB Serial/JTAG
       |
       v
  esp_console REPL
       |
       +--> wasm status / list / info
       |
       +--> wasm replay hello-frame
       |        |
       |        v
       |   host command queue
       |        |
       |        v
       |   AtomS3R canvas/display backend
       |
       +--> wasm run <module>
                |
                v
          WAMR runtime
                |
                +--> no imports: return-42
                +--> log import: log-only
                +--> display imports: hello-frame
```

## Implementation Notes

### Console transport

Use USB Serial/JTAG by default. The local repository guidance is explicit about preferring USB Serial/JTAG for ESP32-S3 interactive consoles because UART pins are routinely repurposed on M5 hardware.

### Wasm runtime

Start from the current `0079` runtime structure:

- interpreter mode first
- narrow feature set
- pool allocator
- explicit status reporting

This keeps the AtomS3R experiment close to the PaperS3 setup we are trying to compare against.

### Display path

Do not attempt to port the PaperS3 canvas abstraction directly. The AtomS3R path should stay honest to the hardware:

- initialize the GC9107 panel through `M5GFX`
- use direct fill/draw operations or a simple offscreen canvas only if needed
- keep the first version straightforward enough that a crash can be localized quickly

### Wasm assets

The initial asset set should be:

- `return-42`
- `log-only`
- `hello-frame`

The first two are diagnostic probes. The third is the cross-device display probe.

## Pseudocode

```text
app_main():
  init backlight safely
  init AtomS3R display
  draw boot banner
  init wasm runtime
  if runtime ok:
    init wasm host api
  start console repl

command wasm run <name>:
  module = find embedded module
  load module
  instantiate module
  lookup export "run"
  create exec env
  invoke export
  flush queued host commands to AtomS3R display
  print result

command wasm replay hello-frame:
  reset queued frame
  queue same host commands used by hello-frame
  flush queue without invoking WAMR
  print result
```

## Success Criteria

- the project builds cleanly under `ESP-IDF 5.3.4`
- the board boots and exposes a usable console
- `return-42` and `log-only` establish whether runtime-only execution is stable on AtomS3R
- `replay hello-frame` establishes whether the AtomS3R display pipeline is stable without Wasm
- `run hello-frame` shows whether Wasm plus Atom display interaction remains stable

## Failure Recording Rules

Because this ticket is primarily a debugging/control experiment, every failure should be recorded with:

- exact command issued
- whether the board had been freshly flashed or only reset
- whether the failure happened before guest execution, during guest execution, or during host flush
- full backtrace when available
- the interpreted crash boundary in prose

That detail belongs in the diary, not just in terminal history.
