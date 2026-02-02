---
Title: Lab status slides (Obsidian)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Obsidian Slides deck summarizing project status, results, issues, and next steps for the Zigbee power plug research work."
LastUpdated: 2026-01-31T12:52:01-05:00
WhatFor: "Provide a concise, lab-ready status presentation in Obsidian Slides format."
WhenToUse: "Use for lab meetings or stakeholder briefings on project progress and validation outcomes."
---

# Project Status: Zigbee Power Plug Research

---

## Executive Summary

- Validated Zigbee2MQTT + Mosquitto stack in tmux with live coordinator
- Confirmed bridge request behaviors and corrected expectations from docs
- Captured reproducible workflow (playbooks) and postmortem
- Uploaded key docs to reMarkable for field use

---

## Project Scope

- Device: Third Reality Gen 2 plug (3RSP02028BZ)
- Stack: Zigbee2MQTT + Mosquitto + coordinator
- Focus: pairing, MQTT control, bridge request validation, sniffing workflow

---

## Current Status (Summary)

- Core docs complete: quickstart, command compendium, sniffing playbook
- Validation run complete with live coordinator
- Known issue: some bridge requests publish to state topics, not response topics
- Documentation corrected and verified against Zigbee2MQTT docs

---

## System Under Test

- Coordinator: Sonoff Zigbee 3.0 USB Dongle Plus
- Zigbee2MQTT: v2.7.2
- MQTT broker: Mosquitto (host port 1884 -> container 1883)
- Host tools: tmux, mosquitto_pub/sub

---

## Key Artifacts Produced

- Quickstart reference (Docker, pairing, MQTT control)
- MQTT command compendium (bridge + device commands)
- Sniffing playbook (nRF 802.15.4, CLI, decryption)
- Validation playbook + postmortem
- Step 9 verification report (doc-corrected behavior)

---

## Validation Results (Bridge Requests)

- permit_join -> response on `bridge/response/permit_join`
- info/devices/definitions -> publish on state topics
- health_check -> documented request; response topic must be watched
- logging -> not a request endpoint; use `bridge/request/options`

---

## Issue: Port Conflict

- Host port 1883 already in use
- Resolution: map Mosquitto to host 1884 (internal 1883 unchanged)
- Impact: none on Zigbee2MQTT, minor test command changes

---

## Sniffing Workflow (Summary)

- OTA capture via nRF 802.15.4 sniffer
- Must align sniffer channel with Zigbee2MQTT config
- Decryption requires network key (and sometimes Trust Center key)
- CLI-first capture -> pcapng -> tshark/Wireshark

---

## Risks / Open Questions

- health_check/logging behavior may vary by Zigbee2MQTT version
- device list responses may only publish on state topics
- need to validate with actual plug joined (currently coordinator-only)

---

## Next Experiments

- Pair 3RSP02028BZ and validate device-specific exposes
- Confirm health_check response with extended timeout + response topic
- Run OTA update check (if device supports)
- Capture join trace with nRF sniffer for on-air verification

---

## Decision Log

- Use host port 1884 to avoid conflicting services
- Treat bridge/state topics as authoritative when documented
- Store all scripts under ticket scripts for traceability

---

## References

- Zigbee2MQTT MQTT topics and messages
- Zigbee2MQTT health and logging docs
- Ticket playbooks and postmortem

---

## Appendix: Key Commands

```bash
# Permit join
mosquitto_pub -h localhost -p 1884 \
  -t 'zigbee2mqtt/bridge/request/permit_join' \
  -m '{"time": 60}'

# Device inventory (state topic)
mosquitto_sub -h localhost -p 1884 \
  -t 'zigbee2mqtt/bridge/devices' -C 1 &
mosquitto_pub -h localhost -p 1884 \
  -t 'zigbee2mqtt/bridge/request/devices' -m '{}'
wait
```
