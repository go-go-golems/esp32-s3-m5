# Changelog

## 2026-03-23

- Created the ticket as the board/config comparison track
- Scoped it to environment differences rather than the direct loader-mutation hypothesis
- Added a reusable sdkconfig comparison script for AtomS3R vs PaperS3
- Confirmed that the major shared memory-mode settings are much closer than the bug history had implied
- Identified the clearest board-level distinction so far: AtomS3R uses `ESP32-S3-PICO-1-N8R8`, while PaperS3 uses `ESP32-S3R8` plus external 16 MB flash
- Ported the minimal direct-embedded root-cause proof surface into `0081` so AtomS3R can run the same `empty-module` / `return-42` comparison as PaperS3
- Preserved the ignored Atom-side WAMR const-string trace patch under `scripts/wamr-patches/01-wasm-runtime-const-str-trace.diff`
- Recorded two small build fixes needed to align the Atom PSRAM probe with the PaperS3 control helpers (`esp_mm`, `esp_memory_utils.h`, `esp_cache_private.h`)
- Confirmed a new board-specific blocker: normal USB Serial/JTAG console attach on AtomS3R currently resets the board into ROM download mode, so the cross-check is prepared but not yet completed

## 2026-03-23

- Initial workspace created
