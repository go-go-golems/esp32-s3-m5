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
LastUpdated: 2026-03-23T12:14:57-04:00
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

### 2026-03-23 12:11 EDT

Started the next control slice by turning `0082` into an allocator A/B harness instead of cloning yet another probe app. Added a dedicated Kconfig choice for WAMR allocator backing:

- `pool-spiram`
- `pool-internal`
- `system-allocator`

The point of this change is narrow and important. If the PaperS3 PSRAM fault disappears when WAMR stops using the SPIRAM-backed pool, then the pool itself becomes the main suspect. If the fault survives under the system allocator, then the problem is lower than "WAMR suballocates from an external RAM pool."

### 2026-03-23 12:15 EDT

While wiring the allocator choice into `wasm_runtime_service.cpp`, I also made the runtime status output more explicit:

- `allocator`
- `allocator_backing`
- `wamr.pool_buffer`
- `wamr.pool_size`

This was not cosmetic. The earlier investigation already showed how easy it is to make a false assumption about which allocator mode is actually running. The status output now tells us directly whether the build is:

- using a pool at all
- preferring SPIRAM for that pool
- or running with no WAMR pool buffer

### 2026-03-23 12:18 EDT

The first draft of the new system-allocator flash script had a build hygiene bug. It used `-B build-system-allocator` from the repo root, which created an untracked top-level build directory instead of keeping the alternate build under `0082`. I fixed that in two places before relying on the script:

- updated the script to use `${PROJECT_DIR}/build-system-allocator`
- expanded `.gitignore` to ignore generic `build-*` directories

This is exactly the sort of provenance issue worth recording. The allocator experiment itself was fine, but the build-output placement was sloppy on the first pass.

### 2026-03-23 12:24 EDT

The first full run of `flash_and_probe_allocator_system.sh` produced a misleading failure. The build and flash completed successfully, and the boot log clearly showed:

- `allocator=system-allocator`
- `backing=system`
- `host_api: Registered 2 host symbols`

But the wrapper script still exited with `serial_probe_sequence: prompt not observed before timeout`. I did not treat that as a product finding because the captured boot output already proved the image was alive and reached the console banner. This was a probe-window issue, not a firmware regression.

### 2026-03-23 12:28 EDT

To avoid another full rebuild just to fix a prompt timeout, I reran only the serial helper against the already-flashed system-allocator image with a longer `--prompt-timeout 12`. That direct run succeeded and gave the actual allocator A/B answer.

The single-boot command sequence was:

- `wasm status`
- `wasm replay psram-persistent-init`
- `wasm instantiate-bare-keepalive return-42`
- `wasm replay psram-persistent-touch-sync`

The status output confirmed the intended configuration:

- `allocator=system-allocator`
- `allocator_backing=system`
- `wamr.pool_buffer=0x0`
- `wamr.pool_size=0`
- `wamr.heap=unavailable-for-system-allocator`

That matters because it proves the run was not silently falling back to the old SPIRAM pool mode.

### 2026-03-23 12:31 EDT

The system-allocator result is decisive: the PaperS3 PSRAM crash still reproduces.

Fresh in the same boot:

- `wasm replay psram-persistent-init` succeeded

Then:

- `wasm instantiate-bare-keepalive return-42` succeeded

And finally:

- `wasm replay psram-persistent-touch-sync` still crashed with `Cache disabled but cached memory region accessed`

The logged state before the crash was especially useful:

- `replay_mem.flash_cache_enabled=yes`
- `replay_mem.internal_heap_ok=yes`
- `replay_mem.spiram_heap_ok=yes`
- `persistent_psram_probe.sync_err=ESP_OK`
- `wamr_memmap.ptr=0x3fca9b08`
- `wamr_memmap.external=no`

So the allocator A/B test rules out one more plausible explanation: the PaperS3 fault does **not** depend on WAMR owning a SPIRAM-backed runtime pool. The contamination boundary survives even when WAMR runs with the system allocator and no pool buffer at all.

### 2026-03-23 11:36 EDT

Started the next allocator comparison by adding the `sdkconfig.internal_pool` profile and a dedicated `flash_and_probe_allocator_internal.sh` wrapper. I also fixed both variant scripts to pass `-DSDKCONFIG=${BUILD_DIR}/sdkconfig.variant` instead of relying on `SDKCONFIG_DEFAULTS` alone. That change came directly from the earlier allocator confusion: a variant experiment is not trustworthy in this repo unless the build uses its own generated `sdkconfig.variant` file rather than inheriting from the shared project `sdkconfig`.

### 2026-03-23 11:38 EDT

The first internal-pool configure pass was finally valid at the Kconfig layer. I verified the generated files under `build-internal-pool/config/` before trusting anything else:

- `sdkconfig.json` showed `PAPERS3_WAMR_ALLOCATOR_POOL_INTERNAL=true`
- `sdkconfig.cmake` showed `CONFIG_PAPERS3_WAMR_ALLOCATOR_POOL_INTERNAL "y"`
- `sdkconfig.h` contained `#define CONFIG_PAPERS3_WAMR_ALLOCATOR_POOL_INTERNAL 1`

That matters because it separates "the variant selection is working" from "the implementation actually does what the variant name implies."

### 2026-03-23 11:40 EDT

The first internal-pool flash/probe attempt still hit the known serial-helper prompt issue. The wrapper flashed successfully and the boot log already showed `allocator=pool, backing=pool-internal`, but `serial_probe_sequence.py` timed out waiting for the prompt even though the prompt was visibly present in the captured stream. I did not count that as a firmware result. Instead, I reused the already-flashed image and ran the raw serial helper directly against the live prompt, the same workaround that had already proved reliable for the system-allocator pass.

### 2026-03-23 11:42 EDT

That first direct internal-pool run produced a very important negative result. The runtime status said:

- `allocator=pool`
- `allocator_backing=pool-internal`
- `wamr.pool_buffer=0x3c060a88`
- `wamr.pool_buffer_external=yes`
- `wamr.pool_size=524288`

### 2026-03-23 12:03 EDT

Started the load-path instrumentation slice by snapshotting the ignored vendor files before touching them. Because `managed_components/` is ignored, I added tracked copies and a sync checker under `ESP-42/scripts/wamr-local-debug-snapshots` first. That keeps the exact debug context reviewable later instead of hiding it in a dirty build tree.

### 2026-03-23 12:06 EDT

Added narrow loader-stage probes in the local Espressif WAMR sources around:

- `wasm_runtime_load(...)`
- `wasm_loader_load(...)`
- the inner `load(...)` path

The probes log heap integrity, free sizes, module pointer locality, and source-buffer locality at a few strategic stages. This was deliberate. At this point the wrong debugging move would have been "add logs everywhere"; the right move was to mark the last clearly healthy stage before the later PSRAM fault.

### 2026-03-23 12:08 EDT

Ran the embedded-buffer control on the internal-pool build. The important result was that the loader path looks healthy all the way through success:

- `wamr_rt_load.stage=enter`
- `wamr_loader.stage=loader-create-module-ok`
- `wamr_loader.stage=load-enter`
- `wamr_loader.stage=load-before-create-sections`
- `wamr_loader.stage=load-after-load-from-sections`
- `wamr_loader.stage=load-exit-ok`
- `wamr_loader.stage=loader-exit-ok`
- `wamr_rt_load.stage=exit-ok`

Heap integrity stayed good, and the later persistent PSRAM touch still crashed. So the new answer for Task 7 is: the last clearly healthy point is after loader success, not before it.

### 2026-03-23 12:10 EDT

Started the source-buffer-location experiment by extending `0082` with explicit binary-source selection:

- embedded bytes
- copied internal RAM
- copied SPIRAM

I threaded that state through `wasm_module_runner` and added new CLI commands for `load-only-copy-internal` and `load-only-copy-spiram`. The first build broke for a narrow reason: I had accidentally left two visible `BinarySourceName(...)` declarations in scope. I fixed that before running anything on hardware.

### 2026-03-23 12:11 EDT

Hit one operational trap worth preserving: I built the new image, but my first copied-buffer probe still printed the old usage text without the new commands. That immediately told me the latest image had not actually been flashed yet. I treated that run as invalid, reflashed explicitly, and then confirmed the new command surface with `wasm examples` before trusting any A/B result.

This is exactly the kind of error the diary is for. Without recording it, a later reader could easily misread the "copied-internal succeeded" result as if it had come from the same image as the earlier usage failure.

### 2026-03-23 12:12 EDT

The source-buffer-location experiment produced the clearest result of this ticket so far.

Fresh-boot sequence:

- `wasm replay psram-persistent-init`
- `wasm load-only-copy-internal return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success. The same later persistent PSRAM touch that crashes after embedded `load-only` now completes cleanly. The loader probes show `buf=0x3fc9c1a8`, `buf_external=no`, and `binary_source=copied-internal`.

### 2026-03-23 12:13 EDT

Ran the same fresh-boot sequence with a SPIRAM copy instead:

- `wasm replay psram-persistent-init`
- `wasm load-only-copy-spiram return-42`
- `wasm replay psram-persistent-touch-sync`

Result: also success. The loader probes show `buf=0x3c0a00f4`, `buf_external=yes`, and `binary_source=copied-spiram`, yet the later persistent PSRAM touch still succeeds. That immediately rules out the simplistic theory that "any RAM-backed source buffer works only if it is internal."

### 2026-03-23 12:14 EDT

Reran the embedded-buffer baseline on the same flashed image to make sure the comparison stayed fair:

- `wasm replay psram-persistent-init`
- `wasm load-only return-42`
- `wasm replay psram-persistent-touch-sync`

Result: crash reproduced again. So the live boundary is now much sharper:

- embedded flash-mapped Wasm buffer -> later persistent PSRAM write crashes
- copied internal-RAM Wasm buffer -> later persistent PSRAM write succeeds
- copied SPIRAM Wasm buffer -> later persistent PSRAM write succeeds

That strongly suggests the remaining PaperS3 bug is not "WAMR load from arbitrary memory poisons PSRAM." It is much closer to "WAMR load from the embedded flash-mapped module bytes poisons later PSRAM writes on PaperS3."

So the build configuration was correct, but the implementation was not actually forcing the WAMR pool into internal RAM. The relevant code path in `wasm_runtime_service.cpp` still used `heap_caps_malloc(..., MALLOC_CAP_8BIT)` for the internal variant, which is allowed to return external RAM on PaperS3 once PSRAM is part of the heap. That run still crashed on the final persistent PSRAM write, but I explicitly did **not** treat it as the final answer to the internal-pool question because the WAMR pool was still external in practice.

This was a good debugging lesson by itself: a variant name in logs is not enough. The runtime has to expose the actual buffer address and whether it is external or internal, or the experiment can still be mislabeled.

### 2026-03-23 11:44 EDT

To make the internal-pool variant real, I changed `wasm_runtime_service.cpp` so that:

- the default SPIRAM-pool variant keeps its `512 KiB` pool
- the internal-pool variant uses a smaller `128 KiB` pool that can fit in internal RAM
- the internal-pool allocator request now uses `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`

I committed that separately as `debug(papers3): enforce internal wamr pool variant` before rerunning the hardware test, so the code-change provenance stays separate from the eventual runtime conclusion.

### 2026-03-23 11:47 EDT

Reflashed the corrected internal-pool image. The wrapper still missed the prompt during boot-time probing, so I again reused the flashed image and ran the command sequence directly against the live prompt instead of mixing another rebuild into the evidence. This repetition is worth recording because it is now a recurring operational quirk of the USB Serial/JTAG setup for these stripped control harnesses: the probe wrapper is good for build+flash provenance, but the direct live-prompt helper is more reliable for the actual command transcript.

### 2026-03-23 11:49 EDT

The corrected internal-pool run is finally the trustworthy one. `wasm status` reported:

- `allocator=pool`
- `allocator_backing=pool-internal`
- `wamr.pool_buffer=0x3fca8350`
- `wamr.pool_buffer_external=no`
- `wamr.pool_size=131072`

That means the internal-pool variant is now genuinely using an internal-RAM WAMR pool instead of silently falling back to PSRAM.

The rest of the run was the important product result:

- `wasm replay psram-persistent-init` succeeded
- `wasm instantiate-bare-keepalive return-42` succeeded
- `wasm replay psram-persistent-touch-sync` still crashed with `Cache disabled but cached memory region accessed`

The state immediately before the crash was still clean by every high-level probe we have:

- `replay_mem.flash_cache_enabled=yes`
- `replay_mem.internal_heap_ok=yes`
- `replay_mem.spiram_heap_ok=yes`
- `persistent_psram_probe.sync_err=ESP_OK`

And WAMR's own internal-pool metrics still looked healthy after instantiate:

- `wamr.pool_buffer_external=no`
- `runtime_mem.wamr_pool_total=130880`
- `runtime_mem.wamr_pool_free=129752`
- `runtime_mem.wamr_pool_highmark=1488`

So the corrected internal-pool run rules out one more explanation: the PaperS3 PSRAM fault does **not** depend on WAMR using an explicit external-RAM pool. The failure survives under:

- default SPIRAM-backed WAMR pool
- system allocator with no WAMR pool buffer
- true internal-RAM WAMR pool

At this point, the remaining suspect is no longer "which allocator backing WAMR uses." The open boundary is deeper in the instantiate path or in a PaperS3-specific external-memory interaction that survives all three allocator layouts.

### 2026-03-23 11:52 EDT

With allocator backing now largely ruled out, I added two narrower lifecycle probes to `0082`:

- `wasm load-only <name>`
- `wasm load-only-keepalive <name>`

The purpose was to split `wasm_runtime_load(...)` from `wasm_runtime_instantiate(...)`. Up to this point we already knew runtime init alone was safe and instantiate was toxic, but we did **not** yet know whether the fault actually began at module load/parse and only appeared later, or whether instantiate itself was the first dangerous step.

### 2026-03-23 11:55 EDT

Rebuilt and reflashed the corrected internal-pool harness with the new load-only commands. The wrapper still hit the same boot-time prompt-detection issue, so I again used the direct-against-live-prompt helper after flash. I am recording that repetition explicitly because this workflow is now stable enough to be considered the normal operating procedure for these stripped USB Serial/JTAG probes:

1. use the wrapper for build+flash provenance
2. use the direct serial helper for the actual single-boot command transcript

### 2026-03-23 11:57 EDT

Ran the first new same-boot load-only sequence:

- `wasm status`
- `wasm replay psram-persistent-init`
- `wasm load-only return-42`
- `wasm replay psram-persistent-touch-sync`

This result is important: the crash still reproduced.

`wasm load-only return-42` reported:

- `invocation_mode=load-only`
- `execution=success`
- `loaded=yes`
- `instantiated=no`
- `exec_env=no`
- `executed=no`

So the later persistent PSRAM write now crashes even though WAMR never instantiated the module, never created an exec env, and never called guest code. That moves the fault boundary earlier than every previous result in this ticket.

### 2026-03-23 12:00 EDT

I then ran the paired cleanup control:

- `wasm status`
- `wasm replay psram-persistent-init`
- `wasm load-only-keepalive return-42`
- `wasm replay psram-persistent-touch-sync`

This also crashed.

That point matters because it removes another plausible explanation. If `load-only` had crashed but `load-only-keepalive` had not, then the immediate unload/cleanup path would have become the main suspect. Instead, both variants reproduce the fault:

- `load-only` poisons PSRAM
- `load-only-keepalive` also poisons PSRAM

So the remaining interpretation is much narrower: on PaperS3, `wasm_runtime_load(...)` itself is already sufficient to poison later PSRAM writes, even when:

- WAMR is using a true internal-RAM pool
- the module is never instantiated
- no exec env is created
- no guest code runs
- and immediate unload is skipped

That is the strongest reduction we have achieved so far in this whole investigation.

## Related

- `../design/01-minimal-papers3-allocator-control-implementation-plan.md`
- `../../../../2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/index.md`
