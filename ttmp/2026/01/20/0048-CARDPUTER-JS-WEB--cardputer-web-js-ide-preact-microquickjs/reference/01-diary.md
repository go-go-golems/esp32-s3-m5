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
LastUpdated: 2026-01-21T00:00:00-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Maintain a high-frequency, highly detailed record of work on `0048-CARDPUTER-JS-WEB`: what changed, why, what commands ran, what failed, and how to review/validate.

## Step 1: Bootstrap ticket + vocabulary + diary

This ticket is documentation-first: it aims to produce a “textbook style” design-and-playbook bundle for building a Cardputer-hosted Web IDE (Preact/Zustand editor UI + REST execution endpoint backed by MicroQuickJS), and then extending it with WebSocket telemetry (encoder position + click).

I started by ensuring the docmgr workspace was healthy, then created the ticket workspace, and finally created a `Diary` reference doc to record the work as it happens.

**Commit (docs):** bb71c08 — "docmgr: create ticket 0048-CARDPUTER-JS-WEB"  
**Commit (diary/changelog):** 8577fd5 — "Diary: step 1 bootstrap ticket"

### What I did
- Confirmed docmgr root and vocabulary location:
  - `docmgr status --summary-only`
- Added missing topics to vocabulary:
  - `docmgr vocab add --category topics --slug javascript --description "JavaScript language and runtimes"`
  - `docmgr vocab add --category topics --slug quickjs --description "QuickJS JavaScript engine"`
  - `docmgr vocab add --category topics --slug microquickjs --description "micro-quickjs embedding wrapper for QuickJS"`
  - `docmgr vocab add --category topics --slug rest --description "REST-style HTTP APIs"`
- Created the ticket workspace:
  - `docmgr ticket create-ticket --ticket 0048-CARDPUTER-JS-WEB --title "Cardputer: Web JS IDE (Preact + microquickjs)" --topics cardputer,esp-idf,webserver,http,rest,websocket,preact,zustand,javascript,quickjs,microquickjs`
- Created this diary document:
  - `docmgr doc add --ticket 0048-CARDPUTER-JS-WEB --doc-type reference --title "Diary"`
- Updated ticket changelog (bootstrap entry):
  - `docmgr changelog update --ticket 0048-CARDPUTER-JS-WEB --entry "Step 1: Bootstrap ticket + vocabulary + diary (commit bb71c08)" ...`

### Why
- Vocabulary keeps topics consistent across tickets and makes docmgr validation/search useful.
- Diary provides provenance and a “how to review” trail.

### What worked
- Ticket workspace created under:
  - `esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/`

### What didn't work
- N/A

### What I learned
- This repo’s docmgr root is `esp32-s3-m5/ttmp` (not top-level `ttmp/`).

### What was tricky to build
- N/A (bootstrap-only step)

### What warrants a second pair of eyes
- Confirm new topic slugs are consistent with existing taxonomy.

### What should be done in the future
- N/A

### Code review instructions
- Review workspace skeleton and vocabulary:
  - `esp32-s3-m5/ttmp/vocabulary.yaml`
  - `esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/index.md`

### Technical details
- docmgr status showed:
  - Root: `esp32-s3-m5/ttmp`
  - Config: `.ttmp.yaml`
  - Vocabulary: `esp32-s3-m5/ttmp/vocabulary.yaml`

## Step 2: Locate prior art (docs + firmwares) and record a reading list

The repo already contains multiple “nearly the same” systems: embedded Preact/Zustand UIs served by `esp_http_server`, multiple WebSocket implementations, and a MicroQuickJS REPL that already solves “evaluate code + format result + show exceptions”. The key to writing a credible design doc is to anchor every major claim to a concrete file and symbol in this repo.

So I searched docmgr for key terms (websocket, esp_http_server, preact, spiffs, encoder) and opened the highest-signal tickets. Then I created a dedicated “Prior Art and Reading List” reference doc and filled it with exact filenames and key symbols to start from.

**Commit (docs):** 82fa6d7 — "Docs: add prior art reading list (0048)"  
**Commit (diary/changelog):** 152b1a5 — "Diary: step 2 prior art map"

### What I did
- Searched docmgr:
  - `docmgr doc search --query "esp_http_server"`
  - `docmgr doc search --query "websocket"`
  - `docmgr doc search --query "preact"`
  - `docmgr doc search --query "SPIFFS"`
  - `docmgr doc search --query "encoder"`
- Created and filled the reading list doc:
  - `docmgr doc add --ticket 0048-CARDPUTER-JS-WEB --doc-type reference --title "Prior Art and Reading List"`
  - Edited: `reference/02-prior-art-and-reading-list.md`
- Related canonical implementation files (absolute paths) to the doc:
  - `docmgr doc relate --doc .../reference/02-prior-art-and-reading-list.md --file-note "...:reason"`
- Updated ticket changelog:
  - `docmgr changelog update --ticket 0048-CARDPUTER-JS-WEB --entry "Step 2: Locate prior art + record reading list (commit 82fa6d7)" ...`

### Why
- Phase 1/2 docs must be reproducible: a reviewer should be able to jump from a claim to the reference implementation file/symbol.

### What worked
- docmgr search surfaced strong, on-point prior art:
  - `0017-atoms3r-web-ui` (embedded assets + WS patterns)
  - `0029-mock-zigbee-http-hub` (WS broadcaster patterns + playbooks)
  - `imports/esp32-mqjs-repl` (MicroQuickJS evaluator and stdlib wiring)
  - MO-033 / MO-036 docs for device-hosted UI and encoder semantics

### What didn't work
- N/A

### What I learned
- There is already a concrete “eval line → printable output” contract in:
  - `imports/esp32-mqjs-repl/.../eval/JsEvaluator.cpp` (`JsEvaluator::EvalLine`, `JS_Eval`, `JS_GetException`, `JS_PrintValueF`)

### What was tricky to build
- Keeping the reading list tight enough to be usable.

### What warrants a second pair of eyes
- Confirm the chosen canonical examples (0017 vs 0029) are the right ones to imitate for `0048`.

### What should be done in the future
- Extend the reading list as new constraints appear (e.g., editor library impacts bundle size).

### Code review instructions
- Review:
  - `esp32-s3-m5/ttmp/.../reference/02-prior-art-and-reading-list.md`

### Technical details
- N/A

## Step 3: Draft Phase 1 design doc (REST eval + embedded editor UI)

With prior art located, I wrote the Phase 1 design as a “textbook”: it defines the system contract (routes + request/response schemas), decomposes the firmware into modules, and makes embedded constraints explicit (bounded request sizes, timeouts, memory budgets).

**Commit (docs):** 1daac0a — "Docs: Phase 1 Web JS IDE design (0048)"  
**Commit (diary/changelog):** 740c825 — "Diary: step 3 Phase 1 design drafted"

### What I did
- Created and filled the Phase 1 design doc:
  - `design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`
- Related canonical code files to the design doc:
  - `docmgr doc relate --doc .../design-doc/01-...md --file-note "...:reason"`
- Updated ticket changelog with a Phase 1 design milestone.

### Why
- Phase 1 is the spine: once REST eval + embedded assets work, Phase 2 is “just” WS telemetry.

### What worked
- The repo already contains building blocks that map cleanly onto this design:
  - `0017-atoms3r-web-ui/main/http_server.cpp`
  - `imports/esp32-mqjs-repl/.../eval/JsEvaluator.cpp`

### What didn't work
- N/A

### What I learned
- Treating timeouts as first-class requirements changes architecture: you must plan for `JS_SetInterruptHandler` early.

### What was tricky to build
- Balancing completeness and focus: implementable but not a 50-page treatise on HTTP and JS engines.

### What warrants a second pair of eyes
- REST schema minimality and evolvability (`/api/js/eval` response shape).
- Interpreter lifecycle (single VM + mutex vs per-session/per-request).

### What should be done in the future
- If UI needs richer output semantics, split return-value formatting vs captured logs vs exceptions.

### Code review instructions
- Start here:
  - `esp32-s3-m5/ttmp/.../design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`

### Technical details
- N/A

## Step 4: Draft Phase 2 design doc (encoder telemetry over WebSocket)

Phase 2 adds realtime device→browser telemetry: encoder position + click streamed over WebSocket. The design makes the signal model explicit (“authoritative state + optional deltas”), defines a minimal JSON schema, and specifies bounded-rate broadcasting and reconnect-friendly semantics.

**Commit (docs):** feae3f7 — "Docs: Phase 2 WS encoder telemetry design (0048)"  
**Commit (diary/changelog):** a2b1959 — "Diary: step 4 Phase 2 WS encoder design"

### What I did
- Created and filled:
  - `design-doc/02-phase-2-design-encoder-position-click-over-websocket.md`
- Related WS broadcaster prior art and encoder protocol references to the design doc.
- Updated ticket changelog with a Phase 2 design milestone.

### Why
- WS changes failure modes (disconnect/backpressure/bursts); the design must pre-commit to coalescing/rate strategy.

### What worked
- WS patterns in repo already solve client tracking + async send:
  - `0017-atoms3r-web-ui/main/http_server.cpp`
  - `0029-mock-zigbee-http-hub/main/hub_http.c`

### What didn't work
- N/A

### What I learned
- For human-facing UIs, coalescing bursts into a bounded-rate “latest state” stream is usually better than emitting every tick.

### What was tricky to build
- Hardware ambiguity: “encoder” might be built-in or a Chain Encoder; design isolates via a small driver interface.

### What warrants a second pair of eyes
- Rate defaults (sample vs broadcast) and schema evolution strategy (`type`, `seq`).

### What should be done in the future
- If richer click semantics are needed (double/long press), extend schema carefully.

### Code review instructions
- Read:
  - `esp32-s3-m5/ttmp/.../design-doc/02-phase-2-design-encoder-position-click-over-websocket.md`

### Technical details
- N/A

## Step 5: Add playbooks + wire ticket index/tasks

Design docs without repeatable validation procedures rot. I added Phase 1/Phase 2 smoke-test playbooks and updated the ticket index so a new reader can navigate quickly.

**Commit (docs):** 1d235f8 — "Docs: add 0048 playbooks + update index/tasks"  
**Commit (diary/changelog):** 335b68e — "Diary: step 5 playbooks + index wiring"

### What I did
- Created and filled:
  - `playbook/01-playbook-phase-1-web-ide-smoke-test-build-connect-run-snippets.md`
  - `playbook/02-playbook-phase-2-websocket-encoder-telemetry-smoke-test.md`
- Updated:
  - `index.md` and `tasks.md`
- Updated ticket changelog with the playbooks milestone.

### Why
- Playbooks convert architecture into confidence and make regression checking possible.

### What worked
- Phase 1 playbook can validate via curl + browser.
- Phase 2 playbook can validate via `websocat`/`wscat` + browser.

### What didn't work
- N/A

### What I learned
- “Operationalization” (exit criteria + procedures) is what makes documentation actionable.

### What was tricky to build
- Writing playbooks without assuming a final firmware directory name (used an explicit placeholder).

### What warrants a second pair of eyes
- Confirm endpoint paths and assumptions match the intended implementation (`/api/js/eval`, `/ws`).

### What should be done in the future
- Once the firmware exists, replace placeholders with the real project path and exact commands.

### Code review instructions
- Start with:
  - `esp32-s3-m5/ttmp/.../index.md`

### Technical details
- N/A

## Step 6: Upload initial bundle to reMarkable

I exported the “core reading set” as a single bundled PDF and uploaded it to reMarkable.

### What I did
- Verified:
  - `remarquee status`
- Dry-run:
  - `remarquee upload bundle --dry-run <...> --name "0048 CARDPUTER JS WEB (Design + Playbooks + Diary)" --remote-dir "/ai/2026/01/21/0048-CARDPUTER-JS-WEB" --toc-depth 3`
- Upload:
  - `remarquee upload bundle ... --name "0048 CARDPUTER JS WEB (Design + Playbooks + Diary)" --remote-dir "/ai/2026/01/21/0048-CARDPUTER-JS-WEB" --toc-depth 3`
- Verify:
  - `remarquee cloud ls /ai/2026/01/21/0048-CARDPUTER-JS-WEB --long --non-interactive`

### Why
- A single PDF with ToC is easier to review and annotate than many Markdown files.

### What worked
- Upload succeeded; document appears under:
  - `/ai/2026/01/21/0048-CARDPUTER-JS-WEB`

### What didn't work
- N/A

### What I learned
- Bundling is the best default export mode for tickets.

### What was tricky to build
- Avoiding overwrite: I did not use `--force` to preserve annotations.

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Upload versioned bundles (date/time suffix) as docs evolve.

### Code review instructions
- N/A

### Technical details
- Uploaded name: `0048 CARDPUTER JS WEB (Design + Playbooks + Diary).pdf`

## Step 7: Find and enumerate MicroQuickJS usage playbooks (docmgr search)

Before implementing the REST execution endpoint, I verified whether the repo already has canonical “how to use MicroQuickJS properly” guidance beyond the `JsEvaluator` code itself.

### What I did
- Searched docmgr:
  - `docmgr doc search --query "mquickjs"`
  - `docmgr doc search --query "microquickjs"`
  - `docmgr doc search --query "JsEvaluator"`
  - `docmgr doc search --query "JS_Eval"`

### Why
- MQJS has sharp edges (stdlib generation/wiring, memory pools, interrupt timeouts, SPIFFS integration). Reuse existing guidance where possible.

### What worked
- Search surfaced high-signal docs:
  - `0014-CARDPUTER-JS` docs and diary
  - `MO-02-ESP32-DOCS` MQJS guide/diary
  - `MO-02-MQJS-REPL-COMPONENTS` component extraction plan

### What didn't work
- N/A

### What I learned
- There is enough MQJS documentation in-repo to justify a single consolidated “house style” guide for our setting.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Link the MQJS guide to the most important legacy docs (extensions manual, etc.).

### Code review instructions
- N/A

### Technical details
- N/A

## Step 8: Consolidate “how to use MicroQuickJS properly” into a single guide

I created a consolidated reference doc that answers: what is the right way to embed and operate MicroQuickJS in our ESP-IDF setting (stdlib wiring, memory pools, timeouts, output capture, concurrency, extensions).

**Commit (docs):** 72d4fbc — "Docs: add MicroQuickJS usage guide (0048)"

### What I did
- Created and filled:
  - `reference/03-guide-using-microquickjs-mquickjs-in-our-esp-idf-setting.md`
- Related primary sources (absolute paths) to the guide:
  - `imports/esp32-mqjs-repl/.../eval/JsEvaluator.cpp`
  - `imports/esp32-mqjs-repl/.../esp32_stdlib_runtime.c`
  - `imports/esp32-mqjs-repl/.../esp32_stdlib.h`
  - plus extension playbooks and partition tables
- Updated ticket changelog with this guide entry.

### Why
- Without a single guide, every new JS evaluation surface rediscovers the same failure modes.

### What worked
- Existing code provides an evidence-based canonical pattern (`JsEvaluator`) and a canonical stdlib wiring strategy.

### What didn't work
- N/A

### What I learned
- In this repo, `js_stdlib` is defined in a generated header included by the runtime C file; “half copying” causes link failures or subtle mismatches.

### What was tricky to build
- Keeping the guide concise while still making invariants explicit (timeouts, concurrency).

### What warrants a second pair of eyes
- Confirm the guide’s “house rules” match expectations (mutex vs dedicated task ownership).

### What should be done in the future
- As implementation proceeds, append any newly observed failure modes and their remedies.

### Code review instructions
- Read:
  - `esp32-s3-m5/ttmp/.../reference/03-guide-using-microquickjs-mquickjs-in-our-esp-idf-setting.md`

### Technical details
- N/A

## Step 9: Upload updated bundle (includes MQJS guide) to reMarkable

Once the MicroQuickJS guide existed, I exported an updated ticket bundle and uploaded it to reMarkable as a second (versioned) document.

### What I did
- Dry-run:
  - `remarquee upload bundle --dry-run <...> --name "0048 CARDPUTER JS WEB (Docs + MQJS Guide) 2026-01-21" --remote-dir "/ai/2026/01/21/0048-CARDPUTER-JS-WEB" --toc-depth 3`
- Upload:
  - `remarquee upload bundle ... --name "0048 CARDPUTER JS WEB (Docs + MQJS Guide) 2026-01-21" --remote-dir "/ai/2026/01/21/0048-CARDPUTER-JS-WEB" --toc-depth 3`
- Verify:
  - `remarquee cloud ls /ai/2026/01/21/0048-CARDPUTER-JS-WEB --long --non-interactive`

### Why
- Bundling keeps the MQJS guide adjacent to the designs and playbooks for offline review.

### What worked
- The updated PDF appeared under:
  - `/ai/2026/01/21/0048-CARDPUTER-JS-WEB`

### What didn't work
- N/A

### What I learned
- Avoiding overwrite preserves annotations by keeping multiple versions.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Continue uploading versioned bundles as implementation progresses.

### Code review instructions
- N/A

### Technical details
- Uploaded name: `0048 CARDPUTER JS WEB (Docs + MQJS Guide) 2026-01-21.pdf`

## Step 10: Backfill diary + expand implementation task breakdown

After additional work (uploads, MQJS guide, and implementation planning) accumulated outside the original diary steps, I rewrote the diary into a clean, strictly ordered set of steps that captures *everything done so far*. I also expanded the ticket task list from a few one-liners into a granular execution checklist organized by subsystem (Wi‑Fi, HTTP/assets, JS runner, frontend editor, Phase 2 WS telemetry, validation).

**Commit:** TBD

### What I did
- Rewrote this diary file for completeness and chronological clarity:
  - `reference/01-diary.md`
- Expanded the implementation checklist:
  - `tasks.md`

### Why
- The diary is only useful if it is complete and reviewable; scattered partial notes are not enough.
- Granular tasks enable incremental commits and avoid “big bang” integration.

### What worked
- Existing prior art (0017/0029/MQJS REPL) provides natural task boundaries and implementation milestones.

### What didn't work
- N/A

### What I learned
- Having an explicit “stdlib strategy” task forces an early decision about whether to reuse the existing generated `js_stdlib` or generate a minimal one.

### What was tricky to build
- Keeping tasks detailed without duplicating the design docs.

### What warrants a second pair of eyes
- Confirm task breakdown matches the intended MVP path (Phase 1 first, Phase 2 later).

### What should be done in the future
- Check off tasks as they are implemented and add newly discovered subtasks (especially build + asset pipeline quirks).

### Code review instructions
- Review:
  - `esp32-s3-m5/ttmp/.../tasks.md`
  - `esp32-s3-m5/ttmp/.../reference/01-diary.md`

### Technical details
- N/A
