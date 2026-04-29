---
title: "ATOM_PRINTER_CMD_v1.06 — English Translation"
tags:
  - reference
  - translation
  - thermal-printer
  - escpos
  - m5stack
  - k118
created: 2026-04-29
source_pdf: ATOM_PRINTER_CMD_v1.06.pdf
source_text: ATOM_PRINTER_CMD_v1.06.txt
status: active
---

# ATOM_PRINTER_CMD_v1.06 — English Translation

This document is an English translation and implementation-oriented reference for M5Stack's `ATOM_PRINTER_CMD_v1.06.pdf` command manual.

Local files:

- Original PDF: `0090-m5printer-research/docs/ATOM_PRINTER_CMD_v1.06.pdf`
- Extracted Chinese text: `0090-m5printer-research/docs/ATOM_PRINTER_CMD_v1.06.txt`
- This English translation/reference: `0090-m5printer-research/docs/ATOM_PRINTER_CMD_v1.06.en.md`

> [!note]
> This is a practical engineering translation. It translates the command list and the commands most relevant to the SToMS3R firmware in detail: status queries, raster bitmaps, baud rate, density, speed, software flow control, barcode, QR code, and general printer setup. The original extracted Chinese text is kept next to this file for exact wording.

## Notation

| Symbol | Meaning |
|--------|---------|
| `ESC` | `0x1B` |
| `GS` | `0x1D` |
| `FS` | `0x1C` |
| `DLE` | `0x10` |
| `NUL` | `0x00` |
| `SP` | space, `0x20` |
| `##` | two literal `#` bytes, `0x23 0x23` |

Most multibyte integer parameters in the custom `ESC ##...` commands use little-endian byte order.

## Table of contents — translated command index

### Chapter 1 — General print and setup commands

| Section | Command | English description |
|---------|---------|---------------------|
| 1.1 | `ESC S0` | Set double-width character printing |
| 1.2 | `ESC DC4` | Cancel double-width character printing |
| 1.3 | `ESC SP n` | Set right-side character spacing |
| 1.4 | `ESC $ nL nH` | Set absolute print position |
| 1.5 | `ESC V n` | Select/cancel 90° clockwise rotation |
| 1.6 | `ESC { n` | Select/cancel upside-down printing mode |
| 1.7 | `GS ! n` | Select character size |
| 1.8 | `GS L nL nH` | Set left margin |
| 1.9 | `GS P x y` | Set horizontal and vertical motion units |
| 1.10 | `ESC \\ nL nH` | Set relative horizontal print position |
| 1.11 | `ESC a n` | Select alignment |
| 1.12 | `ESC ! n` | Select print mode |
| 1.13 | `ESC E n` | Select/cancel bold mode |
| 1.14 | `ESC G n` | Select/cancel double-strike mode |
| 1.15 | `ESC @` | Initialize printer |
| 1.16 | `ESC - n` | Select/cancel underline mode |
| 1.17 | `ESC 2` | Set default line spacing |
| 1.18 | `ESC 3 n` | Set line spacing |
| 1.19 | `ESC D n1...nk NUL` | Set horizontal tab positions |
| 1.20 | `ESC d n` | Print and feed `n` lines |
| 1.21 | `ESC J n` | Print and feed paper |
| 1.22 | `ESC B n t` | Beeper prompt when printing an order |
| 1.23 | `ESC C m t n` | Beeper prompt and alarm light flash when printing an order |
| 1.24 | `ESC p m t1 t2` | Open cash drawer |
| 1.25 | `GS r n` | Return printer status |
| 1.26 | `GS S` | Print test page and cut paper |
| 1.27 | `GS I n` | Print machine information |
| 1.28 | `GS V m` / `GS V m n` | Select cut mode and cut paper |
| 1.29 | `ESC M n` | Select font |
| 1.30 | `ESC c 3 n` | Select paper sensor for paper-out signal output |
| 1.31 | `ESC 6 n` | Configure whether buffer data is cleared when out of paper |
| 1.32 | `ESC 7 n` | Select DTR signal behavior when out of paper |
| 1.33 | `ESC c 4 n` | Select paper sensor that stops printing |
| 1.34 | `ESC c 5 n` | Enable/disable panel key |
| 1.35 | `ESC = n` | Select printer |
| 1.36 | `ESC t n` | Select character code table |
| 1.37 | `FS &` | Select Chinese-character mode |
| 1.38 | `FS q n ...` | Define Flash bitmap(s) |
| 1.39 | `FS p n m` | Print bitmap downloaded to Flash |
| 1.40 | `DLE EOT n` | Real-time status transmission |
| 1.41 | `GS v 0 m xL xH yL yH d1...dk` | Print raster bitmap |
| 1.42 | `ESC * m n1 n2 k1...kn` | Select bit-image mode |
| 1.43 | `GS * x y d1...d(x*y*8)` | Define downloaded bit image |
| 1.44 | `GS / m` | Print downloaded bit image |
| 1.45 | `GS B n` | Black/white inverse printing |
| 1.46 | `GS r n` | Return status |
| 1.47 | `ESC % n` | Select/cancel user-defined characters |
| 1.48 | `ESC &` | Define user-defined characters |
| 1.49 | `ESC ? n` | Cancel user-defined characters |
| 1.50 | `US ESC` | Modify IP address |
| 1.51 | `US ESC` | Modify mask |
| 1.52 | `US ESC` | Modify gateway |

### Chapter 2 — Custom print and setup commands

| Section | Command | English description |
|---------|---------|---------------------|
| 2.1 | `ESC ## SELF` | Print self-test information |
| 2.2 | `ESC ## EHEX` | Enter hexadecimal mode |
| 2.3 | `ESC ## ETBN` | Enter burn-in/aging mode |
| 2.4 | `ESC ## RTFA` | Restore factory settings |
| 2.5 | `ESC ## STSN` | Set mainboard serial number |
| 2.6 | `ESC ## SDAT` | Set factory date |
| 2.7 | `ESC ## STIF` | Set printer interface |
| 2.8 | `ESC ## SBDR` | Set serial baud rate |
| 2.9 | `ESC ## STID id` | Set printer ID |
| 2.10 | `ESC ## STDP n` | Set print density |
| 2.11 | `ESC ## BDIT time` | Set burn-in/aging time |
| 2.12 | `ESC ## STSP speed` | Set print speed |
| 2.13 | `ESC ## DLSF` | Download self-test information |
| 2.14 | `ESC ## UPPG` | Update program |
| 2.15 | `ESC ## ECAT n` | Enable/disable cutter function |
| 2.16 | `ESC ## EFCT n` | Enable/disable full-cut function |
| 2.17 | `ESC ## ECAH n` | Enable cutter auto-homing |
| 2.18 | `ESC ## STBP n` | Set beeper function |
| 2.19 | `ESC ## CTFD n` | Set feed distance before cutter |
| 2.20 | `ESC ## ACFD n` | Set feed distance after cutter |
| 2.21 | `ESC ## CTGH n` | Cut, then search for black mark |
| 2.22 | `ESC ## EAFB n` | Enable automatic black-mark search |
| 2.23 | `ESC ## RESD type len d0...dlen` | Forward data between multiple interfaces |
| 2.24 | `ESC ## TDNA len d0...dlen` | Set manufacturer name |
| 2.25 | `ESC ## MANA len d0...dlen` | Set machine name |
| 2.26 | `ESC ## SFQR len d0...dlen` | Set self-test page QR print information |
| 2.27 | `ESC ## FEMC n` | Enable multi-interface data conversion |
| 2.28 | `ESC ## FMD5 n` | Enable MD5 function |
| 2.29 | `ESC ## FTKT n` | Enable one-ticket-one-control function |
| 2.30 | `ESC ## FLLF` | Enable/disable LF line-feed function |
| 2.31 | `ESC ## SPTI` | Set printer standby time |
| 2.32 | `ESC ## TACH` | Set Thai font type |
| 2.33 | `ESC ## QPIX` | Set QR-code pixel size |
| 2.34 | `ESC ## splv` | Set portable Bluetooth printer speed level |
| 2.35 | `ESC ## enfw` | Enable/disable firewall function |
| 2.36 | `ESC ## fwps` | Firewall: data print |
| 2.37 | `ESC ## BTPI` | Set Bluetooth PIN code |
| 2.38 | `ESC ## BTRN` | Set Bluetooth name |
| 2.39 | `ESC ## BTTY` | Set Bluetooth type |
| 2.40 | `ESC ## PGMD` | Set reprint mode |
| 2.41 | `ESC ## CDTY` | Set encoding type |
| 2.42 | `ESC ## strm` | Set right limit |
| 2.43 | `ESC ## BMUL` | Enable/disable Bluetooth multipoint connection |
| 2.44 | `ESC ## SBTT` | Set Bluetooth type |
| 2.45 | `ESC ## LFCH` | Set line-feed command |
| 2.46 | `ESC ## BTFP` | Bluetooth command free channel |
| 2.47 | `ESC ## SPMD` | Set print mode |
| 2.48 | `ESC ## SFFC` | Enable/disable software flow control |
| 2.49 | `ESC ## CBUF` | Enable/disable clearing buffer on error |
| 2.50 | `ESC # D` | Print factory date, serial number, and ID |
| 2.51 | `ESC # S` | Print mainboard serial number |
| 2.52 | `ESC # V` | Print software version |
| 2.53 | `ESC # F` | Print function list |
| 2.54 | `ESC # G` | Print switch status |
| 2.55 | `ESC # H` | Print language list |
| 2.56 | `ESC # I` | Print manufacturer name |
| 2.57 | `ESC # J` | Print machine name |
| 2.58 | `ESC # K` | Print density level |
| 2.59 | `ESC # L` | Print print-head temperature |
| 2.60 | `ESC # M` | Print print speed |
| 2.61 | `ESC # N` | Print battery information |
| 2.62 | `ESC # O` | Print Bluetooth information |

### Chapter 3 — Black-mark commands

| Section | English description |
|---------|---------------------|
| 3.1 | Enable black-mark detection |
| 3.2 | Disable black-mark detection |
| 3.3 | Set maximum ticket length for black-mark paper |
| 3.4 | Set black-mark width |
| 3.5 | Set distance between sensor and print position, in steps |
| 3.6 | Set distance between print position and tear-off position, in steps |
| 3.7 | Feed paper to next ticket/sheet |
| 3.8 | `ESC ## GBCV` — get black-mark threshold voltage |

### Chapter 4 — Barcode and 2D-code commands

| Section | Command | English description |
|---------|---------|---------------------|
| 4.1 | `GS E C` | Barcode on/off command |
| 4.2 | `GS h n` | Select barcode height |
| 4.3 | `GS H n` | Select HRI character print position |
| 4.4 | `GS k ...` | Print barcode |
| 4.5 | QR Code | Set pixel-dot size |
| 4.6 | QR Code | Set module/cell size |
| 4.7 | QR Code | Set error-correction level |
| 4.8 | QR Code | Transfer data to encoding buffer |
| 4.9 | QR Code | Print 2D barcode from encoding buffer |
| 4.10 | PDF417 | Set number of data-area columns |
| 4.11 | PDF417 | Set number of data-area rows |
| 4.12 | PDF417 | Set module width |
| 4.13 | PDF417 | Set row height |
| 4.14 | PDF417 | Set error-correction level |
| 4.15 | PDF417 | Select/cancel truncated mode |
| 4.16 | PDF417 | Transfer data to encoding buffer |
| 4.17 | PDF417 | Print 2D code from encoding buffer |

### Chapter 5 — Status query commands

| Section | Command | English description |
|---------|---------|---------------------|
| 5.1 | `GS a n` | Return printer status |
| 5.2 | `GS g 1` | Return print density level |
| 5.3 | `GS g 2` | Return print speed |
| 5.4 | `GS g 3` | Return current language type |
| 5.5 | `GS g 4` | Return black-mark parameters |
| 5.6 | `GS g 5` | Return current black-mark sensor voltage |
| 5.7 | `GS g 6` | Return printer temperature |
| 5.8 | `GS g 7` | Return serial baud rate |
| 5.9 | `GS g 8` | Return whether beeper is enabled |
| 5.10 | `GS g 9` | Read mainboard serial number |
| 5.11 | `GS g a` | Return printer ID |
| 5.12 | `GS g b` | Return factory date |
| 5.13 | `GS g c` | Return whether cutter auto-reset is enabled |
| 5.14 | `GS g d` | Return whether barcode function is enabled |
| 5.15 | `GS g e` | Return feed distance before cutter |
| 5.16 | `GS g f` | Return printer software version |
| 5.17 | `GS g g` | Return manufacturer name |
| 5.18 | `GS g h` | Return machine name |
| 5.19 | `GS g i` | Return feed distance after cutter |
| 5.20 | `GS g k` | Return machine type |
| 5.21 | `GS g l` | Return print-head voltage |
| 5.22 | `GS g m` | Return or print print-head/paper-feed usage record |

### Chapters 6–9 — Other command groups

| Chapter | English title / purpose |
|---------|-------------------------|
| 6 | MD5 encryption commands: download/read write-only ID, download random key, get MD5-encrypted data |
| 7 | Language-setting commands |
| 8 | Network-parameter setting commands: IP, mask, gateway, DHCP, router name/password, cloud server domain/port/activation/binding codes |
| 9 | Page-mode commands: enable/disable page mode, change command mode (`ESC`/`TSC`) |

## Detailed translations for firmware-relevant commands

### 1.15 `ESC @` — Initialize printer

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `ESC @` |
| Hex | `1B 40` |
| Decimal | `27 64` |

Function: initialize the printer and restore default state.

Implementation note: this is the reset command used by `printer_init` and at firmware startup.

### 1.20 `ESC d n` — Print and feed `n` lines

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `ESC d n` |
| Hex | `1B 64 n` |
| Decimal | `27 100 n` |

Function: print buffered data and feed paper forward by `n` lines.

### 1.25 / 1.46 `GS r n` — Return status

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `GS r n` |
| Hex | `1D 72 n` |
| Decimal | `29 114 n` |

Function: return printer status selected by `n`.

Implementation note: this is separate from `DLE EOT n`, which is a real-time status command.

### 1.40 `DLE EOT n` — Real-time status transmission

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `DLE EOT n` |
| Hex | `10 04 n` |

Range: `1 <= n <= 4`.

Function: transmit the printer status selected by `n` in real time.

#### `n = 1` — Printer status

| Bit | Value | Meaning |
|-----|-------|---------|
| Bit0 | `0` | Fixed 0 |
| Bit1 | `1` | Fixed 1 |
| Bit2 | `0` | One or two cash drawers open |
| Bit2 | `1` | Both cash drawers closed |
| Bit3 | `0` | Online |
| Bit3 | `1` | Offline |
| Bit4 | `1` | Fixed 1 |
| Bit5–6 | — | Undefined |
| Bit7 | `0` | Fixed 0 |

#### `n = 2` — Offline status

| Bit | Value | Meaning |
|-----|-------|---------|
| Bit0 | `0` | Fixed 0 |
| Bit1 | `1` | Fixed 1 |
| Bit2 | `0` | Cover closed |
| Bit2 | `1` | Cover open |
| Bit3 | `0` | Feed key not pressed |
| Bit3 | `1` | Feed key pressed |
| Bit4 | `1` | Fixed 1 |
| Bit5 | `0` | Paper present |
| Bit5 | `1` | Paper out |
| Bit6 | `0` | No error |
| Bit6 | `1` | Error present |
| Bit7 | `0` | Fixed 0 |

#### `n = 3` — Error status

| Bit | Value | Meaning |
|-----|-------|---------|
| Bit0 | `0` | Fixed 0 |
| Bit1 | `1` | Fixed 1 |
| Bit2 | — | Undefined |
| Bit3 | `0` | Cutter normal |
| Bit3 | `1` | Cutter error |
| Bit4 | `1` | Fixed 1 |
| Bit5 | `0` | No unrecoverable error |
| Bit5 | `1` | Unrecoverable error |
| Bit6 | `0` | Print-head temperature and voltage normal |
| Bit6 | `1` | Print-head temperature or voltage out of range |
| Bit7 | `0` | Fixed 0 |

#### `n = 4` — Paper sensor status

| Bit | Value | Meaning |
|-----|-------|---------|
| Bit0 | `0` | Fixed 0 |
| Bit1 | `1` | Fixed 1 |
| Bit2–3 | `0` | Paper present |
| Bit2–3 | `1` / `0C` | Paper near end |
| Bit4 | `1` | Fixed 1 |
| Bit5 | `0` | No unrecoverable error |
| Bit5 | `1` | Unrecoverable error |
| Bit6 | `0` | Paper present |
| Bit6 | `1` / `60` | Paper end |
| Bit7 | `0` | Fixed 0 |

### 1.41 `GS v 0 m xL xH yL yH d1...dk` — Print raster bitmap

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `GS v 0 m xL xH yL yH d1...dk` |
| Hex | `1D 76 30 m xL xH yL yH d1...dk` |
| Decimal | `29 118 48 m xL xH yL yH d1...dk` |

Parameter ranges:

- `m`: `0 <= m <= 3`, or ASCII `'0'..'3'` (`48 <= m <= 51`).
- `xL`, `xH`: horizontal bitmap width in bytes, little-endian: `x = xL + xH * 256`.
- `yL`, `yH`: vertical bitmap height in dots, little-endian: `y = yL + yH * 256`.
- `d`: bitmap data bytes, each `0 <= d <= 255`.
- Data length: `k = (xL + xH * 256) * (yL + yH * 256)`, and `k != 0`.

Raster modes:

| `m` | Mode | Vertical resolution | Horizontal resolution |
|-----|------|---------------------|-----------------------|
| `0`, `48` | Normal | 200 DPI | 200 DPI |
| `1`, `49` | Double height | 200 DPI | 100 DPI |
| `2`, `50` | Double width | 100 DPI | 200 DPI |
| `3`, `51` | Double height + double width | 100 DPI | 100 DPI |

Notes:

- `xL`, `xH` specify the bitmap width in bytes.
- `yL`, `yH` specify the bitmap height in vertical dots.
- In standard mode, this command is valid only when the printer buffer is empty.
- Character scaling, bold, double-strike, upside-down printing, underline, inverse printing, and similar text modes do not affect this command.
- Any part of the bitmap outside the print area is not printed.
- `ESC a` alignment affects raster bitmaps.
- During macro definition, this command stops macro definition and executes immediately; it is not included as part of the macro.
- Each data byte represents 8 pixels; a bit value of `1` prints a dot, and a bit value of `0` leaves it blank.

Implementation notes for SToMS3R:

- K118 58mm width is up to 384 dots, so full-width raster rows are 48 bytes.
- Use `xL = 48`, `xH = 0` for 384-dot width.
- For `height = H`, use `yL = H & 0xFF`, `yH = H >> 8`.
- The web UI packs pixels MSB-first, which matches this command's expected byte-oriented raster format.

### 2.8 `ESC ## SBDR baudrate` — Set serial baud rate

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `ESC ##SBDR baudrate` |
| Hex | `1B 23 23 53 42 44 52 baudrate` |

Function: set the serial-port baud rate. `baudrate` is the baud-rate parameter.

Notes:

- `baudrate` length is 4 bytes.
- The integer is stored little-endian.
- Example: set baud to `115200` (`0x0001C200`):

```text
1B 23 23 53 42 44 52 00 C2 01 00
```

Known examples from M5Stack product docs:

| Baud | 32-bit value | Payload bytes |
|------|--------------|---------------|
| 9600 | `0x00002580` | `80 25 00 00` |
| 115200 | `0x0001C200` | `00 C2 01 00` |

Implementation note: SToMS3R's `set_baudrate <rate>` sends this command at the current baud rate, waits for TX to drain, then switches the ESP32 UART to the requested baud.

### 2.10 `ESC ## STDP n` — Set print density

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `ESC ## STDP n` |
| Hex | `1B 23 23 53 54 44 50 n` |

Function: adjust print density according to `n`.

- `n` is 1 byte.
- Range: `0 <= n <= 39`.
- `39` is the highest density / darkest print.

Example: set density level to `3`:

```text
1B 23 23 53 54 44 50 03
```

### 2.12 `ESC ## STSP speed` — Set print speed

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `ESC ## STSP n` |
| Hex | `1B 23 23 53 54 53 50 n` |

Function: set print speed. `n` is 1 byte.

Valid values:

| `n` | Speed |
|-----|-------|
| 25 | 25 mm/s |
| 30 | 30 mm/s |
| 37 | 37 mm/s |
| 50 | 50 mm/s |
| 56 | 56 mm/s |
| 62 | 62.5 mm/s |
| 70 | 70 mm/s |
| 80 | 80 mm/s |
| 90 | 90 mm/s |
| 100 | 100 mm/s |
| 120 | 120 mm/s |
| 150 | 150 mm/s |
| 180 | 180 mm/s |
| 200 | 200 mm/s |
| 220 | 220 mm/s |

Values not in the table are invalid.

Example: set print speed to 200 mm/s:

```text
1B 23 23 53 54 53 50 C8
```

### 2.48 `ESC ## SFFC n` — Enable/disable software flow control

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `ESC ## SFFC n` |
| Hex | `1B 23 23 53 46 46 43 n` |

Function: enable or disable software flow control.

| `n` | Meaning |
|-----|---------|
| `0` | Disable software flow control |
| `1` | Enable software flow control |

Implementation note: this is software flow control, not the same as the GPIO CTS hardware flow-control line used by the ESP32 UART driver.

### 2.50 `ESC ## SPSM n` — Set graphics print mode

The extracted text labels this section as `2.50 ESC ##SPSM 设置图形打印模式`.

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `ESC ## SPSM n` |
| Hex | `1B 23 23 53 50 53 4D n` |

Function: set graphics print mode.

| `n` | Meaning |
|-----|---------|
| `30` | BLE graphics printing |
| `31` | Adaptive-speed graphics printing |
| `32` | Constant-speed graphics printing |

This may be relevant for bitmap stripe/pause experiments.

### 4.4 `GS k ...` — Print barcode

The manual lists two forms:

```text
GS k m d1...dk NUL
GS k m n d1...dn
```

Function: print a barcode of type `m`, with either NUL-terminated data or explicit length `n` depending on barcode type/dialect.

Implementation note: SToMS3R currently uses the explicit-length form for supported barcode types.

### 4.7–4.9 QR Code commands

The manual groups QR code commands as:

1. Set error-correction level.
2. Transfer data to the encoding buffer.
3. Print the 2D barcode from the encoding buffer.

Implementation note: the M5Stack Arduino firmware uses the standard ESC/POS `GS ( k` style QR sequence:

```text
1D 28 6B 03 00 31 45 n          ; set error-correction level
1D 28 6B pL pH 31 50 30 data    ; store data
1D 28 6B 03 00 31 51 30         ; print stored symbol
```

### 5.1 `GS a n` — Return printer status

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `GS a n` |
| Hex | `1D 61 n` |
| Decimal | `29 97 n` |

`n` may be any number. The printer returns 4 status bytes.

#### Byte 1

| Bit | Meaning |
|-----|---------|
| Bit0–1 | Undefined, fixed 0 |
| Bit2 | Undefined, fixed 1 |
| Bit3 = 0 | Printer has buffer space and can accept data |
| Bit3 = 1 | Printer buffer is full and cannot accept data |
| Bit4 | Undefined, fixed 1 |
| Bit5 = 0 | Cover closed |
| Bit5 = 1 | Cover open |
| Bit6 = 0 | Not feeding by feed key |
| Bit6 = 1 | Feeding by feed key |
| Bit7 | Undefined, fixed 0 |

#### Byte 2

| Bit | Meaning |
|-----|---------|
| Bit0–2 | Undefined, fixed 0 |
| Bit3 = 0 | Cutter normal |
| Bit3 = 1 | Cutter error |
| Bit4 | Undefined, fixed 0 |
| Bit5 = 0 | No automatically recoverable error |
| Bit5 = 1 | Automatically recoverable error occurred |
| Bit6 = 0 | Printer temperature normal |
| Bit6 = 1 | Printer overheated |
| Bit7 | Undefined, fixed 0 |

#### Byte 3

| Bit | Meaning |
|-----|---------|
| Bit0–1 = 0 | Paper near-end not detected |
| Bit0–1 = 1 / `03` | Paper near-end detected |
| Bit2–3 = 0 | Paper present |
| Bit2–3 = 1 / `0C` | Paper out |
| Bit4 | Undefined, fixed 0 |
| Bit5–6 | Unused |
| Bit7 | Undefined, fixed 0 |

#### Byte 4

All relevant bits are unused or fixed 0 in the extracted manual.

### 5.2 `GS g 1` — Return print density level

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `GS g 1` |
| Hex | `1D 67 31` |

Function: return printer density level as a string value `level`.

The manual states `level` ranges from `0` to `3`, with larger numbers meaning higher density/darker print. This is a query command and may not expose the full `0..39` density range used by `ESC ## STDP n`.

### 5.3 `GS g 2` — Return print speed

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `GS g 2` |
| Hex | `1D 67 32` |

Function: return print speed as a string value `speed`.

Documented returned values include `50`, `80`, `120`, `150`, `180`, `200`, `220`. Larger values indicate faster printing. The returned value may represent a speed range rather than the exact internal speed.

### 5.7 `GS g 6` — Return printer temperature

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `GS g 6` |
| Hex | `1D 67 36` |

Function: return printer temperature as a string like:

```text
temp:d1d2
```

`d1d2` is the detected temperature in degrees Celsius.

This is relevant when debugging bitmap pause behavior and thermal throttling.

### 5.8 `GS g 7` — Return serial baud rate

Format:

| Encoding | Bytes |
|----------|-------|
| ASCII | `GS g 7` |
| Hex | `1D 67 37` |
| Decimal | `29 103 55` |

Function: return the printer's serial baud rate.

The return value is a string like:

```text
uart baudrate: d1d2
```

`d1d2` is the detected baud rate.

Implementation note: this is useful after `set_baudrate <rate>` to verify that the printer actually switched.

## Implementation notes for SToMS3R

### Commands already implemented

| Firmware command/API | Manual command(s) |
|----------------------|-------------------|
| `printer_init` | `ESC @` |
| `printer_text` | raw text + LF |
| `printer_feed` | `ESC d n` |
| `printer_size` | `GS ! n` |
| `printer_bold` | `ESC E n` |
| `printer_align` | `ESC a n` |
| `printer_barcode` | `GS k ...` |
| `printer_qr` | QR `GS ( k` sequence |
| `printer_bitmap_test` / web bitmap | `GS v 0 ...` |
| `printer_probe` | `DLE EOT n` |
| `set_baudrate` | `ESC ## SBDR baudrate` |

### Commands worth adding next

| Proposed firmware command | Manual command | Why it matters |
|---------------------------|----------------|----------------|
| `printer_density <0-39>` | `ESC ## STDP n` | Adjust darkness/current draw; useful for stripe and power tests |
| `printer_speed <n>` | `ESC ## STSP n` | Tune mechanism speed versus quality |
| `printer_temp` | `GS g 6` | Observe thermal throttling |
| `printer_get_baud` | `GS g 7` | Verify printer-side baud after `set_baudrate` |
| `printer_status4` | `GS a n` | Read buffer-full, cover, overheat, paper state |
| `printer_graphics_mode <30|31|32>` | `ESC ## SPSM n` | Try BLE/adaptive/constant-speed graphics modes |
| `printer_softflow <on|off>` | `ESC ## SFFC n` | Test whether software flow control helps when CTS is insufficient |

### Bitmap timing implications

The raster bitmap command requires the host to send exactly:

```text
8-byte GS v 0 header + (bytes_per_row * height) bytes of pixel data
```

For a full-width 384-dot image:

```text
bytes_per_row = 384 / 8 = 48
payload = 48 * height
```

At 9600 baud, a 9600-byte image takes roughly 10 seconds just to transmit over UART, before mechanical print time and CTS pauses. Raising baud rate with `ESC ## SBDR` can reduce transfer time, but it does not remove thermal or power limitations of dense black printing.
