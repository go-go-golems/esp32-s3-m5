# Tasks

## In Progress

- [x] Task 1: Create a focused PaperS3 `Panel_EPD` debugging slice that starts from the existing `ESP-39` baseline instead of reopening generic WAMR questions
- [x] Task 1.1: Document the exact crash choke point and the immediate code paths on both the app side and the M5GFX side
- [x] Task 1.2: Decide the smallest useful instrumentation set for `Panel_EPD.cpp`
- [x] Task 1.3: Add reusable ticket-local probe notes and keep the diary current
- [x] Task 3.4: Reframe the active hypothesis after the PSRAM scratch probe and decide whether the next slice belongs in `Panel_EPD`, WAMR platform code, or a broader PaperS3 memory/cache boundary
- [ ] Task 3.5: Inspect the active WAMR ESP-IDF memory/cache path for operations that can leave PaperS3 PSRAM writes in a bad state after a successful Wasm call
- [x] Task 3.5: Inspect the active WAMR ESP-IDF memory/cache path for operations that can leave PaperS3 PSRAM writes in a bad state after a successful Wasm call
- [x] Task 3.6: Compare the new headless PaperS3 same-boot PSRAM result against the earlier headful PaperS3 repro and decide whether display initialization is required for the contamination
- [x] Task 3.7: Decode the headless same-boot PSRAM crash against the exact headless ELF and map it back to the new control path
- [x] Task 3.8: Add an `instantiate-only` Wasm lifecycle probe so we can split instantiate/teardown contamination from actual guest execution contamination
- [x] Task 3.9: Use the headless `instantiate-only` probe with `psram-scratch` to decide whether `call_wasm` is required for the repro
- [x] Task 3.10: Add a pre-exec-env lifecycle probe so we can split module instantiation contamination from exec-env creation contamination
- [x] Task 3.11: Use the headless pre-exec-env probe with `psram-scratch` to decide whether `wasm_runtime_create_exec_env(...)` is required for the repro
- [x] Task 3.12: Decide whether one more local lifecycle split before export lookup is worth the cost, or whether the next best move is direct inspection of WAMR instantiate/deinstantiate internals
- [x] Task 3.13: Add a post-instantiate/pre-lookup probe so we can split module instantiate/deinstantiate contamination from export lookup/signature logic
- [x] Task 3.14: Use the headless post-instantiate/pre-lookup probe with `psram-scratch` to decide whether export lookup is required for the repro
- [x] Task 3.15: Move from runner-side splits into direct WAMR instantiate/deinstantiate instrumentation now that the required boundary is effectively `wasm_runtime_instantiate(...)` plus cleanup
- [x] Task 3.16: Record what the direct WAMR memory instrumentation proved about allocation locality and cleanup ordering on PaperS3
- [x] Task 3.17: Add an `instantiate-bare-keepalive` probe so we can decide whether cleanup is required for the contamination
- [x] Task 3.18: Use the keepalive probe with `psram-scratch` to decide whether instantiate alone is sufficient
- [x] Task 3.19: Add RAM-locality control probes so we can split allocator metadata corruption from broader PSRAM/cache poisoning
- [x] Task 3.20: Compare post-instantiate writes into internal RAM, newly allocated PSRAM, and preallocated PSRAM buffers on the same boot
- [x] Task 3.21: Add direct cache-enabled and heap-integrity probes around instantiate and replay control paths
- [x] Task 3.22: Decide whether those state probes expose a simple stuck-cache or heap-corruption state before the PSRAM crash
- [x] Task 3.23: Add an explicit `esp_cache_msync(...)` replay control around the poisoned persistent PSRAM buffer and check whether cache sync repairs the crash boundary
- [x] Task 3.24: Decide whether a successful `esp_cache_msync(...)` call changes the failure mode, fixes the write, or leaves the same direct CPU-write crash in place
- [x] Task 3.25: Add a cache-line-aligned persistent PSRAM control buffer and test whether `32 B` alignment plus `DIR_M2C` sync changes the PaperS3 post-instantiate failure
- [x] Task 3.26: Decide whether the surviving crash still reproduces on a cache-line-aligned PSRAM buffer after a successful pre-touch `DIR_M2C` sync
- [x] Task 3.27: Read and map the active WAMR instantiate/deinstantiate call graph before adding more low-level logs
- [x] Task 3.28: Add structured runtime, instantiate, and linear-memory trace logs at the smallest ownership-transition points
- [x] Task 3.29: Preserve the ignored WAMR source edits as tracked snapshots in the ticket workspace before the next hardware run

## Planned

- [x] Task 2: Instrument the PaperS3 EPD write path around `writeFillRectPreclipped(...)`
- [x] Task 2.1: Add bounded diagnostics for `_buf`, row stride math, rect bounds, and display mode at the first draw after reset
- [x] Task 2.2: Make the diagnostics safe enough that they do not themselves swamp the crash path
- [x] Task 2.3: Rebuild `0079` with the instrumented M5GFX nested repo
- [x] Task 2.4: Flash the attached PaperS3 and rerun the smallest useful probes
- [x] Task 2.5: Decode the current same-boot contamination crash against the exact `0079` ELF and map it back to `Panel_EPD` and app bridge code
- [x] Task 2.6: Add one tighter replay-after-WAMR probe around the first `Panel_EPD` entry after a successful `return-42`
- [x] Task 2.7: Compare the fresh-boot-success and post-WAMR-crash paths and write down the first concrete divergence
- [x] Task 2.8: Add a non-display PSRAM scratch-write control probe and compare it against the post-WAMR crash path
- [x] Task 2.9: Use that probe to split “general PSRAM poisoned after WAMR” from “PaperS3 EPD-specific path poisoned after WAMR”
- [x] Task 2.10: Rerun the `psram-scratch` control in the true headless PaperS3 build so we can test whether display initialization is required for the contamination
- [x] Task 2.11: Fix the `wasm replay` command gate so headless-compatible controls like `psram-scratch` are not blocked before execution

- [ ] Task 3: Compare the local `Panel_EPD.cpp` against newer upstream changes that plausibly affect PaperS3 refresh/write behavior
- [ ] Task 3.1: Recheck the local `M5GFX` history around `Panel_EPD.cpp`
- [ ] Task 3.2: Identify whether `fd824ee` or any later PaperS3-specific delta is safe to A/B locally
- [ ] Task 3.3: If justified, apply the smallest upstream-aligned patch and rerun the same probes

- [ ] Task 4: Decide whether the evidence now points to a broader PaperS3 PSRAM/cache issue, a WAMR/platform cleanup problem, or a remaining EPD-specific layer on top
- [ ] Task 4.1: Record what instrumentation proved and what it falsified
- [ ] Task 4.2: Commit the code slice and then the ticket diary/task/changelog slice
- [x] Task 4.3: Add a PaperS3 control build that skips app-owned M5 display bring-up entirely, so the remaining board-specific state can be separated from WAMR instantiate
- [x] Task 4.4: Preserve the ignored `managed_components` WAMR instrumentation as a tracked patch artifact in the ticket workspace
- [x] Task 4.5: Compare PaperS3 and AtomS3R board docs and local sdkconfig slices to identify any board-level external-memory topology differences worth prioritizing
