# Tasks

## TODO

- [x] Research + write intern design doc (analysis only) <!-- t:n4l3 -->
- [x] Implement Phase 1: standalone ESP-IDF NFC reader (ST25R3916 component + esp_console commands) <!-- t:2t3u -->
- [x] Flash to /dev/ttyACM0, verify nfc-scan/probe/read on a real NTAG <!-- t:tr32 -->
- [ ] Verify NFC power-enable path against body schematic (open question) <!-- t:o65r -->
- [ ] Phase 2: integrate as Mooncake app with LVGL UID display <!-- t:2nrd -->
- [x] Upload guide bundle to reMarkable <!-- t:pbpb -->
---

- [x] UI-0: pinned StackChan overlay and serialized NFC service <!-- t:41zj -->
- [x] UI-1: 320x240 app shell and Reader page <!-- t:pddn -->
- [x] UI-2: Bus and RF/IRQ diagnostic pages <!-- t:bmxl -->
- [ ] UI-3: Register matrix and event log <!-- t:grvn -->
- [ ] UI-4: auto-poll lifecycle and stability validation <!-- t:rvng -->
- [ ] NFC Debug UI final hardware verification and write-up <!-- t:ruz0 -->
- [x] Research ESP-IDF/M5 I2C failure behavior and publish intern debugging implementation guide <!-- t:mu8t -->
- [x] Add structured NFC transport/service serial logging and validate on ESP-IDF 5.5.4 <!-- t:vmwz -->
- [x] Instrument and rerun official Arduino Detect.ino for transaction-level comparison with NFC LAB <!-- t:2hyl -->
- [x] Add continuous Arduino NFC polling with on-screen event log and compact serial summaries <!-- t:ybrj -->
- [x] Display multiple Arduino NFC tags and retain a UID-deduplicated seen-device registry <!-- t:3snv -->
- [x] Publish textbook deep-dive project report on the Arduino-to-ESP-IDF NFC porting batch to go-go-parc <!-- t:tdpt -->
- [x] Design apples-to-apples ESP-IDF instrumentation against the Arduino M5 I2C trace and diagnose the backend divergence <!-- t:fl3w -->
- [x] Phase 1: ESP-IDF trace data model (st25r_trace) with host tests for wraparound + first-error freeze <!-- t:6q8e -->
- [x] Phase 2: instrument standalone 0115 transport (rd8/wr8/direct_cmd/fifo/space-b) with observer-safe recording <!-- t:tl1q -->
- [x] Phase 3: first-error freeze + nfc-trace console commands (clear/status/dump/first-error/mode) <!-- t:2cn6 -->
- [x] Phase 4: driver DEBUG diagnostic build config to confirm I2C_EVENT_NACK <!-- t:d6he -->
- [x] Phase 5: Arduino/ESP-IDF normalized comparison script + report <!-- t:y7yo -->
- [x] Apply the I2C FSM-reset diagnostic patch to the local ESP-IDF copy, rebuild, reflash, measure NACK rate <!-- t:7j5c -->
- [ ] Fresh base-principles investigation: isolate transport, RF request, anticollision, selection, and UID behavior with four connected tags <!-- t:89gi -->
- [x] Inventory official StackChan NFC sketches and map each capability to native ESP-IDF APIs <!-- t:cfa2 -->
- [x] Create pure ESP-IDF M5StackChan NFC feature explorer on the existing I2C bus with safe console commands <!-- t:80jn -->
- [x] Implement read-only quick scan, identification, raw dump, and NDEF inspection equivalents <!-- t:lcpm -->
- [x] Implement guarded raw write, NDEF write, and MIFARE Classic value-block equivalents <!-- t:5ki3 -->
- [x] Implement reboot-selectable Ultralight and NTAG213 emulation modes <!-- t:3r5e -->
- [x] Build all feature modes with ESP-IDF 5.5.4 and validate read-only behavior on hardware <!-- t:9ex1 -->
- [ ] Validate 0117 Ultralight and NTAG213 emulation over RF with a phone or second NFC reader <!-- t:5n2a -->
- [ ] Validate guarded Type 2 write/NDEF replacement and Classic value-block operations on named sacrificial tags <!-- t:z5hl -->
