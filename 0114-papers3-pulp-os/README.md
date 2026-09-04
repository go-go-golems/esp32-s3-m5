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
- On some hosts (observed: kernel 6.8 cdc_acm) even *opening* the port
  asserts DTR/RTS and can wedge the chip in the ROM downloader — the
  console goes totally silent while esptool still works. Diagnostic:
  `esptool --before no_reset flash_id` syncing instantly = download mode;
  the fix is a power-on reset (side button / power cycle). Use the
  hold-open client, which opens once and keeps the fd for all commands:
  `python3 ttmp/2026/08/17/ESP-55-*/scripts/04-papers3-console-hold.py
  --no-reset --cmd "status" --output out.log`
  (legacy: `ttmp/2026/07/14/ESP-50-*/scripts/52-papers3-console-client.py`).
- One owner per port; stop any capture before `idf.py flash`.
- USB output is dropped while no host reads: design evidence output to
  repeat (screens print `pulp screen: <name>` on every present).

## Console

`status`, `heap`, `events`, `display`, `ping`, `touch [on|off|status]`,
`sd [mount|unmount|seed|status|reload|fault <kind> <mode>]`,
`sleep [status|deep N|rtc-off N|off|auto N]`, `home`,
`js [status|probe N|pulp|tap X Y|swipe K|hits|measure]`, `serve`, `net`,
`http`, `buzz`, `bat`, `images`.

## Apps (ESP-55)

Apps are single-file JS descriptors `({id, title, subtitle, version,
abi, main: function (os, arg) {...}})` loaded at launch by the native
`load()` — never part of the bytecode image (which holds only the OS core,
`tools/js/os/*.js`). Sources, in override order:

- `/sdcard/apps/<id>.js` + one-line `<id>.json` manifest (SD overrides
  ROM, except `settings`); seeded from ROM on first boot.
- `rom:<id>` flash assets: `tools/js/apps/*.js`, embedded via
  `main/CMakeLists.txt` EMBED_TXTFILES + `main/js_assets.cpp`.

Adding a built-in app: write `tools/js/apps/<id>.js`, add the
EMBED_TXTFILES line + `js_assets.cpp` row + `ROM_APPS` entry in
`tools/js/os/20-catalog.js`, rebuild. Adding an app WITHOUT reflashing:

```bash
# push (developer loop; PUT or POST):
ttmp/2026/08/17/ESP-55-*/scripts/06-pulp-app-push.sh my.js --host pulp.local --run
# or pull from the device: Settings -> Apps -> Install from URL
# inspect: curl http://pulp.local/apps/list   (run: /apps/run?id=<id>)
```

Caps: module ≤ 32 KiB, id `[a-z0-9_-]{1,24}`, manifests are one JSON
line. The web upload/run routes live in `osRoutes()`
(`tools/js/os/00-kernel.js`) and `main/net_serve.cpp` (`/apps/upload`).

## Architecture (short form)

One UI owner task (core 1, prio 5) owns ALL application and display state;
producers post bounded POD events. JS (MicroQuickJS, from Phase 5) runs
only on the owner under a deadline. Rendering goes retained widget tree ->
layout -> diff -> clipped ops -> refresh planner -> EPD; zero visible
change = zero panel work. The engine copy lives in `components/mquickjs`
(local to this firmware — atoms are stdlib-specific).
