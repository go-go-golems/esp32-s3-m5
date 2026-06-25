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

No LCD, WiFi, filesystem, or storage APIs are part of the first smoke. Those should be added only after QuickJS memory and reset behavior are validated on the S3R hardware. The first memory stress pass supports keeping the QuickJS cap at 1 MiB until WiFi/TLS/storage pressure is measured.

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
js reset
js gc
js bench
storage status
storage mount [format]
storage list /scripts
storage read /scripts/demo.js
storage write /scripts/demo.js "print('hi')"
```

The firmware installs `system` and `storage` namespaces after boot and after `js reset`.

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

## Future namespace plan

The ticket design guide defines the remaining namespace contract:

- `wifi`: native ESP32-S3 WiFi status/request API, starting with firmware-owned state and polling; do not block the QuickJS owner task on scans/connects and never expose passwords.

Do not add desktop QuickJS `std`/`os` compatibility as a shortcut. Keep firmware APIs explicit and reset-safe.
