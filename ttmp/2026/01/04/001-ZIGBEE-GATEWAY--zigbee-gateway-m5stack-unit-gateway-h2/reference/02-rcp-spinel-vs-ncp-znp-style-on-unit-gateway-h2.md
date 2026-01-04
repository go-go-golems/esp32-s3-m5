---
Title: RCP (Spinel) vs NCP (ZNP-style) on Unit Gateway H2
Ticket: 001-ZIGBEE-GATEWAY
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
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md
      Note: RCP-based gateway bring-up playbook (CoreS3 host + H2 RCP)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_gateway.txt
      Note: Source snapshot of the M5Stack RCP-based gateway guide
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_ncp.txt
      Note: Source snapshot of the M5Stack NCP-based guide
    - Path: /home/manuel/esp/esp-5.4.1/examples/openthread/ot_rcp/README.md
      Note: ESP-IDF RCP firmware description and build entrypoint
    - Path: /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_gateway/README.md
      Note: RCP-based Zigbee gateway example (host runs Zigbee + Wi-Fi)
    - Path: /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/README.md
      Note: NCP firmware example (H2 runs Zigbee stack, exposes host protocol)
    - Path: /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/README.md
      Note: Host firmware example (S3 runs app logic, talks to NCP)
    - Path: /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/priv/esp_host_frame.h
      Note: Host↔NCP frame header definition (EGSP-like framing)
ExternalSources:
    - https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway:M5Stack guide for RCP-based gateway bring-up (ot_rcp + esp_zigbee_gateway)
    - https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_ncp:M5Stack guide for NCP-based bring-up (esp_zigbee_ncp + esp_zigbee_host)
    - https://github.com/espressif/esp-zigbee-sdk/tree/master/examples/esp_zigbee_gateway:Upstream RCP-based gateway example docs
    - https://github.com/espressif/esp-zigbee-sdk/tree/master/examples/esp_zigbee_host:Upstream host example docs (talks to NCP)
    - https://github.com/espressif/esp-zigbee-sdk/tree/master/examples/esp_zigbee_ncp:Upstream NCP firmware example docs
Summary: Explain the two architectures for a 2-chip Zigbee setup (RCP+Spinel vs NCP host protocol), relate them to the common ZNP pattern, and compare the two M5Stack Unit Gateway H2 guides.
LastUpdated: 2026-01-04T21:22:00Z
WhatFor: "Avoid confusion when bringing up Unit Gateway H2 + CoreS3 by clearly describing what runs where and what protocol is used between the chips."
WhenToUse: "When deciding between the M5Stack Zigbee Gateway guide vs the Zigbee NCP guide, or when debugging host↔H2 serial/radio issues."
---

# RCP (Spinel) vs NCP (ZNP-style) on Unit Gateway H2

## Goal

Explain:

- What an **RCP** is and what **Spinel** does in the RCP architecture.
- What an **NCP** is and how it compares to “classic” **ZNP** (Zigbee Network Processor) style designs.
- How those map onto the two M5Stack Unit Gateway H2 tutorials:
  - `zigbee_gateway` (RCP-based)
  - `zigbee_ncp` (NCP-based)

## Context

The Unit Gateway H2 + CoreS3 setup is a **two-SoC** Zigbee design:

- **CoreS3 (ESP32-S3)**: Wi-Fi capable “host/application” SoC.
- **Unit Gateway H2 (ESP32-H2)**: 802.15.4-capable SoC used for Zigbee radio/stack duties.

There are two common ways to split responsibilities between the host and the 802.15.4 SoC:

1. **RCP (Radio Co-Processor)**: the H2 acts as a “radio” controlled by the host; the **Zigbee stack runs on the host**.
2. **NCP (Network Co-Processor)**: the H2 runs the Zigbee stack; the host runs application logic and uses a **host protocol** to request Zigbee operations.

These correspond to two different “serial protocols” across the UART:

- **Spinel**: used in the RCP architecture (and also sometimes for NCP in Thread ecosystems).
- **ZNP-like command framing**: “host sends commands; NCP returns events”, typical of many Zigbee stacks (TI ZNP, Silicon Labs EZSP, etc). Espressif’s NCP uses its own frame format (see `esp_host_frame.h`).

The key practical implication:

- “Stock firmware” on the H2 is only useful if it implements the protocol the host firmware expects. The host code is not talking “generic Zigbee” over UART; it’s talking a very specific wire protocol (Spinel or Espressif NCP frames).

## Quick Reference

### Definitions (the minimum set)

- **RCP**: Radio Co-Processor. Exposes 802.15.4 radio operations to a host.
- **NCP**: Network Co-Processor. Runs the Zigbee stack and exposes a command/event protocol to a host.
- **Spinel**: a binary, property-based protocol used to control an RCP (and sometimes NCP) over UART/SPI.
- **ZNP**: Zigbee Network Processor. A classic Zigbee architecture where the Zigbee stack runs on a coprocessor and the host controls it via a serial API.

### One-screen comparison

| Axis | RCP + Spinel | NCP + host protocol (ZNP-style) |
|---|---|---|
| Zigbee stack location | Host (CoreS3 / ESP32-S3) | NCP (Unit Gateway H2 / ESP32-H2) |
| H2 responsibilities | 802.15.4 radio driver/operations | Full Zigbee stack + ZCL processing + network state |
| UART protocol | Spinel (property/command interface) | Command/event frames (Espressif EGSP-like framing) |
| Host responsibilities | Zigbee stack + Wi-Fi + application | Application + a “Zigbee proxy” client |
| Typical motivation | Tight integration with host networking; reuse existing “host-side” Zigbee gateway code | Isolate Zigbee stack; host can be simpler; decouple radio/stack |
| Debugging | Mostly host Zigbee stack + radio link quality | Two stacks + explicit host↔NCP protocol debugging |

## RCP + Spinel: how it works

### What runs where (Unit Gateway H2 + CoreS3)

**CoreS3 (host)**:

- Runs the Zigbee stack (in Espressif examples, Zigbee is backed by ZBOSS/`esp-zigbee-lib`).
- Uses a “radio backend” that speaks to the H2 over UART.

**Unit Gateway H2 (RCP)**:

- Runs `ot_rcp` (ESP-IDF OpenThread RCP example) which exposes an 802.15.4 radio interface.
- Does *not* run a Zigbee network stack; it is a radio/coordinator backend for the host.

In this architecture, **Zigbee “network brain” is on the host**, and the H2 is “radio muscle”.

### What Spinel is (at a practical level)

Spinel is a binary protocol that provides:

- **Commands** (host → RCP) to get/set properties and request operations.
- **Asynchronous notifications** (RCP → host) for events (RX frames, state changes, etc).

Spinel is typically:

- **Property-based** (think: “set `PROP_PHY_CHAN` to 13”, “get `PROP_HWADDR`”, “insert a TX frame into `PROP_STREAM_RAW`”).
- **Framed** over UART with an HDLC-like wrapper (frame delimiter, escaping, CRC) so raw binary can traverse a serial stream robustly.

In OpenThread ecosystems, this is implemented by host-side “radio spinel” drivers; Espressif’s Zigbee gateway example uses similar concepts (you’ll see references to spinel/radio-spinel code paths in `esp-zigbee-lib` and `esp_radio_spinel` components).

### Why RCP+Spinel is attractive for a gateway

- The host already needs Wi-Fi + TCP/IP + “gateway” logic; keeping the Zigbee stack on the host simplifies “bridge” functionality.
- The H2 firmware can be smaller/simpler than a full Zigbee stack image.
- With the right reset/boot wiring, the host can also manage RCP updates (though M5’s CoreS3 pin setup often disables reset/boot control).

### Failure modes (common)

- UART wiring/pin mismatch (TX/RX swapped or wrong GPIOs configured).
- UART baud too high for cable/board drive strength (M5 suggests reducing `ot_rcp` baud if instability appears).
- Host expects Spinel but H2 is running non-Spinel firmware (factory/stock image or NCP image).

## NCP + host protocol: how it works (and why it resembles ZNP)

### What runs where

**Unit Gateway H2 (NCP)**:

- Runs `esp_zigbee_ncp` (from `esp-zigbee-sdk/examples/esp_zigbee_ncp`).
- This is a **Zigbee Network Co-Processor**: it runs Zigbee stack and responds to commands from the host over UART.

**CoreS3 (host)**:

- Runs `esp_zigbee_host` (from `esp-zigbee-sdk/examples/esp_zigbee_host`).
- This code sends “do X” requests to the NCP and receives events/confirmations back.

In this architecture, the **Zigbee “network brain” lives on the H2**, and the S3 is mostly a client of that brain.

### The wire protocol (Espressif EGSP-like framing)

Espressif’s host↔NCP protocol is not Spinel; it uses a frame header like:

- protocol version + frame type (request/response/notify)
- frame ID (which API call/event this represents)
- sequence number (transaction correlation)
- payload length

See `esp_host_header_t` in `esp_host_frame.h` for the exact layout.

This is conceptually very similar to the classic ZNP style:

- host sends a “command frame”
- NCP replies with “response frame”
- NCP emits “asynchronous event frames” when something happens (device joined, attribute report, etc)

### Why this resembles “standard ZNP”

“ZNP” is used colloquially to describe the pattern where:

- a dedicated processor runs the Zigbee stack
- the host interacts via a serial command API

Even when the exact byte-level protocol differs (TI ZNP vs Silicon Labs EZSP vs Espressif EGSP-like), the architecture is the same:

- **NCP** owns Zigbee network state, security keys, frame counters, routing, etc.
- **Host** is effectively an RPC client that asks the NCP to form/join networks, send ZCL, read attributes, etc.

### Failure modes (common)

- UART pin mismatch (host and NCP need to agree on TX/RX pins, and wiring must match).
- Host and NCP firmware are out of sync (protocol versions/frame IDs mismatch).
- Debugging requires reading both host logs and NCP logs (two binaries, two serial ports).

## Comparing the two M5Stack tutorials (Unit Gateway H2)

### 1) M5Stack “ESP Zigbee Gateway” (`zigbee_gateway`) is RCP-based

The M5 guide at:

- https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway

uses:

- **H2**: ESP-IDF `examples/openthread/ot_rcp` (RCP firmware)
- **CoreS3**: `esp-zigbee-sdk/examples/esp_zigbee_gateway` (gateway firmware)

Pins per the guide (CoreS3 side):

- RCP TX = 18, RCP RX = 17
- RCP reset/boot = `-1` (no control pins)

Notes:

- Since reset/boot are `-1`, auto-update flows that require toggling boot/reset are typically disabled/avoided.
- The guide includes the AW9523 I2C power-enable snippet for CoreS3.

### 2) M5Stack “ESP Zigbee NCP” (`zigbee_ncp`) is NCP-based

The M5 guide at:

- https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_ncp

uses:

- **H2**: `esp-zigbee-sdk/examples/esp_zigbee_ncp` (NCP firmware)
- **CoreS3**: `esp-zigbee-sdk/examples/esp_zigbee_host` (host firmware)

Pins per the guide:

- On **NCP (H2)**: UART RX = 23, UART TX = 24 (configured under `Component config -> Zigbee Network Co-processor`)
- On **Host (CoreS3)**: UART RX = 18, UART TX = 17 (configured under `Component config -> Zigbee NCP Host`)

Notes:

- The AW9523 power-enable snippet is also required on CoreS3 per the guide.
- In this model, the H2 is a full Zigbee stack node (coordinator/router/ED configurable), and the host is the client that drives it.

### What’s “the same” vs “different”

Same:

- Two chips.
- UART link between them.
- CoreS3 needs the AW9523 power-enable snippet for Grove power output.

Different:

- **Which firmware runs on the H2**
  - Gateway guide: `ot_rcp` (RCP firmware, Spinel)
  - NCP guide: `esp_zigbee_ncp` (Zigbee stack + NCP protocol)
- **Which firmware runs on the CoreS3**
  - Gateway guide: `esp_zigbee_gateway` (host runs Zigbee stack + Wi-Fi)
  - NCP guide: `esp_zigbee_host` (host app talks to NCP)
- **Which serial protocol is spoken**
  - Gateway guide: Spinel
  - NCP guide: Espressif host/NCP frames

## Usage Examples

### RCP-based path (what we’re doing in this ticket)

Build + flash H2 RCP:

```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-5.4.1/examples/openthread/ot_rcp
idf.py set-target esp32h2
idf.py -p /dev/ttyACM0 flash
```

Build CoreS3 gateway:

```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_gateway
idf.py set-target esp32s3
idf.py build
```

### NCP-based path (if switching to the M5Stack NCP guide)

Build H2 NCP:

```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp
idf.py set-target esp32h2
idf.py menuconfig   # set Zigbee Network Co-processor UART pins
idf.py build flash
```

Build CoreS3 host:

```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_host
idf.py set-target esp32s3
idf.py menuconfig   # set Zigbee NCP Host UART pins
idf.py build flash
```

## Related

- Bring-up playbook (RCP-based): `../playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md`
- Diary: `./01-diary.md`

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
