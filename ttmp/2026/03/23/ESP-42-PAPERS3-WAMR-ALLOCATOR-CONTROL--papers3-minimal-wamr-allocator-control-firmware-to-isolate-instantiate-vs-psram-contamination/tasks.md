# Tasks

## Objective

Build a stripped PaperS3 control firmware that keeps only the pieces needed to answer one question: does WAMR module instantiate poison later PSRAM writes on PaperS3 even when the rest of the demo stack is removed?

## Task List

- [x] Task 1: Write the reduced-firmware plan and probe matrix in the ticket before changing code.
- [x] Task 2: Fork `0082-papers3-wamr-allocator-control` from the copied `0079` baseline into a truly minimal harness.
- [x] Task 2.1: Remove display bring-up, display host API registration, and display replay paths.
- [x] Task 2.2: Reduce embedded Wasm assets to the minimum needed control modules.
- [x] Task 2.3: Keep only console commands required for status, instantiate lifecycle probes, and PSRAM/internal-RAM touch probes.
- [x] Task 2.4: Update project docs and defaults so the firmware documents itself as a control harness rather than a demo app.
- [x] Task 3: Build the reduced firmware against `ESP-IDF 5.3.4`.
- [x] Task 4: Flash the attached PaperS3 and run the strict control matrix.
- [x] Task 4.1: Confirm PSRAM persistent-touch works with no module load or instantiate.
- [x] Task 4.2: Confirm runtime initialization alone does not poison PSRAM.
- [x] Task 4.3: Confirm bare instantiate still poisons later PSRAM touch, or record if the reduced app changes that boundary.
- [x] Task 5: Compare `0082` results against `0079` and decide whether the next step should be deeper WAMR instrumentation or a smaller non-WAMR control app.
- [x] Task 5.1: Add an allocator-backing A/B mode to `0082` so the same harness can run with the current SPIRAM-backed WAMR pool and a system-allocator variant.
- [x] Task 5.2: Build and flash the system-allocator variant using a separate build directory and explicit `sdkconfig` override.
- [x] Task 5.3: Run the same single-boot persistent-PSRAM repro sequence on the system-allocator variant and compare the boundary against the default SPIRAM-pool build.
- [x] Task 5.4: Record whether the PSRAM poisoning depends on WAMR's SPIRAM-backed pool, or survives when WAMR stops using that pool.
- [x] Task 5.5: Add an explicit internal-pool build/profile so `0082` can force WAMR onto an internal-RAM pool without touching the system allocator.
- [x] Task 5.6: Build and flash the internal-pool variant using its own build directory and `sdkconfig` override.
- [x] Task 5.6a: Verify at runtime that the internal-pool build does not silently allocate its WAMR pool in external RAM.
- [x] Task 5.6b: If the internal build still lands in external RAM, tighten the allocator implementation so it requests true internal memory with a pool size that actually fits.
- [x] Task 5.7: Run the same single-boot persistent-PSRAM repro sequence on the internal-pool variant and compare it against both the default SPIRAM-pool build and the system-allocator build.
- [x] Task 5.8: Record whether the PaperS3 fault survives when WAMR is prevented from using any explicit external pool at all.
