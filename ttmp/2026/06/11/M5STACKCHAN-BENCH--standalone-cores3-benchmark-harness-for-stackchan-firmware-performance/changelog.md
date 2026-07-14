# Changelog

## 2026-06-11

- Initial workspace created


## 2026-06-11

Created intern-ready standalone benchmark harness design guide with source-backed current-state analysis, architecture, metrics, pseudocode, decision records, phased implementation plan, and test strategy

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/11/M5STACKCHAN-BENCH--standalone-cores3-benchmark-harness-for-stackchan-firmware-performance/design/01-standalone-benchmark-harness-analysis-design-and-implementation-guide.md — Primary benchmark design deliverable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/11/M5STACKCHAN-BENCH--standalone-cores3-benchmark-harness-for-stackchan-firmware-performance/reference/01-investigation-diary.md — Chronological investigation diary


## 2026-06-11

Uploaded benchmark guide bundle to reMarkable at /ai/2026/06/11/M5STACKCHAN-BENCH and verified cloud listing


## 2026-06-11

Re-uploaded corrected benchmark guide bundle after fixing Mermaid labels; verified reMarkable listing


## 2026-06-11

Paused benchmark execution and wrote extended diary covering Kconfig/CMake integration, build/flash attempts, WDT crash, LVGL label heap assertion, static-buffer mitigation, and current paused state

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/11/M5STACKCHAN-BENCH--standalone-cores3-benchmark-harness-for-stackchan-firmware-performance/reference/01-investigation-diary.md — Extended implementation diary for benchmark attempts


## 2026-06-11

Stabilized standalone benchmark by moving metrics off the main task stack; captured first successful four-mode hardware summaries

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/benchmark_main.cpp — Global metrics storage and stack-safe percentile calculation
- /tmp/stackchan-bench-monitor4.log — Successful serial benchmark summaries


## 2026-06-11

Re-uploaded updated benchmark guide and diary bundle to reMarkable after first successful hardware run

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/11/M5STACKCHAN-BENCH--standalone-cores3-benchmark-harness-for-stackchan-firmware-performance/design/01-standalone-benchmark-harness-analysis-design-and-implementation-guide.md — Updated with Appendix A first hardware benchmark results
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/11/M5STACKCHAN-BENCH--standalone-cores3-benchmark-harness-for-stackchan-firmware-performance/reference/01-investigation-diary.md — Updated with stack-safe metrics fix and serial output evidence


## 2026-06-11

Added reproducible chart renderer for Obsidian draw-performance article illustrations

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/11/M5STACKCHAN-BENCH--standalone-cores3-benchmark-harness-for-stackchan-firmware-performance/scripts/01-render-benchmark-charts.py — Parses BENCH_SUMMARY lines and renders PNG charts

