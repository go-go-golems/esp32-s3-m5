---
Title: "Changelog: GROVE GPIO signal tester (GPIO1/GPIO2)"
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - gpio
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Chronological record of decisions and changes for the signal tester firmware ticket."
LastUpdated: 2025-12-25T00:00:00.000000000+00:00
WhatFor: "Keep a tight history of design choices so future debugging doesn’t re-litigate old conclusions."
WhenToUse: "Whenever we change scope, CLI contract, or hardware assumptions."
---

## 2025-12-25

- Created ticket and initial spec + tasks.

- Field observation (scope): Cardputer TX looks like normal UART (0–3.3V) when unloaded, but when AtomS3R is connected
  the waveform becomes ~**1.5V–3.3V** (low level no longer reaches ~0V).
  - Interpretation: this strongly suggests the line is being **pulled up or actively driven** by something on the AtomS3R side
    (i.e., not behaving like a high-impedance UART RX input), or the signal is landing on the wrong pin.
  - Hypotheses captured in spec: TX↔TX contention, wrong G1/G2 mapping, strong pull-ups/external circuitry, ground/reference issues.

- Related debugging note (from `0013`): flashing **bootloader + app** (not app-only) removed a misleading early-boot symptom where the
  “wrong” GROVE pin appeared to carry UART output for ~400ms. With a full flash, boot-to-`esp_console` prompt output became consistent
  on the configured TX pin. This matters for this ticket because early-boot waveforms can otherwise be misinterpreted as “pins swapped”.

## 2025-12-26

- Added stable reference docs for Milestone A (contracts + tables):
  - CLI contract for `0016` REPL commands
  - Wiring/pin mapping tables (AtomS3R/Cardputer G1/G2 → GPIO1/GPIO2)
  - A copy/paste test matrix for cable mapping + contention diagnosis



## 2025-12-24

Scope clarified: firmware is AtomS3R-only; Cardputer is treated as optional peer device/signal source. Updated spec/tasks/analysis accordingly.

Rationale:

- The goal is a deterministic *instrument* on the AtomS3R side; building/maintaining peer firmware is unnecessary scope for diagnosing reachability/mapping/contestion on GPIO1/GPIO2.
- Treating the peer as external keeps the workflow flexible (Cardputer, USB↔UART dongle, function generator, etc.).


## 2025-12-24

Added a playbook for installing Saleae Logic 2 on Linux via AppImage (launcher + udev + Electron/AppArmor sandbox troubleshooting).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/playbook/01-install-saleae-logic-2-on-linux-appimage-udev-launcher.md — New user-facing install playbook


## 2025-12-24

Implemented initial AtomS3R-only signal tester firmware as new ESP-IDF project 0016; includes USB Serial/JTAG esp_console control plane, GPIO TX/RX modules, and LCD status UI. idf.py build succeeded.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/hello_world_main.cpp — New firmware entrypoint

