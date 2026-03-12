---
Title: Diary
Ticket: ESP-30-M5DIAL-MQJS-LAIN-DSL
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - javascript
    - ui
    - websocket
    - webserver
    - http
    - wifi
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/CMakeLists.txt
      Note: Shows the component dependencies needed for MicroQuickJS integration
    - Path: 0074-m5dial-web-remote/firmware/main/app_main.cpp
      Note: Primary runtime evidence inspected during the research pass
    - Path: 0074-m5dial-web-remote/firmware/main/remote_client.h
      Note: Shows the currently supported browser command set that should become internal DSL primitives
    - Path: 0074-m5dial-web-remote/web/src/app.tsx
      Note: Lain OS browser UI state and interaction surfaces considered during the design
ExternalSources: []
Summary: Chronological diary for the research and design work on adding a MicroQuickJS scripting layer to the 0074 M5Dial Lain OS stack.
LastUpdated: 2026-03-11T21:18:35-04:00
WhatFor: Record what was inspected, what conclusions were drawn, and how to continue implementation safely.
WhenToUse: Use when resuming this ticket or reviewing why the proposed scripting design looks the way it does.
---


# Diary

## Goal

Track the research and design work for adding an on-device MicroQuickJS service and browser-delivered scripting DSL to `0074-m5dial-web-remote`.

## Step 1: Capture the current Lain OS state and design the scripting architecture

I started from the current 0074 codebase rather than from prior assumptions. The user had just explained that the firmware, Go server, and web UI had already been expanded into a Lain-themed radio experience, so the first task was to anchor the future scripting design to that actual code, not to the earlier simpler remote-control version.

The critical outcome of this step was architectural, not cosmetic: the design now treats `0048-cardputer-js-web` as the reference implementation for the QuickJS service lifecycle, but it deliberately keeps 0074's app-task ownership model. That means JavaScript will request Lain OS actions through an internal command bus rather than directly mutating the device state.

### Prompt Context

**User prompt (verbatim):** "We had a colleague pimp up our firmware and web server and web UI to be a lain OS. 

commit the current status. 

Then, create a new ticket where we now run the microquickjs service on the device and use a nice JS API/DSL to control the new Lain OS primitives. Then we can send Javascript overwebsocket back to the device to execute custom code. 

Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Commit the current 0074 Lain OS code state, then create a new documentation ticket describing how to add a MicroQuickJS-based scripting runtime and browser-to-device JavaScript execution path.

**Inferred user intent:** Preserve the current Lain OS snapshot in git, then produce a high-quality implementation package that an unfamiliar engineer can use to build the next phase safely.

**Commit (code):** `8c609fc` — "feat: turn web remote into lain radio os"

### What I did

- Inspected the current worktree with `git status --short`.
- Confirmed the Lain OS work was concentrated in `0074-m5dial-web-remote`.
- Rebuilt the web bundle with:
  - `cd 0074-m5dial-web-remote/web && npm run build`
- Verified the Go server still built with:
  - `cd 0074-m5dial-web-remote/server && go test ./...`
- Committed the current 0074 source snapshot as `8c609fc`.
- Noticed a built Go server binary had slipped into that commit and corrected it immediately with:
  - `8c609fc` followed by
  - `5540dff` — "chore: ignore built server binary"
- Created ticket `ESP-30-M5DIAL-MQJS-LAIN-DSL`.
- Inspected current 0074 runtime files:
  - `firmware/main/app_main.cpp`
  - `firmware/main/remote_client.cpp`
  - `firmware/main/remote_client.h`
  - `server/main.go`
  - `server/hub.go`
  - `web/src/store.ts`
  - `web/src/app.tsx`
- Inspected the QuickJS reference implementation in:
  - `0048-cardputer-js-web/main/js_service.cpp`
  - `0048-cardputer-js-web/main/CMakeLists.txt`
  - `0048-cardputer-js-web/main/http_server.cpp`
- Wrote the primary design doc, tasks, changelog, and this diary.

### Why

- The design needed to be grounded in the actual Lain OS runtime that exists today.
- The current firmware already has a queue-based command ingress, which is the right seed for a future scripting API.
- `0048-cardputer-js-web` already proves that `mquickjs` and `mqjs_service` can run in this repository, so it is the least speculative reference point.

### What worked

- The current 0074 snapshot was committed successfully.
- The accidental binary inclusion was corrected cleanly in a follow-up commit.
- `npm run build` succeeded for the web bundle.
- `go test ./...` succeeded for the Go server.
- The research pass found a strong in-repo QuickJS precedent instead of forcing a fresh design.

### What didn't work

- The first commit accidentally included `0074-m5dial-web-remote/server/m5dial-web-remote-server`, a built binary artifact. That happened because it was already present in the worktree and ended up staged into the commit snapshot.
- The exact corrective commands were:
  - `git rm 0074-m5dial-web-remote/server/m5dial-web-remote-server`
  - update `.gitignore`
  - `git commit -m "chore: ignore built server binary"`

### What I learned

- The current 0074 firmware is already closer to a scriptable architecture than it first appears because it has:
  - a dedicated input queue,
  - a dedicated websocket command queue,
  - a single app task that owns the visible state.
- The most important design choice is not “how to run QuickJS,” but “how to keep QuickJS from bypassing app-task ownership.”
- There is also an operational inconsistency worth calling out in the design doc: `README.md` still documents port `8080`, while `PLAYBOOK.md` and the Vite proxy are built around `18080`.

### What was tricky to build

- The tricky part was separating transport concepts from ownership concepts. It would have been very easy to say “browser sends JS, JS updates radio state,” but that skips the concurrency boundary that currently keeps 0074 understandable.
- Another subtle point was choosing the correct precedent. `0048` does device-hosted HTTP eval, while `0074` is server-hosted on the web side. The right approach is to copy the **service pattern** from `0048`, not the transport topology.

### What warrants a second pair of eyes

- The recommendation to replace `RemoteUiCommand` with a more general app-command bus.
- The suggested websocket contract split between `script_eval`, `script_result`, `script_console`, and `script_event`.
- The decision to recommend that remote script execution default to disabled until explicitly enabled.

### What should be done in the future

- Implement the proposed firmware command bus refactor before wiring in the JS bindings.
- Port the `mqjs_service` pattern into 0074 firmware.
- Add browser script UX after the device runtime contract is stable.
- Correct the remaining port-documentation inconsistency in 0074 operational docs.

### Code review instructions

- Start with `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/app_main.cpp` and `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.cpp`.
- Then compare the proposed runtime design with `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`.
- Validate the documented current state with:
  - `cd 0074-m5dial-web-remote/server && go test ./...`
  - `cd 0074-m5dial-web-remote/web && npm run build`
  - `cd 0074-m5dial-web-remote/firmware && . $HOME/esp/esp-idf-5.4.1/export.sh && idf.py build`

### Technical details

- Current 0074 app-task ownership points:
  - `app_task()` drains `input_queue` and `ui_command_queue`
  - `handle_ui_command()` applies inbound browser commands
  - `draw_radio_screen()` renders the Lain radio display
- Reference 0048 QuickJS service points:
  - `js_service_start()`
  - `job_bootstrap()`
  - `js_service_eval_to_json()`
  - `mqjs_service_post()` and `mqjs_service_run()`

## Step 2: Narrow the ticket tasks to backend, protocol, and JS runtime ownership

After the initial design pass, the task list still mixed system-level implementation with browser editor work. The user clarified the ownership boundary: this ticket should cover the firmware service, protocol contract, and Go/backend responsibilities, while the browser authoring experience belongs to UX/frontend.

I updated the task list and design guide to match that boundary. The ticket still documents the browser-side protocol needs, because the backend contract has to serve some client, but it no longer treats editor or presentation work as part of the system implementation plan.

### Prompt Context

**User prompt (verbatim):** "Ok, make tasks for the backend / protocol / js part of it, you are a system developer. Leave the UI and design stuff and all that for the UX designers."

**Assistant interpretation:** Narrow the existing ESP-30 implementation plan so its actionable work items focus only on firmware runtime, websocket protocol, and server/backend changes.

**Inferred user intent:** Keep the system ticket disciplined so implementation ownership is clear and UX concerns do not get folded into backend planning.

### What I did

- Edited `tasks.md` to keep only backend/protocol/JS runtime implementation phases.
- Added an explicit out-of-scope section for browser editor and visual UX ownership.
- Updated the main design guide so the scope section and browser-related phases describe protocol hooks rather than UI implementation.
- Prepared the ticket for validation with `docmgr doctor`.

### Why

- Mixed ownership in the task list creates drift during implementation.
- The backend contract should be stable regardless of whether UX chooses a console, full editor, presets, or some other browser interaction pattern.

### What worked

- The ticket structure was already modular, so the scope could be narrowed without rewriting the architecture sections from scratch.
- The existing design already emphasized app-task ownership and transport boundaries, which aligns well with a system-only implementation plan.

### What didn't work

- The first design draft still carried over browser-editor tasks from the broader concept, which no longer matched the clarified ownership boundary.

### What I learned

- The right level of browser detail for this ticket is protocol-facing state and event contracts, not component or interaction design.
- It is useful to separate “browser transport hooks” from “browser UX surface” explicitly in the documentation, because they sound similar but belong to different owners.

### What was tricky to build

- The subtle part was keeping enough browser detail for the system contract to be implementable while removing anything that would read like a UI spec. The fix was to rewrite those sections around typed state, message shapes, and store-level hooks instead of visible controls.

### What warrants a second pair of eyes

- Whether the remaining browser references in the design doc are scoped tightly enough to protocol/runtime concerns.
- Whether the open questions now reflect only system-developer decisions and not product/UI decisions.

### What should be done in the future

- If UX wants a parallel design ticket, split browser authoring/editor work into that separate planning track.

### Code review instructions

- Review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/11/ESP-30-M5DIAL-MQJS-LAIN-DSL--m5dial-microquickjs-scripting-for-lain-os/tasks.md` first.
- Then review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/11/ESP-30-M5DIAL-MQJS-LAIN-DSL--m5dial-microquickjs-scripting-for-lain-os/design-doc/01-analysis-design-and-implementation-guide.md` to confirm the scope wording matches the task list.
- Validate with `docmgr doctor --ticket ESP-30-M5DIAL-MQJS-LAIN-DSL --stale-after 30`.

### Technical details

- Task categories retained:
  - firmware `mquickjs` integration
  - app-command bus refactor
  - websocket message additions
  - Go hub routing and broadcast changes
  - device guardrails and hardware validation
- Task categories explicitly removed from system ownership:
  - browser editor surface
  - visual output/log presentation
  - frontend interaction design
