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
    - Path: repo://0112-papers3-reader-primitives/components/s3paper_core/include/s3paper/geometry.h
      Note: Pure defensive geometry contracts incl. provisional EPD alignment (commit a00161e)
    - Path: repo://0112-papers3-reader-primitives/components/s3paper_core/include/s3paper/widget.h
      Note: Phase 9 retained widget arena contract (commit c15fa32)
    - Path: repo://0112-papers3-reader-primitives/components/s3paper_core/src/refresh_planner.cpp
      Note: 'Refresh policy owner: damage merge, waveform mapping, clean-full triggers (commit 7aec26e)'
    - Path: repo://0112-papers3-reader-primitives/components/s3paper_core/tests/host/test_main.cpp
      Note: 237-check host suite for geometry/arena/builder/fake-backend (commit a00161e)
    - Path: repo://0112-papers3-reader-primitives/components/s3paper_m5/src/m5_backend.cpp
      Note: M5GFX transaction shell, only module calling M5.Display (commit a00161e)
    - Path: repo://0112-papers3-reader-primitives/main/app_console.cpp
      Note: Console proxy plus stress/flood fixtures (commit 1aea3b4)
    - Path: repo://0112-papers3-reader-primitives/main/app_events.h
      Note: Phase 1 POD event/reply contracts and StatusCode vocabulary (commit f7c5a21)
    - Path: repo://0112-papers3-reader-primitives/main/app_owner.cpp
      Note: Single owner task, ordering validation, bounded replies (commit f7c5a21)
    - Path: repo://0112-papers3-reader-primitives/main/app_reader.cpp
      Note: Phase 8 reading controller with gesture page turns (commit 3ec8c0c)
    - Path: repo://0112-papers3-reader-primitives/main/app_storage.cpp
      Note: Serialized catalog + static scratch fix (commit a5de610)
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

## Step 4: Phase 2 — s3paper_core primitives, fake backend, and the M5 transaction shell

Implemented the host-testable rendering substrate: a pure `s3paper_core` component (geometry, status/result vocabulary, DrawOps, frame arena, clip-stack FrameBuilder, fake backend) with 237 host checks under ASan/UBSan, and a `s3paper_m5` backend that is the only module permitted to call `M5.Display`. The same deterministic 74-op fixture now renders through both backends behind one `DisplayBackend` boundary, selected by a `fixture [fake|m5]` console command that round-trips through the owner task like everything else.

The M5 dependency decision is now a real decision record instead of an inherited accident: `s3paper_m5/idf_component.yml` pins upstream `m5stack/m5unified ==0.2.18` and `m5stack/m5gfx ==0.2.25` from the component registry (exact hashes in the committed `dependencies.lock`), explicitly avoiding the locally patched `M5PaperS3-UserDemo/components` checkout left over from the display investigation.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Continue phase-by-phase: implement task group P2.1–P2.11.

**Inferred user intent:** Host-provable rendering primitives before any refresh-policy or text work.

**Commit (code):** a00161ef3ce9b29563f1788050033023006bcb0a — "Reader: add s3paper_core primitives, fake backend, and M5 shell (Phase 2)"

### What I did
- `components/s3paper_core/`: `status.h` (stable `StatusCode` + `Result<T>`), `geometry.h/.cpp` (half-open rects, int64 intermediates, Intersect/Union/ClampTo/Translate/Shrink/RotateInBounds, single authoritative `AlignDamageForEpd`), `draw_ops.h` (POD ops with arena-offset payloads), `frame_arena.h/.cpp`, `frame_builder.h/.cpp` (clip stack depth 8, drop-and-count fully-clipped ops, damage union, `CorruptData` on unbalanced clips), `fake_backend.h/.cpp` (normalized text traces, explicit truncation flag).
- Host tests `components/s3paper_core/tests/host/` (Makefile + test_main.cpp): geometry overflow at INT32 extremes, half-open edge semantics, rotation round trips, EPD alignment for widths 1..16 and all four corners, arena capacity/reset/alignment, builder clipping/capacity/lifetime, golden fake-backend trace, determinism, truncation.
- `components/s3paper_m5/`: `M5Backend` transaction shell — bounded 5 s busy-wait (never a blind `waitDisplay()`), naive intent→`epd_mode_t` map, `startWrite`/per-op clip rects/`endWrite`, per-present metrics via `ESP_LOGI`, Bitmap explicitly counted as skipped.
- Firmware integration: unified `reader::StatusCode` onto `s3paper::StatusCode`; `DisplayServiceInit()` allocates op array (512), arena (32 KB), and trace buffer (16 KB) in PSRAM inside the owner task; `ConsoleOp::Fixture` (arg selects backend, 15 s reply timeout because a CleanFull EPD present takes seconds) and a real `display` diagnostics snapshot.
- Validated on hardware: `fixture fake` printed the normalized 74-op trace (clip demo confined a full-screen fill to `bounds=300,340,100,100 clip=300,340,100,100`); `fixture m5` initialized M5Unified (board=19, 540x960) and presented the identical frame: `ops=74 skipped=0 damage=0,0,540,960 render_us=203829 status=Ok`. Transcripts: `scripts/output/0112-phase2-validation-01.log`, `-02-m5.log`.

### Why
- The design doc's Phase 2 gate: host tests must cover overflow/edge geometry, and one fixture must produce both an expected trace and a hardware frame through the same boundary. Everything above exists to make later display-policy work (Phase 3) a planner change, not an application rewrite.

### What worked
- Host suite passed first run (237 checks, ASan/UBSan clean).
- The registry pin resolved and built against IDF 5.3.4 without patches.
- Both backends consumed the identical frozen frame; ownership rules held (fixture runs inside the owner; the console only sees the POD `PresentResult`).

### What didn't work
- First target build: `Failed to resolve component 'm5gfx'` — m5unified 0.2.18's registry manifest does not declare m5gfx as a dependency; fixed by pinning `m5stack/m5gfx ==0.2.25` explicitly.
- Second build round, exact errors:
  - `error: union member 'reader::AppReply::<unnamed union>::present' with non-trivial 'constexpr s3paper::PresentResult::PresentResult()'` — the geometry structs' default member initializers (`int32_t x = 0;`) made `Rect` (and so `PresentResult`) non-trivially-default-constructible and thus illegal in the reply union. Fixed by stripping the initializers (plain aggregates; value-init `{}` where zeroing is needed).
  - `error: deducing from brace-enclosed initializer list requires '#include <initializer_list>'` in `app_display.cpp` (same class of miss as Step 2).
  - `-Werror=format=` on xtensa: `int32_t` is `long int`, so `%d` needs `static_cast<int>` in `ESP_LOGI`/`printf` (three sites).

### What I learned
- On this xtensa toolchain `int32_t` is `long int`; every `%d` of a geometry field needs a cast, and custom variadic helpers (fake-backend `Append`) silently escape `-Wformat` checking.
- `wait_us=24` after `endWrite` suggests `displayBusy()` clears almost immediately on this M5GFX path; real flush time is inside the write phase (`render_us≈204ms`). Phase 3 instrumentation must not equate `wait_us` with panel busy time.

### What was tricky to build
- Union-compatibility versus ergonomic defaults: the cleanest fix (removing default member initializers from `Point/Size/Insets/Rect`) ripples into every default-constructed use; it worked because all uses already aggregate-initialize, and the host suite proved it immediately.
- Keeping the trace normalization stable enough for golden-string tests while still human-readable: glyph text is escaped byte-wise (`\x22`) so arbitrary UTF-8 cannot corrupt trace parsing.

### What warrants a second pair of eyes
- **Operator visual check (blocking tb0m):** the panel should now show the fixture — border, corner squares, 1..16 px width ladder, 16-step gray ladder, checkerboard, a mid-gray 100x100 clip window, crosshair lines, and the text "s3paper phase2 fixture". Please confirm/photograph; the software result alone makes no optical claim.
- The naive intent→`epd_mode_t` map in `m5_backend.cpp` (CleanFull→`epd_quality` etc.) — placeholder policy that Phase 3 must own.
- `AlignDamageForEpd` right-edge behavior: the aligned right edge re-clamps to `bounds.w` (540 is not a multiple of 8), so edge damage ends unaligned at the panel border. Tested and documented, but a driver constraint might require full-block alignment instead.

### What should be done in the future
- Task `lvjt` stays open: replace the provisional `align_x=8` with a Phase-0-measured driver constraint when hardware qualification resumes.
- Task `tb0m` stays open until the operator confirms the visible frame (a photo under `scripts/output/` would close it).
- Phase 3 (`hdvv`, P3.1–P3.10): refresh planner, damage merge, present metrics, clean-full triggers, soak test.

### Code review instructions
- Start at `components/s3paper_core/include/s3paper/` (contracts first), then `frame_builder.cpp`, then `components/s3paper_m5/src/m5_backend.cpp` (the transaction shell), then `main/app_display.cpp` (`BuildFixture`).
- Validate host: `cd 0112-papers3-reader-primitives/components/s3paper_core/tests/host && make run`.
- Validate hardware: flash per README, then `python3 ttmp/.../scripts/52-papers3-console-client.py --settle 12 --cmd "fixture fake" --cmd "fixture m5" --cmd display`.

### Technical details

```text
component pins  -> m5stack/m5unified ==0.2.18, m5stack/m5gfx ==0.2.25 (dependencies.lock committed)
host tests      -> PASS: 237 checks, 0 failures (g++ -fsanitize=address,undefined)
fixture (fake)  -> present id=1 intent=CleanFull ops=74 damage=0,0,540,960; clip demo op:
                   op kind=FillRect gray=128 bounds=300,340,100,100 clip=300,340,100,100
fixture (m5)    -> init board=19 display=540x960
                   present id=2 intent=CleanFull ops=74 skipped=0 damage=0,0,540,960
                   render_us=203829 wait_us=24 status=Ok
frame storage   -> 512 ops + 32KB arena + 16KB trace in PSRAM, allocated in owner task
```

## Step 5: Phase 3 — refresh planner and the 10,000-update hardware soak

Implemented the refresh planner as the single owner of display refresh policy and proved it on hardware. `RefreshPlanner` lives in pure `s3paper_core`: damage rects are clamped, EPD-aligned, and distance-merged (with an explicit collapse-to-bounding-box fallback when all 8 slots fill), intents map to backend-neutral `EpdWaveform` classes, and clean fulls are forced by first render, wake, screen change, explicit request, or turn/area/elapsed budgets. Every owner present now goes through `PresentPlanned()`, and every plan carries a `RefreshReason` — the planner can explain each full refresh, which is the heart of the Phase 3 gate.

The soak fixture is the part worth reading: instead of blocking the owner for minutes, `soak start` keeps exactly one self-posted `TimerDue` event circulating through the ordinary bounded queue. Each step builds a small deterministic frame (prime-scattered 64x48 region, cycling intents), plans, presents on the M5 backend, and updates per-waveform stats; console commands interleave naturally with a running soak. The 10,000-step run finished in 148 s with zero errors, zero heap-integrity failures (40 checks), heap byte-stable after one-time M5 init, and exactly 154 fulls — 10000/65, matching the 64-turn budget to the step.

### Prompt Context

**User prompt (verbatim):** "keep a diary (see skill for format), make sure to not forget that."

**Assistant interpretation:** Mid-task reminder to keep writing diary steps in the skill's strict format as each phase lands (Steps 1–4 already existed; this step records Phase 3).

**Inferred user intent:** The paper trail matters as much as the firmware; don't defer documentation to the end.

**Commit (code):** 7aec26e377eb0150f5309df94f40072362cc825c — "Reader: add refresh planner and 10k hardware soak (Phase 3)"

### What I did
- `s3paper_core/include/s3paper/refresh_planner.h` + `src/refresh_planner.cpp`: policy (`max_turns_between_full=64` in firmware, merge_distance 16, align_x 8), history, `AddDamage` (cascading merge, capacity fallback counted in history), `Plan` (priority: FirstRender > Wake > ScreenChange > Explicit/CleanFull > BudgetTurns > BudgetPartialArea > BudgetElapsed), `RecordPresent`.
- Host tests: damage merge semantics (gap < merge_distance merges; gap == merge_distance does not — half-open expansion), capacity fallback, and a scripted synthetic history walking every full-refresh trigger and the history arithmetic (now 299 checks total, still ASan/UBSan clean).
- Firmware: `PresentPlanned()` in `app_display.cpp` (planner-forced fulls present as `CleanFull`), `RunSoakStep()`, owner-side `SoakState` + `RunOneSoakStep()` + `MaybeQueueSoakStep()` self-posting loop, `ConsoleOp::{Refresh,SoakStart,SoakStatus}` with `RefreshSnapshot`/`SoakSnapshot` POD replies, console commands `refresh` and `soak start [n]` / `soak status`.
- Ran on hardware: planner fixture sequence (FirstRender → ExplicitRequest fulls), then `soak start 10000`. Transcripts: `scripts/output/0112-phase3-validation-01.log`, `-soak-10k-final.log`, `-soak-status-timeout-fix.log`.

### Why
- Phase 3 gate: "10,000 mixed bounded updates complete without heap corruption; the planner explains every full refresh." Both are now demonstrated; only the visual-artifact half (operator judgment) remains.

### What worked
- Soak: `10000/10000 errors=0 fulls=154 partials=9846`, `integrity checks=40 failures=0`, heap `min == now` after init, all 10,000 internal events accepted in order with zero queue rejects (high_water=2 — the self-posting design keeps the queue almost empty).
- `refresh` history reconciles: `turns_since_full=54` after 10,000 steps ≡ 10000 mod 65.

### What didn't work
- `soak start 10000` first ran only 16 steps: `RunConsoleOpWithArg` still declared `uint8_t arg`, truncating 10000 (0x2710) to 0x10. Fixed to `uint32_t`. Lesson: the demoted-parameter class of bug survives even when the wire struct is right.
- `soak status` during a soak hit `error: reply wait failed: Timeout (timeout 500ms)` — the owner can be inside a ~1 s full refresh when the status event lands. Raised that command's reply bound to 15 s; verified mid-soak queries now answer while steps continue interleaving.

### What I learned
- `render_us` dominates and `wait_us` is usually microseconds (occasionally ~44 ms) on this M5GFX path: `displayBusy()` clears long before the panel physically settles, so soak timing measures software cost, not ink physics. Phase 3's optical claims must come from the operator/photos, not these counters.
- Full-refresh cost in the soak (~3-9 ms render for 1-2 ops) is op-count-bound, not area-bound, on the write side; the panel-side settle is invisible to us (same lesson as above, quantified).

### What was tricky to build
- Keeping the soak from starving the console: a naive `for (10000)` loop inside one event handler would block every reply for minutes. The self-posting design (one `TimerDue` in flight, re-armed in the owner loop after every event) makes soak progress and console traffic share the queue fairly, and a failed re-post self-heals because every subsequent event re-runs `MaybeQueueSoakStep()`.
- Merge semantics at the boundary: expansion by `merge_distance` with half-open intersection means a gap of exactly `merge_distance` does not merge. The first test data assumed inclusive; the planner was right and the test was fixed, with the boundary now pinned by an explicit check.

### What warrants a second pair of eyes
- **Operator visual check (blocking 71dg):** after the soak the panel has seen ~10k small updates plus 154 fulls; please assess ghosting/artifacts and photograph the panel for the baseline-policy record. The last soak leaves scattered rects; run `fixture m5` first for a clean reference scene.
- The soak's `heap start=350591 → 299991` drop is one-time M5Unified/M5GFX init (lazy at first step), not a leak — verified by `min == now` across all 10k steps — but worth a skeptical read.
- Plan regions are currently metrics-only: the backend redraws all frame ops (each op carries its clip). True region-limited redraw arrives with retained widgets (Phase 9); confirm this staging is acceptable.

### What should be done in the future
- P3.4 (`k7og`) partially done: per-present logging exists, but queue-wait time and true panel-busy time are not yet measured; needs event-timestamp deltas and a qualified busy signal.
- P3.8 (`cmmr`): build the committed visual corpus (checkerboards, gray bars, inverse text, folios, page pairs) — the phase 2 fixture is a start, not the corpus.
- P3.3 (`1ow9`) and P2.4 (`lvjt`) stay open pending Phase 0 hardware qualification (waveform mapping + measured alignment).
- Next implementation phase: Phase 4 input/gestures/scheduler (P4.1–P4.9).

### Code review instructions
- Read `s3paper_core/src/refresh_planner.cpp` (AddDamage merge cascade and Plan trigger ordering), then `TestPlannerFullTriggers` in the host suite, then `RunOneSoakStep`/`MaybeQueueSoakStep` in `main/app_owner.cpp`.
- Validate host: `cd 0112-papers3-reader-primitives/components/s3paper_core/tests/host && make run` (299 checks).
- Validate hardware: `python3 ttmp/.../scripts/52-papers3-console-client.py --settle 3 --cmd "soak start 1000" --cmd "soak status"`, then poll `soak status` until `active=0` and read `refresh`.

### Technical details

```text
soak (10k)   -> progress=10000/10000 errors=0 fulls=154 partials=9846 elapsed_ms=148024
                heap start=350591 now=299991 min=299991 (one-time M5 init delta)
                integrity checks=40 failures=0
waveforms    -> Quality n=308 avg=8.9ms | Text n=2462 avg=40.0ms
                Fast n=4769 avg=4.5ms  | Fastest n=2461 avg=2.8ms (max ~58ms each)
planner      -> policy max_turns=64; history turns_since_full=54 = 10000 mod 65
queue        -> internal accepted=10000 rejected=0 high_water=2 out_of_order=0
plan proof   -> fixture #1 reason=FirstRender, #2 reason=ExplicitRequest (CleanFull intent)
```

## Step 6: Phase 4 — input primitives and the GT911 pipeline

Implemented the input layer as pure host-testable primitives plus a thin hardware pipeline. `s3paper_core/input.h` provides: `TouchToLogical` (the exact inverse of `RotateInBounds`' point maps, round-trip tested for all four rotations and all corners), `PointerTracker` (deduplicates identical samples, cancels stale sequences whose release sample was lost), `GestureDetector` (tap, long-press via time-driven `Update()`, cardinal swipes with off-axis ratio rejection), z-ordered deterministic `HitTest` (highest z wins, paint order breaks ties), and a fixed-capacity monotonic `Scheduler` where re-adding an id reschedules it — the primitive behind quiet/deferred region scheduling.

On the firmware side the ownership rule shaped the design: `M5.update()`/`M5.Touch` mutate M5Unified state, so the poll must happen in the owner task. A dedicated tick-producer task posts a `TimerDue` every 20 ms (never touching M5 or the model); the owner does the GT911 read, feeds the tracker/detector, reschedules the 2 s quiet-window deadline on every input, and exposes everything through `touch [on|off|status]`.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Continue phase-by-phase: implement task group P4.1–P4.9.

**Inferred user intent:** Stable logical input events with host-provable semantics before the reader UI consumes them.

**Commit (code):** 0024622e9f2a8af861735cd3bfed61cd57f6612f — "Reader: add input primitives and GT911 pipeline (Phase 4)"

### What I did
- `s3paper_core/include/s3paper/input.h` + `src/input.cpp` (all pure), added to both the IDF component and the host suite.
- Host tests (now 455 checks): rotation round-trips against `RotateInBounds`, tracker dedup/stale-cancel, gesture corpus (tap, slow-release rejection, right/up swipes, diagonal rejection, long-press firing once from `Update()` and swallowing its `Up`, cancel suppression), hit regions (z-order, paint-order ties, explicit miss), scheduler (order, reschedule-by-readd, cancel, capacity), and a recorded 11-sample replay fixture asserting the exact event and gesture sequence including a stale-touch `Cancel`.
- `M5Backend::ReadTouch()` (the only GT911 access), `EnsureM5Init()`/`ReadM5Touch()` pass-throughs in `app_display`, `main/app_input.{h,cpp}` (owner-only state + tick producer), `ConsoleOp::Touch` + `TouchSnapshot`, console `touch` command.
- Validated on hardware: `touch on` initializes M5 and polling runs (samples counting at 50 Hz, internal tick events accepted in order, queue high_water=1). Armed a read-only capture and asked the operator to interact with the panel for real-gesture evidence.

### Why
- Phase 4's gate: stable logical events independent of raw polling, with recorded-trace host replays. Everything gesture-shaped is decided in pure code; the firmware only samples and forwards.

### What worked
- Host suite passed after one real fix (see below); hardware pipeline runs with zero queue rejects at 50 Hz.

### What didn't work
- First host run failed `RunGestureTrace(detector, diag, 2, out, 4) == 0`: a diagonal drag with **no Move samples** (coarse polling) slipped past the tap check because `moved_beyond_tap_` only tracked Move events. Real bug, not a test bug: the detector now also requires the Up position to be within tap distance.
- `touch on` initially hit the 500 ms reply timeout — `M5.begin()` takes seconds; raised that command's bound to 15 s (same class as the fixture/soak-status lessons; long-running owner work needs matching reply bounds).
- Initial link failed with `undefined reference to s3paper::PointerTracker::Feed(...)` — `input.cpp` was in the host Makefile but not the IDF component SRCS.

### What I learned
- M5.Touch coordinates already arrive in the panel's logical orientation, so the firmware path uses them directly at rotation 0; `TouchToLogical` is exercised on host and will matter when the reader adds rotation support.
- The tick-producer pattern (dumb producer + owner-side polling) keeps the single-owner rule intact even for hardware that must be polled, at the cost of one 20 ms task.

### What was tricky to build
- The inverse rotation transforms: my first attempt used the wrong bounds dimension for rotations 1 and 3 (physical height where the logical height — which equals the physical *width* — was required). The round-trip-against-`RotateInBounds` test pinned it immediately; deriving the inverse from the forward map's algebra rather than intuition is the way.

### What warrants a second pair of eyes
- **Operator action needed:** tap, swipe (left/right/up/down), and long-press the panel while touch polling is enabled, then run `touch status` — the counters and gesture log are the P4.9 hardware evidence. A capture monitor is armed for this session.
- Long-press position tolerance: movement beyond `tap_max_dist` suppresses long-press entirely (deliberate, prevents drag-then-hold misfires) — confirm that matches intended reader UX.
- The quiet-window counter currently only proves the mechanism; nothing consumes it until widgets exist.

### What should be done in the future
- P4.5 hit regions are implemented and tested but not yet fed by layout output (layout emits them in Phase 9).
- Wire gesture events into `AppEventKind::Pointer` payloads for controllers once Phase 8 states exist (currently gestures only update counters/logs).
- Next: Phase 5 (fonts, UTF-8, measured text layout) — the biggest remaining pure-core chunk before the vertical slice.

### Code review instructions
- Start at `s3paper_core/src/input.cpp` (`TouchToLogical` comments, `GestureDetector::Feed` Up handling), then `TestInputReplayFixture` in the host suite, then `main/app_input.cpp` (`InputHandleTick`).
- Validate host: `make run` in the host test dir (455 checks).
- Validate hardware: `python3 ttmp/.../scripts/52-papers3-console-client.py --settle 8 --cmd "touch on"`, touch the panel, then `--cmd "touch status"`.

### Technical details

```text
poll rate     -> 20 ms tick producer (prio 4, core 0); owner does M5.update()
pipeline      -> sample -> PointerTracker -> GestureDetector(+Update) -> counters
quiet window  -> Scheduler id=1 re-added at last_input + 2s; PopDue in tick
hardware      -> touch on: enabled=1, samples at 50 Hz, internal events ordered,
                 queue high_water=1, no rejects
host checks   -> 455 total (input adds ~156)
```

## Step 7: Phase 5 — text pipeline with one metrics source

Implemented the text stack around the design doc's central constraint: layout and rendering must use the same metrics or pages drift and locators lose meaning. The solution vendors the Adafruit-GFX FreeSerif 12pt/18pt data files verbatim from the pinned m5gfx 0.2.25 into `s3paper_core/fonts/` (GNU FreeFont, GPL + font-embedding exception, documented in `fonts/README.md`). Host-side measurement, line breaking, pagination (later), and device-side blitting all read the same glyph arrays.

`s3paper/text.h` provides `Utf8Next` (malformed input yields U+FFFD advancing exactly one byte — progress guaranteed, byte positions exact), a font/metrics API with a deterministic fallback box glyph for uncovered codepoints, `SplitParagraphs` (CRLF-tolerant), and `BreakLines` — greedy measured breaking at spaces with long-word hard breaks at codepoint boundaries. The M5 backend's GlyphRun path dropped the Phase 2 builtin-font placeholder and now blits vendored glyph bitmaps as horizontal runs of set bits (`writeFastHLine` per run). `fixture text` renders a two-paragraph Alice in Wonderland page (including an `étude` to show the fallback box) through decode → split → break → GlyphRun → planner → M5.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Continue phase-by-phase: implement task group P5.1–P5.10.

**Inferred user intent:** Real measured body text with lowercase — the capability whose absence made the 0080 reader a dead end.

**Commit (code):** 9c51e5c5b20f7613307449563fcd53a36d73a592 — "Reader: add text pipeline with vendored FreeSerif metrics (Phase 5)"

### What I did
- Vendored `FreeSerif12pt7b.h`/`FreeSerif18pt7b.h` (byte-identical to the pinned m5gfx) + license README; mapped their `GFXglyph`/`GFXfont` names onto s3paper structs so the data files stay unmodified.
- `s3paper_core/src/text.cpp`: UTF-8 decoder (overlong/surrogate/truncation rejection), `GetGlyphMetrics` with fallback, `MeasureText`, `SplitParagraphs`, `BreakLines`.
- Host tests (564 checks): 1–4-byte decoding, each malformed class advancing one byte, lowercase coverage, fallback for accents, measurement monotonicity, line-break invariants (no leading/trailing spaces, width == independent re-measurement, monotone spans), long-word hard breaks, no mid-UTF-8 splits (every line re-decodes cleanly), capacity/invalid-arg, and a **pinned golden segmentation** of an Orwell sentence at 460 px.
- `M5Backend`: `BlitGlyph` (bit-run HLine blitter) + `RenderGlyphRun`; unsupported font ids count as `ops_skipped`.
- `BuildTextPage` fixture + `fixture text` console arg; rendered on hardware: `ops_drawn=22 status=Ok`, planner `reason=FirstRender waveform=Quality`, render 358 ms for a full page of 18 pt text.

### Why
- Phase 5's gate is body-text quality with shared metrics. The golden host test and the on-panel page come from the identical code path up to the backend boundary.

### What worked
- 564 host checks on first run after the pipeline landed (including the golden — the breaker and metrics agreed immediately).
- Full text page renders in 358 ms via the run-blitter — acceptable without any glyph caching yet.

### What didn't work
- A flash raced the still-armed Phase 4 read-only capture monitor (it held the port flock; esptool doesn't flock): the device kept running the old firmware and `fixture text` answered `InvalidArgument: usage fixture [fake|m5]`. Stopped the monitor, reflashed cleanly, verified. Lesson recorded: **stop any capture monitor before flashing** — the single-owner serial rule applies to my own background tasks too.
- The console client refused to start with `BlockingIOError: [Errno 11] Resource temporarily unavailable` for the same reason (its flock is doing its job).

### What I learned
- GFX 7b fonts cover 0x20–0x7E only; accents arrive as fallback boxes for now. Proper Latin-1+ coverage needs either a larger vendored table or a font-format change — that is the still-open P5.1 comparison (`3r0u`).
- `étude`'s é box on the panel is the visible proof the fallback path runs (deliberate fixture content).

### What was tricky to build
- The line breaker's three-way exit (word fits / break at last word / hard-break inside an oversized word) with the progress guarantee (at least one codepoint per line) — the hard-break path re-measures its span so `LineSpan.width` stays consistent with `MeasureText`, which the tests then assert for every line.

### What warrants a second pair of eyes
- **Operator:** the panel now shows the Alice page — please judge body-text quality/readability (P5.10, `zfpj`) and note the é fallback box. Touch polling is re-enabled and a capture is armed: taps/swipes/long-presses now also close the Phase 4 evidence (`utsz`).
- The greedy breaker has no hyphenation and treats only ASCII space as a break opportunity (no NBSP handling); fine for Phase 5, must be revisited with justification requirements.
- `BuildTextPage` baseline math approximates ascent with `y_advance`; real page layout (Phase 7) should derive ascent from glyph extents.

### What should be done in the future
- `3r0u` (typography requirements + font-format comparison) stays open — the vendored GFX data is a pragmatic Phase 5 baseline, not the final reader font decision.
- Glyph blit performance: consider per-glyph caching or pushImage batching when page-turn latency matters (Phase 8).
- Next: Phase 6 (SD content sources, catalog, persistence) needs a microSD card in the device — confirm one is inserted — or Phase 7 (pure streaming pagination) can proceed host-first without hardware.

### Code review instructions
- Read `s3paper_core/src/text.cpp` (`Utf8Next` validity rules, `BreakLines` exits), then the Phase 5 tests (`TestUtf8` … `TestGoldenLineBreaks`), then `RenderGlyphRun`/`BlitGlyph` in `m5_backend.cpp`, then `BuildTextPage` in `main/app_display.cpp`.
- Validate host: `make run` (564 checks). Hardware: `--cmd "fixture text"` then inspect the panel.

### Technical details

```text
fonts        -> vendored FreeSerif12pt7b (kFontUi) / FreeSerif18pt7b (kFontBody),
                y_advance 29 / 42 px; coverage 0x20-0x7E + fallback box
golden       -> "It was a bright cold day in April, ..." @460px/kFontBody
                = 3 exact lines (pinned strings in TestGoldenLineBreaks)
hardware     -> fixture text: ops_drawn=22 skipped=0 damage=0,0,540,960
                render_us=358425 status=Ok plan reason=FirstRender
host checks  -> 564 total (text adds ~109)
```

## Step 8: Phases 7–8 — streaming pagination and the reading vertical slice

This step turned the primitive stack into an e-reader. `s3paper_core` gained `ContentSource` (pure interface + `MemoryContentSource`; the SD adapter arrives with Phase 6) and `Paginator`: locator-based streaming page composition inside an 8 KiB window, sparse checkpoints (128-slot ring) with bounded backward reconstruction for previous-page, context-hash locator validation, and non-blocking permille progress. Positions are locators, never page numbers, exactly as the design doc demands. Mid-word page seams from window truncation are avoided by dropping a window-edge paragraph's final broken line and recomposing it from a fresh window on the next page.

The firmware side is the first actual reading experience: `app_reader.cpp` (owner-only) opens an embedded Alice fixture (~4.7 KB), renders pages with a title header, measured body lines, and a progress footer through the frame builder and refresh planner, and maps gestures to page turns (tap right half / swipe left = next; tap left half / swipe right = previous). The `reader` console command mirrors the same operations through the event queue. On hardware: 9-page traversal to 100%, previous-page returns byte-exact to earlier page starts, and begin/end refusals are explicit `InvalidArgument` with human-readable console output.

Real operator touch evidence also landed this step (preserved as `scripts/output/0112-phase4-touch-gestures-operator.jsonl`): 3 taps, 3 SwipeLeft, 1 SwipeUp, and a quiet-window firing — closing the Phase 4 hardware task (`utsz`).

### Prompt Context

**User prompt (verbatim):** "let me know when you need me to test touch"

**Assistant interpretation:** The operator is available for hardware interaction; call for touches at the right moment (done — mid-step, while the text fixture was on screen), and continue implementation.

**Inferred user intent:** Keep autonomous progress going; involve the human only where a finger on the panel is genuinely required.

**Commit (code):** 3ec8c0ce6936a53aa728b19aa924fe4dd0b76824 — "Reader: add streaming paginator and reading vertical slice (Phases 7-8)"

### What I did
- `s3paper/content.h|.cpp`: `ContentSource` (Size/ReadAt/Hash), `MemoryContentSource`, shared FNV-1a; identity = FNV over first 4 KiB + size.
- `s3paper/paginator.h|.cpp`: `TextLocator` (offset + 16-byte context hash), `LayoutKey` (+ `kLayoutEngineVersion`), `ComposePage`, `PreviousPageStart` (checkpoints → bounded back-scan for a paragraph boundary → forward reconstruction), `Validate`, `ProgressPermille`.
- Host tests (now 4541 checks): content-source reads/identity, 200-paragraph forward walk (per-line bounds, monotone coverage, at_end), prev round trips with warm checkpoints and with a cold cache, empty/one-line books, a 40 KB single paragraph (window bound), malformed UTF-8 books, locator corruption detection.
- Firmware: `app_reader.{h,cpp}` + `app_reader_book.h` (embedded fixture), `ConsoleOp::Reader` + `ReaderSnapshot`, gesture hook from `app_input` into the reader (both owner-context), console `reader [open|next|prev|status]`.
- Hardware validation transcripts: `0112-phase8-validation-02.log` (round trip 0→580→1151→580→0 with identical snapshots at repeated offsets), `-03-walk.log` (walk to `at_end=1`, two explicit end refusals).

### Why
- Phase 8's gate is a usable native reading slice with no JavaScript. Pagination had to be locator-based and streaming *first*, or the reading UI would have baked in page-number assumptions that Phase 6 persistence could never honor.

### What worked
- The full pipeline — content → decode → break → compose → GlyphRun → planner → M5 — produces a readable paged book with correct progress on real hardware.
- Cold-cache previous-page (no checkpoints) reconstructs a valid earlier page start via the bounded backward scan, and composing forward from it reaches the target page exactly.

### What didn't work
- The first hardware run showed page starts drifting (`0 → 1151 → 1692` where the corrected build gives `0 → 580 → 1151`) with stale line/progress values after `prev`. Root cause: `ComposeAndRender(s_state.page.next)` bound a `const TextLocator&` into `s_state.page`, and `ComposePage` overwrites `out->next` before the caller copies the locator — the classic alias-the-output hazard. Fixed by passing locators **by value** in `ComposeAndRender` and, defensively, in the core `Paginator::ComposePage` signature itself, with a comment explaining why. Host tests kept passing throughout because they never aliased — the lesson is that the core API allowed a hazard only integration code would hit.

### What I learned
- The observable symptom of the aliasing bug (page starts equal to what should have been *next* page's start) looked exactly like an off-by-one-page pagination error; only reasoning about storage lifetimes found it.
- 19 lines per page at FreeSerif 18 pt with these margins; the embedded chapter is 9 pages — a good smoke-test size.

### What was tricky to build
- Window-edge handling in `ComposePage`: distinguishing "paragraph ends at the window edge because the window cut it" from "paragraph genuinely ends here", and keeping the guarantee that every page consumes at least one byte even for degenerate content. The 40 KB single-paragraph test exists precisely to pin these paths.
- `PreviousPageStart` without checkpoints must not overshoot: it returns the last page start whose composed `next` reaches the target, bounded by a 4096-page guard returning explicit `Timeout`.

### What warrants a second pair of eyes
- **Operator:** the book is open on the panel with touch enabled — please page through by tap/swipe and judge the reading experience (page-turn latency, text quality, ghosting after several turns). This is the Phase 8 gate's human half.
- `PreviousPageStart`'s backward scan takes the *last* newline before the target; if a page starts exactly at a paragraph boundary the scan can land on the page itself and fall back to the window start — covered by tests via the forward-reconstruction bound, but the fallback path deserves review.
- The embedded book approach means `reader open` re-uses a function-static `Paginator`; when Phase 6 brings multiple books this must become per-book state with explicit invalidation.

### What should be done in the future
- Phase 6 (SD content sources, catalog, atomic persistence) — needs a microSD card inserted in the PaperS3; resume/bookmarks and multi-book library build on it. Left open: 07pv (persisted checkpoints), 4otq (round trips across reboot), vh8v (multi-megabyte books), yqb2 (full LayoutKey fields), hfge (page cache), P8 library/state tasks.
- Page-turn latency: full pipeline re-reads and re-blits every turn; a composed-page cache (hfge) is the obvious next optimization.

### Code review instructions
- Read `s3paper_core/src/paginator.cpp` (`ComposePage` window-edge branches, `PreviousPageStart`), then the paginator host tests, then `main/app_reader.cpp` (`ComposeAndRender` by-value note, `ReaderHandleGesture`).
- Validate host: `make run` (4541 checks). Hardware: `--cmd "reader open"` then `reader next/prev/status`, or tap/swipe the panel with touch on.

### Technical details

```text
book         -> embedded Alice fixture, 4708 bytes, 9 pages @ 19 lines
walk         -> 0/580/1151/1692/2257/2776/3370/3959/4548(end); progress
                12.3% .. 100.0%; prev returns byte-exact; refusals explicit
prev (cold)  -> backward scan + forward reconstruction verified on host
operator     -> gestures: 3x Tap, 3x SwipeLeft, 1x SwipeUp, 1 quiet window
                (0112-phase4-touch-gestures-operator.jsonl)
host checks  -> 4541 total
```

## Step 9: Operator approval of the reading experience

The operator paged through the embedded book by touch and approved the result: touch works, body text reads well, and ghosting is invisible even during fast page turns under the Phase 3 planner policy (64-turn full-refresh budget, Text/Fast partial waveforms). This closes the visual halves of the Phase 3 and Phase 5 gates that software evidence alone could not.

### Prompt Context

**User prompt (verbatim):** "looks good, touch works, ghosting is invisible even in fast turns. where does the font come from?"

**Assistant interpretation:** Operator sign-off on touch, text quality, and ghosting; plus a provenance question about the reader font.

**Inferred user intent:** Confirm the slice is genuinely usable and understand the font supply chain (licensing/quality implications).

### What I did
- Checked tasks `zfpj` (body-text quality approval) and `71dg` (ghosting baseline review) and recorded the approval in the changelog. No photographs were taken; the approval is verbal and dated.
- Answered the font question: GNU FreeFont **FreeSerif** (GPLv3 + font-embedding exception) → rasterized to Adafruit-GFX bitmap format (`7b` = ASCII 0x20–0x7E) → shipped in the pinned m5gfx 0.2.25 → vendored byte-identical into `components/s3paper_core/fonts/` so host layout and device blitting share one metrics source.

### What I learned
- The naive Phase 2/3 intent→`epd_mode_t` mapping is good enough that ghosting is not user-visible at reading cadence — the planner's budgeted fulls are doing their job on this panel despite the paused optical qualification.

### What warrants a second pair of eyes
- The approval is verbal; if a durable baseline record is wanted for `cmmr`/hardening later, a few photos of text pages and post-soak screens should be captured and committed.

### What should be done in the future
- `3r0u` remains the right place to revisit font coverage (accents currently render as fallback boxes) and the long-term reader font decision.

### Code review instructions
- N/A (bookkeeping step; no code changed).

## Step 10: TTF typography — stb_truetype + PT Serif with Ukrainian support

Executed the `3r0u` decision (recorded first in `design-doc/04-font-rendering-decision-stb-truetype-with-pt-serif.md`): the reader's typography moved from 7-bit bitmap fonts to runtime TTF rasterization. `stb_truetype.h` v1.26 is vendored into `s3paper_core/third_party/` and compiled identically on host and device — the strongest possible version of the one-metrics-source rule, since advances and kern pairs now come from the same code *and* the same font bytes everywhere. PT Serif Regular (SIL OFL, Cyrillic-first foundry) is subsetted to Latin + Ukrainian + punctuation (134 KB) by a checked-in reproducible script that also verifies every Ukrainian-critical codepoint (Ґґ Єє Іі Її, U+2019, «») is present, then embedded in firmware and registered at UI 22 px / body 34 px.

The pipeline gained kerning end to end: `MeasureText` and `BreakLines` accumulate `advance + kern(prev, cp)` with the breaker's accumulation deliberately mirroring `MeasureText` so the `line.width == re-measurement` invariant stays exact (host-asserted per line, including on the Ukrainian pangram). The M5 backend gained a 256 KB PSRAM glyph cache (explicit reset-on-full, counted) and a run-based coverage blitter that quantizes AA to the panel's 16 grays for TextPage/Quality intents and thresholds to 1-bit for fast waveforms — the waveform-aware AA policy from the decision record.

### Prompt Context

**User prompt (verbatim):** "ok, do stb, update, then implement"

**Assistant interpretation:** Adopt the stb_truetype + PT Serif recommendation (from the preceding font discussion, refined for Ukrainian), record the decision in the ticket, then implement and validate it.

**Inferred user intent:** High-quality Latin+Ukrainian typography in the working reader without compromising the host-testable architecture.

**Commit (code):** 55d273586a8e3adb1f6d6a4888b428254782b625 — "Reader: switch typography to stb_truetype + PT Serif Ukrainian subset"

### What I did
- `design-doc/04-*.md`: decision record (alternatives table, constraints incl. trusted-fonts-only policy, rollback path).
- `scripts/53-subset-pt-serif.sh`: downloads PT Serif from google/fonts, subsets via pyftsubset (pinned unicode ranges, `--layout-features=kern --legacy-kern`), installs font + OFL license into `s3paper_core/fonts/`, verifies Ukrainian coverage via ttx. Output sha256 recorded in the script output.
- `s3paper_core`: vendored `third_party/stb_truetype.h` (v1.26, public domain); `text.h/.cpp` gained `RegisterTtfFont`/`IsTtfFont`, `FontLineMetrics`/`GetFontLineMetrics` (GFX fallback derives ascent/descent from glyph extents), `GetKernAdvance`, `RasterizeGlyph`, TTF dispatch in `GetGlyphMetrics`, kerned `MeasureText`, kerning-aware `BreakLines`; deterministic integer advances via `lround(units * scale)`.
- `paginator.cpp`, `app_display.cpp`, `app_reader.cpp`: all raw `GfxFont::y_advance` uses replaced with `GetFontLineMetrics` (real ascent-based baselines).
- `m5_backend.cpp`: glyph cache + `BlitCoverage` (equal-value horizontal runs) + `RenderTtfGlyphRun` (kerned pen, fallback boxes); GlyphRun dispatch selects AA by present intent. Font embedded via `EMBED_FILES` in the s3paper_core component.
- Fixture text rebuilt as a typography acceptance page: kerning pairs, the Ukrainian pangram, apostrophe/guillemet cases, Latin accents, one CJK fallback box.
- Host tests: `TestUkrainianAndKerning` (coverage, `MeasureText("AV") == adv(A)+adv(V)+kern`, kerned break invariants, ґ rasterization), é moved from fallback-expected to covered, golden line breaks re-pinned for PT Serif 34 px (3 lines → 2; deliberate, dated comment). 3922 checks pass under ASan/UBSan.

### Why
- Ukrainian reading requires Ґґ Єє Іі Її and the typographic apostrophe; quality typography requires kerning and AA. The bitmap-font path could deliver none of these without a custom offline toolchain, and stb_truetype preserves the architecture's host/device metrics identity more cheaply than FreeType (~40 KB vs ~300 KB flash, negligible steady-state RAM difference once the glyph cache dominates).

### What worked
- On hardware: typography fixture `status=Ok` — 640 ms cold (full glyph-cache population), 260 ms warm; reader pages now 23 lines of PT Serif at ~450 ms per turn with byte-exact next/prev round trips (0 → 867 → 0).
- The kerned-accumulation-equals-MeasureText design held on the first full test run; only the golden needed re-pinning (expected — metrics changed by design).

### What didn't work
- The subset script's coverage check first ran under the wrong Python (`ModuleNotFoundError: No module named 'fontTools'` — pyenv shim provides `pyftsubset`/`ttx` but not the module in the default env); switched verification to `ttx -t cmap` + grep from the same install.
- pyftsubset warned `'kern' subtable longer than defined: 125762 bytes instead of 60226 bytes` on the upstream font — a source-font quirk it repairs during subsetting; kerning verified working by the AV test.

### What I learned
- PT Serif at 34 px sets ~23 lines per 960 px page (line height 44) vs 19 lines for the old 42 px bitmap font — denser and, per the fixture, more readable.
- The stb rasterizer under `-Wextra -Werror` needs `-Wunused-function`/`-Wsign-compare` suppressions around the include (STBTT_STATIC makes unused API static).

### What was tricky to build
- Keeping the line-break width invariant exact under kerning: the greedy breaker's word-fitting accumulation must apply kern pairs across word/space boundaries exactly as `MeasureText` does over the final span, with the kerning context (`prev_cp`) threaded through word scans and space consumption and reset at line starts. The Ukrainian-pangram per-line equality check pins this.
- The glyph cache's reset-on-full recursion (`GlyphCacheGet` → `GlyphCacheReset` → retry) is safe only because a single glyph can never exceed the pool; that invariant is a size argument (max ~2 KB per 34 px glyph vs 256 KB pool), not a code guard — noted for review.

### What warrants a second pair of eyes
- **Operator:** the typography fixture is on the panel — please judge PT Serif quality, the Ukrainian pangram (Ґ ґ є ї shapes), apostrophes, and whether AA text looks clean after a few page turns (tap returns to the book; touch is on).
- `BlitCoverage` assumes a white background when compositing coverage (documented); glyph boxes overlapping non-white surfaces would need real alpha blending.
- Page-turn render grew from ~360 ms to ~450 ms (AA edge runs are shorter than 1-bit runs). Acceptable now; candidates if it ever matters: 4-bit packed cache + `pushGrayscaleImage`, or caching composed line images.

### What should be done in the future
- Bold/italic PT Serif faces (same subset script) when the reader gains emphasis rendering.
- If user-supplied SD fonts ever become a feature: migrate parsing to libschrift or FreeType first (decision-record constraint #2).
- Consider U+02BC → U+2019 normalization in the content pipeline for Ukrainian books encoded with the modifier-letter apostrophe.

### Code review instructions
- Read `design-doc/04-*.md`, then `text.cpp` (`RegisterTtfFont`, TTF branch of `GetGlyphMetrics`, kerned `BreakLines`), then `m5_backend.cpp` (`GlyphCacheGet`, `BlitCoverage`), then `scripts/53-subset-pt-serif.sh`.
- Validate host: `make run` (3922 checks). Hardware: `--cmd "fixture text"` (Ukrainian page), `--cmd "reader open"` + page turns.
- Reproduce the font asset: run `scripts/53-subset-pt-serif.sh` and compare sha256 `718b987ce653eb65132cd3e83e896e37ee85d5d8b5b223129bb769bde1f1bef1`.

### Technical details

```text
font          -> PT Serif Regular subset, 134264 bytes, sha256 718b987c...
                 ranges: 0020-007E, 00A0-00FF, 0400-045F, 0490-0491,
                 2010-2027, 20AC, 2116; kern + GPOS pair kerning kept
sizes         -> kFontUi 22 px, kFontBody 34 px (body line_height 44,
                 23 lines/page)
kerning       -> MeasureText("AV") == adv(A)+adv(V)+kern(A,V), kern < 0
cache         -> 512 slots / 256 KB PSRAM, reset-on-full counted
hardware      -> fixture: 640 ms cold, 260 ms warm; reader page ~450 ms;
                 all presents status=Ok, plan reasons as expected
host checks   -> 3922 (goldens re-pinned for PT Serif; check-count drop
                 vs 4541 is denser pages => fewer pagination loop checks)
```

## Step 11: Phase 6 — SD library, content sources, and resume across reboot

With a microSD card freshly inserted by the operator, this step made the reader read real books from removable media and remember where you stopped. `app_storage.cpp` (owner-task-only) provides: a non-destructive SDSPI mount (`format_if_mount_failed=false`, pins per the vendor demo: MISO 40 / MOSI 38 / SCLK 39 / CS 47), a bounded deterministic library scan of `*.txt` in `/sdcard` and `/sdcard/books`, an `SdContentSource` whose FNV-head+size identity matches `MemoryContentSource` semantics, and reading-position persistence — a versioned, checksummed, fixed-capacity record file written atomically (tmp → fflush → fsync → rename, with the previous generation kept as `.bak` and used as load fallback).

The reader controller now opens library books by scan index (`reader open <n>`, gestures unchanged), persists the locator on every page turn (embedded book included — positions key on content hash), and on open validates a persisted locator against live content before resuming; a failed validation falls back to the beginning explicitly. The full loop was proven on hardware: read to 54.7% on an SD book, hard-reboot the device, remount, reopen — `resumed=1` at the exact byte offset.

### Prompt Context

**User prompt (verbatim):** "ok, continue then" / mid-turn: "i put a sd card in"

**Assistant interpretation:** Proceed with the next phase (Phase 6, SD content and persistence), now unblocked by real media.

**Inferred user intent:** A real e-reader: books from the card, position remembered across power cycles.

**Commit (code):** d9c59120dd96979fe3b1d60a90f5f67cb50a7055 — "Reader: add SD library, content source, and resume persistence (Phase 6)"

### What I did
- `main/app_storage.{h,cpp}`: mount/unmount/status, library scan (title-sorted, 32-book bound), `LibraryPrint` (owner prints; strings don't fit POD replies), `StorageWriteDemoBook` (writes the embedded Alice text to `/sdcard/books/alice-demo.txt`, never overwrites), `SdContentSource`, positions file (magic `S3RP`, version, CRC over payload, 32 records, oldest-slot replacement policy).
- Reader: `OpenCommon` (fresh heap paginator per open, resume lookup + `Validate`), `ReaderOpenSd(index)`, `PersistPosition()` after successful next/prev; snapshot gained title/source/resumed.
- Events/console: `ConsolePayload.arg2`, `ConsoleOp::{Sd,Library}`, `SdSnapshot`; commands `sd [mount|unmount|demo|status]`, `library [scan|list]`, `reader open [n]`.
- `sdkconfig.defaults`: `CONFIG_FATFS_LFN_HEAP=y`, `CONFIG_FATFS_MAX_LFN=255` (removed `sdkconfig` to apply — the documented seeding trap).
- Validation transcripts: `0112-phase6-validation-01.log` (first attempt, two real failures below), `-02.log` (mount 7580 MiB, demo write, 2-book scan, SD reading to offset 1701, `position_writes=2 failures=0`), `-03-resume.log` (reboot → `loaded 1 reading position(s)` → `resuming "alice-demo" at offset 1701` → `resumed=1`).

### Why
- Phase 6 is what turns the vertical slice into an e-reader: content from user media, identity by content hash (never list order), positions as validated locators (never page numbers), and persistence that survives interruption.

### What didn't work
- **SPI bus collision reboot:** the first `reader open 0` lazily initialized M5GFX *after* the SD mount owned the SPI bus — `spi: spi_bus_initialize(806): SPI bus already initialized`, then LGFX's failure path de-initialized the live bus (`spi_master_deinit_driver(345): not all CSses freed`) and the device rebooted. Root cause: nondeterministic bus-ownership order between two components sharing SPI2. Fix: `StorageMount` calls `EnsureM5Init()` first (M5 always owns bus setup) and treats `ESP_ERR_INVALID_STATE` from `spi_bus_initialize` as reuse.
- **8.3 filenames:** the scan listed `P3C_FW~1.TXT` and the demo-book write returned `CorruptData` — FATFS long-filename support is off by default; `alice-demo.txt` and the `.s3paper` state dir are illegal as 8.3 names. Fix: LFN via heap + fresh `sdkconfig`.

### What I learned
- M5GFX's PaperS3 preset touches the same SPI bus the SD card uses (heritage of the M5Paper shared-bus design); any firmware combining both must fix the init order deliberately.
- The card already contained a `.txt` from earlier PaperS3 experiments — the scan's non-destructive, additive behavior got a free real-world test.

### What was tricky to build
- Resume correctness across content change: a persisted locator is only trusted after `Paginator::Validate` re-hashes the bytes at the offset; an edited/replaced book falls back to page one rather than composing garbage from a stale offset. The content-hash identity (head + size) also means a re-written demo book with identical text resumes seamlessly.

### What warrants a second pair of eyes
- Position writes happen on *every* page turn (~10 ms SD write). Fine at reading cadence; if wear or latency ever matters, coalesce via the scheduler (quiet-window deadline already exists).
- `PositionsFile` capacity policy replaces slot 0 when full (explicit but crude); a proper LRU needs a timestamp field (schema v2).
- Card removal mid-read is handled only reactively (reads fail with explicit errors; `sd mount` recovers after reinsertion) — no removal detection task. Task `5zpj` (fault-injection tests) remains open.

### What should be done in the future
- On-screen library UI and bookmarks (rest of Phase 8's `ylnh`), catalog serialization (`i78k`), settings records (`1y51`), interrupted-write/removal test fixtures (`5zpj`), pagination-checkpoint cache on SD (`07pv`).
- Then Phase 9 (retained widgets — the JS binding surface) and Phase 10 (sleep/wake with final persistence flush).

### Code review instructions
- Read `main/app_storage.cpp` (`StorageMount` ordering comment, `PositionsSave` rename dance, `ScanDirectory` bounds), then `OpenCommon`/`PersistPosition` in `app_reader.cpp`.
- Validate: `sd mount`, `sd demo`, `library scan`, `reader open 0`, page a few times, reboot, `sd mount` + `reader open 0` → expect `resumed=1` at the persisted offset.

### Technical details

```text
card         -> 7580 MiB, FAT, preexisting P3C_FW_RESULT_AB.txt preserved
library      -> [0] alice-demo (4699 B, hash bd230159), [1] P3C_FW_RESULT_AB
positions    -> /sdcard/.s3paper/positions.bin, magic S3RP v1, CRC=FNV,
                32 records, .bak generation; writes=2 failures=0
resume proof -> reboot -> loaded 1 position -> resumed=1 offset=1701 (54.7%)
spi fix      -> EnsureM5Init() before esp_vfs_fat_sdspi_mount; INVALID_STATE
                from spi_bus_initialize treated as bus reuse
```

## Step 12: Phase 8 complete — library screen, bookmarks, and standalone boot

The vertical slice became a standalone product this step. The device now boots straight into reading: mount the card, scan the library, reopen the last book at its persisted position (a small versioned `lastbook.bin` record), else show the library screen — and touch enables automatically. No console interaction is needed to read from power-on.

The on-screen library renders the embedded book plus every scanned SD book (title, size, persisted-percent) with card-missing and empty-library states as rendered rows, and emits an **immutable hit-region array** per render — the Phase 4 `HitTest` finally consuming layout output (closing `dfp0` and with it the Phase 4 umbrella `a7sc`). Gestures are screen-aware: in the library, a tap hit-tests to a row and opens that book; in reading, swipe-down returns to the library, long-press toggles a bookmark on the current page (header `*` indicator, footer mark count), and taps/swipes turn pages as before. Bookmarks persist exactly like positions (versioned, CRC, atomic with `.bak`), and position writes now **coalesce**: stores mark a dirty flag, the owner loop flushes after 2 s of quiet (bounded 500 ms queue wait makes deferred work possible), and screen changes/shutdown flush immediately — the sda9 requirement, replacing the write-per-page-turn behavior.

### Prompt Context

**User prompt (verbatim):** "continue" / mid-turn: "check off the tasks you finished, btw..."

**Assistant interpretation:** Proceed with the remaining Phase 8 work (library UI, bookmarks, boot restore) and reconcile the ticket task list with what is actually done.

**Inferred user intent:** Finish the native e-reader product loop and keep the ticket bookkeeping trustworthy.

**Commit (code):** 4474aae7deadd1e0da52611af63fc179460964a3 — "Reader: complete Phase 8 - library screen, bookmarks, boot restore"

### What I did
- Task reconciliation first (user request): checked phase-level tasks whose gates were already met and operator-approved — `t1yc` (P2), `hdvv` (P3), `kkmo` (P5), `0dxd` (P7), `9gye` (P8 acceptance incl. power cycle) — leaving honest gaps open (`mcac` settings, `cmmr` corpus, `k7og` instrumentation, fault-injection `5zpj`, `r3wg` intern docs).
- `app_storage`: bookmark file (magic `S3MB`, 64 records, toggle-removes semantics), shared `AtomicWrite` helper, dirty-flag coalescing (`StorageFlushIfDue`/`StorageFlushNow`), `lastbook.bin` record, all loaded on mount.
- `app_reader`: `Screen` enum (None/Library/Reading), `LibraryShow()` with row renderer + hit regions + persisted-percent per book, `ReaderBookmarkToggle/Goto/Print`, `ReaderBootRestore()`, screen-aware `ReaderHandleGesture`, header bookmark indicator and footer mark count (fixing a printf format bug that would have printed "turns 30" for zero marks).
- Owner: boot flow calls `ReaderBootRestore()` + `TouchEnable()`; event loop uses a bounded 500 ms receive so flushes run without traffic; `ShutdownRequest` flushes persistence.
- Console: `library show`, `bookmark [list|toggle|goto n]`, extended `reader status` (screen/bookmarked/marks).
- Validated on hardware (`0112-phase8ui-*.log`): library render with 3 regions; bookmark set → page away → set second → list shows both → `goto 0` jumps 2572→4022; refusals explicit at book end; boot restore reopened the operator's last-tapped book after a reset.

### Why
- Phase 8's definition of done is a usable reader without JavaScript: library, reading, page turns, progress, bookmarks, resume. All six now exist and were exercised on hardware, so `ylnh` is checked.

### What worked
- The operator's tap on a library row (real finger, previous boot) opened `P3C_FW_RESULT_AB` — and after my reset, boot restore reopened exactly that book: two features accidentally validating each other.
- The hit-region path from render → immutable array → `HitTest` → open worked on the first hardware attempt.

### What didn't work
- My first post-flash `reader status` returned `screen=none` and looked like a boot-restore failure; it was a race — the boot flow (M5 init ~5 s + mount + per-book hash scan) takes ~30 s, and the query landed mid-flow. The settled state was correct. Lesson: status queries after reset must wait for the boot flow, or the boot flow needs a progress marker in `status`.
- A quick-and-dirty boot-log capture (background read-only capture + esptool reset + broken pgrep loop) timed out and left a straggler process holding the port; killed it and queried the settled state instead. The read-only capture tool is for preplanned evidence, not ad-hoc races.

### What I learned
- The library scan is slow (~15 s for 2 books) because each book's identity hash opens and reads 4 KiB over SDSPI plus FATFS mount overhead; fine at this scale, but a cached catalog (`i78k`) becomes performance-relevant, not just hygiene, with tens of books.
- `LibraryShow` doubles as the "leaving reading" flush point — a natural place for durability without a timer.

### What was tricky to build
- The coalescing needed a way to run deferred work in an event-driven owner: switching `xQueueReceive` from `portMAX_DELAY` to a bounded 500 ms wait keeps the single-owner model intact while giving flushes (and future deferred work) a heartbeat. The flush also runs after every event so busy periods don't starve it.

### What warrants a second pair of eyes
- **Operator:** the full product loop is live — power-cycle the device and it should come back reading your last book at your last page; swipe down for the library, tap to open, long-press to bookmark. Please try the loop end to end.
- Crash-window tradeoff: up to 2 s of reading position can be lost on abrupt power removal (the coalescing window). Bookmarks flush promptly (age forced to 0). Acceptable for now; Phase 10's sleep path flushes explicitly.
- Library screen shows only the first screenful (bounded, no scrolling) — fine for ≤14 books; scrolling is future work.

### What should be done in the future
- Library scrolling, corrupt-book row indicator, settings records (`1y51`), serialized catalog (`i78k`), fault-injection tests (`5zpj`), slice metrics/screenshots for intern docs (`r3wg`).
- Phase 9 (retained widgets — the JS binding surface) and Phase 10 (sleep/wake with wake sources) are the remaining pre-JS phases.

### Code review instructions
- Read `LibraryShow`/`ReaderHandleGesture`/`ReaderBootRestore` in `main/app_reader.cpp`, then the bookmark/lastbook/coalescing sections of `main/app_storage.cpp`, then the owner-loop bounded-wait change.
- Validate: power-cycle → device resumes last book; `library show` + tap; `bookmark toggle/list/goto`; `shutdown` then reboot → position retained.

### Technical details

```text
screens      -> None/Library/Reading; card-missing + empty as library rows
hit regions  -> up to 33 immutable rows/render, z=1, full-width strips
bookmarks    -> S3MB v1, 64 records, toggle-removes; header '*', footer count
coalescing   -> dirty flags + 2 s age; owner loop waits 500 ms; flush on
                screen change + shutdown; bookmark changes flush promptly
boot restore -> mount -> scan -> lastbook.bin -> reopen at persisted locator
                (validated: reopened operator's last-tapped book after reset)
evidence     -> 0112-phase8ui-validation-01/-02.log, -boot-restore.log
```

## Step 13: Serialized library catalog — boot scans without re-hashing

The library scan re-opened and re-hashed every book on every boot, which made the boot flow noticeably slow and would scale linearly with library size. This step added the serialized catalog from task i78k: the scan result persisted on the card itself, validated per file by path+size+mtime, so a steady-state boot only stats files instead of reading them. Along the way it produced the project's first genuine crash-loop, caused not by the catalog logic but by where its buffer lived.

### Prompt Context

**User prompt (verbatim):** "go ahead. do that one first, then do phase 9 and 10" — followed mid-turn by "continue keeping a detailed diary as you work (see skill for the format), and commit at appropriate intervals", "make sure to read the design doc to refresh your memory as well", and "seems to crash? not sure."

**Assistant interpretation:** Implement the serialized catalog (i78k) first, then proceed to Phases 9 and 10, keeping the diary and commit cadence, and grounding the work in the design doc (§11.2 catalog, Phase 9/10 work lists).

**Inferred user intent:** Eliminate the slow boot scan before moving on to the remaining architecture phases; keep the documentation trail intact for later review.

**Commit (code):** a5de610 — "Storage: add serialized library catalog to skip re-hashing unchanged books"

### What I did
- Re-read design doc §11 (storage/catalog), §12 (boot flow), and the Phase 9/10 sections before coding.
- Added `CatalogFile` (magic "S3CT", version 1, CRC, 32 fixed-size records of path/title/size/mtime/content_hash) to `main/app_storage.cpp`, persisted with the same AtomicWrite tmp/bak/rename pattern as positions and bookmarks.
- `ScanDirectory` now consults `CatalogLookup(path, size, mtime)` and only opens+hashes files with no matching record; `LibraryScan` rebuilds the catalog after sorting and rewrites it only when records changed (memset first so struct padding is deterministic for memcmp/CRC).
- `CatalogLoad` runs at mount next to PositionsLoad/BookmarksLoad; `StorageUnmount` drops the in-memory catalog so a swapped card cannot reuse stale hashes.
- Added `mtime` to `BookEntry`, scan stats (`scan_cached/scan_hashed/scan_ms/catalog_writes`) to `SdSnapshot`, and a second line to the console `sd` printer.
- Fixed the crash the user spotted: moved the ~5 KiB `CatalogFile` locals off the owner-task stack into a single static scratch buffer.
- Hardware evidence in `scripts/output/0112-catalog-{first,second}-boot.log`.

### Why
- Boot ran the full hash pass every time; the design doc (§11.4) explicitly wants derived caches disposable and separate from critical state, which this catalog is: deleting it costs one slow rescan, never a reading position.

### What worked
- First boot after flash: `scan cached=0 hashed=2 last_ms=84 catalog_writes=1`.
- After a hard reset: `scan cached=2 hashed=0 last_ms=13 catalog_writes=0`, and boot restore still resumed alice-demo at offset 2572 with `bookmarked=1 marks=2` — proving cached hashes key correctly into positions/bookmarks.

### What didn't work
- **First flash crash-looped the device.** Transcript: boot reached `W (6624) storage: catalog not loaded:` (status name garbled to empty — an early corruption symptom) then went silent; console commands never answered. Cause: `CatalogFile loaded;` and `CatalogFile fresh;` are ~5136 bytes each, allocated on the owner task's 8192-byte stack deep inside the boot call chain (StorageMount → CatalogLoad under M5/FATFS frames). The positions (656 B) and bookmarks (~1.3 KiB) files made the same stack-local pattern safe; scaling the record to 160 bytes × 32 silently broke it. Fix: one namespace-scope `s_catalog_scratch` reused by load and rebuild (owner-task-only file, so no aliasing risk).

### What I learned
- The measured scan itself is fast (84 ms hashing 2 books): the ~15 s boot observed earlier is dominated by M5 init, mount, and EPD present, not hashing. The catalog still matters — hashing cost is the only per-book term and now it's gone — but the honest number is 84 ms → 13 ms for 2 books, not 15 s → instant.
- Copying a persistence pattern must include re-checking its stack budget when the struct grows ~8×.

### What was tricky to build
- Comparing "did the catalog change" via memcmp requires every byte of the record array to be deterministic; snprintf leaves garbage after the NUL and the struct has tail padding. Symptom would have been a catalog rewrite on every boot (wear + latency); avoided by memset-ing the rebuilt file before filling records.

### What warrants a second pair of eyes
- FAT mtime granularity is 2 s and files written by a clock-less device carry epoch-era timestamps; size+mtime as the validation key is standard but an in-place same-size edit within the mtime granularity would keep a stale hash. Acceptable for books; worth a comment if the catalog ever guards writable state.
- The static scratch makes CatalogLoad/LibraryScan non-reentrant; safe today under the owner-task-only rule, but AssertOwner-style protection doesn't cover this file.

### What should be done in the future
- Fault-injection tests for the catalog (truncated/corrupt/interrupted write) belong in task 5zpj alongside positions/bookmarks.
- If libraries grow past 32 books the record array and kMaxBooks scale together; the file is 5 KiB per 32 books.

### Code review instructions
- Start at `0112-papers3-reader-primitives/main/app_storage.cpp`: `CatalogLoad`, `CatalogSave`, `CatalogLookup`, and the rebuild block at the end of `LibraryScan`; note `s_catalog_scratch` and its comment.
- Validate on hardware: flash, then `python3 scripts/52-papers3-console-client.py --cmd sd` — first boot shows `hashed=N catalog_writes=1`, any later boot shows `cached=N hashed=0`.

### Technical details
- On-disk format: `{u32 magic 'S3CT', u32 version=1, u32 count, CatalogRecord[32]{char path[96], char title[40], u64 size, i64 mtime, u32 hash}, u32 crc}`, FNV-1a CRC over all preceding bytes, written via tmp → fsync → bak swap → rename.

## Step 14: Phase 9 — retained widgets, pages, regions, and the reader migration

This step generalized the proven vertical slice into the widget architecture the future JavaScript API will bind: a bounded retained tree, measured layout, flat draw-op compilation, dependency-driven diffing, routable pages, and overlay regions — all pure and host-tested first, then wired under the real reader without behavior regression. The flagship hardware demo is a live clock region updating every 2 s through diff-computed damage: 2 draw ops, a 460×34 partial refresh, zero full flashes.

### Prompt Context

**User prompt (verbatim):** (see Step 13 — "go ahead. do that one first, then do phase 9 and 10")

**Assistant interpretation:** After the catalog, implement Phase 9 per the design doc: widget trees, layout, draw-op output, regions, dependency invalidation, routable pages, fixtures, and the reader migration.

**Inferred user intent:** Establish the stable native binding surface (design §13.3) so the MicroQuickJS layer later manipulates widget handles instead of drawing.

**Commits (code):** c15fa32 — "Widgets: retained tree, layout, render compile, diff, pages, regions"; 04b9d92 — "Reader: run library and reading screens on the generic widget tree"; a352f3f — "Tests: pin golden widget-page draw-op trace"

### What I did
- New s3paper_core modules: `widget.h` (typed nodes text/row/col/spacer/divider/progress/list/book/region in a 128-slot arena with generation-safe handles and builder helpers), `widget_layout.h` (flexbox-lite: padding/gap/fixed/flex/alignment; List paginates, never scrolls), `widget_render.h` (compile to FrameBuilder ops + immutable HitRegions + RegionSpecs, ancestor-frame clipping), `widget_diff.h` (DependencyTracker + RenderStateDiff), `page.h` (PageRouter with header/content/footer/overlay slots, bounded stack), `region.h` (RegionTable).
- 277 new host checks (4199 total), including a pinned golden draw-op trace of a widget page.
- Firmware: `main/app_ui.{h,cpp}` owns the PSRAM arena, router, diff snapshot, region table, and one present pipeline (LayoutPage → CompileTree → extra ops → planner). `widget hello|status` console fixtures; the status fixture's clock Region updates via owner-loop ticks: SetText → re-layout → diff → clipped re-render → TextRegion present. Quiet-while-active regions defer while touch input is fresher than 2 s (`InputLastInputUs`).
- Migrated the reader (P9.10): the reading page is a retained tree (title/star/footer updated via SetText across page turns; body composited into the reserved Book node with unchanged LayoutKey margins), the library builds rows as widgets whose hit regions come from compilation (SD hit ids are 1-based since 0 = not tappable).

### Why
- §6.1 of the design doc: JS closures can't live in native nodes. Everything dynamic is now a DependencyId, a hit id, or a SetText from app code — exactly the callback-agnostic boundary the JS runtime needs.

### What worked
- Boot restore, page turns (full=0), bookmark toggle re-render, library (3 hit regions), and open-from-library all behaved identically after the migration — validated over the console against the flashed device.
- The region demo: `region update: damage 460x34 at (40,180) full=0 ops=2` every 2 s, indefinitely, planner never forced a full.

### What didn't work
- First host-test run segfaulted under ASan (`widget_layout.cpp:144 member access within null pointer`): destroying a widget still linked in its parent's child list left a dangling index. Fixed twice over: added `RemoveChild` (unlink before destroy) and made all three tree walks stop deterministically at a dead link.
- One wrong test expectation: a child with `fixed_w` set does not stretch under `CrossAlign::Stretch` (fixed wins) — the test, not the code, was corrected.

### What I learned
- Compiling clips from ancestor *frames* (not a clip stack) lets a flat entry array carry the whole overflow rule; FrameBuilder's stack is only pushed one level per op emission.
- The retained tree pays off immediately: page turns now touch three SetText calls instead of rebuilding chrome geometry.

### What was tricky to build
- Per-entry effective clips: each layout entry stores its parent's entry index, so clip[i] = Intersect(clip[parent], frame[parent]) computes incrementally in paint order; roots use their own frame. Multi-slot pages (LayoutPage) must offset parent indices when appending slot layouts into the shared array — off-by-one there scrambles clipping silently, which is why the router test asserts every parent index precedes its child.
- Region partial updates re-render the whole page under a PushClip of the diff damage: ops outside are dropped by the builder, so frame.damage collapses to the region and the planner refreshes only it. No per-widget framebuffers needed.

### What warrants a second pair of eyes
- `RouteTo` heuristics (Back-if-it-lands-right, else Push) keep the stack bounded for two pages but would surprise with deeper navigation graphs; revisit when a third page arrives.
- The fixture's region tick is single-region by design; generalizing to N regions needs per-region deadlines through the shared Scheduler (task vzbo's full ambition) — the RegionTable API already supports it.
- WidgetArena::kCapacity=128 caps the library at ~40 rows (3 nodes/row); the List paginates entries, but row *creation* is what consumes slots.

### What should be done in the future
- Hardware screenshots for the fixture corpus (n7h2 stays open).
- Migrate `s_state.regions` consumers to read hit regions straight from the compile result if a future screen needs >33 regions.
- Operator validation of quiet-region deferral (needs live touch during a fixture interval).

### Code review instructions
- Core: `components/s3paper_core/include/s3paper/{widget,widget_layout,widget_render,widget_diff,page,region}.h` and matching src; tests at the end of `tests/host/test_main.cpp` (`TestWidget*`, `TestPageRouter`, `TestRegionTable`, `TestWidgetGoldenTrace`).
- Firmware: `main/app_ui.cpp` (`UiPresentPage`, `UiRegionTick`), `main/app_reader.cpp` (`BuildReadingTree`, `LibraryShow`, `RouteTo`).
- Validate: `make run` in tests/host (4199 checks); on device `widget status` then watch `ui: region update` lines; `library show` + `reader open` for the migrated screens.

### Technical details
- Node mutation contract: setters bump `content_version`; `RenderStateDiff` compares (generation, kind, version, frame) per arena slot between Capture and Diff — damage falls out of retained state, never from dirty flags in app code.

## Step 15: Phase 10 — coordinated power lifecycle, verified wake sources, resume

This step made battery behavior a lifecycle instead of an ad-hoc powerOff(): a documented quiesce sequence, a retained sleep image on the panel, verified wake sources for the actual M5PaperS3 revision, and hardware-proven resume. The strongest evidence pair: two deep-sleep cycles waking by timer (`reset_reason=8, wakeup_cause=4`) and one true power-off re-powered by the BM8563 RTC alarm (`reset_reason=1` POWERON) — each time the reader came back at offset 2572 with both bookmarks intact.

### Prompt Context

**User prompt (verbatim):** (see Step 13 — "go ahead. do that one first, then do phase 9 and 10")

**Assistant interpretation:** Implement Phase 10 (power/resume) after Phase 9, keeping the diary and commit cadence.

**Inferred user intent:** A real e-reader must survive being put down for a week: sleep deep, keep the page on the panel, wake fast, and never lose the reading position.

**Commit (code):** 7e31f8f — "Power: coordinated sleep/wake lifecycle for PaperS3"

### What I did
- `components/s3paper_m5/{include/s3paper_m5/m5_power.h,src/m5_power.cpp}`: the only module calling M5.Power/M5.Rtc — battery read (ADC GPIO3 ratio 2.0, CHG_STAT GPIO4), `PowerDeepSleep` (timer wake), `PowerRtcOff` (BM8563 alarm + power-latch pulse), `PowerOff`.
- `main/app_power.{h,cpp}`: `PowerSleep(mode, s)` runs the documented quiesce order (touch tick off → StorageFlushNow → retained widget sleep image (title, percent read, wake hint) presented CleanFull → SD unmount → console flush → wake source + transition); `PowerAutoTick` implements inactivity auto-sleep (default off, any touch cancels) and a 30 s rate-limited low-battery shutdown (level ≤ 5 % and not charging → same quiesce path); `PowerLogBootCause` stamps every boot with reset/wakeup cause.
- Console `sleep [status|deep N|timer N|off|auto N]`; `PowerSnapshot` in the reply union; owner replies BEFORE sleeping (the reply must cross USB before power drops).
- Hardware validation transcripts in `scripts/output/0112-power-*.log`.

### Why
- Design Phase 10 gate: repeated sleep/wake without losing state or leaving an in-flight EPD operation. Wake on this hardware is a reboot, so `ReaderBootRestore()` (Phase 8) is the resume contract — no separate warm-resume path to maintain.

### What worked
- Cycle 1: `sleep deep 20` → USB re-enumerated ~20 s later → `wakeup_cause=4 reset_reason=8`, reader restored. Cycle 2 (15 s): identical.
- `sleep timer 25` (true power-off): board re-powered by the RTC alarm, `reset_reason=1` — proving the BM8563 alarm + power-latch path works on this revision.
- Battery telemetry: 4150–4180 mV, 100 %, charging=0.
- Auto-sleep arm/disarm round-trips through the console.

### What didn't work
- Nothing failed outright this step; note the first `sleep status` immediately after a wake raced the ~35 s boot-restore and returned nothing (same mid-boot race documented in Step 12 — query again after boot settles).

### What I learned
- Verified wake-source matrix for M5PaperS3 (m5unified 0.2.18): touch INT is GPIO48, NOT an RTC IO on ESP32-S3 → deep sleep cannot wake on touch; timer only. True power-off wakes via BM8563 RTC alarm or the physical side button. This is task go0n's answer and is encoded as a comment in m5_power.h.
- M5's own `_timerSleep`/`deepSleep` call `M5.Display.sleep(); M5.Display.waitDisplay();` first, which is what guarantees "no in-flight EPD operation" — the quiesce sequence only needs to get the sleep image *issued* before that.

### What was tricky to build
- Reply-before-sleep ordering in the owner: a sleep command that quiesces first would eat the console reply (host sees a timeout and can't distinguish "sleeping" from "hung"). The owner sends the Ok reply, then runs `PowerSleep`, and only an error path falls through with a log.
- PaperS3 power-off is not a GPIO level: the latch needs a pulse loop (Power_Class does 5 low/high cycles); that's why `PowerRtcOff`/`PowerOff` park in `vTaskDelay(portMAX_DELAY)` — power drops mid-loop.

### What warrants a second pair of eyes
- The low-battery threshold (≤ 5 %, not charging) has never fired on real hardware — the battery reads 100 %. A drained-battery or mocked-ADC test belongs in 1k7u before trusting it in the field.
- `sleep off` and physical-button wake were not exercised (needs a hand on the device); the code path is identical to `timer` minus the alarm.
- Auto-sleep powering off while USB-attached will drop the development console mid-session by design; consider a "USB-powered → skip auto-sleep" qualifier if that annoys.

### What should be done in the future
- 1k7u remainder: shutdown-during-write injection, missing-card-on-wake, low-battery simulation.
- Operator: visually confirm the retained sleep image (title + "asleep" + wake hint should stay on the panel while off).

### Code review instructions
- Start at `main/app_power.cpp` (`PowerSleep` sequence, `PowerAutoTick`), then `components/s3paper_m5/src/m5_power.cpp` against `managed_components/m5stack__m5unified/src/utility/Power_Class.cpp` (PaperS3 cases at lines ~244, ~1079, ~1255).
- Validate: `sleep status` (battery + boot cause), `sleep deep 20` then reconnect after ~60 s and check `reset_reason=8 wakeup_cause=4` plus the reader snapshot; `sleep timer 25` for the RTC path (`reset_reason=1`).

### Technical details
- Boot-cause decoding: reset_reason 1=POWERON, 8=DEEPSLEEP, 11=USB; wakeup_cause 4=TIMER (esp_sleep.h enums, surfaced raw in PowerSnapshot).

## Step 16: Phase 11 — MicroQuickJS feasibility spike, 38/38 on hardware

The bounded spike the design doc demanded before any JS work: a separate firmware project (`0113-papers3-mquickjs-spike`) that answers every integration question on the real PaperS3 without touching the reader. The suite autoruns at boot and loops; the final capture is 38/38 PASS, and the proceed/postpone decision (design-doc/05) is **proceed** — bounded execution, cancellation, and C-API rooting discipline are all demonstrated, which is exactly what the stop rule required.

### Prompt Context

**User prompt (verbatim):** "1 2 3 work. continue with phase 11 now" — followed by "we have a esp-idf components service to run mqjs, did you find it?"

**Assistant interpretation:** Operator confirmed the Phase 10 hardware checks; proceed with the MicroQuickJS spike, using the repo's existing mqjs infrastructure rather than importing anything new.

**Inferred user intent:** Get the JS go/no-go answered cheaply with what the repo already has (the vendored engine and `mqjs_service`), keeping the reader firmware untouched.

**Commit (code):** edba960 — "Spike: MicroQuickJS feasibility suite on PaperS3 (Phase 11)"

### What I did
- Pinned provenance (P11.1): in-repo vendored engine (`imports/esp32-mqjs-repl/.../components/mquickjs`, MIT) copied into the spike project; reference upstream `bellard/mquickjs@84d793e0` recorded (local clone `~/code/others/mquickjs`; header identical, engine .c predates upstream's StringBuffer rework by ~709 diff lines).
- Custom stdlib via the upstream generator flow: `tools/spike_stdlib.c` + patched `mqjs_stdlib_spike.c` (CONFIG_SPIKE block) compiled on host by `tools/gen_spike_stdlib.sh` → `main/spike_stdlib.h` (32-bit table) + project-local `mquickjs_atom.h`. Key insight: `mquickjs_build.h` stringifies all function references, so the generator needs no function symbols at all.
- Device runtime (`main/spike_stdlib_runtime.c`): print/gc/millis + the `S3Widget` class — packed (generation<<16|index)+1 opaque handles, stale-handle TypeErrors, finalizer accounting — mirroring the s3paper_core generation-handle scheme.
- 7-probe suite in `main/app_main.cpp` using the shared `MqjsVm` (from `components/mqjs_service`): arena startup (8 KB–4 MB), 18-case syntax matrix, OOM+recovery, GC rooting under churn, widget lifecycle, 100 ms deadline cancellation, on-device trusted-bytecode round trip.
- Results + decision in `design-doc/05-microquickjs-spike-results-and-proceed-decision.md`.

### Why
- Design §Phase 11 stop rule: postpone scripting if bounded execution/cancellation or C API safety can't be demonstrated in a short ticket. Both demonstrated → Phase 12 is unblocked.

### What worked
- All 38 probes: ~600 µs context creation at every arena size; OOM at 20 ms with the same context recovering; rooted object surviving 20 compaction cycles while observably moving; `"stale widget handle"` on post-teardown access with finalizers balancing; runaway `for(;;);` stopped at exactly 100 ms and the context reusable; 244-byte bytecode image compiled, relocated, and executed across two contexts entirely on-device.
- Syntax matrix matches the ES5-stricter contract, with one pleasant surprise: **for-of is supported** (my initial expectation said no — the probe corrected me).

### What didn't work
- First capture lost the suite's early output: USB Serial/JTAG drops printf while no host reads, and a one-shot suite raced the console attach. Fixed by looping the suite every 30 s.
- PSRAM silently absent on the first flashes: `set(COMPONENTS main)` trimmed `esp_psram` out of the build, so `CONFIG_SPIRAM` wasn't a known Kconfig symbol and the defaults were dropped with only a reconfigure-time warning ("unknown kconfig symbol 'SPIRAM'"). Fix: `set(COMPONENTS main esp_psram)` + rm sdkconfig.
- `TRUE`/`FALSE` from the mquickjs API aren't defined under the ESP-IDF C++ toolchain — used `1`/`0`.
- The console client crashes on a whitespace-only `--cmd " "` (`cmd.split()[0]` IndexError) — worked around with a real dummy command; worth a one-line guard someday.
- An external-source permission gate blocked cloning/building `bellard/mquickjs` directly; resolved better by using the repo's own vendored engine (which the user pointed to), keeping the spike fully in-repo.

### What I learned
- Bytecode is stdlib-coupled: images embed atom indices, so bytecode must be produced by a context with the SAME stdlib. Compiling on-device (JS_NewContext2 prepare mode) sidesteps the whole host-toolchain/atom-mismatch problem for trusted images.
- The generator warning "Too many properties, consider increasing ATOM_ALIGN" is advisory (global-object hash clamps; lookups stay correct, slightly slower).

### What was tricky to build
- The stdlib def/runtime pairing contract: the generated header references C functions by symbol name and must be included at the END of the translation unit that defines them; the atom header it also emits is included by the ENGINE — so a shared engine component cannot serve two different stdlibs. That's why the spike has a project-local engine copy and the README warns about it.

### What warrants a second pair of eyes
- The widget finalizer counts a stale-handle finalization as "finalized" even though the native slot was already torn down — correct for the spike's accounting, but the Phase 12 binding should decide explicitly who owns slot release when teardown races collection.
- `MqjsVm::InterruptHandler` polls `esp_timer_get_time()` on every VM interrupt check; fine at 100 ms deadlines, but worth measuring overhead before using very tight budgets.

### What should be done in the future
- Phase 12 (pj4p): bind the real s3paper widget/page ABI using the spike's rooting + handle patterns; author the fluent facade in the proven ES5 subset.
- Unify the JS-side generation-handle scheme with s3paper_core's WidgetHandle generations.
- Reflash 0112 onto the PaperS3 when spike evidence gathering is done (the device currently runs the spike loop).

### Code review instructions
- Start at `0113-papers3-mquickjs-spike/README.md` (provenance + regeneration), then `main/app_main.cpp` (probes) and `main/spike_stdlib_runtime.c` (binding patterns, especially the JSGCRef use in `js_s3widget_constructor`).
- Validate: `idf.py build flash` in 0113, then capture ~55 s of console output and grep `SPIKE|` — expect `suite|PASS|... pass=38 fail=0` every ~30 s. Evidence: `scripts/output/0113-spike-final.log`.

### Technical details
- Decision record: design-doc/05. Arena guidance for Phase 12: 128 KB internal is comfortable (the whole suite runs there); PSRAM supports 4 MB if scripts ever need it; startup cost is negligible (~1.5 ms create+first-eval).

## Step 17: Phase 12 core — the fluent s3paper facade runs the panel

MicroQuickJS now lives inside the reader firmware and drives real screens through the exact architecture the design prescribed: a flat, versioned native ABI over the Phase 9 widget arena, an ES5 fluent facade on top, callback IDs instead of closures, and gesture dispatch into JS only while a JS page owns the panel. The embedded hello and taps apps render through the same planner/backend as the reader, and the reader coexists — it rebuilds its retained tree when a JS app has reset the shared arena.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Proceed to Phase 12 per the ticket order.

**Inferred user intent:** Reach the product goal: the JS API surface running over the proven native primitives.

**Commit (code):** 9e030af — "JS: fluent s3paper facade over the widget ABI (Phase 12 core)"

### What I did
- Vendored the engine into `0112/components/mquickjs` (byte-identical to the spike/imports copy; atom header regenerated) and added the s3 stdlib generation flow (`tools/js/{s3_stdlib.c,mqjs_stdlib_s3.c,gen_s3_stdlib.sh}` → `main/js_stdlib.h`).
- ABI v1 (task qoou), 15 functions: widget constructors, `s3AddChild`, `s3SetText/-Progress`, one versioned `s3Config(handle, prop, a..d)` property channel (unknown prop → TypeError), and `s3Present(h,c,f,o,full)`. Handles are the s3paper `WidgetHandle` generation+index packed into an int32 — the SAME generation scheme end to end, so a stale JS handle throws `TypeError: stale widget handle` after any arena reset (muo1 validation on every call).
- `main/app_js.{h,cpp}`: owner-task VM host — 160 KB PSRAM arena (internal fallback), every eval under an esp_timer deadline, exceptions recorded in a console-visible snapshot; the embedded ES5 facade (`s3.text('…').pad(…).onTap(fn)…`) plus hello and taps acceptance apps.
- Callback model (0yat/pqfs): `.onTap(fn)` stores the closure in the JS-side `s3._taps` registry keyed by a numeric hit id; the widget node carries only that id. Gestures hit-test natively over the JS page's compiled hit regions and call `s3Dispatch(kind,x,y,hit)` by name — native never holds a JSValue across calls, so no long-lived GC roots exist.
- Screen ownership: `UiPresentCount()` in app_ui; JS is "active" only while no other present happened since its own. `app_input` routes gestures to `JsHandleGesture` first.
- Console `js [status|hello|taps]`; snapshot shows init/active/evals/exceptions/dispatches/last_error.

### Why
- Design §13.3/§13.4: bind the stable native model, never M5GFX; JS owns callbacks, native owns rendering. This step is that boundary made concrete.

### What worked
- `js hello`: 10 draw ops, clean full, zero exceptions. `js taps`: 8 ops, **hits=1** (the counter row's tap region emitted from widget compilation).
- Coexistence: `reader open` after a JS app → JS `screen_active` drops to 0 automatically, the reader rebuilds its tree (full=1) and partial page turns resume (full=0).

### What didn't work
- 160 KB contiguous internal RAM isn't available inside the full reader firmware — first `js hello` returned OutOfMemory. Moved the arena to PSRAM (the Phase 11 spike had measured identical eval latency there); internal is now only a fallback.
- `mquickjs.h` unqualified needs `<stddef.h>` first and C linkage in C++ TUs — two quick build fixes (`unknown type name 'size_t'`, then mangled `_Z10JS_ToInt32…` link errors).

### What I learned
- The generated stdlib table references binding functions by bare identifier with no prototypes, so a dedicated C TU (`js_stdlib_table.c`) including a shared prototypes header (`app_js_bindings.h`) pairs cleanly with C++ implementations defined `extern "C"` inside `namespace reader` (C linkage ignores namespaces — the bindings can touch the host's anonymous-namespace state).

### What was tricky to build
- Panel-ownership without coupling: rather than JS/reader knowing about each other, `UiPresentCount()` gives every transient screen the same rule — "you own gestures until someone else presents." The fixture clock uses its own flag today and should migrate to this counter.

### What warrants a second pair of eyes
- `s3Present` pushes the "js" router page with plain `Push` (no Back heuristics); alternating reader/JS navigation could hit the bounded stack's CapacityExceeded (logged, non-fatal, present unaffected). Fine for acceptance apps; revisit if JS pages become routable peers.
- The dispatch path evals a snprintf'd `s3Dispatch(...)` string — robust and GC-safe, but each tap costs a parse. If tap latency ever matters, switch to JS_GetPropertyStr + JS_Call with the push-args protocol.
- JS apps and the reader share one WidgetArena; `s3.reset()` invalidates reader handles by design (reader self-heals). If a JS app someday must persist alongside the reader, the arena needs partitioning.

### What should be done in the future
- Remaining Phase 12 subtasks: library/reader acceptance ports (wipy), native-vs-JS trace equivalence (17nn), the host authoring/bytecode pipeline (ibe5), fault-injection for scripts (rs5w) — the spike covered engine-level OOM/cancel; app-level fallback-to-native-screen is untested.
- Operator: run `js taps` and tap the counter line — expect the number to increment with a partial refresh (the counter row is the only hit region).

### Code review instructions
- Start at `main/app_js.cpp`: `EvalBounded`, `ArgHandle`, `js_s3_config`, `js_s3_present`, and the embedded `kFacadeJs`; then `main/js_stdlib_table.c` + `main/app_js_bindings.h` for the pairing contract; `tools/js/gen_s3_stdlib.sh` regenerates.
- Validate: `js hello`, `js taps` (check `hits=1`), tap the counter, then `reader open` + `reader next` (reader self-heals). Evidence: `scripts/output/0112-js-apps.log`, `0112-js-reader-coexist.log`.
