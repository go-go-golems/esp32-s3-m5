# Changelog

## 2026-05-27

- Initial workspace created


## 2026-05-27

Ticket created. Wrote design doc and intern guide (01-benchmark-firmware-design-and-intern-guide.md): 7 sections covering hardware, data path, SDIO, ESP-Hosted, httpd, measurement plan, benchmark matrix, firmware architecture (pseudocode), API endpoints, implementation phases, API references, expected results. Created tasks for 5 phases.


## 2026-05-27

Step 2: Built benchmark firmware (0094-tab5-wifi-bench). Headless, 7 endpoints, per-segment timing, SQLite storage. Ran full STA benchmark suite (3 repeats x 6 sizes x 2 compressions). Key: 1.8MB raw 4.1s, deflate 0.2s, download 1.7Mbps, ping 108ms.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0094-tab5-wifi-bench/main/bench_server.c — Benchmark HTTP server with per-segment timing
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0094-tab5-wifi-bench/scripts/01-run-benchmarks.py — Python benchmark runner with SQLite storage
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0094-tab5-wifi-bench/scripts/02-analyze-results.py — 8 pre-built analysis queries


## 2026-05-27

Step 2 complete: Added measured STA results (section 8) to design doc. Upload/download/ping/segment analysis tables. Uploaded to reMarkable.


## 2026-05-27

All tasks complete. SoftAP skipped (rarely used in practice). STA benchmarks fully captured and analyzed. Results in design doc section 8 + SQLite.


## 2026-05-27

Ticket closed


## 2026-05-27

Step 3: Published project report to Obsidian vault. 25KB article with measured results, gap histogram, TCP slow-start analysis, download asymmetry diagnosis, and 5 design rules.

