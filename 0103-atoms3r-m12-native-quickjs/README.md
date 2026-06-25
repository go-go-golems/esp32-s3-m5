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

Milestone 1 is intentionally console-only:

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

No LCD, WiFi, filesystem, or storage APIs are part of the first smoke. Those should be added only after QuickJS memory and reset behavior are validated on the S3R hardware.

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

After the console smoke passes, the next steps are to evaluate whether a 2 MiB QuickJS cap is safe and then add explicit device APIs one namespace at a time.
