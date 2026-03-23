# Changelog

## 2026-03-23

- Initial workspace created
- Added the first design note, task list, and diary for the embedded-buffer mapping phase
- Carried forward the `ESP-42` result that embedded `load-only` is still toxic on PaperS3 while copied-internal and copied-spiram source buffers are healthy
- Confirmed from source and ELF inspection that the embedded Wasm assets are linked via `EMBED_FILES` and land in `.flash.rodata` around `0x3c04f008`
- Added copy-backed `instantiate-bare` and `run` probe commands to extend the mitigation experiment beyond `load-only`
- Reached a temporary hardware block after the rebuild when the PaperS3 stopped enumerating on USB, so this slice currently ends before on-device validation of the new commands
- Recovered the hardware path, reflashed the updated image, and confirmed the new command surface on-device
- Verified that `instantiate-bare-copy-internal return-42` no longer poisons later persistent PSRAM touch
- Verified that `run-copy-internal return-42` also no longer poisons later persistent PSRAM touch
- Elevated "copy embedded Wasm into RAM before load" from a loader-only clue to a strong end-to-end mitigation candidate
- Added a build-time mitigation flag that rewrites the plain embedded-load path to copy into internal RAM before `wasm_runtime_load(...)`
- Caught and corrected a stale `sdkconfig.variant` issue after the first rebuild by forcing the new flag into the active variant config and verifying it with `wasm status`
- Verified that plain `wasm load-only return-42` now runs with `binary_source=copied-internal` and no longer poisons later persistent PSRAM touch
- Verified that plain `wasm instantiate-bare return-42` now runs with `binary_source=copied-internal` and no longer poisons later persistent PSRAM touch
- Verified that plain `wasm run return-42` now runs with `binary_source=copied-internal`, returns `42`, and no longer poisons later persistent PSRAM touch
