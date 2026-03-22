# Tasks

## Completed

- [x] Create ticket `ESP-37-PAPERS3-WAMR-EXECUTION-PRIMITIVES`
- [x] Write the detailed execution-primitives analysis, design, and implementation guide
- [x] Create a detailed diary document for ongoing implementation notes

## In Progress

- [x] Task 1: Land the PaperS3 execution-primitives scaffolding in `0079`
- [x] Task 1.1: Add a PaperS3 canvas wrapper that owns display writes and present-mode mapping
- [x] Task 1.2: Add a WAMR host-API module with static `NativeSymbol` registration
- [x] Task 1.3: Add a module runner that loads, instantiates, executes, and tears down embedded modules
- [x] Task 1.4: Replace the placeholder `wasm run <name>` path with the new runner
- [x] Task 1.5: Verify `idf.py build`
- [x] Task 1.6: Commit execution scaffolding + diary update
- [x] Task 2: Harden the host ABI and execution reporting
- [x] Task 2.1: Add structured execution-result reporting for load, instantiate, execute, and exception failures
- [x] Task 2.2: Surface runner diagnostics in `wasm info` or `wasm status`
- [x] Task 2.3: Clamp drawing arguments and present modes defensively in the host API
- [x] Task 2.4: Verify a clean rebuild after host-ABI hardening
- [x] Task 2.5: Commit execution diagnostics + diary update

- [ ] Task 3: Validate the first runnable demo path on hardware
- [x] Task 3.1: Flash the updated firmware to PaperS3
- [x] Task 3.2: Run `wasm status`, `wasm list`, `wasm info hello-frame`, and `wasm run hello-frame`
- [ ] Task 3.3: Confirm the display updates and the run returns success without WAMR exception output
- [ ] Task 3.4: Record memory headroom and repeated-run behavior
- [ ] Task 3.5: Commit hardware-validation docs + diary update

## Planned

- [ ] Task 4: Expand the execution surface for the remaining demos
- [ ] Task 4.1: Smoke test `nested-boxes`, `bars`, `checkerboard`, and `radar-sweep`
- [ ] Task 4.2: Add any missing host primitives only if the existing demos genuinely require them
- [ ] Task 4.3: Update docs and command examples to reflect the runnable demo pack
- [ ] Task 4.4: Re-upload the final ticket bundle to reMarkable
- [ ] Task 4.5: Commit final docs/status updates
