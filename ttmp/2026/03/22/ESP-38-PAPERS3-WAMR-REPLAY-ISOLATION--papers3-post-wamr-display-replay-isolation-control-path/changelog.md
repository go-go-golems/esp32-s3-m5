# Changelog

## 2026-03-22

- Initial replay-isolation workspace created
- Added the implementation plan, task list, and detailed diary for the WAMR-free PaperS3 replay baseline
- Task 1: added a host-side `hello-frame` replay control path, reusable host-command queue helpers, and `wasm replay <name>` console wiring in `0079`; verified with `idf.py build`
- Task 2: flashed the firmware to PaperS3 and confirmed `wasm replay hello-frame` succeeds while `wasm run hello-frame` still panics inside `FlushWasmHostFrame()` after the WAMR path returns
- Task 3: added `wasm run-preflush <name>` and confirmed the panic still occurs even when queued display replay happens before WAMR teardown, shifting suspicion away from cleanup and toward the WAMR execution/native-import boundary itself

## 2026-03-22

- Initial workspace created
