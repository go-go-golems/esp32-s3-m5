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

## 2026-07-14

P0.13: audited pinned EPD_Painter before hardware use; pin mapping matches M5GFX, but five correctness/observability blockers prohibit flashing the upstream source unchanged. Captured all source, replay commands, audit logic, and outputs under ticket scripts/sources.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md — Pre-hardware gate, findings, and constrained local patch scope
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/10-epd-painter-pre-hardware-audit.md — Generated audit evidence

## 2026-07-14

Step 12: completed P0.13 independent-driver pre-hardware audit and blocked unmodified EPD_Painter; all replay/audit artifacts are ticket-owned (commit 4c1c89c76e22768d142310b75db631132379a711).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md — Audit findings and patch gate
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Chronological Step 12 record

## 2026-07-14

P0.14: designed the independent EPD_Painter control as a pure ESP-IDF 5.4.2, no-drive-on-boot firmware with an explicit safety state machine, bounded idle proof, deterministic fixtures, fixed optical protocol, stop gates, and result-to-hypothesis table. Expanded the audit to eight upstream blockers.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md — Complete pre-hardware experiment protocol
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/10-epd-painter-pre-hardware-audit.md — Expanded generated source gate

## 2026-07-14

Step 13: completed P0.14 pure-IDF independent-driver experiment protocol, bounded state machine, optical gates, and causal decision table (commit e7e4848d9544b902dcf79246fa520f039c2d74ee).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md — Committed experiment design
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Chronological Step 13 record

## 2026-07-14

P0.15: created and clean-built the no-drive pure-IDF 5.4.2 independent control in 0107. The reproducible patch resolves initialization/build/resource blockers without changing waveform bytes; static/binary audit passes 12 checks. No hardware was flashed.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0107-papers3-epd-painter-control/main/app_main.cpp — Status-only BOOT_LOCKED control entrypoint
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/12-epd-painter-build-latest.md — Exact binary and build evidence
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/13-built-control-audit-latest.md — Passing static and ELF gate
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/patches/11-epd-painter-pure-idf-hardening.patch — Audited local driver hardening

## 2026-07-14

Step 14: completed P0.15 hardened status-only independent control, exact IDF 5.4.2 warning-free build, and 12-check static/binary gate without flashing hardware (commit f7c3e7347ebe75c9d654a9c9d92a5ae7f439dfd7).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0107-papers3-epd-painter-control — Committed independent control firmware
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Chronological Step 14 build record

## 2026-07-14

P0.16: added no-drive-at-boot, state-gated HIGH/two-stage cleanup/full/area/checker/page commands, bounded worker/FAULT behavior, operation evidence records, and a reproducible binary reader fixture. Exact-IDF build is warning-free and the expanded binary audit passes 14 checks; no flash occurred.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0107-papers3-epd-painter-control/main/app_main.cpp — Bounded command state machine and operation records
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0107-papers3-epd-painter-control/main/fixtures/reader_page.bin — Deterministic packed reader workload
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/14-generate-epd-control-fixtures.py — Reproducible reader fixture generator
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/13-built-control-audit-latest.md — Passing 14-check pre-flash binary gate

## 2026-07-14

Step 15: completed P0.16 bounded state-gated EPD commands, deterministic fixtures, warning-free build, and 14-check pre-flash audit without modifying hardware (commit e9f3769dc417adb1623ac0a1435b891c5f936d0f).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0107-papers3-epd-painter-control/main/app_main.cpp — Committed bounded command state machine
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Chronological Step 15 record

## 2026-07-14

P0.17 preliminary: first independent-control flash booted safely in BOOT_LOCKED with zero pending stages and idle rails. Detected that idf.py flash relinked the app descriptor after preflight; no waveform was run. Fixed deterministic PROJECT_VER and direct-esptool exact-artifact flashing, then rebuilt/audited candidate SHA f24705a6...

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0107-papers3-epd-painter-control/CMakeLists.txt — Deterministic project version
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/15-flash-epd-control.sh — Exact-artifact flash with pre/post SHA guard
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/16-epd-control-monitor-20260714T210036Z.log — Passing no-drive runtime boot evidence

## 2026-07-14

P0.17 HARD-white gate: audited independent driver completed in 397 ms with zero pending stages and idle rails, but operator observed lots of ghosting from the prior screen. Classified automatic PASS / optical FAIL; stopped before black or matrix operations.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/16-epd-control-monitor-20260714T210836Z.log — Runtime transcript
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/17-p0.17-hard-white-observation.md — Optical stop-gate evidence

## 2026-07-14

Designed minimally perturbing runtime M5GFX trace instrumentation and immutable experiment ledgers; statically decoded 0.2.15/0.2.25 LUTs to identical canonical SHA d24b2df1... without hardware access.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/04-m5gfx-runtime-waveform-instrumentation-and-scientific-experiment-ledger.md — Runtime trace and scientific ledger design
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/17-decode-m5gfx-epd-waveforms.py — Static waveform decoder

## 2026-07-14

Implemented fixed-ring M5GFX runtime timing hooks and warning-free trace-off/trace-on builds without flashing. Observer audit passed 18/18; trace-off Panel_EPD task and Bus_EPD power-control text are byte-identical to clean Cell D, trace-on app delta is 1,584 bytes with 24 KiB ring BSS.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0108-papers3-m5gfx-runtime-trace/main/epd_trace_runtime.cpp — Fixed nonblocking trace ring
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/20-m5gfx-runtime-trace-audit-latest.md — Observer-effect audit
