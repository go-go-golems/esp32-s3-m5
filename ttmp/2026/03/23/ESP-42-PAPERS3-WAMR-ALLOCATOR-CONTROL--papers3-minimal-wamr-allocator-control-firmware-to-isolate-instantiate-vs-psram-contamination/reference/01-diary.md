---
Title: Diary
Ticket: ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T10:56:06.846153759-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Create and validate a smaller PaperS3 control firmware that preserves the WAMR instantiate and PSRAM-touch reproducer while removing almost all of the demo/display surface from `0079`.

## Context

`0079` already proved that:

- runtime initialization alone is not enough to reproduce the PSRAM fault
- `wasm_runtime_instantiate(...)` is enough to poison later PSRAM writes on PaperS3
- the same general WAMR baseline works on AtomS3R

The open question is whether the remaining fault boundary survives once the PaperS3 application is reduced to a near-minimum harness.

## Quick Reference

- New firmware: `0082-papers3-wamr-allocator-control`
- Ticket: `ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL`
- First strict probe matrix:
  - `wasm status`
  - `wasm replay psram-persistent-init`
  - `wasm replay psram-persistent-touch-sync`
  - reboot
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch-sync`

## Usage Examples

### 2026-03-23 11:08 EDT

Started the new slice by creating `ESP-42` and copying `0079` to `0082`. The copied project is intentionally too large at this point; it still contains display files, multiple Wasm demos, and the old demo-oriented command surface. The first job in this ticket is to cut that down before building anything, so the ticket begins with a concrete reduced-scope plan and explicit probe matrix rather than an ad hoc code edit.

### 2026-03-23 11:12 EDT

Reviewed the copied `0082` tree. Confirmed that the current build still drags in `PaperCanvas`, `M5Unified`, display feature flags, and the full embedded module registry. That is exactly the wrong shape for a control harness, so the implementation plan now states that `0082` should be aggressively minimized rather than hidden behind more feature flags.

### 2026-03-23 11:22 EDT

Cut `0082` down at the build-system level first. Removed `papers3_canvas.cpp` from `main/CMakeLists.txt`, dropped the direct `M5Unified` dependency, reduced embedded Wasm assets to `return-42` and `log-only`, and rewrote `app_main.cpp` so it no longer performs any board display initialization at all. This was deliberate: if the control harness still reproduces the fault after that, the display stack cannot be the explanation.

### 2026-03-23 11:31 EDT

Reworked the command and runtime surface to match the new scope. `wasm_command.cpp` now keeps only:

- `status`
- `list`
- `info`
- `run`
- instantiate lifecycle commands
- PSRAM/internal-RAM replay probes

At the same time, `wasm_host_api.cpp` was collapsed to a non-display host module with only `host_log_i32` and `host_delay_ms`, and `wasm_replay_control.cpp` was stripped of all display-oriented replay sequences. `wasm_module_runner.cpp` also stopped calling `PaperCanvasResetFrame()`.

### 2026-03-23 11:37 EDT

The first build failed in a useful way. After removing the old transitive component chain, `0082` no longer had explicit access to the low-level cache headers already used by the PSRAM probes:

- `esp_private/cache_utils.h`
- `esp_cache.h`

The error was not a logic regression. It was a dependency declaration gap caused by making the harness more explicit. I fixed it by adding `esp_mm` and `spi_flash` to `main/CMakeLists.txt`.

### 2026-03-23 11:40 EDT

The second build succeeded under `ESP-IDF 5.3.4`. One surprising but non-blocking observation remained: `M5GFX` and `M5Unified` still appeared in the overall project component graph because of the workspace setup inherited from the broader PaperS3 environment, even though `main` no longer requires or references them. That is not ideal, but it does not invalidate the reduced harness because the app code path itself no longer initializes or calls the display stack.

### 2026-03-23 11:45 EDT

Added fresh reusable probe tooling directly into `ESP-42/scripts`:

- `serial_probe_sequence.py`
- `flash_and_probe_allocator_control.sh`

This was important for two reasons. First, the user explicitly wants the scripts tracked in the ticket. Second, the prior debugging taught us that shell history and ad hoc serial sessions are not reliable provenance on USB Serial/JTAG.

### 2026-03-23 11:48 EDT

The first end-to-end flash/probe attempt failed for an operational reason, not a firmware reason. The device booted cleanly and printed the `paper>` prompt, but the script timed out before observing the prompt. I did not treat that as a product finding. I increased the prompt window and reused the already-flashed firmware instead of reflashing again.

### 2026-03-23 11:52 EDT

Ran the fresh-boot strict control sequence on `0082`:

- `wasm status`
- `wasm replay psram-persistent-init`
- `wasm replay psram-persistent-touch-sync`

Result: success. The reduced harness still reports a healthy runtime, a pool-backed external WAMR heap, and a disabled display host API. More importantly, persistent PSRAM init and touch both succeed on a fresh boot in `0082`, just as they did in the richer `0079` project.

### 2026-03-23 11:56 EDT

My first follow-up attempt at the instantiate case was flawed. I opened a new serial session and sent:

- `wasm instantiate-bare-keepalive return-42`
- `wasm replay psram-persistent-touch-sync`

That session rebooted the device on open, so the persistent probe buffer no longer existed. The resulting failure said the persistent probe was uninitialized. This was not the right comparison, and it is recorded here because it is exactly the kind of false trail that becomes invisible if the diary only records successful probes.

### 2026-03-23 12:01 EDT

Ran the correct same-boot sequence in a single serial session:

- `wasm replay psram-persistent-init`
- `wasm instantiate-bare-keepalive return-42`
- `wasm replay psram-persistent-touch-sync`

Result: reproduced the crash in the reduced harness. Fresh persistent init still succeeds, `instantiate-bare-keepalive return-42` still succeeds, and the later persistent PSRAM write still crashes with `Cache disabled but cached memory region accessed`.

The decoded backtrace now lands entirely inside `0082`’s reduced PSRAM probe path:

- `TouchPersistentPsramProbe(...)` at `main/wasm_replay_control.cpp:209`
- `TouchPersistentPsramProbeWithCacheSync(...)` at `main/wasm_replay_control.cpp:308`
- `RunWasmReplayControlExample(...)` at `main/wasm_replay_control.cpp:390`
- `CmdWasm(...)` at `main/wasm_command.cpp:152`

That matters because it proves the old boundary survives even after removing all app-owned display behavior from the firmware.

## Related

- `../design/01-minimal-papers3-allocator-control-implementation-plan.md`
- `../../../../2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/index.md`
