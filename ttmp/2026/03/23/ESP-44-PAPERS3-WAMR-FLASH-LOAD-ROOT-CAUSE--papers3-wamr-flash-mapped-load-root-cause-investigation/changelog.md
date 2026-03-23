# Changelog

## 2026-03-23

- Created the ticket to separate “why the direct embedded path breaks” from the already-working copy-before-load mitigation
- Read the WAMR loader and runtime string-handling path closely enough to identify a concrete candidate mechanism
- Elevated `wasm_const_str_list_insert(...)` source-buffer mutation under `is_load_from_file_buf=true` to the leading root-cause hypothesis
- Added direct-embedded proof commands in `0082` so the investigation can bypass the default copy-before-load mitigation on purpose
- Verified that `load-only-embedded-direct` still reproduces the old post-load PSRAM crash on PaperS3
- Verified that `load-only-embedded-direct-freeable` keeps the same embedded source pointer but no longer poisons later PSRAM touch
- Strengthened the explanation from “embedded flash-mapped load is bad” to “loader reuse/mutation of the original source buffer is the leading root-cause candidate”

## 2026-03-23

- Initial workspace created
