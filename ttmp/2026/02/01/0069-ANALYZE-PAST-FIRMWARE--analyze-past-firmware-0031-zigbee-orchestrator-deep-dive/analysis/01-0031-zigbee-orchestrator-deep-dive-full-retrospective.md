---
Title: '0031 Zigbee Orchestrator Deep Dive: Full Retrospective'
Ticket: 0069-ANALYZE-PAST-FIRMWARE
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32s3
    - esp32h2
    - esp32
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md
      Note: Host+NCP bring-up history and UART lessons
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md
      Note: Foundational 0031 architecture and migration analysis
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md
      Note: Primary investigation of rejoin loop and channel issue
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/design-doc/01-design-cardputer-zigbee-orchestrator-esp-event-bus-http-202-protobuf-ws.md
      Note: Formal design contract and phased plan
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md
      Note: Primary implementation timeline for 0031
    - Path: ttmp/2026/01/06/0032-ANALYZE-NCP-FIRMWARE--analyze-ncp-h2-gateway-firmware/analysis/01-ncp-firmware-architecture-and-protocol-analysis.md
      Note: NCP architecture and protocol deep dive used to patch and debug 0031
    - Path: ttmp/2026/01/06/0034-ANALYZE-ESP-ZIGBEE-LIB--analyze-esp-zigbee-lib-low-level-stack/analysis/01-esp-zigbee-lib-low-level-stack-analysis.md
      Note: Zigbee stack API and signal analysis referenced during 0031 integration
    - Path: ttmp/2026/01/06/0035-IMPROVE-NCP-LOGGING--improve-ncp-logging-convert-constants-to-strings/analysis/01-ncp-logging-constant-conversion-analysis.md
      Note: Logging and command ID mapping analysis for NCP debugging
ExternalSources: []
Summary: ""
LastUpdated: 2026-02-02T00:00:00-05:00
WhatFor: ""
WhenToUse: ""
---


# 0031 Zigbee Orchestrator Deep Dive: Full Retrospective

## 0. What this is and how to use it

This is a full retrospective of ticket 0031-ZIGBEE-ORCHESTRATOR and the work that directly fed into it. It is written for someone who needs to catch up fast but also wants the long-form technical history and the exact stopping points. The emphasis is on the "why" behind changes, the "where" in the code and docs, and the "what happened next" chain of consequences.

How to read:
- If you want the fast orientation, start with Sections 1 and 2.
- If you want the full chronology, read Sections 3 through 7 in order.
- If you want only the "where we stopped and why," jump to Section 8.
- If you want the actionable path forward, read Sections 9 and 10.

Everything here is sourced from the 0031 diary and its related docmgr tickets and docs, not from memory. File paths are given so you can jump to the source material.

## 1. Executive summary (one page)

0031 is a Zigbee orchestrator for Cardputer (ESP32-S3) that uses an internal esp_event bus and exposes an HTTP control plane plus a protobuf WebSocket event stream. The Zigbee stack itself runs on an ESP32-H2 NCP (Unit Gateway H2). The host and NCP communicate over UART using the Espressif host<->NCP protocol (ZNSP-style), SLIP framed.

The orchestrator architecture and tooling were not invented in 0031; they were inherited from earlier work:
- 001-ZIGBEE-GATEWAY established the host+NCP split, the UART wiring, and the NCP/host protocol bring-up, including several hard hardware and tooling failures.
- 0029-HTTP-EVENT-MOCK-ZIGBEE established the "hub shell" architecture: esp_event bus, HTTP control plane, and protobuf WS stream with nanopb encoding.
- 0029a added the USB-Serial/JTAG esp_console Wi-Fi setup and scanning workflow that became the default control-plane UX for 0031.
- 0030-CARDPUTER-CONSOLE-EVENTBUS provided the reusable pattern for bus-centric modules and console monitoring, including the first nanopb "IDL on device" proof.
- 0032, 0034, 0035 are analysis/diagnostic tickets that mapped the NCP firmware, esp-zigbee-lib stack, and logging/command mapping, which were then used to patch and debug the real system.

0031 itself evolved in two phases:
1) Design and scaffolding (analysis + design docs, bus/console bring-up, and project fork from 0029). This established the orchestrator contract and a working console + event bus.
2) Real Zigbee integration and debugging: integrating the host NCP stack on Cardputer, patching the H2 NCP firmware, proving permit-join and ZCL OnOff control, then confronting a persistent "device rejoin loop" and "channel stuck on 13" issue. The investigation led to a security-focused hypothesis (Trust Center key / link-key exchange) and a set of diagnostic commands and scripts.

Current stopping points:
- The device rejoin loop is not fully resolved. Logs show authorization timeouts, suggesting Trust Center key / link-key exchange issues. TCLK/LKE controls were added but not yet validated in a clean environment.
- Channel selection remains stuck on channel 13 even after applying channel masks; likely overwritten by persisted Zigbee storage (zb_fct/zb_storage) or timing of mask application.
- Environment constraints (DNS unavailable, serial access denied) blocked full on-device experimentation inside this runtime; scripts were added for execution on a developer machine.

## 2. Source map and related tickets

This section names the source tickets and why they are relevant. These are the "related docmgr tickets" that directly shaped 0031.

### Primary ticket
- 0031-ZIGBEE-ORCHESTRATOR
  - analysis: evolve 0029 mock hub into real Zigbee orchestrator
  - design: Cardputer Zigbee orchestrator (esp_event bus + HTTP 202 + protobuf WS)
  - investigation: device rejoin loop + channel selection stuck on ch13
  - diary: 23-step implementation and debugging log

### Direct predecessors and architecture foundations
- 001-ZIGBEE-GATEWAY
  - host+NCP architecture bring-up on CoreS3 + Unit Gateway H2
  - clarifies RCP/Spinel vs NCP/ZNSP architectures
  - includes diary of UART bring-up failures and fixes

- 002-ZIGBEE-NCP-UART-LINK
  - smoke-test firmware and bugreport confirming physical UART link problems

- 0029-HTTP-EVENT-MOCK-ZIGBEE
  - event-bus + HTTP + protobuf WS shell for the future Zigbee orchestrator
  - diary shows how the bus, protobuf, and WS stream were stabilized

- 0029a-ADD-WIFI-CONSOLE
  - USB-Serial/JTAG esp_console Wi-Fi configuration and scanning commands
  - this became the default control plane for Cardputer

- 0030-CARDPUTER-CONSOLE-EVENTBUS
  - pattern documentation for esp_event-centric architecture
  - nanopb "IDL on device" proof (protos as schema, not necessarily wire)

### Deep stack analysis and tooling
- 0032-ANALYZE-NCP-FIRMWARE
  - NCP architecture, frame format, SLIP, task model, command mapping

- 0034-ANALYZE-ESP-ZIGBEE-LIB
  - esp-zigbee-lib low-level stack API and signal analysis

- 0035-IMPROVE-NCP-LOGGING
  - logging conversions and command ID mapping improvements for NCP debug

### Post-0031 follow-ons (awareness only)
- 0067-zigbee-powerplug
  - device-specific pairing notes and behavior on a real plug

- 0068-ZIGBEE-SNIFFING
  - Zigbee sniffer tooling that may help future RF/channel debugging

These tickets are the authoritative record for 0031's foundations and the technical "why."

## 3. Architectural lineage: what 0031 inherited and why

0031 is best understood as a synthesis of two lines of work:
1) The Zigbee host+NCP hardware and protocol stack (tickets 001/002 and later 0032/0034/0035).
2) The event-bus + HTTP + protobuf tooling shell (tickets 0029/0029a/0030).

### 3.1 Host+NCP architecture from 001

Ticket 001 established that the only realistic "Wi-Fi + Zigbee coordinator" architecture for this hardware stack is a two-chip system:
- Host: ESP32-S3 (CoreS3 or Cardputer) for Wi-Fi + control plane.
- NCP: ESP32-H2 (Unit Gateway H2) for Zigbee stack and radio.
- Protocol: host<->NCP frames over UART, SLIP framed.

The 001 diary also captured the painful hardware realities:
- UART wiring errors and partially seated Grove cables cause total "no data" symptoms that look like protocol issues.
- USB-Serial/JTAG console must not share UART pins used for the host<->NCP link, or the protocol channel is corrupted.
- `idf.py monitor` requires a real TTY; automated workflows must use `script` or custom capture scripts.

This is the foundational "real Zigbee over Wi-Fi" constraint that 0031 inherits. The 0031 analysis doc explicitly points to 001 as the real Zigbee architecture anchor.

### 3.2 Event-bus + protobuf shell from 0029

Ticket 0029 established the event-driven hub shell that 0031 forks and evolves. Key outcomes that 0031 reuses:
- `esp_event` as the internal bus (single event loop, serialized handlers).
- HTTP handlers that only parse/validate and then post bus commands (no direct state mutation or device calls).
- A protobuf (nanopb) event envelope that mirrors C event IDs to protobuf event IDs.
- A WebSocket event stream that pushes protobuf frames for observability.

The 0029 diary demonstrates why this architecture mattered:
- JSON WS streams were too heavy and noisy for embedded; protobuf solved it.
- In-bus JSON formatting caused REPL instability; moving encoding and WS send to a dedicated task fixed it.
- HTTP wildcard routing quirks and httpd stack sizes were real pitfalls that had to be resolved before the system was usable.

By the time 0031 started, 0029 had validated the basic plumbing: Wi-Fi + HTTP + protobuf WS, plus a web UI proof of concept.

### 3.3 Console-first workflow and Wi-Fi tooling from 0029a

Ticket 0029a added the "no hardcoded credentials" workflow that later became the default for Cardputer-based projects:
- USB-Serial/JTAG `esp_console` REPL.
- `wifi scan`, `wifi set`, `wifi connect`, `wifi status` with NVS persistence.
- A safety fix so the REPL starts on whichever console backend is enabled (USB or UART), avoiding dead "no console" situations.

0031 adopted this pattern for early bring-up and future HTTP/WS control.

### 3.4 Bus-centric architecture patterns from 0030

Ticket 0030 created a clean, minimal demo that showed the event-bus pattern with console input, a monitor command, and nanopb encoding of bus events. The key pattern that 0031 reuses:
- The event bus is the "spine." Producers post events; consumers listen; all state mutations happen in the bus task.
- A console "monitor on/off" is essential to make internal bus traffic visible without a UI or WS.
- Protobuf is best treated as an IDL and serialization boundary, not as the in-memory event representation.

The 0030 pattern docs were explicitly referenced in 0031's analysis as the generic "event-centric architecture" foundation.

## 4. 0031's formal design and decisions (analysis + design docs)

The 0031 analysis and design docs did three critical things before implementation began:
1) Made the architectural decision to commit to host+NCP immediately.
2) Defined the bus + HTTP + WS contract, including async HTTP semantics.
3) Scoped the MVP to simple Zigbee capabilities (OnOff + LevelControl, basic reports).

### 4.1 Core decisions

These decisions are frozen in the 0031 analysis doc and echoed in the design doc:
- Host target: Cardputer (ESP32-S3).
- Zigbee split: host + H2 NCP (ZNSP over SLIP/UART).
- MVP clusters: OnOff + LevelControl plus basic attribute reports.
- HTTP contract: 202 Accepted with async completion events on the WS stream.

### 4.2 Bus / Zigbee execution pattern

0031 explicitly chose "Pattern A" for Zigbee calls:
- Bus handler -> Zigbee worker task queue.
- Zigbee worker serializes all host-to-NCP calls.
- Results are posted back into the bus as events.

This ensures determinism and avoids making esp_event handlers call Zigbee APIs directly.

### 4.3 Event taxonomy and protobuf envelope

0031 planned to replace 0029's HUB_* events with GW_* events, while keeping the essential structure:
- A top-level event envelope with `schema_version`, `event_id`, `ts`, and a `oneof` payload.
- Event IDs in protobuf match C enums.
- Payload structs remain bounded and copyable (esp_event copies payloads by value).

Key events include:
- GW_CMD_PERMIT_JOIN, GW_CMD_ZCL_CMD_DEVICE, GW_CMD_DEVICE_INTERVIEW
- GW_EVT_ZB_DEVICE_ANNCE, GW_EVT_ZB_ATTR_REPORT, GW_EVT_CMD_RESULT

### 4.4 HTTP contract rationale

The design doc explicitly rejected synchronous HTTP for Zigbee actions:
- Zigbee operations are multi-step and asynchronous.
- 202 Accepted with req_id correlation avoids lying about completion.
- Event stream becomes the "source of truth."

This decision influenced all later bus and WS tooling decisions.

## 5. 0031 implementation: phase-by-phase narrative

This section follows the 0031 diary steps (1-23) and groups them into coherent phases. Each phase includes the "why," the "where," and the critical outcomes.

### Phase A: Ticket creation and architecture consolidation (Steps 1-5)

#### Step 1: Create ticket + initial archaeology
- Why: formalize the plan and link to the known-good sources (001, 0029, 0030).
- Where: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR/...`
- Outcome: analysis doc created, key files linked, architecture direction chosen.

#### Step 2: Draft analysis and migration plan
- Why: map 0029 hub shell into a real Zigbee orchestrator with minimal disruption.
- Where: `analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md`
- Outcome: event taxonomy, protobuf strategy, HTTP endpoints, and migration steps written.

#### Step 3: Hardware target lock (Cardputer)
- Why: avoid implementation drift.
- Where: analysis doc + tasks; updated decisions section.
- Outcome: Cardputer is the fixed host, reducing ambiguity.

#### Step 4: Zigbee glossary and decision closure
- Why: remove Zigbee jargon friction for new developers.
- Where: analysis doc glossary and decisions.
- Outcome: clarified terms like scheduler alarms, ZCL database, and HTTP contract.

#### Step 5: Write full design doc
- Why: formalize module boundaries, async contract, and phased implementation.
- Where: `design-doc/01-design-cardputer-zigbee-orchestrator-esp-event-bus-http-202-protobuf-ws.md`
- Outcome: complete design document with phases 0-4, explicit rejections, and references.

### Phase B: Host firmware scaffold and console/bus bring-up (Step 6)

#### Step 6: Fork 0029 into 0031 host project and implement bus + console
- Why: prove the control-plane spine (console + bus) before Zigbee complexity.
- Where: `0031-zigbee-orchestrator/` (new project)
  - `main/app_main.c`, `main/gw_bus.c`, `main/gw_console.c`, `main/gw_console_cmds.c`
  - `main/Kconfig.projbuild`, `sdkconfig.defaults`
- Key changes:
  - `GW_CMD_PERMIT_JOIN` and stub result events.
  - `monitor on/off` bus tap with rate limiting.
  - `gw demo` event generator.
  - USB-Serial/JTAG console as default.
- Outcomes:
  - Build and flash succeeded on Cardputer.
  - Console commands and bus monitor were functional.
- Early issues:
  - Missing `esp_app_desc.h` include fixed by adding `esp_app_format` to `PRIV_REQUIRES`.
  - Removed unused console headers to match 0029 approach.

### Phase C: Real Zigbee control path (Steps 7-11)

#### Step 7: Identify why permit_join is stubbed
- Why: the host can only open the network if the NCP implements the request.
- Key finding: NCP dispatch table had `ESP_NCP_NETWORK_PERMIT_JOINING` with NULL handler.
- Where:
  - Host protocol ID defined in `esp_host_zb.h` (0x0005).
  - NCP `esp_ncp_zb.c` had notify but no request handler.
- Outcome: Need to patch H2 NCP firmware.

#### Step 8: Integrate host-side NCP protocol + patch NCP permit_join handler
- Why: enable real permit_join and start real Zigbee operations.
- Host changes:
  - Vendored host protocol as `components/zb_host/`.
  - Added `gw_zb.c` and `gw_zb_app_signal.c` to translate bus events into NCP requests.
  - Added response timeout and hardened SLIP parsing.
- NCP changes:
  - Implemented `esp_ncp_zb_permit_joining_fn()` calling `esp_zb_bdb_open_network()`.
  - Fixed off-by-one in NCP output loop.
- Outcome:
  - Host sends permit_join but H2 response still missing because H2 not flashed / ROM logs present.
  - Identified need to flash H2 firmware and monitor separately.

#### Step 9: End-to-end permit_join validation on real hardware
- Why: close the loop (console -> bus -> NCP -> Zigbee stack -> notify).
- Critical fix: NCP crashed because a request-buffer pointer was passed into `esp_zb_init()` without copying; fixed by copying to static storage.
- Outcome:
  - H2 forms a network, host receives permit-join status.
  - `gw post permit_join 60` produces GW_EVT_CMD_RESULT + GW_EVT_ZB_PERMIT_JOIN_STATUS.

#### Step 10: Add ZCL OnOff command path
- Why: show real control, not just commissioning.
- Host changes:
  - New `GW_CMD_ONOFF` payload and console `gw post onoff`.
  - NCP `ESP_NCP_ZCL_WRITE` request used.
- Outcome:
  - OnOff command toggled a test power plug; NCP logs confirmed received fields.

#### Step 11: Add in-memory device registry + `gw devices`
- Why: track devices by IEEE rather than ephemeral short addresses.
- Where:
  - `main/gw_registry.c`/`.h` + bus handler integration.
- Outcome:
  - `gw devices` prints stable IEEE entries and short address changes.

### Phase D: Observability and rejoin debugging (Steps 12-16)

#### Step 12: Add rejoin visibility and raw ZNSP request tool
- Why: the plug appears to rejoin; need to confirm by IEEE vs short churn.
- Changes:
  - More Zigbee signal logs in `gw_zb_app_signal.c`.
  - Registry logs short changes for same IEEE.
  - `znsp req` command to inject raw request IDs.
  - Mutex for host<->NCP request serialization.
- Outcome:
  - Better visibility; low-level ZNSP debugging possible.

#### Step 13: NCP logs include IEEE on device announce
- Why: correlate NCP and host logs quickly.
- Where:
  - `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
- Outcome:
  - NCP logs now show short + IEEE + capability.

#### Step 14: Add NCP rejoin/auth logs + allow `permit_join 0`
- Why: diagnose repeated rejoin by understanding authorization and update signals.
- Host: allow closing permit join (seconds=0).
- NCP: log device update, device authorized, leave details with IEEE.
- Outcome:
  - Builds fixed; duplicate switch-case labels resolved.

#### Step 15: Validate real join and tools
- Why: confirm the system can join a device and control it end-to-end.
- Outcome:
  - Device announce and OnOff commands worked intermittently.
  - Rejoin loop remained: short addresses changed repeatedly.

#### Step 16: Channel mask configuration (attempt to move off channel 13)
- Why: hypothesis that RF interference (channel 13 near Wi-Fi ch1) causes instability.
- Host:
  - Added `zb ch`, `zb mask`, `zb status`, `zb reboot` with NVS persistence.
- NCP:
  - Deferred channel mask application until after `esp_zb_init()` to avoid crash.
  - Added logs for applied masks.
- Outcome:
  - Mask logs show apply OK, but formation still reports channel 13.
  - Hypothesis: Zigbee storage overrides channel or timing is wrong.

### Phase E: Environment blockers and tooling hardening (Steps 17-23)

#### Step 17: Environment blocks serial + DNS
- Problems:
  - DNS resolution failed (rmapi upload blocked).
  - Serial device access denied in this runtime.
- Response:
  - Created scripts for monitor, flash, and storage erase.
  - Added PDF-only export script.
- Outcome:
  - Documentation and reproducible scripts exist, but experiments must run on dev machine.

#### Step 18: Vendor H2 NCP firmware into repo
- Why: make NCP firmware a first-class, versioned dependency.
- Where: `thirdparty/esp-zigbee-sdk/...`
- Outcome:
  - Local builds possible without relying on `/home/manuel/esp/...`.

#### Step 19: Add TCLK/LKE/NVRAM controls (security diagnosis)
- Why: logs suggest authorization timeout (link-key exchange failure).
- Changes:
  - Implemented TCLK get/set, link-key exchange requirement toggle, nvram_erase_at_start.
  - New host console commands: `zb tclk`, `zb lke`, `zb nvram`.
- Outcome:
  - Builds clean, but hardware validation still blocked in this runtime.

#### Step 20: Script Experiment 0 (security-first workflow)
- Why: make "set TCLK, disable LKE, permit join" reproducible.
- Outcome:
  - Script exists (`scripts/30-experiment0-security.sh`), blocked by serial access here.

#### Step 21: Fix monitoring workflow (avoid H2 reset)
- Why: `idf.py monitor` toggles DTR/RTS and resets H2, corrupting UART bus.
- Outcome:
  - tmux monitor script uses `--no-reset` for both H2 and host.

#### Step 22: Add pyserial capture (no TTY required)
- Why: `idf.py monitor` refuses to run without TTY, and it resets the device.
- Outcome:
  - `scripts/11-capture-serial.py` records H2 logs without side effects.

#### Step 23: Pairing attempts and evidence collection
- Why: confirm whether LKE on/off affects join stability.
- Outcome:
  - LKE=on run shows authorization timeout and short churn.
  - LKE=off sometimes sees no join events; timing may be off.
  - Evidence logs captured under `sources/local/runs/...`.

## 6. Deep dive: the most important technical threads

This section extracts the cross-cutting technical threads from the timeline above. These are the "textbook" explanations: what happened, why, where, and what it means.

### 6.1 The host<->NCP protocol gap (permit_join stub)

Early in 0031, permit_join was stubbed on the host because the NCP did not implement the request handler. The NCP already emitted permit-join status notifications, which made the gap easy to miss. The fix required a coordinated host and NCP change:
- Host had to send request ID 0x0005 over ZNSP.
- NCP had to implement the handler (`esp_zb_bdb_open_network(seconds)`) and return status.

This was a pivotal moment: it established that "real Zigbee" changes often require both sides of the link, not just host-side code. That pattern repeated later for security controls and channel masks.

### 6.2 The UART bus is a shared physical link, not a control plane

Multiple times, the UART bus was corrupted by boot logs or resets:
- If the H2 resets while still wired to the host, ROM logs appear on the same UART, which looks like protocol garbage to the host parser.
- `idf.py monitor` toggles DTR/RTS by default; on some boards this resets the target and pollutes the UART.

The response was to treat the UART as a protocol-only link and isolate console logs to USB-Serial/JTAG. This is now baked into:
- host `sdkconfig.defaults` (USB-Serial/JTAG console)
- tmux monitor scripts using `--no-reset`
- custom pyserial capture to avoid TTY requirements

### 6.3 The "rejoin loop" is likely a security failure, not a radio failure

The rejoin loop looked like RF interference at first, but log decoding showed a stronger hypothesis:
- `Device update status=0x01` indicates an unsecured join.
- `Device authorized type=0x01 status=0x01` indicates authorization timeout.
- The same IEEE returns with a new short address.

This indicates a Trust Center key or link-key exchange failure. The response was to add TCLK get/set and LKE requirement toggles, plus a scriptable experiment that disables LKE to confirm the diagnosis. Validation is still pending.

### 6.4 Channel selection stuck on 13: config applied but ignored

Channel selection was addressed with a robust host->NCP config path and NVS persistence. The NCP accepts and logs the applied channel mask, yet forms on channel 13. This suggests:
- Zigbee persistent storage (zb_fct/zb_storage) overrides the config.
- Or the mask is applied too late in the BDB formation sequence.

The next step is a controlled erase of Zigbee storage and a re-test with `zb ch 25` + `zb reboot` to see if formation moves. Scripts exist to do this on a dev machine.

### 6.5 NCP stability fixes were necessary for a clean signal

Several NCP-side issues had to be fixed to make debugging trustworthy:
- Off-by-one loops in dispatch code.
- Passing ephemeral request buffers into `esp_zb_init()` caused crashes.
- Applying channel masks before init crashed the NCP.

These fixes were essential to avoid attributing protocol errors to "noise" when they were actually crashes.

### 6.6 Tooling and evidence capture were elevated to first-class artifacts

During 0031, scripts and log capture became part of the product:
- `scripts/10-tmux-dual-monitor.sh`: safe dual monitor with `--no-reset`.
- `scripts/11-capture-serial.py`: raw capture without TTY or reset.
- `scripts/13-dual-drive-and-capture.py`: pairing runs with dual capture.
- `scripts/20/21/22/30`: flash and experiment scripts.
- Evidence logs stored under `sources/local/runs/`.

This is why the 0031 ticket is still actionable even when the runtime environment blocks direct serial access.

## 7. Related ticket deep dives (condensed but complete)

This section summarizes the key insights and deliverables from each related ticket, with emphasis on how they fed 0031.

### 7.1 001-ZIGBEE-GATEWAY (CoreS3 + H2 bring-up)

Key outcomes:
- Established the two architectures and their implications:
  - RCP + Spinel (ot_rcp on H2, esp_zigbee_gateway on host).
  - NCP + ZNSP (esp_zigbee_ncp on H2, esp_zigbee_host on host).
- Confirmed that the NCP architecture is the correct fit for "host orchestrator + Zigbee NCP" work.

Important failures and fixes:
- Host<->NCP protocol errors traced to missing UART RX on H2, which was a physical cable seating issue.
- Pattern queue reset (`uart_pattern_queue_reset()`) was needed for SLIP pattern detection to work reliably.
- `idf.py monitor` required TTY; `script` used for headless logging.

Why it matters for 0031:
- 0031's host+NCP split, UART pin choices, and console defaults were inherited directly from 001.
- 0031's expectation that the NCP can form and steer a network is grounded in 001's successful handshake logs.

### 7.2 002-ZIGBEE-NCP-UART-LINK (UART smoke test)

Key outcomes:
- Built minimal UART firmware for both host and H2 to confirm wiring.
- Demonstrated that a partially seated Grove cable can mimic protocol bugs.
- Produced a reproducible debug pattern: always run the smoke test before chasing ZNSP framing.

Why it matters for 0031:
- The 0031 rejoin debugging relies on a stable UART link; this ticket defined how to verify it.

### 7.3 0029-HTTP-EVENT-MOCK-ZIGBEE (hub shell)

Key outcomes:
- Stabilized the hub by removing JSON WS from the hot path and quieting logs.
- Introduced nanopb with a protobuf envelope and verified on-device encoding.
- Moved WS encoding to a queue+task bridge to keep bus handlers deterministic.
- Fixed HTTP issues: stack size, wildcard routing, route matching.
- Eventually removed JSON and served a small embedded UI that decodes protobuf frames.

Why it matters for 0031:
- 0031 is architecturally a fork of 0029 with `hub_sim` replaced by a real Zigbee driver. The bus/HTTP/WS patterns are shared.

### 7.4 0029a-ADD-WIFI-CONSOLE (Wi-Fi REPL)

Key outcomes:
- Added a USB-Serial/JTAG REPL for Wi-Fi config and scanning.
- Fixed naming collisions (`wifi_sta_disconnect` -> `hub_wifi_*`).
- Ensured REPL starts on whichever console backend is configured.

Why it matters for 0031:
- 0031's bring-up and debugging rely on the same console workflow and assumptions.

### 7.5 0030-CARDPUTER-CONSOLE-EVENTBUS (event-bus pattern)

Key outcomes:
- Implemented a minimal bus demo where UI task owns esp_event dispatch.
- Added a console monitor with a queue to avoid blocking handlers.
- Proved nanopb integration for bus event encoding.

Why it matters for 0031:
- 0031's `monitor on/off` and event-bus design borrow directly from this pattern.

### 7.6 0032-ANALYZE-NCP-FIRMWARE (NCP architecture guide)

Key outcomes:
- Documented the NCP layered stack: UART -> SLIP -> frame -> command dispatch -> Zigbee stack.
- Described command ID ranges and async vs sync patterns.
- Mapped FreeRTOS tasks and priorities (bus, main, Zigbee).

Why it matters for 0031:
- The NCP modifications in 0031 (permit_join, channel masks, TCLK) rely on understanding where to insert behavior in `esp_ncp_zb.c` and how the bus task processes frames.

### 7.7 0034-ANALYZE-ESP-ZIGBEE-LIB (stack API analysis)

Key outcomes:
- Mapped esp-zigbee-lib API layers: core, APS, ZDO, BDB, platform.
- Clarified signal/callback semantics (esp_zb_app_signal_handler).
- Outlined commissioning and BDB flow, which later helped interpret authorization timeouts.

Why it matters for 0031:
- The host and NCP signal handling in 0031 depends on understanding ZDO/BDB signals and their meanings.

### 7.8 0035-IMPROVE-NCP-LOGGING (debug ergonomics)

Key outcomes:
- Identified command ID, status, and state values that need stringification for readable logs.
- Proposed codegen approaches for mapping command IDs to names.

Why it matters for 0031:
- High-quality logs were essential for decoding rejoin loops and channel issues; this ticket was the precursor to that effort.

## 8. Stopping points and open problems (the "why we stopped" list)

This section distills all known stopping points from the 0031 timeline and the related tickets. Each is stated with root cause and the "where" you should investigate.

### Stop A: Environment blocks serial access and DNS
- Symptom: `/dev/ttyACM*` open fails with permission denied; `rmapi` DNS fails.
- Root cause: runtime constraints (no device access, no DNS).
- Impact: cannot flash or run pairing experiments in this environment.
- Where to continue: use scripts in `ttmp/.../0031.../scripts/` on a dev machine with hardware access.

### Stop B: Device rejoin loop / authorization timeout
- Symptom: repeated `DEVICE_ANNCE`, short address churn, `Device authorized status=0x01` on H2.
- Root cause hypothesis: Trust Center key / link-key exchange failure.
- Status: TCLK and LKE controls implemented but not validated in a clean environment.
- Where to continue: run Experiment 0 (`scripts/30-experiment0-security.sh`) and compare LKE on/off behavior.

### Stop C: Channel mask applies but formation stays on channel 13
- Symptom: NCP logs show mask applied OK, yet formation log says channel 13.
- Root cause hypotheses: persisted Zigbee storage overrides config; timing of mask application.
- Status: no definitive test yet because storage erase was blocked.
- Where to continue: run `scripts/21-h2-erase-zigbee-storage-only.sh` then `zb ch 25` + `zb reboot`.

### Stop D: H2 reset pollutes UART bus
- Symptom: ROM logs appear on protocol UART, SLIP parsing fails.
- Root cause: `idf.py monitor` resets H2 by toggling DTR/RTS.
- Fix: always use `--no-reset` or use pyserial capture.
- Where to continue: keep using scripts; consider host-side auto-resync if `ESP-ROM` seen on UART.

### Stop E: Early NCP crashes due to pointer lifetime and premature config application
- Symptom: NCP crash in `zb_bufpool_storage_allocate()` or when applying channel mask before init.
- Fix: copy config to static storage; delay config application until after `esp_zb_init()`.
- Status: fixed and validated in earlier steps.

## 9. Current state snapshot (as of the last diary entries)

What is confirmed working:
- Cardputer host builds and runs with USB-Serial/JTAG console.
- esp_event bus with monitor and demo commands is stable.
- Host<->NCP protocol path is functional for permit_join and ZCL OnOff.
- Device announces are observed; IEEE and short addresses are visible.
- Registry tracks devices by IEEE; `gw devices` is usable.
- NCP logs include device announce, update, and authorize status.
- TCLK/LKE/NVRAM control commands exist and map to real NCP handlers.

What is not confirmed:
- Channel selection off 13 (requires storage erase test).
- Stable device join without rejoin loop (requires security experiment).
- End-to-end HTTP/WS control plane for 0031 (not started yet; still in console/bus phase).

What is blocked by environment:
- Running hardware experiments in this runtime (serial access denied).
- reMarkable upload (DNS resolution failure).

## 10. Concrete next actions (recommended order)

1) Run Experiment 0 (security-first) on a dev machine with hardware:
   - `scripts/10-tmux-dual-monitor.sh` (with `--no-reset`)
   - `scripts/30-experiment0-security.sh`
   - Observe whether `Device authorized status=0x00` appears and short churn stops.

2) Run Experiment 1 (channel selection) on a dev machine:
   - `scripts/21-h2-erase-zigbee-storage-only.sh`
   - `zb ch 25` then `zb reboot`
   - Confirm formation channel in H2 logs.

3) If security stabilizes:
   - Re-enable LKE (`zb lke on`) and re-test.
   - Add an IEEE-based command path (resolve IEEE -> short) to avoid manual short address tracking.

4) When Zigbee stability is proven:
   - Resume 0031 tasks Phase 3+ (Wi-Fi, HTTP, WS) using the 0029 shell as reference.
   - Implement protobuf HTTP endpoints and WS stream.

5) Optional hardening:
   - Add host-side detection for H2 reset on the UART (ESP-ROM) and resync.
   - Add NCP readback command for current channel/pan to confirm config.

## 11. File and artifact index (where things live)

This is a non-exhaustive but high-signal index of where to look.

### 0031 host firmware
- `0031-zigbee-orchestrator/main/app_main.c`
- `0031-zigbee-orchestrator/main/gw_bus.c`
- `0031-zigbee-orchestrator/main/gw_console.c`
- `0031-zigbee-orchestrator/main/gw_console_cmds.c`
- `0031-zigbee-orchestrator/main/gw_zb.c`
- `0031-zigbee-orchestrator/main/gw_zb_app_signal.c`
- `0031-zigbee-orchestrator/main/gw_registry.c`
- `0031-zigbee-orchestrator/main/Kconfig.projbuild`
- `0031-zigbee-orchestrator/sdkconfig.defaults`
- `0031-zigbee-orchestrator/components/zb_host/`

### NCP firmware (vendored)
- `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`
- `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c`
- `thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/partitions.csv`

### 0031 investigation artifacts
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../sources/local/Zigbee debug logs.md`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../sources/local/runs/2026-01-05-pairing-lke-on-224457/`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../sources/local/runs/2026-01-06-exp0b/`

### 0031 scripts
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/10-tmux-dual-monitor.sh`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/11-capture-serial.py`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/13-dual-drive-and-capture.py`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/20-h2-erase-and-flash.sh`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/21-h2-erase-zigbee-storage-only.sh`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/22-host-flash.sh`
- `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/30-experiment0-security.sh`

## 12. Glossary (short)

- NCP: Network Co-Processor; runs Zigbee stack on ESP32-H2.
- Host: ESP32-S3 orchestrator; runs HTTP, console, and event bus.
- ZNSP: Espressif host<->NCP protocol over SLIP.
- SLIP: Serial Line Internet Protocol; framing for the UART link.
- TCLK: Trust Center Link Key; preconfigured key used during joining.
- LKE: Link Key Exchange; security step in Zigbee commissioning.
- BDB: Base Device Behavior; Zigbee commissioning procedures.
- GW_CMD / GW_EVT: 0031's command and event namespaces on the esp_event bus.

## 13. What this deep dive does not cover

- It does not replace the raw diaries or the investigation logs. It is a synthesis and map, not a data dump.
- It does not include the full HTTP/WS implementation for 0031 because those phases are not executed yet; those exist in 0029.
- It does not attempt to resolve the rejoin loop; it documents the current best hypothesis and the next experiments.

## 14. Closing summary

0031 is not a greenfield project; it is a deliberate fusion of an event-bus orchestration shell (0029/0030) and a validated Zigbee host+NCP architecture (001/002). Its current state is technically strong in terms of architecture and tooling: the console-driven bus, the NCP protocol integration, and the diagnostic scripts are all robust. The remaining issues are not architectural, but operational: security authorization and channel persistence. Solving those two will unlock the remaining phases (Wi-Fi, HTTP, WS, UI) with low risk, because the platform spine already exists.
