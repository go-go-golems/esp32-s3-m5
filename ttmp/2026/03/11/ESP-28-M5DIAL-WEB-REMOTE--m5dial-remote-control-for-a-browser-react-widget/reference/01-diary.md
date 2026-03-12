---
Title: Diary
Ticket: ESP-28-M5DIAL-WEB-REMOTE
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - ui
    - websocket
    - webserver
    - http
    - wifi
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0045-xiao-esp32c6-preact-web/main/http_server.c
      Note: Core embedded asset precedent referenced in the diary
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: Core WebSocket precedent referenced in the diary
    - Path: 0072-m5dial-timer-demo/main/app_main.cpp
      Note: Core M5Dial evidence referenced in the diary
    - Path: ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/design-doc/01-implementation-plan-and-intern-guide.md
      Note: Primary deliverable authored during this diary
    - Path: ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/sources/local/01-esp32-knob-web.md
      Note: Imported note studied before planning
ExternalSources:
    - local:esp32-knob-web.md
Summary: Chronological research and documentation diary for the M5Dial browser-widget remote-control ticket, including ticket setup, evidence gathering, design decisions, validation, and reMarkable delivery.
LastUpdated: 2026-03-11T22:45:00-04:00
WhatFor: Continuation-friendly record of how the ticket was created, what evidence was gathered, what decisions were made, and how the deliverables were validated and delivered.
WhenToUse: Use when reviewing the research trail behind this ticket or continuing implementation from the plan.
---


# Diary

## Goal

This diary records the full research and documentation workflow for the M5Dial browser-widget remote-control ticket. It is meant to let another engineer continue the work without guessing what was inspected, why certain architectural choices were made, or which repo examples informed the plan.

## Step 1: Create The Ticket And Import The Source Note

I started by creating a fresh `docmgr` ticket workspace because the user explicitly asked for a new ticketed deliverable rather than ad hoc notes. I also imported `/tmp/esp32-knob-web.md` immediately, because the prompt said to import it first and read it carefully before planning anything else.

This step established the working area and confirmed that the imported source note should be treated as input evidence, not as the final architecture. That distinction mattered later because the imported note describes an external Go server, while the user asked for a self-contained firmware folder.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to build a remote control with the m5 dial that controls a react widget on a webbrowser . The thing should be self contained within its firmware folder. 

First, import with docmgr import file --file /tmp/esp32-knob-web.md --ticket ... to import the file, read it care fully, then look at other m5dial firmware in here, and create a detailed implementation plan in the ticket. Keep a diary as you go.

Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new `docmgr` ticket, import the source note into it first, study the local M5Dial firmware and related web UI precedents, then write a thorough intern-facing design and implementation guide plus a diary, validate the ticket, and upload the bundle to reMarkable.

**Inferred user intent:** Produce a documentation-first implementation package that is actionable enough for someone new to the codebase to build a self-contained M5Dial firmware project that controls a browser widget.

**Commit (code):** N/A — documentation-only ticket work in this turn; no firmware code was changed.

### What I did

- Ran `docmgr status --summary-only` to confirm the workspace and vocabulary roots.
- Created ticket `ESP-28-M5DIAL-WEB-REMOTE`.
- Added a primary design doc and a diary doc.
- Imported `/tmp/esp32-knob-web.md` with `docmgr import file --file /tmp/esp32-knob-web.md --ticket ESP-28-M5DIAL-WEB-REMOTE`.
- Confirmed the imported source landed under `sources/local/01-esp32-knob-web.md`.

### Why

- The ticket workspace was required to store the deliverables in a structured way.
- Importing the source file first ensured the later design work remained anchored to the exact note the user pointed at.

### What worked

- `docmgr` was already initialized for this repo.
- Ticket creation, doc creation, and file import all succeeded cleanly.
- The ticket index automatically registered the imported file as `local:esp32-knob-web.md`.

### What didn't work

- Nothing failed in the ticket bootstrap step.

### What I learned

- The repo already contains recent M5Dial ticket work (`ESP-26-M5DIAL-TIMER-DEMO` and `ESP-27-M5DIAL-FILM-DEVELOPER`), which suggested that the new deliverable should be equally detailed and evidence-driven.

### What was tricky to build

- The tricky part was not the CLI mechanics; it was keeping the imported note in the right role. The note is useful as design input, but its deployment boundary conflicts with the self-contained firmware requirement, so I had to treat it as a conceptual reference rather than a blueprint.

### What warrants a second pair of eyes

- The eventual interpretation of “self-contained” should be reviewed carefully during implementation, especially whether vendored local copies of helper sources are acceptable or whether every dependency must be managed purely through ESP-IDF mechanisms.

### What should be done in the future

- When implementation starts, create the new firmware folder before writing any feature code so the self-contained boundary is enforced from day one.

### Code review instructions

- Start with the ticket root and confirm the imported source exists under `sources/local/`.
- Verify the design doc references the imported note but does not adopt its external-server deployment unchanged.

### Technical details

- Commands run:
  - `docmgr status --summary-only`
  - `docmgr ticket create-ticket --ticket ESP-28-M5DIAL-WEB-REMOTE --title "M5Dial remote control for a browser React widget" --topics esp32-s3,esp32s3,firmware,m5stack,ui,websocket,webserver,http,wifi`
  - `docmgr doc add --ticket ESP-28-M5DIAL-WEB-REMOTE --doc-type design-doc --title "Implementation plan and intern guide"`
  - `docmgr doc add --ticket ESP-28-M5DIAL-WEB-REMOTE --doc-type reference --title "Diary"`
  - `docmgr import file --file /tmp/esp32-knob-web.md --ticket ESP-28-M5DIAL-WEB-REMOTE`

## Step 2: Gather Evidence From Existing M5Dial And Web UI Projects

After the ticket existed, I switched into repository study. The main goal was to avoid writing a speculative design. I needed to know what parts of the target system already exist in the repo and which pieces are missing.

This step is where the final architecture direction became clear. The repo already contains a strong M5Dial input/task model, a strong device-hosted asset embedding pattern, and a strong firmware-to-browser WebSocket precedent. What it does not contain is a self-contained project that combines those three things.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Inspect the imported note carefully, study existing M5Dial firmware in the repo, and derive a detailed implementation plan from concrete local examples.

**Inferred user intent:** Make the plan feel native to this repository rather than generic embedded advice.

**Commit (code):** N/A — documentation-only ticket work in this turn; no firmware code was changed.

### What I did

- Read the imported note with line numbers.
- Inspected `0072-m5dial-timer-demo` and `0073-m5dial-film-developer-timer`.
- Inspected device-hosted web UI precedents in `0045-xiao-esp32c6-preact-web`, `0048-cardputer-js-web`, and `0071-cardputer-adv-photo-timer`.
- Inspected supporting files such as `sdkconfig.defaults`, `CMakeLists.txt`, Vite configs, WebSocket handlers, and browser stores.
- Checked whether the repo already had a React-in-firmware example. It does not; the browser examples are Preact-based.

### Why

- I needed evidence for the hardware, task model, and network/UI patterns.
- I also needed to identify which older examples break the self-contained requirement so the new design could explicitly avoid those patterns.

### What worked

- The repo study quickly showed a coherent path:
  - `0072` and `0073` provide the M5Dial hardware and task structure.
  - `0045` provides the embedded asset/Vite pattern.
  - `0048` provides the WebSocket device-to-browser pattern.
  - `0071` provides REST-style status and control endpoints plus a device-hosted page.

### What didn't work

- One exploratory ripgrep command had a shell quoting bug:

```text
zsh:1: unmatched '
```

- The failing command was the first attempt to search broadly for React usage with nested quotes. I reran the search with simpler quoting and continued.

### What I learned

- The imported source note’s event-normalization idea is good, but its Watermill plus Go-server deployment is not compatible with the ticket’s self-contained requirement.
- The current M5Dial examples are not truly self-contained because their `main/CMakeLists.txt` files compile sources via absolute paths into other checkouts.
- The repo’s browser-side examples rely on a single browser WebSocket and a small state store, which is a very good fit for this feature.

### What was tricky to build

- The tricky part was architectural triangulation. It would have been easy to overfit to the imported note or to overfit to one local project. Instead, I had to merge lessons from multiple sources:
  - M5Dial hardware from `0072` and `0073`
  - embedded asset serving from `0045`
  - live WebSocket telemetry from `0048`
  - compact JSON API patterns from `0071`

- Another tricky point was framework choice. The repo does not contain an existing React firmware app, only Preact ones. I chose to preserve the Vite embedding pattern but switch the frontend framework to React 18 because the user explicitly asked for a React widget.

### What warrants a second pair of eyes

- The exact interpretation of browser state ownership versus firmware state ownership should be reviewed during implementation. I recommended firmware-owned transport and browser-owned widget semantics, but a reviewer should confirm that matches the intended demo behavior.
- The final dependency-vendoring approach deserves review because M5Dial support currently depends on files outside the project directory.

### What should be done in the future

- When the real implementation starts, create a dependency checklist that rejects absolute external source paths during review.
- Decide early whether Wi-Fi provisioning should be SoftAP-first or STA-first, because that affects the README, on-device screen, and initial demo flow.

### Code review instructions

- Read the evidence-backed sections of the design doc and compare each architecture recommendation with its source precedent.
- Verify that the design cites the correct local files for:
  - M5Dial tasks and board abstraction
  - browser asset embedding
  - WebSocket broadcasting
  - browser-side socket/store logic

### Technical details

- Key commands run:
  - `sed -n '1,240p' /tmp/esp32-knob-web.md`
  - `nl -ba ttmp/.../sources/local/01-esp32-knob-web.md | sed -n '1,260p'`
  - `nl -ba 0072-m5dial-timer-demo/main/app_main.cpp | sed -n '1,260p'`
  - `nl -ba 0072-m5dial-timer-demo/main/m5dial_board.cpp | sed -n '1,420p'`
  - `nl -ba 0073-m5dial-film-developer-timer/main/app_main.cpp | sed -n '1,320p'`
  - `nl -ba 0045-xiao-esp32c6-preact-web/main/http_server.c | sed -n '1,320p'`
  - `nl -ba 0045-xiao-esp32c6-preact-web/web/vite.config.ts | sed -n '1,220p'`
  - `nl -ba 0048-cardputer-js-web/main/http_server.cpp | sed -n '1,280p'`
  - `nl -ba 0048-cardputer-js-web/main/encoder_telemetry.cpp | sed -n '1,220p'`
  - `nl -ba 0048-cardputer-js-web/web/src/ui/store.ts | sed -n '1,260p'`
  - `nl -ba 0071-cardputer-adv-photo-timer/main/http_server.cpp | sed -n '1,320p'`

## Step 3: Write The Intern Guide, Update Ticket Bookkeeping, And Deliver

Once the evidence set was complete, I replaced the stub design doc with a full analysis/design/implementation guide. I also wrote this diary in the required chronological format, then updated the ticket bookkeeping so the ticket would be usable later instead of just containing one large markdown file.

This step converted raw exploration into a repeatable handoff. The design doc is intentionally opinionated where the repo evidence is strong, especially on task ownership, event normalization, embedded-asset layout, and the need to keep everything under one firmware folder.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Produce a detailed onboarding-grade design package, keep the ticket organized, validate it, and upload it to reMarkable.

**Inferred user intent:** Leave behind a ticket that a new intern can actually use to build the project, not just a one-off answer in chat.

**Commit (code):** N/A — documentation-only ticket work in this turn; no firmware code was changed.

### What I did

- Replaced the template design doc with a detailed implementation guide.
- Added:
  - executive summary
  - current-state architecture analysis
  - gap analysis
  - proposed system design
  - protocol sketches
  - pseudocode
  - phased implementation plan
  - testing strategy
  - risks, alternatives, and open questions
  - line-anchored references
- Wrote this diary with the required section structure.
- Planned ticket bookkeeping updates, validation, and bundle upload.

### Why

- The prompt asked for a document suitable for a new intern, which means the deliverable needed both explanation and execution order.
- The design had to be detailed enough to reduce ambiguity around the boundary between firmware, transport, and browser widget logic.

### What worked

- The evidence already gathered mapped cleanly into the final document sections.
- The repo precedents were strong enough that the design could be explicit and file-level, rather than generic.
- The imported note still added value as conceptual input after being reframed away from the external-server deployment.

### What didn't work

- No writing-stage failures occurred beyond the earlier shell quoting mistake already captured in Step 2.

### What I learned

- The most valuable part of the imported note was not Watermill. It was the idea that the browser should see one coherent control stream rather than hardware noise. That idea transferred well into a self-contained firmware-hosted WebSocket design.
- The most valuable local insight was that the new project should be treated as an integration of existing patterns, not as a blank-sheet invention.

### What was tricky to build

- The hardest part of the writing step was keeping the guide detailed without becoming vague or bloated. The solution was to anchor each recommendation to a local precedent and then explain exactly how the new project should differ from that precedent.
- Another tricky part was describing self-containedness precisely enough to be enforceable in code review. I solved that by defining allowed and disallowed dependency patterns directly in the design doc.

### What warrants a second pair of eyes

- Review the final project-folder proposal and make sure it matches how implementation will actually be staged.
- Review the proposed browser-to-firmware messages and confirm they are minimal enough for v1.
- Review whether the on-device status screen should mirror widget state in v1 or only show transport/session state.

### What should be done in the future

- Implement the new firmware folder in phases exactly as laid out in the design doc.
- Keep the first browser widget intentionally small so transport and control semantics stabilize before adding more UI.
- Add smoke-test playbooks once implementation begins.

### Code review instructions

- Read the design doc top to bottom once, then re-check the references section against the cited repo files.
- Verify the plan explicitly solves the self-contained-folder constraint.
- Confirm the proposed file tree, route table, and message contracts are internally consistent.

### Technical details

- Deliverables created:
  - design doc
  - diary
  - updated ticket metadata/bookkeeping
- Planned validation commands:
  - `docmgr doctor --ticket ESP-28-M5DIAL-WEB-REMOTE --stale-after 30`
  - `remarquee status`
  - `remarquee cloud account --non-interactive`
  - `remarquee upload bundle --dry-run ...`
  - `remarquee upload bundle ...`
  - `remarquee cloud ls /ai/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE --long --non-interactive`

## Step 4: Rewrite The Architecture Around A Server-Hosted React App

After the initial ticket delivery, the user clarified that the deployment model was wrong. The M5Dial should connect to a web server that serves the React app, rather than serving the React app itself. That meant the design document needed a real architectural rewrite, not a cosmetic wording tweak.

This clarification actually made the imported source note more relevant, because its external-server event-pipeline shape now matches the requested deployment boundary more directly. The rewrite therefore pivoted away from device-hosted web UI patterns and toward a dial-as-client model.

### Prompt Context

**User prompt (verbatim):** "so i want the device to connect to a web server which serves the react, not the dial to host the react. Rewrite"

**Assistant interpretation:** Replace the ticket’s current device-hosted React architecture with a design where the dial is a Wi-Fi client to an external web server that serves the React app and brokers browser sessions.

**Inferred user intent:** Correct the deployment boundary so implementation effort goes into the right system shape before any firmware work starts.

**Commit (code):** N/A — documentation-only ticket work in this turn; no firmware code was changed.

### What I did

- Rewrote the primary design doc so the dial is now a network client to an external server.
- Removed the assumption that the firmware folder contains `web/` and `main/assets/`.
- Reframed the protocol around:
  - device -> server
  - server -> browser
  - optional server -> device feedback
- Updated the ticket index summary and task list to match the new direction.
- Updated the changelog to record the design pivot.

### Why

- The old draft would have pushed implementation in the wrong direction.
- The clarified deployment model affects the protocol, folder layout, task plan, and testing strategy, so the design had to change holistically.

### What worked

- The existing M5Dial evidence from `0072` and `0073` stayed valid.
- The imported source note became a much closer architectural match after the clarification.
- The browser-side WebSocket/store pattern from `0048` remained useful even though the dial is no longer the host.

### What didn't work

- The first-draft design was no longer correct once the user clarified the boundary between device and server.

### What I learned

- The most important design variable in this ticket is not the UI framework; it is the deployment boundary.
- Once the boundary changes, many “good” precedents become secondary. Device-hosted asset examples are much less central when the dial is only a client.

### What was tricky to build

- The tricky part was rewriting enough of the document to make it internally consistent without losing the useful evidence gathered earlier. I kept the embedded hardware/task guidance and swapped out the device-hosted web assumptions.

### What warrants a second pair of eyes

- Review the new interpretation of “self-contained within its firmware folder” and confirm it now means a self-contained firmware client, not a self-hosted browser app.
- Review the proposed `/device/ws` plus browser `/ws` split and confirm it fits the planned server implementation.

### What should be done in the future

- If the server implementation will live in this repo too, create a companion ticket for it rather than overloading the firmware ticket with server code.
- Add a small mock server or protocol harness once firmware work begins so the dial can be tested independently of the final React UI.

### Code review instructions

- Start with the design doc executive summary and confirm it now states that the M5Dial connects to an external web server.
- Check the folder layout section and verify that `web/` and embedded asset assumptions were removed from the firmware plan.
- Check the tasks list and verify that the next implementation steps are now about Wi-Fi client + outbound protocol, not serving React from the device.

### Technical details

- Files updated:
  - `design-doc/01-implementation-plan-and-intern-guide.md`
  - `index.md`
  - `tasks.md`
  - `changelog.md`

## Step 5: Turn The Ticket Into An Actual 0074 Firmware Project

The next user turn switched from planning into implementation. I first rewrote the ticket task list so the execution order would be visible in the docs, then I created a new `0074-m5dial-web-remote/` project with separate `firmware/`, `server/`, and `web/` subdirectories.

The firmware step was intentionally biased toward self-containedness. I copied the M5Dial hardware layer and localized every dependency that would otherwise have leaked in from elsewhere in the repo: `LovyanGFX`, `wifi_mgr`, `wifi_console`, `esp_websocket_client`, and the encoder/button sources. Then I implemented the new runtime pieces on top of that base.

### Prompt Context

**User prompt (verbatim):** "Ok, now write the tasks to implement : 

- the esp32 firmware (using esp_console to configure wifi and outgoing connections)
- the go server
- a react app that leverages and displays the knob stuff (both a real scroll list, and also debug messages / connection status, etc...) 

Then, go through the tasks and implement, one by one, committing as you go. The device is connected through ACM0"

**Assistant interpretation:** Convert the research ticket into code, create the implementation task breakdown, then implement firmware, server, and browser layers in sequence with focused commits.

**Inferred user intent:** Move from design into a runnable end-to-end prototype while preserving traceability through commits and ticket updates.

**Commit (code):** `d2d3690` — "feat: add M5Dial firmware websocket client"

### What I did

- Updated `tasks.md` with the concrete implementation checklist before coding.
- Created `0074-m5dial-web-remote/firmware`.
- Vendored:
  - `LovyanGFX`
  - `wifi_mgr`
  - `wifi_console`
  - `esp_websocket_client`
  - `ESP32Encoder` and `Button`
- Added:
  - `remote_config.*` for NVS-backed URL/device-id storage
  - `remote_client.*` for the outbound WebSocket client
  - `remote_console.*` for `remote status|set-url|set-id|connect|disconnect|clear`
  - a direct-rendered local status screen in `app_main.cpp`
- Reused the M5Dial board/input code from `0072` and adapted it into the new project.
- Built the firmware with:
  - `. $HOME/esp/esp-idf-5.4.1/export.sh`
  - `idf.py set-target esp32s3`
  - `idf.py build`

### Why

- The firmware was the most constrained layer because it had to satisfy the self-contained-folder requirement.
- Implementing the console, config persistence, and outbound transport first made the remaining server/browser work concrete instead of speculative.

### What worked

- `idf.py build` completed successfully for the new project.
- The runtime shape is in place:
  - USB Serial/JTAG console
  - Wi-Fi console commands
  - remote transport commands
  - local status display
  - outbound hello/heartbeat/encoder/button messages

### What didn't work

- One early attempt to flash used `/dev/ttyACM0` after the build and failed because the ACM device was gone by the time esptool tried to open it:

```text
A fatal error occurred: Could not open /dev/ttyACM0, the port is busy or doesn't exist.
([Errno 2] could not open port /dev/ttyACM0: [Errno 2] No such file or directory: '/dev/ttyACM0')
```

### What I learned

- The M5Dial implementation can stay relatively small if the local screen is rendered directly with LovyanGFX instead of pulling in LVGL.
- The self-contained requirement is expensive mostly because of vendored graphics support, not because of the new application logic.

### What was tricky to build

- The tricky part was dependency localization. The existing `0072` example compiles against absolute paths outside the project. I had to copy those pieces in, fix include paths, and make the new build independent before the first firmware compile could work.
- Another tricky part was deciding how much logic belongs on the dial. I kept the dial focused on transport/session state and input telemetry, and left list semantics to the browser.

### What warrants a second pair of eyes

- The vendored `LovyanGFX` copy is large; a reviewer should confirm whether pruning examples or replacing it with a slimmer local component is worth doing later.
- The firmware currently uses the deprecated legacy PCNT-based encoder driver indirectly through the vendored encoder code.

### What should be done in the future

- Replace or modernize the vendored encoder implementation so it uses the current pulse counter driver.
- Once hardware is stably visible again on USB, validate the full Wi-Fi provisioning and remote-connect path on-device.

### Code review instructions

- Start at `0074-m5dial-web-remote/firmware/main/app_main.cpp`.
- Then read:
  - `remote_client.cpp`
  - `remote_console.cpp`
  - `remote_config.cpp`
  - `m5dial_board.cpp`
- Validate with:
  - `cd 0074-m5dial-web-remote/firmware`
  - `. $HOME/esp/esp-idf-5.4.1/export.sh`
  - `idf.py build`

### Technical details

- Main files added:
  - `0074-m5dial-web-remote/firmware/main/app_main.cpp`
  - `0074-m5dial-web-remote/firmware/main/remote_client.cpp`
  - `0074-m5dial-web-remote/firmware/main/remote_console.cpp`
  - `0074-m5dial-web-remote/firmware/main/remote_config.cpp`
- Supporting config:
  - `0074-m5dial-web-remote/firmware/sdkconfig.defaults`
  - `0074-m5dial-web-remote/firmware/CMakeLists.txt`

## Step 6: Build The Go Hub And React Dashboard

With the firmware protocol implemented, the server and browser layers became straightforward. I built a small Go WebSocket hub first, then a React/Vite UI that subscribes to the browser WebSocket, renders device state, and uses the current knob position to drive a real scrollable list.

I deliberately made the Go server serve embedded static assets from `server/static/` so the runtime path stays simple: start one Go process, open one browser URL, and point the dial at one WebSocket endpoint.

### Prompt Context

**User prompt (verbatim):** (see Step 5)

**Assistant interpretation:** After the firmware, implement the host server and the browser UI in separate commits, while preserving the remote-control semantics defined in the ticket.

**Inferred user intent:** Produce a realistic end-to-end demo stack, not just an ESP32 sketch.

**Commit (code):**
- `ca3e85d` — "feat: add M5Dial websocket server"
- `a9bc89e` — "feat: add React dashboard for M5Dial remote"
- `fed38ca` — "chore: clean generated web artifacts"

### What I did

- Added `0074-m5dial-web-remote/server` with:
  - `/ws/device`
  - `/ws/browser`
  - `/api/status`
  - embedded static file serving
- Implemented a hub that tracks connected devices, stores recent history, and broadcasts status snapshots to browser clients.
- Added `0074-m5dial-web-remote/web` as a React/Vite app using Zustand.
- Implemented browser features:
  - browser WebSocket connection state
  - selected device summary
  - a real scroll list driven by the device `position`
  - recent server/device history
  - bounded raw WebSocket frame log
- Built the React app and copied `web/dist/` into `server/static/`.
- Added a project README with build/run instructions.

### Why

- The Go server is the rendezvous point between dial and browser, so it needed to exist before the React app could be meaningfully validated.
- Embedding the built React assets into the server keeps deployment simple and matches the corrected server-hosted architecture.

### What worked

- `go test ./...` succeeded in `0074-m5dial-web-remote/server`.
- `npm install` and `npm run build` succeeded in `0074-m5dial-web-remote/web`.
- `curl http://127.0.0.1:18080/` returned the built React shell.
- `curl http://127.0.0.1:18080/api/status` returned a valid empty snapshot.

### What didn't work

- A first attempt to combine asset syncing and server testing into one shell invocation was blocked by the tool policy, so I re-ran those as separate commands.
- The first React build script used `tsc -b`, which emitted `tsbuildinfo` and compiled Vite config artifacts into the source tree. I fixed that in a cleanup commit by switching to `--noEmit` checks and deleting the generated files.

### What I learned

- The server/browser side can stay small if the browser derives widget semantics from a compact `position` field rather than needing a richer RPC protocol.
- Having the server broadcast full snapshots to browsers keeps the React store simple and continuation-friendly.

### What was tricky to build

- The tricky part was not the WebSocket upgrade path; it was choosing a state model that made the React app simple. I solved that by keeping the server authoritative for device presence/history while letting the browser compute the selected list row locally from `position`.
- Another tricky point was keeping the Go runtime and the React build aligned. Using `server/static/` as the copy target avoids an extra packaging tool but requires build-discipline, which I documented in the README.

### What warrants a second pair of eyes

- Review whether broadcasting full `server_status` snapshots on every device message is acceptable for the intended event rate.
- Review whether the server should preserve disconnected devices in the snapshot or prune them after a timeout in a later iteration.

### What should be done in the future

- Add a small browser control path only if there is a concrete need for server -> device commands.
- Add an automated asset-sync script or Make target so `web/dist` -> `server/static` cannot be forgotten.

### Code review instructions

- Start with `0074-m5dial-web-remote/server/main.go` and `hub.go`.
- Then read the React store and view layer:
  - `0074-m5dial-web-remote/web/src/store.ts`
  - `0074-m5dial-web-remote/web/src/app.tsx`
  - `0074-m5dial-web-remote/web/src/styles.css`
- Validate with:
  - `cd 0074-m5dial-web-remote/server && go test ./...`
  - `cd 0074-m5dial-web-remote/web && npm install && npm run build`

### Technical details

- Commands run:
  - `gofmt -w main.go hub.go`
  - `go mod tidy`
  - `go test ./...`
  - `npm install`
  - `npm run build`
  - `curl -s http://127.0.0.1:18080/`
  - `curl -s http://127.0.0.1:18080/api/status`

## Step 7: Check ACM0 And Record The Hardware Blocker

The user explicitly asked to be notified if serial got weird during flashing, and that issue did happen. After the firmware build succeeded, the `idf.py -p /dev/ttyACM0 flash` step failed because the ACM device disappeared before esptool could open it.

I checked for the obvious signs of re-enumeration. There were no `/dev/ttyACM*` nodes, no serial symlinks under `/dev/serial`, and the ESP device no longer appeared in `lsusb`. That means the current blocker is physical/USB presence, not an `idf.py` configuration error inside the new firmware project.

### Prompt Context

**User prompt (verbatim):** "let me know if you see weird serial disappearing and such while flashing. You can use tmux to interact"

**Assistant interpretation:** Watch for ACM reset/re-enumeration failures during flash and report them explicitly if they happen.

**Inferred user intent:** Make hardware-side instability visible immediately so software work does not hide a USB/bootloader problem.

**Commit (code):** N/A — validation step only; no code changes were made in response yet.

### What I did

- Ran `idf.py -p /dev/ttyACM0 flash`.
- Confirmed the flash failed because `/dev/ttyACM0` did not exist at open time.
- Checked for replacement serial nodes and USB visibility.

### Why

- The user called out ACM instability specifically, so this needed to be tested and documented rather than silently skipped.

### What worked

- The flash attempt produced a concrete failure mode instead of an ambiguous timeout.
- The follow-up checks established that the board disappeared from USB entirely.

### What didn't work

- Flashing itself did not complete because the device vanished from USB.
- `dmesg` could not be read in this environment:

```text
dmesg: read kernel buffer failed: Operation not permitted
```

### What I learned

- The current obstacle to hardware validation is device presence on USB, not the new firmware image format or build system.
- If the board comes back on a different ACM path, the project is otherwise ready for the next flash attempt.

### What was tricky to build

- The tricky part here was separating a software/tooling regression from a hardware enumeration failure. `lsusb` showing no ESP-class device made that distinction much clearer.

### What warrants a second pair of eyes

- The next engineer attempting flash should confirm cable/power/boot-mode state first before debugging the firmware.
- If the board intermittently reappears, capture kernel logs outside this sandbox so the exact USB disconnect/reset reason is visible.

### What should be done in the future

- Reconnect or power-cycle the device and retry flashing as soon as `/dev/ttyACM*` or a matching `lsusb` entry returns.
- If ACM remains unstable, try a lower flash baud rate and capture host USB logs.

### Code review instructions

- Re-run:
  - `lsusb`
  - `find /dev/serial/by-id -maxdepth 1 -type l -printf '%f -> %l\n'`
  - `cd 0074-m5dial-web-remote/firmware && . $HOME/esp/esp-idf-5.4.1/export.sh && idf.py -p /dev/ttyACM0 flash`

### Technical details

- Flash error:

```text
A fatal error occurred: Could not open /dev/ttyACM0, the port is busy or doesn't exist.
([Errno 2] could not open port /dev/ttyACM0: [Errno 2] No such file or directory: '/dev/ttyACM0')
```
