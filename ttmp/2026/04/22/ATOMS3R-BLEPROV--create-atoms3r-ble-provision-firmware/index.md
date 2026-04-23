---
Title: Create ATOMS3R BLE-Provision Firmware
Ticket: ATOMS3R-BLEPROV
Status: active
Topics:
    - firmware
    - esp32
    - ble
    - provisioning
    - ios
    - m5stack
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0090-m5printer-research/source/ATOM-PRINTER/examples/PRINTER_FW/ATOM_PRINTER_WIFI.cpp
      Note: Current WiFi implementation to extend
    - Path: esp32-s3-m5/0090-m5printer-research/source/ATOM-PRINTER/examples/PRINTER_FW/PRINTER_FW.ino
      Note: Current printer firmware to modify
    - Path: esp32-s3-m5/0091-m5printer-ble-provision/reference/07-wifi_prov_example.md
      Note: Example code for wifi_prov
    - Path: esp32-s3-m5/0091-m5printer-ble-provision/reference/12-wifi_provisioning_api.md
      Note: Reference for wifi_provisioning API
    - Path: esp32-s3-m5/ttmp/2026/04/22/ATOMS3R-BLEPROV--create-atoms3r-ble-provision-firmware/sources/ATOMS3R_BLE_PROVISION/ATOMS3R_BLE_PROVISION.ino
      Note: Main Arduino sketch with BLE provisioning integration
    - Path: esp32-s3-m5/ttmp/2026/04/22/ATOMS3R-BLEPROV--create-atoms3r-ble-provision-firmware/sources/ATOMS3R_BLE_PROVISION/ATOM_PRINTER_BLE_PROV.cpp
      Note: BLE GATT server implementation using NimBLE-Arduino
ExternalSources: []
Summary: ""
LastUpdated: 2026-04-22T20:34:07.856741601-04:00
WhatFor: ""
WhenToUse: ""
---







# Create ATOMS3R BLE-Provision Firmware

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- firmware
- esp32
- ble
- provisioning
- ios
- m5stack

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
