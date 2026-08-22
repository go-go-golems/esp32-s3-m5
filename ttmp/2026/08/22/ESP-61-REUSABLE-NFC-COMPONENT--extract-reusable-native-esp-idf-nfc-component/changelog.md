# Changelog

## 2026-08-22

- Initial workspace created


## 2026-08-22

Created evidence-backed reusable NFC component architecture and intern implementation guide covering current-state mapping, public APIs, ownership, safety, ten implementation phases, migration, and test strategy

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/design-doc/01-reusable-native-esp-idf-nfc-component-analysis-design-and-intern-implementation-guide.md — Primary design and implementation deliverable
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/01-investigation-diary.md — Chronological evidence and design decisions

## 2026-08-22

Validated the ticket cleanly, committed the research package at 3e275482, and uploaded the index/design/diary PDF bundle to reMarkable at /ai/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/design-doc/01-reusable-native-esp-idf-nfc-component-analysis-design-and-intern-implementation-guide.md — Validated and delivered primary guide
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/01-investigation-diary.md — Validation, commit, and upload evidence

## 2026-08-22

Phase 0 started: printed overview and phase slips, passed the 0117 ESP-IDF 5.5.4 build and all 0115 trace host tests, recorded dependency revisions, and preserved the active serial-monitor blocker; hardware acceptance remains open

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/01-investigation-diary.md — Step 4 implementation narrative
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/02-phase-0-baseline-evidence.md — Phase 0 partial baseline and blocker evidence

## 2026-08-22

Phase 0: released the stale monitor, restored reader mode, and ran the fresh read-only probe; result was zero detected tags, so hardware acceptance still requires placing the NTAG215

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/01-investigation-diary.md — Step 4 updated with monitor release and no-tag probe
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/02-phase-0-baseline-evidence.md — Updated Phase 0 hardware status

## 2026-08-22

Phase 1 complete: created dependency-free gogolem_nfc component (types, Result<T>, version), host tests pass, and examples/nfc_types_smoke builds cleanly under ESP-IDF 5.5.4

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/components/gogolem_nfc/include/gogolem/nfc/types.hpp — Domain types
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/examples/nfc_types_smoke/CMakeLists.txt — Component integration smoke
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/01-investigation-diary.md — Step 5 Phase 1 implementation

## 2026-08-22

Phase 2 partial: added host-testable lifecycle state machine and 4K-aware safety validators; all four host test suites pass and the smoke project rebuilds under ESP-IDF 5.5.4; M5Unit-NFC Engine wiring and WUPA hardware validation remain

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/components/gogolem_nfc/include/gogolem/nfc/lifecycle.hpp — Lifecycle rules
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/components/gogolem_nfc/include/gogolem/nfc/safety.hpp — Safety validators
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/reference/01-investigation-diary.md — Step 6 Phase 2 pure-logic subset
