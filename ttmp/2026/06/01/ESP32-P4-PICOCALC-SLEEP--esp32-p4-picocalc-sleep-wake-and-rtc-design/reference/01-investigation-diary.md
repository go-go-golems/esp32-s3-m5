---
Title: Investigation Diary
Ticket: ESP32-P4-PICOCALC-SLEEP
Status: active
Topics:
    - esp32-p4
    - picocalc
    - sleep
    - rtc
    - deep-sleep
    - light-sleep
    - wake
    - firmware-port
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for sleep, wake, and RTC design on ESP32-P4 PicoCalc"
LastUpdated: 2026-06-01T22:30:00-04:00
WhatFor: "Track sleep, wake, and RTC design and investigation"
WhenToUse: "Resume or review the sleep/wake/RTC design effort"
---

# Diary

## Goal

Track the creation of a design guide for sleep modes, wake sources, and RTC on the ESP32-P4 PicoCalc handheld.

## Step 1: Ticket Created and Design Guide Written

Created the ticket and wrote a detailed intern-facing design guide covering light sleep, deep sleep, RTC time keeping, wake sources, and the interaction with the STM32 southbridge power-off control.

### What I did

- Created ticket ESP32-P4-PICOCALC-SLEEP.
- Added design doc and diary doc.
- Gathered ESP-IDF sleep modes documentation, Waveshare RTC battery header details.
- Stored research sources in `sources/`.
- Wrote the design guide with sleep API, RTC API, wake sources, idle policy, and peripheral re-init strategy.

### What was tricky to build

The most important open question is whether the STM32 southbridge has a hardware interrupt output pin connected to the ESP32-P4. Without a hardware interrupt, the ESP32-P4 cannot wake from sleep on keyboard activity directly. The workaround is periodic RTC timer wake + keyboard I2C poll.

### What should be done in the future

- Discover if STM32 has an interrupt pin routed to ESP32-P4.
- Implement light sleep with timer wake as the first step.
- Test deep sleep and measure current consumption.
- Add RTC time keeping and verify across sleep cycles.
