---
Title: ESP-IDF CoreS3 + Module13.2 QRCode scanner — analysis, design, and implementation guide
Ticket: ESP-62-CORES3-QRCODE
Status: active
Topics:
    - m5stack
    - cores3
    - module13.2-qrcode
    - barcode
    - qr-code
    - esp32-s3
    - esp-idf
    - m5gfx
    - m5unified
    - intern-guide
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0095-cores3-wifi-bench/sdkconfig.defaults
      Note: CoreS3 baseline sdkconfig (quad PSRAM, USB Serial/JTAG)
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os
      Note: ESP-IDF + M5Unified/M5GFX template (IDF 5.3.4, esp32s3)
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/components/s3paper_m5/src/m5_backend.cpp
      Note: M5GFX displayBusy/canvas pattern to mirror in the UI
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/scripts/01-probe-qrcode-uart.py
      Note: Host-side protocol probe for bring-up validation
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/sources/arduino-lib/src/M5ModuleQRCode.cpp
      Note: IO-expander (PI4IOE5V6408) + UART init reference
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/sources/arduino-lib/src/qrcode_m14.cpp
      Note: Reference protocol implementation to port to ESP-IDF
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/sources/docs/01-Module13.2-QRCode-product-page.txt
      Note: M5-Bus pinmap, DC power, CoreS3 bus mapping, symbologies
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/sources/protocol-pdf/Module13.2-QRCode-Protocol-EN.txt
      Note: Primary UART protocol spec — every command byte and framing rule
    - Path: repo://ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/scripts/02-bringup-build-flash.sh
      Note: Build/flash helper respecting AGENTS.md (IDF 5.3.4, USB Serial/JTAG)
    - Path: repo://ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/sources/MANIFEST.md
      Note: Provenance of every gathered source
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---



# ESP-IDF CoreS3 + Module13.2 QRCode — Barcode/QR Scanner with On-Screen Display
### Analysis, Design & Implementation Guide (intern-ready)

**Ticket:** ESP-62-CORES3-QRCODE
**Status:** proposed (analysis + design; implementation pending)
**Target:** M5Stack CoreS3 (ESP32-S3) + Module13.2 QRCode (SKU M145)
**Framework:** ESP-IDF 5.3.4 with M5Unified + M5GFX managed components (Arduino-on-ESP-IDF hybrid)
**Last updated:** 2026-08-23

---

## 1. Executive summary

This firmware turns an M5Stack **CoreS3** (an ESP32-S3 core with a 320×240
ILI9341 touch LCD) stacked with a **Module13.2 QRCode** (SKU **M145**, a
1D/2D barcode-scanning expansion) into a self-contained barcode/QR reader that
**shows each decoded code on the CoreS3 screen**. The user holds the device,
aims it at a code, and the decoded text appears on the display.

Two hardware facts from the user drive the design:

1. **"The device is connected over USB."** — The CoreS3 is the host we flash
   and talk to. Its console/flash path is the **USB Serial/JTAG** peripheral
   (the repo standard for ESP32-S3, see `AGENTS.md`). The CoreS3 is *not*
   decoding images itself; it is a *controller + display* for the scanner
   module.
2. **"I plugged in 12 V into the qr code reader."** — The Module13.2 QRCode
   has a **DC 9–24 V barrel jack (5.5×2.1 mm, center-positive)** that powers
   the scanner engine's illumination/aiming LEDs and, through an internal
   DC-DC step-down to 5 V, powers the **whole stack** (module + CoreS3) over
   the M5-Bus. 12 V is squarely inside the rated range. **This is required**:
   the CoreS3's USB 5 V alone is not enough to run the scanner engine LEDs at
   full brightness, and without the external DC the engine will not decode
   reliably.

The CoreS3 talks to the scanner module over **UART (115200 8N1)** using a
compact binary protocol, and controls module power + hardware trigger through
an **I2C GPIO expander (PI4IOE5V6408 @ address 0x43)** that sits on the
module. Both of these reach the CoreS3 through the **M5-Bus** stacking
connector and a small **DIP switch** on the module that routes the scanner's
UART lines to the CoreS3's Port-C UART pins.

The implementation strategy is deliberately low-risk: we reuse the repo's
proven **M5Unified + M5GFX on ESP-IDF** pattern (exactly as
`0114-papers3-pulp-os` does) and port the official **M5Module-QRCode** Arduino
library's protocol layer into a small ESP-IDF component. We do *not* write a
new camera/decoder from scratch — the Module13.2 QRCode already contains a
complete CMOS decode engine (the "M14-Pro"); our firmware only *drives* it and
*renders* results.

---

## 2. Problem statement and scope

### 2.1 Goal

Build a single ESP-IDF firmware project (e.g. `0118-cores3-qrcode-scanner`)
that:

1. Initializes the CoreS3 display (M5GFX, ILI9341, 320×240).
2. Initializes the Module13.2 QRCode: powers it on via the I2C expander and
   opens a UART to the M14-Pro engine.
3. Configures the scanner (trigger mode, lights, buzzer) over UART.
4. Continuously reads decoded codes from UART and renders them on screen —
   most-recent code large, plus a scroll-back history of recent codes.
5. Exposes a small **USB Serial/JTAG console** for commands (start/stop,
   mode, status, factory reset) and debug logging, consistent with the rest
   of this repo.

### 2.2 In scope

- CoreS3 + Module13.2 QRCode hardware combination.
- UART control + scan-result streaming from the M14-Pro engine.
- I2C GPIO expander (PI4IOE5V6408) for power enable and TRIG.
- On-screen rendering with M5GFX (text + simple layout).
- USB Serial/JTAG console for operator commands.

### 2.3 Out of scope (explicitly)

- **On-host image decoding.** We do *not* pull raw frames (`0x60` image
  command) and we do *not* run quirc/zxing on the CoreS3. The engine decodes;
  we consume text. (Image pull is documented as an optional future path.)
- **USB-HID / USB-HID-POS modes** of the module. Those modes make the module
  emulate a keyboard/POS scanner to a *PC* over its own USB-C port; they
  disable UART and are irrelevant when the CoreS3 is the host. We keep the
  module in **RS232 (UART) mode**.
- Wi-Fi / networking. Headless standalone reader.
- Barcode *generation*.

---

## 3. The hardware: what each part is and how they connect

An intern with no M5Stack background should read this section first. There
are three physical things on the desk:

### 3.1 CoreS3 (the "core" / host)

- An **ESP32-S3** module with:
  - a **2.0" 320×240 ILI9341 SPI LCD** with capacitive touch (FT6206),
  - an **AXP2102** PMIC (power/battery/charging),
  - a 6-axis IMU, a microphone, a speaker (I2S via ES8311 codec),
  - **16 MB flash + 8 MB PSRAM** (octal),
  - a **USB Serial/JTAG** interface (one USB-C cable carries both console
    and the JTAG/flash path — no separate UART chip needed).
- Its internal I2C bus (**"In_I2C"**, GPIO12 SDA / GPIO11 SCL) hosts the AXP,
  touch, and — when a Module13.2 is stacked — the module's IO expander.
- It exposes three Grove ports: **PORT.A** (I2C, G2/G1), **PORT.B**
  (analog/digital, G8/G10 area), **PORT.C** (UART, **G17=TX / G18=RX**).

### 3.2 Module13.2 QRCode (SKU M145, the "scanner expansion")

- A stacking module that screws **under** the CoreS3; they share the
  **M5-Bus** 30-pin connector.
- Contains:
  - a **CMOS barcode decode engine** ("M14-Pro", 640×480) that does all the
    imaging + decoding internally,
  - a **PI4IOE5V6408 I2C GPIO expander** at **address 0x43** (on In_I2C)
    that the CoreS3 uses to (a) switch the engine's 5 V power
    (`QR_5V_EN`, expander channel **0**) and (b) drive the hardware
    `TRIG` line (expander channel **4**),
  - a **DC 9–24 V barrel jack** (5.5×2.1 mm, **center-positive**) feeding an
    internal DC-DC that produces the 5 V for the engine and back-feeds 5 V
    to the M5-Bus (so the whole stack runs off the 12 V wall supply),
  - illumination (white) + aiming (red) LEDs, a buzzer,
  - a side **DIP switch** that selects **USB vs UART** interface mode and
    routes the engine UART to specific M5-Bus pins,
  - three Grove ports (A/B/C) passthrough.
- Supported symbologies: 2D — **QR, Micro QR, Data Matrix, PDF417** (+ Aztec,
  Micro PDF417, Grid Matrix, Chinese Xin); 1D — Code39/93/128, EAN-8/13,
  UPC-A/E, Codabar, Interleaved/Matrix/Industrial 2-of-5, MSI, Code11, GS1
  Databar, etc.

### 3.3 Power (the 12 V question — answered)

- The engine's LEDs and decode ASIC want more headroom than USB 5 V gives.
  M5Stack's own spec table ("Operating Current (with Core2)") lists DC 5 V /
  9 V / **12 V** operating points; 12 V is explicitly supported.
- **With 12 V plugged into the module's DC jack, the internal DC-DC steps it
  down to 5 V and powers the CoreS3 too** (via M5-Bus pin 28 `5V`). So the
  CoreS3 can run with *no USB power* — USB is then only needed for
  flashing/console.
- Polarity is **center-positive, outer-negative**. A standard 12 V
  center-positive 5.5×2.1 mm adapter is correct. Reversing polarity risks
  damage; double-check the plug before applying power.
- **Operating current** (from spec): at 12 V the stack draws roughly
  ~80 mA idle / ~125 mA scanning (the table's "on" figure). A modest 12 V
  ≥0.5 A adapter is plenty.

> **Decision: external 12 V is required for reliable decoding.** The
> firmware must assume the module is DC-powered. We still drive
> `QR_5V_EN` so the engine can be cleanly enabled/disabled and so the
> stack behaves on USB-only bench testing (engine present but dim/slow),
> but the operator must connect 12 V for real scans.

### 3.4 How the two talk: the M5-Bus + DIP switch

The M5-Bus is a 30-pin bus. The relevant CoreS3-side pins and what the
module puts on them (from the product PinMap):

```
 M5-Bus pin | Module13.2 QRCode side          | CoreS3 side
 -----------+---------------------------------+----------------------
  15        | QR_TX  -> PORT.C_UART_RX (SW)    | G18 / PC_RX  (UART RX)
  16        | QR_RX  <- PORT.C_UART_TX (SW)    | G17 / PC_TX  (UART TX)
  17        | I2C_SDA                          | G12 / In_SDA
  18        | I2C_SCL                          | G11 / In_SCL
  28        | 5V (from DC-DC when 12V present) | 5V
```

`(SW)` means the DIP switch must be set to route that pin; set to `NC` it
disconnects. **For CoreS3 + UART control, set the DIP switch so the engine
UART lands on Port-C UART (pins 15/16) → CoreS3 G18(RX)/G17(TX).** The
Arduino tutorial's "DIP Switch for Pin Selection" figure shows the exact
position; the ContinuousMode example for Basic v2.7 uses G17 TX / G16 RX,
but on **CoreS3** the Port-C UART is **G17 TX / G18 RX** — adjust pins in
`Config_t` accordingly (see §6).

> File evidence: `sources/docs/01-Module13.2-QRCode-product-page.txt`
> (M5-Bus table + CoreS3 bus-mapping section) and
> `sources/arduino-lib/src/M5ModuleQRCode.cpp` (defaults
> `i2c = &M5.In_I2C`, channels 0 and 4).

### 3.5 Block diagram

```
        +----------------------- 12 V DC (5.5x2.1, center+) -----------------------+
        |                                                                        |
        v                                                                        |
 +------------------+   DC-DC 5V   +-----------------------------------+            |
 | Module13.2 QRCode|<------------|  M14-Pro engine (CMOS decoder)    |            |
 |  - M14-Pro engine|             |  - 640x480 imaging + decode        |            |
 |  - PI4IOE5V6408  |<---I2C 0x43|  - white illumination + red aim   |            |
 |    (IO expander) |             |  - buzzer                         |            |
 |  - DIP switch    |             +-----------------------------------+            |
 |  - DC jack       |                       |  UART 115200 8N1                    |
 +------------------+                       QR_TX ------------------------+           |
        | M5-Bus                            QR_RX <-----------------------+          |
        |  (30-pin stack)                              (via DIP switch -> PC UART)   |
        v                                                                        |
 +-----------------------------------------+                                    |
 | CoreS3 (ESP32-S3)                       | <---- 5V (back-fed) ----+            |
 |  G18 = UART1 RX  <--- QR_TX             |                                    |
 |  G17 = UART1 TX  ---> QR_RX             |                                    |
 |  G12/G11 = In_I2C ---> PI4IOE5V6408@0x43|  (QR_5V_EN=ch0, TRIG=ch4)          |
 |  ILI9341 320x240 SPI LCD (M5GFX)        |                                    |
 |  AXP2102 PMIC, touch, IMU, codec        |                                    |
 |  USB Serial/JTAG <-- USB-C --> host PC  |  (flash + console)                 |
 +-----------------------------------------+                                    |
        |                                                                        |
        +------------------------------------------------------------------------+
```

---

## 4. The communication protocol (what the intern must implement)

This is the heart of the firmware. Everything is in
`sources/protocol-pdf/Module13.2-QRCode-Protocol-EN.txt`; the official Arduino
implementation is `sources/arduino-lib/src/qrcode_m14.cpp`.

### 4.1 UART electrical + framing

- **115200 baud, 8 data bits, 1 stop bit, no parity, no flow control** (the
  engine's serial is fixed 8N1; see the ZBarcode user guide
  `sources/protocol-pdf/ZBarcode-Scanner-User-Guide-2.5-EN.txt`, "Baud Rate"
  and "Data bit … 8, 1, None in fixed"). Default baud is 115200.
- Logic levels are **TTL 3.3 V** (the module's UART is level-shifted to the
  CoreS3; both are 3.3 V devices, so direct M5-Bus connection is safe).

### 4.2 Command packet format

Every host→module command is:

```
 byte 0   byte 1   byte 2          byte 3..
 TYPE     PID      FID             PARAM (0..N bytes)
```

- **TYPE** = command class:
  - `0x21` Configuration Write (reply `0x22`)
  - `0x23` Configuration Read  (reply `0x24`)
  - `0x32` Control            (reply `0x33`)
  - `0x43` Status Read        (reply `0x44`)
  - `0x60` Image Read         (reply `0x61`)
- **PID** = property/attribute group (e.g. `0x61` = reading parameters,
  `0x62` = fill light, `0x63` = buzzer, `0x75` = decode control,
  `0x02` = product info).
- **FID** = sub-function. Its **top 2 bits encode the PARAM byte count**:
  - bits[7:6]=`00` → 0 bytes
  - bits[7:6]=`01` → 1 byte
  - bits[7:6]=`10` → 2 bytes
  - bits[7:6]=`11` → >2 bytes (then PARAM's first 2 bytes are the remaining
    length).
- **PARAM** = the value bytes (MSB-first for multi-byte).

### 4.3 Replies

- Config Write reply: `22 <PID> <FID> <PAR> <RID>` where `RID` `0x00`=
  success, `0x01`=illegal PID/FID. `PAR` echoes the written value (or `00`
  for multi-byte writes).
- Control reply: `33 <PID> <FID> <PAR> <RID>`.
- Status reply: `44 <PID> <FID> <len_hi> <len_lo> <data...>` — **the data
  length is the 2 bytes at offset [3:4], big-endian.** The official lib
  parses this in `getResponseDataSize()` (qrcode_m14.cpp).
- **Some control commands have no reply** (e.g. Start Decode `32 75 01`
  returns nothing; the lib's `sendCmd` treats "no ACK expected" as success).

### 4.4 The commands we actually need

| Function | Host bytes | Expected reply | Notes |
|----------|-----------|----------------|-------|
| Start decoding | `32 75 01` | (none) | software trigger |
| Stop decoding | `32 75 02` | `33 75 02 00 00` | |
| Set trigger mode | `21 61 41 <mode>` | `22 61 41 <mode> 00` | mode: 0 Key, 1 Continuous, 2 Auto, 4 Pulse, 5 Motion |
| Set fill-light mode | `21 62 41 <mode>` | `22 62 41 <mode> 00` | 0 off, 2 on-decode, 3 on |
| Set fill-light brightness | `21 62 48 <0..100>` | `22 62 48 <v> 00` | percent |
| Set position-light mode | `21 62 42 <mode>` | `22 62 42 <mode> 00` | 0 off, 1 flash-decode, 2 on-decode |
| Good-read beep on/off | `21 63 46 <0/1>` | `22 63 46 <v>` | |
| Read firmware version | `43 02 C1` | `44 02 C1 <len> <data>` | status |
| Read serial number | `43 02 C5` | `44 02 C5 <len> <data>` | status |
| Factory reset | `32 76 01` | (control reply) | use with care |
| Set comm iface to UART/RS232 | `21 42 40 00` | (no ack expected in lib) | restores UART from USB modes |

> Full tables (every symbology enable/disable, all light/buzzer/keyboard
> options) are in `sources/protocol-pdf/Module13.2-QRCode-Protocol-EN.txt`
> §3.1–§3.3. We only need a handful for the MVP.

### 4.5 How scan results arrive (the subtle part)

When the engine decodes a code it **streams the decoded content out of the
UART as raw bytes**, optionally followed by a configurable **suffix**
(default `\r\n`). The official library treats scan output as plain bytes —
see `M5ModuleQRCode::update()` / `QRCodeM14::waitScanResult()` in
`sources/arduino-lib/src/M5ModuleQRCode.cpp` and `qrcode_m14.cpp`: it simply
reads whatever bytes are available on the serial and stuffs them into a
`std::string`.

So our ESP-IDF parser is a **line-oriented accumulator**:

- Accumulate incoming bytes into a buffer.
- A complete scan result = buffer up to the suffix (`\r\n` by default) **or**
  a quiet-time gap (e.g. 50 ms with no new bytes) **or** a max length cap.
- On a complete result, trim the suffix and emit the string to the UI.

> **Validate on hardware:** the exact terminator depends on the module's
> configured suffix/protocol-format (config `51 4C` suffix enable, `51 C2`
> suffix content, `51 43` protocol format). The MVP assumes the default
> `\r\n` suffix and a quiet-time fallback. If results look glued together
> or split, probe with `scripts/01-probe-qrcode-uart.py --scan` to see raw
> bytes, then adjust the parser or set the suffix explicitly via
> `21 51 C2 00 02 0D 0A`.

### 4.6 Pseudocode: the protocol engine (ESP-IDF port)

```c
// qrcode_m14.c — minimal port of the Arduino qrcode_m14.cpp

typedef enum { QR_OK=0, QR_INVALID, QR_TIMEOUT, QR_ACK_MISMATCH } qr_result_t;

static QueueHandle_t s_qr_uart_queue;   // UART event queue
static uint8_t s_rxbuf[512];

// Send a framed command; optionally match an ACK.
qr_result_t qr_send(const uint8_t *cmd, size_t n,
                    const uint8_t *ack, size_t ack_len,
                    uint32_t timeout_ms) {
    uart_flush_input(QR_UART);                       // "clear rx buffer"
    uart_write_bytes(QR_UART, cmd, n);
    if (!ack || ack_len == 0) return QR_OK;           // no reply expected
    int got = 0;
    uint8_t rx[16];
    int64_t deadline = esp_timer_get_time() + timeout_ms*1000;
    while (esp_timer_get_time() < deadline) {
        int len = uart_read_bytes(QR_UART, rx+got, ack_len-got,
                                  pdMS_TO_TICKS(5));
        if (len > 0) { got += len; if (got >= (int)ack_len) break; }
    }
    if (got < (int)ack_len) return QR_TIMEOUT;
    return (memcmp(rx, ack, ack_len) == 0) ? QR_OK : QR_ACK_MISMATCH;
}

// Status query: reply is 44 <pid> <fid> <len_hi> <len_lo> <data...>
qr_result_t qr_get_info(uint8_t id, char *out, size_t out_cap) {
    uint8_t cmd[3] = {0x43, 0x02, id};
    qr_send(cmd, 3, NULL, 0, 0);
    int n = uart_read_bytes(QR_UART, s_rxbuf, sizeof(s_rxbuf),
                            pdMS_TO_TICKS(500));
    if (n < 5 || s_rxbuf[0] != 0x44) return QR_TIMEOUT;
    uint16_t len = (s_rxbuf[3] << 8) | s_rxbuf[4];
    if (len > out_cap-1) len = out_cap-1;
    memcpy(out, s_rxbuf+5, len); out[len] = 0;
    return QR_OK;
}

// Scan-result pump: feed bytes into a line buffer; emit complete codes.
void qr_rx_task(void *arg) {
    uart_event_t ev;
    uint8_t b[64];
    while (true) {
        if (xQueueReceive(s_qr_uart_queue, &ev, portMAX_DELAY)) {
            if (ev.type == UART_DATA) {
                int len = uart_read_bytes(QR_UART, b, ev.size, 0);
                scan_accumulate(b, len);   // see §4.5; fires on_scan_result()
            }
        }
    }
}
```

### 4.7 Pseudocode: scan-result accumulator

```c
// scan_line.c
static char s_line[1024];
static size_t s_len;
static int64_t s_last_byte_us;

void scan_accumulate(const uint8_t *data, int n) {
    int64_t now = esp_timer_get_time();
    // quiet-time boundary: if >50ms gap, start a new line
    if (s_len && (now - s_last_byte_us) > 50000) s_len = 0;
    for (int i = 0; i < n; i++) {
        if (s_len < sizeof(s_line)-1) s_line[s_len++] = data[i];
        if (s_len >= 2 && s_line[s_len-2]=='\r' && s_line[s_len-1]=='\n') {
            s_line[s_len-2] = 0;                 // strip \r\n
            on_scan_result(s_line);             // -> UI queue
            s_len = 0;
        }
    }
    s_last_byte_us = now;
    // safety: cap length
    if (s_len >= sizeof(s_line)-1) { s_line[s_len]=0; on_scan_result(s_line); s_len=0; }
}
```

---

## 5. Current-state analysis (this repo)

### 5.1 What already exists and we will reuse

- **`0114-papers3-pulp-os`** — the most recent ESP-IDF firmware that uses
  **M5Unified + M5GFX** via `managed_components/m5stack__m5unified` and
  `m5stack__m5gfx`. It pins **IDF 5.3.4**, target `esp32s3`. This is the
  template for how to do "Arduino libs inside ESP-IDF" in *this* repo.
  Evidence: `0114-papers3-pulp-os/README.md` line 12
  (`source ~/esp/esp-idf-5.3.4/export.sh  # 5.3.4 PINNED`) and
  `0114-papers3-pulp-os/managed_components/`.
- **`0095-cores3-wifi-bench`** — the only existing **CoreS3** ESP-IDF project
  (headless). Its `sdkconfig.defaults` is the CoreS3 baseline: USB
  Serial/JTAG console, PSRAM quad (CoreS3 is 8 MB quad PSRAM — note 0114 uses
  octal for PaperS3; **CoreS3 uses quad** per `0095`), CPU 240 MHz, QIO flash
  80 MHz.
- **The official M5Module-QRCode Arduino library** (mirrored in
  `sources/arduino-lib/`) — the protocol layer (`qrcode_m14.cpp`) is ~300
  lines of straightforward serial code and is directly portable; the wrapper
  (`M5ModuleQRCode.cpp`) shows the IO-expander wiring.

### 5.2 What does NOT exist

- No barcode/QR firmware anywhere in the repo (searched
  `rg -il "quirc|barcode|qr.?code|qrcode|zxing"` — only unrelated hits in
  printer/printer-research projects and LovyanGFX examples).
- No ESP-IDF component for the PI4IOE5V6408 expander — but M5Unified ships
  `utility/PI4IOE5V6408_Class.hpp`, which the Arduino lib already uses. So we
  get it for free via the M5Unified managed component.

### 5.3 Gap analysis

| Need | Gap | Resolution |
|------|-----|------------|
| Drive M14-Pro UART from ESP-IDF | No existing driver | Port `qrcode_m14.cpp` → small `qrcode_m14` ESP-IDF component (C, UART driver) |
| Power-enable + TRIG via I2C expander | No ESP-IDF expander driver | Use M5Unified's `PI4IOE5V6408_Class` (it's C++ and works under ESP-IDF via the managed component) — same as the Arduino lib |
| Render decoded text on CoreS3 LCD | 0095 is headless | Use M5GFX `M5.Display` (the repo standard); build a tiny UI (current code + history) |
| USB Serial/JTAG console with commands | Pattern exists in many projects (e.g. `wifi_console`) | Add an `esp_console` REPL with `scan`, `start`, `stop`, `mode`, `status`, `reset` commands |
| CoreS3 PSRAM config | 0114 (PaperS3) uses octal; CoreS3 needs **quad** | Copy `0095-cores3-wifi-bench/sdkconfig.defaults` PSRAM block (`SPIRAM_MODE_QUAD`) not 0114's octal |

---

## 6. Proposed architecture

### 6.1 Component layout

```
0118-cores3-qrcode-scanner/
├── CMakeLists.txt
├── sdkconfig.defaults                 # CoreS3: USB Serial/JTAG, quad PSRAM, 240MHz
├── dependencies.lock
├── partitions.csv                      # 16 MB flash, custom partitions (app > 1 MB)
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml               # m5stack/m5unified, m5stack/m5gfx  (NOT root-level)
    ├── app_main.cpp                    # boot: init display, init scanner, start tasks
    ├── qr_engine.h / qr_engine.cpp     # M14-Pro UART protocol (port of qrcode_m14)
    ├── qr_module.h / qr_module.cpp     # PI4IOE5V6408 power+TRIG, UART init, scan pump
    ├── qr_ui.h / qr_ui.cpp             # M5GFX render: current code + history + status
    ├── qr_console.h / qr_console.cpp    # esp_console commands over USB Serial/JTAG
    └── assets/                         # (optional) splash/logo
```

> **Why a `main/`-only layout (no separate `components/`)?** The protocol
> is small and tightly coupled to the app; keeping it in `main/` matches
> the repo's simpler projects. If the driver later becomes reusable across
> devices, promote `qr_engine` + `qr_module` to `components/qrcode_m14/`
> (mirror the NFC-component extraction done in ESP-61).

### 6.2 Runtime model (tasks)

```
 app_main (core 0)
   ├─ M5.begin()                    # M5Unified: display (M5GFX ILI9341), In_I2C, AXP
   ├─ qr_module_begin()             # PI4IOE5V6408 @0x43 on In_I2C; QR_5V_EN=1; UART1 115200 8N1
   ├─ qr_engine_configure()         # set trigger mode, lights, beep (over UART)
   ├─ qr_console_start()            # esp_console on USB Serial/JTAG
   ├─ xTaskCreate(qr_ui_task)       # core 1: render loop (~20 fps), reads result queue
   └─ xTaskCreate(qr_rx_task)        # core 0: UART event task, feeds scan_accumulate()

 Data flow:
   M14-Pro --UART--> qr_rx_task --accumulate--> xQueue(result_t) --qr_ui_task--> M5.Display
                                                                          \
                                                                           --> ESP_LOG / console
```

- `qr_rx_task` is the UART event task (ESP-IDF UART driver event queue). It
  never blocks the UI.
- Results go on a FreeRTOS queue (`ScanResult { char text[256]; uint8_t symbology; int64_t ts_us; }`).
- `qr_ui_task` pops the queue and updates the display. The display is
  touched from **one task only** (the UI task) to avoid M5GFX races.
- The console runs on the system console task (esp_console), independent.

### 6.3 Pin/UART assignment (CoreS3)

| Function | Peripheral | GPIO | Direction | Notes |
|----------|------------|------|-----------|-------|
| QR module UART | UART1 | RX=**G18**, TX=**G17** | CoreS3↔module | Port-C UART; set DIP switch to route engine UART here |
| IO expander I2C | I2C num 0 (In_I2C) | SDA=G12, SCL=G11 | CoreS3→expander | PI4IOE5V6408 @0x43 |
| LCD | SPI (M5GFX manages) | M5-bus SPI pins | output | ILI9341 320×240 |
| Touch | I2C (In_I2C) | G12/G11 | input | FT6206 (optional for UI) |
| Console/flash | USB Serial/JTAG | — | host link | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` |

> The CoreS3's UART0 (G43/G44) is consumed by the USB Serial/JTAG bridge
> internally; **do not** use UART0 for the scanner. Use **UART1** on
> G17/G18. (The Arduino example's pin numbers differ because it targets
> Basic v2.7, not CoreS3.)

---

## 7. Decision records

### Decision: Arduino-on-ESP-IDF via M5Unified/M5GFX (not pure C/esp_lcd)

- **Context:** The scanner module needs M5Unified's `PI4IOE5V6408_Class` and
  the repo already standardizes on M5GFX for M5 device displays.
- **Options considered:**
  1. Pure C ESP-IDF + `esp_lcd` (ILI9341 panel) + hand-written I2C expander
     driver. Maximally "native", but reimplements both the display stack and
     the expander driver, and diverges from every display firmware in the
     repo.
  2. **Arduino-on-ESP-IDF with M5Unified + M5GFX managed components** (as
     `0114-papers3-pulp-os` does). Reuses `PI4IOE5V6408_Class`, reuses M5GFX
     display, can lift the protocol layer from the Arduino lib almost
     verbatim.
- **Decision:** Option 2.
- **Rationale:** Lowest risk, repo-consistent, reuses the official
  expander class and the proven display stack; the only truly new code is
  the ESP-IDF UART glue and the UI.
- **Consequences:** Build pulls two managed components; C++ in `main/`
  (fine — 0114 is C++ too). Must keep PSRAM config to **quad** (CoreS3),
  not octal.
- **Status:** accepted

### Decision: RS232/UART mode for the module (not USB-HID/POS)

- **Context:** The module can be a USB keyboard/POS scanner to a PC, but
  that disables UART and makes the CoreS3 pointless.
- **Decision:** Keep module in **RS232 (UART)** mode (config `0x42 0x40 0x00`).
- **Rationale:** The CoreS3 is the host; it needs the decoded text over UART.
- **Consequences:** The on-module DIP switch must be set to the **UART**
  side (not USB). Document this in the bring-up steps.
- **Status:** accepted

### Decision: external 12 V required; firmware still drives QR_5V_EN

- **Context:** USB 5 V is insufficient for the engine LEDs; 12 V on the DC
  jack powers the whole stack.
- **Decision:** Operator **must** connect 12 V. Firmware still toggles
  `QR_5V_EN` via the expander for clean enable/disable and USB-only bench
  testing.
- **Rationale:** Spec table lists 12 V as supported; back-feeds 5 V to the
  CoreS3.
- **Consequences:** Bring-up docs must warn about center-positive polarity;
  add a runtime check (if `getFirmwareVersion()` times out, log "is 12 V
  connected / DIP switch set to UART?").
- **Status:** accepted

### Decision: line-oriented scan parser with quiet-time fallback

- **Context:** Scan output is raw bytes + suffix; exact framing depends on
  config.
- **Decision:** Accumulate bytes; emit on `\r\n`, or 50 ms quiet-time, or
  length cap. Make the suffix configurable.
- **Consequences:** May need one tuning pass on real hardware; probe script
  (`scripts/01-probe-qrcode-uart.py`) is the debugging tool.
- **Status:** proposed (validate in Phase 2)

---

## 8. Implementation plan (phased, file-level)

### Phase 0 — Bring-up & proof (no firmware yet)

1. **Hardware setup:** stack Module13.2 QRCode under CoreS3; set the
   module DIP switch to **UART** side routing engine UART → Port-C UART.
2. **Power:** connect 12 V center-positive to the module DC jack.
3. **Verify the module standalone** (before writing CoreS3 firmware): wire
   the module's PORT.C UART (TX/RX) to a 3.3 V USB-TTL adapter and run
   `python3 scripts/01-probe-qrcode-uart.py --port /dev/ttyUSB0 --scan`.
   Expect a `0x44` firmware-version reply and streamed decoded text when
   aiming at a QR code. **If no reply:** the DIP switch is on USB, or 12 V
   is missing, or polarity is wrong.
4. Commit the probe output to `various/` for the diary.

### Phase 1 — Project skeleton + display

1. `cp -r 0114-papers3-pulp-os`-style structure or create
   `0118-cores3-qrcode-scanner/` with `CMakeLists.txt`,
   `main/CMakeLists.txt`, `main/idf_component.yml`:
   ```yaml
   dependencies:
     m5stack/m5unified:
       version: "~0.2.0"
     m5stack/m5gfx:
       version: "~0.2.0"
   ```
2. `sdkconfig.defaults` (from `0095-cores3-wifi-bench` baseline, quad PSRAM):
   see §9.
3. `app_main.cpp`: `M5.begin()`, `M5.Display` "Hello", print chip info,
   confirm boot on USB Serial/JTAG. Build + flash with
   `scripts/02-bringup-build-flash.sh monitor`.
4. **Done when:** CoreS3 shows "Hello" and `idf.py monitor` prints logs over
   USB Serial/JTAG.

### Phase 2 — Scanner driver (UART + I2C expander)

1. `qr_module.cpp`: init `PI4IOE5V6408_Class` on `M5.In_I2C` at 0x43; set
   ch0 (QR_5V_EN) and ch4 (TRIG) as outputs; enable power; open UART1 on
   G17/G18 @115200 with an event queue.
2. `qr_engine.cpp`: port `qrcode_m14.cpp` — `sendCmd`, `getInfos`,
   `setTriggerMode`, `setFillLightMode`, `setPosLightMode`, `startDecode`,
   `stopDecode`.
3. `qr_rx_task`: UART event task → `scan_accumulate` → result queue.
4. **Done when:** console command `status` prints firmware version +
   serial number read back from the module; aiming at a code logs the
   decoded text over USB Serial/JTAG.

### Phase 3 — On-screen UI

1. `qr_ui.cpp`: M5GFX canvas. Layout:
   - top bar: mode + state (scanning/idle) + firmware version,
   - center: **last decoded code** in large mono font, word-wrapped,
   - bottom: scroll-back history of last ~8 codes with timestamps,
   - footer: hint ("BtnA: toggle scan  BtnB: mode").
2. `qr_ui_task` pops the result queue and redraws (only on change to avoid
   flicker; M5GFX double-buffer / `displayBusy()` guard as in
   `components/s3paper_m5/src/m5_backend.cpp`).
3. CoreS3 touch / physical buttons (M5.BtnA/BtnB via M5Unified) for
   start/stop and mode cycling.
4. **Done when:** aiming at a QR/barcode shows the text on screen within
   ~1 read cycle; repeated reads update history.

### Phase 4 — Console commands + polish

1. `qr_console.cpp`: `esp_console` commands: `start`, `stop`, `mode
   <key|cont|auto|pulse|sense>`, `light <off|decode|on>`, `brightness
   <0-100>`, `beep <on|off>`, `status`, `reset` (factory reset), `info`.
2. Boot banner: log config + warn if `getFirmwareVersion()` failed
  (likely 12 V / DIP switch).
3. Partition table for 16 MB flash (app > 1 MB) — see §9.
4. **Done when:** all commands work; `idf.py fullclean && idf.py build`
  reproducible; flash + boot stable.

### Phase 5 — Hardening (future)

- Configurable suffix/protocol-format over console; quiet-time tuning.
- Optional image pull (`0x60`) to show a live preview thumbnail (advanced).
- Persist last codes to NVS; export over console.
- Symbology enable/disable menu.

---

## 9. Key configuration & API references

### 9.1 `sdkconfig.defaults` (CoreS3 baseline)

```ini
# Target
CONFIG_IDF_TARGET="esp32s3"

# USB Serial/JTAG console (AGENTS.md S3 rule)
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y

# Flash: QIO 80 MHz (CoreS3)
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

# PSRAM: 8 MB Quad (CoreS3)  -- NOT octal (that's PaperS3)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_USE_MALLOC=y

# CPU 240 MHz
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y

# M5GFX/M5Unified expect these; custom partitions for >1MB app
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"
```

`partitions.csv` (16 MB flash): factory ~3 MB + nvs + otadata + app nvs.

### 9.2 ESP-IDF UART API (the calls to use)

```c
#include "driver/uart.h"
#define QR_UART       UART_NUM_1
#define QR_UART_BAUD  115200
#define QR_TX         17
#define QR_RX         18
#define QR_BUF        1024

uart_config_t u = {
    .baud_rate = QR_UART_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};
uart_driver_install(QR_UART, QR_BUF, QR_BUF, 20, &s_qr_uart_queue, 0);
uart_param_config(QR_UART, &u);
uart_set_pin(QR_UART, QR_TX, QR_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
// read: uart_read_bytes(QR_UART, buf, n, pdMS_TO_TICKS(5));
// write: uart_write_bytes(QR_UART, cmd, len);
```

### 9.3 M5Unified I2C expander (reuse, not rewrite)

```cpp
#include <M5Unified.h>
#include <utility/PI4IOE5V6408_Class.hpp>
m5::PI4IOE5V6408_Class ioexp(0x43, 100000, &M5.In_I2C);
ioexp.begin();
ioexp.setDirection(0, OUTPUT);  // ch0 = QR_5V_EN
ioexp.setDirection(4, OUTPUT);  // ch4 = TRIG
ioexp.digitalWrite(0, true);    // power on engine
ioexp.digitalWrite(4, true);    // TRIG idle high (active low)
```
(Equivalent to `M5ModuleQRCode::_init_pi4ioe5v6408()` in
`sources/arduino-lib/src/M5ModuleQRCode.cpp`.)

### 9.4 M5GFX display (UI)

```cpp
M5Canvas canvas(&M5.Display);
canvas.createSprite(320, 240);
canvas.setFont(&fonts::Font0);
canvas.setTextColor(TFT_WHITE, TFT_BLACK);
canvas.printf("...");            // draw current code
canvas.pushSprite(0,0);          // blit
```
The `s3paper_m5` component (`components/s3paper_m5/src/m5_backend.cpp`) shows
the repo's `displayBusy()` guard + PSRAM canvas pattern to copy.

---

## 10. Test & validation strategy

1. **Unit-ish (host):** `scripts/01-probe-qrcode-uart.py` proves the protocol
   against the real module before any CoreS3 code.
2. **Boot smoke:** Phase 1 — display + USB Serial/JTAG logs.
3. **Driver:** Phase 2 — `status` console command returns firmware/serial
   (proves UART + I2C expander + power).
4. **Functional:** Phase 3 — aim at 5 different codes (QR, Data Matrix,
   Code128, EAN-13, PDF417); each appears on screen; history scrolls.
5. **Robustness:** unplug/replug 12 V; power-cycle; verify clean re-init.
   Continuous mode for 10 min, no watchdog reboots.
6. **Build reproducibility:** `idf.py fullclean && idf.py build` from a
   clean shell with IDF 5.3.4 sourced.

---

## 11. Risks, alternatives, open questions

- **R1 — exact scan terminator.** Default `\r\n` assumed; if the module is
  in a non-default protocol format, results may not split cleanly. Mitigation:
  probe script + configurable suffix.
- **R2 — DIP switch misconfiguration.** If the switch is on USB, UART is
  dead and `status` times out. Mitigation: boot-time check + clear log
  message.
- **R3 — 12 V polarity / supply.** Wrong polarity can damage the module.
  Mitigation: document center-positive; operator double-checks.
- **R4 — PSRAM octal vs quad.** Copying 0114's octal PSRAM config to a
  CoreS3 will fail to boot. Mitigation: use `0095`'s quad config.
- **A1 (alternative):** Use `esp_lcd` + LVGL instead of M5GFX. More work,
  diverges from repo. Rejected for MVP.
- **A2 (alternative):** Pull raw images (`0x60`) and decode on the CoreS3
  with quirc. Unnecessary — the engine already decodes. Future only.
- **Q1:** Does the CoreS3 draw enough from the 12 V back-feed to also run
  USB flashing without a USB power source? (Likely yes, but the operator
  should keep USB for flash/console regardless.)

---

## 12. References (key files)

- `sources/protocol-pdf/Module13.2-QRCode-Protocol-EN.txt` — the protocol
  spec (read first).
- `sources/arduino-lib/src/qrcode_m14.cpp` — reference protocol impl (port this).
- `sources/arduino-lib/src/M5ModuleQRCode.cpp` — expander + UART init reference.
- `sources/arduino-lib/examples/ContinuousMode/ContinuousMode.ino` — usage shape.
- `sources/docs/01-Module13.2-QRCode-product-page.txt` — pinmap + power + CoreS3 bus mapping.
- `sources/protocol-pdf/ZBarcode-Scanner-User-Guide-2.5-EN.txt` — engine serial/trigger details.
- `scripts/01-probe-qrcode-uart.py` — host-side protocol probe.
- `scripts/02-bringup-build-flash.sh` — build/flash helper.
- `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os/` — ESP-IDF + M5Unified/M5GFX template (IDF 5.3.4, esp32s3).
- `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0095-cores3-wifi-bench/sdkconfig.defaults` — CoreS3 baseline config.
- `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/AGENTS.md` — build/console rules.
- `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/components/s3paper_m5/src/m5_backend.cpp` — M5GFX displayBusy/canvas pattern.
