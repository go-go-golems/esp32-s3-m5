# Changelog

## 2026-03-23

- Initial workspace created
- Added the first design note, task list, and diary for the embedded-buffer mapping phase
- Carried forward the `ESP-42` result that embedded `load-only` is still toxic on PaperS3 while copied-internal and copied-spiram source buffers are healthy
- Confirmed from source and ELF inspection that the embedded Wasm assets are linked via `EMBED_FILES` and land in `.flash.rodata` around `0x3c04f008`
- Added copy-backed `instantiate-bare` and `run` probe commands to extend the mitigation experiment beyond `load-only`
- Reached a temporary hardware block after the rebuild when the PaperS3 stopped enumerating on USB, so this slice currently ends before on-device validation of the new commands
