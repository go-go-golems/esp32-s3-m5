---
title: "Diary"
tags:
  - reference
  - diary
  - esp32s3
  - atoms3r
  - thermal-printer
created: 2026-04-28
status: active
intent: long-term
---

# Diary — STOMS3R-001

## Goal

Track the design, research, and implementation of the SToMS3R firmware:
an `esp_console`-based REPL for the M5Stack AtomS3R Lite driving the K118
thermal printer over UART, with WiFi management and NVS persistence.

---

## Step 1: Ticket Creation and Design Guide

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to write a esp32s3 atom s3r lite firmware with esp_console for setting up the wifi, and a esp_console for using the m5paper printer (stoms3r for sure this time). Create a detailed analysis / design / implementation guide that is very detailed for a new intern..."

**Assistant interpretation:** Create a docmgr ticket (STOMS3R-001) and write a comprehensive intern-facing design/implementation guide covering all system layers.

**Inferred user intent:** Replace the previous ATOM Lite (ESP32-PICO-D4) printer firmware attempts with a clean ESP-IDF design on the AtomS3R Lite, leveraging USB Serial/JTAG and PSRAM. Document everything so an intern can implement it.

### What I did

- Created docmgr ticket `STOMS3R-001` with topics: esp32s3, atoms3r, thermal-printer, console, wifi, esp-idf, firmware, provisioning, escpos
- Read existing research from tickets 0090 (K118 research), 0091 (BLE provision), 0092 (ESP-IDF provision on ATOM Lite)
- Read the 0037 Cardputer fan control console as an `esp_console` reference pattern
- Read the existing `app_printer.c` UART driver from ticket 0092
- Wrote a 2000-line design and implementation guide covering 22 sections

### Why

The previous attempts (0090, 0091, 0092) used the ATOM Lite (ESP32-PICO-D4) which has no USB Serial/JTAG and no PSRAM. The AtomS3R Lite solves both problems. A comprehensive guide was needed because the intern needs to understand the full stack from hardware wiring to ESC/POS protocol to the ESP-IDF build system.

### What worked

- Reading the existing `app_printer.c` from 0092 provided the exact UART init pattern
- The 0037 fan console project provided the `esp_console` + USB Serial/JTAG pattern
- The TECHNICAL-DEEP-DIVE.md from 0090 had complete ESC/POS command documentation

### What didn't work

- N/A — pure documentation step, no code was compiled

### What I learned

- The AtomS3R Lite exposes G5/G6 on the HY2.0-4P port — perfect for UART1 TX/RX to the printer
- USB Serial/JTAG on ESP32-S3 consumes zero GPIO pins, unlike the ATOM Lite's FTDI UART bridge
- 8 MB PSRAM allows buffering full-page bitmaps (384 × 1000 = 48 KB) without heap pressure

### What was tricky to build

- Determining the correct GPIO pins for the AtomS3R Lite: the HY2.0-4P port maps to G5/G6, but G38/G39 are input-only. G7/G8 are only on the bottom header, not the HY2.0 port.

### What warrants a second pair of eyes

- The TX↔RX crossover wiring between AtomS3R Lite TX (GPIO5) and printer RX — this must be verified on the physical K118 cable pinout before testing
- The PSRAM configuration (`SPIRAM_MODE_OCT=y`) assumes the AtomS3R Lite uses octal SPI PSRAM — should be confirmed from the schematic

### What should be done in the future

- Implement the firmware (follow tasks.md phase 1 through 7)
- Verify physical wiring with the actual K118 cable
- Upload this guide to reMarkable

### Code review instructions

- Start with the design guide: `design-doc/01-stoms3r-complete-design-and-implementation-guide.md`
- Cross-reference the UART init pattern against `0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_printer.c`
- Cross-reference the console pattern against `0037-cardputer-adv-fan-control-console/main/app_main.c`

---

## Step 2: Full Firmware Implementation (Phases 1–6)

### Prompt Context

**User prompt (verbatim):** "Create tasks if not existings, then build task by task, commit at appropriate intervals, keep a diary."

**Assistant interpretation:** Implement all firmware tasks from the task list, building up from the project skeleton through all modules, verify the build, commit, and track progress.

**Inferred user intent:** Get the firmware to a compilable, flashable state so it can be tested on real hardware.

**Commit (code):** 5651b3f — "stoms3r: add complete firmware (Phase 1-6)"

### What I did

- Created the ESP-IDF project directory structure at `stoms3r/`
- Wrote `sdkconfig.defaults` (USB Serial/JTAG, PSRAM octal, 8MB flash, custom partitions)
- Wrote `partitions.csv` (nvs + phy + 4MB factory + 1MB storage)
- Wrote all 6 module pairs (`.c` + `.h`): `nvs_store`, `printer_drv`, `printer_cmd`, `wifi_mgr`, `wifi_cmd`, `app_main`
- Added `build.sh`, `README.md`, `.gitignore`
- Verified build succeeds: binary = 821 KB, 80% partition space free
- Committed all 18 files (1414 LOC)

### Why

All the module interfaces were designed in Step 1, so the implementation was straightforward. I wrote all files at once because the modules have compile-time dependencies (headers reference each other) and a partial build would just fail on missing symbols.

### What worked

- The 0037 fan console pattern translated directly to the USB Serial/JTAG REPL setup
- The 0092 `app_printer.c` UART init pattern was reusable almost verbatim (just changed pins from GPIO23/33 to GPIO5/6 and UART NUM from 2 to 1)
- `idf.py set-target esp32s3` + `idf.py build` worked on the first real attempt after fixing the argtable3 type issues

### What didn't work

- **First build had two errors:**
  1. `argtable3` type names: I used `arg_str1_t *` but ESP-IDF's argtable3 uses `struct arg_str *` (the factory functions like `arg_str1()` return `struct arg_str *`). Fixed with a sed pass.
  2. `ESP_RETURN_ON_ERROR` macro in `wifi_mgr.c` — needed `#include "esp_check.h"`. Replaced with explicit error-checking code instead.
  3. Unused variable warnings (`TAG` in `wifi_cmd.c`, `auth_mode_to_str` in `wifi_mgr.c`) — removed the dead code.

### What I learned

- ESP-IDF's argtable3 wrapper does NOT define `arg_str1_t` typedefs — use `struct arg_str *` etc.
- `ESP_RETURN_ON_ERROR` requires `esp_check.h`, not just `esp_err.h`
- The ESP-IDF build system with `-Werror=all` is strict about unused functions and variables

### What was tricky to build

- The argtable3 type mismatch was subtle — the header declares the factory functions returning `struct arg_str *`, but many online examples use `arg_str1_t` which is from the standalone argtable3, not the ESP-IDF bundled version.

### What warrants a second pair of eyes

- The QR code ESC/POS byte sequence — the 3-step GS ( k command with `pL = len + 3` encoding. Should be verified against the actual printer output.
- The `wifi_mgr_connect()` blocking wait with 15s timeout — the STA_START event handler calls `esp_wifi_connect()`, but we also call `xEventGroupWaitBits()` after `esp_wifi_set_config()`. Need to verify the timing doesn't race.

### What should be done in the future

- Flash to hardware and run smoke tests (tasks 9, 15, 17, 19, 20, 22, 23, 25)
- Verify physical wiring (TX↔RX crossover)
- Add `heap_caps_malloc` for bitmap buffers in PSRAM

### Code review instructions

- `stoms3r/main/app_main.c` — check initialization order (NVS → netif → event loop → WiFi → printer → console)
- `stoms3r/main/printer_drv.c` — verify ESC/POS command byte sequences against the K118 protocol
- `stoms3r/main/wifi_mgr.c` — verify event handler registration and auto-reconnect logic
- Build and verify: `cd stoms3r && idf.py build`

---

## Step 3: Fix printer GPIO pin mapping

### Prompt Context

**User prompt (verbatim):** "nothing happens when I use the printer commands. should we get anything back from the printer?"

**Assistant interpretation:** Printer produces no output. Need to add diagnostics to determine if bytes are even leaving the ESP32 and if the printer responds.

**Inferred user intent:** Diagnose why the printer is silent — is it a software bug, wrong GPIO pins, missing power, or a wiring issue?

### What I did

- Added hex TX logging to every `send_bytes()` call
- Added `printer_probe` command: queries 4 DLE EOT status registers, drains RX, sends ESC @, prints pass/fail verdict with troubleshooting hints
- Added `printer_raw <hex>` command: send arbitrary bytes and read back
- Added `printer_drv_query_status()`, `printer_drv_drain_rx()`, `printer_drv_send_raw()`
- User reported the K118 pinout: TX=GPIO8, RX=GPIO7, CTS=GPIO6 (not GPIO5/GPIO6 from HY2.0-4P)
- Updated all GPIO defines and wired CTS pin

### Why

The K118 was designed for the ATOM Lite. The ATOM Lite bottom header has GPIO23/GPIO33/GPIO19 at the same physical positions where the AtomS3R Lite has GPIO8/GPIO7/GPIO6. I originally assumed the HY2.0-4P port (GPIO5/GPIO6) was the correct connection, but the K118 cable actually uses the other side of the bottom header.

### What worked

- The diagnostic approach was correct — hex TX logging and DLE EOT status queries will immediately reveal whether bytes leave the ESP32 and whether the printer responds.

### What didn't work

- The original GPIO5/GPIO6 assumption was wrong. The K118 cable maps to GPIO8/GPIO7/GPIO6 on the AtomS3R Lite.

### What was tricky to build

- Mapping ATOM Lite header positions to AtomS3R Lite equivalents. The two boards have completely different GPIO assignments at the same physical header positions. The HY2.0-4P port (G5/G6) on the AtomS3R Lite is a separate connector from the bottom row header pins (G7/G8) where the K118 actually connects.

### What warrants a second pair of eyes

- The TX↔RX crossover: ESP TX=GPIO8 connects to printer RX. Verify on the actual K118 cable which pin is which.
- The CTS pin: we wired it but left flow control disabled (`UART_HW_FLOWCTRL_DISABLE`). This is fine for "fire and forget" mode, but if the printer has a small buffer and we flood it, we might lose data. Can enable `UART_HW_FLOWCTRL_CTS_RTS` later if needed.

### What should be done in the future

- Flash with corrected GPIOs and run `printer_probe` to verify printer responds
- Test actual printing with `printer_text "Hello"`
- Update the design guide with the corrected pin mapping

### Code review instructions

- Check `stoms3r/main/printer_drv.h` defines match the physical K118 cable pinout
- Check `stoms3r/main/printer_drv.c` uart_set_pin() uses PRINTER_CTS_GPIO

---

## Step 4: Bitmap stripe investigation — pause behavior and safe raster bands

### Prompt Context

**User prompt (verbatim):** "Examine the striped print with pause behaviour, use `surf kagi search` cli to search the web if necessary."

**Assistant interpretation:** The bitmap artifacts persisted or changed character: the important new observation is not merely "horizontal stripes", but visible printer pauses while the raster is printing. That points away from pure TCP chunking and toward printer pacing: thermal throttling, power sag, or input-buffer overrun while the mechanism is busy.

**Commit (code):** 41fc01c — "stoms3r: pace bitmap printing in safe raster bands"

### What I did

- Re-read the current ESP-IDF bitmap path in `stoms3r/main/web_server.c` and `stoms3r/main/printer_drv.c`.
- Re-read the original M5Stack `ATOM_PRINTER::printBMP()` implementation.
- Tried to run `surf kagi search`, but the local surf browser socket was not available: `/home/manuel/snap/chromium/common/surf-cli/surf.sock`.
- Used the available web search tool and local research docs instead.
- Confirmed from the local M5Stack docs that the K118 requires DC 12V and 2.5A, and that power supply capability directly affects print display quality.
- Checked the Adafruit thermal printer library source for a comparable small serial thermal printer pacing strategy. Its comments explicitly warn that without flow control, code must avoid overrunning the printer buffer; its bitmap path uses a 256-byte input-buffer heuristic and splits 384px-wide bitmaps into about 5-row chunks.
- Changed the firmware to print bitmaps as small complete GS v 0 raster bands: 5 rows per band, 50ms delay between bands.
- Kept the full HTTP-body buffering step, so network receive gaps still never occur inside a raster payload.
- Verified `idf.py build` succeeds.

### Why

The previous fix assumed the printer needed one uninterrupted raster payload. That remains true inside a single GS v 0 command: a pause while the printer is still waiting for pixel bytes can create artifacts or cause dropped data. But the new observation — visible pauses while printing dense bitmaps — suggests the printer itself is throttling. If we continue feeding one giant raster command at 9600 baud while the mechanism pauses, the printer's internal input buffer can overflow, or the thermal head/power supply can recover in a way that shows up as horizontal bands.

The safer pattern is to create legal pause points. A pause between two complete raster commands is valid ESC/POS behavior: the first band has already ended, the printer advances by the band's height, and the next GS v 0 command begins at the next line. This is different from a pause inside the pixel payload of one large image.

### What worked

- The original M5Stack firmware confirms that the web upload is buffered before print, not streamed directly from HTTP into the printer.
- The K118 documentation confirms that inadequate 12V supply current directly affects print quality.
- The Adafruit thermal printer library gives a useful heuristic: if you cannot trust flow control, keep bitmap chunks near 256 bytes. For 384px output, one row is 48 bytes, so 5 rows is 240 bytes.
- The new `printer_drv_print_bitmap_banded()` helper makes this explicit and reusable.

### What didn't work

- `surf kagi search` could not run because the surf socket was missing. This was an environment issue, not a search-query issue.
- Treating all stripes as "UART gaps" was too narrow once the printer was visibly pausing. The pause behavior changes the diagnosis.

### What I learned

- There are two different kinds of pauses:
  - A pause inside one raster command's pixel data is dangerous.
  - A pause between complete raster commands is legal and useful.
- Dense images, especially full-black and high-density test patterns, are power and thermal stress tests. They should not be interpreted the same way as normal dithered photos.
- The CTS pin was already wired in the firmware but flow control is still disabled. Banded output is the safer immediate test because it does not depend on CTS polarity or whether the printer actually asserts the line.

### What was tricky to build

- The fix appears to contradict the previous "continuous stream" fix, but it does not. The invariant is more precise now: do not let arbitrary network or scheduler gaps occur inside the payload of a raster command. Splitting the image into small complete raster commands preserves that invariant while adding legal cooling/buffer points.

### What warrants a second pair of eyes

- Verify that consecutive GS v 0 raster commands stack without visible seam on the K118. ESC/POS says the print position returns to the beginning of the next line after a raster image, so this should work, but the K118's exact dialect should be tested.
- Verify whether GPIO6 CTS actually tracks printer busy. If it does, a future firmware version should add an optional `printer_flow cts` mode or enable `UART_HW_FLOWCTRL_CTS` by default.
- Check the power supply: the K118 should use a 12V adapter rated at 2.5A or higher, with short leads.

### What should be done in the future

- Flash commit `41fc01c` and re-run the bitmap test patterns.
- Start with low-density patterns (`border`, `diagonal`, `fullwhite`) to verify geometry.
- Then test `graylevels` and observe exactly where pauses begin.
- Treat `fullblack` as a power/thermal stress test, not as a normal expected print workload.
- If banding still shows stripes, test CTS flow control next.

### Code review instructions

- Review `printer_drv_print_bitmap_banded()` in `stoms3r/main/printer_drv.c`.
- Confirm the default band size: 5 rows × 48 bytes/row = 240 data bytes.
- Confirm `web_server.c` still buffers the full request body before calling the printer driver.
- Build with: `cd stoms3r && idf.py build`.

---

## Step 5: CTS flow control after band seams

### Prompt Context

**User prompt (verbatim):** "use cts. theyre twice as freqyent now, regular"

**Assistant interpretation:** The banded raster experiment introduced a new regular artifact: stripes became twice as frequent and evenly spaced. That confirms the bands themselves were visible seams, not the right final strategy. The next experiment should use the printer's CTS/busy signal and return to one complete raster command.

**Commit (code):** dac86e8 — "stoms3r: use printer CTS for bitmap flow control"

### What I did

- Enabled UART TX flow control with `.flow_ctrl = UART_HW_FLOWCTRL_CTS`.
- Kept `uart_set_pin(..., rts=UART_PIN_NO_CHANGE, cts=GPIO6)` so GPIO6 is used as the printer busy/CTS input.
- Restored the default bitmap print path to a single complete GS v 0 raster command (`header + full pixel payload`).
- Left the banded bitmap helper in the code only as a diagnostic fallback, not as the default path.
- Updated `web_server.c` to call `printer_drv_print_bitmap()` again instead of the 5-row banded helper.
- Changed `uart_wait_tx_done()` to use a length-based timeout. At 9600 baud, bitmap payloads take many seconds even without CTS pauses.
- Verified `idf.py build` succeeds.

### Why

The regular stripe spacing after the banding change is strong evidence that the band boundaries were becoming visible. That means the correct fix is not artificial application-layer delays between raster commands. The printer already exposes a hardware busy signal. If CTS works, the UART peripheral can stop transmitting exactly when the printer says it is not ready, then resume without dropping bytes and without inserting fake raster seams.

### What should be tested next

- Flash commit `dac86e8`.
- Test `border`, `diagonal`, and `graylevels` first.
- If printing hangs immediately, CTS polarity or wiring may be inverted/wrong, and we should add a runtime `printer_flow on|off` command.
- If it prints but still stripes, the remaining cause is likely power/thermal behavior rather than byte loss; verify 12V/2.5A+ supply.

---

## Step 6: K118 baud-rate command

### Prompt Context

**User prompt (verbatim):** "add a set_baudrate command."

**Assistant interpretation:** The existing `printer_baud` command only changed the ESP32 UART baud rate. The user wanted a command that sends the K118 printer-side baud-rate command and then switches the ESP32 side to match.

**Commit (code):** d966764 — "stoms3r: add K118 set_baudrate console command"

### What I did

- Added `printer_drv_set_printer_baudrate(int baud)`.
- Implemented the K118 documented command format: `1B 23 23 53 42 44 52 <baud little-endian uint32>`.
- Added console command `set_baudrate <rate>`.
- Kept `printer_baud <rate>` as an ESP32-UART-only recovery command.
- Limited both commands to common rates: 9600, 19200, 38400, 57600, and 115200.
- Updated the README with the corrected K118 header pins and new commands.
- Verified `idf.py build` succeeds.

### Why

Changing the ESP32 UART speed alone does not change the printer. The printer must receive its own baud-rate command at the current working speed. Only after that command has fully left the UART should the ESP32 switch its local UART peripheral to the new rate.

### Recovery note

If `set_baudrate` leaves the printer and ESP32 out of sync, use `printer_baud <rate>` to change only the ESP32 side, or power-cycle the printer to return it to the default 9600 baud.

---

## Step 7: Experimental high baud-rate steps

### Prompt Context

**User prompt (verbatim):** "can we go faster 2-3 steps"

**Assistant interpretation:** Extend the new `set_baudrate` command beyond the documented/common 115200 target so bitmap transfer speed can be tested at higher UART rates.

**Commit (code):** e6fb23a — "stoms3r: allow experimental high printer baud rates"

### What I did

- Added 230400, 460800, and 921600 to the accepted baud-rate list.
- Kept 9600, 19200, 38400, 57600, and 115200.
- Added console warnings for rates above 115200 because the K118 docs explicitly show only 9600 and 115200 examples.
- Updated README and driver comments to mark higher rates as experimental.
- Verified `idf.py build` succeeds.

### Why

The K118 Set Baud Rate command encodes the requested baud as a 32-bit little-endian integer. That makes higher standard UART rates plausible even though only two examples are documented. Testing them incrementally is the safest path.

### Recommended test ladder

1. `set_baudrate 115200`
2. `printer_probe`
3. `set_baudrate 230400`
4. `printer_probe`
5. `set_baudrate 460800`
6. `printer_probe`
7. Try `921600` only if 460800 is stable.

If communication is lost, use `printer_baud <rate>` to resync the ESP32 side or power-cycle the printer to return it to 9600.
