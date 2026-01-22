---
Title: Diary
Ticket: 0052-MLED-HOST-UI
Status: active
Topics:
    - ui
    - http
    - webserver
    - backend
    - preact
    - zustand
    - rest
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/mled-server/cmd/mled-server/main.go
      Note: Glazed root bootstrap
    - Path: esp32-s3-m5/mled-server/internal/commands/apply.go
      Note: CLI verb to apply patterns via /api/apply
    - Path: esp32-s3-m5/mled-server/internal/commands/preset_add.go
      Note: CLI preset create (POST /api/presets)
    - Path: esp32-s3-m5/mled-server/internal/commands/serve.go
      Note: Server command refactor to Glazed BareCommand and server-side logging
    - Path: esp32-s3-m5/mled-server/internal/commands/settings_set.go
      Note: CLI settings update (GET+PUT /api/settings)
    - Path: esp32-s3-m5/mled-server/internal/httpapi/request_logging.go
      Note: HTTP request logging middleware
    - Path: esp32-s3-m5/mled-server/internal/httpapi/server.go
      Note: REST+WS endpoints with INFO/DEBUG logging on key operations
    - Path: esp32-s3-m5/mled-server/internal/restclient/client.go
      Note: Shared REST client used by CLI verbs
    - Path: esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/scripts/e2e-rest-verbs.sh
      Note: Repeatable end-to-end CLI verb smoke test
    - Path: esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/scripts/tmux-run-mled-server.sh
      Note: Start server in tmux for manual/e2e testing
ExternalSources: []
Summary: 'Implementation diary for ticket 0052: Glazed CLI bootstrap + logging, REST client verbs, tmux/e2e scripts, and task bookkeeping.'
LastUpdated: 2026-01-21T20:44:06-05:00
WhatFor: Record what changed in the Go host controller and CLI (Glazed bootstrap, server logging, REST verbs) with exact commands/errors for repeatable review and debugging.
WhenToUse: Use when extending CLI verbs, debugging server behavior/logging, or validating the REST API via the provided tmux/e2e scripts.
---


# Diary

## Goal

Capture the end-to-end work for ticket `0052-MLED-HOST-UI`: import initial UI notes, analyze the MLED/1 node protocol sources, and produce a detailed Go host + embedded Preact/Zustand UI design.

## Step 1: Ticket Setup + Source Import

This step creates the new docmgr ticket workspace, imports the provided UI spec notes, and scaffolds the working documents (analysis + design) so subsequent protocol review can be tracked cleanly.

### What I did
- Ran `docmgr status --summary-only` to confirm doc root was `esp32-s3-m5/ttmp` (not repo root).
- Created the ticket: `docmgr ticket create-ticket --ticket 0052-MLED-HOST-UI --title "MLED Host UI (Go HTTP server + Preact/Zustand)" --topics ui,http,webserver,backend,preact,zustand,rest`.
- Imported the provided file: `docmgr import file --file /tmp/fw-screens.md --ticket 0052-MLED-HOST-UI`.
- Created initial docs:
  - `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/reference/01-diary.md`
  - `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/01-node-protocol-analysis-0049-mled-web.md`
  - `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/02-design-go-host-http-api-preact-zustand-ui-embedded.md`

### Why
- Establish a reproducible docs workspace with explicit inputs (`/tmp/fw-screens.md`) and a place to record protocol findings and implementation design decisions.

### What worked
- `docmgr` created the workspace under date-based path `2026/01/21/...`, matching “today” (2026-01-21).
- `docmgr import file` correctly stored the source in `sources/local/` and updated the ticket `index.md`.

### What didn't work
- N/A

### What I learned
- This repo’s doc root is configured in `.ttmp.yaml` to `esp32-s3-m5/ttmp`; assuming `./ttmp` would have been wrong.

### What was tricky to build
- Keeping “source-of-truth” inputs isolated: importing `/tmp/fw-screens.md` into `sources/local/` avoids the common failure mode of losing ad-hoc requirements.

### What warrants a second pair of eyes
- Confirm the intended scope: “exhibition edition” UI notes include “PATTERN_APPLY” as a conceptual primitive, but the protocol sources implement “apply now” via `CUE_PREPARE` + scheduled `CUE_FIRE`.

### What should be done in the future
- N/A

### Code review instructions
- Start at `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/index.md` to see imported sources + new docs.

### Technical details
- Imported source file: `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/sources/local/fw-screens.md`

## Step 2: Protocol Source Audit (0049-NODE-PROTOCOL + mled-web)

This step identifies the authoritative protocol “ground truth” (C structs + behavior), cross-checks it against the Python controller/simulator and the existing Flask UI wrapper, and extracts the host responsibilities needed for a Go implementation.

### What I did
- Located the protocol ticket sources: `docmgr doc list --ticket 0049-NODE-PROTOCOL`.
- Read the protocol design doc to confirm invariants: epoch authority, show clock, and two-phase cue (`CUE_PREPARE`/`CUE_FIRE`).
- Read the curated protocol source index and then the underlying files in `2026-01-21--mled-web/`:
  - `c/mledc/protocol.h` (wire structs/constants)
  - `c/mledc/node.c` (node state machine: epoch gating, cue store, scheduling, PING/PONG)
  - `c/mledc/time.c` (wrap-around-safe u32 show-time math)
  - `c/mledc/transport_udp.c` (multicast join + dual sockets; TIME_REQ source port is ephemeral)
  - `mled/protocol.py` (Python wire structs)
  - `mled/controller.py` (host-side behavior; TIME_REQ → TIME_RESP reply path)
  - `web/app.py` (REST endpoints the UI consumes; maps “apply pattern” to prepare+fire)
- Read the imported protocol notes in mled-web ticket `MO-001-MULTICAST-PY` to capture the explicit “pattern data[12] mapping” guidance and UI-driven flows.

### Why
- The Go host server has to be protocol-compatible with the existing node behavior; for reliability, the C wire structs + node.c behavior are the tightest reference.

### What worked
- The C reference makes subtle transport behavior explicit (two sockets; TIME_REQ comes from ephemeral port, TIME_RESP must reply to that port).
- Python controller code already demonstrates the practical controller responsibilities: beacon loop, discovery via ping, ACK tracking hooks, and cue send helpers.

### What didn't work
- N/A

### What I learned
- The protocol is “fixed-layout + little-endian” and intentionally avoids JSON; Go should mirror this with explicit pack/unpack helpers (no `unsafe` struct casts).
- “Apply now” is not its own message type in the core protocol; it’s naturally implemented as `CUE_PREPARE` targeted per-node plus a small scheduled `CUE_FIRE`.

### What was tricky to build
- TIME sync: nodes can send `TIME_REQ` from an ephemeral source port; the controller must reply to the observed `addr.Port`, not assume `4626`.
- Show time is `u32` with wrap-around-safe comparisons; using naive `int64` comparisons can introduce bugs around rollover if mixed.

### What warrants a second pair of eyes
- Confirm whether the Go host should support the optional `HELLO (0x02)` message type (declared in sources but not required for discovery due to `PING/PONG`).

### What should be done in the future
- N/A

### Code review instructions
- Read the “ground truth” in this order:
  - `2026-01-21--mled-web/c/mledc/protocol.h`
  - `2026-01-21--mled-web/c/mledc/node.c`
  - `2026-01-21--mled-web/mled/controller.py`
  - `2026-01-21--mled-web/web/app.py`

### Technical details
- Protocol constants: group `239.255.32.6`, port `4626`, TTL `1`.
- Header: 32 bytes LE; flags: target mode bits `0..1`, `ACK_REQ` bit `2`.
- PONG payload: 43 bytes (includes 5 bytes padding in Python; C uses a 43-byte pack routine).

## Step 3: Write Protocol Analysis + Go Host/UI Design Docs

This step turns the protocol audit into a copy/paste-ready analysis document and a concrete system design for a Go host controller that serves an embedded Preact/Zustand UI. The main goal is to make future implementation work “mechanical”: the docs should be sufficient to build without rediscovering packet sizes, socket quirks, or UI-to-protocol mappings.

### What I did
- Authored protocol analysis doc: `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/01-node-protocol-analysis-0049-mled-web.md`.
  - Included: transport constants, header layout, flags, message types, payload sizes, node/controller semantics, and key interoperability gotchas (TIME_REQ ephemeral source port, u32 wrap-around time math).
- Authored system design doc: `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/02-design-go-host-http-api-preact-zustand-ui-embedded.md`.
  - Included: Go engine responsibilities + goroutines, show clock model, multicast interface selection, REST API surface, SSE events channel, UI tab structure, and the go-web-frontend-embed packaging contract.
- Copied the UI ASCII screenshots **verbatim** from the imported source file into both documents (to ensure the UI design is unchanged):
  - `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/sources/local/fw-screens.md`

### Why
- Consolidate protocol truth into one place so Go implementation doesn’t drift from C/Python behavior.
- Provide a blueprint for a single-binary deployment with an intentionally small web UI footprint (Preact + Zustand).

### What worked
- The C + Python sources align on the critical protocol pieces (header sizes, multicast group/port, cue payload sizes), which makes it feasible to define a clean Go encoding/decoding layer.
- The existing Flask UI endpoints provide a proven mapping from user actions (“apply pattern”) to protocol operations (prepare+fire with retransmits).

### What didn't work
- N/A

### What I learned
- A lot of “host correctness” is not packet layout; it’s *transport behavior*: interface selection, source ports, and “retry semantics” for UDP.

### What was tricky to build
- Writing the design without implementing code required being explicit about invariants and edge cases (TIME_REQ reply port, epoch reset behavior, and target_mode limitations in current node code).

### What warrants a second pair of eyes
- Validate that SSE (vs WebSocket) matches the desired operator experience in `fw-screens.md` (it does for “push node updates + logs”, but confirm whether bidirectional streaming is desired later).
- Confirm the default server bind should be `127.0.0.1` (local-only) vs `0.0.0.0` (LAN access) for the intended exhibition workflow.

### What should be done in the future
- When implementation begins, add a small protocol regression test suite that encodes/decodes fixtures matching `mledc/protocol.h` sizes and validates sample `PONG` parsing from real captures.

### Code review instructions
- Start with:
  - `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/01-node-protocol-analysis-0049-mled-web.md`
  - `esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/design-doc/02-design-go-host-http-api-preact-zustand-ui-embedded.md`
- Cross-check protocol claims against:
  - `2026-01-21--mled-web/c/mledc/protocol.h`
  - `2026-01-21--mled-web/c/mledc/node.c`

### Technical details
- The design doc explicitly follows the go-web-frontend-embed contract (Vite dev proxy, `go generate` copies `ui/dist/public` to `internal/web/embed/public`, prod build uses `-tags embed`).

## Step 4: Upload Analysis + Design to reMarkable

This step publishes the two main documents (protocol analysis + Go host/UI design) to reMarkable as PDFs so they can be reviewed away from the workstation.

### What I did
- Verified remarquee tooling and auth:
  - `remarquee status`
  - `remarquee cloud account --non-interactive`
- Dry-run conversion/upload for both docs:
  - `remarquee upload md --dry-run <analysis.md> <design.md> --remote-dir "/ai/2026/01/21/0052-MLED-HOST-UI"`
- Uploaded both documents:
  - `remarquee upload md <analysis.md> <design.md> --remote-dir "/ai/2026/01/21/0052-MLED-HOST-UI"`
- Verified remote presence:
  - `remarquee cloud ls /ai/2026/01/21/0052-MLED-HOST-UI --long --non-interactive`
- After updating the docs to include additional verbatim ASCII screenshots, uploaded the updated PDFs to a non-overwriting folder:
  - `remarquee upload md <analysis.md> <design.md> --remote-dir "/ai/2026/01/21/0052-MLED-HOST-UI/v2"`
  - `remarquee cloud ls /ai/2026/01/21/0052-MLED-HOST-UI/v2 --long --non-interactive`

### Why
- Ensure the docs are easy to review in a distraction-free format and available on the device under a stable, ticket-specific folder.

### What worked
- Pandoc conversion succeeded for both markdown documents.
- Both PDFs uploaded successfully to `/ai/2026/01/21/0052-MLED-HOST-UI`.

### What didn't work
- N/A

### What I learned
- `remarquee upload md` handles multiple markdown inputs cleanly, producing one PDF per input with the same basename.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Confirm the rendered PDFs preserve the box-drawing characters and emoji in the ASCII screenshots (depends on the PDF font selection).

### What should be done in the future
- If any glyphs render poorly, switch to a font stack known to include box-drawing + emoji for pandoc/xelatex (or split emoji out of the fenced blocks).

### Code review instructions
- Verify the docs on-device in `/ai/2026/01/21/0052-MLED-HOST-UI/`:
  - `01-node-protocol-analysis-0049-mled-web.pdf`
  - `02-design-go-host-http-api-preact-zustand-ui-embedded.pdf`

### Technical details
- Remote folder: `/ai/2026/01/21/0052-MLED-HOST-UI`.
- Updated docs folder: `/ai/2026/01/21/0052-MLED-HOST-UI/v2`.

## Step 5: Glazed CLI Bootstrap + Server Logging

This step standardizes `mled-server` on Glazed for CLI bootstrap: root-level logging flags and consistent Glazed command registration, while keeping the existing HTTP+WS server behavior intact. The practical goal was to make debugging reliable (INFO-level operational logs, DEBUG-level “busy” logs) and to make it easy to add future CLI verbs without “special casing” Cobra vs Glazed commands.

**Commit (code):** f1ca094 — "mled-server: glazed CLI, REST verbs, SSE events"

### What I did
- Read `glaze help build-first-command` and extracted the recommended logging integration:
  - add logging flags to root: `logging.AddLoggingLayerToRootCommand(...)`
  - initialize logging via Cobra parsing: `logging.InitLoggerFromCobra(cmd)` in `PersistentPreRunE`
- Refactored `serve` from plain Cobra into a Glazed `cmds.BareCommand` (still long-running, no structured output).
- Centralized command registration under Glazed so we can group verbs (e.g., `preset ...`, `settings ...`) and keep consistent parser/middleware config:
  - used `cmds.WithParents(...)` for grouping
  - used `cli.AddCommandsToRootCommand(...)` to auto-create parent commands and register all subcommands
- Added “lots of logging”:
  - request logging middleware (INFO for mutating; DEBUG for most GETs)
  - server-side logs for apply/preset/settings changes and WS connect/disconnect
  - engine event logs (node updates at DEBUG; offline/ack at INFO; errors at ERROR)
- Routed stdlib `log` through Zerolog (so legacy `log.Printf(...)` uses Glazed-configured output).
- Implemented `GET /api/events` (SSE) for UI connection state and push updates.

### Why
- Reduce drift between command styles and make it trivial to add future “verbs” that talk to the server over REST.
- Ensure operational visibility: see what the server is doing without adding temporary printf debugging.

### What worked
- Root-level Glazed logging flags show up on `mled-server --help` and apply to all subcommands.
- `serve` runs with richer logs and request logging without changing the external REST API.
- Command grouping works: `mled-server preset ...` and `mled-server settings ...` appear as proper subtrees.

### What didn't work
- Initial build/test failed due to missing `go.sum` entry for Glazed logging’s log rotation dependency:
  - Command: `cd esp32-s3-m5/mled-server && go test ./...`
  - Error:
    - `/home/manuel/go/pkg/mod/github.com/go-go-golems/glazed@v0.7.14/pkg/cmds/logging/init.go:15:2: missing go.sum entry for module providing package gopkg.in/natefinch/lumberjack.v2`
  - Fix: `cd esp32-s3-m5/mled-server && go mod tidy`
- I initially typed the wrong Glazed type name in `client_flags.go` (`fields.Field`), which doesn’t exist:
  - Compile error: `undefined: fields.Field`
  - Fix: use `*fields.Definition` (type alias for parameter definitions).

### What I learned
- Glazed’s `cli.AddCommandsToRootCommand` is ideal when you want nested subcommands driven by `WithParents(...)` without manually wiring Cobra parents.
- Root-level logging initialization is the cleanest way to get consistent logs across both Glazed-bridged commands and any residual Cobra usage.

### What was tricky to build
- Glazed structured output prints “no output” when a command yields zero rows; tests/scripts must handle empty output when using `--output json` for list commands.
- Keeping INFO vs DEBUG sane: node updates are noisy and belong in DEBUG, while “offline” and “apply ack” are operator-relevant and belong in INFO.

### What warrants a second pair of eyes
- Confirm the request-logging policy (GET=DEBUG, mutating=INFO) matches expected operator workflows; we may want certain GET endpoints at INFO when running headless.
- Confirm the default HTTP address choice. The tasks doc says `localhost:8080`, but the server currently defaults to `localhost:8765` (and scripts use `localhost:18765` for isolated tests).

### What should be done in the future
- Consider adding a dedicated `discover` REST endpoint (server-side) and a corresponding CLI verb if we want discovery to be entirely server-driven.

### Code review instructions
- Start at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/cmd/mled-server/main.go`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/commands/serve.go`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/httpapi/request_logging.go`
- Validate:
  - `cd esp32-s3-m5/mled-server && go test ./...`
  - `cd esp32-s3-m5/mled-server && go run ./cmd/mled-server --help`

### Technical details
- Glazed logging hook: `PersistentPreRunE` calls `logging.InitLoggerFromCobra(cmd)` and routes stdlib `log` to Zerolog.
- Request logging: INFO for POST/PUT/PATCH/DELETE; ERROR for 5xx; DEBUG otherwise.

## Step 6: REST Client Verbs (Apply / Presets / Settings)

This step adds the operator-facing CLI verbs that talk to a running `mled-server` instance via REST. The goal is for “verbs” like applying patterns/presets and managing settings/presets to be scriptable and to share a consistent flag surface (`--server`, `--timeout-ms`, plus `--output json` via Glazed).

**Commit (code):** f1ca094 — "mled-server: glazed CLI, REST verbs, SSE events"

### What I did
- Added a shared HTTP client package for CLI verbs:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/restclient/client.go`
- Implemented verbs:
  - `mled-server status`, `mled-server nodes`, `mled-server presets`
  - `mled-server apply` (builds PatternConfig or applies `--preset`)
  - convenience: `mled-server apply-preset`, `apply-solid`, `apply-rainbow`
  - preset CRUD: `mled-server preset add|list|update|delete`
  - settings: `mled-server settings get|set`
- Ensured verbs are registered and grouped correctly via Glazed `WithParents(...)` and the central `AddCommandsToRootCommand`.

### Why
- The UI is useful, but the fastest debug/ops workflow is still “CLI verb → REST → observe logs/WS/events”.
- A REST client CLI makes it easy to integrate into scripts and smoke tests.

### What worked
- After fixes, this works as expected:
  - `cd esp32-s3-m5/mled-server && go run ./cmd/mled-server nodes`
  - `cd esp32-s3-m5/mled-server && go run ./cmd/mled-server apply --type off --all`

### What didn't work
- The first attempt to run `nodes` failed because `--server` default did not populate (settings.Server ended up empty):
  - Command: `cd esp32-s3-m5/mled-server && go run ./cmd/mled-server nodes`
  - Error: `Error: Get "/api/nodes": unsupported protocol scheme ""`
  - Cause: embedding a `ClientSettings` struct didn’t get populated by `values.DecodeSectionInto(...)` as expected.
  - Fix: flatten `Server` and `TimeoutMS` into each command’s settings struct (so decoding reliably sets them).
- `settings get --output json` initially produced keys like `multicastgroup` instead of `multicast_group` because `types.NewRowFromStruct(..., true)` lowercased field names, not JSON tags.
  - Fix: for settings/nodes outputs, build rows explicitly with map keys matching API JSON tags (`bind_ip`, `multicast_group`, etc.).

### What I learned
- For Glazed output, `types.NewRowFromStruct(..., true)` lowercases Go field names (it does not honor JSON tags). For “API-shaped” output, prefer `types.NewRowFromMap` with explicit keys.

### What was tricky to build
- Maintaining contract symmetry:
  - The server speaks JSON using struct tags (`bind_ip`, `multicast_group`, ...).
  - The CLI output should also use those keys if we want round-trippable scripts and predictable `jq` usage.

### What warrants a second pair of eyes
- The mapping of “convenience commands” to `PatternConfig` should be kept in sync with `mledhost.BuildWirePattern`; if new wire patterns are added, we should update both places.

### What should be done in the future
- Add `preset export/import` (file-based) for moving presets between machines.
- Add `apply --all` default behavior confirmation flag when running in “exhibition” environments (optional safety).

### Code review instructions
- Start at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/restclient/client.go`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/commands/apply.go`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/commands/preset_add.go`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/commands/settings_get.go`
- Validate:
  - `cd esp32-s3-m5/mled-server && go test ./...`
  - With a running server: `go run ./cmd/mled-server preset list --output json`

### Technical details
- All REST verbs share `--server` (default `http://localhost:8765`) and `--timeout-ms` (default `5000`).
- Apply endpoint used: `POST /api/apply` with `node_ids: "all"|[]string` and `config: PatternConfig`.

## Step 7: Ticket Scripts (tmux + E2E REST Verbs)

This step packages the exact manual commands used during development into reusable scripts stored under the ticket’s `scripts/` folder. The intent is to make local validation repeatable and low-friction: one script starts the server in tmux, another runs an end-to-end suite of CLI verbs against it.

**Commit (code):** f1ca094 — "mled-server: glazed CLI, REST verbs, SSE events"

### What I did
- Added scripts to:
  - `.../scripts/tmux-run-mled-server.sh`
  - `.../scripts/tmux-stop-mled-server.sh`
  - `.../scripts/e2e-rest-verbs.sh`
  - `.../scripts/e2e-rest-verbs-tmux.sh`
  - `.../scripts/ps-mled-server.sh`
  - `.../scripts/kill-mled-server-serve.sh`
  - `.../scripts/README.md`
- Used `tmux` to run an isolated server instance:
  - `tmux new-session -d -s mled-server-test "cd .../mled-server && go run ./cmd/mled-server serve --http-addr localhost:18765 --data-dir /tmp/mled-server-test-var --log-level info"`
- Ran the e2e verb suite against the tmux server (preset CRUD + apply verbs + settings get/set).

### Why
- This makes “local smoke plan” concrete and runnable without retyping commands.
- tmux keeps the server running while running multiple CLI invocations from another shell.

### What worked
- The tmux-run script starts a server and prints how to attach.
- The e2e script exercises:
  - `status`, `nodes`, `presets`, `preset list`
  - `preset add/update/delete`
  - `apply`, `apply-solid`, `apply-rainbow`, `apply-preset`
  - `settings get/set`

### What didn't work
- The first e2e script run failed when `presets --output json` printed nothing (no rows), causing `jq` to error:
  - Fix: treat empty output as `[]` via a `json_or_empty_array` helper in the script.

### What I learned
- When validating Glazed commands with `--output json`, scripts should handle “no rows” as an empty array explicitly.

### What was tricky to build
- Avoiding side effects: the e2e scripts use a dedicated `--data-dir` under `/tmp` and a dedicated port (`18765`) so they can run without interfering with an existing server session.

### What warrants a second pair of eyes
- Confirm whether we want the scripts to keep the tmux session running by default or always stop it (currently it stops unless `KEEP_RUNNING=1`).

### What should be done in the future
- Add a second e2e mode that runs against a user-provided server without modifying settings (read-only validation mode).

### Code review instructions
- Run:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/scripts/e2e-rest-verbs-tmux.sh`
- Inspect server logs:
  - `tmux attach -t mled-server-e2e`

### Technical details
- `e2e-rest-verbs-tmux.sh` starts the server in tmux on `localhost:18765`, runs verbs, and stops the tmux session unless `KEEP_RUNNING=1`.

## Step 8: Finalize + Smoke Test (tmux + E2E) + Capture Glazed Help

This step is a final validation and “make it repeatable” pass: I started the server under tmux (to avoid orphaned processes), ran the end-to-end REST verb suite against it, and captured the full `glaze help build-first-command` output into the ticket so we can refer back to Glazed’s recommended patterns when adding more verbs.

**Commit (code):** f1ca094 — "mled-server: glazed CLI, REST verbs, SSE events"

### What I did
- Started an isolated server in tmux:
  - `MCAST_PORT=14626 ./scripts/tmux-run-mled-server.sh`
- Ran the verb suite:
  - `./scripts/e2e-rest-verbs.sh`
- Captured Glazed tutorial output:
  - `glaze help build-first-command > reference/02-glaze-help-build-first-command.txt`

### Why
- tmux makes it obvious what server instance is “the one”, and provides a single place to tail logs.
- A stored Glazed tutorial snapshot keeps the “how should we structure future commands?” reference local to the ticket.

### What worked
- `./scripts/e2e-rest-verbs.sh` passed against the tmux server (`http://localhost:18765`).
- `GET /api/events` returned `200` and a valid SSE stream (smoke-checked by the script).

### What didn't work
- N/A

### What I learned
- `go run` will spawn both a wrapper process and a compiled child binary; tmux helps keep that from turning into “how many servers are running?” confusion.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Confirm whether `MCAST_PORT=14626` is the right default for tests in this repo (it avoids collisions, but it won’t talk to real devices if they’re on `4626`).

### What should be done in the future
- Add a second `e2e` mode that runs with real device defaults (`MCAST_PORT=4626`) and optionally requires at least one node online.

### Code review instructions
- Start at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/21/0052-MLED-HOST-UI--mled-host-ui-go-http-server-preact-zustand/scripts/e2e-rest-verbs.sh`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/mled-server/internal/httpapi/sse.go`
- Validate:
  - `cd esp32-s3-m5/mled-server && go test ./...`
  - `./scripts/e2e-rest-verbs-tmux.sh`

### Technical details
- SSE endpoint: `GET /api/events` (EventSource-compatible stream).
