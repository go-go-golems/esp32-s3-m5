---
Title: Investigation Diary
Ticket: ESP32-P4-PICOCALC-AUDIO
Status: active
Topics:
    - esp32-p4
    - picocalc
    - audio
    - i2s
    - es8311
    - pwm
    - firmware-port
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for audio design on ESP32-P4 PicoCalc"
LastUpdated: 2026-06-01T22:30:00-04:00
WhatFor: "Track audio design and investigation"
WhenToUse: "Resume or review the audio design effort"
---

# Diary

## Goal

Track the creation of a design guide for audio output on the ESP32-P4 PicoCalc, covering the Waveshare ES8311 codec path and the PicoCalc PWM speaker path.

## Step 1: Ticket Created and Design Guide Written

Created the ticket and wrote a detailed intern-facing design guide. The Waveshare ES8311 path is well-defined: I2S on GPIO9-13, I2C on GPIO7/8 at address 0x18, PA enable on GPIO53. The PicoCalc PWM speaker path needs GPIO discovery.

### What I did

- Created ticket ESP32-P4-PICOCALC-AUDIO.
- Added design doc and diary doc.
- Gathered ESP-IDF I2S driver docs, Waveshare ES8311 examples, and PicoCalc speaker specs.
- Stored research sources in `sources/`.
- Wrote the design guide with audio API, tone generation, WAV playback, and PWM fallback path.

### What was tricky to build

The ES8311 uses the Waveshare onboard I2C bus (GPIO7/8), which is separate from the keyboard I2C bus (GPIO50/49). The firmware needs to manage two I2C buses. The NS4150B PA enable on GPIO53 must be driven high for speaker output and low when idle.

### What should be done in the future

- Initialize the Waveshare I2C bus (GPIO7/8) and detect ES8311.
- Implement I2S initialization and test tone output.
- Discover PicoCalc PWM speaker GPIOs for the same-position adapter.
