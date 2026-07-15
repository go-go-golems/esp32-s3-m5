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

## 2026-07-14

Step 18 committed fixed-ring M5GFX timing instrumentation, trace-off/on builds, and 18/18 observer audit as 2badb87b0ae91d2f5dd022551d822328c5de2fba; no hardware modified.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0108-papers3-m5gfx-runtime-trace/main/epd_trace_runtime.cpp — Code commit 2badb87
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/20-m5gfx-runtime-trace-audit-latest.md — Audit evidence

## 2026-07-14

Installed exact ESP-IDF v5.3.3, built previously blocked matrix Cells A/B, created 0109 FactoryTest V0.5 clean/F1/F2 source-lineage controls, passed 19/19 no-hardware audit, and preregistered immutable F0/F1/F2 experiments. No firmware flashed.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0109-papers3-factory-v0.5-runtime-trace/main/factory_trace_runtime.cpp — F2 trace implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260714-001-factory-v05-exact-f0/manifest.json — F0 preregistration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/25-factory-v0.5-trace-audit-latest.md — Final audit

## 2026-07-14

Step 19 committed exact-IDF matrix A/B builds, 0109 FactoryTest V0.5 F1/F2 controls, 19/19 audit, and immutable F0/F1/F2 preregistrations as 4ab273a69231d50ccc51fcc5e839715e89fdfa57; no hardware modified.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0109-papers3-factory-v0.5-runtime-trace/main/main.cpp — Code commit 4ab273a
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260714-001-factory-v05-exact-f0/01-preregistration.md — F0 protocol

## 2026-07-14

Captured the Printalyzer Densitometer protocol at upstream commit f91c91e, including the command dispatcher, desktop parser, vendor manual, license, provenance, and verified SHA-256 manifest; no instrument command was sent.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/printalyzer-protocol-f91c91ecc60bb1f435b8dacfc9929f45315f3912/docs/usb-control-protocol.md — USB protocol source
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/printalyzer-protocol-f91c91ecc60bb1f435b8dacfc9929f45315f3912/firmware/cdc_handler.c — Authoritative command implementation

## 2026-07-14

Executed exact F0 once, ingested and hashed the original 60 fps video, added common-host-clock Printalyzer/PaperS3 capture, verified Printalyzer v1.1.0 read-only inventory, and preserved a pyserial observer failure that reset the board into ROM download mode; no automatic recovery was attempted because F0 boot replays the panel sequence.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/29-capture-synchronized-serial.py — Synchronized capture implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260714-001-factory-v05-exact-f0/events.jsonl — F0 execution evidence
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/29-synchronized-serial-capture-validation.md — Validation and observer failure

## 2026-07-14

Step 20 committed exact F0 flash/video evidence and guarded synchronized Printalyzer/PaperS3 serial capture as ec2bf1bc5efd366a684af1f345e3f0f8f62accf0; the board remains deliberately in ROM download mode after the documented pyserial observer reset.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Step 20
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/29-capture-synchronized-serial.py — Code commit ec2bf1b

## 2026-07-14

Passed passive physical Printalyzer reference capture: valid CAL-LO was 0.05 D three times and valid CAL-HI was 1.49 D three times at BASIC 0.01 D resolution; six operator-invalid setup readings remain preserved and excluded; no serial input or panel operation occurred.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/29-printalyzer-passive-calibration-20260714T235306Z.jsonl — Raw immutable event stream
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/29-printalyzer-passive-reference-result.md — Timestamped reference-measurement disposition

## 2026-07-14

Step 21 committed the passive Printalyzer reference result as 3dc771a935ecd81936444f73e82d71c31447e235: 0.05 D ×3 CAL-LO and 1.49 D ×3 CAL-HI, with six operator-invalid setup samples preserved and excluded.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Step 21
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/29-printalyzer-passive-reference-result.md — Evidence commit 3dc771a

## 2026-07-14

Captured a focused official-source snapshot matching the connected Printalyzer v1.1.0 build g7101373 (commit 7101373), including first-party firmware/desktop sources and relevant docs, board, and enclosure resources; explicitly excluded Git history, vendored dependencies, deployment binaries, and toolchains because no Printalyzer build or flash is planned.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/printalyzer-installed-v1.1.0-710137374fa4131693c6dea65670c7759479e6a5-analysis/ANALYSIS-MAP.md — Scope and file rationale
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/printalyzer-installed-v1.1.0-710137374fa4131693c6dea65670c7759479e6a5-analysis/software/firmware/src/densitometer.c — Installed-version density conversion
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/printalyzer-installed-v1.1.0-710137374fa4131693c6dea65670c7759479e6a5-analysis/software/firmware/src/sensor.c — Installed-version sensor timing and raw conversion

## 2026-07-14

Step 22 committed the focused installed-Printalyzer v1.1.0 source subset as 239b4470667b0553305dae8a0be6bf8b54b71b3d; it is for analysis only, with no Printalyzer build or flash planned.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Step 22
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/printalyzer-installed-v1.1.0-710137374fa4131693c6dea65670c7759479e6a5-analysis/ANALYSIS-MAP.md — Source scope and commit mapping

## 2026-07-14

Reduced both Printalyzer source snapshots to code/text only: removed PDFs, images, hardware/enclosure assets, and other non-code resources while preserving installed-version firmware/desktop source, protocol text, provenance, and verified manifests.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/printalyzer-installed-v1.1.0-710137374fa4131693c6dea65670c7759479e6a5-analysis/ANALYSIS-MAP.md — Revised code-only source scope

## 2026-07-14

Extended synchronized Printalyzer raw capture to snapshot read-only calibration, reproduce the installed v1.1.0 channel-normalization/slope/target-density formula per sample, reject saturation or light-duty mismatch, and test the exact gain-2/100 ms/duty-128 command and cleanup sequence against a fake device.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/29-capture-synchronized-serial.py — Density-aware raw capture
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/30-test-synchronized-serial.py — No-hardware command/cleanup integration test

## 2026-07-14

Preregistered and passed a five-second static Printalyzer raw capture over a middle-ish PaperS3 white region in ROM mode: 46 post-settling samples, density estimate 0.678142 ± 0.000625 D, zero saturation/invalid records, 0.002243 D range, and complete cleanup; absolute density remains unqualified pending repositioning and raw reference checks.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260714-004-printalyzer-static-white-raw/04-analysis.md — Static raw statistics and limits
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260714-004-printalyzer-static-white-raw/raw-static-white.jsonl — Immutable synchronized raw evidence

## 2026-07-14

Step 23 recorded code-only Printalyzer sources (194e58a), density-aware raw capture (de423ea), and the passing static PaperS3 point-signal experiment (6d7d19e); absolute density and dynamic eligibility remain gated.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Step 23
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260714-004-printalyzer-static-white-raw/04-analysis.md — Step 23 experiment result

## 2026-07-14

Preserved placement and perturbation experiments 005-007: manual reseating changed mean density by 0.078 D, an immediate repeat contained a recoverable 0.086 D excursion, and a hand-wave run was causally confounded by app-background changes. Preregistered experiment 008 and a guarded orchestrator for an exact-F0 fixed-point dynamic density trace.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/31-run-synchronized-f0-density.sh — Guarded raw-stream plus exact-F0 orchestration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density/01-preregistration.md — Dynamic F0 density protocol

## 2026-07-14

Executed and analyzed exact F0 fixed-point density experiment: 442 valid unsaturated samples over 44.77 s, full flash and cleanup evidence, reproducible CSV/JSON/SVG calculations, and a self-contained retro monochrome HTML dashboard. F1 is only conditionally eligible under a new fixed-head/fixed-camera ledger.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density/05-run-report.md — F0 result and interpretation limits
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density/dashboard.html — Interactive evidence dashboard

## 2026-07-14

Prepared and check-validated the separately preregistered F1 stock-source trace-off density/video comparison. It retains the current fixed Printalyzer position, requires fixed camera recording, and has not flashed hardware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/35-run-synchronized-f1-density.sh — F1 guarded runner
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-009-factory-f1-dynamic-density/01-preregistration.md — F1 physical protocol

## 2026-07-14

Step 24 recorded reproducible F0 density/dashboard evidence (115475f), corrected generated-document hygiene (9b2d9f8/1cd4634), and prepared but did not execute F1 density/video comparison (be55cbb).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Step 24
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density/05-run-report.md — Step 24 F0 result

## 2026-07-14

Pivoted from a new F1 camera run to a separately preregistered F1 density-only control at the unchanged aperture. F1 remains mandatory before F2 because it proves the stock-source trace-off baseline; the new runner/ledger passed check-only validation and did not flash hardware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/36-run-synchronized-f1-density-only.sh — Guarded F1 density-only runner
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-010-factory-f1-density-only-control/01-preregistration.md — F1 density-only rationale and gate

## 2026-07-14

Step 25 recorded the density-only F1 decision and prepared-only control as 396a51d761f62a2c48411c1c3323c3764475e4f9; F1 remains required before F2 but no new camera recording is required.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md — Step 25
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-010-factory-f1-density-only-control/01-preregistration.md — Step 25 protocol

## 2026-07-14

Executed F1 density-only trace-off control: 442 valid unsaturated samples and complete cleanup. Deterministic F1/F0 fixed-point comparison found Pearson 0.943874 over normalized 0–25 s activity with a 0.5 s alignment; F2 remains pending operator/no-anomaly confirmation.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-010-factory-f1-density-only-control/05-f0-comparison.md — Reproducible F1/F0 comparison
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-010-factory-f1-density-only-control/raw-dynamic-f1.jsonl — Immutable F1 raw stream

## 2026-07-14

Prepared and no-hardware-tested the F2 ring-plus-density hardware path after F1 passed: safe combined serial/density capture, 60-second run gate, post-idle ring extraction, contiguous-event validation, and explicitly approximate DISPLAY_IDLE-to-dump-begin host alignment. No F2 flash yet.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/39-run-synchronized-f2-ring-density.sh — F2 guarded runner
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-011-factory-f2-ring-density/01-preregistration.md — F2 physical protocol

## 2026-07-14

Recorded the F2 automatic capture ownership conflict before flash, then replaced it with a separately preregistered manual-reset protocol: no-reset stage, safe read-only capture arm, one operator reset, and post-capture ring extraction. All scripts pass no-hardware checks; F2 remains unflashed.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-011-factory-f2-ring-density/04-failed-attempt.md — Failure evidence
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-012-factory-f2-manual-reset-density/01-preregistration.md — Manual-reset F2 protocol

## 2026-07-14

Preregistered and no-hardware-validated a final F2 sequence that serializes USB ownership: esptool flashes with no reset, esptool executes its ESP32-S3 hard reset, then the existing non-controlling reader captures the delayed ring and 60-second density trace. F2 has not yet executed.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/44-run-f2-auto-reset-density.sh — Guarded execution
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-013-factory-f2-esptool-reset-density/01-preregistration.md — Sequential ownership rationale

## 2026-07-14

Built and executed P0.19 native EPD_Painter density step response. All direct-driver operations and capture cleanup passed, but a successful full-black command did not produce expected fixed-aperture darkening; operator optical disposition remains pending.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0110-papers3-epd-density-step-response/main/app_main.cpp — Automatic semantic treatment
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/experiments/EXP-20260715-016-native-epd-density-step-response/04-analysis.md — Measured result
