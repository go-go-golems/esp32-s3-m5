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
- Committed the recovered migration baseline as `e58a835` (`debug(papers3): recover espressif wamr baseline`)
- Added a detailed web-research brief for an external investigator, focused on Espressif WAMR platform assumptions, ESP32-S3 dual-bus mirroring, `os_self_thread()` expectations, and the remaining PaperS3 preflush/display crash
- Added a second ticket-local probe helper at `scripts/serial_probe_sequence.py` so multiple console commands can run against one boot session without reopening USB Serial/JTAG
- Clean-boot probes now show a more nuanced boundary: `wasm replay hello-frame` succeeds after reset, but `wasm run-preflush hello-frame` still crashes in `Panel_EPD::writeFillRectPreclipped`
- Same-boot sequence probes established the decisive new result: both `wasm run-preflush return-42` and `wasm run-preflush log-only` can succeed first and still poison a later `wasm replay hello-frame` in the same boot, which means the remaining PaperS3 failure is no longer limited to guest-issued drawing imports
- Committed the same-boot probe helper as `42205d7` (`debug(ticket): add same-boot serial probe helper`)
- Added an A/B worker-thread execution path plus reduced replay controls in `0079` so the same firmware can compare inline-vs-worker WAMR execution and `clear-only` vs `frame-no-clear` PaperS3 replays
- The worker-thread experiment falsified the current leading fix idea: `wasm run-preflush-worker return-42` still poisons a later `wasm replay hello-frame`, and `wasm run-preflush-worker hello-frame` still crashes in `Panel_EPD::writeFillRectPreclipped`
- The reduced replay controls further narrowed the PaperS3 side: after a successful worker-thread `return-42`, both `wasm replay clear-only` and `wasm replay frame-no-clear` crash, so the post-WAMR failure is broader than `screenClear` alone and still lands inside `Panel_EPD`
- Committed the worker/reduced-replay probe slice as `e91eaaf` (`debug(papers3): add worker and reduced replay probes`)
- Added a detailed intern-facing analysis of `M5GFX` `Panel_EPD`, its buffer/update/task model, and why it is the current leading interference point between PaperS3 display work and the surviving WAMR contamination bug
- Added a compile-time headless PaperS3 variant for `0079` with `CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK`, plus ticket-local build/flash helpers that use a dedicated headless `sdkconfig` file instead of silently inheriting the project `sdkconfig`
- The first headless probe failed for two useful reasons now captured in the diary: the disabled build initially broke because `CONFIG_PAPERS3_WASM_ENABLE_DISPLAY_STACK` was used as a normal C++ identifier in the `n` case, and the first build/flash scripts only set `SDKCONFIG_DEFAULTS`, which did not override the existing project `sdkconfig`
- The corrected headless build now produces a smaller binary (`0x81b30` vs `0x8d770` for the normal build), still links `M5GFX` / `M5Unified`, but no longer performs app-owned display initialization and only registers the non-display Wasm host imports at runtime
- Hardware validation on the attached PaperS3 now shows the intended headless baseline: `host_api.display=disabled`, `host_api.symbols=2`, `wasm run-preflush return-42` succeeds, and `wasm replay hello-frame` is rejected with `display host API is disabled in this build` instead of entering `Panel_EPD`
- Committed the headless baseline slice as `84197d8` (`debug(papers3): add headless wamr baseline`)
