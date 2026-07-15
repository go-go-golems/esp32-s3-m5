---
Title: Implementation Diary - Reader Primitives Firmware
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - ereader
    - esp-idf
    - esp32s3
    - m5gfx
    - microquickjs
    - architecture
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0112-papers3-reader-primitives/README.md
      Note: 'Decision records: IDF 5.3.4 pin, no display backend in Phase 1'
    - Path: repo://0112-papers3-reader-primitives/main/app_console.cpp
      Note: Console proxy plus stress/flood fixtures (commit 1aea3b4)
    - Path: repo://0112-papers3-reader-primitives/main/app_events.h
      Note: Phase 1 POD event/reply contracts and StatusCode vocabulary (commit f7c5a21)
    - Path: repo://0112-papers3-reader-primitives/main/app_owner.cpp
      Note: Single owner task, ordering validation, bounded replies (commit f7c5a21)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/52-papers3-console-client.py
      Note: Modem-control-safe interactive console client used for gate validation
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/0112-phase1-validation-02.log
      Note: Phase 1 exit-gate transcript (flood/stress/shutdown)
ExternalSources: []
Summary: Chronological implementation diary for the native reader-primitives firmware (0112), starting at Phase 1 of the phased plan.
LastUpdated: 2026-07-15T14:20:00-04:00
WhatFor: Record what was built per phase, what failed with exact errors, validation evidence, and review instructions.
WhenToUse: Read before resuming reader implementation work; the display-investigation history lives in 01-investigation-diary.md.
---


# Implementation Diary - Reader Primitives Firmware

## Goal

Track the actual e-reader implementation (project `0112-papers3-reader-primitives`) phase by phase, as prescribed by the master design (`design-doc/01-*.md`) and the implementation handoff (`reference/02-*.md`). The hardware/display investigation has its own diary (`reference/01-investigation-diary.md`) and remains paused; nothing in this diary makes optical claims about the panel.

## Step 1: Resume the ticket and scope Phase 1

Resumed the ticket after the display-qualification pause with the goal of building the actual reader from native primitives. Read the implementation handoff and the Phase 1 sections of the master design, surveyed prior art (`0080-papers3-ereader` config, `0110`/`0111` experiment scaffolds, serial-safety scripts), and confirmed the next free project number is `0112` (`0110`/`0111` were taken by the density experiments after the design doc provisionally said `0106`).

Two baseline decisions were made explicit rather than inherited: the firmware pins ESP-IDF 5.3.4 (the repo standard for PaperS3 application firmware and standing user guidance; the 5.4.2 lineage belongs to the paused EPD experiments), and Phase 1 has **no display backend at all** — the handoff's Phase 1 responsibility list supersedes the design doc's "initialize the display" bullet because the panel is unqualified and the shared `M5PaperS3-UserDemo/components` M5GFX checkout currently carries local debug patches from the investigation.

### Prompt Context

**User prompt (verbatim):** "Read ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/02-implementation-handoff-native-first-s3paper-e-reader.md

and then resume the docmgr ticket to build the actual ereader. The tablet is connected over USB. 

Keep a detailed diary as you work, implement things step by step (you might need to refresh the tasks list or so), commit at appropriate intervals (read skill for diary format)."

**Assistant interpretation:** Resume ticket ESP-50-PAPERS3-EREADER-PRIMITIVES at the implementation phase: create the new native reader-primitives firmware per the handoff's phased order, validate on the connected PaperS3, and keep docmgr bookkeeping (diary, tasks, changelog, commits) current.

**Inferred user intent:** Move from investigation to a working e-reader, built in the disciplined phase order the ticket prescribes, with a reviewable paper trail.

### What I did
- Read the handoff, Phase 1/2 sections of the master design, the tail of the investigation diary, `AGENTS.md`, and the configs of `0080`/`0110`/`0111`.
- Confirmed serial safety conventions from `scripts/45-capture-papers3-readonly-reconnect.py` and the staging scripts (no pyserial, no modem-control ioctls, single port owner).
- Created this diary document.

### Why
- The handoff explicitly orders: read design → create Phase 1 scaffold → prove ownership/eventing before any display or JS work.

### What worked
- The ticket workspace, task list (P1.1–P1.10 pre-registered), and prior project configs gave everything needed to start without re-deriving toolchain or console conventions.

### What didn't work
- N/A (orientation only).

### What I learned
- The PaperS3 is attached as `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00` and the port was free.
- `0080` (prior reader demo) builds against the shared `M5PaperS3-UserDemo/components` checkout whose M5GFX HEAD is a local debug commit — a reason not to depend on it for the new baseline.

### What should be done in the future
- Phase 2 must record its own M5GFX/M5Unified pin decision (task `ambe` deliberately left open).

### Code review instructions
- Start with `reference/02-implementation-handoff-native-first-s3paper-e-reader.md` and the Phase 1 gate in `design-doc/01-*.md` (§ "Phase 1: Scaffold and ownership model").

## Step 2: Scaffold the Phase 1 firmware (0112-papers3-reader-primitives)

Created `0112-papers3-reader-primitives`: ESP-IDF 5.3.4 project with octal PSRAM, 16 MB flash, custom partition table, USB Serial/JTAG console, a single owner task, bounded POD event/reply queues, a console that can only post messages, diagnostics, and stress/flood fixtures. The build passed on the second attempt (missing `<initializer_list>` include) and produced a 276 KB app.

The core contract follows the handoff: `AppEvent` is POD with a payload union (console op / pointer / timer / storage), a per-source `producer_seq` the owner validates strictly, a `request_id`, and an optional bounded reply queue handle. All application state lives in one `AppState` struct touched only by the `ui_owner` task; `AssertOwner()` aborts on any cross-task access. Producer-side enqueue is non-blocking: a full queue returns `CapacityExceeded` and increments an atomic per-source rejected-sends counter (instrumentation, not model state).

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Implement task group P1.1–P1.10.

**Inferred user intent:** A firmware substrate whose ownership and failure behavior is provable before features are added.

**Commit (code):** f7c5a216d801b62fef87cb78859c2b52023cb25b — "Reader: scaffold Phase 1 owner-task firmware (0112)"

### What I did
- `main/app_events.h`: `StatusCode` vocabulary (Ok/InvalidArgument/CapacityExceeded/Busy/Timeout/CorruptData/OutOfMemory/Unimplemented), `AppEventKind`, `EventSource`, POD payloads, reply snapshots.
- `main/app_owner.cpp`: `ui_owner` task (core 1, prio 5), queue capacity 32, per-source ordering validation, per-kind counters, reply routing with counted reply-drops, `AwaitReply` with stale-reply tolerance, shutdown state machine.
- `main/app_console.cpp`: esp_console REPL on USB Serial/JTAG with `status`, `heap`, `display`, `events`, `ping`, `stress [n]`, `flood [n]`, `shutdown`; every command posts an event and waits ≤500 ms for the reply.
- `sdkconfig.defaults`, `partitions.csv` (4 MB factory + 512 K spiffs), `.envrc` (IDF 5.3.4), `README.md` with decision records; built with `idf.py set-target esp32s3 && idf.py build`.

### Why
- Phase 1's exit gate is deterministic ownership, not UI: every design choice (POD-only payloads, non-blocking sends, bounded replies) exists to make failure modes explicit and testable.

### What worked
- Clean build under 5.3.4 after one include fix; app 0x43910 bytes, 93% factory partition free.

### What didn't work
- First build failed with:
  ```text
  error: deducing from brace-enclosed initializer list requires '#include <initializer_list>'
  ```
  in both `app_owner.cpp` and `app_console.cpp` (range-for over brace lists); fixed by adding the include.

### What I learned
- IDF 5.3.4's `esp_console_cmd_t` has `func_w_context`/`context` members, so designated initializers must cover them (zero-init via explicit fields worked).

### What was tricky to build
- Reply lifetime: replies must never block the owner. `SendReply` uses a 0-tick send and counts drops (`replies_dropped`), and `AwaitReply` discards stale replies from earlier timed-out requests by matching `request_id` — otherwise a timed-out `ping` could poison the next command's reply slot.
- Ordering accounting had to distinguish "rejected send attempts" (producer-side, atomic) from "out-of-order arrivals" (owner-side): the flood fixture posts with `producer_seq=0` specifically so its rejected events cannot masquerade as sequence gaps.

### What warrants a second pair of eyes
- The `AppEvent` union carries a `QueueHandle_t` across tasks; the contract that the reply queue outlives the wait is enforced only by convention (console owns a static queue).
- `AssertOwner()` aborts in release builds too — deliberate, but review whether abort-on-violation is the desired production posture later.

### What should be done in the future
- Phase 2: pure `s3paper_core` geometry/DrawOps with host tests, fake backend, then the M5 transaction shell with a pinned component decision.

### Code review instructions
- Read `0112-papers3-reader-primitives/main/app_events.h` first (contracts), then `app_owner.cpp` (`HandleEvent`, `PostEvent`, `AwaitReply`), then `app_console.cpp`.
- Rebuild: `unset IDF_PYTHON_ENV_PATH && source ~/esp/esp-idf-5.3.4/export.sh && cd 0112-papers3-reader-primitives && idf.py build`.

## Step 3: Flash, validate the exit gate on hardware, and make flood overflow deterministic

Flashed the firmware to the connected PaperS3 and validated over a new modem-control-safe console client. The first pass proved all command round-trips but exposed a real weakness: `flood 100` produced `rejected=0` because the owner task (core 1) drains a 32-deep queue faster than the console task (core 0) can fill it, so queue-full was never actually exercised. I rewrote flood to post its burst from a task pinned to the owner's core at higher priority (6 > 5), which makes overflow deterministic: the owner cannot run mid-burst, so exactly `capacity` events are accepted.

After reflashing, the full exit-gate run passed: `flood 100` → `accepted=32 rejected=68 (CapacityExceeded)`; `stress 100000` → 200,000 events delivered with `out_of_order=0`, `last_seq` exact per source, 5 interleaved console pings, one explicit retried backpressure rejection, in 1.087 s (~184k events/s through the owner); `shutdown` → `ping` reports `busy`, `status` still answers with `state=shutting-down`; and the owner's counters reconcile exactly (`owner_seq` = sum of per-kind counts). Heap was byte-stable across the 200k-event run.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Prove the Phase 1 exit gate on the real device, not just in theory.

**Inferred user intent:** Evidence-backed phase completion consistent with the ticket's discipline.

**Commit (code):** 1aea3b4396eb11c9938b055e28d20801c332a8cf — "Reader: make flood overflow deterministic; capture Phase 1 gate evidence"

### What I did
- Added `scripts/52-papers3-console-client.py`: opens the by-id tty with `os.open` + `flock`, sets raw termios (tcsetattr touches no modem-control lines), never issues DTR/RTS ioctls; sends scripted commands and records transcripts.
- Flashed via `esptool write_flash @flash_args` (`--before default_reset --after hard_reset`) after verifying the port had zero owners with `lsof`.
- Captured three transcripts under `scripts/output/`: `0112-phase1-validation-01.log` (first pass, flood weakness), `-02.log` (full gate: flood/stress/shutdown), `-03-longstress.log` (100k/producer with concurrent pings).
- Reset out of the shutdown state with a plain esptool `read_mac` (its hard reset) rather than any monitor attach.

### Why
- The Phase 1 gate demands *proof* that simultaneous console and input traffic cannot mutate state across tasks and that queue-full/reply-timeout behavior is explicit; a flood that never overflows proves nothing.

### What worked
- Every console command round-trips through the owner (ping ~45 µs quiet, still served mid-stress).
- Deterministic overflow: accepted exactly 32 (queue capacity) with 68 explicit rejections, visible afterwards in `events` as `rejected_sends=68`, `high_water=32`.
- Ordering held at 200k events across two concurrent producers plus live console traffic.

### What didn't work
- Initial `flood 100`: `accepted=100 rejected=0` — cross-core drain outpaced the producer (transcript `-01.log`). Fixed by pinning the burst to the owner's core at priority 6.
- In the 500-event stress, producers finished in 5 ms, before the first concurrent ping could interleave (`concurrent_pings ok=0`); the 100k run gives the gate real interleaving (`ok=5 failed=0`).

### What I learned
- The owner processes ~184k events/s at `-Og` with trivial handlers — the queue capacity of 32 is generous for real input rates, and backpressure (1 rejection in 200k sends under retry) behaves as designed.
- USB Serial/JTAG + raw termios (no modem control) is a safe interactive path; the device never reset or dropped into download mode across the session.

### What was tricky to build
- Proving queue-full on a dual-core system: any producer on the other core loses the race against a fast consumer. The symptom was `rejected=0` at every burst size tried; the underlying cause is that overflow requires the consumer to be *unable to run*, which only priority inversion on the same core guarantees. The fix posts the flood from core 1 at priority 6 and has the console task poll a done-flag with a 10 s explicit timeout.
- The console client must not use pyserial: its open() asserts DTR/RTS, which this board's earlier investigation showed can reset the USB-Serial/JTAG controller into ROM download mode. Raw `os.open` + `tcsetattr` (no `TIOCM*` ioctls) sidesteps that class of failure entirely.

### What warrants a second pair of eyes
- Whether flood-at-higher-priority starving the owner for the burst duration (≤10 s bound) is acceptable as a permanent diagnostic, or should be capped harder.
- The stress PASS criteria (`received == sent`, `out_of_order == 0`, `last_seq == n`) — confirm these are the right formalization of the design doc's "deterministic owner ordering".

### What should be done in the future
- Start Phase 2 (`t1yc`, P2.1–P2.11): host-testable `s3paper_core` geometry/status types, DrawOps + frame arena, fake backend, then the M5 transaction shell with its own component-pin decision record.
- P1.3 (`ambe`, M5GFX/M5Unified pin) remains open by design: Phase 1 has no M5 dependency; the pin belongs to the Phase 2 display-backend decision.

### Code review instructions
- Read the three transcripts under `ttmp/.../scripts/output/0112-phase1-validation-*.log`; `-02.log` is the gate run.
- Re-run on hardware: flash per the project README, then
  `python3 ttmp/.../scripts/52-papers3-console-client.py --cmd status --cmd "flood 100" --cmd "stress 100000" --cmd events`.
- Key code: `CmdFlood`/`FloodTask` and `CmdStress`/`StressProducerTask` in `0112-papers3-reader-primitives/main/app_console.cpp`; `HandleEvent` ordering checks in `main/app_owner.cpp`.

### Technical details

```text
Exit-gate evidence (transcripts -02/-03):
flood 100      -> accepted=32 rejected=68 (CapacityExceeded), high_water=32
stress 100000  -> received=100000+100000 out_of_order=0
                  last_seq console=100000 input=100000
                  rejected_attempts=1+0 concurrent_pings ok=5 failed=0
                  elapsed_ms=1087
shutdown       -> ping: "busy: owner is shutting down"; status: state=shutting-down
counters       -> owner_seq=1040 = ConsoleCommand 507 + Pointer 500 + TimerDue 32 + Shutdown 1
heap           -> internal_free 369827 before/after 200k events (byte-stable)
toolchain      -> ESP-IDF v5.3.4, app sha256 in build/; flashed at 460800 baud
```
