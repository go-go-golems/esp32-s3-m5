---
title: Implementation Diary
ticket: ALMANACH-RENDER-SERVICE
doc_type: reference
status: active
intent: long-term
topics:
  - go
  - chrome-headless
  - api-server
  - almanach
  - stoms3r
  - rendering
created: 2026-04-29
---

# Implementation Diary — ALMANACH-RENDER-SERVICE

---

## Step 1: Ticket Creation and Design Document

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to build a _go_ server under stoms3r which bundles chrome headless in order to expose the almanach generation as an API. Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet points and pseudocode and diagrams and api references and file references. It should be very clear and detailed. Store in the ticket and then upload to remarkable. Keep a diary."

**Inferred user intent:** Create a comprehensive onboarding document for building a Go server that uses Chrome headless to render Almanach Studio pages server-side, convert them to bitmaps, and forward to the ESP32 printer. Written for someone new to the entire system.

### What I did

1. Created docmgr ticket `ALMANACH-RENDER-SERVICE` with topics: go, chrome-headless, api-server, almanach, stoms3r, rendering
2. Added design doc: `design-doc/01-almanach-render-service-analysis-design-and-implementation-guide.md` (53 KB)
3. Added this diary
4. Related 6 key source files to the ticket
5. Wrote a 15-section document covering:
   - Problem statement (manual → automated)
   - System context (every existing component explained)
   - The React SPA data model and JSON layout format
   - The ESP32 firmware API (POST /api/print/bitmap, bitmap format)
   - Proposed Go server architecture with data flow diagram
   - API design (5 endpoints)
   - Chrome Headless integration with chromedp (startup flags, render sequence, lifecycle)
   - Bitmap conversion pipeline (PNG → 1-bit MSB, Floyd-Steinberg dithering)
   - Go project structure (file map with purpose for each file)
   - 6-phase implementation plan with tasks
   - Configuration and deployment (env vars, Raspberry Pi, Docker)
   - Error handling (Chrome crashes, ESP32 unreachable, timeouts, fetcher failures)
   - Testing strategy (unit, integration, E2E)
   - Security considerations
   - File reference map (existing C/JSX files + new Go files)
   - Appendices: ESP32 API quick ref, layout JSON example, wttr.in weather, Wikipedia API

### What worked

- Writing for a "new intern" forced me to explain things from first principles instead of assuming context. The bitmap format section, for example, explains every byte and bit — which is exactly what someone implementing `bitmap.go` needs.
- The data flow pseudocode (step 1-7) makes the entire render pipeline concrete.
- The file reference map (§15) gives a clear reading list — study these files in this order.

### What was tricky

- Balancing "explain everything" with "don't write a 100-page book." The document is 53 KB which is long but each section serves a specific purpose for someone who will implement this.
- Deciding between SPA-side bitmap export vs. Go-side PNG→bitmap conversion. The document presents both approaches with tradeoffs.

### What should be done next

- Upload to reMarkable for offline reading
- Add tasks to the ticket for the 6 implementation phases
- Actually implement Phase 1 (skeleton Go server)

---

## Step 2: Phase 1-4 Implementation

### Prompt Context

**User prompt (verbatim):** "Create tasks for the ticket, with different phases, and then start working on them, task by task."

**Inferred user intent:** Break the design into granular tasks, then implement each one.

### What I did

Created 18 granular tasks across 6 phases, then implemented Phases 1-4:

**Phase 1 — Skeleton Go server (4 tasks):**
- `main.go` — entry point with graceful shutdown on SIGINT/SIGTERM
- `config.go` — all env vars (ALMANACH_PORT, ALMANACH_PRINTER_IP, ALMANACH_WEB_DIR, ALMANACH_CHROME_PATH, etc.)
- `static.go` — serves almanach SPA HTML + JS bundle from disk
- `server.go` — route registration, Server struct, /health endpoint

**Phase 2 — Chrome headless (3 tasks):**
- Modified `almanach-studio.jsx` to expose `window.almanachReady`, `window.almanachLoadLayout(json)`, `window.almanachExportBitmap()`
- `renderer.go` — chromedp allocator, `renderWithChrome()` with navigate → wait ready → inject layout → sleep → export bitmap
- `server.go` handlers for `/api/render` and `/api/render-and-print`

**Phase 3 — Bitmap + printer (3 tasks):**
- `bitmap.go` — PNG decode → grayscale → threshold binarize → MSB-first pack
- `printer.go` — HTTP client that POSTs raw bitmap to ESP32 with X-Width/X-Height/X-Feed headers
- Wired render+print handler in server.go

**Phase 4 — Data fetchers (5 tasks):**
- `layout.go` — Go structs for all 15 block types, `buildDefaultLayout()` assembler
- `fetch_date.go` — local date/day name computation
- `fetch_weather.go` — wttr.in JSON API (no API key)
- `fetch_news.go` — placeholder (hardcoded headlines for now)
- `fetch_quote.go`, `fetch_word.go`, `fetch_history.go` — local pools + Wikipedia "On this day" API

### What worked

- The Go server compiles to a single 12.7 MB binary and serves the SPA correctly
- Health endpoint returns JSON with version and printer status
- SPA static files serve at `/almanach` and `/almanach/bundle.js`
- `go mod tidy` resolved chromedp v0.15.1 (required Go 1.26 — auto-upgraded)

### What was tricky

- `fetchers/` subdirectory caused package mismatch — the fetcher files reference types from `layout.go` in the main package. Moved them flat alongside other `.go` files.
- Web dir relative path failed from `cmd/almanach-render-service/` — must use absolute path via env var
- `dist/index.html` vs `almanach.html` naming mismatch — Go server reads from dist/ which has `index.html`

### What remains

- Phase 5: Scheduler (cron + schedule CRUD endpoints) — task 24
- Phase 6: Makefile, Dockerfile, README — task 25
- End-to-end test with real Chrome + real ESP32 printer
- Implement real RSS news fetcher

### Technical details

- **Commit**: 447e867
- **Files**: 20 files changed, 1229 insertions
- **Binary size**: 12.7 MB
- **Dependencies**: chromedp v0.15.1, robfig/cron/v3 (declared but not yet used)
- **Go version**: 1.26.2 (upgraded for chromedp compatibility)
