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

- [x] **T1.1 — Start a tmux monitor session.** Used a fresh tmux server socket with the AtomS3R by-id path and kept `/dev/ttyACM0`/ESP32-P4 untouched.
- [x] **T1.2 — Flash with `idf.py -p <AtomS3R-by-id> flash monitor`.** Flashed successfully through `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00`.
- [x] **T1.3 — Confirm boot identity.** Logs show `ESP32-S3-PICO-1`, embedded flash 8MB, embedded PSRAM 8MB, `0103`, PSRAM initialized, and QuickJS service ready.
- [x] **T1.4 — Run console smoke.** `js status`, `print(1+2)`, exception, completion value, `js reset`, and `js bench` all passed.
- [x] **T1.5 — Capture logs.** Saved monitor output to `/tmp/0103-atoms3r-m12-native-quickjs-smoke.log`.
- [x] **T1.6 — Update diary/changelog/tasks and commit hardware validation.**

## Phase 2 — Memory characterization on AtomS3R M12

- [x] **T2.1 — Record boot heap and PSRAM size.** Captured `before_qjs` and `after_qjs` logs: PSRAM initialized, size 8,388,608 bytes.
- [x] **T2.2 — Record `js status` before/after eval.** Captured baseline `js status`: QuickJS used 49,760 bytes, atom count 518, internal free 184,715 bytes, 8-bit free 8,570,439 bytes, PSRAM free 8,385,724 bytes.
- [x] **T2.3 — Stress the 1 MiB QuickJS cap.** A 20k-number array completed successfully; an oversized string-array allocation failed cleanly as `InternalError: out of memory`; runtime remained usable and reset restored the baseline.
- [x] **T2.4 — Decide whether 2 MiB is safe.** Keep the default at 1 MiB for now. Do not raise to 2 MiB until WiFi/TLS/storage memory pressure is measured.

## Phase 3 — Extension readiness

- [x] **T3.1 — Split binding installers if needed.** Added a 0103-local `system_namespace` installer that mutates QuickJS through `qjs_service_run()` instead of crowding `qjs_service.cpp`.
- [x] **T3.2 — Add a read-only `system` namespace.** Exposes firmware, board, target, ticket, PSRAM, flash, and QuickJS limit metadata; smoke confirmed non-extensible/read-only behavior and reset persistence.
- [x] **T3.3 — Design WiFi and storage namespaces.** Added concrete staged contracts: virtual-rooted bounded FatFs `storage`, native ESP32-S3 request/status `wifi`, no password exposure, and no blocking WiFi scans/connects on the QuickJS owner task.
- [x] **T3.4 — Add script storage only after runtime memory is characterized.** Added bounded virtual-rooted FatFs storage with console commands and QuickJS `storage.status/list/stat/readText/writeText`; hardware smoke passed after explicit dev-format and survived board reset.

## Phase 4 — Optional AtomS3R display integration

- [ ] **T4.1 — Decide whether this firmware remains console-only or adds AtomS3R GC9107 display output.**
- [ ] **T4.2 — If display is added, reuse prior AtomS3R display/backlight evidence.** Do not mix display work into the first QuickJS smoke.
- [ ] **T4.3 — Keep UART/USB console as recovery path.**
