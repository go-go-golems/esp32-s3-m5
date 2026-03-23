---
Title: Minimal PaperS3 allocator-control implementation plan
Ticket: ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL
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
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T10:56:06.483548045-04:00
WhatFor: ""
WhenToUse: ""
---

# Minimal PaperS3 allocator-control implementation plan

## Goal

Create a new PaperS3 firmware that removes nearly all of the `0079` demo/application surface area while preserving the exact WAMR lifecycle and PSRAM touch controls that matter to the current bug. The output should be a smaller and easier-to-reason-about harness for answering whether WAMR instantiate alone poisons later PSRAM writes on PaperS3.

## Why a new firmware is justified

The current `0079` project has been useful, but it still carries too much unrelated state:

- M5Unified board bring-up
- display initialization and legacy display feature flags
- display-oriented host API plumbing
- multiple embedded Wasm demos
- replay helpers that exist only to support the demo story

Even though previous control paths already ruled out much of the display stack, the project shape still makes it too easy to wonder whether some unrelated initialization detail is involved. `0082` exists to remove that ambiguity.

## Reduced scope

The new harness should keep only:

- USB Serial/JTAG console
- PaperS3 target and PSRAM configuration
- WAMR runtime initialization
- WAMR lifecycle commands:
  - `status`
  - `instantiate-bare`
  - `instantiate-bare-keepalive`
  - `instantiate-no-execenv`
  - `instantiate-only`
- host-side memory probes:
  - `internal-scratch`
  - `psram-scratch`
  - `psram-persistent-init`
  - `psram-persistent-touch`
  - `psram-persistent-touch-sync`
  - `psram-persistent-free`
- minimal embedded modules:
  - `return-42`
  - optionally `log-only` if a non-display host import check still helps

The harness should remove:

- display bring-up
- `PaperCanvas`
- display host imports
- display replay controls
- demo-oriented Wasm modules
- any board UI behavior not required for the console harness

## Design approach

Start from the copied `0079` tree in `0082`, but cut it down aggressively instead of layering more feature flags on top. The goal is not to preserve app flexibility. The goal is to create a clean debugging instrument.

### Command surface

The console should expose only the commands needed for the current investigation:

```text
wasm status
wasm list
wasm instantiate-bare return-42
wasm instantiate-bare-keepalive return-42
wasm instantiate-no-execenv return-42
wasm instantiate-only return-42
wasm replay internal-scratch
wasm replay psram-scratch
wasm replay psram-persistent-init
wasm replay psram-persistent-touch
wasm replay psram-persistent-touch-sync
wasm replay psram-persistent-free
```

### Probe matrix

The first hardware pass should run the following sequence:

1. fresh boot
2. `wasm status`
3. `wasm replay psram-persistent-init`
4. `wasm replay psram-persistent-touch-sync`
5. fresh boot
6. `wasm instantiate-bare-keepalive return-42`
7. `wasm replay psram-persistent-touch-sync`

If time permits, run the stricter split:

1. fresh boot
2. `wasm replay psram-persistent-init`
3. `wasm instantiate-no-execenv return-42`
4. `wasm replay psram-persistent-touch-sync`

### Expected interpretation

- If the reduced harness still reproduces the crash, the remaining bug is very likely below the surrounding demo app and belongs to the WAMR instantiate or lower external-memory interaction path.
- If the reduced harness stops reproducing the crash, some supposedly irrelevant `0079` initialization path still matters, and the diff between `0079` and `0082` becomes the next debugging surface.

## File targets

- `0082-papers3-wamr-allocator-control/main/CMakeLists.txt`
- `0082-papers3-wamr-allocator-control/main/app_main.cpp`
- `0082-papers3-wamr-allocator-control/main/wasm_command.cpp`
- `0082-papers3-wamr-allocator-control/main/wasm_module_registry.cpp`
- `0082-papers3-wamr-allocator-control/main/wasm_replay_control.cpp`
- `0082-papers3-wamr-allocator-control/README.md`
- `0082-papers3-wamr-allocator-control/sdkconfig.defaults`

## Risks

- Copying from `0079` means stale files can stay linked into the build unless `CMakeLists.txt` is simplified decisively.
- Removing the display stack may expose hidden compile dependencies where display files were dragging in shared declarations.
- A too-large command surface defeats the purpose of the control harness.

## Success criteria

The task is successful when:

- `0082` builds cleanly on `ESP-IDF 5.3.4`
- the PaperS3 console comes up on USB Serial/JTAG
- the reduced probe sequence is reproducible from a single scriptable console session
- the ticket diary records the exact commands, outcomes, and any new failure boundary
