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
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T18:40:53.815135986-05:00
WhatFor: ""
WhenToUse: ""
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
