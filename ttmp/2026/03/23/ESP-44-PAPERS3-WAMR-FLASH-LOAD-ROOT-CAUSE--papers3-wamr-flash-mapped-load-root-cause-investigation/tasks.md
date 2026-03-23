# Tasks

## Objective

Determine whether the surviving PaperS3 embedded-load bug is specifically caused by WAMR reusing and mutating the original source buffer for const strings and related loader metadata.

## Task List

- [x] Task 1: Read the WAMR loader path deeply enough to replace the generic flash-access theory with a concrete code-level hypothesis.
- [x] Task 1.1: Trace `wasm_runtime_load()` -> `wasm_loader_load()` -> `load_from_sections()`.
- [x] Task 1.2: Identify any paths that preserve or mutate pointers into the original source buffer.
- [ ] Task 2: Build a targeted PaperS3 experiment that uses the embedded direct buffer but disables source-buffer reuse.
- [x] Task 2.1: Add an experimental command path in `0082` that bypasses the default copy-before-load mitigation and uses `wasm_runtime_load_ex(...)`.
- [x] Task 2.2: Force `LoadArgs.wasm_binary_freeable = true` for that experimental path so const strings are cloned instead of rewritten in place.
- [x] Task 2.3: Run the same post-load PSRAM-touch sequence and compare it against the known-bad direct embedded path.
- [x] Task 3: Document whether the result is strong enough to treat “source-buffer mutation” as the likely root cause.
- [ ] Task 3.1: Decide whether `0082` should keep the simpler copy-before-load mitigation or switch to a narrower direct-embedded `load_ex` fix.
- [ ] Task 4: Keep the diary current and preserve any helper scripts in this ticket’s `scripts/` directory.

## TODO

- [ ] Add tasks here
