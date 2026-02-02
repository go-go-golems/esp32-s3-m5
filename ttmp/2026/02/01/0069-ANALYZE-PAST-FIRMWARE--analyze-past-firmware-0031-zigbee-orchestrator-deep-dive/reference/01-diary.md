---
Title: Diary
Ticket: 0069-ANALYZE-PAST-FIRMWARE
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32s3
    - esp32h2
    - esp32
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/reference/02-esp32-h2-ncp-firmware-and-znsp-protocol-host-integration-reference.md
      Note: Reference doc created and updated in Steps 9-10.
ExternalSources: []
Summary: ""
LastUpdated: 2026-02-02T11:35:30-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Create a comprehensive, structured deep dive for ticket 0031 (Zigbee orchestrator) by synthesizing its diary, analysis, and design docs along with all related docmgr tickets and their diaries, then store the result in ticket 0069.

## Step 1: Create the analysis ticket + map the source set

This step established the new analysis workspace and defined the exact documentation corpus to synthesize. I created ticket 0069, added the analysis and diary docs, and enumerated the related docmgr tickets (0031 plus its upstream/adjacent Zigbee and event-bus tickets) so the deep dive can be built from explicit sources rather than memory.

The key outcome is a concrete source map: 0031's analysis/design/investigation + its diary, plus supporting diaries from 001 (gateway bring-up), 0029/0029a (mock hub + Wi-Fi console), 0030 (event bus demo), and the deeper Zigbee stack analyses (0032/0034/0035).

### Prompt Context

**User prompt (verbatim):** "Go through all the work that went into 0031-zigbee-orchestrator, especially the related docmgr tickets, and their diaries. 

Do an in depth analysis of the history and the stopping points, all the things that happened and why and where. 

Basically a textbook length technical deep dive of the diaries and the related docs. 

Store in a new ticket 0069-ANALYZE-PAST-FIRMWARE and store the document in the ticket. 

Keep your own frequent diary as you progress. 

I need to catch up on this project and know everything aobut it."

**Assistant interpretation:** Create a new 0069 analysis ticket, gather and review 0031 and related docmgr tickets/diaries, then write a long-form technical retrospective including history and stopping points.

**Inferred user intent:** Get a complete, structured catch-up on the 0031 Zigbee orchestrator effort, with clear provenance and a single canonical deep-dive document.

### What I did
- Created the new ticket workspace: `docmgr ticket create-ticket --ticket 0069-ANALYZE-PAST-FIRMWARE ...`
- Added the analysis doc and diary doc for 0069 using `docmgr doc add`.
- Enumerated existing tickets to identify the related Zigbee + event-bus lineage (`docmgr ticket list`).
- Listed docs for the key related tickets: 0031, 001, 0029, 0029a, 0030, 0032, 0034, 0035.
- Confirmed available vocabulary for topics with `docmgr vocab list --category topics`.

### Why
- The user asked for a textbook-length, source-faithful analysis, so the first step is to create a durable workspace and a bounded source set to synthesize.

### What worked
- The ticket workspace and documents were created successfully under `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--...`.
- The related docmgr tickets and their diaries are accessible and clearly enumerated.

### What didn't work
- N/A.

### What I learned
- The 0031 effort pulls directly from prior Zigbee bring-up (001/002) and the event-bus + protobuf patterns in 0029/0030; those diaries are essential inputs to the retrospective.

### What was tricky to build
- N/A (setup and scoping only).

### What warrants a second pair of eyes
- Validate that the related ticket set is complete before the deep dive is finalized (e.g., if other Zigbee tickets should be included).

### What should be done in the future
- Read and extract key facts from each related diary and doc, then draft the long-form retrospective.

### Code review instructions
- N/A (documentation setup only).

### Technical details
- Commands used:
  - `docmgr ticket create-ticket --ticket 0069-ANALYZE-PAST-FIRMWARE --title "Analyze past firmware: 0031 Zigbee orchestrator deep dive" --topics zigbee,esp-idf,esp32s3,esp32h2,esp32`
  - `docmgr doc add --ticket 0069-ANALYZE-PAST-FIRMWARE --doc-type analysis --title "0031 Zigbee Orchestrator Deep Dive: Full Retrospective"`
  - `docmgr doc add --ticket 0069-ANALYZE-PAST-FIRMWARE --doc-type reference --title "Diary"`
  - `docmgr ticket list`
  - `docmgr doc list --ticket 0031-ZIGBEE-ORCHESTRATOR`
  - `docmgr doc list --ticket 001-ZIGBEE-GATEWAY`
  - `docmgr doc list --ticket 0029-HTTP-EVENT-MOCK-ZIGBEE`
  - `docmgr doc list --ticket 0029a-ADD-WIFI-CONSOLE`
  - `docmgr doc list --ticket 0030-CARDPUTER-CONSOLE-EVENTBUS`
  - `docmgr doc list --ticket 0032-ANALYZE-NCP-FIRMWARE`
  - `docmgr doc list --ticket 0034-ANALYZE-ESP-ZIGBEE-LIB`
  - `docmgr doc list --ticket 0035-IMPROVE-NCP-LOGGING`
  - `docmgr vocab list --category topics`

## Step 2: Read source docs and extract the full timeline

This step gathered the primary evidence: the 0031 diary plus the upstream Zigbee and event-bus tickets that fed into it. I read each diary and key analysis/design docs, extracted the major decision points, and recorded the known blockers so the final synthesis could be precise and chronological.

The key outcome is a complete source-driven timeline, including the explicit stopping points around authorization timeouts, channel selection, and environment constraints.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Review the 0031 and related docmgr tickets/diaries in detail to build a full historical narrative.

**Inferred user intent:** Ensure the deep dive is source-accurate and comprehensive rather than a high-level summary.

### What I did
- Read the 0031 diary, analysis, design, and investigation report docs.
- Reviewed `tasks.md` for the explicit open/closed work items in 0031.
- Read the related diaries and key docs from tickets 001, 0029, 0029a, 0030, 0032, 0034, and 0035.
- Extracted the recurring blockers and reasons (UART link, NCP stubs, channel mask timing, authorization timeouts, and environment limitations).

### Why
- The user asked for a textbook-length deep dive, which requires accurate chronology and detail from the primary docs.

### What worked
- The diaries provided explicit steps, commands, and outcomes, which makes it possible to reconstruct the technical narrative end-to-end.

### What didn't work
- N/A.

### What I learned
- The 0031 effort is not isolated; it is the convergence of a Zigbee bring-up line (001/002/0032/0034/0035) and an event-bus/protobuf tooling line (0029/0029a/0030).

### What was tricky to build
- Keeping the ordering straight across multiple tickets that overlap in time; I mapped them into a single timeline before writing the final analysis.

### What warrants a second pair of eyes
- Confirm whether any additional Zigbee-related tickets beyond the ones listed should be included in the final narrative.

### What should be done in the future
- Draft and write the full deep dive document using the extracted timeline and stopping points.

### Code review instructions
- N/A (analysis-only step).

### Technical details
- Files reviewed (sample):
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../reference/01-diary.md`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../design-doc/01-design-cardputer-zigbee-orchestrator-esp-event-bus-http-202-protobuf-ws.md`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md`
  - `ttmp/2026/01/04/001-ZIGBEE-GATEWAY--.../reference/01-diary.md`
  - `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--.../reference/01-diary.md`
  - `ttmp/2026/01/05/0029a-ADD-WIFI-CONSOLE--.../reference/01-diary.md`
  - `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--.../reference/01-diary.md`
  - `ttmp/2026/01/06/0032-ANALYZE-NCP-FIRMWARE--.../reference/01-diary.md`
  - `ttmp/2026/01/06/0034-ANALYZE-ESP-ZIGBEE-LIB--.../reference/01-diary.md`
  - `ttmp/2026/01/06/0035-IMPROVE-NCP-LOGGING--.../reference/01-diary.md`

## Step 3: Write and store the full retrospective deep dive

This step produced the requested "textbook length" technical deep dive by synthesizing the source docs into a structured narrative with chronology, architecture, and stopping points. The final document lives in the 0069 analysis doc and includes a source map, phased timeline, open issues, and concrete next actions.

The key outcome is a single, self-contained document that a new developer can read to understand the entire 0031 lineage and its current status.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Create and store the long-form retrospective in the 0069 analysis doc, using the extracted timeline and related docs.

**Inferred user intent:** Get a canonical, comprehensive catch-up document in one place.

### What I did
- Wrote the full retrospective in:
  - `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md`
- Structured the document to include:
  - Executive summary
  - Source map and ticket graph
  - Architectural lineage and decisions
  - Chronological narrative (0031 steps grouped by phase)
  - Related ticket summaries
  - Stopping points and root causes
  - Current state snapshot and next actions
  - File and artifact index

### Why
- The user requested a "textbook length" deep dive with history, stopping points, and explicit sources.

### What worked
- The source material was detailed enough to support a complete narrative without gaps.

### What didn't work
- N/A.

### What I learned
- The two biggest unresolved issues (authorization timeout and channel stuck on 13) map cleanly to missing hardware validation steps, not to architecture flaws.

### What was tricky to build
- Balancing depth with readability; I used a phased structure and an explicit source map to keep it navigable.

### What warrants a second pair of eyes
- Confirm that the file index includes the most important host and NCP source files and that no critical scripts are omitted.

### What should be done in the future
- Relate key source docs/files to the 0069 analysis doc via `docmgr doc relate` so the document has tight backlinks.

### Code review instructions
- N/A (documentation-only step).

### Technical details
- Output document:
  - `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md`

## Step 4: Relate key sources to the 0069 analysis document

This step tightened the documentation graph by linking the most load-bearing source docs to the new 0069 analysis doc. That ensures future readers can navigate directly from the deep dive to the original diaries and design/analysis documents without hunting.

The outcome is a small, intentional RelatedFiles set (7 entries) that cover the core 0031 docs plus the most important upstream tickets.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Ensure the new analysis doc is linked to the relevant source material for traceability.

**Inferred user intent:** Make the deep dive document navigable and source-backed inside docmgr.

### What I did
- Added RelatedFiles links to the 0069 analysis doc via `docmgr doc relate`.

### Why
- The deep dive should remain grounded in the source diaries and design docs, and docmgr links are the fastest way to preserve that provenance.

### What worked
- RelatedFiles were updated with 7 key sources (0031 diary, 0031 analysis/design/investigation, 001 diary, 0029 diary, 0030 diary).

### What didn't work
- N/A.

### What I learned
- Keeping RelatedFiles small but high-signal makes the doc easier to navigate without clutter.

### What was tricky to build
- N/A (straightforward docmgr relate step).

### What warrants a second pair of eyes
- Confirm whether any additional deep-stack analysis docs (0032/0034/0035) should be linked directly rather than referenced only in the narrative.

### What should be done in the future
- Update RelatedFiles if the 0069 deep dive is extended with new source references.

### Code review instructions
- N/A.

### Technical details
- Command used:
  - `docmgr doc relate --doc .../analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md --file-note "..."` (7 file-note entries)

## Step 5: Add the Xerox PARC-style technical memo

This step produced the requested technical memo in a Xerox PARC format. The memo is a concise, structured counterpart to the long-form deep dive and is intended for quick circulation among stakeholders who want an executive-ready summary with actionable next steps.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Provide a PARC-format technical memo variant of the 0031 retrospective.

**Inferred user intent:** Have a memo-format artifact suitable for distribution and quick onboarding.

### What I did
- Created a new analysis doc for the memo:
  - `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/02-technical-memo-parc-0031-zigbee-orchestrator-retrospective.md`
- Wrote the memo using a PARC-style header (To/From/Subject/Date/Keywords) and a structured body (Abstract, Background, System Overview, Chronology, Findings, Stopping Points, Recommendations, References).

### Why
- The memo format is easier to circulate and aligns with the requested Xerox PARC style.

### What worked
- The memo captures the same conclusions as the deep dive but in a tighter, standardized structure.

### What didn't work
- N/A.

### What I learned
- A memo format helps separate the narrative (deep dive) from the action-focused summary (memo).

### What was tricky to build
- Keeping the memo concise without dropping the key blockers (authorization timeouts, channel persistence, environment limits).

### What warrants a second pair of eyes
- Confirm the memo's scope and phrasing align with stakeholder expectations and that no key blocker was omitted.

### What should be done in the future
- Update the memo when the two primary experiments (security and channel) are completed.

### Code review instructions
- N/A.

### Technical details
- Output file:
  - `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/02-technical-memo-parc-0031-zigbee-orchestrator-retrospective.md`

## Step 6: Update RelatedFiles to include 0032/0034/0035

This step updated the RelatedFiles block on the deep dive analysis to include the requested deep-stack analysis tickets (0032, 0034, 0035). To keep the list focused, the related set now prioritizes the most critical 0031 docs, the 001 gateway diary, and the three analysis docs that inform NCP and stack behavior.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Add 0032/0034/0035 as direct RelatedFiles for the 0069 deep dive.

**Inferred user intent:** Ensure the deep dive links directly to the NCP and stack analyses used during 0031.

### What I did
- Edited the RelatedFiles list in:
  - `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md`
- Added direct links to:
  - `0032-ANALYZE-NCP-FIRMWARE` analysis
  - `0034-ANALYZE-ESP-ZIGBEE-LIB` analysis
  - `0035-IMPROVE-NCP-LOGGING` analysis

### Why
- These documents are the authoritative deep-stack analyses referenced throughout the 0031 debugging and integration work.

### What worked
- The RelatedFiles list now includes the requested tickets and remains focused on the most load-bearing sources.

### What didn't work
- N/A.

### What I learned
- Keeping RelatedFiles tight improves navigability even as the narrative references a broader set of tickets.

### What was tricky to build
- Balancing the RelatedFiles count with relevance while honoring the request to add more sources.

### What warrants a second pair of eyes
- Verify whether the 0029/0030 diaries should be re-added to RelatedFiles for convenience (they remain referenced in the narrative).

### What should be done in the future
- If the deep dive expands further, re-evaluate the RelatedFiles list for optimal coverage.

### Code review instructions
- N/A.

### Technical details
- Files edited:
  - `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md`

## Step 7: Upload all 0069 documents to reMarkable

This step uploaded all markdown documents in the 0069 ticket to the reMarkable tablet as PDFs using `remarquee`. A dry-run confirmed the upload plan and destination, then the uploads were executed in smaller batches after an initial timeout.

### Prompt Context

**User prompt (verbatim):** "upload all to tablet."

**Assistant interpretation:** Upload every 0069 ticket document to the reMarkable device.

**Inferred user intent:** Ensure the new deep dive and memo (plus index/README/tasks/changelog/diary) are accessible on the tablet for review.

### What I did
- Confirmed `remarquee` is available (`remarquee status`).
- Enumerated all markdown files in the 0069 ticket directory.
- Ran a dry-run upload to validate conversion and destination.
- Uploaded all docs to `/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE`.
- Verified the remote directory contents with `remarquee cloud ls`.

### Why
- The user requested all documents be uploaded to the tablet for reading/annotation.

### What worked
- All PDFs uploaded successfully.
- Remote directory listing shows the expected files.

### What didn't work
- The first multi-file upload command timed out after the first file; I reran the remaining uploads in smaller batches with a longer timeout.

### What I learned
- Large multi-file uploads are more reliable when split into smaller batches in this environment.

### What was tricky to build
- Managing command timeouts while keeping the upload atomic per file.

### What warrants a second pair of eyes
- N/A.

### What should be done in the future
- If new docs are added to ticket 0069, re-run the upload with the same remote dir.

### Code review instructions
- N/A.

### Technical details
- Dry-run:
  - `remarquee upload md --dry-run --remote-dir "/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE" <files...>`
- Uploads:
  - `remarquee upload md --remote-dir "/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE" ...`
- Verification:
  - `remarquee cloud ls /ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE --long --non-interactive`

## Step 8: Upload bundled PDF (single document with ToC)

This step bundled all 0069 markdown docs into a single PDF with a table of contents and uploaded it to the same reMarkable folder. This provides a one-file reading experience alongside the individual PDFs.

### Prompt Context

**User prompt (verbatim):** "yes"

**Assistant interpretation:** Create and upload a bundled PDF for the entire 0069 ticket.

**Inferred user intent:** Have a single combined document on the tablet in addition to the individual files.

### What I did
- Ran a dry-run `remarquee upload bundle` to confirm conversion and destination.
- Uploaded the bundle named `0069-ANALYZE-PAST-FIRMWARE` with `--toc-depth 2`.
- Verified the file exists in `/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE`.

### Why
- A single PDF is easier to read and annotate sequentially on the tablet.

### What worked
- The bundle uploaded successfully and appears in the remote folder.

### What didn't work
- N/A.

### What I learned
- Bundling with `remarquee upload bundle` is reliable and produces a clean ToC for this doc set.

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- N/A.

### What should be done in the future
- If new docs are added to ticket 0069, re-run the bundle upload to refresh the combined PDF.

### Code review instructions
- N/A.

### Technical details
- Dry-run:
  - `remarquee upload bundle --dry-run --name "0069-ANALYZE-PAST-FIRMWARE" --remote-dir "/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE" --toc-depth 2 <files...>`
- Upload:
  - `remarquee upload bundle --name "0069-ANALYZE-PAST-FIRMWARE" --remote-dir "/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE" --toc-depth 2 <files...>`
- Verification:
  - `remarquee cloud ls /ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE --long --non-interactive`

## Step 9: Audit H2 NCP firmware and host protocol sources

This step focused on the actual firmware and host protocol implementations. I read the H2 NCP sources (bus, frame, ZB handlers) plus the 0031 host-side components to extract the exact on-wire framing, CRC rules, and command table behaviors.

The key outcome is a vetted, source-derived inventory of protocol details (SLIP framing, header layout, CRC16 seed, command IDs, and handler coverage) to feed the new reference doc.

### Prompt Context

**User prompt (verbatim):** "Work on docmgr ticket 0069-ANALYZE-PAST-FIRMWARE and create a new indepth exploration and reference of the actual H2 firmware and NCP protocol and how to use it from a host device. Be very detailed, look at the firmware as well. 

Store the detailed exploration/reference/textbook document in the ticket, keep a frequent detailed research diary, upload to remarkable and then commit to git."

**Assistant interpretation:** Produce a new, detailed H2 NCP firmware + ZNSP protocol reference in ticket 0069, grounding it in the actual firmware sources and host integration code.

**Inferred user intent:** Get a dependable, code-accurate protocol handbook for the H2 NCP and how to drive it from a host device.

**Commit (code):** N/A (research only)

### What I did
- Searched for NCP/H2 protocol sources with `rg` and collected the relevant files.
- Read the NCP transport and framing code: `esp_ncp_bus.c`, `esp_ncp_frame.c`, `slip.h`, and `esp_ncp_main.c`.
- Read the NCP command handler table and payload logic in `esp_ncp_zb.c` and `esp_ncp_zb.h`.
- Read the host framing/dispatch path in `esp_host_frame.c`, `esp_host_bus.c`, `esp_host_main.c`, and `esp_host_zb.c`.
- Verified host usage patterns in `0031-zigbee-orchestrator/main/gw_zb.c` and the H2 entry point in `esp_zigbee_ncp.c`.

### Why
- The user requested a highly detailed, source-grounded reference; the only reliable way to do this is to read the actual firmware and host protocol implementations.

### What worked
- The esp-zigbee NCP sources are self-contained enough to extract framing, CRC, and command behavior without guessing.

### What didn't work
- N/A.

### What I learned
- The host parser can handle multiple frames per SLIP decode, while the NCP expects a single frame per SLIP packet.
- Several ZNSP IDs exist but are unimplemented in the NCP handler table (they return invalid-arg behavior).

### What was tricky to build
- Reconciling host-vs-NCP semantics (e.g., sequence number handling and ID matching) required tracing both sides carefully because the host only matches responses by ID, not by sequence.

### What warrants a second pair of eyes
- Validate the command table completeness and the endianness assumptions for CRC16 on non-ESP hosts.

### What should be done in the future
- Capture on-wire traffic in a real host+H2 session to confirm framing assumptions and spot any undocumented quirks.

### Code review instructions
- N/A (research-only step).

### Technical details
- Commands used (sample):
  - `rg --files -g '*H2*' -g '*h2*' -g '*ncp*' -g '*firmware*' -g '*slip*' -g '*protocol*'`
  - `rg "H2|h2|ncp|NCP|zb_host|znsp|slip" 0031-zigbee-orchestrator -n`
  - `sed -n '1,220p' thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_frame.c`
  - `sed -n '1,260p' thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
  - `sed -n '1,260p' 0031-zigbee-orchestrator/components/zb_host/src/esp_host_frame.c`
  - `sed -n '1,260p' 0031-zigbee-orchestrator/components/zb_host/src/esp_host_zb.c`

## Step 10: Write the reference doc and upload to reMarkable

This step produced the new protocol reference document and tied it back to the actual firmware sources. I documented the framing rules, command IDs, payload expectations, and host integration patterns, then related the source files that shaped the write-up.

The key outcome is the finished reference doc stored in ticket 0069 and uploaded to reMarkable for offline review.

### Prompt Context

**User prompt (verbatim):** (see Step 9)

**Assistant interpretation:** Write the detailed H2 NCP protocol reference in 0069, keep the diary updated, and upload the document to reMarkable.

**Inferred user intent:** Have a shareable, portable protocol handbook that can be read on-device and used as a living reference.

**Commit (code):** N/A (docs-only)

### What I did
- Created and wrote the new reference doc:
  - `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/reference/02-esp32-h2-ncp-firmware-and-znsp-protocol-host-integration-reference.md`
- Added related source files with `docmgr doc relate`.
- Uploaded the doc to reMarkable using `remarquee upload md` (after a dry run).

### Why
- The user asked for a detailed, source-backed reference and requested it be uploaded for reading on reMarkable.

### What worked
- The docmgr relate step completed cleanly and the reMarkable upload succeeded.

### What didn't work
- N/A.

### What I learned
- The safest reMarkable path is a ticket-scoped folder: `/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE`.

### What was tricky to build
- Ensuring the command ID tables stayed consistent with the firmware sources while keeping the doc readable required cross-checking multiple headers and implementations.

### What warrants a second pair of eyes
- Confirm that the ZNSP command tables match your current esp-zigbee SDK version and that no new IDs were added upstream.

### What should be done in the future
- Add APS notification handlers on the host if the project needs application payloads over ZNSP.

### Code review instructions
- Start here: `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/reference/02-esp32-h2-ncp-firmware-and-znsp-protocol-host-integration-reference.md`
- Validate by reading for protocol completeness (framing + CRC + command tables).

### Technical details
- Commands used:
  - `docmgr doc add --ticket 0069-ANALYZE-PAST-FIRMWARE --doc-type reference --title "ESP32-H2 NCP Firmware and ZNSP Protocol: Host Integration Reference"`
  - `docmgr doc relate --doc ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--.../reference/02-esp32-h2-ncp-firmware-and-znsp-protocol-host-integration-reference.md --file-note "..."`
  - `remarquee upload md --dry-run ... --remote-dir "/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE"`
  - `remarquee upload md ... --remote-dir "/ai/2026/02/02/0069-ANALYZE-PAST-FIRMWARE"`
