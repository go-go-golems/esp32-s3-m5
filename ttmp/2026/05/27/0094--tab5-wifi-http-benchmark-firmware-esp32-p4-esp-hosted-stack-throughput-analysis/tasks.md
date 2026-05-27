# Tasks

## Phase 1: Fork and scaffold

- [x] Copy 0093-tab5-ui-screen-viewer → 0094-tab5-wifi-bench
- [x] Update CMakeLists.txt project name
- [x] Remove display_app.c/h and LVGL image code from app_main.c
- [x] Add bench_server.c/h with benchmark handler stubs
- [x] Build, flash, verify HTTP server starts

## Phase 2: Upload benchmark

- [x] Implement POST /api/bench/upload with T0-T6 timestamps
- [x] Capture per-recv-segment data (bytes, timestamp)
- [x] Capture system counters (heap, PSRAM, RSSI)
- [x] Return timing JSON
- [x] Test with curl for various sizes (1KB, 10KB, 100KB, 500KB, 1MB, 1.8MB)
- [x] Test with raw and deflate payloads
- [x] Fix deflate decompression buffer sizing (use ?size= query param)

## Phase 3: Download and ping benchmarks

- [x] Implement GET /api/bench/download (generate payload, time the send)
- [x] Implement POST /api/bench/ping (echo, round-trip time)
- [x] Test with curl

## Phase 4: Browser automation

- [x] Write index.html with benchmark controls
- [x] Write app.js with automated benchmark matrix
- [x] Display results as table, offer JSON download
- [x] Support both STA and SoftAP base URLs

## Phase 5: Analysis and documentation

- [x] Write Python benchmark runner with SQLite storage (01-run-benchmarks.py)
- [x] Write Python analysis queries (02-analyze-results.py)
- [x] Run benchmarks over STA (full suite, 3 repeats)
- [x] Run benchmarks over SoftAP
- [x] Run raw-vs-deflate and softap-vs-sta analysis
- [x] Analyze TCP segment timing to identify stalls
- [x] Write findings document
- [x] Upload to reMarkable
