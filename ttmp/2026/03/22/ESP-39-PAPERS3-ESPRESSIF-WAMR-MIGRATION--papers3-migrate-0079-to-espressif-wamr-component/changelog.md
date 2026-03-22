# Changelog

## 2026-03-22

- Initial workspace created
- Added the Espressif WAMR migration guide, task plan, and diary
- Switched `0079` from `bytecodealliance/wasm-micro-runtime` on git `main` to `espressif/wasm-micro-runtime` `2.4.0~1`, updated the app component alias to `espressif__wasm-micro-runtime`, and verified a clean `idf.py reconfigure build` on `ESP-IDF 5.3.4`
- The resolved lockfile now points at the Espressif registry package; the old generated `managed_components/bytecodealliance__wasm-micro-runtime` directory still exists locally but was not used by the successful build
- First hardware smoke test showed a different failure boundary than the upstream integration: `wasm run-preflush return-42` now panics during `wasm_runtime_instantiate()` in Espressif's `espidf_memmap.c` rather than later in PaperS3 replay, which makes the active debugging target the WAMR memory-mapping path instead of the display path
- Comparing the old and new managed components revealed that the stock Espressif migration had reintroduced two PaperS3-incompatible platform assumptions: `WASM_MEM_DUAL_BUS_MIRROR=1` on ESP32-S3 and `pthread_self()` in `os_self_thread()` for console-driven execution
- Restoring the old project-specific platform behavior in Espressif's `shared_platform.cmake` and `espidf_thread.c` recovered the runtime baseline: `wasm run-preflush return-42` and `wasm run-preflush log-only` now succeed on hardware
- Added a repeatable ticket-local probe wrapper at `scripts/flash_and_probe_wasm.sh` so hardware validation no longer depends on tmux pane state or manual monitor cleanup
- After the restored platform patches, the remaining hardware failure boundary is back where the older ticket family had it: `wasm run-preflush hello-frame` crashes in the PaperS3 preflush/display path (`FlushWasmHostFrame` -> `PaperCanvasScreenClear` -> `M5GFX Panel_EPD`)
