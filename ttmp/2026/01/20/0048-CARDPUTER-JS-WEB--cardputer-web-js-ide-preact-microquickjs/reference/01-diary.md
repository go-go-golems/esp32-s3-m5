---
Title: Diary
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T22:54:45.661772488-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Maintain a high-frequency, highly detailed record of how the documentation for `0048-CARDPUTER-JS-WEB` was produced: what I looked at, what I changed, which commands I ran, what failed, and how to review the result.

## Step 1: Bootstrap ticket + vocabulary + diary

This ticket is documentation-first: it aims to produce a “textbook style” design-and-playbook bundle for building a Cardputer-hosted Web IDE (Preact/Zustand editor UI + REST execution endpoint backed by micro-quickjs), and then extending it with WebSocket telemetry (encoder position + click).

I started by ensuring the docmgr workspace was healthy, then created the ticket workspace, and finally created a `Diary` reference doc to record the work as it happens.

**Commit (docs):** bb71c08 — "docmgr: create ticket 0048-CARDPUTER-JS-WEB"

### What I did
- Confirmed docmgr root and vocabulary location via:
  - `docmgr status --summary-only`
- Added missing topics to vocabulary so the ticket can tag QuickJS-related work without “unknown topic” warnings:
  - `docmgr vocab add --category topics --slug javascript --description "JavaScript language and runtimes"`
  - `docmgr vocab add --category topics --slug quickjs --description "QuickJS JavaScript engine"`
  - `docmgr vocab add --category topics --slug microquickjs --description "micro-quickjs embedding wrapper for QuickJS"`
  - `docmgr vocab add --category topics --slug rest --description "REST-style HTTP APIs"`
- Created the ticket workspace:
  - `docmgr ticket create-ticket --ticket 0048-CARDPUTER-JS-WEB --title "Cardputer: Web JS IDE (Preact + microquickjs)" --topics cardputer,esp-idf,webserver,http,rest,websocket,preact,zustand,javascript,quickjs,microquickjs`
- Created this diary document:
  - `docmgr doc add --ticket 0048-CARDPUTER-JS-WEB --doc-type reference --title "Diary"`

### Why
- The vocabulary needs to name the concepts we’ll repeatedly reference (QuickJS, micro-quickjs, JavaScript, REST) so doc search, validation, and future cross-linking behave well.
- The diary is required by the task: it functions as both provenance and a review guide (what to read first, what commands/paths matter).

### What worked
- `docmgr` is already initialized in this repo, and the ticket was created successfully under:
  - `esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/`

### What didn't work
- N/A

### What I learned
- This repo’s configured docmgr root is `esp32-s3-m5/ttmp` (not a top-level `ttmp/`).

### What was tricky to build
- N/A (bootstrap-only step)

### What warrants a second pair of eyes
- Topic vocabulary additions: confirm the slugs and descriptions align with existing taxonomy conventions.

### What should be done in the future
- If the ticket’s date directory is unexpected (e.g. created under `2026/01/20`), confirm local timezone/time source assumptions for doc generation. Otherwise, accept docmgr’s default behavior.

### Code review instructions
- Review the ticket workspace skeleton and diary location:
  - `esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/index.md`
  - `esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/01-diary.md`
  - `esp32-s3-m5/ttmp/vocabulary.yaml`

### Technical details
- docmgr status at start (for orientation):
  - Root: `esp32-s3-m5/ttmp`
  - Config: `.ttmp.yaml`
  - Vocabulary: `esp32-s3-m5/ttmp/vocabulary.yaml`

## Step 2: Locate prior art (docs + firmwares) and record a reading list

The repo already contains multiple “nearly the same” systems: embedded Preact/Zustand UIs served by `esp_http_server`, multiple WebSocket implementations, and a MicroQuickJS REPL that already solves “evaluate code + format result + show exceptions”. The key to writing a credible design doc is to anchor every major claim to a concrete file and symbol in this repo.

So I searched docmgr for key terms (websocket, esp_http_server, preact, spiffs, encoder) and opened the highest-signal tickets. Then I created a dedicated “Prior Art and Reading List” reference doc and filled it with the *exact* filenames and symbols to start from.

**Commit (docs):** 82fa6d7 — "Docs: add prior art reading list (0048)"

### What I did
- Searched docmgr for existing work:
  - `docmgr doc search --query "esp_http_server"`
  - `docmgr doc search --query "websocket"`
  - `docmgr doc search --query "preact"`
  - `docmgr doc search --query "SPIFFS"`
  - `docmgr doc search --query "encoder"`
- Opened and skimmed the most relevant docs (frontmatter + executive summary + file pointers):
  - `ttmp/.../0013-ATOMS3R-WEBSERVER.../design-doc/02-final-design-...md`
  - `ttmp/.../MO-033-ESP32C6-PREACT-WEB.../design-doc/01-design-...md`
  - `ttmp/.../0014-CARDPUTER-JS.../design-doc/02-split-firmware-...md`
  - `ttmp/.../0029-HTTP-EVENT-MOCK-ZIGBEE.../playbook/01-websocket-over-wi-fi-esp-idf-playbook.md`
  - `ttmp/.../MO-036-CHAIN-ENCODER-LVGL.../design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md`
- Created a reference doc and wrote the reading list:
  - `docmgr doc add --ticket 0048-CARDPUTER-JS-WEB --doc-type reference --title "Prior Art and Reading List"`
  - Edited: `.../reference/02-prior-art-and-reading-list.md`
- Related the “canonical” implementation files to that doc via absolute paths:
  - `docmgr doc relate --doc .../reference/02-prior-art-and-reading-list.md --file-note "...:reason"`

### Why
- The Phase 1/Phase 2 design docs must be “reproducible”: a reviewer should be able to jump directly from a statement to the example implementation that motivated it.
- The repo already contains mature patterns (embedded assets, WS broadcasting, MicroQuickJS evaluation) that should be reused instead of re-invented.

### What worked
- docmgr search surfaced strong, on-point tickets:
  - 0013 (device-hosted UI + WS)
  - MO-033 (embedded Preact/Zustand pipeline)
  - 0014 (MicroQuickJS evaluator/REPL architecture)
  - MO-036 (encoder protocol + LVGL integration)

### What didn't work
- N/A

### What I learned
- There is already a MicroQuickJS evaluator with an explicit “eval line → printable output” contract:
  - `imports/esp32-mqjs-repl/.../eval/JsEvaluator.cpp` (`JsEvaluator::EvalLine`, `JS_Eval`, `JS_GetException`, `JS_PrintValueF`)
- There is already a compact C++ WS broadcast pattern with client tracking:
  - `0017-atoms3r-web-ui/main/http_server.cpp` (`http_server_ws_broadcast_text`, `http_server_ws_broadcast_binary`)

### What was tricky to build
- Keeping the “reading list” tight: too many links make it useless; too few links risk missing important constraints (e.g., deterministic asset bundling, WS send semantics).

### What warrants a second pair of eyes
- Verify the reading list is *complete enough* for Phase 1/Phase 2, and that the chosen “canonical examples” are the right ones to imitate (0017 vs 0029 vs other WS variants).

### What should be done in the future
- Expand the reading list as new constraints appear (e.g., if the chosen code editor library changes the asset pipeline or memory budget).

### Code review instructions
- Start with:
  - `esp32-s3-m5/ttmp/.../reference/02-prior-art-and-reading-list.md`
- Verify it cites real files/symbols in:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`

### Technical details
- N/A (survey/documentation-only step)

## Step 3: Write Phase 1 design doc (REST eval + embedded editor UI)

With prior art located, I wrote the Phase 1 design as a “textbook”: it defines the system contract (routes + request/response schemas), decomposes the firmware into modules, and makes the implicit embedded constraints explicit (bounded request sizes, timeouts, memory budgets). The writing goal here is not “pretty prose”; it is to make implementation and review *mechanical*—a reader should be able to implement the MVP by following the implementation plan and jumping to the referenced symbols in the repo.

I also related the highest-leverage files (0017 HTTP server + asset pipeline, and the MicroQuickJS `JsEvaluator`) so the doc is anchored in evidence rather than speculation.

### What I did
- Created the Phase 1 design doc:
  - `docmgr doc add --ticket 0048-CARDPUTER-JS-WEB --doc-type design-doc --title "Phase 1 Design: Device-hosted Web JS IDE (Preact+Zustand + MicroQuickJS over REST)"`
- Wrote the Phase 1 design content:
  - `.../design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`
- Related “canonical” code files to the design doc via absolute paths:
  - `docmgr doc relate --doc .../design-doc/01-...md --file-note "...:reason"`

### Why
- Phase 1 is the “spine” of the project: once REST eval + embedded assets work, Phase 2 is “just” another transport (WS) for telemetry.
- The design must explicitly state limits and failure modes; otherwise the system fails in predictable embedded ways (OOM, long-running JS, route drift due to hashed assets).

### What worked
- The repo already contains proven building blocks that map cleanly onto this design:
  - `http_server_start()` and embedded asset symbols in `0017-atoms3r-web-ui/main/http_server.cpp`
  - `JsEvaluator::EvalLine()` in `imports/esp32-mqjs-repl/.../eval/JsEvaluator.cpp`

### What didn't work
- N/A

### What I learned
- Treating “timeouts” as a first-class requirement changes architectural choices (interrupt handler vs “hope user code is short”).

### What was tricky to build
- Balancing completeness and focus: Phase 1 needs enough detail to be implementable without turning into an entire book on HTTP, JS engines, and bundlers.

### What warrants a second pair of eyes
- REST contract shape (`/api/js/eval` response schema): confirm it’s the minimal useful contract for the UI and that it can evolve without breaking clients.
- Interpreter lifecycle decision (single VM vs per-request/per-session): confirm the chosen default (single VM + mutex) matches expected UX and threat model.

### What should be done in the future
- If Phase 1 implementation discovers “UI needs richer output semantics”, revisit the response schema to separate:
  - return value formatting vs captured logs vs exceptions.

### Code review instructions
- Start here:
  - `esp32-s3-m5/ttmp/.../design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`
- Spot-check that the referenced symbols exist in:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`

### Technical details
- N/A (documentation-only step)
