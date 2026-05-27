# Tasks

## Phase 1: Create the firmware project

- [x] Create 0095-cores3se-wifi-bench ESP-IDF project (idf.py create-project)
- [x] Add sdkconfig.defaults for CoreS3 SE
- [x] Copy bench_server.c/h from 0094, adapt for ESP32-S3
- [x] Write wifi_app.c for native WiFi APSTA
- [x] Write wifi_console.c for USB Serial/JTAG console
- [x] Copy browser assets (index.html, app.js)
- [x] Build, flash, verify HTTP server starts

## Phase 2: Upload benchmark

- [x] Implement POST /api/bench/upload with per-segment timing
- [x] Test with curl at various sizes

## Phase 3: Download and ping benchmarks

- [x] Implement GET /api/bench/download and POST /api/bench/ping
- [ ] Test with curl

## Phase 4: Run benchmarks with default config

- [ ] Run full benchmark suite over STA (same matrix as Tab5)
- [ ] Store results in SQLite
- [ ] Compare against Tab5 STA data

## Phase 5: Run benchmarks with optimized config

- [ ] Rebuild with iperf-optimized sdkconfig
- [ ] Run full benchmark suite over STA
- [ ] Compare default vs optimized on same hardware

## Phase 6: Analysis and documentation

- [ ] Run analysis queries against SQLite
- [ ] Write comparison report (CoreS3 SE vs Tab5, default vs optimized)
- [ ] Upload to reMarkable
