# Changelog

## 2026-08-20

- Initial workspace created


## 2026-08-20

Created ticket + intern design doc (analysis only): ESP-IDF ST25R3916 NFC reader console app. Re-cloned StackChan/StackChan-BSP/M5Unit-NFC; saved sources (code excerpts, Kagi research, datasheet pointer). Decision: minimal from-scratch ESP-IDF driver (Option B) over porting M5UnitUnifiedNFC. Doc: 15 sections, 43KB. (commit fe9922b)
---



- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/design-doc/01-esp-idf-st25r3916-nfc-reader-console-app-analysis-design-and-implementation-guide.md — Primary deliverable

### Related Files

## 2026-08-20

Step 10: added ST25R3916 NRT/frame-wait timer before REQA/WUPA and anticollision; build passes, hardware validation awaiting tag placement (commit 74bc45f9)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c — NRT implementation and NFC-A integration
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916_regs.h — NRT register constants


## 2026-08-20

Step 11: live NRT probe produced RXS/RXE/COL; fixed reversed FIFO status byte order; post-reflash coupling remained intermittent and UID is not yet read (commit f8015daa)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c — Correct FIFO status byte order exposed by live RXE


## 2026-08-20

Step 12: matched M5 continuous-carrier polling; receive/collision events increased but FIFO remained empty, requiring tag-absent/present baseline (commit f183853b)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c — Continuous RF carrier across REQA/WUPA/poll


## 2026-08-20

Step 13: preserved official M5 sources/images, corrected IO and Space-B NFC-A initialization, and built/flashed official Detect.ino for bisect (commits 4ccc993e, d4136068, 0b87585c)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c — Corrected documented initialization
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/m5unit-nfc/README.md — Upstream source provenance


## 2026-08-21

Step 14: official Detect.ino read PICC, isolating remaining ESP-IDF failure to I2C transport; published and pushed 4,868-word Obsidian deep dive (vault commit e7003d4)

### Related Files

- /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/08/21/ARTICLE - M5StackChan NFC - Porting the ST25R3916 Reader to ESP-IDF.md — Published technical project report


## 2026-08-21

Step 15: corrected tag-absence record; valid tag-present 400/100 kHz tests both failed, 100 kHz was worse and reverted; vault report corrected (223bcd7)

### Related Files

- /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/08/21/ARTICLE - M5StackChan NFC - Porting the ST25R3916 Reader to ESP-IDF.md — Corrected test preconditions and 100 kHz result


## 2026-08-21

Step 16: archived full official NFC docs and designed a 320x240 Mooncake/LVGL diagnostic UI with ASCII screen sketches, worker/snapshot architecture, and phased implementation plan

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/design-doc/02-m5stackchan-nfc-debug-ui-320x240-lvgl-design.md — Debug UI architecture and ASCII screen sketches
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/web/04-m5stack-stackchan-nfc-full-official-doc.md — Full official StackChan NFC documentation snapshot


## 2026-08-21

Step 17 / UI-0: added pinned StackChan overlay, serialized NFC worker, shared-bus driver lifecycle, and successful full/idempotent builds (commit 50d7c151)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/nfc_debug_service.cpp — Single-owner NFC command and snapshot service
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/scripts/prepare.sh — Reproducible pinned firmware overlay


## 2026-08-21

Step 18 / UI-1: implemented exact 320x240 Reader page with queued touch actions, raw error states, UID/ATQA/SAK rendering, and reproducible source reconfiguration (commit 11d5f0e0)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/view/nfc_debug_view.cpp — Reader page LVGL implementation


## 2026-08-21

Step 19 / UI-2: instrumented every ST25R I2C transaction and added cooperative RF/IRQ sampling, register verification, Bus page, and RF/IRQ page (commit e7229ec9)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/st25r3916/st25r3916.c — Raw transaction and RF diagnostic evidence
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/view/nfc_debug_view.cpp — RF and Bus screens


## 2026-08-21

Step 20: converted to NFC-only auto-open firmware, reduced app by ~613 KiB, completed the first full physical flash, and captured live NFC.LAB execution (commit 51efbe4f)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/scripts/flash.sh — Full migration and later app-only flash workflow
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/scripts/prepare.sh — NFC-only app registry and source filtering


## 2026-08-21

Step 21: published and pushed the 5,093-word NFC LAB deep dive; documented that the rising UI counter represents low-level I2C failures, while synchronized tag-read capture remains pending (vault commit 83513da)

### Related Files

- /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/08/21/ARTICLE - M5StackChan NFC LAB - Building an On-Device NFC Diagnostic Firmware.md — Published NFC LAB architecture and physical deployment deep dive


## 2026-08-21

Step 22: localized first NFC LAB READ failure to Auxiliary Definition write 0x0A before REQA and established that M5 detect retries failed requests for up to one second


## 2026-08-21

Step 23: researched ESP-IDF/M5GFX/ST25R I2C behavior, preserved authoritative sources, published a 6,396-word intern transport-debugging guide, and uploaded it to reMarkable (commit 8c62d8ac)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/design-doc/03-st25r3916-i2c-transport-debugging-analysis-design-and-intern-implementation-guide.md — Intern analysis, experiment design, implementation phases, and acceptance gates
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/I2C-backend-source-provenance.md — Pinned ESP-IDF, M5GFX, and M5Unified source provenance


## 2026-08-21

Step 24: added structured USB serial diagnostics at transport and service layers, built with ESP-IDF 5.5.4, app-flashed NFC LAB, and captured a real pre-REQA register-0x02 failure (commit 9c9fa2e3)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/st25r3916/st25r3916.c — Every failed I2C transaction is now emitted with operation/key/error/timing
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/01-nfc-lab-structured-serial-runtime.log — Hardware proof of structured log output


## 2026-08-21

Step 25: instrumented the exact successful official Arduino path, traced a four-chip run, read UID 047BD44D9E6180, and measured 10,188 reported M5 I2C transactions with zero API-level failures (commits 04c8a7c2 and 73114a33)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/analysis/01-official-arduino-four-chip-i2c-trace-comparison.md — Trace method, empirical comparison, limitations, and backend recommendation
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/scripts/04-instrument-official-arduino-trace.py — Reproducible instrumentation


## 2026-08-21

Step 26: added and deployed a continuous Arduino WUPA poller with 320x240 scrolling logs; validated 49 cycles and 8,126 transactions with zero transport failures (commit 64cd7e94)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/arduino-trace/Detect-continuous-traced.cpp — Live screen and compact serial monitor
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/03-arduino-continuous-screen-runtime.log — Hardware validation


## 2026-08-21

Step 27: added bounded multi-tag collection and a UID-deduplicated persistent four-device screen registry; validated seen=4 across empty scans over 197 cycles (commits eca56bf6 and 32d64476)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/arduino-trace/Detect-continuous-traced.cpp — Multi-tag enumeration and persistent seen registry
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/04-arduino-persistent-four-device-registry.log — Hardware evidence


## 2026-08-21

Step 28: published and pushed a 5,321-word Obsidian deep dive synthesizing the Arduino-to-ESP-IDF NFC porting batch (vault commit 0e916a6)

### Related Files

- /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/08/21/ARTICLE - M5StackChan NFC - From Arduino Reference Firmware to an ESP-IDF Diagnostic System.md — Durable project report, measured comparison, architecture, and backend plan


## 2026-08-21

Step 29: designed observer-safe Arduino-comparable ESP-IDF transaction tracing and traced ESP_ERR_INVALID_STATE to the likely synchronous NACK/non-DONE path (commit 96965440)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/design-doc/04-esp-idf-instrumentation-for-arduino-comparable-st25r3916-transport-traces.md — Trace schema, first-error freeze, debug classification, hypothesis ranking, and experiment plan


## 2026-08-21

Step 30: implemented observer-safe st25r_trace ring (host-tested), instrumented standalone transport, added nfc-trace commands (commits 608c4029 d3e9ef70 ab259991)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r_trace/st25r_trace.c — Trace ring + first-error freeze + dump


## 2026-08-21

Step 31: full-flashed standalone 0115; live trace capture showed clean init (66/66) but 96/13816 silent transport failures during irq-wait polling (READ_A 0x1C INVALID_STATE 251us), masked by read_main_irq as no-tag

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/05-standalone-trace-runtime.txt — Live evidence


## 2026-08-21

Step 32: Phase 4 confirmed I2C_EVENT_NACK via driver DEBUG log (0 timeouts); annotated first error hint=NACK class=HOST_NACK (commit 8e0f97b1)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/nfc_console.c — nfc-i2c-debug command toggles i2c.master log level


## 2026-08-21

Step 33: Phase 5 comparison script proves apples-to-apples divergence (Arduino 2614/2614 OK on wire 0x5C vs ESP-IDF NACKs on same key)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/scripts/08-compare-arduino-espidf-traces.py — Comparison script


## 2026-08-21

Step 34: wrote intern guide 05 (I2C FSM-reset NACK diagnosis, 4202 words) and uploaded to reMarkable /ai/2026/08/21/ESP-60-M5STACKCHAN-NFC (commits 8cf879ad 88ab1fd5)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/design-doc/05-why-arduino-reads-nfc-tags-and-esp-idf-does-not-the-i2c-fsm-reset-diagnosis.md — Intern guide


## 2026-08-21

Step 35: FSM-reset diagnostic patch REFUTED on hardware (patched 1.80% worse + failures in req-setup; reverted 1.17% baseline restored); IDF source reverted, patch file retained (commit 597174a7)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/esp-idf-5.5.4-i2c-fsm-reset-diagnostic.patch — Refuted hypothesis patch


## 2026-08-22

Steps 36-39: restarted from base principles, fixed reversed TX/RX field-on semantics, proved four-tag RF response, ported bounded anticollision, and preserved reusable one-/four-tag probes (commits f580c1f9 7465a834 12dc880f 6e365ebc e9d22b1f)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/analysis/02-fresh-base-principles-reconstruction-of-the-esp-idf-st25r3916-failure.md — Fresh layered analysis


## 2026-08-22

Added pure ESP-IDF 0117 feature explorer covering all official StackChan NFC sketch families; identified the physical tag as NTAG215, read its full memory and empty valid NDEF state, validated both emulator boot profiles locally, and verified mutation guards (code commit 65e27591)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0117-m5stackchan-nfc-feature-explorer/main/nfc_explorer.cpp — Feature implementation
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/21-24-native-feature-explorer.provenance.md — Hardware evidence

