# Tasks

## Completed

- [x] Create ticket `ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION`
- [x] Write the replay-isolation implementation plan
- [x] Create the replay-isolation diary

## In Progress

- [x] Task 1: Add a WAMR-free replay control path for `hello-frame`
- [x] Task 1.1: Expose reusable host-command queue helpers from the existing host API
- [x] Task 1.2: Add a replay-control helper that mirrors the `hello-frame` guest sequence exactly
- [x] Task 1.3: Add console wiring for `wasm replay hello-frame`
- [x] Task 1.4: Verify `idf.py build`
- [x] Task 1.5: Commit replay-control implementation

- [x] Task 2: Validate the control path on hardware
- [x] Task 2.1: Flash the replay-control firmware to PaperS3
- [x] Task 2.2: Run `wasm replay hello-frame`
- [x] Task 2.3: Run `wasm run hello-frame`
- [x] Task 2.4: Compare the failure or success modes and record the interpretation
- [x] Task 2.5: Commit hardware-debugging docs

## Planned

- [ ] Task 3: Compare pre-cleanup and post-cleanup WAMR replay timing
- [x] Task 3.1: Add a diagnostic execution mode that flushes the queued frame before WAMR teardown
- [x] Task 3.2: Preserve the current post-cleanup execution mode for side-by-side comparison
- [x] Task 3.3: Rebuild and flash the diagnostic firmware
- [x] Task 3.4: Run the pre-cleanup mode and record whether the display replay still panics
- [ ] Task 3.5: Commit flush-timing findings

- [ ] Task 4: Reconcile the result with the WAMR integration strategy
- [ ] Task 4.1: Compare control-path and WAMR-backed queue lifecycles using the new timing data
- [ ] Task 4.2: Identify whether the unstable boundary is the WAMR call itself or WAMR teardown
- [ ] Task 4.3: Update the ticket summary, changelog, and diary
- [ ] Task 4.4: Upload the updated bundle to reMarkable again
- [ ] Task 4.5: Add a minimal no-display Wasm probe if the boundary remains ambiguous
