---
Title: WAMR replay isolation postmortem and intern report
Ticket: ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - postmortem
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp
      Note: |-
        PaperS3 drawing surface abstraction used by replay
        PaperS3 canvas abstraction used when replaying queued drawing commands
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: |-
        Queued host-command implementation and replay boundary where the crash manifests
        Queued host imports and replay boundary where cache-sensitive failures surfaced
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_registry.cpp
      Note: |-
        Embedded module catalog including the minimal probe modules
        Embedded registry including the minimal probe modules used during isolation
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: |-
        Main Wasm execution path, flush-timing experiments, and execution-state instrumentation
        Main WAMR execution path
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp
      Note: |-
        WAMR-free control path used to isolate replay behavior
        Control-path baseline used to prove replay works without WAMR
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: |-
        Runtime initialization and allocator configuration for WAMR
        Runtime allocator and mode configuration that shaped the debugging path
    - Path: 0079-papers3-wamr-assemblyscript-console/tools/build-wasm-demos.mjs
      Note: |-
        AssemblyScript build pipeline that generates the embedded Wasm assets
        AssemblyScript build pipeline for the embedded demo set
ExternalSources:
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0~1
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/dependencies?language=en
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/examples/esp-idf
Summary: Detailed postmortem of the PaperS3 WAMR replay-isolation investigation, written for a new intern who needs both the architecture context and the debugging reasoning.
LastUpdated: 2026-03-22T15:00:00-04:00
WhatFor: Explain what the PaperS3 Wasm demo stack is, what failed, how the team isolated the failure, what hypotheses were falsified, and how to continue without repeating the same mistakes.
WhenToUse: Read this before touching the WAMR integration, replay path, or follow-up experiments on `0079`.
---


# WAMR Replay Isolation Postmortem and Intern Report

## Executive Summary

This document explains a debugging investigation around a PaperS3 firmware prototype that embeds WebAssembly modules compiled from AssemblyScript and tries to run them from an `esp_console` command surface.

The short version is:

- The firmware can boot, initialize WAMR, register host APIs, and enumerate embedded Wasm modules.
- A pure host-side control path can replay the same `hello-frame` drawing sequence on real hardware without crashing.
- A Wasm-backed execution path can run simple modules successfully.
- However, after a successful WAMR execution, a later PaperS3 replay can panic with:
  - `Cache disabled but cached memory region accessed`
  - `Write back error occurred while dcache tries to write back to flash`
- The panic still occurs when replay is moved before WAMR teardown.
- The panic can be triggered even after running a Wasm module with no host imports at all.

The main conclusion is not a complete root cause, but it is a strong isolation result:

- the failure is not explained by the PaperS3 replay sequence alone
- the failure is not explained by WAMR teardown alone
- the failure is not explained only by display-related host imports
- plain WAMR execution on the console task is sufficient to make later PaperS3 replay unsafe

That makes this a runtime/platform integration problem, not just an application-level drawing bug.

## Why This Report Exists

This is written for a new intern, not for the person who just debugged it.

That means the report does four jobs:

1. explain the system
2. explain the failure
3. explain the experiments and why each one mattered
4. explain how to continue without wandering into the same rabbit hole

The debugging diary in [01-diary.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION--papers3-post-wamr-display-replay-isolation-control-path/reference/01-diary.md) is chronological. This document is analytical.

## System Overview

The system under investigation is the `0079` PaperS3 firmware project:

- [0079 project root](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console)

Its intended product idea is:

- write several small AssemblyScript programs
- compile them to `.wasm`
- embed them in firmware
- expose a console command like `wasm run <name>`
- let the Wasm guest call a small host API for drawing on PaperS3

At a high level, the stack looks like this:

```text
+-----------------------------+
| User on USB Serial/JTAG     |
| types: wasm run hello-frame |
+-------------+---------------+
              |
              v
+-----------------------------+
| esp_console command layer   |
| wasm_command.cpp            |
+-------------+---------------+
              |
              v
+-----------------------------+
| Wasm module runner          |
| wasm_module_runner.cpp      |
+-------------+---------------+
              |
              v
+-----------------------------+
| WAMR runtime                |
| load / instantiate / call   |
+-------------+---------------+
              |
              v
+-----------------------------+
| Host import callbacks       |
| wasm_host_api.cpp           |
| queue commands, no drawing  |
+-------------+---------------+
              |
              v
+-----------------------------+
| Replay / canvas layer       |
| wasm_host_api.cpp           |
| papers3_canvas.cpp          |
+-------------+---------------+
              |
              v
+-----------------------------+
| PaperS3 display stack       |
| M5Unified -> M5GFX -> EPD   |
+-----------------------------+
```

## Main Subsystems You Need to Understand

### 1. Console Layer

The console surface is implemented in:

- [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- [console_repl.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/console_repl.cpp)

It is important that this runs over USB Serial/JTAG, not the traditional UART console, because the board has other possible UART consumers and the repo’s agent instructions explicitly prefer USB Serial/JTAG for console work.

Relevant commands introduced during the investigation:

- `wasm list`
- `wasm info <name>`
- `wasm replay <name>`
- `wasm run <name>`
- `wasm run-preflush <name>`
- `wasm status`

### 2. Embedded Wasm Module Catalog

The firmware embeds a set of precompiled `.wasm` files:

- [wasm_module_registry.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_registry.cpp)
- [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)

The catalog matters because it defines exactly what the runtime can execute.

By the end of the investigation the registry included:

- `return-42`
- `log-only`
- `hello-frame`
- `nested-boxes`
- `bars`
- `checkerboard`
- `radar-sweep`

### 3. AssemblyScript Build Pipeline

The guest programs live under:

- [wasm-src](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src)

The generator script is:

- [build-wasm-demos.mjs](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/tools/build-wasm-demos.mjs)

Its job is:

- enumerate the demo list
- call `asc`
- write `.wasm` and `.wat`
- sync release assets into `main/wasm-assets`

Pseudocode:

```text
for demo in demos:
    compile demo/assembly/index.ts -> demo.wasm + demo.wat

if target == release:
    copy demo.wasm into main/wasm-assets/
```

### 4. WAMR Runtime Service

Runtime initialization is handled in:

- [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)

This file answers questions like:

- is WAMR initialized?
- what allocator mode is used?
- what runtime mode is enabled?
- are interpreter, AOT, or JIT paths compiled in?

During the earlier debugging work, this layer was already patched to use a pool allocator and an interpreter-first configuration because previous crashes appeared earlier in the lifecycle.

### 5. WAMR Execution Path

The main execution flow is in:

- [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)

Normal path pseudocode:

```text
reset_host_frame()
reset_canvas_frame()

module = wasm_runtime_load(...)
inst = wasm_runtime_instantiate(...)
fn = wasm_runtime_lookup_function(...)
env = wasm_runtime_create_exec_env(...)

wasm_runtime_call_wasm(...)

destroy env
deinstantiate inst
unload module

FlushWasmHostFrame(...)
```

Diagnostic variant added during the ticket:

```text
wasm_runtime_call_wasm(...)
FlushWasmHostFrame(...)   # before cleanup
destroy env
deinstantiate inst
unload module
```

### 6. Host API Queue

The guest does not draw directly into M5GFX in the import callbacks. That was an intentional isolation change.

Instead, host imports queue commands in:

- [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)

Commands include:

- `LogI32`
- `DelayMs`
- `ScreenClear`
- `DrawRect`
- `FillRect`
- `Present`

Why queue first?

- It makes Wasm import callbacks cheap.
- It separates guest execution from drawing side effects.
- It allows the same replay path to be driven by:
  - WAMR-backed execution
  - a pure host-side control path

### 7. Replay Control Path

The most important control experiment was implemented in:

- [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)

This file mirrors the `hello-frame` program literally, but from host-side code instead of from a Wasm guest.

That was the first decisive experiment because it let the team ask:

- if the same queued drawing commands crash without WAMR, the display path is the problem
- if they only crash after WAMR, WAMR state still matters

### 8. PaperS3 Canvas Layer

The replay layer eventually calls:

- [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)

This is the boundary between the host-command queue and the actual display library stack:

```text
queued host commands
    ->
FlushWasmHostFrame()
    ->
PaperCanvasScreenClear / DrawRect / FillRect / Present
    ->
M5Unified / M5GFX / Panel_EPD
```

## The Original Failure Symptom

The recurring hardware panic was:

- `Cache disabled but cached memory region accessed`
- `Write back error occurred while dcache tries to write back to flash`

The most common stack location was in the first queued replay primitive, usually:

- `lgfx::v1::Panel_EPD::writeFillRectPreclipped(...)`
- `papers3_wasm::PaperCanvasScreenClear(...)`
- `papers3_wasm::FlushWasmHostFrame(...)`

That stack alone was not enough to identify the root cause. It only told us where the crash became visible, not what invalidated the system state first.

## Investigation Timeline

### Phase A: Initial WAMR Bring-Up

The earlier ticket (`ESP-37`) handled:

- runtime allocation issues
- task identity issues
- direct import-side effects

At the end of that ticket, the crash had already moved into the replay stage.

This created an ambiguity:

- maybe the display/replay path is bad
- or maybe WAMR still poisons state before replay begins

### Phase B: WAMR-Free Replay Baseline

Experiment:

- run `wasm replay hello-frame`

Result:

- success on hardware

Meaning:

- the replay path can work
- the literal `hello-frame` queue is not enough to explain the crash

### Phase C: Pre-Cleanup Flush Timing

Experiment:

- add `wasm run-preflush hello-frame`
- flush queued display commands before WAMR teardown

Result:

- still crashes

Meaning:

- teardown is not required to trigger the bad state

### Phase D: Minimal Wasm Probes

Experiments:

- `wasm run-preflush return-42`
- then `wasm replay hello-frame`
- after reboot, `wasm run-preflush log-only`

Results:

- `return-42` succeeds
- following host-side `wasm replay hello-frame` crashes
- `log-only` succeeds on its own

Meaning:

- display-related host imports are not required
- plain WAMR execution is enough to make later replay unsafe

### Phase E: Execution-State Instrumentation

Experiment:

- instrument task and CPU state around `wasm_runtime_call_wasm[_a]`

Result on `return-42` success path:

- `PS.INTLEVEL = 0`
- `in_isr = no`
- `interrupt_nesting = 0`
- `critical_nesting = 0`

Meaning:

- the bug is subtler than a simple leaked critical section or obvious interrupt masking bug

## The Most Important Falsified Hypotheses

### Hypothesis 1: The replay path is broken by itself

Status:

- falsified

Why:

- `wasm replay hello-frame` succeeds on clean boot

### Hypothesis 2: WAMR teardown is the culprit

Status:

- falsified

Why:

- `wasm run-preflush hello-frame` still crashes before teardown

### Hypothesis 3: Display-related host imports are the culprit

Status:

- falsified as the sole explanation

Why:

- `return-42` uses no host imports at all
- yet after `return-42`, a pure host-side replay can still crash

### Hypothesis 4: WAMR returns with interrupts visibly disabled or with leaked critical nesting

Status:

- not supported by the current evidence

Why:

- instrumentation showed normal values for basic interrupt/critical counters on the `return-42` success path

## Current Best Model of the Failure

The current best model is:

```text
WAMR execution on the console task
    ->
some cache-sensitive / platform-sensitive state changes
    ->
simple task/interrupt counters still look normal
    ->
later PaperS3 replay touches a path that depends on clean cache/memory state
    ->
panic appears in M5GFX / Panel_EPD
```

This is an inference from the experiments. It is not yet a root cause proven from a single defective line of code.

## Why This Became a Rabbit Hole

The user’s intuition here was correct. This investigation started as an application feature:

- run precompiled AssemblyScript demos from `esp_console`

It evolved into a platform-runtime investigation because:

- WAMR was present
- ESP32-S3 has cache and memory subtleties
- PaperS3 display operations exercise a sensitive hardware path
- the panic was visible in replay, not at the original point of state corruption

This combination is dangerous because each new experiment can be technically correct while still moving deeper into low-level territory.

For an intern, the lesson is:

- debugging is not just about gathering more data
- debugging is also about knowing when the marginal value of another experiment is dropping

## What Was Done Well

- The control path was added early enough to separate replay from WAMR.
- The experiments were designed to falsify hypotheses, not just “try things.”
- Minimal probe modules were introduced to remove attractive but misleading explanations.
- A detailed diary was kept, including mistakes such as the temporary linker failure in the instrumentation step.

## What Could Have Gone Worse

- The team could have kept patching display code after the first replay crash and lost days.
- The team could have over-focused on teardown after the post-cleanup stack traces.
- The team could have skipped the no-import probe and kept blaming host imports.

## Key Commands and What They Mean

### Build and Flash

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build
idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console -p /dev/ttyACM0 flash monitor
```

### Control Baseline

```bash
wasm replay hello-frame
```

Meaning:

- host-side queue and replay only
- no WAMR involved

### WAMR Before-Cleanup Timing Test

```bash
wasm run-preflush hello-frame
```

Meaning:

- Wasm execution plus replay before teardown

### No-Import Probe

```bash
wasm run-preflush return-42
wasm replay hello-frame
```

Meaning:

- test whether plain WAMR execution is enough to poison later replay

### Minimal Logging Probe

```bash
wasm run-preflush log-only
```

Meaning:

- test a Wasm guest with only the simplest host import

## API References a New Intern Should Know

### WAMR Embedding APIs

From the WAMR API list mentioned by the component readme:

- `wasm_runtime_load`
- `wasm_runtime_instantiate`
- `wasm_runtime_lookup_function`
- `wasm_runtime_create_exec_env`
- `wasm_runtime_call_wasm`
- `wasm_runtime_call_wasm_a`
- `wasm_runtime_destroy_exec_env`
- `wasm_runtime_deinstantiate`
- `wasm_runtime_unload`
- `wasm_runtime_register_natives`

Relevant local call sites:

- [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)

### FreeRTOS / Xtensa Diagnostics

Used in the instrumentation step:

- `xPortGetCoreID()`
- `xPortInIsrContext()`
- `uxTaskGetStackHighWaterMark(nullptr)`
- `esp_cpu_get_cycle_count()`
- raw Xtensa `PS` register read

## File Walkthrough for an Intern

Read these in order:

1. [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
   Understand the operator-facing commands.

2. [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
   Understand the control experiment that removed WAMR from the path.

3. [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
   Understand the queue and the flush boundary.

4. [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
   Understand load / instantiate / call / cleanup timing.

5. [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
   Understand where replay becomes actual display I/O.

6. [wasm-src/hello-frame/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/hello-frame/assembly/index.ts)
   Compare guest intent with control replay.

7. [wasm-src/return-42/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/return-42/assembly/index.ts)
   Understand the minimal no-import probe.

8. [wasm-src/log-only/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/log-only/assembly/index.ts)
   Understand the minimal import probe.

## Decision Tree for Future Work

If you continue this work, use a bounded decision tree:

```text
If the goal is "make product progress fast":
    prefer an A/B experiment with Espressif's official WAMR component
Else if the goal is "finish root-causing the current integration":
    inspect WAMR interpreter/native-call path and ESP32-S3 platform glue
Else:
    stop and reconsider whether on-device WAMR is worth the cost for this demo
```

## Recommendation

The most pragmatic next step is not more application-level probing.

It is one of these:

1. A bounded A/B experiment with Espressif’s official component package.
2. A strict low-level investigation of the interpreter/native bridge and ESP32-S3 platform layer.

What should not happen next:

- adding many more demo scenes
- adding many more drawing primitives
- rewriting PaperS3 replay code without new evidence

## Final Takeaway

The team did not end with a complete root-cause fix, but it did produce a valuable engineering outcome:

- the failure surface is now narrow enough to reason about
- several wrong explanations were eliminated decisively
- the next step can be chosen consciously instead of emotionally

That is what a good debugging postmortem looks like. It does not pretend uncertainty is certainty. It reduces the problem until the remaining uncertainty is honest, bounded, and actionable.
