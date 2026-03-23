# Changelog

## 2026-03-23

- Initial workspace created
- Added the first reduced-firmware implementation plan, explicit task list, and debugging diary scaffold for the minimal PaperS3 allocator-control harness
- Reduced `0082` into a displayless allocator-control harness with only `return-42`, `log-only`, instantiate lifecycle commands, and PSRAM/internal-RAM replay probes
- Added reusable serial probe scripts under `ESP-42/scripts` for build/flash/probe and multi-command single-boot capture
- Built `0082` successfully against `ESP-IDF 5.3.4`
- Verified on attached PaperS3 that fresh-boot persistent PSRAM init/touch still succeeds in `0082`
- Verified on attached PaperS3 that `instantiate-bare-keepalive return-42` still poisons later persistent PSRAM touch in the same boot, reproducing the old boundary in the reduced harness
- Added allocator-backing A/B support to `0082` so the reduced harness can run with either the default SPIRAM-backed WAMR pool or a system-allocator variant
- Built, flashed, and probed the `sdkconfig.system_allocator` variant on attached PaperS3 using a separate `build-system-allocator` directory
- Verified that the same PaperS3 PSRAM crash still reproduces after `instantiate-bare-keepalive return-42` even when WAMR is running with `allocator=system-allocator`, `allocator_backing=system`, and `wamr.pool_buffer=0x0`
- Added the `sdkconfig.internal_pool` variant plus corrected per-build `sdkconfig.variant` handling for the allocator probe scripts
- Verified that the first internal-pool run was not yet trustworthy for the intended question because the implementation still allocated the WAMR pool from a generic `MALLOC_CAP_8BIT` heap and landed in external RAM
- Tightened the internal-pool implementation so it uses a smaller `128 KiB` pool allocated with `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`
- Built, flashed, and probed the corrected internal-pool variant on attached PaperS3
- Verified that the same PaperS3 PSRAM crash still reproduces after `instantiate-bare-keepalive return-42` even when WAMR is running with `allocator=pool`, `allocator_backing=pool-internal`, `wamr.pool_buffer_external=no`, and `wamr.pool_size=131072`
- Added explicit `wasm load-only` and `wasm load-only-keepalive` lifecycle probes to `0082`
- Built, flashed, and probed the updated internal-pool harness on attached PaperS3 with the same single-boot persistent-PSRAM sequence around `load-only` and `load-only-keepalive`
- Verified that `wasm_runtime_load(...)` alone is already sufficient to poison later PSRAM writes on PaperS3, and that immediate unload/cleanup is not required for the fault
- Added tracked WAMR loader snapshots plus loader-stage telemetry around `wasm_runtime_load(...)` and `wasm_loader_load(...)`
- Verified on attached PaperS3 that the embedded-buffer `load-only` path still reaches `load-exit-ok` and then poisons a later persistent PSRAM touch
- Added copied-buffer `load-only` variants that feed `wasm_runtime_load(...)` from an internal-RAM copy or a SPIRAM copy of the same Wasm bytes
- Verified on attached PaperS3 that `load-only-copy-internal return-42` no longer poisons later persistent PSRAM touch
- Verified on attached PaperS3 that `load-only-copy-spiram return-42` also no longer poisons later persistent PSRAM touch
- Narrowed the live trigger to the embedded flash-mapped Wasm source buffer path rather than generic `wasm_runtime_load(...)` parsing from arbitrary RAM
