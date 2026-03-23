# Tasks

## Objective

Determine why PaperS3 becomes unstable only when `wasm_runtime_load(...)` parses the embedded module bytes directly, while copied internal-RAM and copied SPIRAM buffers remain healthy.

## Task List

- [x] Task 1: Write the embedded-buffer investigation plan and carry forward the precise `ESP-42` boundary into this ticket.
- [x] Task 2: Map how the embedded Wasm assets are linked and surfaced to runtime in `0082`.
- [x] Task 2.1: Inspect `wasm_module_registry.cpp`, linker-generated symbols, and resulting pointer ranges for the embedded assets.
- [x] Task 2.2: Record whether the embedded bytes live in flash-mapped IROM/DROM space, another mapped region, or a copied RAM region.
- [x] Task 3: Compare the embedded-buffer path against explicit RAM-copy paths at the code level.
- [x] Task 3.1: Trace the exact `wasm_runtime_load(...)` call inputs for embedded, copied-internal, and copied-spiram modes.
- [x] Task 3.2: Record what actually differs besides pointer locality.
- [ ] Task 4: Test a narrow mitigation path that always copies embedded Wasm bytes into RAM before load.
- [ ] Task 4.1: Decide whether the mitigation should target internal RAM, SPIRAM, or a configurable choice.
- [ ] Task 4.2: Build and run the mitigation path on PaperS3 to confirm it removes the post-load PSRAM fault.
- [ ] Task 4.3: Extend the copied-buffer experiment beyond `load-only` so we know whether the mitigation also recovers `instantiate` and `run`.
- [ ] Task 5: Investigate whether a more explicit flash-mapping API path exists or is appropriate.
- [ ] Task 5.1: Compare the current embedded asset access path against ESP-IDF partition/mmap APIs.
- [ ] Task 5.2: Decide whether the real fix should be “copy before load” or “change how embedded bytes are mapped/accessed.”
- [ ] Task 6: Keep the ticket diary current with every debugging cycle and preserve any new helper scripts under this ticket’s `scripts/` directory.
