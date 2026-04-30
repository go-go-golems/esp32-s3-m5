# Tasks

## TODO

- [x] Add tasks here

- [x] Phase 1: Skeleton Go server — go.mod, main.go, config.go, static file serving, health endpoint
- [x] Phase 2: Chrome headless integration — chromedp, renderer.go, modify JSX to expose window.almanach* API
- [x] Phase 3: Bitmap conversion + print forwarding — bitmap.go, printer.go, POST /api/render-and-print
- [x] Phase 4: Data fetchers — weather, news, quote, word, history, date, layout.go builder
- [x] Phase 5: Scheduler — cron, POST/GET/DELETE /api/schedule
- [x] Phase 6: Production — Dockerfile, systemd unit, README
- [x] Phase 1a: Create go.mod, main.go skeleton with HTTP server on configurable port
- [x] Phase 1b: config.go — env var loading (port, printer IP, web dir, chrome path)
- [x] Phase 1c: static.go — serve almanach.html and almanach-bundle.js from disk
- [x] Phase 1d: /health endpoint, graceful shutdown
- [x] Phase 2a: Modify JSX to expose window.almanachReady, almanachLoadLayout(), almanachExportBitmap()
- [x] Phase 2b: renderer.go — chromedp allocator, renderAlmanac() with navigate+inject+export
- [x] Phase 2c: POST /api/render handler — accepts layout JSON, renders via Chrome, returns bitmap
- [x] Phase 3a: bitmap.go — PNG decode, grayscale, threshold binarize, MSB-first pack
- [x] Phase 3b: printer.go — POST /api/print/bitmap client with X-Width/X-Height/X-Feed headers
- [x] Phase 3c: POST /api/render-and-print handler — render + forward to ESP32
- [x] Phase 4a: layout.go — Go structs for block types, JSON layout builder
- [x] Phase 4b: fetchers/date.go — local date, day name, week number
- [x] Phase 4c: fetchers/weather.go — wttr.in API
- [x] Phase 4d: fetchers/news.go — RSS feed parsing
- [x] Phase 4e: fetchers/quote.go, word.go, history.go — remaining data sources
- [x] Phase 5a: scheduler.go — robfig/cron integration, schedule CRUD endpoints
- [x] Phase 6: Makefile, Dockerfile, README
