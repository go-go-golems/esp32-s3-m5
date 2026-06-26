# 0103 — AtomS3R M12 Native QuickJS with PSRAM

Ticket: `ATOMS3R-M12-NATIVE-QUICKJS`

This firmware ports the validated native QuickJS service from the ESP32-P4 work to an AtomS3R M12 / ESP32-S3R8-class board with PSRAM.

## Serial target

There is also an ESP32-P4 board connected. Do not use `/dev/ttyACM0` for this firmware.

Use the AtomS3R M12 Espressif USB Serial/JTAG by-id path:

```bash
/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
```

At ticket creation time this resolved to:

```text
/dev/ttyACM1
```

The ESP32-P4 CH343 path was:

```text
/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 -> /dev/ttyACM0
```

## Firmware scope

Milestone 1 is intentionally console-first and has been hardware validated on AtomS3R M12:

- ESP32-S3 target.
- USB Serial/JTAG console.
- 8 MB flash baseline.
- 8 MB Octal PSRAM expected, configured at conservative 80 MHz.
- Native QuickJS through existing reusable components:
  - `components/quickjs_native`
  - `components/qjs_service`
- 1 MiB QuickJS memory limit for the first S3R port.
- 64 KiB QuickJS stack limit.
- 32 KiB-class owner task stack.
- A read-only JavaScript `system` namespace with board, target, firmware, flash, PSRAM, and QuickJS limit metadata.

Later milestones add bounded storage, STA WiFi, a host-owned HTTP server, dynamic QuickJS HTTP routes, bounded HTTP-only `fetch()`, and explicit stored-script execution with `js run <virtual-path>`. There is still no boot-time script autoload; USB Serial/JTAG remains the recovery path.

## Build

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py set-target esp32s3
idf.py build
```

## Flash and monitor

Use the by-id path:

```bash
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
idf.py -p "$PORT" flash monitor
```

## Console commands

```text
js status
js eval <source>
js run <virtual-path>
js reset
js gc
js bench
storage status
storage mount [format]
storage list /scripts
storage read /scripts/demo.js
storage write /scripts/demo.js "print('hi')"
wifi status
wifi connect
http status
http start 80
http static /static /data
```

The firmware installs `system`, `storage`, `wifi`, and `http` namespaces after boot and after `js reset`.

`system` is a read-only metadata namespace:

```text
js eval "system.board"
js eval "JSON.stringify(system)"
js eval "Object.isExtensible(system)"
```

Expected fields:

```js
system.firmware                  // "0103-atoms3r-m12-native-quickjs"
system.board                     // "AtomS3R M12"
system.target                    // "esp32s3"
system.ticket                    // "ATOMS3R-M12-NATIVE-QUICKJS"
system.psramInitialized          // true
system.psramBytes                // 8388608 on the validated board
system.flashBytes                // 8388608 on the validated board
system.quickjsMemoryLimitBytes   // 1048576
system.quickjsStackLimitBytes    // 65536
```

Smoke sequence:

```text
js status
js eval "print(1+2)"
js eval "throw new Error('boom')"
js eval "let x=41; x+1"
js reset
js eval "typeof x"
js bench
```

## Expected memory posture

The ESP32-P4 native service showed an idle QuickJS memory usage of about 50 KiB. On AtomS3R M12 the first target is more conservative:

- QuickJS memory limit: 1 MiB.
- Keep WiFi/TLS/storage disabled for the first proof.
- Watch `js status` for:
  - `quickjs: used=... malloc=... atoms=...`
  - `esp_heap: internal=... 8bit=... psram=...`

The first memory stress pass completed a 20k-number array and cleanly reported `InternalError: out of memory` for an oversized allocation. Keep the 1 MiB cap until WiFi/TLS/storage are integrated and measured. Add explicit device APIs one namespace at a time.

## Storage namespace

`storage` is virtual-rooted and bounded. It exposes the 3 MiB FatFs `storage` partition through `/scripts`, `/data`, and `/tmp`; it does not expose native absolute paths.

```text
js eval "storage.status().mounted"
js eval "storage.writeText('/scripts/demo.js', 'print(123)')"
js eval "storage.readText('/scripts/demo.js')"
js eval "JSON.stringify(storage.list('/scripts'))"
js eval "JSON.stringify(storage.stat('/scripts/demo.js'))"
```

Limits:

- `readText`: 16 KiB per call.
- `writeText`: 16 KiB per call.
- `list`: 64 entries.
- paths must stay under `/scripts`, `/data`, or `/tmp`.

Startup mounts the partition without formatting. On a blank development device, use `storage mount format` explicitly once.

## HTTP, fetch, and stored scripts

The native HTTP server is host-owned and recoverable from the console. JavaScript route callbacks are owned by QuickJS and are cleared by `js reset`.

```text
http start 80
storage write /scripts/server.js http.get('/run/hello',function(req){return{json:{ok:true,path:req.path}};})
js run /scripts/server.js
curl http://192.168.4.22/run/hello
```

Expected response:

```json
{"ok":true,"path":"/run/hello"}
```

Dynamic handlers should return one of the supported response shapes, or a Promise that resolves to one of those shapes:

```js
return { status: 200, json: { ok: true } };
return { status: 200, text: 'hello', contentType: 'text/plain; charset=utf-8' };
return Promise.resolve({ status: 200, json: { ok: true } });
```

Promise-returning route handlers are drained inside the dynamic dispatch job. Rejected route Promises return `500 Internal Server Error`; Promises that do not settle during the bounded dispatch drain return `504 Gateway Timeout`.

`fetch()` is a bounded firmware API in this milestone:

- `http://` only.
- `GET` and `POST`.
- bounded request and response bodies.
- Promise callbacks are drained after console eval.

Example:

```text
js eval "fetch('http://192.168.4.22/healthz').then(r => { print('status='+r.status+' ok='+r.ok); return r.text(); }).then(t => print('body='+t))"
```

Checked-in source examples live under `examples/scripts/`. They are not embedded automatically. Copy them into `/scripts/...` and run them explicitly with `js run`.

## Recovery policy

Do not add desktop QuickJS `std`/`os` compatibility as a shortcut. Keep firmware APIs explicit and reset-safe.

Do not enable script autoload until there is a documented disable/recovery mechanism. A bad dynamic route script should be recoverable with USB Serial/JTAG plus `js reset`, `http stop`, or replacing the stored `/scripts/...` file.
