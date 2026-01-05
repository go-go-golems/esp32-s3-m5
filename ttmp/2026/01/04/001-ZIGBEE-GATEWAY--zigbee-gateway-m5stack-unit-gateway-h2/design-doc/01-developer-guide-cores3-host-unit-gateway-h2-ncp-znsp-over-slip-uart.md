---
Title: 'Developer Guide: CoreS3 Host + Unit Gateway H2 NCP (ZNSP over SLIP/UART)'
Ticket: 001-ZIGBEE-GATEWAY
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32
    - esp32h2
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-5.4.1/export.sh
      Note: ESP-IDF v5.4.1 environment entrypoint
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c
      Note: NCP UART bus layer (pattern-det framing)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_frame.c
      Note: NCP-side frame parse/dispatch
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_main.c
      Note: NCP event queue + dispatcher task
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: NCP Zigbee command dispatch and notifications
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/slip.c
      Note: NCP SLIP implementation (stream buffer trigger fix)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_bus.c
      Note: Host UART bus layer (pattern-det framing)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_frame.c
      Note: ZNSP framing (CRC16) and SLIP encode/decode
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_main.c
      Note: Host event queue + dispatcher task
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_zb.c
      Note: Host-side request/response and notify dispatch
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/slip.c
      Note: SLIP implementation (stream buffer trigger fix)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_host/sdkconfig.defaults
      Note: Host UART pin config (GPIO17/18)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/sdkconfig.defaults
      Note: NCP UART pin config (GPIO23/24) + USB-Serial/JTAG console
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md
      Note: Bring-up playbook and commands
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md
      Note: Detailed implementation diary (step-by-step)
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md
      Note: Background on architectures and protocols (RCP/Spinel vs NCP/ZNSP)
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-185952-cores3-host-only-retest-script-120s.log
      Note: 'Evidence: Host-only retest run (post-enclosure)'
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-235331-cores3-host-only.log
      Note: 'Evidence: Host-only run with H2 not USB-attached'
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/playbook/01-uart-smoke-test-firmware-cores3-port-c-unit-gateway-h2.md
      Note: UART smoke-test procedure
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/reference/01-bugreport-cores3-h2-ncp-uart-link-appears-dead.md
      Note: Bugreport + resolution for initial UART link issues
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/run_ncp_host_and_h2_ncp_tmux.sh
      Note: Capture helper for paired host+NCP logs
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/uart_smoke
      Note: Smoke-test firmware projects (CoreS3 + H2)
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-cores3-host.log
      Note: 'Evidence: CoreS3 UART smoke-test log'
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-h2-ncp.log
      Note: 'Evidence: H2 UART smoke-test log'
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-cores3-host.log
      Note: 'Evidence: Host↔NCP ZNSP handshake + network formation'
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-h2-ncp.log
      Note: 'Evidence: NCP side ZNSP handshake + network formation'
ExternalSources: []
Summary: "Architecture + onboarding guide for the working CoreS3 (host) + Unit Gateway H2 (Zigbee NCP) setup using ZNSP frames over SLIP/UART on Grove Port C."
LastUpdated: 2026-01-04T19:37:55.245231221-05:00
WhatFor: "Help a new developer rebuild, understand, and debug the NCP-based CoreS3↔H2 system: hardware pin mapping, protocol framing, build/flash steps, and evidence of what was tested."
WhenToUse: "Use when onboarding to this work, reproducing the build, debugging host↔NCP UART/ZNSP issues, or validating behavior with the H2 enclosed (no USB serial)."
---


# Developer Guide: CoreS3 Host + Unit Gateway H2 NCP (ZNSP over SLIP/UART)

## Executive Summary

This work brings up a working “two-chip” Zigbee coordinator using:

- **Host**: M5Stack **CoreS3** (ESP32-S3) running the `esp_zigbee_host` firmware.
- **NCP**: M5Stack **Unit Gateway H2** (ESP32-H2) running the `esp_zigbee_ncp` firmware.
- **Link**: a Grove cable on **Port C**, carrying **UART** (plus power/ground).

The host and NCP communicate using **ZNSP** frames (Espressif’s “ZNP-style” host/NCP protocol), framed on the wire as **SLIP** over UART. The key reliability upgrades needed to make the M5Stack tutorial behave deterministically in this environment were:

- Use **UART pattern detection** (pattern `0xC0`, the SLIP END marker) so the bus layer forwards **complete SLIP frames** to the frame decoder.
- Call `uart_pattern_queue_reset()` after enabling pattern detection (otherwise `UART_PATTERN_DET` may never arrive).
- Fix a **FreeRTOS stream buffer trigger-level edge case** in `slip.c`.
- Avoid pin conflicts by moving the H2 console to **USB-Serial/JTAG** so UART GPIOs 23/24 remain quiet.

We validated, with captured logs, that:

- UART is physically working (bidirectional byte exchange) using dedicated smoke-test firmware.
- The ZNSP/SLIP link is working end-to-end: handshake, network formation, and opening permit-join.
- The system still works when the H2 is “closed up” (no USB serial device), i.e. the CoreS3 alone can drive the NCP over Port C.

## Problem Statement

M5Stack’s Unit Gateway H2 tutorials assume a clean environment and a stable serial link between an ESP32-S3 host and an ESP32-H2 NCP. In practice, we hit multiple failure modes that look identical from the outside (“NCP seems dead”):

- Physical layer issues (Grove cable not fully seated).
- UART RX framing issues (partial reads, buffer boundaries) causing SLIP decoding failures and/or stalls.
- UART driver pattern detection not firing due to missing pattern queue reset.
- UART pin conflicts (H2 boot console noise sharing pins with the NCP UART).

We needed a repeatable setup that a new developer can build/flash, understand, and debug quickly, with clear separation between:

- Hardware wiring/pins,
- Transport framing (UART + SLIP),
- Protocol framing (ZNSP header + CRC + request/response/notify),
- Zigbee stack behavior (forming a network, steering/permit-join).

## Proposed Solution

We use Espressif’s Zigbee SDK examples and treat the system as a set of layers:

1. **Application layer**
   - Host (CoreS3): `esp_zigbee_host` example app drives the Zigbee behavior (“form network”, “permit joining”, etc.) via the host API.
   - NCP (H2): `esp_zigbee_ncp` runs the Zigbee stack on ESP32-H2 and exposes host-callable functionality over ZNSP.

2. **Protocol layer (ZNSP / EGSP-style header)**
   - Shared header format on both sides: `version`, `type` (request/response/notify), `id` (command), `sn` (sequence), `len` (payload length).
   - Each protocol frame is protected by a CRC16 at the end (host side checks it before dispatch).

3. **Framing layer (SLIP)**
   - SLIP wraps the protocol frame so it can be sent over a byte stream.
   - `SLIP_END` (`0xC0`) delimits packets; `SLIP_ESC` (`0xDB`) escapes special bytes.

4. **Transport layer (UART on Grove Port C)**
   - UART pins are configured via Kconfig in each firmware (`CONFIG_HOST_BUS_UART_*` and `CONFIG_NCP_BUS_UART_*`).
   - The bus task uses **UART pattern detection** on `SLIP_END` so the stream delivered to SLIP decode is aligned to packet boundaries.

This design is intentionally boring: it favors deterministic framing and observability over clever buffering tricks.

### System diagram (physical + logical)

```text
               USB (for flashing + logs)
           ┌──────────────────────────────┐
           │                              │
           ▼                              ▼
  ┌──────────────────┐            ┌──────────────────┐
  │ M5Stack CoreS3    │            │ Unit Gateway H2   │
  │ ESP32-S3          │            │ ESP32-H2          │
  │                  │            │                  │
  │ app: esp_zigbee_* │            │ app: esp_zigbee_* │
  │ host (ZNSP client)│            │ ncp  (ZNSP server)│
  └───────┬──────────┘            └──────────┬───────┘
          │ Grove Port C (UART + power + GND)│
          │                                  │
          │ UART1 TX=GPIO17 → RX=GPIO23      │
          │ UART1 RX=GPIO18 ← TX=GPIO24      │
          └──────────────────────────────────┘

Wire protocol: ZNSP frames + CRC16 → SLIP framing → UART byte stream
```

### Protocol stack diagram (what runs where)

```text
CoreS3 (ESP32-S3)                                   H2 (ESP32-H2)
──────────────────                                  ───────────────
[Example App]                                       [Example App]
  esp_zb_* API surface (host shim)                    esp_zb_* real stack
          │                                                  │
          ▼                                                  ▼
  esp_host_zb.c                                        esp_ncp_zb.c
    - request/response queues                             - dispatch by ID
    - notify queue                                        - Zigbee callbacks → notify
          │                                                  │
          ▼                                                  ▼
  esp_host_frame.c                                     esp_ncp_frame.c
    - CRC16 add/check                                     - CRC16 add/check
    - SLIP encode/decode                                  - SLIP encode/decode
          │                                                  │
          ▼                                                  ▼
  esp_host_bus.c                                       esp_ncp_bus.c
    - UART pattern-det framing                            - UART pattern-det framing
    - stream buffers + event queue                        - stream buffers + event queue
          │                                                  │
          └─────────────── UART (Grove Port C) ─────────────┘
```

## Design Decisions

### Decision: Use NCP (ZNSP) rather than RCP (Spinel)

For this build we targeted the M5Stack “Zigbee NCP” flow (host + NCP), not the “Zigbee gateway with RCP” flow. NCP keeps the Zigbee stack on the H2 and exposes a higher-level host protocol (ZNSP). RCP/Spinel is covered separately in:

- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md:1`

### Decision: Treat UART framing as a first-class problem (pattern detection)

The default “read whatever bytes arrived” approach makes SLIP decoding sensitive to partial reads and buffer boundaries. We changed the bus to deliver **complete SLIP frames** by:

- enabling UART pattern detection for `SLIP_END` (`0xC0`), and
- forwarding bytes only when an END marker indicates end-of-frame.

This keeps `slip_decode()` simple and drastically reduces “looks like garbage input” failures.

### Decision: Reset UART pattern queue explicitly

We observed a stall where `UART_PATTERN_DET` never fired even though bytes were flowing. The fix is explicit and local:

- call `uart_pattern_queue_reset(uart_num, 20)` immediately after `uart_enable_pattern_det_baud_intr(...)`.

This is now in both bus implementations.

### Decision: Keep H2 UART pins quiet by using USB-Serial/JTAG console

The H2’s NCP UART uses GPIO23/24. Any boot console traffic on those pins will corrupt SLIP traffic. We therefore configured:

- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` (and disabled UART console),
- plus reduced bootloader log verbosity.

This makes the NCP UART pins dedicated to host/NCP traffic.

### Decision: Add a UART smoke-test firmware and treat it as a go-to diagnostic

We created minimal “PING/PONG over UART” firmwares for both boards so we can conclusively isolate:

- cable seating / port selection / pin mapping
from
- protocol framing issues.

The bugreport ticket includes the smoke test playbook and code:

- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/index.md:1`

## Alternatives Considered

### Use “stock” (factory) firmware on the H2

Rejected because we need a firmware that implements the ZNSP NCP role on the H2 (correct pins, SLIP framing, and the Zigbee stack integration). Factory firmware is not guaranteed to:

- expose the required host protocol,
- match the required pinout, or
- be observable/debuggable for bring-up.

### Keep a raw UART byte stream and try to recover framing in SLIP decode

Rejected because it is fragile: any read boundary or interleaving can cause:

- “SLIP characters must include” errors, or
- multi-frame concatenation that complicates parsing and recovery.

### RCP + Spinel

Viable, but it is a different architecture with a different host stack and constraints (see the dedicated reference doc). The “NCP” route matches the M5Stack `zigbee_ncp` tutorial and keeps the Zigbee stack fully on the H2.

## Implementation Plan

This section is written as a “reproduce from scratch” plan.

### 0) Prereqs

- ESP-IDF: `~/esp/esp-5.4.1` (ESP-IDF v5.4.1 export script).
- Zigbee SDK repo: `~/esp/esp-zigbee-sdk`
- Hardware:
  - M5Stack CoreS3
  - M5Stack Unit Gateway H2
  - Grove cable connected **CoreS3 Port C ↔ H2 Port C**

### 1) Build + flash NCP firmware (H2)

```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp
idf.py set-target esp32h2
idf.py build
idf.py -p /dev/ttyACM1 flash monitor
```

Required configuration (documented in `sdkconfig.defaults`):

- NCP UART pins: RX=23, TX=24
- Console: USB-Serial/JTAG (not UART)

### 2) Build + flash host firmware (CoreS3)

```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_host
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Required configuration (documented in `sdkconfig.defaults`):

- Host UART pins: RX=18, TX=17

### 3) Validate physical UART with smoke-test (if anything looks “dead”)

Use the smoke-test code in ticket `002-ZIGBEE-NCP-UART-LINK` to verify you can exchange bytes before debugging protocol layers:

- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/playbook/01-uart-smoke-test-firmware-cores3-port-c-unit-gateway-h2.md:1`

### 4) Run host-only after enclosing the H2

Once the H2 is enclosed (no USB serial), you can still validate everything from the CoreS3 logs. A reliable way to capture logs in this environment is:

```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_host
script -q -c "idf.py -p /dev/ttyACM0 monitor" host.log
```

## How It Works (deep dive)

### Hardware wiring and pin mapping

The UART wiring is configured in firmware via Kconfig and must match the physical “Port C” mapping.

- CoreS3 host:
  - `CONFIG_HOST_BUS_UART_TX_PIN=17`
  - `CONFIG_HOST_BUS_UART_RX_PIN=18`
- H2 NCP:
  - `CONFIG_NCP_BUS_UART_RX_PIN=23`
  - `CONFIG_NCP_BUS_UART_TX_PIN=24`

The effective UART direction is:

- CoreS3 TX (GPIO17) → H2 RX (GPIO23)
- CoreS3 RX (GPIO18) ← H2 TX (GPIO24)

### Threading and queues (why it’s reliable)

Both host and NCP follow the same concurrency pattern:

- A **bus task** is the only code that touches the UART driver.
- The bus task forwards complete frames through a **StreamBuffer** and signals work via an **event queue**.
- A **main task** (dispatcher) pops events and runs “input” or “output” processing.

This separates:

- interrupt-driven UART events
from
- relatively heavy parsing/CRC/dispatch work.

### Pseudocode: UART framing with pattern detection

Host bus task (`esp_host_bus_task()` in `esp_host_bus.c`) and NCP bus task (`esp_ncp_bus_task()` in `esp_ncp_bus.c`) both implement this pattern:

```text
init_uart()
enable_pattern_det(SLIP_END=0xC0)
uart_pattern_queue_reset()   // critical

loop:
  wait for uart_event
  if UART_PATTERN_DET:
    pos = uart_pattern_pop_pos()
    if pos > 0:
      frame = uart_read_bytes(pos + 1)   // includes END marker
      stream_buffer_send(frame)
      send_event(INPUT/OUTPUT with size)
```

Key consequence: higher layers only see complete SLIP frames.

### Pseudocode: ZNSP frame encode/decode

On the wire, the payload is:

```text
[ZNSP header][payload bytes...][CRC16]  -> SLIP encoded -> UART
```

Host receive path (simplified):

```text
on bus INPUT event(size):
  raw = stream_buffer_receive(size)
  decoded = slip_decode(raw)
  verify decoded has >= header size
  verify CRC16(header+payload) == trailer CRC16
  dispatch by header.type + header.id
```

Host transmit path:

```text
host_zb_output(id, payload):
  header = { type=request, id=id, sn=random, len=len(payload) }
  frame = header + payload + crc16
  slip = slip_encode(frame)
  bus_output(slip)
  wait for matching response on output_queue
```

### What “request/response/notify” means here

The `flags.type` field (in `esp_host_header_t` / `esp_ncp_header_t`) is used as follows:

- `REQUEST`: host asks the NCP to do something (form network, permit joining, …).
- `RESPONSE`: NCP responds to a request.
- `NOTIFY`: NCP emits asynchronous events back to the host (e.g., Zigbee stack signals).

On the host, `esp_host_zb_input()` routes:

- `NOTIFY` → `notify_queue`
- everything else → `output_queue` (used by request/response waits)

## API Reference (practical subset)

### Wire header structs

Host side:

- `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/priv/esp_host_frame.h`:
  - `esp_host_header_t`

NCP side:

- `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_frame.h`:
  - `esp_ncp_header_t`

These are structurally identical:

```c
typedef struct {
  struct { uint16_t version:4; uint16_t type:4; uint16_t reserved:8; } flags;
  uint16_t id;
  uint8_t  sn;
  uint16_t len;
} __attribute__((packed)) esp_*_header_t;
```

### Host event API

- `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_main.c`
  - `esp_host_send_event(esp_host_ctx_t *host_event)`
  - `esp_host_start()`, `esp_host_stop()`

### NCP event API

- `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_main.c`
  - `esp_ncp_send_event(esp_ncp_ctx_t *ncp_event)`
  - `esp_ncp_start()`, `esp_ncp_stop()`

### Bus layers (UART)

Host bus:

- `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_bus.c`
  - `esp_host_bus_task()`
  - `host_bus_init_hdl()` (UART config + pattern det)
  - `esp_host_bus_output()`

NCP bus:

- `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c`
  - `esp_ncp_bus_task()`
  - `ncp_bus_init_hdl()` (UART config + pattern det)
  - `esp_ncp_bus_input()`

### SLIP

- `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/slip.c`
- `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/slip.c`
  - `slip_encode(...)`
  - `slip_decode(...)`

## What We Tested (with evidence)

### Test 1: UART physical link (PING/PONG)

Goal: prove we can exchange bytes on the expected pins, independent of Zigbee.

Evidence logs:

- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-cores3-host.log`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-h2-ncp.log`

Outcome: stable bidirectional `PING`/`PONG` after reseating the Grove cable.

### Test 2: Full ZNSP handshake + network formation + steering

Goal: validate end-to-end host↔NCP protocol operation.

Evidence logs:

- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-cores3-host.log`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-h2-ncp.log`

Outcome: both sides progress to “formed network successfully” and open steering/permit-join.

### Test 3: Host-only validation (H2 enclosed, no USB serial)

Goal: ensure the system still works in the “final physical packaging” where H2 is not USB-attached.

Evidence logs:

- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-235331-cores3-host-only.log`
- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-185952-cores3-host-only-retest-script-120s.log`

Outcome: CoreS3 forms the network and opens permit-join for 180 seconds with the H2 enclosed.

## Operational Notes (debugging and gotchas)

### If the NCP looks “dead”

Work top-down:

1. Confirm power + seating (Grove cables can “look connected” but miss conductors).
2. Run the UART smoke test (it’s fast and decisive).
3. Confirm the correct UART pins are configured in `sdkconfig` on both devices.
4. If framing stalls, confirm `uart_pattern_queue_reset()` is present right after pattern-det enable.

### Capturing logs reliably in this environment

`idf.py monitor` requires a real TTY. Use one of:

- `script -q -c "idf.py -p /dev/ttyACM0 monitor" host.log`
- `tmux` capture helper (see scripts below)

## Reference Files (docs, scripts, code)

### Docs in this repo

- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md`
- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/reference/01-bugreport-cores3-h2-ncp-uart-link-appears-dead.md`

### Scripts

- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/scripts/run_ncp_host_and_h2_ncp_tmux.sh`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/run_ncp_host_and_h2_ncp_tmux.sh`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/scripts/uart_smoke/`

### Code (external workspace paths)

- `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_host/`
  - `components/src/esp_host_bus.c`
  - `components/src/esp_host_frame.c`
  - `components/src/esp_host_main.c`
  - `components/src/esp_host_zb.c`
  - `components/src/slip.c`
- `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/`
  - `sdkconfig.defaults` (UART pins + console configuration)
- `/home/manuel/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/`
  - `esp_ncp_bus.c`
  - `esp_ncp_frame.c`
  - `esp_ncp_main.c`
  - `esp_ncp_zb.c`
  - `slip.c`

## Open Questions

N/A for the bring-up milestone. Next “real product” questions depend on your intended end state:

- Do we want the CoreS3 to be a “real gateway” (Wi‑Fi/Ethernet + application services), or just a minimal host controller?
- Do we want to standardize on NCP (ZNSP) or migrate to RCP (Spinel) for interoperability with other host stacks?

## References

M5Stack documentation:

- Zigbee NCP tutorial: `https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_ncp`
- Zigbee gateway tutorial: `https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway`
