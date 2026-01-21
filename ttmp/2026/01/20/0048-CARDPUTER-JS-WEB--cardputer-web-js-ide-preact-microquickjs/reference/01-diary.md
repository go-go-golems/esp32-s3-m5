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

**Commit (docs):** 1daac0a — "Docs: Phase 1 Web JS IDE design (0048)"

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

## Step 5: Add Phase 1/2 playbooks + wire ticket index/tasks

At this point the design docs exist, but a design without an executable “how to validate it” procedure tends to rot. So I added two playbooks (Phase 1 and Phase 2 smoke tests) that turn the designs into repeatable checklists. I also updated the ticket index and task list so a new reader can start at `index.md` and immediately find the correct docs and the concrete next implementation steps.

**Commit (docs):** 1d235f8 — "Docs: add 0048 playbooks + update index/tasks"

### What I did
- Created and wrote Phase 1 playbook:
  - `.../playbook/01-playbook-phase-1-web-ide-smoke-test-build-connect-run-snippets.md`
- Created and wrote Phase 2 playbook:
  - `.../playbook/02-playbook-phase-2-websocket-encoder-telemetry-smoke-test.md`
- Updated ticket index and tasks:
  - `.../index.md`
  - `.../tasks.md`
- Related “ticket-level” canonical files to `index.md`:
  - `docmgr doc relate --ticket 0048-CARDPUTER-JS-WEB --file-note "...:reason"`

### Why
- Playbooks are the bridge between “architecture” and “confidence”: they make success criteria explicit and give future maintainers a reliable regression checklist.
- Updating the index/tasks prevents the ticket from becoming a pile of unlinked documents.

### What worked
- The playbooks can lean on existing repo conventions (Phase 1: curl-based eval tests; Phase 2: websocat/wscat + browser UI).

### What didn't work
- N/A

### What I learned
- Even for documentation-only tickets, the “operationalization” step (playbooks + exit criteria) is what makes the docs actionable.

### What was tricky to build
- Writing playbooks in a way that is concrete without assuming a final firmware directory name; the playbooks therefore use `esp32-s3-m5/0048-cardputer-js-web` as an explicit placeholder.

### What warrants a second pair of eyes
- Confirm the playbooks’ assumptions match the expected eventual implementation:
  - route paths (`/api/js/eval`, `/ws`)
  - suggested firmware directory naming

### What should be done in the future
- Once the actual firmware project exists, update the playbooks to remove placeholders and to include the exact console/Wi‑Fi steps used by the chosen implementation.

### Code review instructions
- Verify the index is now a functional entry point:
  - `esp32-s3-m5/ttmp/.../index.md`
- Review the playbooks for clarity and correctness:
  - `esp32-s3-m5/ttmp/.../playbook/01-playbook-phase-1-web-ide-smoke-test-build-connect-run-snippets.md`
  - `esp32-s3-m5/ttmp/.../playbook/02-playbook-phase-2-websocket-encoder-telemetry-smoke-test.md`

### Technical details
- N/A (documentation-only step)

## Step 6: Upload current ticket bundle to reMarkable

After the Phase 1/Phase 2 docs and playbooks existed, I exported the “core reading set” as a single bundled PDF and uploaded it to the reMarkable. This is useful because it turns the ticket into a portable artifact that can be reviewed without a laptop, and because it preserves the exact snapshot of documentation that the implementation work should follow.

Notably, this upload step does not modify the git repo; it’s an external sync action. I still record it here because it’s part of the workflow provenance.

### What I did
- Verified the uploader tool is available:
  - `remarquee status`
- Dry-run a bundle upload (to ensure the right docs are included and ordered):
  - `remarquee upload bundle --dry-run <index + prior art + designs + playbooks + diary> --name "0048 CARDPUTER JS WEB (Design + Playbooks + Diary)" --remote-dir "/ai/2026/01/21/0048-CARDPUTER-JS-WEB" --toc-depth 3`
- Uploaded the bundle (no overwrite; new doc):
  - `remarquee upload bundle ... --name "0048 CARDPUTER JS WEB (Design + Playbooks + Diary)" --remote-dir "/ai/2026/01/21/0048-CARDPUTER-JS-WEB" --toc-depth 3`
- Verified remote contents:
  - `remarquee cloud ls /ai/2026/01/21/0048-CARDPUTER-JS-WEB --long --non-interactive`

### Why
- A single PDF with a ToC is easier to review and annotate on reMarkable than a folder of Markdown files.

### What worked
- `remarquee upload bundle` succeeded and the PDF appeared under:
  - `/ai/2026/01/21/0048-CARDPUTER-JS-WEB`

### What didn't work
- N/A

### What I learned
- Bundling is the best “default” export mode for tickets: it captures narrative + contracts + playbooks together.

### What was tricky to build
- Choosing a stable remote name that won’t overwrite annotations. I avoided `--force` and used a new upload.

### What warrants a second pair of eyes
- N/A (operational step)

### What should be done in the future
- When implementation begins and docs evolve, re-upload an updated bundle with a new name suffix (date/time) to avoid destroying annotations.

### Code review instructions
- N/A

### Technical details
- Uploaded bundle name: `0048 CARDPUTER JS WEB (Design + Playbooks + Diary).pdf`

## Step 7: Find and enumerate MicroQuickJS usage playbooks (docmgr search)

Before implementing the REST execution endpoint, I wanted to confirm whether the repo already has “how to use MicroQuickJS properly” guidance beyond the `JsEvaluator` code itself. So I used `docmgr doc search` for `mquickjs`, `microquickjs`, `JsEvaluator`, and `JS_Eval` and then captured the top relevant results.

### What I did
- Searched docmgr:
  - `docmgr doc search --query "mquickjs"`
  - `docmgr doc search --query "microquickjs"`
  - `docmgr doc search --query "JsEvaluator"`
  - `docmgr doc search --query "JS_Eval"`

### Why
- MicroQuickJS has sharp edges (stdlib generation, memory pools, interrupt handling, SPIFFS integration). If the repo already has canonical guidance, we should cite and reuse it.

### What worked
- Search surfaced several high-signal documents:
  - `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`
  - `ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/15-guide-microquickjs-repl-extensions.md`
  - `ttmp/2026/01/11/MO-02-MQJS-REPL-COMPONENTS--extract-microquickjs-repl-components/design-doc/01-component-extraction-plan-replloop-jsevaluator-spiffs.md`
  - Plus the original 0014 design docs and diary.

### What didn't work
- N/A

### What I learned
- There is enough existing MicroQuickJS documentation in this repo to justify writing a “single consolidated guide” for our specific setting, rather than relying on scattered tickets.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Add the newly found MQJS docs to `0048`’s prior art list (or link them from the new MQJS guide doc).

### Code review instructions
- N/A

### Technical details
- N/A

## Step 8: Consolidate “how to use MicroQuickJS properly” into a single guide

At this point, I had (a) our Phase 1/Phase 2 designs, and (b) lots of scattered MicroQuickJS prior art across tickets and code. The missing piece was a single “house style” guide that answers: *what is the right way to embed MicroQuickJS in ESP-IDF in this repo?* In particular, we repeatedly need to get the details right for:

- wiring the stdlib (`js_stdlib`) correctly (it’s generated and easy to mismatch),
- using a fixed memory pool (predictability and anti-fragmentation),
- bounding evaluation time (interrupt handler) to prevent hangs,
- producing stable, UI-friendly output strings,
- and adding extensions safely.

So I created a consolidated reference doc and grounded it in the concrete implementation files we already have.

### What I did
- Created the doc:
  - `docmgr doc add --ticket 0048-CARDPUTER-JS-WEB --doc-type reference --title "Guide: Using MicroQuickJS (mquickjs) in our ESP-IDF setting"`
- Wrote the guide content:
  - `.../reference/03-guide-using-microquickjs-mquickjs-in-our-esp-idf-setting.md`
- Related the primary in-repo sources (absolute paths) to the guide:
  - `docmgr doc relate --doc .../reference/03-...md --file-note "...:reason"`

### Why
- Without a single guide, every new “JS evaluation surface” (REPL, REST, WS-driven scripts) re-discovers the same sharp edges.

### What worked
- The existing codebase already contains a canonical evaluation pattern (`JsEvaluator`) and a canonical stdlib wiring (`esp32_stdlib_runtime.c` + generated `esp32_stdlib.h`). That made it possible to write a guide that is evidence-based rather than speculative.

### What didn't work
- N/A

### What I learned
- The `js_stdlib` contract in this repo is “non-obvious but stable”: it is defined in a generated header and included by the runtime C file, so “half-copying” pieces causes link failures or subtle mismatches.

### What was tricky to build
- Keeping the guide concise enough to be usable while still making the critical invariants explicit (memory pool, timeouts, concurrency).

### What warrants a second pair of eyes
- Confirm the “house rules” in the guide match actual expectations for future work (especially around concurrency model: mutex vs dedicated task ownership).

### What should be done in the future
- As we implement `0048-cardputer-js-web`, update the guide with any new failure modes we hit in practice (timeout behavior, output capture semantics, memory tuning).

### Code review instructions
- Read:
  - `esp32-s3-m5/ttmp/.../reference/03-guide-using-microquickjs-mquickjs-in-our-esp-idf-setting.md`
- Spot-check file pointers resolve:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h`

### Technical details
- N/A (documentation-only step)

## Step 4: Write Phase 2 design doc (encoder telemetry over WebSocket)

Phase 2 is where the system becomes “alive”: instead of the browser only sending requests (REST) and waiting for replies, the device continuously streams state changes (encoder position and click) to the UI. That shift forces us to specify ordering, rate limits, coalescing semantics, and reconnection behavior—exactly the parts that are easy to handwave and painful to debug later.

I wrote a Phase 2 design doc that makes the signal model explicit (“authoritative state + optional deltas”), defines a minimal JSON frame schema, and outlines a firmware architecture that remains stable under bursty encoder input. I grounded the design in the in-repo WS broadcaster patterns (0017/0029) and the Chain Encoder protocol prior art (MO-036 + protocol PDF).

**Commit (docs):** feae3f7 — "Docs: Phase 2 WS encoder telemetry design (0048)"

### What I did
- Created the Phase 2 design doc:
  - `docmgr doc add --ticket 0048-CARDPUTER-JS-WEB --doc-type design-doc --title "Phase 2 Design: Encoder position + click over WebSocket"`
- Wrote the Phase 2 design content:
  - `.../design-doc/02-phase-2-design-encoder-position-click-over-websocket.md`
- Related the key prior-art sources:
  - `docmgr doc relate --doc .../design-doc/02-...md --file-note "...:reason"`

### Why
- WebSocket changes the operational failure modes (disconnects, backpressure, bursts); design needs to pre-commit to a rate/coalescing model.
- The encoder driver itself is ambiguous (built-in vs Chain Encoder). The design must isolate that ambiguity behind a narrow interface.

### What worked
- WS prior art in this repo already solves the two hard parts:
  - client tracking + async send without blocking the server task
  - practical debugging strategy (playbook 0029)
- Encoder prior art already exists at the protocol level (MO-036), which reduces guesswork.

### What didn't work
- N/A

### What I learned
- For human-facing UIs, coalescing encoder bursts into a bounded-rate “latest state” stream produces a better UX than faithfully emitting every tick.

### What was tricky to build
- Writing a design that remains correct even if the “encoder” is not the Chain Encoder (hardware ambiguity).

### What warrants a second pair of eyes
- Message schema evolution strategy: ensure `type` and `seq` are sufficient for future extensions (logs/status) without breaking clients.
- Rate/coalescing defaults (sample vs broadcast): confirm they match expected UX and don’t starve other device work.

### What should be done in the future
- If Phase 2 implementation needs richer button semantics (double-click/long-press), extend the schema carefully (edge events vs state).

### Code review instructions
- Start here:
  - `esp32-s3-m5/ttmp/.../design-doc/02-phase-2-design-encoder-position-click-over-websocket.md`
- Verify the cited prior art exists:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/ttmp/.../MO-036-CHAIN-ENCODER-LVGL.../design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md`

### Technical details
- N/A (documentation-only step)
