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

