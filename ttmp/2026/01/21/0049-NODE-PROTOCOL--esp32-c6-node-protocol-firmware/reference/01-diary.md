---
Title: Diary
Ticket: 0049-NODE-PROTOCOL
Status: active
Topics:
    - esp32c6
    - wifi
    - console
    - esp-idf
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Work log for ticket 0049-NODE-PROTOCOL: protocol source review (mled-web Python + C), and an ESP32-C6 firmware design for a Wi-Fi-registered network node using esp_console."
LastUpdated: 2026-01-21T14:46:03.837384107-05:00
WhatFor: "Capture investigation steps, source material, and design decisions while producing the ESP32-C6 node firmware protocol/design analysis."
WhenToUse: "Use to review what was read/changed, reproduce searches, and continue the work without re-deriving context."
---

# Diary

## Goal

Capture the end-to-end work log for ticket `0049-NODE-PROTOCOL`: creating the docmgr workspace, reviewing the MLED/1 protocol (imported notes + Python review + C node implementation), and writing a textbook-style ESP32-C6 firmware design for a Wi-Fi network node using `esp_console`.

## Step 1: Create ticket workspace + seed documents

Created the `docmgr` ticket workspace and the baseline documents (diary + protocol sources reference + main design-doc) so the rest of the investigation can be recorded continuously. This establishes stable paths for linking the source protocol material and for producing the final “textbook-style” analysis deliverable.

### What I did
- Confirmed docmgr config: `docmgr status --summary-only`
- Created the ticket: `docmgr ticket create-ticket --ticket 0049-NODE-PROTOCOL ...`
- Added baseline documents:
  - `reference/01-diary.md`
  - `reference/02-protocol-sources-mled-web.md`
  - `design-doc/01-esp32-c6-node-firmware-as-a-network-node-protocol-design.md`

### Why
- The work spans multiple codebases/tickets; a ticket workspace keeps the narrative + sources + design tied together and searchable.

### What worked
- `docmgr ticket create-ticket` created `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/`.

### What didn't work
- N/A

### What I learned
- `docmgr` is configured to use `esp32-s3-m5/ttmp` as the docs root for this repository.

### What was tricky to build
- N/A (no code yet).

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Inspect new ticket workspace docs under `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/`.

### Technical details
- Commands:
  - `docmgr status --summary-only`
  - `docmgr ticket create-ticket --ticket 0049-NODE-PROTOCOL --title "ESP32-C6 node protocol + firmware" --topics esp32c6,wifi,console,esp-idf`
  - `docmgr doc add --ticket 0049-NODE-PROTOCOL --doc-type reference --title "Diary"`
  - `docmgr doc add --ticket 0049-NODE-PROTOCOL --doc-type design-doc --title "ESP32-C6 Node Firmware as a Network Node (Protocol + Design)"`
  - `docmgr doc add --ticket 0049-NODE-PROTOCOL --doc-type reference --title "Protocol Sources (mled-web)"`

## Step 2: Locate protocol sources (mled-web) and enumerate implementation surfaces

Collected the exact protocol “spec notes” and the implementation surfaces that define real behavior. The protocol material exists in three complementary forms: (1) an imported protocol markdown (“two-phase cue + ping/pong”), (2) a detailed Python implementation review (which includes wire-layout tables), and (3) the C node implementation (`mledc/`) that acts as a portable reference for packing/unpacking and node state machine behavior.

### What I did
- Located the imported protocol notes:
  - `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/sources/local/protocol.md (imported).md`
- Located the Python protocol analysis + diary for context:
  - `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/design-doc/01-python-app-protocol-review.md`
  - `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/reference/01-diary.md`
- Enumerated the C implementation files in `mled-web`:
  - `2026-01-21--mled-web/c/mledc/protocol.h`
  - `2026-01-21--mled-web/c/mledc/protocol.c`
  - `2026-01-21--mled-web/c/mledc/node.h`
  - `2026-01-21--mled-web/c/mledc/node.c`
  - `2026-01-21--mled-web/c/mledc/time.h`
  - `2026-01-21--mled-web/c/mledc/time.c`
  - `2026-01-21--mled-web/c/mledc/transport_udp.h`
  - `2026-01-21--mled-web/c/mledc/transport_udp.c`
  - `2026-01-21--mled-web/c/cmd/mledc_node.c`

### Why
- The ESP32 firmware design needs to be anchored in “what the protocol actually does”, not just what a markdown spec implies.

### What worked
- The C implementation is cleanly separated into:
  - byte-level packing/unpacking (`protocol.c`)
  - transport (`transport_udp.c`)
  - time model and wrap-around math (`time.c`)
  - node behavior/state machine (`node.c`)

### What didn't work
- N/A (source location step).

### What I learned
- The C node is “epoch gated”: it accepts `BEACON` + `PING` regardless of epoch, but gates cue messages (and `TIME_RESP`) on `hdr.epoch_id == controller_epoch`.
- The C node implements both “coarse sync” via `BEACON.execute_at_ms` and “refined sync” via `TIME_REQ/TIME_RESP` (Cristian-style with half-RTT correction).

### What was tricky to build
- N/A (analysis-only so far).

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Keep `reference/02-protocol-sources-mled-web.md` updated with the precise “ground truth” semantics extracted from the code.

### Code review instructions
- Start with `2026-01-21--mled-web/c/mledc/node.c` and trace message handling to understand node behavior.

### Technical details
- Commands:
  - `find 2026-01-21--mled-web -maxdepth 4 -type f \\( -name '*.c' -o -name '*.h' \\)`
  - `sed -n '1,260p' .../protocol.md (imported).md`
  - `sed -n '1,260p' .../design-doc/01-python-app-protocol-review.md`

## Step 3: Analyze the C node implementation (mledc/) as a porting reference

Read the C implementation in `2026-01-21--mled-web/c/mledc/` as if it were a “reference firmware” for the protocol. The goal is to extract implementation invariants that matter for an ESP32 port: which messages are epoch-gated, how show-time is computed and compared under wrap-around, and how the node’s main loop is structured (single-threaded select/poll + scheduled actions).

The most important conclusion is that the C node captures a minimal, coherent mental model for MLED/1: **epoch establishes authority**, **BEACON establishes show-time**, **TIME_REQ refines show-time**, **PING discovers nodes**, and **CUE_PREPARE/CUE_FIRE drive a small cue store + fire scheduler**.

### What I did
- Read the C wire-format definitions and pack/unpack functions:
  - `2026-01-21--mled-web/c/mledc/protocol.h`
  - `2026-01-21--mled-web/c/mledc/protocol.c`
- Read the time model and wrap-around-safe comparison utilities:
  - `2026-01-21--mled-web/c/mledc/time.h`
  - `2026-01-21--mled-web/c/mledc/time.c`
- Read the UDP multicast transport and its multiplexing strategy:
  - `2026-01-21--mled-web/c/mledc/transport_udp.h`
  - `2026-01-21--mled-web/c/mledc/transport_udp.c`
- Read the node behavior/state machine:
  - `2026-01-21--mled-web/c/mledc/node.h`
  - `2026-01-21--mled-web/c/mledc/node.c`
- Skimmed the CLI wrapper:
  - `2026-01-21--mled-web/c/cmd/mledc_node.c`

### Why
- The ESP32-C6 firmware should behave predictably under UDP loss, message reordering, and show-time wrap-around; the C node is compact enough to treat as “behavioral truth” during porting.

### What worked
- The implementation is intentionally boring/explicit:
  - byte-level little-endian encode/decode (no struct packing reliance),
  - no dynamic allocation,
  - a single run loop that merges scheduled actions with socket receive timeouts.
- Epoch gating is simple and robust:
  - `BEACON` + `PING` accepted regardless of epoch,
  - all cue traffic and `TIME_RESP` is gated on `hdr.epoch_id == controller_epoch` and `controller_epoch != 0`.

### What didn't work
- N/A (analysis-only step).

### What I learned
- **Coarse time sync (BEACON):** the node sets `show_time_offset_ms = diff(master_show_ms, local_ms)` on every beacon, and treats that as “synced”.
- **Refined time sync (TIME_REQ/TIME_RESP):** node uses a Cristian-style half-RTT correction:
  - track `t0_local_ms` when sending `TIME_REQ` (msg_id=req_id),
  - on `TIME_RESP`, compute `rtt = t3 - t0`,
  - subtract estimated controller processing time (`master_tx - master_rx`),
  - set offset so `local_ms + offset ≈ master_show_ms` at `t3`.
- **Scheduling:** `CUE_FIRE` stores `(cue_id, execute_at_ms)` into a pending list; each loop iteration applies all fires that are “due” using wrap-around-safe comparison.
- **Retransmission vs dedup:** there is no dedup of repeated `CUE_FIRE`; correctness relies on applying the same cue multiple times being harmless.

### What was tricky to build
- The wrap-around math is subtle but central:
  - `mled_time_u32_duration(start,end)` and `mled_time_is_due(now,execute_at)` treat the u32 clock as a ring and assume durations are “small” (less than ~2^31 ms).
- The time sync refinement also subtly mixes local and show clocks; a port must preserve the exact meaning of each timestamp (`t0`/`t3` are local_ms; master times are show_ms).

### What warrants a second pair of eyes
- Whether the “server processing time” correction in `handle_time_resp()` is the right thing to subtract for the intended controller implementation.
- Whether `mled_udp_send_unicast()` should use a dedicated socket rather than the multicast-bound socket on some platforms (ESP-IDF/lwIP differences).

### What should be done in the future
- If the controller will send multiple `CUE_FIRE` packets for reliability, consider adding explicit dedup semantics keyed by `(epoch_id,cue_id)` (or `(epoch_id,msg_id)` with stable msg_id across repeats).

### Code review instructions
- Start at `2026-01-21--mled-web/c/mledc/node.c` and trace:
  - `handle_beacon()` → epoch establishment + time sync
  - `handle_ping()` → PONG unicast reply
  - `handle_cue_prepare()`/`handle_cue_fire()` → cue store + scheduler
  - `mledc_node_run()` → receive loop + epoch gating + scheduling

### Technical details
- Key invariants extracted for the ESP32 design:
  - `show_ms = local_ms + show_time_offset_ms` (signed offset; stored as `int32_t`)
  - due check uses wrap-around-safe comparison (not naive `now >= execute_at`)
  - epoch change clears stored cues + pending fires and updates controller address

## Step 4: Review prior ESP-IDF firmwares for `esp_console` Wi‑Fi provisioning and single-owner task patterns

Gathered “known-good” building blocks from earlier ESP-IDF firmware tickets that match this ticket’s needs: (1) provisioning Wi‑Fi STA credentials via `esp_console` (scan/join/save, backend selection across USB Serial/JTAG vs UART), and (2) a clean single-owner queue-driven task model (used for LED patterns) that we can reuse for the network protocol node state machine and cue application.

This step is explicitly about reuse: rather than inventing new console UX/state management, we will lift the proven shapes and adapt them to ESP32‑C6 and the MLED/1 protocol.

### What I did
- Read the MO‑033 design + analysis for “console-configured Wi‑Fi selection” patterns and how they reference existing code:
  - `esp32-s3-m5/ttmp/2026/01/20/MO-033-ESP32C6-PREACT-WEB--esp32-c6-device-hosted-preact-zustand-web-ui-with-esp-console-wi-fi-selection/design-doc/01-design-esp32-c6-console-configured-wi-fi-device-hosted-preact-zustand-counter-ui-embedded-assets.md`
  - `esp32-s3-m5/ttmp/2026/01/20/MO-033-ESP32C6-PREACT-WEB--esp32-c6-device-hosted-preact-zustand-web-ui-with-esp-console-wi-fi-selection/analysis/01-codebase-analysis-existing-patterns-for-esp-console-wi-fi-selection-embedded-preact-zustand-assets.md`
- Read the 0029a Wi‑Fi console diary for concrete implementation lessons and pitfalls:
  - `esp32-s3-m5/ttmp/2026/01/05/0029a-ADD-WIFI-CONSOLE--add-esp-console-wifi-config-scan-to-mock-zigbee-hub/reference/01-diary.md`
- Read the MO‑032 design doc for the “single-owner task + queue control surface” pattern (for LED patterns), which is a strong architectural fit for a network node:
  - `esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md`

### Why
- The ESP32‑C6 node firmware needs:
  - a reliable “operator control plane” (`esp_console`) for Wi‑Fi provisioning and node status,
  - an event-driven Wi‑Fi manager that can persist credentials safely,
  - a concurrency model that avoids “random tasks touching shared state”.

### What worked
- The repo already has a proven Wi‑Fi provisioning workflow over console:
  - `wifi scan`, `wifi set`, `wifi connect`, `wifi status`, `wifi clear`
  - NVS schema and behavior is documented (namespace `wifi`, keys `ssid`/`pass`).
- There is a documented (and implemented) pattern for queue-driven mutability that maps well to MLED/1:
  - console handler parses input → sends “command message” to a single owner task,
  - the owner task applies changes + performs IO on schedule.

### What didn't work
- N/A (review-only step).

### What I learned
- Avoid generic public symbol names that can collide with ESP-IDF internals (`wifi_sta_disconnect` was a real collision; the fix was prefixing to `hub_wifi_*`).
- Console backend selection must be compile-time guarded (`CONFIG_ESP_CONSOLE_*` determines which REPL constructors exist); the firmware should tolerate UART vs USB Serial/JTAG configuration.

### What was tricky to build
- N/A (review-only step), but several “sharp edges” were identified for the forthcoming design:
  - console output/logging can interfere with other serial uses; the design should treat the console as the only interactive serial surface.
  - Wi‑Fi event ordering matters for autoconnect; credentials should be set before `esp_wifi_start()` to avoid races.

### What warrants a second pair of eyes
- Whether the node protocol task should be merged with the LED task (apply cues inside LED owner task) or kept separate with a queue boundary between them.

### What should be done in the future
- Relate the concrete source files that implement the Wi‑Fi console and STA manager (`0029-mock-zigbee-http-hub/main/wifi_console.c`, `wifi_sta.c`) to the main design doc once the integration shape is decided.

### Code review instructions
- Read 0029a’s “symbol collision” lesson and ensure any new “wifi_*” or “node_*” modules use a project-specific prefix.

## Step 5: Write the textbook-style ESP32‑C6 node firmware design (protocol + Wi‑Fi + task model)

Converted the protocol source review (imported notes + Python review + C reference node) and the prior firmware patterns (Wi‑Fi console provisioning + single-owner queue tasks) into a single, implementable ESP32‑C6 firmware design document. The emphasis is on “executable understanding”: define invariants, then show how they become tasks, state machines, packet handlers, and bounded data structures.

The key design move is to make the node protocol “boring” and deterministic:
- one node protocol task owns all protocol state and sockets,
- a separate effect task owns hardware state (e.g., LEDs),
- the boundary between them is a message queue.

### What I did
- Wrote/expanded the main design doc:
  - `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/design-doc/01-esp32-c6-node-firmware-as-a-network-node-protocol-design.md`
- Updated the ticket index and changelog for navigability:
  - `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/index.md`
  - `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/changelog.md`
- Updated the task list to reflect remaining work (upload + any remaining linking):
  - `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/tasks.md`

### Why
- The request is not “implement a firmware now” but “write a deep analysis for how to build it”; the design doc is the deliverable that future implementation can follow without re-deriving protocol semantics or concurrency pitfalls.

### What worked
- The repo already contains a consistent set of patterns that compose cleanly:
  - `0045` for ESP32‑C6 Wi‑Fi provisioning via `esp_console`,
  - `0044` for single-owner task + queue control-plane design,
  - `mledc/` for a compact C node reference implementation of MLED/1.

### What didn't work
- N/A

### What I learned
- The most load-bearing “porting invariants” are:
  - epoch gating rules (what’s accepted pre/post epoch),
  - wrap-around-safe `u32` time comparisons,
  - keeping time sync and scheduling in the same owner task (avoid races).

### What was tricky to build
- Turning protocol semantics into a “spec-like” message-by-message behavior section without accidentally inventing features not present in the reference sources.

### What warrants a second pair of eyes
- Whether the system should deduplicate repeated `CUE_FIRE` messages (recommended) or rely purely on idempotency.
- Whether to implement optional `HELLO` (node boot announce) vs. rely solely on periodic controller `PING`.

### What should be done in the future
- Add a small validation playbook for “controller ↔ ESP32 node smoke test” once the implementation exists (wire capture + expected console outputs).

### Code review instructions
- Start with the design doc and follow its “RelatedFiles” pointers:
  - `.../design-doc/01-esp32-c6-node-firmware-as-a-network-node-protocol-design.md`

## Step 6: Upload the analysis document to reMarkable (PDF)

Generated a single bundled PDF from the main design doc (with a clickable table of contents) and uploaded it to the reMarkable device under a ticket-stable folder. This is the final delivery step for the requested “analysis document”.

### What I did
- Verified the uploader tool is available: `remarquee status`
- Dry-ran the upload to confirm paths and output naming:
  - `remarquee upload bundle --dry-run --name "0049-NODE-PROTOCOL - ESP32-C6 Node Firmware (Protocol + Design)" --remote-dir "/ai/2026/01/21/0049-NODE-PROTOCOL" --toc-depth 2 <design-doc.md>`
- Uploaded the PDF:
  - `remarquee upload bundle --non-interactive --name "0049-NODE-PROTOCOL - ESP32-C6 Node Firmware (Protocol + Design)" --remote-dir "/ai/2026/01/21/0049-NODE-PROTOCOL" --toc-depth 2 <design-doc.md>`

### Why
- reMarkable reads PDFs reliably; bundling also gives a navigable ToC which is valuable for a long “textbook-style” document.

### What worked
- Upload succeeded:
  - `OK: uploaded 0049-NODE-PROTOCOL - ESP32-C6 Node Firmware (Protocol + Design).pdf -> /ai/2026/01/21/0049-NODE-PROTOCOL`

### What didn't work
- N/A

### What changed after upload
- After adding an additional timing error-budget section to the design doc, I re-uploaded the PDF with `--force` so the reMarkable copy matches the latest Markdown.

### What I learned
- For a single doc, `remarquee upload bundle` is still useful because it supports `--name` and ToC generation.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- If additional companion docs are added (e.g., a separate “validation playbook”), include them in the same bundle folder for a unified reMarkable packet.

### Code review instructions
- N/A (upload-only step; no repo code changes beyond documentation).

## Step 7: Create implementation task plan + start the “real” work loop (code + commits)

Converted the high-level design into a concrete checklist of firmware protocol tasks (Wi‑Fi provisioning, UDP multicast membership, wire structs, handlers, scheduler) plus a host-side Python smoke script. I also created a dedicated playbook doc that will become the “how to flash + how to run python + what success looks like” reference.

This step is where the work transitions from “analysis” to “implementation”: changes will now land in the `esp32-s3-m5` git repo, and we’ll commit regularly after each coherent milestone (skeleton project, Wi‑Fi console, UDP node, protocol handlers, etc.).

### What I did
- Added an implementation checklist in:
  - `ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/tasks.md`
- Created a playbook doc:
  - `ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/playbook/01-playbook-flash-esp32-c6-node-python-smoke-test.md`

### Why
- The request is to implement and test, not just design; a task list + playbook ensures fast iteration and keeps “when can I test?” unambiguous.

### What worked
- `docmgr doc add` created the playbook doc under the ticket workspace.

### What didn't work
- N/A

### What I learned
- The `esp32-s3-m5/` subtree is its own git repo; we will commit implementation milestones there (including ticket docs under `esp32-s3-m5/ttmp/...`).

### What was tricky to build
- N/A (planning step).

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Keep checking off tasks in `tasks.md` as code lands, and keep the playbook updated so you can test after each milestone.

### Code review instructions
- Review the new task checklist and playbook skeleton:
  - `ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/tasks.md`
  - `ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/playbook/01-playbook-flash-esp32-c6-node-python-smoke-test.md`

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
