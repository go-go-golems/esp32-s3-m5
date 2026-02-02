---
Title: Research memo (Bell Labs/Xerox PARC style)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Concise research memorandum on Zigbee power plug integration, validation outcomes, and next experiments."
LastUpdated: 2026-01-31T12:52:01-05:00
WhatFor: "Document technical progress, findings, and next experiments in a formal lab memo style."
WhenToUse: "Use for internal lab records, design reviews, or onboarding new contributors."
---

# Research Memorandum

**To:** Zigbee Systems Group
**From:** Research Engineering
**Date:** 2026-01-31
**Subject:** Status and validation results for Zigbee power plug integration (3RSP02028BZ)

## Summary

We established a reproducible Zigbee2MQTT + Mosquitto stack and validated core MQTT bridge interactions with a live coordinator. The system is operational; the primary nuance is that several bridge requests publish results on state topics rather than response topics. Documentation now captures the correct behavior and the port mapping required on this host.

## Background

The project aims to integrate the Third Reality Gen 2 plug (3RSP02028BZ) via Zigbee2MQTT, provide a clear operator workflow (pairing, control, troubleshooting), and enable RF capture and analysis for joining behavior. A secondary goal is to validate bridge request/response semantics for automation tooling.

## Work Completed

1. **Reference docs**: Quickstart and command compendium for Zigbee2MQTT MQTT usage, plus a CLI sniffing playbook.
2. **Validation run**: Live coordinator test with tmux-run services, Mosquitto broker, and bridge request tests.
3. **Postmortem and playbooks**: A documented validation run with corrections based on official Zigbee2MQTT docs.

## Findings

- Zigbee2MQTT is stable under the tested configuration and resumes correctly with the coordinator.
- `bridge/request/permit_join` responds on `bridge/response/permit_join` as documented.
- `bridge/request/info`, `bridge/request/devices`, and `bridge/request/definitions` publish on state topics (`bridge/info`, `bridge/devices`, `bridge/definitions`).
- `bridge/request/logging` is not a documented endpoint. Log-level changes are via `bridge/request/options` with `advanced.log_level`.
- Host port 1883 was occupied; broker access required mapping to 1884 for tests.

## Implications

Automation and monitoring should subscribe to state topics for inventory and definitions, and use explicit request/response only when the command is documented. For health/logging, separate state topics and configuration endpoints must be used. Test harnesses should parameterize the host broker port.

## Risks and Open Questions

- Some bridge requests appear version-sensitive; confirm in future Zigbee2MQTT releases.
- Device-specific exposes for the 3RSP02028BZ are not yet validated on this stack.
- Health check responses may be timing-sensitive or dependent on configuration interval.

## Next Experiments

1. Pair 3RSP02028BZ and validate device exposes (state, power_on_behavior, energy metrics).
2. Capture join traffic with the nRF sniffer and confirm decryption workflow.
3. Re-run health_check and logging changes with extended timeouts and response-topic subscriptions.
4. Run OTA update check if device supports OTA.

## Appendices

**Test stack**
- Coordinator: Sonoff Zigbee 3.0 USB Dongle Plus
- Zigbee2MQTT: v2.7.2
- Mosquitto: host 1884 -> container 1883

**Key command (log level)**
```bash
mosquitto_pub -h localhost -p 1884 \
  -t 'zigbee2mqtt/bridge/request/options' \
  -m '{"options":{"advanced":{"log_level":"info"}}}'
```
