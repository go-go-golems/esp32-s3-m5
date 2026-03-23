---
Title: Embedded buffer mapping investigation plan
Ticket: ESP-43-PAPERS3-WAMR-EMBEDDED-BUFFER-MAPPING
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_command.cpp
      Note: Exposes embedded versus copied-buffer load probes on device
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_module_registry.cpp
      Note: Defines the embedded Wasm asset pointers and names that now look suspicious
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp
      Note: Carries the binary-source selection and the current load-only experiments
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T12:27:01.121735154-04:00
WhatFor: ""
WhenToUse: ""
---


# Embedded buffer mapping investigation plan

## Goal

Turn the new `ESP-42` result into a focused investigation plan:

- embedded `load-only` still poisons later PSRAM writes
- copied-internal `load-only` does not
- copied-spiram `load-only` does not

The purpose of this ticket is to answer whether the remaining PaperS3 fault is caused by the way embedded Wasm bytes are linked and read from flash-mapped memory, rather than by generic WAMR parsing or allocator behavior.

## Context

`ESP-42` already reduced the problem substantially:

- runtime initialization alone is not the trigger
- allocator mode is not the decisive variable
- instantiate is not the smallest toxic boundary anymore
- `wasm_runtime_load(...)` is enough to trigger the later PSRAM fault
- but only when the source buffer is the embedded module bytes

That makes this a mapping and source-buffer problem first, not a broad runtime problem.

## Current hypothesis

Current best hypothesis:

- the embedded Wasm bytes are being read from a flash-mapped region whose access pattern leaves PaperS3 in a state that later breaks PSRAM writes

Competing hypotheses that are now weaker:

- generic WAMR allocator corruption
- generic WAMR loader corruption regardless of source buffer
- simple SPIRAM-vs-internal allocator conflict

## Investigation plan

1. Map the embedded asset path in `0082`.
2. Document exactly where the embedded asset pointers live.
3. Compare that path against the copied-internal and copied-spiram paths.
4. Add a targeted mitigation that always copies before load.
5. Decide whether that mitigation is merely a workaround or the correct production design.

## Immediate code targets

- [wasm_module_registry.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_module_registry.cpp)
- [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp)
- [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_command.cpp)
- [wasm_runtime_common.c snapshot](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/23/ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL--papers3-minimal-wamr-allocator-control-firmware-to-isolate-instantiate-vs-psram-contamination/scripts/wamr-local-debug-snapshots/wasm_runtime_common.c)
- [wasm_loader.c snapshot](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/23/ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL--papers3-minimal-wamr-allocator-control-firmware-to-isolate-instantiate-vs-psram-contamination/scripts/wamr-local-debug-snapshots/wasm_loader.c)

## Expected outcomes

Best-case outcome:

- we confirm that copying embedded Wasm into RAM before `wasm_runtime_load(...)` is a stable fix on PaperS3

Higher-value outcome:

- we also understand why the embedded flash-mapped path is different enough to poison later PSRAM writes, so the fix can be justified and documented rather than left as folklore
