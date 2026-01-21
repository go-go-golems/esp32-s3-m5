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
RelatedFiles:
    - Path: 0048-cardputer-js-web/CMakeLists.txt
      Note: Component dir wiring for mquickjs + exercizer_control
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: Phase 1 routes and asset serving
    - Path: 0048-cardputer-js-web/main/js_runner.cpp
      Note: MicroQuickJS init/eval/timeout hook
    - Path: 0048-cardputer-js-web/sdkconfig.defaults
      Note: Cardputer flash + partition table defaults for reproducible builds
    - Path: 0048-cardputer-js-web/web/src/ui/code_editor.tsx
      Note: CodeMirror 6 editor integration
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T00:00:00-05:00
WhatFor: ""
WhenToUse: ""
---



# Diary

## Goal

Maintain a high-frequency, highly detailed record of work on `0048-CARDPUTER-JS-WEB`: what changed, why, what commands ran, what failed, and how to review/validate.

## Index (step numbers)

This diary was written incrementally; step blocks may not appear in numeric order. Use this index to jump by step number.

- Step 1: Bootstrap ticket + vocabulary + diary
- Step 2: Locate prior art (docs + firmwares) and record a reading list
- Step 3: Draft Phase 1 design doc (REST eval + embedded editor UI)
- Step 4: Draft Phase 2 design doc (encoder telemetry over WebSocket)
- Step 5: Add playbooks + wire ticket index/tasks
- Step 6: Upload initial bundle to reMarkable
- Step 7: Find and enumerate MicroQuickJS usage playbooks (docmgr search)
- Step 8: Consolidate “how to use MicroQuickJS properly” into a single guide
- Step 9: Upload updated bundle (includes MQJS guide) to reMarkable
- Step 10: Backfill diary + expand implementation task breakdown
- Step 11: Start implementation (create 0048 firmware + web skeleton)
- Step 12: Attempt local firmware build (blocked: ESP-IDF tooling missing in this environment)
- Step 13: Replace `<textarea>` with CodeMirror 6 (JS highlighting + Mod-Enter run) and rebuild embedded assets
- Step 14: Source ESP-IDF 5.4.1, fix build blockers, and get the firmware building end-to-end
- Step 15: Switch from SoftAP-only to esp_console-configured STA Wi‑Fi (0046 pattern) and update `/api/status` + Phase 1 playbook

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

## Step 12: Attempt local firmware build (blocked: ESP-IDF tooling missing in this environment)

Before expanding the feature set, I tried to do the obvious “close the loop” validation: build the new ESP-IDF project locally so we can start iterating with confidence. That immediately uncovered an environment constraint: this sandbox did not have `idf.py` (ESP-IDF) available on `PATH`, and there was no vendored ESP-IDF checkout in the workspace to source.

Rather than guessing at compilation errors, I recorded the exact failure and proceeded with web-side work that can still be validated here (node toolchain exists), while leaving firmware build/flash validation for a machine that has ESP-IDF installed.

**Commit (code):** N/A

### What I did
- Tried to check the ESP-IDF tooling in the new project:
  - `cd esp32-s3-m5/0048-cardputer-js-web`
  - `idf.py --version`
- Confirmed there is no vendored `idf.py` in the workspace:
  - `find .. -maxdepth 3 -name idf.py`

### Why
- Phase 1 requires end-to-end build/flash smoke tests to validate assumptions about component linkage and embedded asset symbols.

### What worked
- The failure is explicit and immediately actionable: install/source ESP-IDF before attempting an `idf.py build`.

### What didn't work
- `idf.py --version` failed with:
  - `zsh:1: command not found: idf.py`

### What I learned
- This environment can run Node/Vite builds, but cannot run ESP-IDF builds without additional setup.

### What was tricky to build
- N/A (environment constraint, not a code change)

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Run the Phase 1 playbook on a machine with ESP-IDF installed and capture the first real `idf.py build` output in the diary (that’s where the real linkage issues will appear).

### Code review instructions
- N/A

### Technical details
- N/A

## Step 13: Replace `<textarea>` with CodeMirror 6 (JS highlighting + Mod-Enter run) and rebuild embedded assets

The initial UI used a `<textarea>` purely as a wiring placeholder. To make the Web IDE credible, I integrated CodeMirror 6 (the modern, modular editor) with Preact and the existing Zustand store.

This step adds a dedicated `CodeEditor` component that mounts a single `EditorView` instance, streams edits into Zustand without recreating the editor, and binds `Ctrl/Cmd-Enter` (`Mod-Enter`) to run the current program. Then it rebuilds the embedded assets so the firmware can serve the real editor UI from `main/assets/`.

**Commit (code):** b6f3f3e — "0048: use CodeMirror editor + rebuild embedded assets"

### What I did
- Added CodeMirror 6 dependencies and stabilized the build script:
  - Updated: `esp32-s3-m5/0048-cardputer-js-web/web/package.json`
  - Generated: `esp32-s3-m5/0048-cardputer-js-web/web/package-lock.json`
  - Added ignore rules: `esp32-s3-m5/0048-cardputer-js-web/web/.gitignore`
- Implemented CodeMirror editor component:
  - Added: `esp32-s3-m5/0048-cardputer-js-web/web/src/ui/code_editor.tsx`
  - Key bindings: `Mod-Enter` → `run()`, `Tab` → indent (`indentWithTab`)
  - Language mode: `@codemirror/lang-javascript` (`javascript()`)
- Updated app UI to use `CodeEditor` and avoid keystroke-driven rerenders:
  - Updated: `esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.tsx`
- Built deterministic embedded assets into firmware tree:
  - `cd esp32-s3-m5/0048-cardputer-js-web/web`
  - `npm install --no-fund --no-audit`
  - `npm run build`
  - Produced:
    - `esp32-s3-m5/0048-cardputer-js-web/main/assets/index.html`
    - `esp32-s3-m5/0048-cardputer-js-web/main/assets/assets/app.js` (~406 KiB; gzip ~141 KiB)
    - `esp32-s3-m5/0048-cardputer-js-web/main/assets/assets/app.css`

### Why
- A real editor is a core part of the Phase 1 product requirement (Web IDE), and CodeMirror 6 is the best match for embedded UIs (modular, controllable feature set).
- Rebuilding `main/assets/` ensures firmware-embedded symbols point at the actual UI, not placeholders.

### What worked
- Vite deterministic output stayed stable: `/assets/app.js` and `/assets/app.css` remain fixed paths suitable for firmware routes.
- Editor lifecycle is stable: `EditorView` is created once and not recreated on each keystroke.

### What didn't work
- N/A

### What I learned
- CodeMirror 6 adds noticeable bundle weight, but gzip size remains reasonable for an embedded web UI MVP.

### What was tricky to build
- Avoiding feedback loops between “programmatic doc set” and “updateListener”: the editor uses a suppression flag when it performs a sync write.

### What warrants a second pair of eyes
- Bundle size: check whether additional CodeMirror extensions should be deferred to keep `/assets/app.js` small.
- Editor/store coupling: confirm the approach (initialize once from store, then stream changes outward) matches expected future features (loading snippets, reset, etc.).

### What should be done in the future
- Add a “load snippet” mechanism that intentionally overwrites the editor content (and then make the editor truly controlled, if needed).
- Add basic “console-like output” semantics (separate stdout vs return value vs exception) if we decide to support richer results.

### Code review instructions
- Start at:
  - `esp32-s3-m5/0048-cardputer-js-web/web/src/ui/code_editor.tsx` (`CodeEditor`)
  - `esp32-s3-m5/0048-cardputer-js-web/web/src/ui/app.tsx` (`App`)
  - `esp32-s3-m5/0048-cardputer-js-web/web/vite.config.ts` (deterministic output rules)
- Validate the asset build:
  - `cd esp32-s3-m5/0048-cardputer-js-web/web && npm ci && npm run build`

### Technical details
- Editor extensions used:
  - `oneDark`, `javascript()`, `EditorView.lineWrapping`, `keymap.of([...])`

## Step 14: Source ESP-IDF 5.4.1, fix build blockers, and get the firmware building end-to-end

Once ESP-IDF 5.4.1 was available (via `source /home/manuel/esp/esp-idf-5.4.1/export.sh`), I ran the first real build of the `0048-cardputer-js-web` firmware and iteratively fixed the blockers that showed up: missing/renamed components, transitive assumptions baked into the imported MicroQuickJS stdlib wiring, and a partition-table mismatch that made the final binary fail size checks.

The result is that `idf.py -B build_esp32s3_v2 build` completes successfully for `esp32s3` and produces a flashable binary, with a project-local partition table that leaves comfortable headroom for the current CodeMirror-heavy web bundle.

**Commit (code):** 1f181e1 — "0048: make firmware buildable on ESP-IDF 5.4.1"

### What I did
- Activated ESP-IDF in this shell:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py --version`
- Used an explicit build directory (to avoid interacting with unrelated `build/` directories):
  - `idf.py -B build_esp32s3_v2 set-target esp32s3`
- Fixed build blockers revealed by the initial CMake configure/build:
  - Replaced component name `esp_spiffs` → `spiffs` in `main/CMakeLists.txt` (ESP-IDF 5.4 component name).
  - Reduced `EXTRA_COMPONENT_DIRS` to avoid importing the entire repo `components/` set (some components require `M5GFX` and other dependencies not relevant to this project).
  - Added only the single required repo component `components/exercizer_control` because the imported MQJS stdlib runtime includes `exercizer/ControlPlaneC.h`.
  - Updated HTTP status macro:
    - `HTTPD_413_PAYLOAD_TOO_LARGE` → `HTTPD_413_CONTENT_TOO_LARGE` (ESP-IDF 5.4 name).
  - Fixed JSON escaping bug (incorrect multi-character char constant) in `js_runner.cpp`.
- Fixed the “app partition too small” failure by ensuring the build uses our project-local partition table and Cardputer flash defaults:
  - Updated `sdkconfig.defaults` to set:
    - flash size 8MB
    - custom partition table `partitions.csv`
- Rebuilt successfully:
  - `idf.py -B build_esp32s3_v2 build`
  - Confirmed partition table used `factory` 4M and size check passed with substantial free space.

### Why
- We can’t meaningfully iterate on Phase 1 behavior (HTTPD + MQJS eval) until the firmware builds cleanly in a reproducible ESP-IDF environment.

### What worked
- After narrowing `EXTRA_COMPONENT_DIRS` and adding only the missing `exercizer_control` component, the imported stdlib wiring compiled cleanly.
- Using a project-local partition table (4M app) allowed the binary to pass size checks.

### What didn't work
- Initial build blockers (recorded verbatim from the first attempt):
  - Unknown component:
    - `Failed to resolve component 'esp_spiffs' required by component 'main': unknown name.`
  - Missing component due to importing unrelated components:
    - `Failed to resolve component 'M5GFX' required by component 'echo_gif': unknown name.`
  - Missing header pulled by imported MQJS stdlib runtime:
    - `fatal error: exercizer/ControlPlaneC.h: No such file or directory`
  - API constant mismatch:
    - `error: 'HTTPD_413_PAYLOAD_TOO_LARGE' was not declared in this scope; did you mean 'HTTPD_413_CONTENT_TOO_LARGE'?`
  - Size check failure before enabling custom partitions:
    - `Error: app partition is too small for binary cardputer_js_web_0048.bin ... (overflow 0x485f0)`

### What I learned
- In ESP-IDF, dumping a large shared `components/` directory into `EXTRA_COMPONENT_DIRS` is brittle: every component must have its dependencies resolvable, even if the application doesn’t use it.
- Reusing the imports MQJS stdlib wiring “as-is” implies you must also satisfy its assumed in-repo bindings (here: the exercizer control plane header).

### What was tricky to build
- Distinguishing “component is present but not required” vs “component still needs to resolve”: ESP-IDF treats the component set holistically during the configure step, so missing deps anywhere can block the project.
- Partition tables: `partitions.csv` existing in the repo is not enough — you must also set the sdkconfig knobs so IDF uses it.

### What warrants a second pair of eyes
- Whether `exercizer_control` should remain a dependency of this tutorial, or whether we should instead generate a web-IDE-specific minimal stdlib that does not import the exercizer surface at all.
- The current JSON formatting contract (manual escaping) vs adopting a small JSON builder to avoid future edge-case bugs.

### What should be done in the future
- Consider creating a minimal MQJS stdlib for this project (reduce surface area, reduce dependencies, potentially reduce code size).
- Run the Phase 1 smoke-test playbook end-to-end on hardware (`idf.py flash monitor`) and capture actual runtime behavior (Wi‑Fi + HTTP + eval).

### Code review instructions
- Start at:
  - `esp32-s3-m5/0048-cardputer-js-web/CMakeLists.txt`
  - `esp32-s3-m5/0048-cardputer-js-web/sdkconfig.defaults`
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_runner.cpp`
- Validate:
  - `cd esp32-s3-m5/0048-cardputer-js-web && source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -B build_esp32s3_v2 set-target esp32s3`
  - `idf.py -B build_esp32s3_v2 build`

### Technical details
- Built binary size observed during size check:
  - `cardputer_js_web_0048.bin binary size 0x1485f0 bytes. Smallest app partition is 0x400000 bytes. 0x2b7a10 bytes (68%) free.`

## Step 15: Switch from SoftAP-only to esp_console-configured STA Wi‑Fi (0046 pattern) and update `/api/status` + Phase 1 playbook

SoftAP-only was a convenient MVP, but it’s not the desired UX for this project. I switched `0048-cardputer-js-web` to the same console-driven STA Wi‑Fi flow used by `0046-xiao-esp32c6-led-patterns-webui`: bring up the STA stack early, start an `esp_console` REPL, let the developer `wifi scan` + `wifi join ... --save`, and only start the HTTP server once STA has an IP.

This makes the device behave like a “normal” LAN host: once it’s joined your Wi‑Fi, you browse to the printed IP, the Web IDE loads, and REST eval works. `/api/status` now reports STA connection state (and is useful in the browser without needing serial access).

**Commit (code):** 0ed4cda — "0048: add esp_console Wi-Fi STA manager (0046 pattern)"

### What I did
- Studied the reference implementation:
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c`
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_console.c`
- Replaced the SoftAP-only module with a small STA manager + console:
  - Deleted: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_app.cpp`
  - Deleted: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_app.h`
  - Added: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c`
  - Added: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.h`
  - Added: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.c`
  - Added: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.h`
- Updated the firmware entrypoint to:
  - start Wi‑Fi STA (`wifi_mgr_start()`)
  - start `esp_console` (`wifi_console_start()`)
  - start the HTTP server on STA got-IP (`wifi_mgr_set_on_got_ip_cb(...)`)
  - Updated: `esp32-s3-m5/0048-cardputer-js-web/main/app_main.cpp`
- Updated `/api/status` to report STA state, SSID, IP, and last disconnect reason:
  - Updated: `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
- Updated Kconfig to remove SoftAP knobs and add STA console knobs:
  - Updated: `esp32-s3-m5/0048-cardputer-js-web/main/Kconfig.projbuild`
    - `CONFIG_TUTORIAL_0048_WIFI_MAX_RETRY`
    - `CONFIG_TUTORIAL_0048_WIFI_AUTOCONNECT_ON_BOOT`
- Updated component deps to include REPL backend + console:
  - Updated: `esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt` (`console`, `esp_driver_usb_serial_jtag`)
- Verified the project still builds:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -B build_esp32s3_v2 build`

### Why
- This ticket’s desired workflow is “connect device to the same Wi‑Fi as your laptop” and then browse to it — the SoftAP-only MVP was an implementation convenience, not the target UX.
- Reusing the known-good 0046 Wi‑Fi console pattern reduces new surface area and gives a familiar command set (`wifi status|scan|join|set|connect|disconnect|clear`).

### What worked
- Build succeeded after swapping in the STA manager and adding the right ESP-IDF console dependencies.
- Starting the HTTP server only after got-IP matches the mental model of “this device is now a LAN host”.

### What didn't work
- N/A (the pattern port was straightforward once the 0046 code was copied/adapted)

### What I learned
- Treating Wi‑Fi as a “control-plane” problem (configured by `esp_console`) pairs well with a device-hosted Web UI: the console does enrollment, the UI does runtime interaction.

### What was tricky to build
- Managing dependency scope: the STA manager pulls in NVS, event loop, and Wi‑Fi; the console pulls in the REPL backend; `main/CMakeLists.txt` must name those explicitly.

### What warrants a second pair of eyes
- `/api/status` contract changes (SoftAP fields removed, STA fields added). Confirm this is acceptable vs versioning the schema.
- Whether we want an AP fallback mode (e.g., if STA never connects) for demo resilience.

### What should be done in the future
- Update the Phase 1 smoke-test playbook to be STA/console-first (done in this step), and then run it on real hardware to capture runtime logs.

### Code review instructions
- Start at:
  - `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c`
  - `esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.c`
  - `esp32-s3-m5/0048-cardputer-js-web/main/app_main.cpp`
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
- Validate locally:
  - `cd esp32-s3-m5/0048-cardputer-js-web`
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -B build_esp32s3_v2 build`
  - `idf.py -B build_esp32s3_v2 flash monitor`
  - At the `0048>` prompt:
    - `wifi scan`
    - `wifi join 0 <password> --save`
    - `wifi status`
  - Then browse to the printed URL and confirm `/api/status` and `/api/js/eval`.

### Technical details
- `/api/status` now reports:
  - `mode=sta`, `state`, `ssid`, `ip`, `saved`, `runtime`, `reason`

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

**Commit (docs):** c35795d — "Docs: backfill diary + expand 0048 tasks"

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

## Step 11: Start implementation (create 0048 firmware + web skeleton)

With the designs and playbooks in place, I started the actual implementation by creating a new ESP-IDF project directory and wiring the minimal end-to-end path for Phase 1:

- SoftAP Wi‑Fi bring-up (so you can connect directly with a phone/laptop)
- `esp_http_server` serving embedded assets (`/`, `/assets/app.js`, `/assets/app.css`)
- `POST /api/js/eval` that runs MicroQuickJS and returns JSON
- a minimal Preact+Zustand UI (currently a `<textarea>` editor + output panel) built by Vite into `main/assets/`

This is intentionally “minimum viable wiring”: it gives us something to build/flash and then iterate on (replace textarea with CodeMirror, refine output capture, add WS encoder telemetry).

**Commit (code):** 9340ed4 — "0048: add cardputer web JS IDE skeleton"

### What I did
- Created firmware project skeleton:
  - `esp32-s3-m5/0048-cardputer-js-web/`
- Committed the initial skeleton as a focused code commit:
  - `git add 0048-cardputer-js-web`
  - `git commit -m "0048: add cardputer web JS IDE skeleton"`
- Implemented:
  - Wi‑Fi SoftAP bring-up: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_app.cpp`
  - HTTP server + routes: `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
  - MicroQuickJS runner: `esp32-s3-m5/0048-cardputer-js-web/main/js_runner.cpp`
- Added deterministic Vite build output to embedded assets:
  - `esp32-s3-m5/0048-cardputer-js-web/web/vite.config.ts`

### Why
- A working vertical slice is the fastest way to shake out build-system and runtime assumptions (wifi, HTTPD, embedding, MQJS linkage).

### What worked
- Reusing known-good patterns from `0017` (HTTP server + SoftAP) and the MQJS REPL stdlib wiring reduced guesswork.

### What didn't work
- N/A (implementation is new; validation comes next)

### What I learned
- N/A (pending first build/flash validation)

### What was tricky to build
- Stdlib wiring: the MVP approach uses the imports tree’s `esp32_stdlib_runtime.c` and generated `esp32_stdlib.h` via `#include` to avoid duplicating the giant generated header.

### What warrants a second pair of eyes
- The `#include`-based reuse of `esp32_stdlib_runtime.c`/`esp32_stdlib.h` inside our project (it’s pragmatic, but worth reviewing for build hygiene).

### What should be done in the future
- Replace the `<textarea>` editor with CodeMirror 6 (per the design doc), once end-to-end firmware build is verified.

### Code review instructions
- Start at:
  - `esp32-s3-m5/0048-cardputer-js-web/main/app_main.cpp`
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_runner.cpp`

### Technical details
- N/A

## Step 16: Verify ESP-IDF v5.4.1 build in this environment (source `export.sh`)

The diary previously recorded an “`idf.py` missing” state (Step 12). On this machine, ESP-IDF v5.4.1 is installed under `/home/manuel/esp/esp-idf-5.4.1`, so I validated the actual end-to-end build by sourcing `export.sh` and running a clean out-of-tree build.

This step is intentionally boring-but-important: it closes the loop that the current `0048-cardputer-js-web` firmware compiles successfully under the exact ESP-IDF version we’ve standardized on, and it gives us a stable baseline before Phase 2 (WebSocket + encoder).

**Commit (code):** N/A

### What I did
- Verified ESP-IDF is available after sourcing:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py --version` → `ESP-IDF v5.4.1`
- Built the project (target esp32s3, isolated build dir):
  - `cd esp32-s3-m5/0048-cardputer-js-web`
  - `idf.py -B build_esp32s3_v2 set-target esp32s3`
  - `idf.py -B build_esp32s3_v2 build`

### Why
- Phase 2 work (WS + encoder) should not start until we have a known-good compile baseline; otherwise we’ll conflate “new bug” with “existing build issue”.

### What worked
- `idf.py -B build_esp32s3_v2 build` completed successfully.
- Size check passed with ample headroom (factory app partition is 4 MiB).

### What didn't work
- N/A

### What I learned
- The earlier “`idf.py` missing” constraint was environment-specific; in this environment, sourcing `/home/manuel/esp/esp-idf-5.4.1/export.sh` is sufficient to build.

### What was tricky to build
- N/A (validation-only step)

### What warrants a second pair of eyes
- Confirm the chosen build directory convention (`-B build_esp32s3_v2`) matches how you want to manage multiple local configs/targets.

### What should be done in the future
- Run the Phase 1 playbook on real hardware and capture the first successful `flash monitor` session logs in the diary.

### Code review instructions
- Build validation:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `cd esp32-s3-m5/0048-cardputer-js-web`
  - `idf.py -B build_esp32s3_v2 set-target esp32s3`
  - `idf.py -B build_esp32s3_v2 build`

### Technical details
- Build output included:
  - `Generated .../cardputer_js_web_0048.bin`
  - `cardputer_js_web_0048.bin binary size 0x158410 bytes` with `0x2a7bf0 bytes (66%) free`.
