---
Title: 'Bugreport: CoreS3↔H2 NCP UART link appears dead'
Ticket: 002-ZIGBEE-NCP-UART-LINK
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32
    - esp32h2
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c
      Note: NCP bus UART RX instrumentation and pattern detect
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_main.c
      Note: Fix startup queue race (bus task could emit before queue exists)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_bus.c
      Note: Host bus RX frame alignment (pattern detect) and TX debug logs
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_frame.c
      Note: TX debug log for initial host frame
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_main.c
      Note: Fix startup queue race (bus task could emit before queue exists)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/sdkconfig.defaults
      Note: Move console to USB to keep bus UART pins quiet
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-cores3-host.log
      Note: UART smoke test (CoreS3) shows TX and RX working
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-h2-ncp.log
      Note: UART smoke test (H2) shows RX and TX working
ExternalSources: []
Summary: Following M5Stack’s zigbee_ncp guide, CoreS3 host TX is confirmed but Unit Gateway H2 NCP sees no UART RX bytes; requires UART framing fixes and may be a physical connection issue (Grove cable seating/ground/pinout).
LastUpdated: 2026-01-04T18:17:23.041605559-05:00
WhatFor: Provide a reproducible bugreport + delta vs the M5Stack guide, with concrete diagnostics to distinguish firmware/protocol issues from physical UART wiring issues.
WhenToUse: When the CoreS3↔Unit Gateway H2 NCP setup fails to connect (host blocks waiting for NCP response, invalid frames, or apparent dead UART link).
---


# Bugreport: CoreS3↔H2 NCP UART link appears dead

## Goal

Document the current state of the CoreS3 (ESP32-S3) host ↔ Unit Gateway H2 (ESP32-H2) NCP UART link bring-up (per M5Stack `zigbee_ncp` guide), including:
- What we changed vs the guide (required to even get the examples running reliably)
- What we observed (host TX confirmed; H2 sees no UART RX)
- How to reproduce and collect logs
- How to conclusively validate the physical UART link (see smoke test playbook)

## Context

### Upstream guide being followed
- https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_ncp

### Hardware
- Host: M5Stack CoreS3 (ESP32-S3) connected to computer (USB Serial/JTAG)
- NCP: M5Stack Unit Gateway H2 (ESP32-H2) connected to computer (USB Serial/JTAG)
- UART interconnect: CoreS3 Grove Port C ↔ Unit Gateway H2 Grove (UART)

### Expected behavior (per guide)
- H2 (`esp_zigbee_ncp`) initializes and exchanges SLIP-framed ZNSP protocol with host
- CoreS3 (`esp_zigbee_host`) connects to NCP and proceeds to Zigbee formation/steering

### Observed behavior (before cable reseat)
- CoreS3 host logs show it transmits a SLIP frame on its configured host bus UART.
- H2 NCP logs show *no evidence of receiving any UART bytes at all* on its configured NCP bus UART RX.
- Result: host blocks waiting for a response and never progresses to the “Normal Running Log Content” described by the guide.

This “host TX but NCP sees 0 RX bytes” strongly suggests a physical/wiring issue (Grove cable not seated, wrong port, missing ground), but we also uncovered upstream software issues that can masquerade as link problems (see next section).

### Observed behavior (after cable reseat)
- A minimal UART smoke test shows **bidirectional byte exchange** on the expected pins:
  - CoreS3: TX=GPIO17, RX=GPIO18
  - H2: TX=GPIO24, RX=GPIO23
- This confirms the previous “dead UART” symptom was very likely caused by the Grove cable not being fully seated.

### Observed behavior (NCP protocol, after fixing UART pattern queue reset)
- `esp_zigbee_host` and `esp_zigbee_ncp` now exchange SLIP-framed ZNSP packets (pattern detection triggers and frames decode on both sides).
- Both sides reach “network formation” and “network steering started”, matching the M5Stack guide’s expected “Normal Running Log Content”.

## Quick Reference

### Pin mapping (from M5Stack guide)

| Board | Role | Bus | UART | RX pin | TX pin |
|------:|------|-----|------|--------|--------|
| CoreS3 | Host | Zigbee NCP Host | UART1 | GPIO18 | GPIO17 |
| Unit Gateway H2 | NCP | Zigbee Network Co-processor | UART1 | GPIO23 | GPIO24 |

### What we had to change vs the M5Stack guide (so far)

The guide is “happy-path”; these changes were required in this environment to make debugging possible / avoid false failures:

1) **SLIP crash fix (upstream bug)**  
`slip.c` in the host and NCP code created a FreeRTOS stream buffer with a trigger level of `8` even when the buffer is smaller (e.g., when `inlen < 8`), which asserts:
- `assert failed: xStreamBufferGenericCreate ... (xTriggerLevelBytes <= xBufferSizeBytes)`

Patched files (in `~/esp/esp-zigbee-sdk`):
- `examples/esp_zigbee_host/components/src/slip.c`
- `components/esp-zigbee-ncp/src/slip.c`

2) **Host frame reassembly / alignment**  
Host originally fed arbitrary `UART_DATA` chunks into `slip_decode()` and `esp_host_frame_input()`. If SLIP frames arrive split across UART events, decode/parsing fails. We changed host bus RX to use `SLIP_END (0xC0)` pattern detection and forward one complete frame at a time.

Patched file:
- `examples/esp_zigbee_host/components/src/esp_host_bus.c`

2b) **Pattern detection queue reset (required)**  
ESP-IDF UART pattern detection requires a pattern queue; without calling `uart_pattern_queue_reset()`, `UART_PATTERN_DET` may never fire and the RX path stalls.

Patched files:
- `examples/esp_zigbee_host/components/src/esp_host_bus.c`
- `components/esp-zigbee-ncp/src/esp_ncp_bus.c`

3) **Startup queue race**  
Both host and NCP started the bus task before creating the event queue. Early events could be dropped. We reordered startup so the queue exists before the bus can post events.

Patched files:
- `examples/esp_zigbee_host/components/src/esp_host_main.c`
- `components/esp-zigbee-ncp/src/esp_ncp_main.c`

4) **Keep NCP UART pins quiet (avoid console noise)**  
Moved H2 console to USB Serial/JTAG and reduced bootloader log verbosity so the bus UART pins aren’t polluted with boot log ASCII.

Patched file:
- `examples/esp_zigbee_ncp/sdkconfig.defaults` (and then forced regeneration of `sdkconfig` so defaults applied)

5) **Monitoring/log capture (environment constraint)**  
Used a tmux helper script to run paired monitors and capture logs into a ticket directory.

Script:
- `.../scripts/run_ncp_host_and_h2_ncp_tmux.sh`

### Current “minimal reproducer” (host + ncp)

Host (CoreS3):
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_host
idf.py -p /dev/ttyACM0 build flash
```

NCP (H2):
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp
idf.py -p /dev/ttyACM1 build flash
```

Paired log capture (60s):
```bash
DURATION_SECONDS=60 CORES3_PORT=/dev/ttyACM0 H2_PORT=/dev/ttyACM1 \
  esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/run_ncp_host_and_h2_ncp_tmux.sh
```

### Evidence (logs)

Bidirectional UART smoke test success (PING/PONG):
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-cores3-host.log`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-h2-ncp.log`

ZNSP/SLIP NCP protocol handshake + network formation:
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-cores3-host.log`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-h2-ncp.log`

### Physical link checklist

When H2 shows **zero UART RX bytes**, verify:
- Grove cable is fully seated on both ends (Port C on CoreS3).
- H2 is actually connected to CoreS3 via Grove (not just USB power).
- Common ground exists over the Grove cable (USB power alone is not a UART ground).
- If the cable/adapter crosses TX/RX unexpectedly, swap RX/TX on one side *temporarily* (or use smoke tests to prove directionality).

## Usage Examples

### Verify the UART link *before* debugging Zigbee protocol
Run the smoke-test playbook in:
- `.../playbook/01-uart-smoke-test-firmware-cores3-port-c-unit-gateway-h2.md`

If the smoke test does not show bytes received on H2 when CoreS3 transmits, treat this as a wiring/hardware problem first.

## Related

- Ticket with full bring-up diary and protocol comparisons: `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/index.md`
- UART smoke test playbook: `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/playbook/01-uart-smoke-test-firmware-cores3-port-c-unit-gateway-h2.md`
