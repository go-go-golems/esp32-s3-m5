---
Title: 'Implementation guide: Screenshot to serial (B3)'
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - ui
    - keyboard
    - esp32s3
    - esp-idf
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/utility/lgfx_miniz.c
      Note: PNG encoder allocation uses MZ_MALLOC/MZ_FREE semantics
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp
      Note: createPng implementation and readRectRGB usage
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp
      Note: createPng API signature
    - Path: 0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Serial backend patterns (USB-Serial-JTAG and UART)
ExternalSources: []
Summary: Intern guide for implementing PNG screenshots over serial using createPng() with a robust framing protocol and host capture script.
LastUpdated: 2026-01-01T20:09:23.279552337-05:00
WhatFor: ""
WhenToUse: ""
---



# Implementation guide: Screenshot to serial (B3)

This guide explains how to implement the “Screenshot to Serial” demo. This is a real-world developer tool: it lets you capture the current UI as a PNG file without needing a camera, and it makes bug reports and documentation much easier.

The core primitive is `createPng(...)` in LovyanGFX (vendored inside M5GFX).

## Outcome (acceptance criteria)

- Pressing a key (e.g., `P`) triggers a screenshot.
- The device prints a PNG blob over **USB-Serial-JTAG** with a clear framing protocol.
- A host script can capture the bytes and write a valid `.png` file.
- The demo is robust against normal log output (no interleaving corrupting the PNG).

## Where to look (source of truth)

- PNG creation API:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp` (`createPng`)
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp` (implementation)
- PNG encoder allocation behavior (uses miniz malloc/free):
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/utility/lgfx_miniz.c`

## Key APIs

From `LGFXBase.hpp`:

```cpp
void* createPng(size_t* datalen,
                int32_t x = 0,
                int32_t y = 0,
                int32_t width = 0,
                int32_t height = 0);
```

Notes:

- `createPng` returns a pointer to a heap buffer containing a full PNG file.
- You must free it after use. The encoder uses `MZ_MALLOC`, so `free(ptr)` is the expected release mechanism.

## Safety considerations (don’t corrupt the PNG)

Serial logs and screenshot bytes must not interleave.

Recommended approach:

- During screenshot transfer, temporarily disable non-essential logging to the same serial port.
- Use a framing protocol with:
  - an ASCII header line that includes the byte length
  - then raw bytes
  - then an ASCII footer line

Example framing:

```text
PNG_BEGIN <len>\n
<len bytes of raw PNG data>
\nPNG_END\n
```

If you cannot guarantee “no extra bytes”, switch to a robust encoding (base64), but that increases size and time. Start with raw bytes + strict logging discipline.

## Implementation plan (device side)

### 1) Choose the trigger key

In the demo firmware’s common key handling:

- `P` => take screenshot

### 2) Capture and transmit

Pseudocode:

```cpp
void send_png_over_serial(m5gfx::M5GFX& display) {
  size_t len = 0;

  // Ensure any pending DMA has completed so readback is consistent.
  display.waitDMA();

  void* png = display.createPng(&len, 0, 0, display.width(), display.height());
  if (!png || len == 0) {
    printf("PNG_BEGIN 0\nPNG_END\n");
    return;
  }

  printf("PNG_BEGIN %u\n", (unsigned)len);

  // Raw write. Prefer the exact serial backend used by your firmware:
  // - USB-Serial-JTAG: usb_serial_jtag_write_bytes(...)
  // - UART: uart_write_bytes(...)
  //
  // If you only have printf, you will corrupt binary output. Do not do that.
  serial_write_bytes((const uint8_t*)png, len);

  printf("\nPNG_END\n");
  free(png);
}
```

Important: do not emit binary data via `printf` or line-oriented logging. Use a byte-oriented write API.

### 3) Pick a serial write primitive

If the demo firmware already uses USB-Serial-JTAG, use `driver/usb_serial_jtag.h`:

```cpp
#include "driver/usb_serial_jtag.h"
usb_serial_jtag_write_bytes(png, len, portMAX_DELAY);
```

If using UART, use `driver/uart.h`:

```cpp
uart_write_bytes(UART_NUM_X, (const char*)png, len);
```

You can wrap both behind:

```cpp
void serial_write_bytes(const uint8_t* data, size_t len);
```

## Implementation plan (host side)

Create a small host script that:

1. Reads serial until it sees `PNG_BEGIN <len>`
2. Reads exactly `<len>` bytes
3. Reads until `PNG_END`
4. Writes the bytes to `screenshot.png`

Python sketch:

```python
import re, serial

ser = serial.Serial("/dev/ttyACM0", 115200, timeout=1)

header_re = re.compile(rb"PNG_BEGIN (\\d+)\\n")

while True:
    line = ser.readline()
    m = header_re.match(line)
    if not m:
        continue
    length = int(m.group(1))
    data = ser.read(length)
    # optional: read trailing newline + footer line(s)
    # consume until PNG_END
    while b"PNG_END" not in ser.readline():
        pass
    open("screenshot.png", "wb").write(data)
    print("wrote screenshot.png", len(data))
    break
```

## Demo UX (make it obvious)

On-device:

- show a one-line help hint: `P: screenshot to serial`
- after trigger:
  - briefly show “Screenshot sent” toast (but do not print extra logs during transfer)

## Validation checklist

- Host script writes a PNG that opens correctly on your computer.
- Repeated screenshots work (no memory leak).
- Large PNG transfer doesn’t time out or wedge the serial driver.

## Common pitfalls

- Using `printf` for binary output: it will corrupt the PNG.
- Logging during transfer: even one extra byte breaks parsing.
- Not waiting for DMA: might cause readback artifacts on some panels.
