# Changelog

## 2025-12-31

- Initial workspace created
- Created analysis document and diary. Documented I2S audio capture, web server options (ESPAsyncWebServer vs esp_http_server), storage patterns, and control API design. Related key reference files from EchoBase SDK, M5Cardputer, ATOMS3R-CAM-M12, and esp32-s3-m5 projects.
- Added design doc for the website audio recorder (MVP contract: WAV PCM16 mono @16kHz, REST API surface, state model). Populated actionable implementation tasks in tasks.md.
- Added waveform preview spec (`GET /api/v1/waveform`) and a dedicated API contract doc.
- Added an end-to-end manual verification playbook (connect → record → stop → list → download/play → delete).
- Landed a buildable ESP-IDF firmware baseline in `esp32-s3-m5/0021-atoms3-memo-website/` and documented build recovery/toolchain fixes in the diary (commit 5807ac2).
- Added WiFi STA-by-default (with SoftAP fallback) and made SPIFFS recordings path startup-safe for `0021` (commit bbedc38).
- Added AP+STA mode + per-request HTTP logging to debug network segmentation/timeouts (commit fef0c11).
- Reduced HTTP log spam (polling endpoints) and fixed recording start on SPIFFS by avoiding directory-only assumptions (commit 778c414).
- Reduced noisy ESP-IDF WiFi init logs to keep boot output readable (commit c91abca).
