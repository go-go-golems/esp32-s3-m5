# Changelog

## 2026-03-22

- Initial workspace created
- Added the execution-primitives design guide, detailed task breakdown, and diary scaffold for the `0079` runner work
- Task 1: added the PaperS3 canvas wrapper, WAMR host API registration, structured module runner, and real `wasm run` execution path in `0079` (commit `1d6ebf2`)
- Task 2: hardened runtime diagnostics and execution plumbing for PaperS3 hardware bring-up, including pooled WAMR allocator mode, interpreter-oriented ESP-IDF platform adjustments, last-run status reporting, and queued host side-effect replay
- Task 3: hardware validation reached the real `hello-frame` execution path on-device, but remains blocked by a cache-disabled panic that now reproduces during queued host-frame replay on the PaperS3 display path
