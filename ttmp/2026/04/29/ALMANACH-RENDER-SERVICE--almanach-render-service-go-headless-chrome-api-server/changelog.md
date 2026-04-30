# Changelog

## 2026-04-29

- Initial workspace created


## 2026-04-29

Step 1: Created ticket, wrote 53 KB analysis/design/implementation guide (15 sections, pseudocode, diagrams, API refs, file refs). Added 6 tasks for implementation phases. Related 6 source files.


## 2026-04-29

Step 2: Implemented Phases 1-4 (18 tasks). Go server builds to 12.7 MB binary. Serves SPA, health endpoint works. renderer.go uses chromedp. bitmap.go does MSB packing. printer.go sends to ESP32. layout.go + 6 fetchers. Commit 447e867.


## 2026-04-29

Step 3: Docker self-contained image. Multi-stage Dockerfile using chromedp/headless-shell as base. docker-compose.yml for two-container mode. Remote Chrome support via CHROME_WS_URL. Image builds at 456 MB. Commit 6d1506d.


## 2026-04-29

Step 4: End-to-end test passed. Fixed SVG foreignObject canvas taint by switching to chromedp.Screenshot. Fixed Chrome flag type (string not float). Full pipeline: Chrome loads SPA → fetches live data → renders 384×1019 PNG → converts to 1-bit bitmap → POSTs to ESP32 → printer returns ok. ~2.4s total. Commit cb7d630.

