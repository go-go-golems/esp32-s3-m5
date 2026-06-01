---
Title: Investigation diary
Ticket: ESP32-P4-PICOCALC
Status: active
Topics:
    - esp32-p4
    - picocalc
    - hardware
    - firmware-port
    - waveshare
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0098-esp32-p4-wifi6-webserver/README.md
      Note: Operator docs for persistent Wi-Fi credentials (commit 84dd320)
    - Path: 0098-esp32-p4-wifi6-webserver/main/app_main.c
      Note: NVS-backed Wi-Fi credential persistence and console commands (commit 84dd320)
ExternalSources: []
Summary: Chronological diary of the ESP32-P4-WIFI6 PicoCalc replacement investigation
LastUpdated: 2026-06-01T17:30:00-04:00
WhatFor: Track investigation progress, findings, and next steps
WhenToUse: Resume work on this ticket or understand how conclusions were reached
---


# Diary

## Goal

Document the investigation of using a Waveshare ESP32-P4-WIFI6 board as a replacement for the RP2350 Pico in the ClockworkPi PicoCalc, including feasibility analysis, pin mapping, power analysis, and firmware migration planning.

## Step 1: Ticket Creation and Initial Research

Gathered the ChatGPT conversation transcript that analyzed ESP32-P4 feasibility for PicoCalc (focused on M5Stack Stamp-P4 but applicable to the Waveshare board), searched for Waveshare ESP32-P4-WIFI6 documentation and pinouts, and collected the complete PicoCalc hardware specification from the PiPAPo project.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket where I want to actually use a waveshare esp32-p4-wifi6 board to use as a replacement for the rpico 2350 i'm using for my clockworkpi (see firmwares I've been building for now /home/manuel/code/wesen/2026-05-05--ulisp-picocalc). Potentially use `surf chat --help` to gather the information from : https://chatgpt.com/c/6a1753cb-5580-83ea-a636-5b375b68f5a0 but also if necessary do your own research, and save important resources in the sources folder of the ticket using defuddle. Keep a diary as you work, don't work outside of this folder except for reading."

**Assistant interpretation:** Create a docmgr ticket for investigating the Waveshare ESP32-P4-WIFI6 as a PicoCalc MCU replacement, pull the ChatGPT conversation for context, do independent web research, save sources, and keep a diary.

**Inferred user intent:** The user wants to move from RP2350 to ESP32-P4-WIFI6 on their PicoCalc project, and needs a structured research package with pin mapping, feasibility analysis, and firmware migration plan before committing to hardware design.

### What I did

- Created docmgr ticket `ESP32-P4-PICOCALC` with design-doc and diary
- Extracted ChatGPT conversation transcript using `surf chatgpt transcript` (after user opened the correct conversation in Chromium)
- Searched for Waveshare ESP32-P4-WIFI6 board specs, pinouts, and datasheets via Kagi
- Defuddled/saved key reference pages to `sources/` folder
- Downloaded ESP32-P4 datasheet PDF and Waveshare module schematic PDF
- Found the best-in-class pinout document: the `adsb-p4` project's `board_pinout.md` on GitHub (defuddled successfully)
- Downloaded the PiPAPo PicoCalc hardware spec (complete pin map + southbridge protocol)
- Downloaded PicoCalc LCD spec (ST7365P init sequence)
- Read existing `pico-sdk-picocalc-wm` firmware source to understand current architecture
- Wrote the primary design document with full pin mapping, power analysis, and firmware migration plan

### Why

The ChatGPT transcript provided a good starting analysis but was focused on the M5Stack Stamp-P4, not the Waveshare ESP32-P4-WIFI6. I needed Waveshare-specific pinout information and had to cross-reference the PicoCalc's pin map with the Waveshare board's available GPIOs.

### What worked

- `surf chatgpt transcript` worked perfectly after user opened the right conversation in Chromium
- The `adsb-p4` project on GitHub (by nullbeing) provided the most detailed and accurate Waveshare ESP32-P4-WIFI6 pinout document available anywhere — a complete cross-referenced table of all 55 GPIOs, on-board peripheral allocations, and free-pool status
- The PiPAPo `picocalc.md` reference provided the complete PicoCalc pin map and southbridge I²C protocol in one place
- Defuddle worked well for GitHub markdown content but failed for JS-heavy Waveshare product pages (returned empty)

### What didn't work

- `surf defuddle` is not a valid command — defuddle is a separate CLI (`defuddle parse <url>`)
- Defuddle returned empty content (1-line files) for Waveshare docs pages and the ClockworkPi forum — these sites are JS-rendered SPAs
- Playwright browser couldn't access the ChatGPT conversation (not logged in) — had to ask the user to open it in Chromium first
- Waveshare documentation pages have pinout tables as images, not text — had to rely on the GitHub `board_pinout.md` for the actual pin data

### What I learned

- The Waveshare ESP32-P4-WIFI6 has **only 9 truly free GPIOs** (GPIO27, 32, 33, 46, 47, 48, 49, 51, 52) after accounting for all on-board peripherals — this is tight for PicoCalc's needs
- Another 5 GPIOs (GPIO2–4, GPIO24–25) are available but conflict with JTAG/USB-JTAG
- The PicoCalc keyboard's 10 kHz I²C speed is a potential problem when sharing the bus with the ES8311 codec and BNO085 IMU on the Waveshare board
- The onboard SDMMC slot (4-bit SDIO) is much faster than PicoCalc's SPI-mode SD, making it the preferred option for first bring-up
- ESP32-P4 Arduino support is still preliminary — ESP-IDF is the only viable development framework right now
- The right-header GND between GPIO46 and GPIO47 is a known trap — multi-pin Dupont housings will short to ground
- ESP-IDF now requires ESP32-P4 chip revision ≥ 3.1; early boards with v1.3 silicon may not work with current ESP-IDF

### What was tricky to build

- The pin mapping required cross-referencing three separate sources (PicoCalc spec, Waveshare module pinout, adsb-p4 board pinout) because no single document covers both sides. The Waveshare board's on-board peripheral commitments are not documented in the official Waveshare docs — they had to be extracted from the adsb-p4 project's reverse-engineered pinout.
- The I²C bus sharing decision is subtle: the PicoCalc keyboard runs at 10 kHz (unusually slow), the Waveshare's on-board codec/IMU expect standard I²C speeds. Whether they can coexist on the same bus at mixed speeds is unclear and needs hardware testing.

### What warrants a second pair of eyes

- **Power budget analysis**: I estimated 400–500 mA peak for the ESP32-P4-WIFI6. This needs verification against the PicoCalc's actual VSYS current budget. If VSYS can't supply this, the adapter board needs a buck-boost regulator, which changes the PCB layout significantly.
- **I²C bus sharing at 10 kHz**: This is the riskiest electrical decision. If the ES8311 or BNO085 has minimum I²C clock requirements above 10 kHz, the shared bus won't work and we need a second I²C bus on different GPIOs (consuming 2 more free GPIOs, leaving only 7 free).
- **Chip revision**: If the Waveshare board ships with ESP32-P4 v1.3 silicon, current ESP-IDF won't flash it. Need to verify the chip revision before buying.
- **SD card choice**: I recommended using the Waveshare onboard SD slot only (Option A) for first bring-up. But if the PicoCalc's external SD slot is important for the user experience (e.g., loading .uf2 firmware from SD), Option B needs to be evaluated early.

### What should be done in the future

- Verify ESP32-P4 chip revision on the actual Waveshare board (run `esptool.py chip_id` or check the chip marking)
- Measure PicoCalc VSYS voltage and current capacity under all power states
- Test I²C bus sharing with the keyboard southbridge at 10 kHz alongside the ES8311 codec
- Prototype the LCD SPI2 driver on the Waveshare board (Phase 2 of firmware migration)
- Design the interposer PCB in KiCad once the pin mapping is validated
- Check physical dimensions of the Waveshare board against the PicoCalc case interior

### Code review instructions

- The primary design doc is at `design-doc/01-esp32-p4-picocalc-adapter-design.md` — review the pin mapping tables for correctness against the source documents
- Key source to verify: `sources/esp32-p4-waveshare-board-pinout.md` — this is the ground truth for the Waveshare board's GPIO allocations
- Key source to verify: `sources/picocalc-hardware-spec-pipapo.md` — this is the ground truth for PicoCalc's peripheral connections
- Cross-check the "Proposed Pin Mapping" table against both sources

### Technical details

- surf CLI: `surf chatgpt transcript --export-file <path>` extracts the current ChatGPT conversation from the active Chromium tab
- defuddle CLI: `defuddle parse <url>` extracts article content from web pages (works for static/SSR pages, fails for JS-rendered SPAs)
- docmgr CLI: standard ticket/doc/relate workflow
- ESP32-P4 SPI2 IO-MUX direct pins: GPIO28 (CS), GPIO29 (MOSI/D), GPIO30 (SCK), GPIO31 (Q/MISO) — these are the fastest SPI path, bypassing the GPIO matrix entirely

## Step 2: Corrected GPIO availability — board has 25 header GPIOs, not 9

The initial analysis incorrectly stated only 9 free GPIOs. That count came from the adsb-p4 project's pinout document, which listed only the GPIOs that were *unallocated by that specific project* — not the GPIOs that are inherently unavailable on the board. The board itself exposes 25 GPIOs on the two 2×20 headers, and all 25 are available for our PicoCalc project (some with JTAG/I²C/USB caveats, but none are hard-committed to board peripherals on the headers).

### Prompt Context

**User prompt (verbatim):** "hold on, this is the pinout, plenty more GPIOs there: /tmp/pi-clipboard-11d6aa54-a75f-4c33-9202-dc258e69d832.png"

**Assistant interpretation:** The user corrected my GPIO count — the pinout diagram shows 25 header GPIOs, not 9.

**Inferred user intent:** Ensure the design doc accurately reflects the board's actual GPIO availability so the pin mapping and feasibility assessment are correct.

### What I did

- Read the pinout image the user shared — confirmed 25 GPIOs on the headers
- Re-examined the adsb-p4 board_pinout.md to distinguish board-internal commitments from project-specific allocations
- Updated the design doc: replaced the "9 free GPIOs" section with a complete listing of all 25 header GPIOs, categorized by caveat level
- Corrected SD card Option B text (no longer says "5 of 9 free GPIOs")

### Why

The adsb-p4 project's "Current allocation" column confused project-specific wiring (LCD, IMU, buttons) with board-inherent constraints. The on-board peripherals (C6 SDIO, I²S codec, SDMMC, CH343P) are all on internal traces using GPIOs that aren't even on the headers. Every header GPIO is available.

### What worked

- The user caught the error immediately by sharing the pinout diagram
- The corrected 25-GPIO budget is much healthier — plenty of room for all PicoCalc peripherals plus extras

### What didn't work

- I should have read the adsb-p4 document more carefully — section 2 ("On-board peripherals") clearly states these are internal traces, and section 3 ("Current add-on peripherals") is labeled as user-wired, not board-committed

### What I learned

- The Waveshare ESP32-P4-WIFI6 has an excellent GPIO budget: 25 on headers, 18 truly free (no caveats), 2 with I²C sharing, 5 with JTAG/USB defaults
- The right-header GND between GPIO46 and GPIO47 is a real physical trap for multi-pin Dupont housings
- The I²C0 bus (GPIO7/GPIO8) shares with the on-board ES8311 codec — this is a board design fact, not a project choice

### What was tricky to build

- Separating "board-committed" from "project-committed" GPIOs required careful reading of the adsb-p4 document's section structure. The "Current allocation" column in the header tables mixes both categories.

### What warrants a second pair of eyes

- The corrected GPIO count and categories in the design doc
- Whether GPIO2–4 (JTAG defaults) and GPIO24–25 (USB Serial/JTAG defaults) should be counted as truly available or reserved for debugging

### What should be done in the future

- Verify the pinout image against the physical board when it arrives
- Decide on JTAG/USB-JTAG strategy: if we commit GPIO2–4 and GPIO24–25 to PicoCalc peripherals, we lose USB JTAG debugging

### Code review instructions

- Review the "All 25 header GPIOs" table in the design doc for accuracy against the pinout image

### Technical details

- 18 truly free GPIOs: GPIO2–5, GPIO20–23, GPIO26–27, GPIO32–33, GPIO46–49, GPIO51–52
- 2 I²C-shared: GPIO7 (SDA), GPIO8 (SCL) — ES8311 codec at 0x18 always on bus
- 5 JTAG/USB-default: GPIO2–4 (JTAG), GPIO24–25 (USB Serial/JTAG)
- 0 hard-committed on headers

## Step 3: Corrected serial-console understanding and successful UART0 capture

The serial-console failure was not an ESP32-P4 v1.3 support failure. It was a console-routing mistake plus a stale process holding the port. The board enumerates as a WCH/QinHeng CH343-family USB CDC serial bridge (`1a86:55d3`) on `/dev/ttyACM1`, and that bridge is wired to ESP32-P4 UART0 on GPIO37/GPIO38. ESP-IDF's `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` is the correct console choice for this board because ESP32-P4 UART0's direct IO-MUX pins are GPIO37 TX and GPIO38 RX.

A controlled single-owner capture confirmed that the ROM, second-stage bootloader, PSRAM initialization, and the application logs all appear on `/dev/ttyACM1` at 115200 baud after toggling EN via RTS. The earlier `cat /dev/ttyACM1` test was inconclusive because it started after the boot logs had already been emitted and then remained running as a stale port holder.

### Prompt Context

**User prompt (verbatim):** "ok we're taking over for your little brother who is struggling with idf.py. Analyze what they did so far, where we are at, and what we should do next to properly get a serial going. I don't really trust their understanding of the serial setup on this board or in esp-idf in general, so that might be worth analyzing."

**Assistant interpretation:** Re-evaluate the ESP32-P4-WIFI6 serial setup from first principles, identify what was misunderstood, and determine the correct procedure for obtaining reliable console output.

**Inferred user intent:** The user wants a reliable handoff and a trustworthy serial/debugging baseline before continuing firmware bring-up.

**Commit (code):** 432aadd — "0097: add ESP32-P4 PicoCalc bringup firmware"

### What I did

- Inspected `sdkconfig` and `sdkconfig.defaults` for console settings.
- Verified that the active project is configured for `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`, `CONFIG_ESP_CONSOLE_UART_NUM=0`, and `CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200`.
- Checked ESP-IDF v5.4.2's ESP32-P4 UART pin definitions: UART0 TX is GPIO37 and UART0 RX is GPIO38.
- Confirmed `/dev/ttyACM1` is the CH343-family bridge (`/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`).
- Found and killed a stale `cat` process that was still holding `/dev/ttyACM1`.
- Ran a single-owner Python/pyserial capture that opened `/dev/ttyACM1`, set 115200 baud, deasserted IO0 via DTR, toggled EN through RTS, and captured eight seconds of boot output.

### Why

The earlier reasoning mixed together two different console paths: native USB Serial/JTAG on ESP32-P4 GPIO24/GPIO25, and the board's USB-to-UART bridge connected to UART0 GPIO37/GPIO38. The visible Linux device was not Espressif USB Serial/JTAG; it was the CH343 bridge. Therefore selecting USB Serial/JTAG in ESP-IDF moved application logs away from the only connected USB serial path.

### What worked

- `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` is correct for the Waveshare ESP32-P4-WIFI6 board's USB-C serial path.
- ESP-IDF's ESP32-P4 UART0 defaults match the board wiring: TX GPIO37 and RX GPIO38.
- The controlled capture produced complete boot and application output.
- PSRAM was detected as 32 MB at 200 MHz and passed the ESP-IDF memory test.
- The application reached `app_main()`, printed system info, passed the 1 MB PSRAM write/read test, and started the GPIO49 blink task.

### What didn't work

- `idf.py monitor` cannot be run from a non-TTY shell; it failed with `Monitor requires standard input to be attached to TTY`.
- A background `cat /dev/ttyACM1` process remained alive and held the serial port, causing port-busy failures.
- A raw `cat` opened after flash did not show output because the boot/application logs had already been printed by the time the reader attached.

### What I learned

- The visible USB device is `1a86:55d3 QinHeng Electronics USB Single Serial`, not an Espressif USB Serial/JTAG device.
- On this board, `/dev/ttyACM1` is the CH343 bridge to UART0, not the ESP32-P4 native USB console.
- Boot ROM logs are printed on UART0. When ESP-IDF was configured for USB Serial/JTAG, the boot ROM lines could still appear on `/dev/ttyACM1`, while later ESP-IDF application logs disappeared because they were routed to native USB Serial/JTAG instead.
- Successful flash over `/dev/ttyACM1` also confirms the CH343 bridge and UART0 path are physically working.

### What was tricky to build

The confusing part was that `/dev/ttyACM1` is a CDC ACM device, which can look like native USB CDC/JTAG at first glance. The vendor ID exposed the truth: `1a86:55d3` is QinHeng/WCH, so the device is an external USB-to-UART bridge. Once that was known, the ESP-IDF configuration became straightforward: use UART0 console, not USB Serial/JTAG console.

### What warrants a second pair of eyes

- The project comments still mention USB Serial/JTAG in places and should be cleaned up so future readers do not repeat the mistake.
- The board documentation should be checked to confirm whether GPIO24/GPIO25 are merely header pins or also routed to a native USB connector/receptacle path not currently connected.
- The right reset sequence should be documented for both automated scripts and interactive tmux monitoring.

### What should be done in the future

- Keep `/dev/ttyACM1` single-owner during flash/probe/monitor work.
- Use `lsof /dev/ttyACM1` before every flash or capture attempt.
- Use `idf.py monitor` only inside a real TTY/tmux pane, or use the pyserial reset-and-capture script for non-interactive probes.
- Update the README and source comments to say `CH343 UART0 console`, not `USB Serial/JTAG console`.

### Code review instructions

- Review `0097-esp32-p4-picocalc-bringup/sdkconfig.defaults` first; the key setting is `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`.
- Review ESP-IDF v5.4.2's `components/soc/esp32p4/include/soc/uart_channel.h` to verify UART0 default pins.
- Validate serial output with a single-owner reset capture before using `idf.py monitor`.

### Technical details

Controlled capture command shape:

```python
import serial, time
ser = serial.Serial('/dev/ttyACM1', 115200, timeout=0.05)
ser.reset_input_buffer()
ser.dtr = False  # IO0 high
ser.rts = True   # EN low
sleep(0.12)
ser.rts = False  # EN high, boot app
# read for several seconds
```

Key confirmed output:

```text
I (25) boot: ESP-IDF v5.4.2 2nd stage bootloader
I (27) boot: chip revision: v1.3
I (376) esp_psram: Found 32MB PSRAM device
I (377) esp_psram: Speed: 200MHz
I (1466) main_task: Calling app_main()
I (1466) bringup: Booting...
I (1496) bringup: PSRAM: 32768 KB total, 32765 KB free
I (1576) bringup: PSRAM: 1MB write/read test PASSED
I (1576) bringup: blink: starting on GPIO49
I (1586) bringup: Phase 1 bring-up complete. LED blinking on GPIO49.
```


## Step 4: ESP-Hosted networking experiment with STA webserver and esp_console

With the PicoCalc hardware not yet connected, the next useful milestone was to exercise the Waveshare board's network path. This matters because ESP32-P4 itself has no native Wi-Fi radio; networking on this board depends on the onboard ESP32-C6 module and the ESP-Hosted/`esp_wifi_remote` SDIO stack. A simple HTTP server is a compact end-to-end test: it proves that the P4 can boot a larger app, initialize the C6 transport, join Wi-Fi, receive an IP address, and serve TCP traffic through lwIP.

I created a new firmware project, `0098-esp32-p4-wifi6-webserver`, rather than modifying the validated `0097` bring-up app. The new app uses the CH343 UART0 console, defaults to STA credentials `yolobolo` / `bring3248camera`, starts an `esp_console` REPL for Wi-Fi inspection, and serves `/`, `/status`, and `/api/ping` once the board gets a LAN IP.

### Prompt Context

**User prompt (verbatim):** "We don't yet have the picocalc connected, so let's work on some networking experiments. Let's create a simple webserver app on the device."

**User prompt (verbatim, follow-up):** "keep a detailed diary as you work, commit at appropriate intervals (including ticket stuff in ttmp)"

**User prompt (verbatim, correction):** "are you using esp_console and stuff to set up wifi? default ssid / password should be yolobolo / bring3248camera , as well."

**Assistant interpretation:** Build a separate ESP-IDF networking experiment for the Waveshare ESP32-P4-WIFI6 board, use the onboard C6 via ESP-Hosted, expose a simple HTTP server, include an interactive console for Wi-Fi control, and document/commit the work as it progresses.

**Inferred user intent:** The user wants to validate networking independently before connecting PicoCalc peripherals, while preserving a detailed record of technical decisions, failures, and hardware-specific ESP-Hosted setup.

**Commit (code):** c71c485 — "0098: add ESP32-P4 WiFi webserver experiment"

### What I did

- Created `0098-esp32-p4-wifi6-webserver/` as a new ESP-IDF project.
- Added managed component dependencies:
  - `espressif/esp_hosted: 1.4.0`
  - `espressif/esp_wifi_remote: 0.8.5`
- Added a 3 MB factory app partition table so the ESP-Hosted/HTTP firmware has enough room while still staying below the known 16 MB flash-access warning boundary.
- Configured the CH343 UART0 console path:
  - `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`
  - `CONFIG_ESP_CONSOLE_SECONDARY_NONE=y`
  - native USB Serial/JTAG disabled.
- Configured Waveshare-specific C6 SDIO pins, not Tab5 pins:
  - CLK GPIO18
  - CMD GPIO19
  - D0 GPIO14
  - D1 GPIO15
  - D2 GPIO16
  - D3 GPIO17
  - C6 reset/EN GPIO54, active-high from P4 firmware view.
- Implemented STA Wi-Fi with default credentials `yolobolo` / `bring3248camera`.
- Implemented `esp_console` over UART with commands:
  - `wifi status`
  - `wifi scan`
  - `wifi reconnect`
  - `wifi set <ssid> [password]`
- Implemented HTTP routes:
  - `GET /`
  - `GET /status`
  - `GET /api/ping`
- Built the firmware successfully with ESP-IDF v5.4.2.
- Flashed the firmware to `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- Captured boot logs with a single-owner pyserial reset/capture script.
- Verified HTTP from the host with `curl`:
  - `curl http://192.168.0.88/api/ping`
  - `curl http://192.168.0.88/status | python3 -m json.tool`

### Why

Networking is a separate risk from the PicoCalc adapter. The LCD, keyboard, SD, and audio work will depend on header wiring and the eventual adapter board, but Wi-Fi can be tested now because the Waveshare board already contains the ESP32-C6 radio. This gives an early answer to whether ESP-Hosted is configured correctly for this board.

The initial SoftAP-only direction was too limited for the user's intended workflow. STA mode with default credentials and an `esp_console` REPL is more useful: the board joins the existing LAN, prints a URL, and can be controlled through the same serial port used for flashing and logs.

### What worked

- The managed `esp_hosted` and `esp_wifi_remote` dependencies resolved through the ESP-IDF component manager.
- The final build succeeded:
  - binary size `0xaced0` bytes
  - factory app partition `0x300000` bytes
  - `0x253130` bytes free.
- Flashing succeeded over the CH343 UART bridge.
- ESP-Hosted initialized on the Waveshare SDIO wiring:

```text
I (2814) sdio_wrapper: SDIO master: Data-Lines: 4-bit Freq(KHz)[40000 KHz]
I (2814) sdio_wrapper: GPIOs: CLK[18] CMD[19] D0[14] D1[15] D2[16] D3[17] Slave_Reset[54]
I (2924) transport: Received INIT event from ESP32 peripheral
I (2944) transport:    * WLAN
```

- The application started the HTTP server and `esp_console` REPL:

```text
I (4004) p4_web: starting HTTP server on port 80
Type 'help' to get the list of commands.
p4web>
I (5014) p4_web: console ready: try 'help' or 'wifi status'
```

- The board eventually joined Wi-Fi and obtained a LAN address:

```text
I (24674) p4_web: STA connected: ssid=yolobolo channel=1 authmode=3
I (25694) esp_netif_handlers: sta ip: 192.168.0.88, mask: 255.255.255.0, gw: 192.168.0.1
I (25694) p4_web: Browse:  http://192.168.0.88/
I (25704) p4_web: Status:  http://192.168.0.88/status
```

- HTTP responses worked from the host:

```text
$ curl -sS --max-time 5 http://192.168.0.88/api/ping
{"ok":true,"message":"pong"}
```

```json
{
    "ok": true,
    "project": "0098-esp32-p4-wifi6-webserver",
    "uptime_ms": 40280,
    "chip": {
        "target": "esp32p4",
        "revision": 103,
        "cores": 2
    },
    "flash": {
        "bytes": 33554432
    },
    "heap": {
        "internal_free": 494799,
        "psram_total": 33554432,
        "psram_free": 33549744
    },
    "wifi": {
        "mode": "sta",
        "state": "got_ip",
        "ssid": "yolobolo",
        "ip": "192.168.0.88",
        "retries": 0,
        "last_disconnect_reason": -1
    }
}
```

### What didn't work

- The first CMake attempt failed because I listed `esp_flash` as a component requirement. ESP-IDF exposes `esp_flash.h`, but the component name is `spi_flash`.

```text
Failed to resolve component 'esp_flash' required by component 'main': unknown name.
```

- The first compile attempt failed because the code used `MACSTR`/`MAC2STR` without including the right header. Rather than keep AP-client logging in the STA-focused app, I removed the SoftAP path and then included the correct networking/logging headers in the final implementation.
- The initial app direction used SoftAP-first networking. The user corrected the requirement: use `esp_console` for Wi-Fi setup/debugging and default to `yolobolo` / `bring3248camera`.
- Association took multiple retries before success. The log showed transient disconnect reasons `2` and `205` before the C6 connected. This may be normal during ESP-Hosted bring-up, but it warrants watching in later tests.

### What I learned

- The Waveshare ESP32-P4-WIFI6 C6 wiring in the pinout source is correct for ESP-Hosted SDIO. The runtime logs confirmed the configured pins exactly.
- GPIO54 active-high reset is the right configuration for this board. This is different from the Tab5 examples, which use different pins and active-low reset.
- ESP-Hosted makes the application-level Wi-Fi code look like normal ESP-IDF `esp_wifi_*` code, but the boot log remains the best evidence that the remote transport is actually working.
- `esp_console` works on the CH343 UART backend. The terminal used by the pyserial capture does not support line editing escape probes, so the REPL disables line editing/history in that non-interactive capture. In a real tmux/terminal `idf.py monitor` session it should be more usable.

### What was tricky to build

The main trap was distinguishing three similar but different P4 networking examples in the workspace. Tab5 examples already use ESP-Hosted, but their SDIO pinout and reset polarity are not portable to the Waveshare board. Native ESP32-S3 examples use ordinary `esp_wifi`, but that mental model is incomplete for ESP32-P4 because there is no local radio. The correct implementation combines normal `esp_wifi_*` application calls with Waveshare-specific `esp_hosted` transport configuration.

Another subtle point is component naming. ESP-IDF header names and component names do not always match. `esp_flash.h` is used in source, but the build-system component is `spi_flash`; using `esp_flash` in `PRIV_REQUIRES` fails at CMake dependency resolution.

### What warrants a second pair of eyes

- Whether the transient disconnect reasons `2` and `205` are expected during first association through ESP-Hosted or indicate a timing/config issue.
- Whether the first networking app should persist console-provided credentials to NVS now, or whether runtime-only credentials are acceptable for this early experiment.
- Whether the 40 MHz SDIO clock is the right default for this Waveshare C6 path in all power conditions.
- Whether the current 3 MB app partition is sufficient once richer web UI assets or OTA support are added.

### What should be done in the future

- Add persistent Wi-Fi credential storage through NVS.
- Add an HTTP endpoint that reports ESP-Hosted transport counters or C6 firmware version if available through the API.
- Run a longer stability test: repeated HTTP requests for 10–30 minutes while watching disconnect/reconnect behavior.
- Add a small benchmark endpoint or reuse the 0094 benchmark server once the basic webserver is stable.
- Keep the project under the 16 MB flash boundary until the >16 MB flash warning is resolved.

### Code review instructions

- Start at `0098-esp32-p4-wifi6-webserver/sdkconfig.defaults` and verify the console backend plus Waveshare C6 SDIO pins.
- Then review `0098-esp32-p4-wifi6-webserver/main/app_main.c`, especially `start_wifi_sta()`, the Wi-Fi/IP event handler, `start_console()`, and the `/status` handler.
- Build with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
```

- Flash and validate with:

```bash
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
lsof "$PORT" || true
idf.py -p "$PORT" flash
idf.py -p "$PORT" monitor
```

- After the log prints the Browse URL, validate with:

```bash
curl -sS http://<device-ip>/api/ping
curl -sS http://<device-ip>/status | python3 -m json.tool
```

### Technical details

The confirmed webserver runtime address for this run was:

```text
http://192.168.0.88/
http://192.168.0.88/status
http://192.168.0.88/api/ping
```

The app's default credentials are compiled constants:

```c
#define DEFAULT_WIFI_SSID      "yolobolo"
#define DEFAULT_WIFI_PASSWORD  "bring3248camera"
```

The ESP-Hosted managed component versions are pinned in `main/idf_component.yml`:

```yaml
dependencies:
  idf: '>=5.4'
  espressif/esp_hosted: 1.4.0
  espressif/esp_wifi_remote: 0.8.5
```


## Step 5: Persist Wi-Fi credentials in NVS

The first networking firmware proved that ESP-Hosted, STA mode, `esp_console`, and the HTTP server work together on the Waveshare ESP32-P4-WIFI6 board. The next missing piece was persistence: without NVS-backed credentials, every non-default network change would disappear on reset. This step added a small credential store while keeping the first experiment simple and operator-driven.

The implementation now loads credentials from NVS namespace `wifi` during boot, falls back to the compiled `yolobolo` / `bring3248camera` defaults when no saved credentials exist, and exposes console commands to save or clear credentials. The HTTP `/status` endpoint reports whether the active credentials were loaded/saved, so persistence can be verified without attaching an interactive console.

### Prompt Context

**User prompt (verbatim):** "commit, update diary if necessary, add credential persistent"

**Assistant interpretation:** Commit the current work, ensure the ticket diary is updated, and add persistent Wi-Fi credential storage to the webserver firmware.

**Inferred user intent:** The user wants the networking experiment to become a practical reusable firmware, not only a one-off hard-coded credential test, while preserving a reliable chronological record and Git history.

**Commit (code):** 84dd3205ee6a6867c57f52f202f0d8c529de8990 — "0098: persist WiFi credentials in NVS"

### What I did

- Added NVS credential storage to `0098-esp32-p4-wifi6-webserver/main/app_main.c`.
- Added `nvs.h` and a small NVS layer:
  - `load_credentials_from_nvs()`
  - `save_credentials_to_nvs()`
  - `clear_credentials_from_nvs()`
- Added NVS keys:
  - namespace: `wifi`
  - key: `ssid`
  - key: `password`
- Kept the compiled defaults as fallback credentials:
  - SSID: `yolobolo`
  - password: `bring3248camera`
- Extended the `wifi` console command:
  - `wifi set <ssid> [password]` changes runtime credentials only.
  - `wifi set <ssid> [password] save` changes runtime credentials and persists them.
  - `wifi save` persists the current runtime credentials.
  - `wifi clear` removes saved credentials from NVS.
- Added a `saved` boolean to the `/status` JSON response.
- Updated `0098-esp32-p4-wifi6-webserver/README.md` with the persistence behavior.
- Rebuilt the firmware successfully.
- Flashed the firmware over the CH343 UART bridge.
- Ran `wifi save` over the UART console.
- Reset the board and verified the next boot loaded saved credentials from NVS.
- Verified HTTP `/status` after the reset.

### Why

The networking experiment should be able to move between networks without recompilation. A full provisioning UX is not needed yet, but storing credentials in NVS is the minimum durable behavior: it lets a console-provided SSID/password survive reset and makes the firmware useful outside the single default LAN.

The implementation intentionally keeps persistence separate from runtime changes. This makes `wifi set` safe for quick experiments and makes saving explicit through `wifi save` or the `save` flag.

### What worked

- Build succeeded after adding the NVS code:

```text
0098-esp32-p4-wifi6-webserver.bin binary size 0xaeec0 bytes.
Smallest app partition is 0x300000 bytes. 0x251140 bytes (77%) free.
```

- Flashing succeeded through the CH343 serial path.
- First boot after the new image correctly reported no saved credentials yet and used compiled defaults:

```text
I (1655) p4_web: using built-in Wi-Fi defaults (no NVS namespace yet)
```

- The board joined the network and served status:

```json
"wifi": {
  "mode": "sta",
  "state": "got_ip",
  "ssid": "yolobolo",
  "saved": false,
  "ip": "192.168.0.88",
  "retries": 0,
  "last_disconnect_reason": -1
}
```

- The console command persisted the runtime credentials:

```text
wifi save
I (52965) p4_web: saved Wi-Fi credentials for ssid='yolobolo'
saved credentials for ssid=yolobolo
```

- After reset, the firmware loaded the saved credentials from NVS:

```text
I (1655) p4_web: loaded saved Wi-Fi credentials for ssid='yolobolo'
```

- HTTP `/status` then reported `saved: true`:

```json
"wifi": {
  "mode": "sta",
  "state": "got_ip",
  "ssid": "yolobolo",
  "saved": true,
  "ip": "192.168.0.88",
  "retries": 0,
  "last_disconnect_reason": -1
}
```

### What didn't work

- The reset capture that verified `loaded saved Wi-Fi credentials` ran for only 15 seconds and ended before DHCP completed. A subsequent polling `curl` confirmed the board did reach `got_ip` at `192.168.0.88`.
- The app version in the ESP-IDF boot log still showed `c71c485-dirty` during validation because the firmware was flashed before the `84dd320` code commit existed. The source state was committed immediately after validation.

### What I learned

- The credential path works without changing the partition table because the default `nvs` partition already exists at `0x9000` with size `0x6000`.
- Explicit persistence is clearer than automatic persistence for this stage. It avoids accidentally saving a temporary SSID/password during console experiments.
- Adding `saved` to the status endpoint is a cheap but useful validation hook: it lets the host verify persistence state over HTTP without relying only on serial logs.

### What was tricky to build

The main edge case was command parsing. `wifi set <ssid> [password] save` needs to support both secured networks and open networks. The implementation treats `save`/`--save` as flags and the first non-flag argument after the SSID as the password. That means `wifi set myssid save` saves an open-network credential, while `wifi set myssid mypass save` saves a password-protected credential.

Another subtle point is that clearing NVS should not immediately destroy the active runtime connection. `wifi clear` only removes saved credentials; the current runtime SSID remains active until the operator changes it, reconnects, or reboots. This makes the command less surprising during a live session.

### What warrants a second pair of eyes

- The console parser is intentionally minimal. It does not handle quoted SSIDs/passwords with spaces.
- The password is stored as plain text in NVS. That is acceptable for the current bring-up experiment, but a production firmware may need secure storage or a provisioning flow that avoids logging/secrets exposure.
- The status endpoint exposes the SSID and saved state, but not the password. Confirm that this is the desired observability/security balance.

### What should be done in the future

- Add command help text that explicitly documents open-network behavior.
- Consider NVS schema versioning if more network settings are added.
- Add a reboot command or a `wifi reload` command to test saved credentials without manually resetting the board.
- Add a longer HTTP stability test now that saved credentials survive reset.

### Code review instructions

- Review `0098-esp32-p4-wifi6-webserver/main/app_main.c`:
  - NVS helpers near the top of the file.
  - `cmd_wifi()` command parsing for `set`, `save`, and `clear`.
  - `/status` JSON generation and the `saved` field.
  - `app_main()` load order: initialize NVS, load credentials, then start Wi-Fi.
- Review `0098-esp32-p4-wifi6-webserver/README.md` for operator command accuracy.
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
idf.py -p "$PORT" flash
idf.py -p "$PORT" monitor
```

Then run in the console:

```text
wifi status
wifi save
```

Reset and confirm:

```text
loaded saved Wi-Fi credentials for ssid='yolobolo'
```

And over HTTP:

```bash
curl -sS http://192.168.0.88/status | python3 -m json.tool
```

### Technical details

The NVS helpers use the normal ESP-IDF string API:

```c
nvs_set_str(nvs, "ssid", s_ssid);
nvs_set_str(nvs, "password", s_password);
nvs_commit(nvs);
```

Boot order matters:

```c
ESP_ERROR_CHECK(init_nvs());
(void)load_credentials_from_nvs();
ESP_ERROR_CHECK(start_wifi_sta());
```

Loading credentials before `start_wifi_sta()` ensures the first `esp_wifi_set_config(WIFI_IF_STA, ...)` receives either the saved credentials or the compiled fallback.
