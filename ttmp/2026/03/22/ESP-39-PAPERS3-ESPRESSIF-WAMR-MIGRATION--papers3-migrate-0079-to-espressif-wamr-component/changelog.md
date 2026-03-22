# Changelog

## 2026-03-22

- Initial workspace created
- Added the Espressif WAMR migration guide, task plan, and diary
- Switched `0079` from `bytecodealliance/wasm-micro-runtime` on git `main` to `espressif/wasm-micro-runtime` `2.4.0~1`, updated the app component alias to `espressif__wasm-micro-runtime`, and verified a clean `idf.py reconfigure build` on `ESP-IDF 5.3.4`
- The resolved lockfile now points at the Espressif registry package; the old generated `managed_components/bytecodealliance__wasm-micro-runtime` directory still exists locally but was not used by the successful build
