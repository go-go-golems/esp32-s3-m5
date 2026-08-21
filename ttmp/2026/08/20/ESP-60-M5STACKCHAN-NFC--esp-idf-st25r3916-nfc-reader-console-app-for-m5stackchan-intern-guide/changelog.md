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

