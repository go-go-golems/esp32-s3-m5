---
Title: '0016: Add UART TX/RX modes for GROVE UART functionality + speed testing'
Ticket: 0013-GROVE-SERIAL-DEBUG
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - gpio
    - uart
    - debugging
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/control_plane.h
      Note: Will gain CtrlType events for uart baud/map/tx/rx
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp
      Note: Will be extended to parse mode uart_tx/uart_rx and uart subcommands
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: Will apply UART mode by starting/stopping the new uart_tester module
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.h
      Note: Will gain new TesterMode values and UART config fields
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-26T17:12:22.181226269-05:00
WhatFor: 'Design the UART test feature for 0016: add uart_tx/uart_rx modes and REPL verbs to transmit repeat strings and echo RX, enabling GROVE UART functionality and speed testing.'
WhenToUse: Use before implementing UART support in 0016; reference while reviewing command surface changes and wiring/pin-ownership decisions.
---


# 0016: Add UART TX/RX modes for GROVE UART functionality + speed testing

## Executive Summary

Add two new tester modes to `0016-atoms3r-grove-gpio-signal-tester`:

- `uart_tx`: configure GROVE pins as a UART peripheral and repeatedly transmit a configured payload with a configured inter-send delay (ms).
- `uart_rx`: configure GROVE pins as a UART peripheral, read bytes, and **echo** received bytes (and optionally print them to the USB Serial/JTAG console), enabling basic loopback and “does RX work at this baud?” validation.

Implementation: introduce a small `uart_tester` module that owns the ESP-IDF UART driver lifecycle plus TX/RX tasks, wired into the existing single-writer state machine (`signal_state.cpp`) and manual REPL (`manual_repl.cpp`).

## Problem Statement

The current 0016 tester validates GROVE wiring at the **GPIO** layer (TX generator and RX edge counter). For serial debugging we also need to validate the **UART peripheral path**:

- prove UART TX pin mapping and that the UART output toggles at the requested baud,
- prove UART RX sampling works and bytes are received (and echoed),
- iterate baud rates to find where failures begin (“speed testing”).

Constraints:

- Control plane remains USB Serial/JTAG (do not depend on GROVE UART to control the tester).
- Keep the manual REPL small and deterministic (no `esp_console`).

## Proposed Solution

### Command surface (verbs)

The existing REPL has:

- `mode tx|rx|idle`
- `tx ...`
- `rx ...`
- `status`

We extend it with two new modes and a new top-level verb `uart`.

#### Modes

- `mode uart_tx`
- `mode uart_rx`

#### UART verbs

- `uart baud <baud>`
  - Sets the UART baud rate used in both UART modes.
  - If currently in `mode uart_tx|uart_rx`, re-apply immediately.

- `uart map normal|swapped`
  - `normal`: `RX=GPIO1`, `TX=GPIO2`
  - `swapped`: `RX=GPIO2`, `TX=GPIO1`

- `uart tx <token> <delay_ms>`
  - Configures TX payload and starts repeating TX in `mode uart_tx`.
  - `<token>` is a single whitespace-delimited token (current parser limitation).
  - `<delay_ms>` is the delay between writes; `0` means “back-to-back” (best-effort).

- `uart tx stop`
  - Stop the TX loop.
  - Confirmed behavior: stays in `mode uart_tx` (UART remains configured; TX becomes idle).

- `uart rx get [max_bytes]`
  - In `mode uart_rx`, read and consume bytes from the RX buffer and print them to the USB Serial/JTAG console.
  - Default `max_bytes` is small (e.g. 64) to avoid flooding the console; implementation may cap (e.g. 1024).

- `uart rx clear`
  - Clear the RX buffer and reset per-mode RX counters (bytes buffered / dropped), without changing mode.

#### Status output extensions

Extend `status` to include UART details when in UART modes:

- mode (`UART_TX`/`UART_RX`), baud, map, tx token, tx delay
- UART counters: `uart_tx_bytes_total`, `uart_rx_bytes_total`
- RX buffer stats: `uart_rx_buf_used`, `uart_rx_buf_dropped` (and/or `uart_rx_overrun`)

### Firmware structure

#### 1) Extend `TesterMode` and `tester_state_t`

In `main/signal_state.h`:

- extend `TesterMode` with `UartTx` and `UartRx`
- extend `tester_state_t` with UART config fields (baud/map/tx token/delay/echo + stats)

#### 2) Add `main/uart_tester.{h,cpp}`

Responsibilities:

- Own UART driver lifecycle (install/config/delete) on a dedicated UART port (default `UART_NUM_1`).
- Configure pins using `uart_set_pin()` with the selected mapping.
- TX repeat task:
  - loop: `uart_write_bytes()` payload, delay `delay_ms`
  - maintain byte counters
- RX buffer task:
  - loop: `uart_read_bytes()` into a ring buffer
  - update counters: total RX bytes, buffered bytes, dropped bytes
  - do **not** print asynchronously (keeps REPL deterministic); use `uart rx get` to drain/print

RX buffer requirements (v1):

- Fixed-size ring buffer (size TBD; e.g. 4096 bytes).
- On overflow: drop oldest or newest (decision TBD) and increment `uart_rx_buf_dropped`.
- Provide a function to drain up to N bytes for `uart rx get` (consuming).

#### 3) Apply semantics (pin ownership)

UART driver should own pins while active; we must avoid configuring GPIO ISR/pulls on those pins concurrently.

Proposed apply rules:

- Entering UART modes:
  - stop `gpio_tx` and `gpio_rx`
  - safe-reset both pins once (baseline)
  - start `uart_tester` with current UART config
- Leaving UART modes:
  - stop `uart_tester` (tasks + driver delete)
  - safe-reset both pins (baseline), then apply next mode

This preserves “instrument resets first” while respecting UART driver ownership.

## Design Decisions

### Decision: keep UART verbs under a `uart` namespace

Rationale: avoid confusing GPIO `tx`/`rx` with UART `uart_tx`/`uart_rx`.

### Decision: support only `normal|swapped` pin mapping in v1

Rationale: GROVE has exactly two signal wires; this covers the most common failure mode (TX/RX swapped) without adding a general pin router UI.

### Decision: payload is a single token in v1 (no quoting)

Rationale: current parser is `strtok`-based; we keep it small/deterministic. If we need spaces/newlines/hex, we can add a minimal quoting/escape parser later.

## Alternatives Considered

### Alternative: print RX bytes to USB console as they arrive

Rejected: it interleaves with the REPL prompt and makes the control plane less deterministic. Buffering + explicit `uart rx get` keeps interaction predictable and still allows verifying RX content.

### Alternative: use UART0 for GROVE

Rejected (tentative): UART0 is often entangled with boot/log defaults. Prefer a dedicated UART (UART1) for the tester to reduce surprises.

## Implementation Plan

## 1) CLI contract changes

- [ ] Extend `mode` to accept `uart_tx` and `uart_rx`
- [ ] Add `uart baud <baud>`
- [ ] Add `uart map normal|swapped`
- [ ] Add `uart tx <token> <delay_ms>` and `uart tx stop`
- [ ] Add `uart rx get [max_bytes]` and `uart rx clear`
- [ ] Extend `status` output with UART config + counters

## 2) Code changes (files)

- [ ] **Modify** `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.h`
- [ ] **Modify** `.../main/signal_state.cpp`
- [ ] **Modify** `.../main/control_plane.h` (new `CtrlType`s for UART)
- [ ] **Modify** `.../main/manual_repl.cpp` (parse `mode uart_*` and `uart ...`)
- [ ] **Add** `.../main/uart_tester.h`
- [ ] **Add** `.../main/uart_tester.cpp`
- [ ] **Modify** `.../main/lcd_ui.cpp` (show UART status + counters)
- [ ] **Modify** `.../main/CMakeLists.txt` (compile/link new module)

## 3) Docs/playbooks

- [ ] Update `0016` README with the new UART commands
- [ ] Add a short playbook “UART RX test”: peer sends bytes, tester buffers; operator drains with `uart rx get`

## Open Questions

1. RX buffer size: what default capacity do you want (e.g. 4096 or 16384 bytes)?
2. RX overflow policy: drop oldest (ring overwrite) vs drop newest (preserve earlier bytes)? (Either way track dropped count.)
3. Output format for `uart rx get`: hex-only vs mixed hex + ASCII rendering for printable bytes?
4. What baud range do you want to support (e.g. `1200..2000000`)?
5. Do you need payloads with spaces/newlines/hex bytes (requires small parser enhancement)?

## References

- Current REPL: `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp`
- Current state machine: `.../main/signal_state.{h,cpp}`
- Current control-plane event types: `.../main/control_plane.h`
- Wiring/mapping reference: `esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/reference/03-wiring-pin-mapping-atoms3r-cardputer-grove-g1-g2.md`
