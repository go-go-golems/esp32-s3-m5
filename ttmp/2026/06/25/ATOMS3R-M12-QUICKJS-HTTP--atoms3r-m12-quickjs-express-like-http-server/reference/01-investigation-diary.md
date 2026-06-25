---
Title: Investigation Diary
Ticket: ATOMS3R-M12-QUICKJS-HTTP
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - http
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Chronological diary for the Express-like HTTP server on AtomS3R M12 QuickJS.
LastUpdated: 2026-06-25T23:30:00-07:00
WhatFor: Use to resume or review HTTP serving, static assets, and JavaScript route registration work.
WhenToUse: Read before adding `esp_http_server`, static file serving, or QuickJS dynamic route dispatch.
---

# Diary

## Goal

Capture the HTTP server work for `0103-atoms3r-m12-native-quickjs`: simple serving, static assets from storage/flash, and an Express-like JavaScript API inspired by `go-go-goja` but sized for ESP32-S3 firmware.

## Step 1: Create the HTTP ticket and intern guide

This step creates a dedicated HTTP implementation track. The design takes inspiration from `go-go-goja`'s Express module and static asset helpers, but intentionally removes auth, sessions, resource policies, and desktop assumptions.

The embedded design separates static serving from dynamic JavaScript route handling. Static files should stream from FatFs outside QuickJS. Dynamic handlers should run through `qjs_service_run()` so HTTP server tasks never call QuickJS directly.

### Prompt Context

**User prompt (verbatim):**

> Create a new ticket for each, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.
>
> Then add tasks and work on them in sequence, keeping a detailed log as you go.

**Assistant interpretation:** Create an HTTP ticket with a detailed guide and task list, using `go-go-goja` as inspiration for a simple embedded Express-like API.

**Inferred user intent:** Add web serving to the AtomS3R QuickJS firmware in a way that is understandable, constrained, and compatible with the existing storage and WiFi work.

**Commit (code):** N/A — design-only step.

### What I did

- Created ticket `ATOMS3R-M12-QUICKJS-HTTP`.
- Added a design document and diary document.
- Replaced the generated task list with phased HTTP tasks.
- Inspected `go-go-goja` references:
  - `modules/express/express.go`
  - `modules/express/typescript.go`
  - `modules/fs/http.go`
- Wrote the HTTP analysis/design/implementation guide.

### Why

- HTTP serving is a separate subsystem from WiFi and storage.
- The intern guide needs to explain ESP-IDF HTTP serving, static assets, QuickJS dispatch, route tables, request/response DTOs, and embedded limits.
- The user asked for Express-like ergonomics without auth complexity.

### What worked

- The guide defines a global `http` API, static file serving, dynamic route dispatch, limits, route lifecycle, and reset behavior.
- The task list sequences host service, static assets, QuickJS route registration, and script workflow.

### What didn't work

- N/A for this documentation step.

### What I learned

- The relevant Goja pattern is host-owned HTTP server lifecycle plus JavaScript route registration. The embedded firmware should keep that idea but avoid desktop-only features such as generic `http.Handler` mounting and auth builders.

### What was tricky to build

- The design needed to balance Express familiarity with QuickJS ownership constraints. The result is not a full Express clone; it is an embedded global `http` object with familiar methods and strict limits.

### What warrants a second pair of eyes

- Review whether dynamic JavaScript routes should be included in the first HTTP implementation or delayed until static serving is validated.
- Review response/body limits before implementation.
- Review runtime-reset behavior for registered JS handlers.

### What should be done in the future

- Implement WiFi first.
- Add `esp_http_server` host service with `/healthz`.
- Add static storage-backed serving before dynamic QuickJS handlers.
- Add route registration only after server and storage serving are stable.

### Code review instructions

- Start with the HTTP guide's architecture and pseudocode.
- Compare against `go-go-goja/modules/express/express.go` for API inspiration, but avoid copying auth machinery.
- Validate static serving before dynamic route dispatch.

### Technical details

- Ticket path: `ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HTTP--atoms3r-m12-quickjs-express-like-http-server/`.
- Design doc: `design-doc/01-analysis-design-and-implementation-guide.md`.
- Goja references:
  - `/home/manuel/code/wesen/go-go-golems/go-go-goja/modules/express/express.go`
  - `/home/manuel/code/wesen/go-go-golems/go-go-goja/modules/express/typescript.go`
  - `/home/manuel/code/wesen/go-go-golems/go-go-goja/modules/fs/http.go`
