# Tasks

## Completed

- [x] Create ticket `ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION`
- [x] Inspect the current `0079` WAMR dependency layout
- [x] Write the migration guide
- [x] Create the migration diary

## In Progress

- [x] Task 1: Switch `0079` from the upstream WAMR package to Espressif's official component package
- [x] Task 1.1: Replace the upstream dependency in `main/idf_component.yml`
- [x] Task 1.2: Update `main/CMakeLists.txt` to depend on the new component alias
- [x] Task 1.3: Refresh the resolved dependency state so `dependencies.lock` reflects the new package
- [x] Task 1.4: Rebuild `0079` against `ESP-IDF 5.3.4`
- [x] Task 1.5: Record the outcome in the ticket and commit the migration slice
- [x] Task 2: Compare the migrated build surface against the previous upstream integration
- [x] Task 2.4: Decide whether the next ticket should be runtime A/B validation on hardware or additional static integration cleanup
- [ ] Task 2.5: Decide whether to explicitly clean the stale generated `bytecodealliance__wasm-micro-runtime` directory or leave it as a harmless local cache artifact
- [x] Task 3: Identify the first hardware regression introduced by the stock Espressif component
- [x] Task 3.1: Decode the instantiation-time panic boundary in `espidf_memmap.c`
- [x] Task 3.2: Compare the old and new ESP-IDF platform-layer files to find project-specific migration deltas
- [x] Task 3.3: Restore the PaperS3 interpreter-only `WASM_MEM_DUAL_BUS_MIRROR=0` behavior and re-test `return-42`
- [x] Task 3.4: Restore the console-safe `os_self_thread()` behavior and re-test `return-42`
- [x] Task 3.5: Replace the manual monitor loop with a repeatable scripted flash/probe path under `scripts/`
- [x] Task 3.6: Commit the recovered platform patches and scripted probe helpers
- [ ] Task 4: Re-establish the post-migration runtime baseline with probe modules
- [x] Task 4.1: Confirm trivial guest execution with `return-42`
- [x] Task 4.2: Confirm simple host-import execution with `log-only`
- [x] Task 4.3: Confirm that `hello-frame` still fails only in the PaperS3 preflush path
- [x] Task 4.4: Decide whether the next slice belongs in `ESP-39` or should be handed back to the replay-isolation/debugging ticket family
- [x] Task 4.5: Add a same-boot multi-command serial probe helper under ticket-local scripts
- [x] Task 4.6: Determine whether a successful non-drawing Wasm execution poisons later PaperS3 replay in the same boot
- [x] Task 4.7: Compare same-boot replay behavior after `return-42` and `log-only`
- [x] Task 4.8: Add a worker-thread Wasm execution path for A/B comparison against the inline console-task path
- [x] Task 4.9: Test whether worker-thread Wasm execution prevents same-boot replay poisoning on PaperS3
- [x] Task 4.10: Add reduced replay controls that isolate clear-only from frame-no-clear PaperS3 drawing
- [x] Task 4.11: Test clear-only and frame-no-clear after successful worker-thread Wasm execution

## Planned

- [x] Task 2.1: Confirm whether the Kconfig symbols consumed by `sdkconfig.defaults` and `wasm_runtime_service.cpp` remain compatible
- [x] Task 2.2: Record the exact resolved Espressif package version
- [x] Task 2.3: Note any new warnings, missing symbols, or build regressions introduced by the migration
