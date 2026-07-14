# Tutorial 0107 — PaperS3 Independent EPD_Painter Control

This project is the independent direct-driver control for ticket `ESP-50-PAPERS3-EREADER-PRIMITIVES`. It tests whether an unchanged EPD_Painter HIGH waveform family can produce a different optical endpoint from M5GFX on the same PaperS3.

## Current gate

P0.15 intentionally boots into `BOOT_LOCKED` with only:

```text
epd help
epd status
```

No panel waveform or power-on operation runs at boot. Physical commands are added only in P0.16 after the hardened driver builds and its source/build evidence is reviewed. Do not flash this project with ad hoc commands; use the ticket scripts.

## Fixed inputs

- ESP-IDF 5.4.2 from `/home/manuel/esp/esp-idf-5.4.2`
- ESP32-S3 target with octal PSRAM
- native USB Serial/JTAG console
- EPD_Painter commit `753c521da8aef59756df07c1a4eb88f1c64c8227`
- explicit `EPD_PAINTER_PRESET_M5PAPER_S3`
- pure ESP-IDF; no Arduino, Adafruit_GFX, M5GFX, or M5Unified
- HIGH waveform tables unchanged from upstream
- 1000 Hz FreeRTOS tick so 4 ms and 8 ms waveform delays are not truncated

## Reproduce the component

From the repository root:

```bash
T=ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api
$T/scripts/11-prepare-epd-painter-control.sh
```

The script verifies the ticket-owned upstream manifest, applies the reviewed patch with zero fuzz, proves that the preset/waveform file is byte-identical, and writes a prepared-source manifest.

## Build

Use the ticket build script once present. The underlying manual command is documented for review, not for evidence collection:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
rm -f sdkconfig
idf.py set-target esp32s3
idf.py build
```

`rm -f sdkconfig` is required when changing defaults because `sdkconfig.defaults` only seeds absent options.

## Safety boundary

The local patch changes only initialization correctness, resource checks, idle/power observability, pure-IDF compatibility, and control-pin safe state. It does not change:

- PaperS3 pin assignment;
- waveform arrays;
- row padding;
- LCD clock configuration;
- scan order;
- hard-clear phase counts;
- quality delays.

The board remains on official FactoryTest V0.5 until task P0.17 explicitly authorizes a flash.
