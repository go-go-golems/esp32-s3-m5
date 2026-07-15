# 0112-papers3-reader-primitives

Native reader-primitives firmware for the M5Stack PaperS3 (ESP32-S3), Phase 1
of ticket `ESP-50-PAPERS3-EREADER-PRIMITIVES`. This is the substrate for the
future e-reader and `s3paper` JavaScript API described in the ticket's design
doc (`ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--*/design-doc/01-*.md`)
and implementation handoff (`reference/02-*.md`).

## Phase 1 scope

- One UI/application **owner task** (`ui_owner`, core 1) that is the only
  place application state is read or written.
- A bounded POD `AppEvent` queue (capacity 32) plus bounded reply queues.
  Producers never block: a full queue is an explicit `CapacityExceeded`.
- A USB Serial/JTAG `esp_console` REPL whose commands only post events and
  wait for replies (500 ms timeout, explicit error on expiry).
- Diagnostics: heap, event queue depth/high-water, per-source accept/reject
  counters, ordering violations, reply drops, display backend state.
- A stress fixture proving deterministic per-source ordering under three
  concurrent producers.

## Phase 2 scope (rendering primitives)

- `components/s3paper_core/` — pure, host-testable component (no ESP/M5
  headers): defensive half-open geometry, `Status`/`Result`, EPD damage
  alignment (provisional `align_x=8` pending Phase 0 measurements), POD
  `DrawOp`s, fixed-capacity frame arena, clip-stack `FrameBuilder`, and a
  deterministic trace-recording `FakeBackend`. Host tests:
  `cd components/s3paper_core/tests/host && make run` (237 checks,
  ASan/UBSan).
- `components/s3paper_m5/` — the only module allowed to call `M5.Display`.
  Transaction shell: bounded busy-wait, `setEpdMode`, `startWrite`/batched
  ops/`endWrite`, bounded flush wait, per-present metrics logging. Naive
  intent→mode mapping until the Phase 3 refresh planner.
- `fixture [fake|m5]` console command renders the same deterministic
  primitive scene (border, corner markers, width ladder 1..16, 16-step gray
  ladder, checkerboard, clip demo, lines, glyph run) through either backend.

## Phase 3 scope (refresh planner)

- `s3paper_core/include/s3paper/refresh_planner.h` — the single owner of
  refresh policy. Collects damage (aligned, distance-merged, explicit
  capacity fallback), maps `PresentIntent` to backend-neutral `EpdWaveform`,
  and forces clean fulls for first render, wake, screen change, explicit
  request, and turn/area/elapsed budgets. Host-tested with synthetic
  histories.
- Owner presents go through `PresentPlanned()`; a planner-forced full
  becomes a `CleanFull` present. Plan regions currently inform policy and
  metrics only — ops carry their own clip rects until retained widgets
  (Phase 9) do region-limited redraws.
- Console: `refresh` (policy + history inspection), `soak start [n]` /
  `soak status` — a mixed partial/full soak on the M5 backend driven by
  self-posted owner events (console stays responsive), with per-waveform
  timing, heap high-water, and `heap_caps_check_integrity_all` every 256
  steps.

Touch, SD, power, and JavaScript work still belong to later phases.

## Decision records

- **Toolchain pin: ESP-IDF 5.3.4** (`~/esp/esp-idf-5.3.4`). Matches the repo
  standard for PaperS3 application firmware (`0080`, `.envrc` convention).
  The EPD hardware-qualification experiments (`0110`, `0111`) deliberately
  use 5.4.2; that lineage is for the paused display investigation, not for
  application firmware.
- **Display backend: none in Phase 1.** The panel is not optically qualified
  (see ticket diary steps 20–26). The shared `M5PaperS3-UserDemo/components`
  M5GFX checkout currently carries local debug patches, so Phase 2 must pin
  clean upstream M5GFX/M5Unified revisions as its own decision record before
  adding the M5 transaction shell. Early rendering development targets the
  fake backend.

## Architecture

```text
console REPL task ─┐ (AppEvent: ConsoleCommand + reply queue)
stress producer A ─┼──> bounded AppEvent queue ──> ui_owner task
stress producer B ─┘ (AppEvent: Pointer, ConsoleCommand)   │
                                                           └─ owns AppState,
                                                              replies via
                                                              bounded queues
```

- `main/app_events.h` — POD event/command/reply contracts and the stable
  `StatusCode` vocabulary (`Ok`, `InvalidArgument`, `CapacityExceeded`,
  `Busy`, `Timeout`, `CorruptData`, `OutOfMemory`, `Unimplemented`).
- `main/app_owner.cpp` — owner task, event queue, per-source ordering
  validation, reply routing. `AssertOwner()` aborts if app state is ever
  touched from another task.
- `main/app_console.cpp` — REPL registration; every command is a producer.
- `main/app_main.cpp` — boot diagnostics and startup.

## Build

```bash
unset IDF_PYTHON_ENV_PATH
source ~/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3   # once
idf.py build
```

## Flash

Serial rules (see repo `AGENTS.md`): single owner per port, by-id path, no
`idf.py monitor`, no pyserial modem-control opens against the PaperS3.

```bash
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00
cd build && python -m esptool --chip esp32s3 -p "$PORT" -b 460800 \
  --before default_reset --after hard_reset write_flash @flash_args
```

## Validate (Phase 1 exit gate)

Interact over the raw USB Serial/JTAG device without modem-control ioctls,
e.g. with the ticket console client
(`ttmp/.../scripts/52-papers3-console-client.py`):

- `status`, `heap`, `display`, `events`, `ping` — round trips through the
  owner task.
- `stress [n]` — runs two concurrent producer tasks (console-shaped and
  pointer-shaped events, n each, default 500) while the console keeps
  pinging; expects `received == sent` per source, `out_of_order=0`,
  `result=PASS`.
- `flood [n]` — bursts more events than the queue holds; expects a nonzero
  explicit `rejected` count (CapacityExceeded), visible afterwards in
  `events` as `rejected_sends`.
- `shutdown` — owner acknowledges and transitions to `shutting-down`;
  `ping` then reports `Busy` (reboot to leave the state).

The exit gate is deterministic owner ordering under concurrent producers and
explicit queue-full/reply-timeout behavior — not a screen.
