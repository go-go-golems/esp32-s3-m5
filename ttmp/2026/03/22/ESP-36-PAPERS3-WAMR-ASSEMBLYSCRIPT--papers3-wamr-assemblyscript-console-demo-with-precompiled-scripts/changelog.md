# Changelog

## 2026-03-22

- Initial workspace created
- Added vocabulary support for `papers3`, `wasm`, and `assemblyscript`
- Reviewed local PaperS3, `esp_console`, and prior script-runtime references
- Authored the main intern-facing design and implementation guide
- Added the investigation diary and task breakdown
- Scaffolded `0079-papers3-wamr-assemblyscript-console` with `ESP-IDF 5.3.4`, PaperS3 donor components, USB Serial/JTAG console defaults, and a placeholder `wasm` command family
- Verified a clean local `idf.py build` after forcing the shell back onto the `ESP-IDF 5.3.4` environment
- Added `wasm_runtime_service.*`, initialized WAMR during app startup, and replaced placeholder runtime status output with real version/mode/heap reporting
- Fixed the first runtime-service build breakage around `esp_system.h`, disabled `CONFIG_WAMR_ENABLE_AOT`, and strict `PRIu32` formatting
- Added the host-side `wasm-src/` AssemblyScript workspace, shared `host` import declarations, and five first-pass demo programs
- Added `tools/build-wasm-demos.mjs`, pinned `assemblyscript@0.28.10`, and generated tracked `release` and `debug` `.wasm`/`.wat` demo artifacts under `wasm-build/`
