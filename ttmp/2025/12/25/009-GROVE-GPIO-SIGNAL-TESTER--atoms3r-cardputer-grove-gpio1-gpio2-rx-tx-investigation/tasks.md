---
Title: "Tasks: GROVE GPIO signal tester (GPIO1/GPIO2)"
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - gpio
    - uart
    - debugging
DocType: tasks
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Implementation checklist for the GROVE GPIO signal tester tool."
LastUpdated: 2025-12-25T00:00:00.000000000+00:00
WhatFor: "Concrete tasks to build and validate the signal tester firmware."
WhenToUse: "As the authoritative list of work items for this ticket."
---

## Milestone A — Define the tool contract (no code)

- [x] Write a stable CLI command contract (command names, args, examples)
- [x] Write a wiring/pin mapping reference table (AtomS3R and Cardputer GROVE pin labels → GPIO numbers)
- [x] Define the test matrix (cable orientation, pin swap cases, expected observations)
- [x] Capture the current scope symptom as an explicit acceptance criterion:
  - unloaded peer TX: ~0–3.3V
  - connected to AtomS3R: low lifts to ~1.5V (suspected contention/bias)
  - tester must provide Hi‑Z / pull / drive modes to diagnose this deterministically

## Milestone B — Build the AtomS3R firmware skeleton

Scope clarification: **we are building the firmware for AtomS3R only**.

The peer device for tests can be Cardputer (running any existing firmware), a USB↔UART adapter, or any other known-good signal source/sink. The key requirement is that the AtomS3R side is deterministic and provides the control plane + status UI.

Tasks:

- [x] Confirm AtomS3R-only scope in `changelog.md` (record rationale)
- [x] Create the AtomS3R ESP-IDF project with minimal dependencies
- [x] Bring up `esp_console` over USB Serial/JTAG (control plane)
- [x] Add a periodic LCD “status screen” (mode/pin/state/counters)

## Milestone C — GPIO transmit modes (TX)

- [x] Set pin to output and drive steady high/low
- [x] Generate square wave at selectable frequency (software toggle, timer-based, or RMT)
- [x] Generate a “pulse train” pattern useful for scope/logic-analyzer trigger
- [ ] Add “inverted” option (helpful when debugging swapped pins)

## Milestone D — GPIO receive modes (RX)

- [x] Set pin to input with selectable pull (none/pullup/pulldown)
- [x] Count edges via GPIO ISR (rising/falling/both)
- [ ] Record last-edge timestamp and estimate frequency (coarse)
- [ ] Optional: capture short bursts (ring buffer of timestamps) for deeper inspection
- [x] Add an explicit “Hi‑Z input” mode (input + pulls off) to validate whether the AtomS3R is biasing the peer TX line

## Milestone E — UART-specific investigations (optional)

These tasks are only needed if GPIO-level tests prove the physical path is correct but UART still fails.

- [ ] Add a “UART loopback” mode (configure UART peripheral on chosen pins and echo bytes)
- [ ] Add explicit newline/echo behaviors to match `esp_console` expectations

## Milestone F — Validation playbooks + evidence capture

- [x] Write “bring-up” playbook: build/flash/monitor steps for AtomS3R (and how to connect a peer device)
- [x] Write “cable mapping” playbook: how to determine if GROVE is pin-to-pin and whether G1/G2 are swapped
- [ ] Collect evidence: scope screenshots, measured frequencies, observed edge counts
- [ ] Summarize root cause hypotheses and which are ruled in/out

## Milestone G — Remove `esp_console` entirely (manual REPL over system USB Serial/JTAG console only)

Rationale: `0016-atoms3r-grove-gpio-signal-tester` currently uses `esp_console` (linenoise + command registry) for its USB Serial/JTAG control plane. If we suspect `esp_console` is introducing side effects (tasking, stdio rebinding, line ending conversions, global state), then the signal tester must not depend on it at all. The target end-state is a minimal, deterministic manual REPL over the **system USB Serial/JTAG console** (`stdin`/`stdout`) that can control GPIO1/GPIO2 modes for electrical/serial debugging.

Tasks:

- [x] Define the manual control-plane contract (no `esp_console` semantics):
  - keep command names compatible with the current REPL (`mode`, `pin`, `tx`, `rx`, `status`)
  - define prompt behavior (e.g. print `sig> ` before each read)
  - define line ending acceptance (`CR`, `LF`, `CRLF`)
- [x] Remove `esp_console` usage from `0016`:
  - delete/stop calling `esp_console_new_repl_usb_serial_jtag()` / `esp_console_start_repl()`
  - remove all `esp_console_cmd_register(...)` command registration
  - remove `console_repl.{h,cpp}` (or rename it to the manual REPL module)
- [x] Implement a minimal manual REPL task (line-based parsing) over system console stdio:
  - spawn a FreeRTOS task that does line-based reads on USB Serial/JTAG
  - parse tokens into `CtrlEvent` and send via `ctrl_send(...)`
  - avoid linenoise/history; keep parsing intentionally dumb and deterministic
- [x] Remove the `console` component dependency from `0016` build:
  - update `main/CMakeLists.txt` `PRIV_REQUIRES` so `console` is no longer required
  - ensure the firmware still builds and runs with USB Serial/JTAG console I/O
- [x] Update `0016` README and ticket playbooks:
  - document behavioral differences vs `esp_console` (no tab-completion/history, simpler prompt)
- [ ] Validation: run the 0016 tester in both control-plane modes and confirm identical GPIO behavior:
  - N/A (we will not maintain two modes)
  - validate that the manual REPL can still drive the full test surface:
    - TX waveforms (high/low/square/pulse)
    - RX edge counts (rising/falling/both + pulls)
    - status output (mode/pin/tx/rx counters)


