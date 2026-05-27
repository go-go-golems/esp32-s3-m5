---
title: Investigation Diary
doc-type: reference
status: active
---

# Diary

## Goal

Record the implementation journey for ticket 0095: CoreS3 SE WiFi/HTTP benchmark firmware.

## Step 1: Ticket creation and design doc

Created ticket 0095 for a CoreS3 SE benchmark firmware to compare native ESP32-S3 WiFi throughput against the Tab5's ESP-Hosted architecture. Wrote a comprehensive intern-guide-style design document covering the single-chip architecture, the SDIO-elimination comparison, TCP window tuning as a new variable, and the iperf-optimized sdkconfig.

### Prompt Context

**User prompt (verbatim):** "I connected a cores3se this time, do the same benchmarking on this kind of chip. Create a new ticket for it. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet points and pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and then upload to remarkable."

**Assistant interpretation:** Create ticket 0095 for CoreS3 SE benchmark, write intern-grade design doc explaining the ESP32-S3 native WiFi architecture and how it differs from the Tab5's ESP-Hosted approach, then upload to reMarkable.

**Inferred user intent:** Get a complete onboarding document for benchmarking the CoreS3 SE, with clear comparison points against the Tab5 data from ticket 0094.

### What I did

- Created ticket 0095 with title and topics
- Wrote 24 KB design doc (7 sections)
- Uploaded to reMarkable at /ai/2026/05/27/0095
- Updated changelog

### What worked

- The architecture comparison table directly highlights the SDIO-elimination experiment
- The iperf-optimized sdkconfig.defaults from the IDF source provides concrete tuning values
- Reuse of bench_server.c from 0094 reduces implementation effort

### What didn't work

- The CoreS3 SE is not showing up on USB yet (only ttyACM0 = Tab5 JTAG, ttyACM1 = Loupedeck). May need the user to verify the USB connection.

### What was tricky to build

- Distinguishing the "SE" variant: the CoreS3 SE has 8 MB Quad PSRAM (not 8 MB Octal + 8 MB OMAP like the full CoreS3). The PSRAM size affects the maximum test payload and buffer allocation strategy.

### What warrants a second pair of eyes

- The WiFi PHY rate comparison: ESP32-S3 supports 802.11n only (72 Mbps max), while the Tab5's C6 supports 802.11ax. Both will negotiate 802.11n with the same AP, so the comparison is fair in practice.

### What should be done in the future

- Build and flash the firmware once the CoreS3 SE is visible on USB
- Run benchmarks with both default and optimized sdkconfig
- Compare CoreS3 SE results against Tab5 STA data

### Code review instructions

- Read design doc: `design-doc/01-cores3-se-benchmark-firmware-design-and-intern-guide.md`
- Verify the sdkconfig.defaults match the ESP-IDF iperf example for ESP32-S3
- Verify the architecture comparison table against Tab5 specs
