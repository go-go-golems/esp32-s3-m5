# Tasks

## Phase 1: Fork and scaffold

- [ ] Copy 0093-tab5-ui-screen-viewer → 0094-tab5-wifi-bench
- [ ] Update CMakeLists.txt project name
- [ ] Remove display_app.c/h and LVGL image code from app_main.c
- [ ] Add bench_server.c/h with benchmark handler stubs
- [ ] Build, flash, verify HTTP server starts

## Phase 2: Upload benchmark

- [ ] Implement POST /api/bench/upload with T0-T6 timestamps
- [ ] Capture per-recv-segment data (bytes, timestamp)
- [ ] Capture system counters (heap, PSRAM, RSSI)
- [ ] Return timing JSON
- [ ] Test with curl for various sizes (1KB, 10KB, 100KB, 500KB, 1MB, 1.8MB)
- [ ] Test with raw and deflate payloads

## Phase 3: Download and ping benchmarks

- [ ] Implement GET /api/bench/download (generate payload, time the send)
- [ ] Implement POST /api/bench/ping (echo, round-trip time)
- [ ] Test with curl

## Phase 4: Browser automation

- [ ] Write index.html with benchmark controls
- [ ] Write app.js with automated benchmark matrix
- [ ] Display results as table, offer JSON download
- [ ] Support both STA and SoftAP base URLs

## Phase 5: Analysis and documentation

- [ ] Run benchmarks over STA and SoftAP
- [ ] Run benchmarks with raw and deflate payloads
- [ ] Run benchmarks at multiple payload sizes
- [ ] Analyze TCP segment timing to identify stalls
- [ ] Write findings document
- [ ] Upload to reMarkable
