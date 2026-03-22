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
- [ ] Task 1.5: Commit replay-control implementation

- [ ] Task 2: Validate the control path on hardware
- [ ] Task 2.1: Flash the replay-control firmware to PaperS3
- [ ] Task 2.2: Run `wasm replay hello-frame`
- [ ] Task 2.3: Run `wasm run hello-frame`
- [ ] Task 2.4: Compare the failure or success modes and record the interpretation
- [ ] Task 2.5: Commit hardware-debugging docs

## Planned

- [ ] Task 3: Reduce the replay path further if the control path still crashes
- [ ] Task 3.1: Add a `clear-only` control example
- [ ] Task 3.2: Add a `single-rect` control example
- [ ] Task 3.3: Add a `present-only` control example
- [ ] Task 3.4: Re-run hardware tests and narrow the failing primitive
- [ ] Task 3.5: Commit narrowed replay findings

- [ ] Task 4: Reconcile the result with the WAMR integration strategy
- [ ] Task 4.1: If replay succeeds, compare control-path and WAMR-backed queue lifecycles
- [ ] Task 4.2: If replay fails, move investigation to the PaperS3 display/replay layer
- [ ] Task 4.3: Update the ticket summary, changelog, and diary
- [ ] Task 4.4: Upload the updated bundle to reMarkable again
