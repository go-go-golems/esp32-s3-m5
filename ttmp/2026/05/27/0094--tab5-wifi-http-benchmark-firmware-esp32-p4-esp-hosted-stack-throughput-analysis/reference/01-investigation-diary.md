---
title: Investigation Diary
doc-type: reference
status: active
---

# Diary

## Goal

Record the implementation journey for ticket 0094: Tab5 WiFi/HTTP benchmark firmware.

## Step 1: Ticket creation and design doc

Created ticket 0094 for a dedicated benchmark firmware to measure throughput at every layer of the WiFi-to-HTTP data path on the M5Stack Tab5. Wrote a comprehensive intern-guide-style design document.

### Prompt Context

**User prompt (verbatim):** "Close the ticket, then open a new one, and let's make a new firmware to benchmark the HTTP and wifi speed, for different factors, so that we know where the bottlenecks are in our stack. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet points and pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and then upload to remarkable. Keep a diary as you work."

**Assistant interpretation:** Close ticket 0093, create ticket 0094 for a benchmark firmware, write an intern-grade technical guide with full system architecture, pseudocode, API references, and Mermaid diagrams.

**Inferred user intent:** Get a comprehensive onboarding + design document that an intern can use to understand the full ESP-Hosted WiFi stack and implement a benchmark firmware, then upload it to reMarkable for reading.

### What I did

- Closed ticket 0093 (all tasks complete)
- Created ticket 0094 with title and topics
- Wrote 24 KB design doc (7 sections covering hardware, data path, SDIO bus, ESP-Hosted, httpd, WiFi modes, measurement plan, firmware architecture, implementation plan, API references, expected results)
- Uploaded to reMarkable at /ai/2026/05/27/0094
- Updated changelog and related files

### What worked

- Mermaid diagram for the data path clearly shows all the hops
- Pseudocode for the benchmark handlers is concrete enough to implement directly

### What didn't work

- N/A - documentation step, no code was built

### What was tricky to build

- Getting the correct httpd default config values from the IDF source to ensure the design doc is accurate

### What warrants a second pair of eyes

- The SDIO bus configuration should match the actual sdkconfig.defaults

### What should be done in the future

- Implement the firmware, run benchmarks, analyze results

### Code review instructions

- Read design doc: `design-doc/01-benchmark-firmware-design-and-intern-guide.md`
- Verify SDIO config matches `sdkconfig.defaults`

## Step 2: Firmware implementation and STA benchmark run

Built the benchmark firmware from the 0093 fork, stripped display code, implemented all benchmark endpoints, wrote the Python benchmark runner with SQLite storage, and ran the full STA benchmark suite.

### Prompt Context

**User prompt (verbatim):** "Go ahead. Store results in sqlite and save all your scripts and queries and such in the scripts/ folder of the firmware."

**Assistant interpretation:** Implement the benchmark firmware, store results in SQLite, and put all scripts in the firmware's scripts/ directory.

**Inferred user intent:** Get working firmware with real benchmark data in SQLite, ready for analysis.

### What I did

- Forked 0093-tab5-ui-screen-viewer to 0094-tab5-wifi-bench
- Stripped display_app.c/h, LVGL image code from app_main.c
- Wrote bench_server.c/h with 7 endpoints (GET /, GET /app.js, GET /api/health, GET /api/system, POST /api/bench/upload, GET /api/bench/download, POST /api/bench/ping)
- Upload endpoint captures T0-T6 timestamps and per-recv-segment data (bytes + timestamp) with MAX_SEGMENTS=2048 cap
- Download endpoint generates incrementing pattern payload in SPIRAM
- Ping endpoint echoes body back with timing
- Updated AP SSID to "Tab5-Bench"
- Renamed log tags from tab5_text_echo_* to tab5_bench_*
- Removed m5stack_tab5, esp_lcd from CMakeLists (headless firmware)
- Added esp_timer to PRIV_REQUIRES
- Wrote browser benchmark UI (index.html + app.js) with auto suite, segment timeline chart, JSON export
- Wrote Python benchmark runner (01-run-benchmarks.py) with SQLite storage
  - Tables: runs, upload_results, download_results, ping_results, segments
  - Supports --quick, --upload-only, --download-only, --ping-only, --repeats
  - Auto-compresses with zlib.compress (Python) for deflate tests
- Wrote Python analysis queries (02-analyze-results.py) with 8 pre-built queries
- Fixed format specifier warnings (%zu -> %lu with casts for ESP32 RISC-V)
- Fixed deflate decompression: use ?size= query param as decompressed size hint instead of compressed*2 heuristic
- Built, flashed, verified WiFi + HTTP + all endpoints working
- Ran full STA benchmark suite (3 repeats, 6 sizes, 2 compression modes)

### Why

The firmware needs to be headless (no display) to avoid LVGL overhead skewing results. SQLite on the host side (not ESP32) makes sense because the ESP32 has no filesystem, and piping JSON results into a local SQLite DB gives us query power without firmware complexity.

### What worked

- Build succeeded after fixing format specifiers
- All 7 endpoints respond correctly
- Full STA benchmark suite completed in ~5 minutes with 3 repeats
- SQLite storage works well for analysis queries
- Deflate compression shows dramatic speedup: 1.8 MB raw 4.1s -> deflate 0.2s

### What didn't work

- First deflate attempt failed because `dec_buf_size = compressed * 2` was far too small (7 KB compressed -> 1.8 MB decompressed). Fixed by using `?size=` query param.
- Serial port busy on first flash attempt (old monitor still running). Had to kill with `fuser -k`.
- `esp_get_free_heap_size()` returns `uint32_t` on ESP32, not `size_t`. GCC -Werror caught this.

### What was tricky to build

- The deflate decompression buffer sizing: the design doc assumed we'd know the decompressed size, but in practice the HTTP handler only sees the Content-Length (compressed size). The `?size=` query parameter bridges this gap, but it requires the client to tell the server the expected output size. This is a protocol design choice that should be documented.
- Per-segment timing in SPIRAM: 2048 segments x 16 bytes = 32 KB, which is fine in PSRAM but would blow internal RAM.

### What warrants a second pair of eyes

- The segment timing accuracy: `esp_timer_get_time()` is called immediately after `httpd_req_recv()` returns. This measures the time from the previous recv to this recv, which includes both network wait and TCP/IP processing on the P4.
- The download endpoint sends the response in a single `httpd_resp_send()` call. For 1.8 MB, this might block differently than chunked sending.

### What should be done in the future

- Run SoftAP benchmarks for comparison
- Run the raw-vs-deflate and softap-vs-sta analysis queries
- Add gap histogram analysis
- Update design doc with actual benchmark results

### Code review instructions

- Key files: `bench_server.c` (upload handler with segment timing), `01-run-benchmarks.py` (benchmark runner), `02-analyze-results.py` (analysis queries)
- Verify: curl upload returns timing JSON, SQLite DB has correct data, analysis queries produce correct tables
- Check: segment cap (MAX_SEGMENTS=2048), SPIRAM allocation, stack size (48 KB for tinfl)

### Technical details

**Key benchmark results (STA, 3-repeat averages):**

| Size | Raw recv (ms) | Deflate total (ms) | Raw kbps | Deflate kbps |
|------|-------|-------|-------|-------|
| 1KB | 0 | 0.2 | 86537 | 27024 |
| 10KB | 11 | 0.4 | 8274 | 34740 |
| 100KB | 170 | 3.2 | 5114 | 61277 |
| 500KB | 953 | 18.8 | 4319 | 64408 |
| 1MB | 1823 | 38.3 | 4681 | 78161 |
| 1.8MB | 3534 | 67.1 | 4193 | 6772 |

| Direction | 1.8MB throughput (kbps) |
|---|---|
| Upload raw | 4193 |
| Upload deflate | 6772 (recv) + 67ms decompress |
| Download raw | 1744 |
| Ping RTT (1KB) | 108ms |

Segment analysis for 1.8MB raw upload:
- ~1230 segments, avg 2.8ms delta, avg 1495 bytes/seg
- 5-7 stalls > 50ms per run
- 45-58 gaps > 10ms per run
- Max gap: 109-363ms (likely WiFi retransmissions or SDIO backpressure)
