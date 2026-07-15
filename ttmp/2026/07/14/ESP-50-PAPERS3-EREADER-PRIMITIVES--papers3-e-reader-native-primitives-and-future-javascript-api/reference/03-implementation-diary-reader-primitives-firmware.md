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
