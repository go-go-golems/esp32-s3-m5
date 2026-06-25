# Tasks

This task list is implementation-grade. Work top to bottom. Keep the ESP32-P4 and AtomS3R serial ports separate by using `/dev/serial/by-id/...` paths.

## Phase 0 — Ticket, design, and initial firmware scaffold

- [x] **T0.1 — Identify the AtomS3R M12 serial interface by USB identity.** Confirm the AtomS3R M12 is the Espressif USB Serial/JTAG device, not the ESP32-P4 CH343 bridge.
- [x] **T0.2 — Create docmgr ticket.** Create `ATOMS3R-M12-NATIVE-QUICKJS` with topics `atoms3r`, `esp32s3`, `quickjs`, `javascript`, `firmware`, `psram`, `repl`.
- [x] **T0.3 — Create intern-facing design guide.** Explain the hardware baseline, PSRAM assumptions, QuickJS internals, service reuse, firmware layout, command API, validation plan, and extension path.
- [x] **T0.4 — Create investigation diary.** Record serial identification, design, firmware scaffold, and first build result.
- [x] **T0.5 — Create `0103-atoms3r-m12-native-quickjs`.** Add firmware skeleton that reuses `components/quickjs_native` and `components/qjs_service`.
- [x] **T0.6 — Build the scaffold.** Build for `esp32s3`; binary currently builds at `0xb4d00`, with 82% free in the 4 MiB app partition.
- [x] **T0.7 — Relate files, update changelog, run doctor, upload bundle to reMarkable, and commit.**

## Phase 1 — Hardware flash and console smoke

- [ ] **T1.1 — Start a tmux monitor session.** Use the AtomS3R by-id path and keep `/dev/ttyACM0`/ESP32-P4 untouched.
- [ ] **T1.2 — Flash with `idf.py -p <AtomS3R-by-id> flash monitor`.** Use tmux so logs can be captured without flooding the chat.
- [ ] **T1.3 — Confirm boot identity.** Verify logs show `0103`, ESP32-S3, USB Serial/JTAG, PSRAM initialized, and QuickJS service ready.
- [ ] **T1.4 — Run console smoke.** Test `js status`, `js eval "print(1+2)"`, exception, completion value, `js reset`, and `js bench`.
- [ ] **T1.5 — Capture logs.** Save tmux pane output to `/tmp/0103-atoms3r-m12-native-quickjs-smoke.log`.
- [ ] **T1.6 — Update diary/changelog/tasks and commit hardware validation.**

## Phase 2 — Memory characterization on AtomS3R M12

- [ ] **T2.1 — Record boot heap and PSRAM size.** Capture `before_qjs` and `after_qjs` logs.
- [ ] **T2.2 — Record `js status` before/after eval.** Capture QuickJS memory use, atom count, internal heap, 8-bit heap, and PSRAM free.
- [ ] **T2.3 — Stress the 1 MiB QuickJS cap.** Run bounded array/string/object scripts and confirm clean failures or recoverability.
- [ ] **T2.4 — Decide whether 2 MiB is safe.** Only raise the cap if WiFi/storage headroom remains credible.

## Phase 3 — Extension readiness

- [ ] **T3.1 — Split binding installers if needed.** Prepare `qjs_service` for namespace-based APIs without crowding `qjs_service.cpp`.
- [ ] **T3.2 — Add a read-only `system` namespace.** Expose version/heap/status first.
- [ ] **T3.3 — Design WiFi and storage namespaces.** Keep operations bounded or asynchronous; avoid blocking the QuickJS owner task.
- [ ] **T3.4 — Add script storage only after runtime memory is characterized.** Use size-limited reads and virtual roots.

## Phase 4 — Optional AtomS3R display integration

- [ ] **T4.1 — Decide whether this firmware remains console-only or adds AtomS3R GC9107 display output.**
- [ ] **T4.2 — If display is added, reuse prior AtomS3R display/backlight evidence.** Do not mix display work into the first QuickJS smoke.
- [ ] **T4.3 — Keep UART/USB console as recovery path.**
