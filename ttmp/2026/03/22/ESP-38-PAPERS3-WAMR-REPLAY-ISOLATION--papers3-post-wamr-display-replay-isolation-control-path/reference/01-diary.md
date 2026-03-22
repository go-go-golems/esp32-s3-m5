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
LastUpdated: 2026-03-22T12:15:00-04:00
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
