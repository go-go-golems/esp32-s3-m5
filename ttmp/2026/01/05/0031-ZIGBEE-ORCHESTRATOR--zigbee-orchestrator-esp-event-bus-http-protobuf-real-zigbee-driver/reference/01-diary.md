---
Title: Diary
Ticket: 0031-ZIGBEE-ORCHESTRATOR
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp-event
    - http
    - websocket
    - protobuf
    - nanopb
    - architecture
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: NCP-side changes we made during bringup and debugging (permit_join
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/partitions.csv
      Note: Persistent Zigbee storage partitions relevant to channel/debug
    - Path: 0031-zigbee-orchestrator/main/gw_console_cmds.c
      Note: |-
        Chronicles most console-facing changes (gw/zb/znsp/monitor verbs)
        Step 19 adds zb tclk/lke/nvram commands.
    - Path: 0031-zigbee-orchestrator/main/gw_registry.c
      Note: Device list keyed by IEEE (added while debugging rejoin/short churn)
    - Path: 0031-zigbee-orchestrator/main/gw_zb.c
      Note: Core Zigbee host logic and debugging changes (transactions
    - Path: thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Step 19 adds TCLK/LKE/NVRAM controls and fixes stub handlers.
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md
      Note: Document monitor-reset failure mode and mitigations
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh
      Note: |-
        Created while investigating environment constraints and reproducibility.
        Ensure H2 monitor uses --no-reset to avoid corrupting Grove ZNSP bus
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/13-dual-drive-and-capture.py
      Note: Dual capture + host command driver used for pairing runs
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/90-remarkable-pdf-only.sh
      Note: Offline PDF export when rmapi upload is blocked.
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-05-pairing-lke-on-224457/h2.log
      Note: 'Evidence: device announce + authorization timeout + short churn'
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-05-pairing-lke-on-224457/host.log
      Note: 'Evidence: host device announce + registry short change'
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T21:54:02-05:00
WhatFor: ""
WhenToUse: ""
---






# Diary

## Goal

Keep a step-by-step narrative of the work for ticket `0031-ZIGBEE-ORCHESTRATOR`, including what changed, why, what worked, what failed (with exact commands/errors), and how to review/validate.

## Step 1: Create ticket + initial archaeology

This step established the 0031 ticket workspace and gathered the “source of truth” documents and code locations needed to design a real Zigbee orchestrator that reuses the 0029 event-bus + HTTP + protobuf/WS shell.

The key outcome is a curated set of pointers (0029 firmware modules + 001 Zigbee bringup docs + 0030 patterns), and an initial analysis doc that frames the major design decision: host+NCP (S3+H2) is the practical “real Zigbee over Wi‑Fi” path.

### What I did
- Created ticket workspace: `docmgr ticket create-ticket --ticket 0031-ZIGBEE-ORCHESTRATOR ...`
- Created documents:
  - `docmgr doc add --ticket 0031-ZIGBEE-ORCHESTRATOR --doc-type analysis --title "Analysis: evolve 0029 mock hub into real Zigbee orchestrator"`
  - `docmgr doc add --ticket 0031-ZIGBEE-ORCHESTRATOR --doc-type reference --title "Diary"`
- Located 0029 firmware modules to reuse by listing and grepping:
  - `ls -la 0029-mock-zigbee-http-hub/main`
  - `rg -n "HUB_EVT|esp_event_post_to|/v1/|/v1/events/ws|nanopb|pb_encode|pb_decode" 0029-mock-zigbee-http-hub -S`
- Identified “real Zigbee” reference set from ticket 001 (host+H2 NCP) using doc search:
  - `docmgr doc list --ticket 001-ZIGBEE-GATEWAY`
  - `docmgr doc list --ticket 0029-HTTP-EVENT-MOCK-ZIGBEE`
  - `docmgr doc list --ticket 0030-CARDPUTER-CONSOLE-EVENTBUS`
- Related the most relevant code/doc files directly onto the 0031 analysis doc via:
  - `docmgr doc relate --doc ... --file-note "...:reason"`

### Why
- We want a “real product” Zigbee orchestrator, but 0029/0030 already proved a strong internal architecture for: event-driven modules, a control plane (HTTP/console), and an observability plane (protobuf over WS).
- Before coding, we need to decide what “real Zigbee” means on ESP hardware; the existing 001 work demonstrates the practical, Wi‑Fi-compatible approach: host (ESP32‑S3) + Zigbee NCP (ESP32‑H2).

### What worked
- `docmgr` ticket and doc creation worked and produced the expected workspace under `ttmp/2026/01/05/`.
- The 0029 firmware already contains a full nanopb toolchain and protobuf event stream implementation (good reuse candidate), specifically:
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_stream.c`
- The 001 Zigbee documentation already contains the relevant “real Zigbee” host+NCP constraints and debugging lessons (UART framing/console pin conflicts).

### What didn't work
- N/A (no implementation attempted in this step).

### What I learned
- 0029 is already “beyond mock”: it includes protobuf request/reply handling in `hub_http.c` and a bounded WS broadcaster. That makes it a strong shell to evolve into “real Zigbee”.
- The previously documented host+H2 NCP setup (ticket 001) is the best anchor for turning “mock Zigbee-shaped events” into “real Zigbee events” while keeping Wi‑Fi + HTTP on the host.

### What was tricky to build
- Avoiding scope creep: it’s tempting to jump straight into Zigbee cluster modeling, but the immediate goal is to preserve the bus-driven architecture and swap only the “device_sim” module for a “zigbee_driver”.

### What warrants a second pair of eyes
- Event taxonomy design: once we commit to event IDs/payload shapes (and protobuf schema), changes become costly across firmware + tooling. A quick review of naming and payload bounds (nanopb max sizes) is valuable.

### What should be done in the future
- Decide the 0031 host target (CoreS3 vs Cardputer) and lock in the Zigbee split (host+H2 NCP).
- Turn the analysis into an implementation plan (tasks) and start the 0031 firmware scaffold (fork 0029).

### Code review instructions
- Start here:
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md`
- Check related references (for constraints and reuse targets):
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_bus.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.md`

### Technical details
- Commands executed (high-signal):
  - `docmgr ticket create-ticket --ticket 0031-ZIGBEE-ORCHESTRATOR ...`
  - `docmgr doc add --ticket 0031-ZIGBEE-ORCHESTRATOR --doc-type analysis --title "..."`
  - `docmgr doc add --ticket 0031-ZIGBEE-ORCHESTRATOR --doc-type reference --title "Diary"`
  - `rg -n "HUB_EVT|esp_event_post_to|/v1/events/ws|nanopb" 0029-mock-zigbee-http-hub -S`

## Step 2: Draft the 0031 architecture/migration analysis

This step translated the “esp_event as an internal bus” Zigbee orchestrator design into a concrete plan that reuses 0029’s proven shell (bus + HTTP + protobuf/WS) and swaps only the simulator module for a real Zigbee integration module.

The outcome is a structured analysis document that highlights the primary decision point (single-chip Zigbee vs host+H2 NCP) and recommends the host+NCP path because it preserves Wi‑Fi HTTP control while using the already-documented 001 real Zigbee constraints.

### What I did
- Wrote the analysis document:
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md`
- Added a module diagram and a migration plan (fork 0029 → replace `hub_sim` with `zigbee_driver`).
- Defined a proposed event taxonomy (`GW_CMD_*` vs `GW_EVT_*`) and payload rules (bounded, copyable, `req_id` correlation).
- Captured a protobuf schema strategy (fork 0029’s schema, add Zigbee address/ZCL payload types with nanopb bounds).

### Why
- 0029 already solved the hard “firmware platform” problems: event-driven boundaries, request/reply over HTTP, and a protobuf WS event stream.
- The missing piece for the final orchestrator is a real Zigbee driver. We want to integrate it without destroying the architecture.

### What worked
- The analysis doc cleanly maps existing code locations to their future roles and enumerates which parts stay stable vs change.

### What didn't work
- N/A (analysis only).

### What I learned
- 0029’s existing event envelope and “EventId matches C enum” convention is directly reusable for Zigbee events, provided we bound payload sizes and keep ZCL payload handling disciplined.

### What was tricky to build
- Getting the boundary right between “bus handler code” and “Zigbee call sites” without yet picking the exact Zigbee SDK integration style (single-chip vs NCP). The analysis keeps both patterns, but commits to host+NCP as the practical default.

### What warrants a second pair of eyes
- The proposed event taxonomy and protobuf schema changes (especially payload max sizes and whether to model ZCL frames as opaque bytes or typed submessages).

### What should be done in the future
- Convert `tasks.md` into an executed implementation plan by checking off the “Decisions” section first (hardware target + MVP clusters), then start the 0031 firmware fork.

### Code review instructions
- Read the analysis doc end-to-end, then spot-check the referenced reuse targets in 0029:
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_bus.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_stream.c`

### Technical details
- N/A (no build/run performed in this step).

## Step 3: Confirm 0031 host hardware (Cardputer) + prep reMarkable upload

This step locked the previously-open hardware question: the 0031 host firmware will target **Cardputer (ESP32‑S3)**. That decision matters because it fixes the Wi‑Fi + HTTP platform and keeps the NCP split (ESP32‑H2 on Unit Gateway H2) as the “real Zigbee” path.

I also updated the 0031 docs to reflect this decision so the ticket stays self-consistent before implementation starts.

### What I did
- Marked task 1 as complete via `docmgr task check --ticket 0031-ZIGBEE-ORCHESTRATOR --id 1`.
- Updated the analysis doc open question to “Chosen: Cardputer”.

### Why
- Avoid implementation drift: pinning the target hardware early prevents building a plan around one board and implementing on another.

### What worked
- `docmgr` task tracking updated `tasks.md` as expected.

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- N/A (administrative updates only).

### What warrants a second pair of eyes
- N/A.

### What should be done in the future
- Start the firmware fork from 0029 once the remaining “Decisions” tasks are confirmed (NCP split + MVP clusters).

### Code review instructions
- Check the decision is reflected in:
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md`
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/tasks.md`

### Technical details
- Commands executed:
  - `docmgr task check --ticket 0031-ZIGBEE-ORCHESTRATOR --id 1`

## Step 4: Clarify Zigbee-specific terms + lock in remaining 0031 decisions

This step closed the remaining open design decisions (commit to host+H2 NCP immediately; MVP clusters OnOff + LevelControl; HTTP contract is async-first) and expanded the analysis doc with plain-language explanations of Zigbee-specific terms that were previously used as shorthand.

This is important because these terms (“scheduler alarms”, “ZCL database”, “full cluster support”, “202 async contract”) are the kind of implicit background knowledge that blocks onboarding if not written down.

### What I did
- Updated the 0031 analysis doc to include:
  - A glossary section explaining `esp_zb_scheduler_alarm*` and what it means to have a ZCL database / full cluster support.
  - A concrete explanation of the HTTP “contract style” options and why 202+async results is the default for Zigbee operations.
  - Updated the “open questions” section into “decided” values.
- Marked the corresponding decisions as complete in `tasks.md` (tasks 2 and 3).

### Why
- We want the 0031 ticket to be readable by someone who understands ESP-IDF but not Zigbee or coordinator patterns yet.
- Pinning decisions early keeps implementation aligned and reduces churn in event IDs, protobuf schema, and endpoint behavior.

### What worked
- The doc updates give a clearer “why this architecture” story and remove ambiguity about what we’re building next.

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- Keeping explanations accurate while still acknowledging the host+NCP split: some Zigbee SDK APIs exist only when the Zigbee stack runs on the same SoC (single-chip), but it’s still useful to explain them because they show the underlying threading model.

### What warrants a second pair of eyes
- The chosen HTTP contract defaults (202+async events) and whether we want *any* synchronous endpoints beyond registry reads.

### What should be done in the future
- Start the 0031 firmware fork and implement the first thin slice:
  - bring up host↔NCP link,
  - implement permit join,
  - forward device announce + attr report into the event bus + WS stream.

### Code review instructions
- Review the glossary and decision updates in:
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md`
- Confirm tasks reflect the decisions:
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/tasks.md`

### Technical details
- Files updated:
  - `.../analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md`
  - `.../tasks.md`

## Step 5: Write the detailed 0031 design document (Option 202 async HTTP)

This step produced the “single source of truth” design doc for how 0031 should be built: module boundaries, concurrency model, event taxonomy, protobuf/WS observability, and the decision to use an async HTTP contract (`202 Accepted` + `req_id` correlation via the WS event stream).

The aim is onboarding-grade clarity: a developer should be able to read the design doc and understand what components exist, how they communicate, and what the first implementation phases should deliver.

### What I did
- Created and wrote the design doc:
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/design-doc/01-design-cardputer-zigbee-orchestrator-esp-event-bus-http-202-protobuf-ws.md`
- Related the key reference implementations and constraints into the design doc using `docmgr doc relate` (0029 shell modules + 001 Zigbee NCP lessons + 0031 analysis doc).
- Updated the 0031 changelog to reflect that the design doc now exists.

### Why
- The design is non-trivial (two-chip Zigbee, async contracts, schema decisions). We want the rationale and module contracts frozen in writing before implementation starts.

### What worked
- The design doc now captures the end-to-end flow and the “why” behind `202` + event-stream correlation, without requiring Zigbee background knowledge.

### What didn't work
- N/A.

### What I learned
- Reusing 0029’s nanopb + WS patterns substantially reduces risk for 0031; the main remaining complexity is the host↔NCP integration and the device registry model.

### What was tricky to build
- Describing “Zigbee execution context” concepts accurately while still committing to host+NCP (where some single-chip Zigbee APIs like `esp_zb_scheduler_alarm*` are not central).

### What warrants a second pair of eyes
- The proposed event taxonomy and payload size bounds (especially ZCL payload and attr report representations).

### What should be done in the future
- Start Phase 0 implementation: fork 0029 into a new `0031-zigbee-orchestrator/` firmware project and keep the HTTP+WS pipeline intact while stubbing Zigbee.

### Code review instructions
- Read the design doc end-to-end:
  - `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/design-doc/01-design-cardputer-zigbee-orchestrator-esp-event-bus-http-202-protobuf-ws.md`
- Spot-check the referenced reuse targets:
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_stream.c`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/components/hub_proto/CMakeLists.txt`

### Technical details
- N/A (documentation only).

## Step 6: Implement Phase 0–2 host firmware (console + bus) and validate on Cardputer

This step started the actual 0031 firmware implementation by forking the 0029 project into a new host firmware project and then focusing on the first bring-up milestones: a reliable `esp_console` REPL over USB‑Serial/JTAG and a minimal `esp_event`-based application bus that we can drive from the console.

The goal was to have something that can be flashed and interacted with immediately, before introducing Wi‑Fi, HTTP, or Zigbee complexity.

### What I did
- Created a new firmware project directory by forking 0029 without its build artifacts:
  - `esp32-s3-m5/0031-zigbee-orchestrator/`
- Updated the firmware to start only:
  - `gw_bus_start()` (application `esp_event` loop)
  - `gw_console_start()` (USB‑Serial/JTAG REPL)
- Implemented a minimal bus and command set:
  - `GW_CMD_PERMIT_JOIN` command event (stubbed)
  - `GW_EVT_CMD_RESULT` notification event (stubbed)
  - `monitor on|off|status` console command to print bus traffic
  - `gw post permit_join <seconds> [req_id]` to generate events
  - `gw demo start|stop` to generate periodic events (stress test)
  - `version` command (prints project/version/IDF)
- Added `Kconfig.projbuild` knobs for:
  - bus queue size
  - monitor rate limiting
  - console prompt

### Why
- We want to prove the “developer control plane” first: interactive console + deterministic event bus.
- This becomes the foundation for later phases (Wi‑Fi, HTTP, WS, Zigbee NCP) without debugging everything at once.

### What worked
- Build succeeded:
  - `idf.py -C 0031-zigbee-orchestrator build`
- Flash to Cardputer succeeded on USB‑Serial/JTAG:
  - Cardputer port resolved as `/dev/ttyACM1` (by-id includes MAC `d0:cf:13:0e:be:00`)
  - `idf.py -C 0031-zigbee-orchestrator -p /dev/ttyACM1 flash`
- Interactive validation via `idf.py monitor` in tmux:
  - Console prompt appeared as `gw>`
  - Commands executed successfully:
    - `version`
    - `monitor on`
    - `gw post permit_join 5`
    - `gw demo start` / `gw demo stop`
  - With monitor enabled, we observed both the command and the stubbed result events printed to the console.

### What didn't work
- Initial build failed due to a missing include path for `esp_app_desc.h`:
  - Fix: added `esp_app_format` to `PRIV_REQUIRES` in `0031-zigbee-orchestrator/main/CMakeLists.txt`.
- A follow-up compile failed due to including non-existent/unused console headers:
  - Fix: removed `#include "esp_console_repl.h"` and `#include "esp_console_repl_usb_serial_jtag.h"` and relied on `esp_console.h` (matching the proven 0029 approach).

### What I learned
- The “USB‑Serial/JTAG console as default” guideline is the right baseline for Cardputer: the REPL is usable immediately and doesn’t fight UART pin usage.
- Even at Phase 0–2, having the `monitor` command is valuable: it gives us a “wiretap” view of the internal bus before we build WS streaming.

### What was tricky to build
- Keeping monitor output safe: printing from inside an event handler can overwhelm the REPL. A simple per-second rate limiter + drop counter keeps it usable.

### What warrants a second pair of eyes
- The event monitoring implementation in `gw_bus.c`: it’s intentionally simple, but console I/O can still be surprisingly slow; we should sanity-check that the rate limiter is sufficient and that we don’t accidentally deadlock on I/O in future handlers.

### What should be done in the future
- Remove/replace the leftover 0029 components from the fork (e.g., `components/hub_proto/`) to keep builds fast and avoid confusion.
- Start Phase 3 (Wi‑Fi) by porting the known-good Wi‑Fi console commands and ensuring logs don’t trample the prompt.

### Code review instructions
- Start with the new host project:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/app_main.c`
- Review the core bus + console:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
- Check config knobs:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/Kconfig.projbuild`
  - `esp32-s3-m5/0031-zigbee-orchestrator/sdkconfig.defaults`

### Technical details
- Commands (exact):
  - `rsync -a --delete --exclude build 0029-mock-zigbee-http-hub/ 0031-zigbee-orchestrator/`
  - `idf.py -C 0031-zigbee-orchestrator build`
  - `idf.py -C 0031-zigbee-orchestrator -p /dev/ttyACM1 flash`
  - tmux monitor:
    - `tmux new-session -d -s 0031gw -c ... "idf.py -p /dev/ttyACM1 monitor"`
    - sent commands: `version`, `monitor on`, `gw post permit_join 5`, `gw demo start`, `gw demo stop`, `monitor status`

## Step 7: Investigate real `permit_join` (host + H2 NCP) and identify why it is currently stubbed

This step focused on understanding what it would take to replace the stubbed `GW_CMD_PERMIT_JOIN` in the 0031 host firmware with a real Zigbee “open the network for joining” operation using the **host + H2 NCP** split. The main goal was to find the actual request/notify path in Espressif’s host/NCP protocol and to identify the missing link preventing a real implementation.

The key outcome is that the **H2 NCP side is currently missing the request handler for permit-join**, even though it already emits permit-join status notifications. So “unstubbing `permit_join`” is an end-to-end change: the host must send the request, and the NCP firmware must implement it.

### What I did
- Traced the host/NCP protocol IDs and where they are handled:
  - Host-side protocol ID for permit-join:
    - `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/priv/esp_host_zb.h`
      - `ESP_ZNSP_NETWORK_PERMIT_JOINING` (0x0005)
- Traced the NCP dispatch table and verified the request handler is missing:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
    - `ncp_zb_func_table` contains `{ESP_NCP_NETWORK_PERMIT_JOINING, NULL}`
    - `esp_ncp_zb_output()` returns `ESP_ERR_INVALID_ARG` when `set_func` is `NULL`
- Verified NCP does already emit permit-join status notifications:
  - Same file (`esp_ncp_zb.c`), `esp_zb_app_signal_handler()` case `ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS`
    - Sends a notify frame with `ncp_header.id = ESP_NCP_NETWORK_PERMIT_JOINING` and payload `uint8_t seconds`
- Identified the likely correct Zigbee API that the NCP handler should call:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-lib/include/esp_zigbee_core.h`
    - `esp_err_t esp_zb_bdb_open_network(uint8_t permit_duration);`

### Why
- The host firmware can’t “make the network joinable” by itself in the host+NCP split; it must send a request to the H2 NCP.
- The existing notifications suggest the protocol was designed for this feature, but the handler wasn’t implemented on the NCP side (at least in the checked-out SDK version).

### What worked
- We now have a concrete and actionable explanation for the stub:
  - Host knows the request ID (0x0005).
  - NCP already knows how to notify status for (0x0005).
  - NCP is missing the request handler for (0x0005).

### What didn't work
- Attempting to “unstub” purely on the host side would still fail (NCP returns `ESP_ERR_INVALID_ARG`) until the NCP firmware is patched and reflashed.

### What I learned
- In this SDK version, not all host/NCP protocol features are symmetric (notify exists, request handler missing).
- For this kind of two-chip system, it’s crucial to treat “unstubbing” as a host+NCP change and to validate it with real hardware pairing.

### What was tricky to build
- The host example code and headers are easy to misread as fully-featured Zigbee APIs; in reality, they are a thin protocol layer and some parts are stubbed or incomplete, so the safest path is to identify the protocol IDs and verify both directions (request + notify) exist.

### What warrants a second pair of eyes
- Confirming the correct NCP-side API call and expected behavior:
  - Is `esp_zb_bdb_open_network(seconds)` the right call for the coordinator’s “permit joining” semantics in this SDK/version?
  - What should happen when `seconds == 0` (close network) and what status should be returned to host?

### What should be done in the future
- Patch the H2 NCP firmware (`esp-zigbee-ncp`) to implement `ESP_NCP_NETWORK_PERMIT_JOINING`.
- Integrate the host-side permit-join request into the 0031 firmware and surface:
  - console command
  - bus event
  - bus monitoring output
- Validate with a real Zigbee device (power switch) by opening the network and observing `DEVICE_ANNCE`.

### Code review instructions
- Review the NCP dispatch table and permit-join notify emission:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
- Review the host protocol ID mapping:
  - `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/priv/esp_host_zb.h`
- Review the Zigbee API candidate on the NCP side:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-lib/include/esp_zigbee_core.h`

### Technical details
- Key grep commands:
  - `rg -n "PERMIT_JOIN" /home/manuel/esp/esp-zigbee-sdk -S`
  - `rg -n "NETWORK_PERMIT" /home/manuel/esp/esp-zigbee-sdk -S`

## Step 8: Add host↔NCP Zigbee host layer in 0031 and prepare the H2 NCP permit-join implementation

This step integrated Espressif’s host-side NCP protocol layer into the 0031 Cardputer firmware so we can start issuing real Zigbee operations (starting with permit-join) through the internal `GW_EVT` bus and console. In parallel, it patched the H2 NCP implementation to add the missing permit-join request handler and built the updated NCP firmware image.

The most important discovery during validation is that the Grove UART link is electrically alive (we see **ESP32‑H2 ROM boot logs** on the bus), but the H2 is not currently speaking the expected ZNSP frames to the host. This means we need to ensure the H2 is flashed with the NCP firmware and ideally monitored over its own USB/JTAG console.

### What I did
- Vendored the Zigbee host protocol component into the 0031 firmware as `zb_host`:
  - `esp32-s3-m5/0031-zigbee-orchestrator/components/zb_host/`
  - Fix: corrected an off-by-one loop in the upstream host notify dispatcher (`esp_zb_stack_main_loop`) to avoid out-of-bounds access.
- Added a minimal public header for sending host protocol requests from our app code:
  - `esp32-s3-m5/0031-zigbee-orchestrator/components/zb_host/include/esp_host_zb_api.h`
- Implemented a 0031 Zigbee integration module:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.h`
  - Starts host+NCP stack in a dedicated task and exposes an async `permit_join` request via a small worker queue.
- Implemented the required Zigbee signal hook (`esp_zb_app_signal_handler`) and forwarded key signals into the GW bus:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb_app_signal.c`
  - Posts:
    - `GW_EVT_ZB_PERMIT_JOIN_STATUS`
    - `GW_EVT_ZB_DEVICE_ANNCE`
- Wired the existing GW bus command handler to call the Zigbee module instead of returning a stubbed result:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c`
- Set host bus UART pins to match the Cardputer Grove wiring assumption:
  - `esp32-s3-m5/0031-zigbee-orchestrator/sdkconfig.defaults`
    - `CONFIG_HOST_BUS_UART_RX_PIN=1` (G1)
    - `CONFIG_HOST_BUS_UART_TX_PIN=2` (G2)
    - `CONFIG_HOST_BUS_UART_NUM=1`, `CONFIG_HOST_BUS_UART_BAUD_RATE=115200`
- Hardening fixes discovered during on-device validation:
  - Added a response timeout (2s) to `zb_host`’s `esp_host_zb_output()` so commands can’t hang forever if the NCP doesn’t respond.
  - Changed `zb_host`’s `esp_host_main_task` to treat framing/parse errors on UART input as non-fatal (drop and continue), because boot ROM logs/noise can appear on the bus.
- Patched the H2 NCP Zigbee component to implement the missing permit-join request handler and fixed a matching off-by-one bug:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
    - Added `esp_ncp_zb_permit_joining_fn()` calling `esp_zb_bdb_open_network(seconds)`
    - Updated `ncp_zb_func_table` to route `ESP_NCP_NETWORK_PERMIT_JOINING` to the new function
    - Fixed loop bounds in `esp_ncp_zb_output()` to avoid out-of-bounds access
- Built the patched H2 NCP example firmware:
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp build`

### Why
- 0031 must be able to send “open network” and observe “device announce” in order to validate real Zigbee pairing before any HTTP/UI work begins.
- The SDK’s NCP side was missing the permit-join request handler, so the only way to make `permit_join` real is to patch + rebuild + reflash the H2 NCP firmware.

### What worked
- The 0031 firmware builds and flashes cleanly on Cardputer and the console remains usable:
  - `idf.py -C 0031-zigbee-orchestrator build`
  - `idf.py -C 0031-zigbee-orchestrator -p /dev/ttyACM1 flash`
- We can issue `gw post permit_join 30` and it now routes into the Zigbee module and emits a `GW_EVT_CMD_RESULT` (success or timeout/fail), rather than returning a stubbed OK.
- The host UART link is physically active: we captured ESP32-H2 ROM boot banners over the Grove UART connection (meaning RX/TX/GND are correct at least for UART0).

### What didn't work
- The H2 is not currently responding with valid ZNSP frames on the Grove UART bus:
  - The host sees H2 ROM boot logs (ASCII “ESP-ROM:esp32h2-...”) which the SLIP/ZNSP framing layer treats as invalid packets.
  - Without the H2 running the NCP firmware, host requests time out or produce meaningless status bytes.
- The H2 is not present as a `/dev/ttyACM*` device at the moment, so we could not flash it directly from this session (Cardputer USB-JTAG is present as `/dev/ttyACM1`, but H2 USB is not connected).

### What I learned
- The host-side ZNSP framing layer must be resilient to UART noise (boot ROM logs, partial frames). Treating input parse errors as fatal makes the system fragile.
- To validate real Zigbee, we must be able to flash and (ideally) monitor the H2 independently over USB/JTAG; relying on the bus alone makes it hard to tell “wrong firmware” vs “wrong pins” vs “reset loop”.

### What was tricky to build
- Static library extraction order caused a surprising link failure for `esp_zb_app_signal_handler`. Fix was to force-link the handler object so the `zb_host` component can always resolve it.
- The upstream host/NCP example code contains at least two off-by-one loops in dispatch paths; these had to be corrected to avoid hard-to-debug memory corruption.

### What warrants a second pair of eyes
- The `zb_host` hardening choices:
  - `esp_host_zb_output()` timeout value (2s) and whether we should make it configurable via Kconfig.
  - Treating all input framing errors as non-fatal (right for robustness, but we should ensure it doesn’t hide real protocol bugs).
- The H2 reset/boot-log behavior on the bus: it’s unclear if the H2 is rebooting continuously, or just rebooting when we interact; we should validate power and firmware on the H2 side.

### What should be done in the future
- Plug the H2 into USB so it appears in `/dev/serial/by-id/`, then:
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp -p <H2PORT> flash monitor`
- Confirm the H2 NCP firmware uses the same UART pins as the Grove connection and uses USB‑Serial/JTAG (not UART0) for its console logs to keep the bus clean after boot.
- After the H2 speaks ZNSP correctly, validate pairing with the test power switch:
  - `gw post permit_join 180`
  - put switch into pairing mode
  - observe `GW_EVT_ZB_DEVICE_ANNCE` events.

### Code review instructions
- 0031 Zigbee integration:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb_app_signal.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c`
- Vendored host protocol layer + robustness tweaks:
  - `esp32-s3-m5/0031-zigbee-orchestrator/components/zb_host/src/esp_host_zb.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/components/zb_host/src/esp_host_main.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/components/zb_host/src/esp_host_frame.c`
- NCP permit-join patch (external SDK):
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`

### Technical details
- Commands (exact):
  - Cardputer build/flash:
    - `idf.py -C 0031-zigbee-orchestrator build`
    - `idf.py -C 0031-zigbee-orchestrator -p /dev/ttyACM1 flash`
  - tmux monitor (Cardputer):
    - `tmux new-session -d -s 0031zb -c ... "idf.py -C 0031-zigbee-orchestrator -p /dev/ttyACM1 monitor"`
    - sent commands: `monitor on`, `gw post permit_join 30`
- H2 NCP build:
    - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp build`

## Step 9: Flash both devices and validate real host↔NCP `permit_join` end-to-end

This step completed the end-to-end validation loop with real hardware connected: Cardputer as the host and Unit Gateway H2 as the NCP. The goal was to prove that our host firmware can bring up the host bus, that the H2 can form a coordinator network, and that `gw post permit_join <seconds>` results in a real Zigbee “open network” action on the H2 (with both a request/response and a permit-join status notification).

The key fix that made the system stable was ensuring the NCP does **not** pass ephemeral request-buffer pointers into Zigbee stack initialization (`esp_zb_init`). After copying the config into static storage before calling `esp_zb_init`, the earlier H2 crash in `zb_bufpool_storage_allocate()` disappeared.

### What I did
- Confirmed both devices are present over USB Serial/JTAG:
  - H2: `/dev/ttyACM0` (by-id includes `48:31:b7:ca:45:5b`)
  - Cardputer: `/dev/ttyACM1` (by-id includes `d0:cf:13:0e:be:00`)
- Flashed the patched NCP firmware to the H2 and started a monitor in tmux:
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 flash`
  - `tmux new-session -d -s h2ncp "idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 monitor"`
- Updated the host firmware behavior to improve robustness during bring-up:
  - Added a host-side delay before initializing the host bus so we can flush the H2 ROM boot banner before SLIP framing begins.
  - Temporarily disabled sending the primary channel mask command from the host (this was part of the failing sequence during earlier crash investigation).
- Flashed the updated 0031 host firmware to Cardputer and started a monitor in tmux:
  - `idf.py -C esp32-s3-m5/0031-zigbee-orchestrator -p /dev/ttyACM1 flash`
  - `tmux new-session -d -s 0031zb "idf.py -C esp32-s3-m5/0031-zigbee-orchestrator -p /dev/ttyACM1 monitor"`
- Validated `permit_join` end-to-end from the Cardputer console:
  - `monitor on`
  - `gw post permit_join 60`

### Why
- We needed to validate the entire host+NCP path (console → GW bus → ZNSP request → NCP Zigbee stack → ZNSP notify → GW bus monitor) on the actual connected hardware, before proceeding to device join and higher-level orchestration work.

### What worked
- H2 successfully formed a Zigbee coordinator network (e.g. channel 13) and entered network steering:
  - H2 logs showed: “Start network formation”, “Formed network successfully … Channel:13”, “Network steering started”.
- Cardputer host correctly received ZNSP notifies and surfaced them via our `esp_zb_app_signal_handler`:
  - Host logs showed `ZB signal: FORMATION status=ESP_OK`.
  - Host logs showed `ZB permit-join status: seconds=180` (from NCP’s automatic open window) and later `seconds=60` (from the explicit command).
- `gw post permit_join 60` produced:
  - a successful response on the host: `GW_EVT_CMD_RESULT ... status=0 (ESP_OK)`
  - a corresponding notify event: `GW_EVT_ZB_PERMIT_JOIN_STATUS seconds=60`
- The H2 no longer crashed in `zb_bufpool_storage_allocate()` once we avoided passing transient request-buffer pointers into `esp_zb_init`.

### What didn't work
- Prior to the NCP-side config-copy fix, the H2 intermittently crashed during the early bring-up command sequence (notably around `esp_zb_set_primary_network_channel_set` / `esp_zb_start`), which also polluted the UART bus with ROM banners and caused the host to mis-parse responses.

### What I learned
- For host↔NCP protocols, any “struct payload passed as pointer” used to initialize long-lived subsystems must be treated as suspect; copying into stable storage is a safe default unless the API guarantees deep-copy semantics.
- Once the H2 is stable, the host can rely on ZNSP request/response semantics and treat permit-join as a normal bus command with deterministic completion + asynchronous status notify.

### What was tricky to build
- It was easy to misattribute the crash to “UART noise” or “SLIP decode” issues because the symptom on the host looked like framing corruption; the actual root cause was an NCP-side crash, which then produced ROM ASCII on the bus and cascaded into parsing failures.

### What warrants a second pair of eyes
- The NCP-side lifetime fix for `esp_zb_init` input:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
    - confirm this matches Espressif’s expectations (i.e. that `esp_zb_init` might not deep-copy config).
- The decision to temporarily disable host-side `esp_zb_set_primary_network_channel_set` until we’re confident the bring-up sequence is stable across resets.

### What should be done in the future
- Re-enable primary channel mask configuration (likely `1<<13`) once we’ve confirmed stability.
- Test “real join” with the power switch:
  - keep join open: `gw post permit_join 180`
  - put the switch into pairing mode (factory reset + join sequence)
  - watch for `GW_EVT_ZB_DEVICE_ANNCE` on the host console.

### Code review instructions
- Host changes:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb_app_signal.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c`
- NCP stability fixes (external SDK):
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`

### Technical details
- tmux sessions used:
  - `h2ncp`: H2 NCP `idf.py monitor` on `/dev/ttyACM0`
  - `0031zb`: Cardputer host `idf.py monitor` on `/dev/ttyACM1`

## Step 10: Add a real ZCL On/Off command path (host console → NCP)

This step turned the “device joined” event into an actionable, user-visible capability: we can now send a basic **ZCL On/Off** command to a joined device (power plug) from the Cardputer console and see the NCP accept and transmit it.

This is still a thin slice: we are not doing endpoint discovery, ZCL interview, or state reads yet. The intent is to prove the request/response path for ZCL/APS traffic via the host↔NCP protocol before layering on higher-level abstractions.

### What I did
- Added a new GW command event and payload:
  - `GW_CMD_ONOFF` and `gw_cmd_onoff_t` in `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_types.h`
- Routed the command through the bus into the Zigbee worker:
  - Registered a handler for `GW_CMD_ONOFF` in `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c`
  - Implemented `gw_zb_request_onoff()` and a `ZNSP_ZCL_WRITE` request in `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
- Added an interactive console command:
  - `gw post onoff <short_addr> <ep> <on|off|toggle> [req_id]` in `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`

### Why
- `permit_join` + `DEVICE_ANNCE` proves device commissioning, but not “real control”.
- On/Off is the smallest real cluster interaction that’s meaningful to validate with a commodity plug.

### What worked
- End-to-end command worked on the wired host+H2 setup:
  - Host console:
    - `monitor on`
    - `gw post onoff 0xa04d 1 toggle`
    - expected: `GW_EVT_CMD_RESULT ... status=0 (ESP_OK)`
  - H2 monitor logged the received command fields:
    - `ESP_NCP_ZB: addr a04d, dst_endpoint 1, src_endpoint 1, address_mode 2, profile_id 104, cluster_id 06, cmd_id 02, direction 00`

### What didn't work
- N/A (no new failures encountered in this step).

### What I learned
- The host↔NCP protocol already supports a ZCL/APS write primitive (`ESP_NCP_ZCL_WRITE`), so we can use it for MVP control commands even before we build a full “device model” layer.

### What was tricky to build
- The `ESP_NCP_ZCL_WRITE` payload is a packed struct expected by the NCP implementation; we must keep the layout stable (and treat it as part of the protocol contract).

### What warrants a second pair of eyes
- Validate the `ESP_NCP_ZCL_WRITE` payload struct layout and constants in:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c` (`esp_ncp_zb_zcl_write_fn`)

### What should be done in the future
- Add endpoint discovery / simple “try endpoints 1..3” helper for devices that don’t use endpoint 1.
- Add attribute reads (OnOff attribute 0x0000) to confirm state and not rely on visual observation.

### Code review instructions
- Start here:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`

### Technical details
- Key console commands:
  - `monitor on`
  - `gw post onoff 0xa04d 1 toggle`

## Step 11: Add an in-memory device registry + `gw devices` listing

This step added the first “operator ergonomics” feature: being able to ask the firmware what devices it has seen join/rejoin, instead of relying on scrolling monitor logs.

This is intentionally a small, fixed-size in-memory registry (no NVS persistence yet) so we can iterate quickly while building out interview and control paths.

### What I did
- Implemented a fixed-size registry component:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.h`
- Updated bus wiring so every `GW_EVT_ZB_DEVICE_ANNCE` updates the registry:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c` (`on_evt_zb_device_annce`)
- Added a console command to dump the registry:
  - `gw devices` in `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
- Fixed build integration:
  - Added `esp_timer` to `PRIV_REQUIRES` in `esp32-s3-m5/0031-zigbee-orchestrator/main/CMakeLists.txt`

### Why
- “What joined?” is the first question you ask when debugging Zigbee commissioning.
- The join event provides short address + IEEE + capability; that’s enough to make follow-up control tests reproducible.

### What worked
- After flashing and restarting monitors, the command is available:
  - `gw devices` prints `devices: N` and each device line including short address, IEEE, capability, and a seen counter.

### What didn't work
- Right after a reboot, the registry starts empty (expected) until devices re-announce or join again.

### What I learned
- The cleanest place to build a registry is in the bus loop (single writer), but console reads are cross-task; snapshotting under a short critical section keeps it safe without overengineering.

### What was tricky to build
- Making the registry safe to read from the console task while the event loop updates it; we avoid printing under the lock by snapshotting.

### What warrants a second pair of eyes
- Concurrency: confirm the critical-section usage in `gw_registry_snapshot()` is acceptable on ESP32-S3 and doesn’t block longer than necessary.

### What should be done in the future
- Persist registry entries into NVS (and clear them when forming a new network).
- Add per-device “endpoint list” and “clusters” once interview is implemented.

### Code review instructions
- Review:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`

### Technical details
- Flash/monitor workflow used:
  - stop existing monitors (they lock the port), then `idf.py -C esp32-s3-m5/0031-zigbee-orchestrator -p /dev/ttyACM1 flash`
  - `tmux new-session -d -s 0031zb "idf.py -C esp32-s3-m5/0031-zigbee-orchestrator -p /dev/ttyACM1 monitor"`
  - `gw devices`

## Step 12: Improve “device keeps rejoining” debuggability (short changes + ZDO update signals) + add `znsp req` console tool

This step addressed a practical debugging issue observed during real device testing: the plug appears to “keep rejoining” and therefore shows a different short address each time. This is normal Zigbee behavior (the IEEE is stable; the 16-bit short can change), but we needed better visibility to confirm what’s happening.

The outcome is twofold: (1) richer logs for join/leave/update/auth signals, and (2) a safe(ish) low-level console tool (`znsp req`) that sends framed host↔NCP requests without needing to write arbitrary bytes onto the UART and potentially desync SLIP framing.

### What I did
- Added additional Zigbee signal logs in:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb_app_signal.c`
    - `ESP_ZB_ZDO_SIGNAL_LEAVE_INDICATION`
    - `ESP_ZB_ZDO_SIGNAL_DEVICE_UPDATE`
    - `ESP_ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED`
- Improved the device registry to explicitly log short-address changes for the same IEEE:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.c` (logs `0xOLD -> 0xNEW`)
- Added a mutex-protected “ZNSP output” helper so interactive debug commands don’t race the Zigbee worker:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.h`
- Added a new console command:
  - `znsp req <id> [bytes...]` in `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`

### Why
- The H2 logs show “New device commissioned or rejoined (short: 0x....)” with varying shorts; we want the host firmware to make it obvious that:
  - IEEE stays the same (stable identity)
  - short can change (routing address)
  - we may also see leave/update/auth signals that explain rejoin reasons
- A low-level command helps debug host↔NCP protocol issues without writing “raw bytes” onto the UART (which can corrupt SLIP framing state).

### What worked
- The `znsp` command is visible in `help`, and a simple request succeeds:
  - `znsp req 0x0005 0x00` prints a 1-byte response status (ex: `00`).
- After a device announce, `gw devices` shows the device entry and updates it on later announces.

### What didn't work
- N/A.

### What I learned
- The most useful debugging invariant is: **IEEE is stable; short is not**. The registry’s job is to map IEEE → “latest short”.

### What was tricky to build
- Avoiding request concurrency: `esp_host_zb_output()` is not something we want multiple tasks calling at once. Adding a mutex and routing both the worker and `znsp` through it prevents interleaved frames.

### What warrants a second pair of eyes
- Verify the log verbosity is acceptable and doesn’t spam in normal operation (especially if devices are chatty).

### What should be done in the future
- Add a command to send ZCL OnOff by IEEE (`onoff_ieee`) that uses the registry mapping and avoids “copy/paste latest short”.
- Add an attribute read to confirm OnOff state instead of relying on observation.

### Code review instructions
- Review:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb_app_signal.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`

### Technical details
- Example debug commands:
  - `znsp req 0x0005 0x00`
  - `gw devices`

## Step 13: Print IEEE address on the H2 (NCP) join log

This step makes the H2 side logs just as useful as the host side logs during commissioning: when a device joins/rejoins, the NCP now prints both the transient 16-bit short address and the stable IEEE address.

### What I did
- Updated the NCP-side Zigbee app signal handler to print IEEE+capability on `ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE`:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
    - log now includes: `short`, `ieee`, and `cap`
- Rebuilt and flashed the H2 firmware:
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp build`
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 flash`
- Validated by observing the new log line in the H2 monitor:
  - `New device commissioned or rejoined (short: 0x81b1, ieee: 28:2c:02:bf:ff:e6:98:70, cap: 0x8e)`

### Why
- The short address is not stable; having IEEE in the NCP log makes it immediately obvious when it’s the same physical device rejoining vs a different device.

### What worked
- The log appears in the H2 monitor and matches the IEEE printed by the host when it receives the corresponding `DEVICE_ANNCE`.

### What didn't work
- N/A.

### What I learned
- The NCP already has the IEEE available in the announce params; this was just missing observability.

### What was tricky to build
- N/A (simple logging change).

### What warrants a second pair of eyes
- N/A.

### What should be done in the future
- Consider logging the “device update” status code on the NCP side as well (secured rejoin vs unsecured join), to make rejoin reasons clearer without needing host logs.

### Code review instructions
- Review:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c` (`ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE` case)

### Technical details
- N/A.

## Step 14: Add rejoin/authorization logs on H2 + allow `permit_join 0` from host console

This step focused on diagnosing a real-world symptom: the plug appears to “keep joining” and its short address changes frequently. That is not what you expect in a stable network; it usually indicates either coordinator resets, RF/link instability, or a security/join completion problem.

To make that diagnosable without guesswork, I extended the NCP’s logs to include the Zigbee “update/authorized/leave” signals (with IEEE + status codes), and I removed an unnecessary limitation in the host console so we can explicitly close permit-join from the REPL (`seconds=0`).

### What I did
- Host: allow closing permit-join from the console:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
    - `gw post permit_join 0` is now valid (prints “seconds=0 closes” in help)
- NCP: add logs for “why is this device rejoining?” signals:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
    - `ESP_ZB_ZDO_SIGNAL_DEVICE_UPDATE` (status + IEEE + short)
    - `ESP_ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED` (type/status + IEEE + short)
    - improve leave log to include `rejoin` + IEEE
- Rebuilt/validated builds:
  - `idf.py -C esp32-s3-m5/0031-zigbee-orchestrator build`
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp build`
- Flashed both devices after stopping monitors (ports must not be locked):
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 flash`
  - `idf.py -C esp32-s3-m5/0031-zigbee-orchestrator -p /dev/ttyACM1 flash`

### Why
- Permit-join being “open” is about whether the coordinator allows *new joins*; it does not by itself cause a device to continuously change addresses.
- Frequent re-announces + changing short addresses usually means the device is repeatedly rejoining due to instability or incomplete security commissioning; we need signal-level logs to confirm.

### What worked
- Host REPL can now close joining explicitly:
  - `gw post permit_join 0`
- NCP builds clean again after removing duplicate `case` labels (the upstream file already had a “do nothing” case list that included the same signal IDs).

### What didn't work
- Initial NCP build failed due to duplicate `case` labels for `DEVICE_UPDATE` and `DEVICE_AUTHORIZED` until the old “do nothing” cases were removed.

### What I learned
- The esp-zigbee-ncp example’s signal handler already enumerates many signals in a grouped “ignore” list; adding detailed handling requires removing duplicates from that list.

### What was tricky to build
- Keeping the NCP switch statement consistent: this file mixes concrete handling for a few signals with a large “ignored signals” fallthrough list, so it’s easy to accidentally introduce duplicate case values.

### What warrants a second pair of eyes
- Confirm which signal ID is actually emitted for leave indication in this NCP build (`ESP_ZB_ZDO_SIGNAL_LEAVE` vs `ESP_ZB_ZDO_SIGNAL_LEAVE_INDICATION`) so we don’t miss meaningful leave/rejoin info.

### What should be done in the future
- If the plug keeps rejoining:
  - verify H2 is not rebooting (watch for ROM banners / “Formed network successfully” repeating)
  - consider switching Zigbee to a less congested channel (e.g. 20/25 vs 13 near Wi‑Fi ch1)
  - use the new NCP “device update/authorized” logs to determine whether this is secured rejoin vs unsecured join loops.

### Code review instructions
- Host console change:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
- NCP log changes:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`

## Step 15: Validate real join + add device registry + add low-level ZNSP request tooling

This step moved from “permit-join seems to work” to “we can observe actual device joins and interact with joined devices reliably from the host console”. The two biggest gaps were (1) not having an easy, stable way to refer to devices when the short address changes and (2) not having a safe way to poke the NCP at the byte/protocol level when debugging.

The outcome is a thin but practical developer workflow: put the coordinator in permit-join mode, observe `DEVICE_ANNCE` with IEEE, list devices keyed by IEEE, and send ZCL OnOff to a chosen short address/endpoint. When that fails, we can issue raw ZNSP requests (`znsp req ...`) and compare host vs NCP logs.

### What I did
- Host: added an in-memory registry keyed by IEEE and a console command to view it:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.h`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c` (`gw devices`)
  - Registry tracks: IEEE, last short, announce count, last-seen tick.
- Host: added console commands for ZCL OnOff (short-address based) and basic “happy path” join testing:
  - `gw post permit_join <seconds>`
  - `gw post onoff <short> <ep> on|off|toggle [req_id]`
- Host: added a “low-level escape hatch” for protocol debugging:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c` (`znsp req <id> <hex bytes...>`)
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c` (a mutex around host↔NCP transactions, so concurrent console commands don’t interleave SLIP frames).
- NCP: improved announce logging to include IEEE (so we can correlate host vs NCP view of the same device):
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
- Validated end-to-end with a test power plug:
  - Put coordinator in permit-join (`gw post permit_join 180`)
  - Put plug in pairing mode
  - Observed the NCP log “commissioned or rejoined” and the host bus event `GW_EVT_ZB_DEVICE_ANNCE`
  - Sent `onoff` toggles and observed the plug switching (intermittently; see Step 16).

### Why
- Zigbee short addresses are not stable identifiers across rejoins; IEEE is. A registry keyed by IEEE is the minimal “coordinator” feature needed for debugging and later persistence.
- When the system misbehaves (timeouts, incorrect frames, NCP rejects), having a raw request tool lets us isolate whether the issue is in our high-level wrappers or in the wire protocol/timing.

### What worked
- Real join visibility on both sides (example logs captured during testing):
  - NCP:
    - `ESP_NCP_ZB: New device commissioned or rejoined (short: 0xa04d)`
  - Host:
    - `[gw] GW_EVT_ZB_DEVICE_ANNCE (201) short=0xa04d ieee=28:2c:02:bf:ff:e6:98:70 cap=0x8e`
- Permit-join request/response and status updates are consistent:
  - Host:
    - `[gw] GW_CMD_PERMIT_JOIN (1) req_id=3 seconds=180`
    - `[gw] GW_EVT_CMD_RESULT (100) req_id=3 status=0`
  - NCP/host both emit periodic permit-join status events with the remaining seconds.
- OnOff commands sometimes work and physically toggle the plug, confirming the core “host→NCP→device” path is viable.

### What didn't work
- Device stability is not yet good: the plug appears to rejoin repeatedly, and its short address changes. This makes “send command by short” unreliable and is the central issue investigated next (Step 16).

### What I learned
- The IEEE is the only stable primary key we can assume (so registry must be IEEE-first).
- Debugging ZNSP/SLIP issues without a raw request tool is slow; with `znsp req ...`, we can rapidly answer “is the NCP receiving what we think we sent?”

### What was tricky to build
- Preventing concurrent console commands from interleaving ZNSP transactions on the same UART required strict serialization (one request in flight). Without this, occasional “random” failures appear that are actually self-inflicted framing corruption.

### What warrants a second pair of eyes
- The ZCL OnOff wrapper: confirm endpoint/cluster/command IDs and payload encoding are correct for common devices, and ensure we’re not accidentally mixing ZCL “frame control” bits across device types.

### What should be done in the future
- Add “by IEEE” addressing for user commands (resolve IEEE → current short), so users never need to type short addresses.
- Decide how to persist the registry (NVS vs flash filesystem) once rejoin stability is solved.

### Code review instructions
- Start with the registry flow:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb_app_signal.c` (where announce events are decoded and forwarded into the registry)
- Review the console verbs:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
- Review the serialized host↔NCP transaction path:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`

### Technical details
- Commands used:
  - `gw post permit_join 180`
  - `gw devices`
  - `gw post onoff 0xa04d 1 toggle`
  - `znsp req ...` (manual request injection for debugging)

## Step 16: Investigate “rejoin loop” + attempt to move coordinator off channel 13 (ch20/ch25)

This step investigated the main observed failure mode: the plug appears to repeatedly rejoin and its short address changes frequently, which makes the system feel like it’s stuck in “always joining”. The primary hypothesis was RF/channel interference (Zigbee ch13 is close to Wi‑Fi ch1), so the first concrete mitigation was to add a way to change the coordinator channel to something like 20 or 25 and re-test.

The outcome so far is “inconclusive but actionable”: we can now set and persist channel masks from the host, and the NCP accepts them, but the network formation still reports it is on channel 13. We also found (and fixed) a hard crash caused by applying channel masks before the Zigbee stack is initialized on the NCP.

### What I did
- Host: added “zb channel/mask” console commands and NVS persistence:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
    - `zb status` (show current configured masks)
    - `zb ch 20|25` (set a single-channel primary mask)
    - `zb mask <hex>` (set full channel mask)
    - `zb reboot` (request NCP restart / re-form)
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
    - stores channel config in NVS namespace `gw_zb`
    - sends both “channel mask set” and “primary channel set” requests to the NCP.
- NCP: fixed a crash by deferring channel config application until the Zigbee stack is initialized:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
    - Store requested masks in static variables when received early
    - Apply them only after `esp_zb_init()` (tracked with a boolean gate)
    - Apply again right before network formation in the BDB “first start / reboot” signal path.
- Added explicit NCP logs for channel config application (so we can see whether it was accepted), for example:
  - `apply channel mask: 0x02000000 (ESP_OK)`
  - `apply primary channel mask: 0x02000000 (ESP_OK)`

### Why
- If rejoin instability is caused by RF conditions, moving the coordinator to a cleaner channel should reduce rejoin frequency and make addressing stable enough to proceed with higher-level features (registry persistence, HTTP API, WS stream).

### What worked
- The host now has a developer-grade interface for channel configuration and preserves it across reflashes (NVS-backed), which is critical for field debugging.
- The NCP no longer crashes if the host sets channel masks early; it safely defers and later applies them when safe.

### What didn't work
- Even after the NCP logs that the channel masks were applied successfully, the NCP still forms a network on channel 13:
  - `Formed network successfully ... Channel:13`
  - This indicates either:
    - the channel config APIs are not affecting formation in this NCP build, or
    - the stack is restoring channel config from persistent Zigbee storage (e.g. `zb_fct` / `zb_storage`) and overriding our requested masks, or
    - we are setting the correct values but at the wrong time relative to BDB formation.

### What I learned
- Timing matters: applying channel config before Zigbee init can crash the NCP; the safe place is after `esp_zb_init()` but before formation/start.
- “Accepted” does not mean “effective”: the NCP APIs can return OK even if some other config source overrides the values at formation time.

### What was tricky to build
- Coordinating two independent state machines:
  - host wants to push config at boot (so it’s always consistent)
  - NCP can only accept config after init
  - BDB formation can run quickly after init (so the window to apply config is tight).

### What warrants a second pair of eyes
- The exact Zigbee API intended for “formation channel selection” in host+NCP mode:
  - confirm whether `esp_zb_set_primary_network_channel_set()` / `esp_zb_set_channel_mask()` are the correct APIs in the NCP example
  - confirm if the NCP build is reading `zb_fct` at boot and overriding these values.

### What should be done in the future
- Perform an explicit storage reset experiment:
  - erase `zb_storage` / `zb_fct` partitions (or full flash erase) on the H2
  - then apply channel mask again and observe formation channel
- Add logging of “current channel” right before and right after BDB start, to see if it changes during formation.
- Once channel is stable, re-test the plug: observe whether short address stays stable across minutes and across coordinator reflashes.

### Code review instructions
- Host channel config:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
- NCP channel config + signal timing:
  - `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
- NCP partition layout that may influence persisted channel selection:
  - `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/partitions.csv`

### Technical details
- Channel mask reference:
  - channel 25 mask: `0x02000000` (i.e. `1U << 25`)
- Example observed mismatch:
  - NCP: `apply primary channel mask: 0x02000000 (ESP_OK)`
  - NCP: `Formed network successfully ... Channel:13`

## Step 17: Attempt reMarkable upload + attempt to run “erase storage” experiment (blocked by environment)

This step tried to (1) upload the newly updated docs to reMarkable and (2) execute the highest-signal experiment (erase H2 Zigbee storage and re-test channel selection). Both attempts exposed environment-level blockers: DNS resolution is broken in this runtime, and direct access to `/dev/ttyACM*` is denied (even though device node ACLs look correct).

The outcome is that the ticket now contains runnable helper scripts for a developer machine (tmux monitor, erase/flash, targeted storage erase, and PDF-only export). These scripts let us proceed with the same experiments outside this restricted runtime while keeping a single source of truth in the ticket.

### What I did
- Tried to upload 0031 docs to reMarkable using the standard uploader:
  - `python3 /home/manuel/.local/bin/remarkable_upload.py --ticket-dir ... <doc.md>`
- Verified DNS is non-functional in this environment:
  - `curl -I https://example.com -m 5` → `Could not resolve host: example.com`
  - `getent hosts google.com` → no output
- Tried to run the H2 “erase-flash” experiment using `idf.py` and direct serial tooling:
  - `idf.py -C /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 erase-flash` (failed)
  - `esptool.py ... -p /dev/ttyACM0 erase_flash` (failed)
  - `python3 -c 'import serial; serial.Serial(\"/dev/ttyACM0\",115200)'` → `Permission denied`
- Confirmed the OS-level ACLs look permissive, but open() still fails:
  - `getfacl -p /dev/ttyACM0` shows `user:manuel:rw-`
- Confirmed we cannot use `sudo` here to change system DNS because `no new privileges` is set:
  - `sudo ...` → `The "no new privileges" flag is set`
- Added helper scripts directly into the ticket to run these workflows on a normal developer machine:
  - `.../scripts/10-tmux-dual-monitor.sh` (two-pane tmux monitors: H2 + host)
  - `.../scripts/20-h2-erase-and-flash.sh` (erase-flash + flash via idf.py)
  - `.../scripts/21-h2-erase-zigbee-storage-only.sh` (parttool erase `zb_fct` + `zb_storage`)
  - `.../scripts/90-remarkable-pdf-only.sh` (offline PDF generation without rmapi)
- Updated the investigation report to reference the new scripts.
- Generated PDFs offline (no upload) for the main 0031 docs:
  - `ttmp/.../scripts/90-remarkable-pdf-only.sh`
  - Output: `ttmp/.../exports/remarkable-pdf/*.pdf`

### Why
- The next debugging step (erasing Zigbee persistent storage) requires reliable flash/serial access and is the highest-signal way to confirm whether `zb_fct` / `zb_storage` is overriding our requested channel mask.
- Uploading to reMarkable is part of the workflow, but it depends on working DNS + rmapi connectivity.

### What worked
- The scripts are now present in the ticket so the workflow is reproducible on a developer machine with working serial + DNS.
- PDF generation works offline and produces shareable artifacts even when rmapi upload is blocked.

### What should be done in the future
- Vendor the H2 NCP firmware sources into this repo so we can build/flash from here without relying on `/home/manuel/esp/...` paths (done in the next step).

### What didn't work
- reMarkable upload failed due to DNS resolution failure:
  - `rmapi mkdir failed ... dial tcp: lookup internal.cloud.remarkable.com: no such host`
- Serial access to `/dev/ttyACM0` and `/dev/ttyACM1` failed with `Errno 13 Permission denied` even though device ACLs list user access.
- Writing under `/home/manuel/esp/...` also fails with `Permission denied`, so running `idf.py` there cannot create its stdout/stderr log files.

### What I learned
- This runtime cannot be used to flash/monitor physical devices or reach external network services; “continue” must be executed via scripts on the developer machine.

### What was tricky to build
- Diagnosing why serial access fails: the device node permissions are correct, so the denial is likely coming from a higher-level restriction (e.g., container device/cgroup restrictions), not Unix file permissions.

### What warrants a second pair of eyes
- Whether the team wants to vendor the H2 NCP example into this repo (so all required firmware sources are in the workspace and don’t depend on `/home/manuel/esp/...` write access), or keep using the external SDK tree.

### What should be done in the future
- Run Experiment 1 on the developer machine:
  - `scripts/21-h2-erase-zigbee-storage-only.sh` (preferred) or `scripts/20-h2-erase-and-flash.sh`
  - then `zb ch 25` + `zb reboot` from the host console and confirm formation channel.
- Run the PDF-only script and upload PDFs manually if rmapi continues to fail in some environments:
  - `scripts/90-remarkable-pdf-only.sh`

### Code review instructions
- Scripts:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/20-h2-erase-and-flash.sh`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/21-h2-erase-zigbee-storage-only.sh`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/90-remarkable-pdf-only.sh`
- Investigation report update:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md`

### Technical details
- reMarkable failure:
  - `rmapi ... dial tcp: lookup internal.cloud.remarkable.com: no such host`
- Serial failure:
  - `SerialException(13, "could not open port /dev/ttyACM0: [Errno 13] Permission denied")`

## Step 18: Vendor the H2 NCP firmware into this repo (thirdparty) and build it locally

This step moves the ESP32‑H2 NCP firmware sources we were previously building from `/home/manuel/esp/esp-zigbee-sdk/...` into this repository under `thirdparty/`. The reason is stability and reviewability: the NCP firmware is part of our product stack, we are modifying it during bring-up, and we want it under git control next to the host firmware and ticket docs.

The outcome is a reproducible, workspace-local NCP build path: `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp set-target esp32h2 && idf.py -C ... build`.

### What I did
- Copied a minimal subset of `esp-zigbee-sdk` into this repository:
  - `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/`
  - `thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/` (including `managed_components/`)
- Removed the checked-in `sdkconfig` from the vendored example and added local `.gitignore` rules so build outputs don’t get committed.
- Added vendored README:
  - `thirdparty/esp-zigbee-sdk/README.md` (includes upstream origin commit `3d86dd0c...`)
- Updated ticket scripts to use the vendored project path by default:
  - `.../scripts/10-tmux-dual-monitor.sh`
  - `.../scripts/20-h2-erase-and-flash.sh`
  - `.../scripts/21-h2-erase-zigbee-storage-only.sh`
- Built the vendored NCP project from this repo (and fixed the target):
  - `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp set-target esp32h2`
  - `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp build`

### Why
- We are actively patching NCP behavior (permit join handling, signal logs, channel config timing). Keeping those changes in `/home/manuel/esp/...` makes code review, CI, and long-term maintenance harder.
- Vendoring the exact sources used to produce the running H2 firmware ensures we can bisect/debug and share a reproducible build with new developers.

### What worked
- The vendored NCP firmware builds successfully from the workspace.
- The example includes `managed_components/`, so it can build without relying on fetching components during build.

### What didn't work
- The component manager still “updates” `dependencies.lock` during configure and may record absolute paths; we patched it to a repo-relative path to keep it portable, but developers should treat this file as “generated metadata” if it changes.

### What I learned
- The upstream example expects `idf.py set-target esp32h2` to be run at least once; otherwise it may build for the default target (esp32). Our scripts now enforce this.

### What was tricky to build
- Keeping the vendored example clean: the upstream example directory tends to accumulate `sdkconfig` from local builds; we explicitly remove/ignore it so the repo stays tidy.

### What warrants a second pair of eyes
- Whether we want to pin and enforce `dependencies.lock` strictly in git, or ignore it and rely on `managed_components/` + `idf_component.yml` for reproducibility.

### What should be done in the future
- Update remaining docs to prefer the vendored NCP paths (some diary entries reference the old `/home/manuel/esp/...` locations for historical accuracy).
- Add a CI check that the vendored NCP firmware builds (optional, if CI exists for this repo).

### Code review instructions
- Vendored NCP:
  - `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
  - `thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/main/esp_zigbee_ncp.c`
  - `thirdparty/esp-zigbee-sdk/README.md`
- Updated scripts:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/20-h2-erase-and-flash.sh`

### Technical details
- Build commands:
  - `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp set-target esp32h2`
  - `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp build`

## Step 19: Use “authorization timeout” diagnosis to add TCLK/link-key debug controls (host + NCP)

This step incorporated a decode of our join/rejoin logs that points to the most likely explanation for the “short address churn”: the device is joining, but it never completes Trust Center authorization (TCLK/key exchange), so it times out and retries. That failure mode looks exactly like what we observed: repeated `Device update (unsecured join)` and repeated `Device authorized ... status=timeout`.

The outcome is a set of concrete knobs we can exercise from the Cardputer console: get/set the Trust Center preconfigured key (TCLK), temporarily disable the link-key exchange requirement to confirm the diagnosis, and (optionally) enable `nvram_erase_at_start` to force fresh formation and avoid NVRAM overriding channel settings.

### What I did
- Read and related the decoded log analysis:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/Zigbee debug logs.md`
- NCP: implemented real behavior for previously-stubbed ZNSP handlers:
  - `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
    - Apply the Trust Center preconfigured key via `esp_zb_secur_TC_standard_preconfigure_key_set()` during form-network.
    - Store/apply `link_key_exchange_required` via `esp_zb_secur_link_key_exchange_required_set()`.
    - Expose `esp_zb_nvram_erase_at_start()` via new request IDs `0x002E/0x002F`.
    - Make `0x0027/0x0028` (link key get/set) return/use the configured key instead of hardcoded values.
- Protocol: added new NCP request IDs for nvram erase flag:
  - `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_zb.h` (`0x002E/0x002F`)
  - `0031-zigbee-orchestrator/components/zb_host/src/priv/esp_host_zb.h` (mirror)
- Host console: added friendly `zb` commands so this is not “raw-hex-only”:
  - `0031-zigbee-orchestrator/main/gw_console_cmds.c`
    - `zb tclk get|set default|<16 bytes>`
    - `zb lke on|off|status` (link-key exchange required)
    - `zb nvram on|off|status` (erase zb_storage at start)
- Updated investigation report with an explicit “Experiment 0” (security first), and referenced the decoded-log doc.

### Why
- The fastest way to stop guessing is to confirm whether the join loop is truly “authorization/TCLK exchange failing”. If disabling the requirement stabilizes the device, we can focus on the right layer (security + RF) instead of chasing symptoms.
- If NVRAM restore is overriding channel config, `esp_zb_nvram_erase_at_start(true)` is a controlled way to force a clean formation and verify channel selection logic.

### What worked
- Both the vendored H2 NCP project and the Cardputer host firmware build clean after these changes:
  - `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp build`
  - `idf.py -C 0031-zigbee-orchestrator build`

### What didn't work
- Hardware validation is still blocked in this environment (serial access + DNS issues noted in Step 17), so the new knobs are unverified on-device in this runtime.

### What I learned
- The NCP code had stubs for link key and security mode that returned fixed values; this makes it easy to believe we “set the key” when we actually didn’t. Making these real is necessary before we can interpret authorization failures.

### What was tricky to build
- Keeping semantics minimal and safe: these controls must be usable for diagnosis without inadvertently bricking commissioning (e.g., enabling NVRAM erase at start permanently by accident). The console verbs are explicit so it’s obvious what mode is active.

### What warrants a second pair of eyes
- Security API mapping: confirm `esp_zb_secur_TC_standard_preconfigure_key_set()` is the correct hook for “TCLK join key” behavior for our device set (vs distributed key or install-code policy).

### What should be done in the future
- Validate on real hardware:
  - `zb tclk set default`
  - `zb lke off` (diagnostic) → pair → confirm stability
  - `zb lke on` (secure) → pair again
  - if channel remains 13: `zb nvram on` and re-form / reboot

### Code review instructions
- Start at the NCP request handlers:
  - `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
- Review console UX and request IDs:
  - `0031-zigbee-orchestrator/main/gw_console_cmds.c`
  - `0031-zigbee-orchestrator/components/zb_host/src/priv/esp_host_zb.h`

### Technical details
- Default TCLK key bytes (`ZigBeeAlliance09`):
  - `5a 69 67 42 65 65 41 6c 6c 69 61 6e 63 65 30 39`

## Step 20: Create and run Experiment 0 script (blocked here, ready for developer machine)

This step focused on turning the new “security-first” debug flow into an executable playbook: a script that (optionally) flashes both devices and then drives the Cardputer console to set TCLK, disable link-key exchange requirement, and open permit-join. This is meant to remove friction and make it easy to reproduce the same investigation sequence when someone else joins the team.

In this environment, we still cannot open the USB serial/JTAG devices, so the script can’t be executed end-to-end here. However, it is now committed in the ticket and should work on the normal developer machine.

### What I did
- Added flash helper for the host:
  - `ttmp/.../scripts/22-host-flash.sh`
- Added Experiment 0 driver script:
  - `ttmp/.../scripts/30-experiment0-security.sh`
  - It sends:
    - `zb tclk set default`
    - `zb tclk get`
    - `zb lke off`
    - `gw post permit_join 180`
- Attempted to run it here:
  - `bash ttmp/.../scripts/30-experiment0-security.sh`

### Why
- The join-loop issue is time-consuming to debug if the sequence is “manual and different every time”. A scripted experiment makes results comparable and makes it easier to hand off debugging to another developer.

### What worked
- Script is present and runnable in the repo; it uses `/dev/serial/by-id/` defaults but supports `HOST_PORT`/`H2_PORT` overrides.

### What didn't work
- The script fails here because the runtime cannot open the serial device:
  - `SerialException(13, "... [Errno 13] Permission denied: '/dev/serial/by-id/...')`

### What I learned
- Even with correct code and tooling, this environment cannot perform the real “flash + interactive console + pairing” loop. We must run these experiment scripts on the developer machine directly attached to the hardware.

### What was tricky to build
- Keeping the script robust against timing: the console prompt may appear after boot logs; the script waits for `gw>` before/after commands and warns if it can’t find the prompt.

### What warrants a second pair of eyes
- The script’s prompt detection (`gw>`): if we ever change the prompt, the automation will need updates.

### What should be done in the future
- Run the experiment on the dev machine:
  - `ttmp/.../scripts/10-tmux-dual-monitor.sh`
  - `ttmp/.../scripts/30-experiment0-security.sh`
  - Put the plug into pairing mode and observe whether `Device authorized ... status=0x00` appears.

### Code review instructions
- Review scripts:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/30-experiment0-security.sh`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/22-host-flash.sh`

### Technical details
- Expected “good” console transcript (high level):
  - `zb tclk set default` → status OK
  - `zb lke off` → status OK
  - `gw post permit_join 180` → status OK

## Step 21: Fix the monitoring workflow (avoid resetting the H2 and corrupting the Grove ZNSP bus)

During Zigbee bring-up we discovered a subtle but very practical failure mode: simply starting `idf.py monitor` on the H2 can reset the H2 (because the tool toggles DTR/RTS by default). When the H2 resets while it is still wired to the Cardputer over the Grove UART, its boot ROM banner text can spill onto the same UART lines that carry SLIP/ZNSP, which then corrupts framing and can make the host interpret garbage as protocol responses.

The outcome of this step is a safer default monitoring playbook: always pass `--no-reset` when monitoring both devices, and treat “ESP-ROM:esp32h2-…” observed on the Grove bus as a sign that the H2 rebooted at the wrong time (so the host should be rebooted too, or requests should be retried only after the H2 is stable).

### What I did
- Updated the tmux dual-monitor helper to always run monitors without toggling reset lines:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh`
    - `idf.py ... monitor --no-reset` for both H2 and host
- Updated the investigation report with an explicit “monitor reset” warning and mitigation:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md`

### Why
- When the H2 resets mid-session, the host UART sees ASCII boot logs (not SLIP frames), which can:
  - trigger SLIP parse errors (`Invalid packet len ...` in `zb_host`), and
  - cause in-flight ZNSP request/response pairs to desynchronize (making subsequent debug conclusions unreliable).
- Avoiding unintended resets makes Experiment 0 (TCLK/LKE) and channel/NVRAM experiments reproducible and comparable across runs.

### What worked
- The tmux helper now uses a safe default (`--no-reset`) so the common “open monitor” action doesn’t destabilize the bus.
- The investigation report now documents this trap so a new developer doesn’t lose time chasing phantom protocol errors.

### What didn't work
- N/A (this step is a workflow hardening change; it doesn’t by itself fix the underlying join/authorization loop).

### What I learned
- On this topology (host↔NCP over a “real” UART), monitor tooling and control-line behavior can materially affect protocol correctness; debugging needs “do no harm” defaults.

### What was tricky to build
- The failure mode is non-obvious because it looks like a protocol/stack bug at first: the host sees invalid frames and timeouts, but the root cause is out-of-band ASCII boot logs injected onto the protocol UART during NCP reset.

### What warrants a second pair of eyes
- Whether we should additionally harden the host ZNSP transport with a “detect reset + resync” strategy (e.g. detect `ESP-ROM` strings on the protocol UART and temporarily block/flush until a clean SLIP frame boundary).

### What should be done in the future
- When capturing logs for commissioning experiments, always use:
  - `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 monitor --no-reset`
  - `idf.py -C 0031-zigbee-orchestrator -p /dev/ttyACM1 monitor --no-reset`
- If the H2 is reset (intentional or not), reboot the host (or at minimum restart its ZNSP stack/task) before interpreting further ZNSP results.

### Code review instructions
- Start with the tmux helper change:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh`
- Review the new warning text:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md`

### Technical details
- Unsafe (may reset H2): `idf.py monitor`
- Safe: `idf.py monitor --no-reset`

## Step 22: Capture H2 logs without `idf.py monitor` (serial capture script) and re-run Experiment 0

This step validated that our new TCLK/LKE controls are not just “host-side prints”, but actually reach the H2 NCP and produce the expected request frames. It also addressed a practical tooling mismatch: `idf.py monitor` refuses to run unless stdin is a real TTY in this environment, which makes it unreliable for automated capture from within scripts.

The key outcome is a simple, robust alternative: a tiny pyserial-based capture script that can record the H2’s USB-Serial/JTAG console output to a file while we drive the host console automation. With that, we have a reproducible artifact proving that ZNSP request IDs `0x0028` (set TCLK), `0x0027` (get TCLK), `0x002A` (set LKE mode), `0x0029` (get LKE mode), and `0x0005` (permit-join) are flowing over the bus correctly.

### What I did
- Added a serial capture script to the ticket workspace:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/11-capture-serial.py`
- Re-ran Experiment 0 while capturing the H2 UART/JTAG console to a file:
  - Created run folder:
    - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/`
  - Commands (high level):
    - `python3 .../scripts/11-capture-serial.py --port /dev/ttyACM0 --seconds 25 --out .../h2-raw.log &`
    - `HOST_PORT=/dev/ttyACM1 bash .../scripts/30-experiment0-security.sh | tee .../host-exp0.log`

### Why
- `idf.py monitor` errors in this environment when stdin isn’t a TTY:
  - `Error: Monitor requires standard input to be attached to TTY. Try using a different terminal.`
- We still need continuous H2 visibility while driving the host console (especially while debugging commissioning/authorization).

### What worked
- `scripts/11-capture-serial.py` successfully captured H2 logs without any TTY requirements.
- The captured H2 log includes the expected frames corresponding to our Experiment 0 host actions (example excerpt):
  - `ESP_NCP_FRAME: 00 00 28 00 ... 5a 69 67 42 65 65 41 6c 6c 69 61 6e 63 65 30 39 ...` (set TCLK to ZigBeeAlliance09)
  - `ESP_NCP_FRAME: 00 00 27 00 ...` (get TCLK)
  - (and subsequent frames for LKE and permit-join)

### What didn't work
- Using `idf.py monitor` for programmatic capture remains unreliable here due to the TTY constraint (even when wrapped in `timeout`).

### What I learned
- For automated investigations, raw serial capture via pyserial is often a better building block than `idf.py monitor` because it avoids stdin/terminal assumptions and avoids side effects like DTR/RTS toggling.

### What was tricky to build
- Making the capture script behave “like a passive tap”: it must not change control lines, must not block, and must flush frequently so partial logs are visible during an ongoing run.

### What warrants a second pair of eyes
- Whether we should standardize on this capture script for all “dual-device” workflows (host+NCP), or keep it as a fallback only when TTY/monitor is problematic.

### What should be done in the future
- Extend the capture script to optionally timestamp each line on write (helpful when correlating host vs H2 logs precisely).
- Capture both ports simultaneously into separate files (host + H2) and add a small merge tool for timeline correlation.

### Code review instructions
- Review the capture script:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/11-capture-serial.py`
- Review the evidence artifact:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/h2-raw.log`

### Technical details
- The host console output for this run is in:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/host-exp0.log`

## Step 23: Pairing attempts (LKE off vs on) + dual host/H2 capture

This step focused on answering the simplest question: “is the plug actually attempting to join, and if it does join, where does it get stuck?” We ran several pairing windows while capturing both the host console and the H2 NCP logs into a timestamped run directory. The capture method evolved during the step: first we used background `nohup` captures and ad-hoc tailing, then we consolidated into a single script that drives the host commands and captures both ports concurrently.

The key outcome is a clean reproduction of the suspected failure mode: with **LKE on**, the plug announces and gets a short address, but we repeatedly see **`Device authorized ... status=0x01` (authorization timeout)** and the short address changes over time. With **LKE off**, we sometimes saw “nothing happens” (no announce at all), suggesting some devices may not attempt joining (or we were missing the timing window) when the coordinator is configured that way.

### What I did
- Implemented a combined driver + dual capture tool:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/13-dual-drive-and-capture.py`
  - It:
    - sends `monitor on`, `zb tclk set default`, `zb lke <on/off>`, and `gw post permit_join <seconds>`
    - captures `/dev/ttyACM0` (H2) and `/dev/ttyACM1` (host) concurrently to `h2.log` and `host.log`
    - prints “interesting” lines live to stdout (announce/authorized/update/leave keywords)
- Fixed a real-world gotcha during repeated relaunches: serial ports can’t be opened by two processes at once.
  - Example failure:
    - `serial.serialutil.SerialException: device reports readiness to read but returned no data (device disconnected or multiple access on port?)`
  - Root cause: an earlier tmux capture session still held `/dev/ttyACM0` and `/dev/ttyACM1`.
  - Fix: kill the old tmux session before relaunching a new capture.
- Captured runs (selected):
  - “Nothing joins” window (permit-join open, no announce):
    - `.../sources/local/runs/2026-01-05-pairing-223926/`
  - LKE=on pairing (announce observed, but authorization timeouts + short churn):
    - `.../sources/local/runs/2026-01-05-pairing-lke-on-224457/`

### Why
- We needed evidence that separates:
  - “the plug never even tries to join” vs
  - “it joins but fails authorization” vs
  - “our host registry/command path is wrong”
- Capturing both sides simultaneously is the only reliable way to correlate the join procedure.

### What worked
- With LKE=on we consistently captured the full chain:
  - `Device update (status=0x01 ...)` on H2
  - `GW_EVT_ZB_DEVICE_ANNCE` on host
  - then `Device authorized (type=0x01 status=0x01 ...)` (authorization timeout) on H2
  - and later, the same IEEE returning with a *different* short address (churn)
- The “dual capture” script makes relaunching a debug window fast and produces stable artifacts (logs) that can be reviewed after the fact.

### What didn't work
- With LKE=off, we sometimes observed no join activity at all during the permit-join window (no announce/update logs).
  - This might be a timing issue (plug not actually in join mode while the window is open), or a behavior difference in the device when the coordinator is configured this way.
- We also observed H2 reboot banners (`ESP-ROM:esp32h2-...`) appearing inside one capture; this indicates the H2 reset during/near commissioning, which can invalidate conclusions for that window unless the reset was intentional.

### What I learned
- “Permit join is open” is necessary but not sufficient; you still need a reliable repeatable method to confirm whether the device is actually sending join requests on-air.
- Repro tooling needs to own the serial ports end-to-end; otherwise old monitor/capture processes will silently sabotage new runs.

### What was tricky to build
- Coordinating two serial streams without `idf.py monitor` (TTY requirements, DTR/RTS behavior) while still getting useful timestamps and context.
- Avoiding side effects from serial control lines (best-effort disabling DTR/RTS in the capture scripts).

### What warrants a second pair of eyes
- Whether the H2 reset observed in `.../2026-01-05-pairing-lke-on-224457/h2.log` is:
  - an artifact of capture/tooling (control lines/power),
  - a stability issue in the NCP firmware,
  - or something triggered by the commissioning/security flow.

### What should be done in the future
- Re-run the LKE=on capture with the H2 physically stable (no USB replugging, avoid any tools that toggle reset) and confirm whether the H2 resets recur.
- Focus next debugging on why authorization times out:
  - channel interference (still forming on ch13 despite mask),
  - NVRAM persistence overriding config,
  - Trust Center key / policy mismatch,
  - or device-specific commissioning requirements.

### Code review instructions
- Start with the dual capture tooling:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/13-dual-drive-and-capture.py`
- Review pairing evidence logs:
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-05-pairing-lke-on-224457/h2.log`
  - `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-05-pairing-lke-on-224457/host.log`

### Technical details
- LKE=on run excerpt (symptom):
  - `Device authorized (type=0x01 status=0x01 ...)` followed later by the same IEEE reappearing with a new short address.
