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
LastUpdated: 2026-03-22T14:42:00-04:00
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

## Step 6: Add minimal Wasm probes and verify whether plain WAMR execution alone is enough

Step 5 showed that WAMR cleanup was not necessary for the panic. That still left an important question open: do the display-related native imports matter, or does a plain WAMR execution already make the later PaperS3 replay unsafe?

### What I changed

I added two tiny probe modules:

- [wasm-src/return-42/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/return-42/assembly/index.ts)
  - no host imports
  - just returns `42`
- [wasm-src/log-only/assembly/index.ts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src/log-only/assembly/index.ts)
  - imports only `logI32`
  - logs `(9, 42)` and returns `42`

To make them usable end-to-end, I also updated:

- [tools/build-wasm-demos.mjs](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/tools/build-wasm-demos.mjs)
  - added both probes to the build list
- [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)
  - embedded both new `.wasm` assets
- [main/wasm_module_registry.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_registry.cpp)
  - registered both probes in the module list

Then I regenerated the embedded Wasm assets with:

- `npm run build`

and rebuilt the firmware with:

- `unset IDF_PYTHON_ENV_PATH IDF_PATH; source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null; idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build`

Both commands passed.

### What I ran on hardware

After reflashing the updated firmware, I first checked that the modules were visible:

- `wasm list`

The console showed:

- `return-42`
- `log-only`
- the existing five display demos

Then I ran the key probe sequence one command at a time:

- `wasm run-preflush return-42`
- `wasm replay hello-frame`

After the reboot caused by that second command, I also ran:

- `wasm run-preflush log-only`

### What happened

The no-import module succeeded:

- `module=return-42`
- `flush_timing=before-cleanup`
- `execution=success`
- `return_value=42`

Immediately afterward, the host-only control replay crashed:

- `paper>  wasm replay hello-frame`
- `Guru Meditation Error: ... panic'ed (Cache disabled but cached memory region accessed)`
- stack again reached:
  - `papers3_wasm::FlushWasmHostFrame(...)`
  - `papers3_wasm::RunWasmReplayControlExample(...)`
  - `papers3_wasm::PaperCanvasScreenClear(...)`

That is the crucial part of the result. `wasm replay hello-frame` does not go through WAMR at all, so if it crashes immediately after `return-42`, then the `return-42` execution itself already poisoned the later control replay.

The minimal-import module also succeeded on its own:

- `paper>  wasm run-preflush log-only`
- `guest_log tag=9 value=42`
- `module=log-only`
- `flush_timing=before-cleanup`
- `execution=success`
- `return_value=42`

### Why this result is stronger than the earlier ones

Earlier results already showed:

- teardown is not required
- display-native imports are not required for the panic location

This step goes further:

- a Wasm module with no host imports at all can run successfully
- and after that, a pure host-side control replay can still crash

That means the minimal condition for destabilizing the display replay path is now:

- successful WAMR execution on the current console task

not:

- WAMR teardown
- display host imports
- queued display replay during the Wasm run itself

### Updated hypothesis

The leading hypotheses are now narrower and more runtime-internal:

- WAMR execution is altering cache-sensitive or task-local state that survives the call
- or the interpreter/native bridge is leaving the current task or CPU in a state that PaperS3 drawing cannot tolerate afterward

The evidence now argues against explanations centered on:

- the literal `hello-frame` queue contents
- the cleanup path alone
- display-related host callbacks being the sole trigger

### Debugging lesson

This step is a good example of why "smaller inputs" matter. The `return-42` module is boring on purpose. Because it does almost nothing, it removes almost every attractive but misleading story except one: WAMR execution itself is implicated.

### New next step

The next investigation should stop adding application-level demos and instead inspect runtime state around `wasm_runtime_call_wasm[_a]`:

- task/critical-section state before and after the call
- whether cache-sensitive platform code runs on the return path
- whether the current local WAMR component patches differ materially from the official Espressif integration path in ways that matter on ESP32-S3

## Step 7: Instrument execution-state snapshots around the WAMR call

Step 6 established that plain WAMR execution is enough to poison later replay, but that still did not tell us whether the runtime was returning with an obviously bad interrupt or task state. The next move was to instrument the success path itself.

### What I changed

I added execution-state snapshots to [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp) at three points:

- `before-call`
- `after-call`
- `before-preflush` or `before-postcleanup-flush`

The snapshot prints:

- current core id
- current task handle
- cycle count
- raw Xtensa `PS` register
- decoded `PS.INTLEVEL`
- `xPortInIsrContext()`
- FreeRTOS scheduler-running flag for the current core
- FreeRTOS interrupt nesting
- FreeRTOS critical nesting
- FreeRTOS saved old interrupt state
- stack high-water mark for the current task

This is intentionally diagnostic and temporary. The purpose is to determine whether the WAMR return path is visibly leaving the current task with elevated interrupt level, ISR context, or critical nesting.

### Build mistake and fix

My first build failed. I declared the FreeRTOS port globals inside the C++ anonymous namespace, which caused the linker to look for mangled C++ symbol names that do not exist.

The linker failure looked like:

- `undefined reference to ... port_xSchedulerRunning`
- `undefined reference to ... port_interruptNesting`
- `undefined reference to ... port_uxCriticalNesting`
- `undefined reference to ... port_uxOldInterruptState`

This was a straightforward integration mistake, not a runtime finding. I fixed it by moving those declarations into an `extern "C"` block outside the C++ namespace.

### What I ran

After rebuilding and flashing the instrumented firmware, I ran:

- `wasm run-preflush return-42`

I chose `return-42` because it is the smallest success case and does not pull in host imports that could distract from the runtime-state question.

### What happened

The success output included the new probes:

- `exec_probe.stage=before-call`
- `exec_probe.ps.intlevel=0`
- `exec_probe.in_isr=no`
- `exec_probe.interrupt_nesting=0`
- `exec_probe.critical_nesting=0`

After the Wasm call returned:

- `exec_probe.stage=after-call`
- `exec_probe.ps.intlevel=0`
- `exec_probe.in_isr=no`
- `exec_probe.interrupt_nesting=0`
- `exec_probe.critical_nesting=0`

Immediately before the preflush:

- `exec_probe.stage=before-preflush`
- `exec_probe.ps.intlevel=0`
- `exec_probe.in_isr=no`
- `exec_probe.interrupt_nesting=0`
- `exec_probe.critical_nesting=0`

The command itself still succeeded:

- `module=return-42`
- `execution=success`
- `return_value=42`

### What I learned

This result removes one tempting explanation:

- there is no obvious leak of ISR context
- there is no obvious nonzero Xtensa interrupt level on return
- there is no obvious nonzero FreeRTOS critical nesting on return

That does not clear WAMR. It only tells us the corruption is subtler than "interrupts stayed disabled" or "the task never exited a critical section."

The remaining suspicious areas are now things like:

- cache or memory-mapping state not reflected in the simple task/interrupt counters
- interpreter/native-call bridge behavior specific to Xtensa
- local WAMR platform integration differences that do not show up in these basic task-state snapshots

### Why this step still matters

Negative results are important in debugging when they eliminate popular but weak theories. This step does exactly that. If the counters had come back with `intlevel > 0` or nonzero critical nesting, the next step would be obvious. Because they did not, the investigation has to move lower into WAMR and ESP32-S3 platform behavior.
