---
Title: Diary
Ticket: ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - display
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp
      Note: |-
        Console entrypoint where replay control commands will be added
        Replay control commands and operator-facing debugging output
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: |-
        Host queue and flush implementation being isolated
        Queue and flush behavior observed during debugging
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: |-
        Existing WAMR-backed path used as the comparison baseline
        Existing WAMR-backed path used as comparison baseline
    - Path: 0079-papers3-wamr-assemblyscript-console/wasm-src/hello-frame/assembly/index.ts
      Note: |-
        Guest sequence mirrored by the control path
        Guest behavior mirrored by replay control tests
ExternalSources: []
Summary: Detailed debugging diary for the PaperS3 replay-isolation control-path experiment.
LastUpdated: 2026-03-22T14:04:00-04:00
WhatFor: Record each replay-isolation implementation and hardware-debugging step in enough detail to reconstruct the reasoning later.
WhenToUse: Read this before continuing the ticket or reviewing why a specific control-path experiment was attempted.
---


# Diary

## Goal

This diary exists to make the replay-isolation investigation auditable. The objective of the ticket is not merely to "try another workaround." It is to answer one precise question: does the queued PaperS3 replay path crash even when WAMR is absent from the execution path?

Every debugging slice in this ticket should record:

- what changed
- what command was run
- what happened on-device
- what hypothesis changed as a result

## Step 1: Create the follow-up replay-isolation ticket

The previous ticket reached an important but incomplete milestone. The crash moved from WAMR memory and thread setup into queued host replay, but that alone did not prove WAMR was no longer relevant. A follow-up ticket was needed so the next work item could stay focused on one binary decision instead of mixing runtime fixes, display fixes, and integration cleanup into one stream.

### Prompt context

**User prompt (verbatim):**

> go ahead, create a new ticket. Also, it looks like ESP themselves have added WAMR support to esp-idf, but I think that might be in newer releases?

**Assistant interpretation:** Create a follow-up ticket centered on replay isolation, then continue with a more disciplined baseline experiment.

**Inferred intent:** Stop guessing whether WAMR is still the culprit and create a clean experimental track that can prove it one way or the other.

### What I did

- Created ticket `ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION`.
- Defined the new scope as a WAMR-free replay control path rather than another round of runtime patching.

### Why

- The current best next experiment is a baseline, not another speculative WAMR patch.
- The work deserved its own ticket because the success criterion is different from the previous ticket.

### What I learned

- The fastest path forward is to compare:
  - `wasm replay hello-frame`
  - `wasm run hello-frame`
- If both fail identically, the queue/display path is the real problem.
- If only the Wasm-backed path fails, WAMR or its teardown still matters.

## Step 2: Write the implementation plan before touching firmware

Before writing code, I documented the exact shape of the experiment. That matters because debugging tickets often drift into ad hoc command additions that stop answering the original question clearly.

### What I did

- Wrote the implementation plan in `design/01-replay-isolation-implementation-plan.md`.
- Wrote this diary.
- Planned the first control command around `hello-frame` because that is the smallest realistic sequence already exercised by the Wasm guest.

### Why

- `hello-frame` is short enough to mirror literally.
- A literal mirror avoids ambiguity when comparing control-path and Wasm-path behavior.

### Planned first code slice

- Expose host queue helpers from the existing host API.
- Add a host-side replay helper for `hello-frame`.
- Add a console command to trigger the control path.
- Flash hardware and compare the control path with the Wasm-backed path.

## Step 3: Implement the WAMR-free `hello-frame` replay control path

This step intentionally stopped before any hardware work. The goal was to get the control path into firmware in a buildable form so the next hardware session can compare `wasm replay hello-frame` and `wasm run hello-frame` directly.

### What I changed

- Expanded [wasm_host_api.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.h) so host queue operations are reusable from non-Wasm code.
- Refactored [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp) to expose public queue helpers:
  - `QueueWasmHostLogI32(...)`
  - `QueueWasmHostDelayMs(...)`
  - `QueueWasmHostScreenClear(...)`
  - `QueueWasmHostDrawRect(...)`
  - `QueueWasmHostFillRect(...)`
  - `QueueWasmHostPresent(...)`
  - `GetWasmHostQueuedCommandCount()`
- Added [wasm_replay_control.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.h) and [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp).
- Implemented a literal `hello-frame` control sequence that mirrors the guest AssemblyScript program in [wasm-src/hello-frame/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/hello-frame/assembly/index.ts).
- Updated [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp) to add:
  - `wasm replay <name>`
- Updated [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt) to compile the new replay helper.

### Why this structure

- The queue helpers needed to be callable from both:
  - WAMR native-import callbacks
  - host-side control code
- The replay helper needed to stay literal and narrow.
  A generic scene abstraction would only create room for accidental divergence from the guest behavior.

### Build validation

I validated the implementation with:

- `source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build`

The build passed cleanly.

### What I learned

- The replay-isolation command can be added without disturbing the current WAMR-backed command structure.
- The control-path code is small enough that future hardware logs should be easier to interpret than the existing Wasm-backed path.

### What remains for the next step

- No hardware was touched in this step because the user explicitly disconnected the eink device.
- The next step is purely comparative hardware validation:
  - `wasm replay hello-frame`
  - `wasm run hello-frame`

### Review instructions

- Read [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp) first.
- Compare its queue sequence against [wasm-src/hello-frame/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/hello-frame/assembly/index.ts).
- Then inspect [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp) to confirm the control path and Wasm path are sharing the same queue implementation.

## Step 4: Flash the replay-control firmware and run the control-path baseline

This was the first hardware step in the ticket and the most important comparison point so far. The point of the control path was not to "show something on screen." The point was to determine whether the display queue and replay logic could execute on real PaperS3 hardware when WAMR was absent from the path.

### What I ran

- Cleared any stale serial users:
  - `lsof -t /dev/ttyACM0 | xargs -r kill`
- Started a fresh tmux shell+monitor session so the shell would survive monitor exit:
  - `tmux new-session -d -s papers3-0079-replay 'unset IDF_PYTHON_ENV_PATH IDF_PATH; source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console -p /dev/ttyACM0 flash monitor; exec zsh -li'`
- Waited for the `paper>` prompt.
- Ran:
  - `wasm replay hello-frame`
  - `wasm run hello-frame`

### What happened

The firmware flashed and booted normally. The console came up over USB Serial/JTAG and WAMR initialized cleanly:

- `Initializing WAMR runtime (allocator=pool, mode=interp)`
- `WAMR ready (version=2.4.3, interp=yes, aot=no, fast-jit=no, llvm-jit=no)`
- `Registered 6 host symbols for module 'host'`

The control-path command succeeded:

- `wasm replay hello-frame`
- `guest_log tag=1 value=79`
- `control_example=hello-frame`
- `queued_commands=9`
- `control_execution=success`

The WAMR-backed command still panicked:

- `wasm run hello-frame`
- panic class:
  - `Guru Meditation Error: ... panic'ed (Cache disabled but cached memory region accessed)`
  - `Write back error occurred while dcache tries to write back to flash`
- relevant stack region:
  - `lgfx::v1::Panel_EPD::writeFillRectPreclipped(...)`
  - `papers3_wasm::PaperCanvasScreenClear(...)`
  - `papers3_wasm::FlushWasmHostFrame(...)`
  - `papers3_wasm::RunEmbeddedWasmModuleOnCurrentThread(...)`

### Why this result matters

This is the strongest isolation result so far in the overall investigation:

- The same queue/replay machinery works on hardware when WAMR is absent.
- The crash only appears on the path that enters WAMR and then later flushes the queued display commands.
- That means the display replay sequence itself is not sufficient to explain the panic.

This result eliminates the previous ambiguous interpretation that "maybe the PaperS3 display code is just fragile." It may still be sensitive, but it is demonstrably capable of handling the `hello-frame` queue when WAMR is not in the lifecycle.

### Updated hypothesis

The unstable boundary is now much narrower. The two paths are:

- control path:
  - reset frame
  - queue commands
  - flush commands
- WAMR path:
  - reset frame
  - load module
  - instantiate
  - create exec env
  - call Wasm
  - destroy exec env
  - deinstantiate module
  - unload module
  - flush commands

Since only the second path panics, one of these is true:

- the Wasm call itself leaves the system in a bad state even after it returns
- or the WAMR cleanup sequence leaves the system in a bad state before replay begins

That is why the next task changed from "reduce the control replay further" to "compare pre-cleanup and post-cleanup replay timing on the WAMR path."

### Mistake avoided

Without this hardware baseline, it would have been easy to keep patching the PaperS3 drawing layer or adding smaller control examples. That would have generated work, but it would not have answered the core question. Running the direct control-path baseline first avoided that debugging drift.

### Next step

Add a diagnostic execution mode that flushes the queued frame before WAMR teardown, while preserving the current behavior as the post-cleanup comparison path.

## Step 5: Add `run-preflush` and test whether teardown is actually the trigger

Step 4 narrowed the problem, but it still left one ambiguity: perhaps the Wasm call itself was fine and only WAMR cleanup poisoned the system. The correct next move was not another broad patch. It was a timing experiment.

### What I changed

I added a second WAMR execution mode in the firmware:

- `wasm run <name>`
  - existing behavior
  - flush queued host commands after WAMR cleanup
- `wasm run-preflush <name>`
  - new diagnostic behavior
  - flush queued host commands before WAMR cleanup

Implementation changes:

- Updated [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h) with:
  - `enum class WasmFlushTiming`
  - a `flush_timing` field in `WasmExecutionResult`
  - an execution API that accepts the flush timing
- Updated [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp) so:
  - the existing path still flushes after cleanup
  - the new diagnostic path flushes before destroying the exec env and unloading the module
  - result output prints the chosen flush timing
- Updated [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp) with:
  - `wasm run-preflush <name>`
  - updated usage/examples output

### Why this experiment matters

This is the shortest path to distinguishing between two concrete hypotheses:

- hypothesis A:
  - WAMR cleanup breaks cache or mapping state
- hypothesis B:
  - the WAMR call path itself, or the native-import path taken during execution, already leaves the system unstable before cleanup starts

If `run-preflush` succeeded while `run` failed, cleanup would be the prime suspect.
If both failed, cleanup would no longer be the leading explanation.

### Build validation

I rebuilt the firmware before touching hardware:

- `unset IDF_PYTHON_ENV_PATH IDF_PATH; source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build`

The build passed.

### What I ran on hardware

- Killed any stale monitor users and started a fresh tmux flash+monitor session.
- Flashed the updated firmware.
- Waited for the `paper>` prompt.
- Ran:
  - `wasm run-preflush hello-frame`

### What happened

The new diagnostic mode still panicked on hardware with the same class of failure:

- `Guru Meditation Error: ... panic'ed (Cache disabled but cached memory region accessed)`
- `Write back error occurred while dcache tries to write back to flash`

The key stack still passed through the first queued display primitive:

- `lgfx::v1::Panel_EPD::writeFillRectPreclipped(...)`
- `papers3_wasm::PaperCanvasScreenClear(...)`
- `papers3_wasm::FlushWasmHostFrame(...)`
- `papers3_wasm::RunEmbeddedWasmModuleOnCurrentThread(..., WasmFlushTiming)`

What changed was the meaning of that stack. In Step 4, the same stack still allowed cleanup to be blamed. In this step, the stack occurred before cleanup, so cleanup can no longer explain the failure.

### What I learned

This result materially changes the debugging model:

- The panic is not caused by WAMR teardown.
- Entering and returning from the WAMR execution path is already enough to make the subsequent PaperS3 replay unsafe.
- The unstable boundary is now either:
  - the Wasm interpreter execution path itself
  - or one of the native imports invoked from Wasm during execution

### Mistake corrected

Before this experiment, it was tempting to over-focus on `wasm_runtime_destroy_exec_env()`, `wasm_runtime_deinstantiate()`, or `wasm_runtime_unload()`. That would have been understandable given the old backtrace location, but this test showed that cleanup was the wrong center of gravity.

### New next step

The next discriminating probe should be smaller on the Wasm side, not the PaperS3 side. A minimal module that exercises WAMR with fewer or no display-related host imports would tell us whether:

- any WAMR execution poisons the later display path
- or only certain native-import activity does
