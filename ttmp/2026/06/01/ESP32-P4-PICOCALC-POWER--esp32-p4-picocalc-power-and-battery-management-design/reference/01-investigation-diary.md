---
Title: Investigation Diary
Ticket: ESP32-P4-PICOCALC-POWER
Status: active
Topics:
    - esp32-p4
    - picocalc
    - power
    - battery
    - axp2101
    - firmware-port
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for power and battery management design on ESP32-P4 PicoCalc"
LastUpdated: 2026-06-01T22:30:00-04:00
WhatFor: "Track power and battery management design"
WhenToUse: "Resume or review the power management design effort"
---

# Diary

## Goal

Track the creation of a design guide for power and battery management on the ESP32-P4 PicoCalc, covering battery monitoring via the STM32 southbridge I2C register and power-off control.

## Step 1: Ticket Created and Design Guide Written

Created the ticket and wrote a detailed intern-facing design guide. Key discovery: the PicoCalc battery and power-off are controlled through the STM32 southbridge at I2C address 0x1F, registers 0x0B (battery) and 0x0E (power off). The AXP2101 PMIC is managed by the STM32, not directly by the ESP32-P4.

### What I did

- Created ticket ESP32-P4-PICOCALC-POWER.
- Added design doc and diary doc.
- Gathered AXP2101 datasheet and driver references.
- Stored research sources in `sources/`.
- Wrote the design guide with battery API, power-off API, phases, and risk analysis.

### What was tricky to build

The battery register 0x0B data format is not fully documented. The first implementation phase is a register discovery exercise. The AXP2101 I2C accessibility from the ESP32-P4 is also unknown.

### What should be done in the future

- Discover register 0x0B format experimentally.
- Check if AXP2101 is reachable from ESP32-P4 I2C bus.
- Implement Phase 1: battery register discovery.
