# Changelog

## 2026-03-23

- Created the ticket to separate “why the direct embedded path breaks” from the already-working copy-before-load mitigation
- Read the WAMR loader and runtime string-handling path closely enough to identify a concrete candidate mechanism
- Elevated `wasm_const_str_list_insert(...)` source-buffer mutation under `is_load_from_file_buf=true` to the leading root-cause hypothesis
- Added direct-embedded proof commands in `0082` so the investigation can bypass the default copy-before-load mitigation on purpose
- Verified that `load-only-embedded-direct` still reproduces the old post-load PSRAM crash on PaperS3
- Verified that `load-only-embedded-direct-freeable` keeps the same embedded source pointer but no longer poisons later PSRAM touch
- Strengthened the explanation from “embedded flash-mapped load is bad” to “loader reuse/mutation of the original source buffer is the leading root-cause candidate”
- Added `empty-module.wasm` as a stringless embedded control and verified that direct embedded load alone is not sufficient to trigger the bug
- Instrumented `wasm_const_str_list_insert(...)` in the local WAMR component and preserved the ignored diff under `scripts/wamr-patches/01-wasm-runtime-const-str-trace.diff`
- Verified on PaperS3 that the failing `load-only-embedded-direct return-42` path hits two in-place const-string rewrites before the later PSRAM crash
- Verified that the successful `load-only-embedded-direct-freeable return-42` path avoids those in-place rewrite logs
- Tied the two rewrite lengths directly to the `return-42.wasm` export strings `run` and `memory`
- Added a narrower loader proof patch that forces `reuse_const_strings = false` while keeping the plain direct embedded `runtime-load` path
- Verified on AtomS3R that the patched plain direct embedded `runtime-load return-42` path no longer crashes on later PSRAM touch
- Strengthened the explanation from “source-buffer mutation is the leading candidate” to “source-buffer const-string mutation is the critical mechanism”
- Added a long-form intern-facing postmortem/report that explains the whole system, the narrowing strategy, the exact failing loader branch, the proof ladder, and the recommended production/upstream follow-up

## 2026-03-23

- Initial workspace created
