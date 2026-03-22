# Changelog

## 2026-03-22

- Initial workspace created
- Added the AtomS3R cross-device debugging guide, detailed task list, and initial diary entry framing the new control experiment.
- Built `0081-atoms3r-wamr-probe-console` by combining the AtomS3R display path from `0013` with the minimal WAMR runtime path from `0079`.
- Added reusable `ESP-40` scripts for USB candidate discovery, command capture, and stable USB Serial/JTAG flashing.
- Confirmed the connected board is `ESP32-S3-PICO-1` with embedded `8MB PSRAM` exposed as `303a:1001 Espressif USB JTAG/serial debug unit`.
- Reproduced the stock Espressif WAMR `os_mmap()` / linear-memory instantiate crash on AtomS3R for `wasm run-preflush return-42`.
- Applied the same local WAMR platform fixes already required by `0079` and recovered AtomS3R runtime execution.
- Verified on AtomS3R that `wasm run-preflush return-42`, `wasm run-preflush log-only`, `wasm replay hello-frame`, and `wasm run-preflush hello-frame` all succeed.
- Updated the working conclusion: the remaining live bug is no longer explained by generic ESP32-S3 WAMR bring-up, because the same runtime path succeeds on AtomS3R after the platform fixes.
