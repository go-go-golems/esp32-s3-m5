---
Title: 'Technical Memo (PARC): 0031 Zigbee Orchestrator Retrospective'
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
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-02-02T00:00:00-05:00
WhatFor: ""
WhenToUse: ""
---

Xerox PARC Technical Memo

To: Project stakeholders, new contributors, and firmware owners
From: Codex analysis agent
Subject: 0031 Zigbee Orchestrator Retrospective (host+NCP, bus-first architecture)
Date: 2026-02-02
Keywords: Zigbee, ESP32-S3, ESP32-H2, NCP, ZNSP, SLIP, esp_event, nanopb, HTTP 202

Abstract

This memo summarizes the full 0031 Zigbee Orchestrator effort, its architectural lineage, and the current stopping points. The system is a Cardputer (ESP32-S3) host orchestrator that exposes a bus-first control plane with an eventual HTTP+protobuf interface while delegating Zigbee stack/radio to an ESP32-H2 NCP over UART (ZNSP over SLIP). The work inherited a proven event-bus/protobuf shell from 0029/0030 and a validated host+NCP bring-up from 001/002. The major unresolved issues are (1) authorization timeouts during commissioning (likely Trust Center key / link-key exchange) and (2) channel selection persistence (mask applied but network forms on channel 13), both requiring on-device experiments that were blocked in this runtime. A complete, source-backed deep dive is stored in `analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md` within ticket 0069.

1. Background

The 0031 project is not greenfield. It is the convergence of two prior tracks:
- Zigbee host+NCP bring-up on CoreS3/Unit Gateway H2 (tickets 001 and 002), which established the two-chip architecture, UART wiring, and the host<->NCP protocol behavior.
- A bus-first HTTP/protobuf shell (tickets 0029/0029a/0030), which proved the esp_event bus, nanopb-based envelopes, and a console workflow that scales to firmware bring-up.

The design decision, recorded before implementation, is that the host must remain a Wi-Fi capable ESP32-S3 while Zigbee runs on the ESP32-H2 NCP. This implies a strict boundary: the host never calls Zigbee stack APIs directly; it only issues host<->NCP requests and consumes asynchronous notifications.

2. System overview (what we built)

Host (ESP32-S3 / Cardputer):
- `esp_event` application bus (`GW_EVT` base) with command and event IDs.
- `esp_console` REPL over USB-Serial/JTAG (preferred; UART console disabled by default).
- Zigbee host-side protocol client (`components/zb_host/`).
- Internal registry keyed by IEEE (short addresses tracked as volatile).
- Diagnostic console commands for Zigbee operations (`gw post`, `zb`, `znsp`).

NCP (ESP32-H2 / Unit Gateway H2):
- Zigbee stack + NCP request handlers in `esp_ncp_zb.c`.
- UART bus (SLIP framed) for host protocol (ZNSP-like).
- Logging enhancements for join/update/authorize signals and security status.

3. Chronology (condensed)

Phase 0-1: Design and scaffolding
- Created 0031 ticket and analysis/design docs; locked host target (Cardputer) and host+NCP split.
- Forked 0029 into `0031-zigbee-orchestrator/`, brought up bus + console with a `monitor` command.

Phase 2: Permit-join and ZCL control
- Found that NCP lacked a request handler for permit-join; implemented it on H2.
- Integrated host-side NCP stack and a Zigbee worker task on the host.
- Validated end-to-end permit-join and ZCL OnOff commands using real hardware.

Phase 3: Registry and observability
- Added IEEE-keyed registry (`gw devices`).
- Added more signal logging and a raw ZNSP request tool (`znsp req`) for debugging.
- Implemented H2 logs that include IEEE for announces and updates.

Phase 4: Rejoin loop + channel diagnostics
- Observed repeated rejoin/short churn and authorization timeout signals.
- Added channel mask controls with NVS persistence and safe apply timing.
- Channel mask applied successfully but formation still on channel 13.

Phase 5: Tooling hardening and security diagnosis
- Environment blocked serial/DNS access; added scripts for capture/flash/experiments.
- Vendored NCP firmware into repo to make it reviewable and reproducible.
- Added Trust Center key (TCLK) get/set, link-key exchange (LKE) toggle, and nvram erase controls.

4. Key findings

4.1 Host+NCP is the correct architecture
- Host must provide Wi-Fi + HTTP; H2 is Zigbee-only. Two-chip split is required and validated.

4.2 Bus-first design scales and is stable
- Console + esp_event monitor validated the bus approach before HTTP/WS was introduced.
- Protobuf is used as an IDL boundary rather than an in-memory payload format.

4.3 Permit-join required NCP-side implementation
- Host-side requests alone are insufficient when NCP handlers are stubbed.
- Dual-sided patches are often required for "real Zigbee" features.

4.4 Authorization timeout is the leading cause of rejoin loops
- H2 logs show unsecured join followed by authorization timeout.
- This strongly implicates Trust Center key / link-key exchange as the root issue.

4.5 Channel masks are accepted but ignored at formation
- Mask apply returns OK, yet formation stays on channel 13.
- Likely overridden by persisted Zigbee storage or applied at the wrong time.

5. Stopping points and blockers

- Hardware access blocked in this runtime (serial open fails, DNS failures for rmapi). This prevented the decisive experiments.
- Security experiment (TCLK/LKE) is prepared but not validated here.
- Channel selection experiment requires erasing Zigbee storage to test persistence override.

6. Recommendations (next actions)

Priority order:
1) Run Experiment 0 (security-first): set default TCLK, set LKE off, permit-join, observe authorization success and short stabilization.
2) Run Experiment 1 (channel selection): erase zb_fct/zb_storage, set channel mask, re-form, confirm channel.
3) If security stabilizes, re-enable LKE and confirm with the same device.
4) Implement IEEE-based command helpers to avoid manual short address usage.
5) Resume HTTP/WS phases from 0031 tasks once Zigbee stability is proven.

7. Related tickets (source set)

Core:
- 0031-ZIGBEE-ORCHESTRATOR (analysis, design, investigation, diary)

Foundations:
- 001-ZIGBEE-GATEWAY (host+NCP bring-up)
- 002-ZIGBEE-NCP-UART-LINK (UART smoke test)
- 0029-HTTP-EVENT-MOCK-ZIGBEE (bus/HTTP/protobuf shell)
- 0029a-ADD-WIFI-CONSOLE (console Wi-Fi UX)
- 0030-CARDPUTER-CONSOLE-EVENTBUS (esp_event pattern + nanopb)

Deep stack analysis:
- 0032-ANALYZE-NCP-FIRMWARE
- 0034-ANALYZE-ESP-ZIGBEE-LIB
- 0035-IMPROVE-NCP-LOGGING

Post-0031 follow-ons (awareness only):
- 0067-zigbee-powerplug
- 0068-ZIGBEE-SNIFFING

8. References

- Full deep dive: `analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md`
- Primary investigation: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md`
- Key scripts: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--.../scripts/`
