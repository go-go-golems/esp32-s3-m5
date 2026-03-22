# Tasks

## TODO

- [x] Task 1. Write the AtomS3R cross-device implementation guide, task breakdown, and diary framing for the new ticket.
- [x] Task 2. Create a new dedicated AtomS3R probe project from the known-good `0013` display/console base and remove unrelated GIF/storage logic.
- [x] Task 3. Port the minimal WAMR runtime service, console command surface, and embedded module registry from `0079`.
- [x] Task 4. Implement an AtomS3R-specific host display path and replay control path that can render the `hello-frame` sequence without invoking WAMR.
- [x] Task 5. Build the new AtomS3R probe project under `ESP-IDF 5.3.4` and fix configuration or dependency issues until the project is cleanly buildable.
- [x] Task 6. Identify the actual USB/serial mode exposed by the connected AtomS3R and capture the exact host-side workflow needed to flash and monitor it reproducibly.
- [x] Task 7. Flash the AtomS3R probe firmware and run the baseline matrix: `wasm status`, `wasm list`, `wasm run return-42`, `wasm run log-only`, `wasm replay hello-frame`, `wasm run hello-frame`.
- [x] Task 8. Compare the AtomS3R results against the PaperS3 results and decide whether the remaining instability is runtime-wide, display-class-wide, or PaperS3-specific.
- [x] Task 9. Record the debugging cycle in the diary, save helper scripts in the ticket `scripts/` directory, update changelog/task state, and commit in focused slices.
