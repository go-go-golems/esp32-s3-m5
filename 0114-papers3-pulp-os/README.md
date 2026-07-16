# 0114-papers3-pulp-os — PULP OS v2

JS-first firmware for the M5Stack PaperS3 (ESP32-S3, 540×960 e-ink), built
on the shared s3paper components (`../components/s3paper_{core,m5,storage,
runtime}`) extracted from the hardware-proven `0112-papers3-reader-
primitives`. Ticket: `ESP-51-PULP-OS-V2` (see the intern guide in the
ticket's `design-doc/` for the full architecture).

## Build & flash

```bash
unset IDF_PYTHON_ENV_PATH && source ~/esp/esp-idf-5.3.4/export.sh   # 5.3.4 PINNED
idf.py build
idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00 flash
```

- m5unified `==0.2.18` + m5gfx `==0.2.25` pinned via `s3paper_m5`;
  `dependencies.lock` is committed.
- `sdkconfig.defaults` only seeds ABSENT values — `rm sdkconfig` to
  re-seed after changing it.

## Serial discipline (read twice)

- **NEVER** `idf.py monitor`, `screen`, `minicom`, or raw pyserial: modem
  control (DTR/RTS) resets the device into ROM download mode.
- Use the console client:
  `python3 ttmp/2026/07/14/ESP-50-*/scripts/52-papers3-console-client.py
  --settle 5 --cmd "status" --output out.log`
- One owner per port; stop any capture before `idf.py flash`.
- USB output is dropped while no host reads: design evidence output to
  repeat (screens print `pulp screen: <name>` on every present).

## Console

`status`, `heap`, `events`, `display`, `ping`, `touch [on|off|status]`,
`sd [mount|unmount|seed|status|reload|fault <kind> <mode>]`,
`sleep [status|deep N|rtc-off N|off|auto N]`, `home`.

## Architecture (short form)

One UI owner task (core 1, prio 5) owns ALL application and display state;
producers post bounded POD events. JS (MicroQuickJS, from Phase 5) runs
only on the owner under a deadline. Rendering goes retained widget tree ->
layout -> diff -> clipped ops -> refresh planner -> EPD; zero visible
change = zero panel work. The engine copy lives in `components/mquickjs`
(local to this firmware — atoms are stdlib-specific).
