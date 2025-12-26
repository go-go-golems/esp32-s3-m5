---
Title: Wiring + pin mapping (AtomS3R/Cardputer GROVE G1/G2)
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - cardputer
    - gpio
    - uart
    - serial
    - usb-serial-jtag
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/README.md
      Note: 0016 firmware assumes G1=GPIO1 and G2=GPIO2 (pin selector contract)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild
      Note: Documents Cardputer GROVE label-to-GPIO mapping and warns about keyboard autodetect conflicts on GPIO1/GPIO2
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/21/003-ANALYZE-ATOMS3R-USERDEMO--analyze-m5atoms3-userdemo-project/reference/02-pinout-atoms3-atoms3r-project-relevant.md
      Note: Cross-check that AtomS3R external I2C / Port.A uses SCL=GPIO1 and SDA=GPIO2 (same physical GROVE pins)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/playbook/02-debug-grove-uart-using-0016-signal-tester.md
      Note: Uses these mappings to design the 2×2 cable mapping workflow
ExternalSources: []
Summary: "Reference tables for GROVE G1/G2 label mapping to GPIO numbers on AtomS3R and Cardputer, plus wiring pitfalls that commonly masquerade as UART/software failures."
LastUpdated: 2025-12-25T22:38:16.992009074-05:00
WhatFor: "Prevent 'wrong pin' and 'wrong convention' debugging loops by making label→GPIO mapping explicit and capturing known conflicts."
WhenToUse: "Before probing scopes/logic analyzers or configuring UART/console backends on GROVE; when interpreting G1/G2 or SDA/SCL labels across boards/cables."
---

# Wiring + pin mapping (AtomS3R/Cardputer GROVE G1/G2)

## Goal

Provide a single “truth table” for how **GROVE G1/G2 labels** map to **GPIO numbers** on AtomS3R and Cardputer, with the minimal wiring pitfalls that matter for this ticket.

## Context

- The GROVE 4‑pin connector provides **VCC**, **GND**, and two signals often labeled **G1/G2**.
- Those two signals are reused by multiple “port conventions”:
  - **External I2C / Port.A** convention: the same two wires are labeled **SCL/SDA**.
  - **UART** convention: the same two wires are treated as **RX/TX**, but *direction is a convention* (not baked into the connector).
- For this ticket, the signal tester (`0016`) treats the pins as **GPIO wires first**; we only care about *reachability, mapping, and contention* before UART semantics.

## Quick Reference

### Label → GPIO mapping (by board)

| Board | GROVE label | GPIO | Notes |
|---|---|---:|---|
| AtomS3R | G1 | 1 | Same physical wire as External I2C **SCL** |
| AtomS3R | G2 | 2 | Same physical wire as External I2C **SDA** |
| Cardputer | G1 | 1 | “Typically GPIO1” (documented in `0015` Kconfig help) |
| Cardputer | G2 | 2 | “Typically GPIO2” (documented in `0015` Kconfig help) |

### Direction convention (UART only; not a property of the connector)

If you treat the GROVE port as UART, a common convention is:

| Signal | GROVE label | GPIO |
|---|---|---:|
| RX | G1 | 1 |
| TX | G2 | 2 |

But: many failures are simply **TX/RX swapped**, so always validate with the test matrix before assuming.

### Critical gotchas (high-signal)

- **Cardputer keyboard conflict risk**: some Cardputer revisions may route keyboard IN0/IN1 to GPIO1/GPIO2; the `0015` tutorial has an autodetect path that may probe/claim GPIO1/GPIO2. If you use Cardputer as a peer device, make sure its firmware is not also manipulating GPIO1/2.
- **I2C label vs UART label mismatch**: one board/cable may call the wires `SCL/SDA` while another calls them `G1/G2`. They can still be the same pins; don’t assume a mismatch means a swap (use the matrix).
- **Ground first**: without a solid shared ground, “low level lifts” symptoms can look like contention.

## Usage Examples

### Example: quick “is the mapping swapped?” check using 0016

On AtomS3R running `0016`:

```text
sig> pin 1
sig> mode tx
sig> tx square 1000
```

Probe the peer device/cable and note which labeled pin sees a 1 kHz square wave.

Repeat:

```text
sig> pin 2
sig> tx square 1000
```

Fill in the 2×2 matrix in `04-test-matrix-cable-mapping-contention-diagnosis.md`.

## Related

- Ticket spec: `../analysis/01-spec-grove-gpio-signal-tester.md`
- CLI contract (0016): `02-cli-contract-0016-signal-tester.md`
- Test matrix: `04-test-matrix-cable-mapping-contention-diagnosis.md`
- Operational playbook: `../playbook/02-debug-grove-uart-using-0016-signal-tester.md`
