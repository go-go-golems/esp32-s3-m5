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
    - Path: repo://0112-papers3-reader-primitives/main/app_reader.cpp
      Note: Phase 8 reading controller with gesture page turns (commit 3ec8c0c)
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
