---
Title: UART Smoke Test Firmware (CoreS3 Port C ↔ Unit Gateway H2)
Ticket: 002-ZIGBEE-NCP-UART-LINK
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32
    - esp32h2
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/run_ncp_host_and_h2_ncp_tmux.sh
      Note: tmux helper for paired monitoring/capture
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/uart_smoke/cores3_uart_smoke/main/uart_smoke.c
      Note: CoreS3 UART1 ping transmitter + RX logger (pins 17/18)
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/uart_smoke/h2_uart_smoke/main/uart_smoke.c
      Note: H2 UART1 RX newline parser + pong responder (pins 24/23)
ExternalSources: []
Summary: 'Build and flash two minimal ESP-IDF firmwares (CoreS3 + H2) to prove the Grove Port C UART link and pin mapping (S3: RX18/TX17, H2: RX23/TX24) before debugging Zigbee NCP protocol.'
LastUpdated: 2026-01-04T18:17:24.595014965-05:00
WhatFor: Quickly validate electrical UART connectivity and correct pin mapping between CoreS3 Port C and Unit Gateway H2.
WhenToUse: When the Zigbee NCP host blocks waiting for responses, you suspect RX/TX wiring or cable seating issues, or you want to confirm directionality before protocol debugging.
---


# UART Smoke Test Firmware (CoreS3 Port C ↔ Unit Gateway H2)

## Purpose

Prove the UART link (both directions) between:
- CoreS3 (ESP32-S3) on Grove Port C, and
- Unit Gateway H2 (ESP32-H2),

without any Zigbee/NCP protocol in the loop.

## Environment Assumptions

- ESP-IDF 5.4.1 is available at `~/esp/esp-5.4.1` (this is a symlink to `~/esp/esp-idf-5.4.1`).
- Devices are connected by USB:
  - CoreS3 console on `/dev/ttyACM0`
  - H2 console on `/dev/ttyACM1`
- Devices are connected by Grove cable:
  - CoreS3 Port C ↔ Unit Gateway H2 Grove UART

### Pin assumptions (from the M5Stack guide)
- CoreS3 host UART pins: RX=GPIO18, TX=GPIO17
- H2 NCP UART pins: RX=GPIO23, TX=GPIO24

If the smoke test fails, the first thing to try is reseating the Grove cable and confirming Port C.

## Commands

### 0) Projects

This ticket includes two minimal ESP-IDF projects:
- CoreS3: `.../scripts/uart_smoke/cores3_uart_smoke`
- H2: `.../scripts/uart_smoke/h2_uart_smoke`

### 1) Build + flash CoreS3 (TX on GPIO17, RX on GPIO18)
```bash
source ~/esp/esp-5.4.1/export.sh
cd esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/uart_smoke/cores3_uart_smoke
idf.py set-target esp32s3
idf.py -p /dev/ttyACM0 build flash
idf.py -p /dev/ttyACM0 monitor
```

Expected:
- Periodic `TX ->` logs on the CoreS3 console
- If H2 responds, you’ll see `RX <-` logs too

### 2) Build + flash H2 (TX on GPIO24, RX on GPIO23)
```bash
source ~/esp/esp-5.4.1/export.sh
cd esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/uart_smoke/h2_uart_smoke
idf.py set-target esp32h2
idf.py -p /dev/ttyACM1 build flash
idf.py -p /dev/ttyACM1 monitor
```

Expected:
- `RX <-` logs whenever CoreS3 transmits
- `TX ->` logs showing a response (the H2 firmware replies to each line it receives)

### 3) Quick interpretation

If CoreS3 prints `TX -> ...` but H2 prints no `RX <- ...`:
- cable/port/ground is wrong, *or*
- RX/TX mapping is wrong for this cable/adapter

If H2 prints `RX <- ...` but CoreS3 prints no `RX <- ...`:
- reverse-direction issue (H2 TX not reaching CoreS3 RX), likely wiring/ground/pinout.

## Exit Criteria

- With both firmwares running, you see:
  - H2 `RX <-` lines matching CoreS3 `TX ->` lines, and
  - CoreS3 `RX <-` lines matching H2 `TX ->` lines.

Once this is true, move back to the Zigbee NCP host/NCP examples (protocol debugging).

## Notes

- If either side appears completely silent on RX, reseat the Grove cable first (it’s the most common failure mode).
- If you suspect cross-over wiring, swap RX/TX pins in *one* firmware temporarily and retry. If swapping makes it work, document the cable behavior.
