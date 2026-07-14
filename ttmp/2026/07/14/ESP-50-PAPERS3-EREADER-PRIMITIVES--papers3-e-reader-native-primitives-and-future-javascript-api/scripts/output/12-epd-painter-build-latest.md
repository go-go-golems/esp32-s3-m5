---
Title: EPD Painter Control Build Evidence
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - esp-idf
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Latest reproducible no-flash build evidence for the hardened independent PaperS3 EPD control."
LastUpdated: 2026-07-14T20:52:20Z
WhatFor: "Verify exact toolchain, patched source, configuration, binary identity, and size before any hardware flash."
WhenToUse: "Regenerate after any firmware, patch, SDK configuration, or toolchain change."
---

# EPD_Painter control build evidence

- Build UTC: `20260714T205220Z`
- ESP-IDF: `ESP-IDF v5.4.2`
- Target: `esp32s3`
- Upstream EPD_Painter: `753c521da8aef59756df07c1a4eb88f1c64c8227`
- Local patch SHA-256: `89e34a7f24060763c3f38aae7d4aaceeb8773e112256f1d21200b4a11fd1557b`
- Prepared manifest SHA-256: `4b4d281e8db55e9b12ad75c2023ba627b03423ea8a198506205bdacddf55d439`
- sdkconfig.defaults SHA-256: `740cac96a2d01cf9121efc736ed9df50e4e82c39ec9d56b6eedd439523463616`
- generated sdkconfig SHA-256: `01f25e79f3ae8e59e6b36fc57a4bcdeb601b39a91c6fdd81fec6aa5dd7d16b3c`
- Application BIN: `433776 bytes`, SHA-256 `2791e8334e2dae02612cf57ef58437758420a8168487fde3994d4fc73f3c5135`
- ELF: `4733344 bytes`, SHA-256 `451b4ffa026217a7fe10ff545174e0d6c62dd92b1ba2e9817577a7411f983358`
- Bootloader SHA-256: `ef682245dadeebb76149f93a5436b8bdaef8aa2bab26e5143fdd5c5ad0c54363`
- Partition table SHA-256: `fd8026bff850ca0dee41c41305160317fffe604dda30a9bd5a701ac82d96fa17`
- Full build log: `ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/12-epd-painter-build-20260714T205220Z.log`
- Hardware modified: **no**

## Fixed safety configuration

~~~text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_FREERTOS_HZ=1000
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
EPD_PAINTER_PRESET_M5PAPER_S3=1
EPD_PAINTER_DISABLE_BOOTCTL=1
~~~
