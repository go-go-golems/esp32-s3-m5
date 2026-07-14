# Changelog

## 2026-07-14

- Initial workspace created


## 2026-07-14

Completed the native-first PaperS3 e-reader research and design package: imported the JS API/studio, mapped local firmware evidence, verified current upstream M5GFX and MicroQuickJS constraints, added fourteen implementation phases, and preserved reproducible research scripts/snapshots.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/design-doc/01-papers3-e-reader-primitives-analysis-design-and-implementation-guide.md — Primary 63KB intern-grade architecture API and phased implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Chronological prompts evidence failures decisions and review guidance
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/00-research-log.md — Retroactive reproducibility trace for all research steps
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/README.md — Provenance index for imported and Defuddle source material


## 2026-07-14

Validated the completed package (frontmatter, diff checks, and clean docmgr doctor) and uploaded the index, design guide, diary, and phase list as a reMarkable bundle at /ai/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/design-doc/01-papers3-e-reader-primitives-analysis-design-and-implementation-guide.md — Primary document delivered in the bundle
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Validation and delivery evidence recorded in Step 5


## 2026-07-14

Expanded the 14 implementation phases into 146 detailed resumable subtasks, added tracking conventions and phase counts, and recorded the task seeding process in the diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Step 6 records the task breakdown and commit workflow
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/05-add-phase-tasks.sh — Idempotent task seed used to create the breakdown
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/tasks.md — Detailed milestone and subtask tracking for Phases 0-13

## 2026-07-14

Phase 0: added and live-tested the PaperS3 EPD qualification harness; Cell C passed boundary and sleep/wake machine checks but exposed quality-mode washed black/ghosting, so waveform policy remains open (commit 62b7b8e).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0106-papers3-epd-qualification/main/app_main.cpp — Qualification implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/2026-07-14-cell-C/03-operator-observations.md — Measured visual evidence

## 2026-07-14

Phase 0: Cell D reproduced Cell C's almost-white TEXT black under IDF 5.4.2, ruling down IDF as the cause; added a fundamentals-first waveform, VCOM, rail, optical, and timing investigation plan (commit `60c3c94c3bf2e724023cedb13a3ccf25c14c117a`).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/01-papers3-epd-waveform-and-physical-drive-investigation-plan.md — Causal assessment and discriminating experiment plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/2026-07-14-cell-D/03-operator-observations.md — Matching Cell D visual failure and automatic boundary result

## 2026-07-14

Phase 0: flashed official FactoryTest V0.5, recorded similar broad-black weakness with crisp dashboard text, downloaded full matching bug reports, and proved M5GFX 0.2.15/0.2.25 built-in EPD LUTs are identical; next control is an independent PaperS3 driver.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/02-similar-papers3-epd-bug-reports-and-independent-driver-controls.md — Factory interpretation, report synthesis, and next independent-driver experiment
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/factory-v0.5/02-operator-observations.md — Measured factory visual disposition

## 2026-07-14

Published a self-contained 64 KB technical deep dive covering the reader program, e-paper physics, PaperS3 hardware, M5GFX waveforms, measured qualification, factory control, related reports, causal hypotheses, and independent-driver experiment design (report commit `fdb97055f0638e5e16dc29d39d0369956c900ef0`; vault commit `218cd195a0e593fa3f1c465a5f48896468db422b`, pushed to `origin/main`).

### Related Files

- /home/manuel/code/wesen/go-go-golems/go-go-parc/Research/2026/07/14/ARTICLE - PaperS3 E-Paper Qualification - Physics, Waveforms, and Physical Drive.md — Full Obsidian article
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/design-doc/02-papers3-e-paper-qualification-deep-dive.md — Primary textbook-style ticket report
