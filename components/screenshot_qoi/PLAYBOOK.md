# screenshot_qoi Component Playbook

## Purpose

`components/screenshot_qoi` provides a reusable ESP-IDF component that captures the current M5GFX framebuffer and streams it over USB Serial/JTAG as a QOI image. It replaces the older PNG-in-firmware approach to reduce code size, heap pressure, and CPU cost on the device.

This component is meant for debug, UI validation, and ticket evidence capture workflows.

## What Is Included

- Firmware component:
  - `include/screenshot_qoi.h`
  - `screenshot_qoi.cpp`
- Host helper script:
  - `tools/capture_screenshot_qoi_from_console.py`

## Integration in a Firmware

1. Add component directory to `EXTRA_COMPONENT_DIRS` in the app `CMakeLists.txt`.
2. Add `screenshot_qoi` to `PRIV_REQUIRES` in `main/CMakeLists.txt`.
3. Include header:

```cpp
#include "screenshot_qoi.h"
```

4. Trigger capture from your console command handler:

```cpp
size_t qoi_len = 0;
bool ok = screenshot_qoi_to_usb_serial_jtag_ex(display, &qoi_len);
```

## Public API

From `include/screenshot_qoi.h`:

- `void screenshot_qoi_to_usb_serial_jtag(m5gfx::M5GFX& display);`
- `bool screenshot_qoi_to_usb_serial_jtag_ex(m5gfx::M5GFX& display, size_t* out_len);`

Behavior:

- Runs capture/encode in a dedicated task (stack allocated for encoder path).
- Streams output to USB Serial/JTAG.
- Returns `true` on successful framed write, `false` on allocation/task/stream failure.

## Wire Protocol (Device -> Host)

Each screenshot is framed as:

1. ASCII header line:

```text
QOI_BEGIN <length>\n
```

2. Exactly `<length>` raw QOI bytes.
3. ASCII trailer line:

```text
QOI_END\n
```

Host tools must parse framing and read exact byte count before waiting for trailer.

## Why QOI Instead of PNG on Device

- No DEFLATE/`miniz` dependency in firmware path.
- Lower runtime overhead on ESP32-S3 than PNG compression.
- Predictable, small implementation.
- Adequate compression for UI screenshots.

Tradeoff:

- QOI is not a browser-native format; host side may convert to BMP/PPM/PNG after capture.

## Memory/Size Expectations

For a 240x135 frame:

- RGB565 source pixels: `240 * 135 * 2 = 64,800` bytes.
- RGB888 raw (if expanded): `240 * 135 * 3 = 97,200` bytes.

QOI output size depends on UI entropy. For UI-like images it is often far below raw RGB888, but worst case can approach raw size plus header/op overhead.

## Host-Side Requirements

Required:

- Python 3.x
- `pyserial`

Install:

```bash
python3 -m pip install pyserial
```

Capture example:

```bash
python3 components/screenshot_qoi/tools/capture_screenshot_qoi_from_console.py \
  /dev/serial/by-id/<device> \
  /tmp/ui.qoi \
  --bmp-out /tmp/ui.bmp
```

Notes:

- Script sends `screenshot` by default (`--cmd` can override).
- It writes raw QOI file and can optionally decode to `.ppm` and/or `.bmp`.
- BMP/PPM conversion is host-side, not firmware-side.

## USB Console Requirements

Use USB Serial/JTAG console for screenshot capture and REPL commands. Recommended in firmware defaults:

```ini
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

This avoids clashes with UART pins used by peripherals/UART links.

## Troubleshooting

- Timeout waiting for `QOI_BEGIN`:
  - Verify firmware is running and command is registered.
  - Ensure correct serial device path.
  - Ensure no other monitor process is holding the same port.
- Truncated payload:
  - Check cable/USB stability.
  - Increase `--timeout-s`.
- No `QOI_END`:
  - Treat capture as failed; retry.
- Garbled decode:
  - Confirm host read exactly `<length>` bytes from header.

## Validation Checklist

1. Firmware boots and console command list includes `screenshot`.
2. Run capture script and confirm `.qoi` file is created.
3. Optional: generate `.bmp` and verify UI visually.
4. Repeat capture 2-3 times to confirm framing stability.

## Migration Notes

This component centralizes screenshot capture logic and host tooling so app folders do not carry duplicated screenshot scripts.
