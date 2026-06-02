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

## Step 6: PicoCalc keyboard implementation guide and ESP-IDF diagnostic driver

Added the first PicoCalc keyboard implementation package for the ESP32-P4-WIFI6 firmware. This step turns the keyboard research into an actionable ticket guide and a buildable ESP-IDF driver that can read the STM32 southbridge over I2C and expose raw keyboard diagnostics through the existing CH343 UART console.

The implementation intentionally stays diagnostic-first. It does not try to integrate keyboard input into a UI yet; instead, it provides `kbd status`, `kbd poll`, and `kbd raw on/off` so the physical adapter wiring and the STM32 FIFO protocol can be validated before the LCD and application input stack are connected.

### Prompt Context

**User prompt (verbatim):** "write a detailed implementation guide and store in the ticket. then implement"

**Assistant interpretation:** Write a detailed guide for reading the PicoCalc keyboard through the STM32 I2C southbridge, store it in the existing docmgr ticket, then implement that plan in the active ESP32-P4 firmware.

**Inferred user intent:** The user wants to move from research into concrete keyboard bring-up: documented wiring/protocol decisions plus firmware that can be flashed and exercised from the serial console once the PicoCalc keyboard bus is physically connected.

**Commit (code):** pending — buildable implementation created but not committed in this step yet.

### What I did

- Added ticket guide `design-doc/02-picocalc-keyboard-implementation-guide.md` with hardware mapping, protocol details, ESP-IDF driver plan, console command behavior, validation steps, and failure-mode debugging notes.
- Added `0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.h`.
- Added `0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.c`.
- Updated `0098-esp32-p4-wifi6-webserver/main/CMakeLists.txt` to compile the keyboard module and require `esp_driver_i2c`.
- Updated `0098-esp32-p4-wifi6-webserver/main/app_main.c` to initialize the keyboard driver and register a `kbd` console command.
- Updated `0098-esp32-p4-wifi6-webserver/README.md` with the new `kbd` commands.
- Built the firmware with ESP-IDF v5.4.2:

```bash
cd 0098-esp32-p4-wifi6-webserver && . $HOME/esp/esp-idf-5.4.2/export.sh >/tmp/esp-idf-export-0098.log 2>&1 && idf.py build
```

### Why

The keyboard is the first PicoCalc peripheral that can be validated without needing display rendering or a full application UI. A raw diagnostic path lets us prove that the ESP32-P4 adapter pins, I2C speed, STM32 address, status register, and FIFO register are correct before depending on the keyboard for interactive firmware control.

Keeping the guide in the ticket makes the protocol reproducible for future implementation phases and records the reasons behind slow 10 kHz I2C, GPIO7/GPIO8 selection, and raw-event-first console behavior.

### What worked

- The implementation built successfully under ESP-IDF v5.4.2.
- The new ESP-IDF I2C master API accepted the ESP32-P4 I2C0/GPIO7/GPIO8/10 kHz configuration.
- The existing `esp_console` structure in `0098` was easy to extend with a second top-level command.
- The driver keeps raw protocol observability: status byte, FIFO count, caps/num bits, event state, key code, printable ASCII, and known special-key names.

### What didn't work

- No live keyboard validation was performed in this step because the PicoCalc keyboard bus is not yet physically connected to the ESP32-P4 board in the current session.
- The build log still reports a dirty app version because local source and ticket files are intentionally modified before the next commit.

Exact build result:

```text
Project build complete. To flash, run:
 idf.py flash
```

### What I learned

- The new ESP-IDF `driver/i2c_master.h` API is a good fit for this peripheral because it lets the bus and the `0x1F` device be configured separately.
- The Pico firmware's write-delay-read sequence can be represented directly with `i2c_master_transmit()`, `vTaskDelay(pdMS_TO_TICKS(2))`, and `i2c_master_receive()`.
- Adding keyboard diagnostics to `0098` is more useful than creating a standalone firmware right now because it preserves the already-working Wi-Fi, HTTP, NVS, and UART console bring-up environment.

### What was tricky to build

The main design choice was whether to initialize the keyboard lazily from the console command or eagerly during boot. The implementation initializes during boot but does not treat keyboard failure as fatal. If no PicoCalc keyboard is connected, the app still starts Wi-Fi, HTTP, and the UART console; later `kbd status` calls can retry the driver path and report I2C errors.

Another subtle point is raw-mode ownership. `kbd raw on` starts a lightweight background task that polls every 20 ms and prints only when events arrive. `kbd raw off` flips a shared flag and lets the task exit itself, which avoids deleting a task while it may be inside an I2C transaction or printing to the console.

### What warrants a second pair of eyes

- The current driver enables internal pull-ups even though the PicoCalc mainboard has external 4.7 kΩ pull-ups. This should be harmless for bring-up, but it is worth confirming electrical expectations once the real adapter is wired.
- The driver uses I2C0 on GPIO7/GPIO8, which may also host the Waveshare board's onboard ES8311/BNO085 I2C devices. The current app does not initialize those devices, but future codec/IMU work needs bus-speed planning.
- The raw task prints directly from a FreeRTOS task while the REPL also uses the UART. This is acceptable for diagnostics, but a production input stack should queue events instead of printing asynchronously.
- `picocalc_keyboard_read_register()` delays 2 ms for every register, not just FIFO reads. This is conservative and simpler for bring-up, but can be optimized later if needed.

### What should be done in the future

- Flash the updated `0098` firmware after checking `lsof "$PORT"`.
- Wire PicoCalc keyboard SDA/SCL/GND/power to the adapter and run `kbd status`, `kbd poll 10`, and `kbd raw on`.
- Record observed raw key events, especially modifiers, arrows, F-keys, Home/End/Delete, Caps Lock, and repeat behavior.
- Add a real event queue and host-side key mapper once raw protocol validation passes.
- Decide whether the keyboard should remain on GPIO7/GPIO8 or move to a dedicated I2C bus if onboard codec/IMU access becomes important.

### Code review instructions

- Start with `0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.c`:
  - `picocalc_keyboard_init()` for bus/device configuration.
  - `picocalc_keyboard_read_register()` for the write-delay-read sequence.
  - `picocalc_keyboard_poll_event()` for status/FIFO behavior.
- Then review `0098-esp32-p4-wifi6-webserver/main/app_main.c`:
  - `print_keyboard_status()`.
  - `cmd_kbd()`.
  - `keyboard_raw_task()`.
  - `app_main()` boot order.
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
```

After hardware wiring and serial ownership check:

```bash
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
lsof "$PORT" || true
idf.py -p "$PORT" flash
```

Then run over the CH343 UART console:

```text
kbd status
kbd poll 10
kbd raw on
kbd raw off
```

### Technical details

The driver constants are:

```c
#define PICOCALC_KBD_I2C_SDA_GPIO      7
#define PICOCALC_KBD_I2C_SCL_GPIO      8
#define PICOCALC_KBD_I2C_SPEED_HZ      10000
#define PICOCALC_KBD_I2C_ADDR          0x1F
#define PICOCALC_KBD_REG_STATUS        0x04
#define PICOCALC_KBD_REG_FIFO          0x09
#define PICOCALC_KBD_COUNT_MASK        0x1F
```

The register-read sequence intentionally mirrors the existing Pico SDK keyboard driver:

```c
i2c_master_transmit(s_dev, &reg, 1, 50);
vTaskDelay(pdMS_TO_TICKS(2));
i2c_master_receive(s_dev, dst, len, 50);
```

The console command surface is:

```text
kbd status
kbd poll [limit]
kbd raw on
kbd raw off
```

## Step 7: Corrected physical adapter pin mapping and keyboard ACK validation

Corrected a major assumption in the PicoCalc keyboard bring-up: the first keyboard driver used a function-optimized ESP32-P4 mapping, but the real adapter discussion and supplied pinout images showed that the board is being treated as a same-position 40-pin physical replacement. In that physical mapping, Pico GP6/GP7 do not land on the Waveshare board's labeled GPIO7/GPIO8 I2C pins; they land on GPIO50/GPIO49.

After updating the firmware to use GPIO50 as SDA and GPIO49 as SCL, `kbd status` successfully read the PicoCalc southbridge status register. This proves the STM32 keyboard controller is present at address `0x1F` on the corrected physical adapter pins. No keypress events were captured yet, but the I2C ACK/status path is now working.

### Prompt Context

**User prompt (verbatim):** "update your diary, commit, and then nice, let's do a new separate firmware with just the display and keybaord, to avoid having to ocmpile in all the wifi stuff."

**Assistant interpretation:** Record the corrected pin-mapping and keyboard validation work, commit it, then start a new lean ESP-IDF firmware that excludes ESP-Hosted/Wi-Fi and focuses only on PicoCalc display plus keyboard.

**Inferred user intent:** The user wants the working keyboard discovery preserved in the ticket/history and wants faster iteration for PicoCalc peripheral bring-up without the compile/runtime overhead of the networking experiment.

**Commit (code):** pending — this diary entry is being written before the focused commit.

### What I did

- Read the supplied Raspberry Pi Pico 2 and Waveshare ESP32-P4-WIFI6 pinout images.
- Reinterpreted the mapping as a same-position physical adapter instead of a function-optimized cross-routed adapter.
- Added `design-doc/03-full-rpico-socket-to-waveshare-esp32-p4-pin-map.md` with the corrected full physical map.
- Corrected `design-doc/02-picocalc-keyboard-implementation-guide.md` from the obsolete GPIO7/GPIO8 keyboard assumption to GPIO50/GPIO49 for the physical adapter.
- Updated `0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.h` to use:

```c
#define PICOCALC_KBD_I2C_SDA_GPIO      50
#define PICOCALC_KBD_I2C_SCL_GPIO      49
```

- Built the firmware with ESP-IDF v5.4.2.
- Started `idf.py monitor` inside tmux session `0098_p4_monitor`.
- Used the monitor menu shortcut `Ctrl-T A` to app-flash without leaving monitor.
- Ran `kbd status` after reboot.

### Why

The earlier GPIO7/GPIO8 keyboard test NACKed because it was probing the Waveshare board's native I2C header labels, not the physical positions where Pico GP6/GP7 land in the adapter. The user's example — Pico GP6 physical pin 9 mapping to Waveshare position 9/GPIO50 — clarified the correct abstraction: route by physical socket position first, then assign firmware GPIO constants from that physical map.

### What worked

- `idf.py monitor` in tmux worked for interactive monitor control.
- `Ctrl-T A` from the monitor successfully ran `idf.py app-flash` and returned to monitor after reset.
- The corrected boot log showed:

```text
I (...) picocalc_kbd: initialized PicoCalc keyboard I2C: sda=50 scl=49 speed=10000 addr=0x1f
```

- `kbd status` succeeded:

```text
kbd status ok=1 raw=0x00 fifo=0 caps=0 num=0 initialized=1 errors=0
```

- `kbd poll 10` ran cleanly with no queued key events:

```text
kbd poll done events=0 limit=10
```

### What didn't work

- The first physical-mapping flash used `sda=49 scl=50`, which still NACKed. That was a line-role swap: PicoCalc sources identify GP6 as SDA and GP7 as SCL, so the correct same-position mapping is SDA GPIO50 and SCL GPIO49.
- The Wi-Fi part of `0098` repeatedly disconnected with reason `201` during this session. This did not block keyboard validation, but it is another reason to create a separate non-Wi-Fi display+keyboard firmware for peripheral bring-up.
- No actual keypress event was captured yet; only the status-register ACK/empty-FIFO path is validated.

### What I learned

- There are two distinct adapter designs:
  - A function-optimized cross-routed adapter can place keyboard on GPIO7/GPIO8 and LCD on SPI2 IO_MUX GPIO28–31.
  - A same-position physical Pico replacement maps keyboard to GPIO50/GPIO49 and LCD to GPIO3/GPIO2/GPIO7/GPIO24/GPIO25.
- The current physical adapter work must use the same-position table, not the function-optimized table.
- The keyboard ACK is a strong validation point: the STM32 southbridge is reachable at `0x1F` once the physical mapping and SDA/SCL roles are correct.

### What was tricky to build

The subtle issue was that the labels `GPIO7/GPIO8` appeared correct if thinking in terms of the Waveshare board's native I2C silkscreen, but they were physically wrong for a Pico socket replacement. A socket adapter is constrained by the Pico physical pin order. Once the user pointed out that Pico physical pin 9 maps to Waveshare physical position 9/GPIO50, the rest of the mapping had to be recalculated by position.

Another tricky point was the SDA/SCL role. The first corrected physical flash still had the roles swapped. The successful configuration is Pico GP6/SDA to ESP GPIO50 and Pico GP7/SCL to ESP GPIO49.

### What warrants a second pair of eyes

- Confirm the physical orientation assumption before schematic capture: both pinout images were interpreted with USB at the top and no mirror/underside inversion.
- Review the full physical map for power pins. Signal validation is improving, but VSYS/VBUS/3V3/EN/RUN still need electrical verification before a final PCB.
- Review the LCD mapping under the same-position adapter because it uses ESP32-P4 GPIO2/GPIO3/GPIO7/GPIO24/GPIO25, not the earlier clean SPI2 IO_MUX group.

### What should be done in the future

- Capture real keypress events with `kbd raw on` or `kbd poll` while pressing keys.
- Create a lean `0099` firmware with only display, keyboard, UART console, and PSRAM; omit ESP-Hosted/Wi-Fi to speed builds and reduce log noise.
- Add a minimal display color-bar/smoke-test path using the physical adapter LCD pins.
- Decide whether the final hardware should remain same-position or become a cross-routed interposer optimized for ESP32-P4 peripheral placement.

### Code review instructions

- Review `0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.h` for the corrected GPIO constants.
- Review `design-doc/03-full-rpico-socket-to-waveshare-esp32-p4-pin-map.md` for the full physical mapping.
- Review `design-doc/02-picocalc-keyboard-implementation-guide.md` for consistency with the corrected GPIO50/GPIO49 keyboard path.
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 monitor
# in monitor: Ctrl-T A
```

Then run:

```text
kbd status
kbd poll 10
```

### Technical details

Correct same-position keyboard mapping:

```text
Pico physical pin 9  / GP6 / SDA -> Waveshare physical position 9  / GPIO50
Pico physical pin 10 / GP7 / SCL -> Waveshare physical position 10 / GPIO49
```

Validated output:

```text
kbd status ok=1 raw=0x00 fifo=0 caps=0 num=0 initialized=1 errors=0
```

## Step 8: Lean display+keyboard firmware without Wi-Fi

Created a new standalone firmware, `0099-esp32-p4-picocalc-display-keyboard`, to bring up PicoCalc peripherals without ESP-Hosted, Wi-Fi, HTTP, or NVS credential machinery. This firmware keeps the proven CH343 UART console, uses the corrected same-position physical adapter keyboard pins, and adds a minimal SPI LCD smoke-test path on the physical LCD pins.

The firmware built and flashed successfully. On the live board, the keyboard status command ACKed the PicoCalc STM32 controller, and the LCD command path initialized the panel and wrote color bars without SPI/API errors. Visual confirmation of the displayed bars still needs human observation, but the firmware-side transaction path is working.

### Prompt Context

**User prompt (verbatim):** (same as Step 7)

**Assistant interpretation:** After committing the corrected keyboard mapping work, create a separate lean firmware focused on PicoCalc display and keyboard only.

**Inferred user intent:** The user wants faster compile/flash/test cycles and less runtime log noise while validating PicoCalc hardware peripherals.

**Commit (code):** pending — this diary entry is being written before the `0099` commit.

### What I did

- Created `0099-esp32-p4-picocalc-display-keyboard/`.
- Copied the validated `picocalc_keyboard.c/.h` driver from `0098`.
- Added a new `app_main.c` with:
  - CH343 UART `esp_console` REPL.
  - `kbd status`, `kbd poll [limit]`, `kbd raw on/off`.
  - `lcd init`, `lcd fill <color>`, `lcd bars`.
  - `status` command.
- Used the same-position physical adapter pins:
  - Keyboard SDA GPIO50, SCL GPIO49.
  - LCD SCK GPIO3, MOSI GPIO2, CS GPIO7, DC GPIO24, RST GPIO25.
- Added `README.md`, `CMakeLists.txt`, `main/CMakeLists.txt`, and `sdkconfig.defaults`.
- Built with ESP-IDF v5.4.2.
- Killed the old `0098_p4_monitor` tmux session to preserve single serial ownership.
- Flashed `0099` to `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- Started tmux monitor session `0099_p4_dk_monitor`.
- Ran console validation commands.

### Why

`0098` is useful for Wi-Fi and ESP-Hosted testing, but it is too heavy for fast PicoCalc peripheral iteration. Its build pulls in the networking stack, and its runtime logs interleave Wi-Fi disconnect/retry messages with keyboard diagnostics. A smaller display+keyboard firmware keeps the bring-up loop focused.

### What worked

- `0099` built successfully.
- The app binary is much smaller than the Wi-Fi firmware:

```text
0099 binary size: 0x5bde0
0098 binary size: 0xb4a40
```

- Flash succeeded.
- Boot log showed the expected pin mapping:

```text
keyboard: SDA GPIO50 SCL GPIO49 addr=0x1f hz=10000
lcd: sck=3 mosi=2 cs=7 dc=24 rst=25 hz=20000000
```

- Keyboard status succeeded:

```text
kbd status ok=1 raw=0x00 fifo=0 caps=0 num=0 initialized=1 errors=0
```

- LCD init succeeded:

```text
lcd init: ESP_OK
```

- LCD color bars command succeeded:

```text
lcd bars ok elapsed_ms=95
```

### What didn't work

- Visual confirmation of the LCD output was not captured in this step. The command path reports success, but a human still needs to confirm whether color bars are visible on the PicoCalc display.
- The minimal LCD init sequence is intentionally basic (`SWRESET`, `SLPOUT`, RGB565, `MADCTL=0x48`, inversion on, display on). If the panel remains blank, the next fix is likely the fuller ST7365P/ILI9488 vendor init sequence from the existing Pico SDK driver/reference docs.
- ESP-IDF still logs the known flash warning:

```text
Detected flash size > 16 MB, but access beyond 16 MB is not supported for this flash model yet.
```

### What I learned

- The lean firmware dramatically reduces runtime noise. There are no ESP-Hosted SDIO logs and no Wi-Fi retry messages.
- GPIO2/GPIO3/GPIO7/GPIO24/GPIO25 can be configured by the app for the LCD path under the same-position physical mapping.
- The current LCD SPI transaction implementation is good enough to send a full 320×320 RGB565 color-bar frame in about 95 ms at 20 MHz, assuming the panel accepts the init sequence.

### What was tricky to build

The main tricky point was that the same-position adapter LCD mapping is not the earlier ideal SPI2 IO_MUX mapping. The physical mapping places LCD SCK/MOSI/CS/DC/RST on GPIO3/GPIO2/GPIO7/GPIO24/GPIO25. GPIO2/GPIO3 and GPIO24/GPIO25 have JTAG/USB-Serial-JTAG caveats, but the project uses the CH343 UART console, so the firmware can still claim them for LCD testing.

Another subtle point is that SPI transaction success does not prove the LCD controller interpreted the commands. The driver can report `ESP_OK` even if the panel wiring, reset, controller variant, or init sequence is wrong. That is why the next validation step is visual confirmation, followed by fuller init-sequence work if needed.

### What warrants a second pair of eyes

- Confirm visually whether `lcd bars` actually appears on the PicoCalc LCD.
- Review whether GPIO24/GPIO25 use has any boot/JTAG side effects on ESP32-P4 beyond disabling USB Serial/JTAG functionality.
- Review whether the minimal LCD init sequence is sufficient for the actual PicoCalc ST7365P panel, or whether the full unlock/vendor profile should be ported immediately.

### What should be done in the future

- Press PicoCalc keys with `kbd raw on` enabled and record raw events.
- If the display is blank, port the full Pico SDK/TFT_eSPI-derived init sequence into `0099`.
- Add a simple on-screen keyboard event renderer once both LCD and keyboard are visually confirmed.
- Consider making `0099` the default PicoCalc peripheral bring-up project and leaving `0098` for networking-only experiments.

### Code review instructions

- Review `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`:
  - LCD pin constants and SPI setup.
  - Minimal panel init sequence.
  - `lcd_fill_rect()` and `lcd bars` path.
  - Console command registration.
- Review `0099-esp32-p4-picocalc-display-keyboard/main/picocalc_keyboard.h` for GPIO50/GPIO49.
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
lsof "$PORT" || true
idf.py -p "$PORT" flash monitor
```

Then run:

```text
kbd status
lcd init
lcd bars
```

### Technical details

Current LCD constants:

```c
#define LCD_PIN_SCK   3
#define LCD_PIN_MOSI  2
#define LCD_PIN_CS    7
#define LCD_PIN_DC    24
#define LCD_PIN_RST   25
#define LCD_SPI_HZ    (20 * 1000 * 1000)
```

Current keyboard constants:

```c
#define PICOCALC_KBD_I2C_SDA_GPIO 50
#define PICOCALC_KBD_I2C_SCL_GPIO 49
#define PICOCALC_KBD_I2C_SPEED_HZ 10000
#define PICOCALC_KBD_I2C_ADDR     0x1F
```

## Step 8: LCD Throughput Optimization Guide and 32 KiB DMA Fill Baseline

This step moved the LCD work from clock-speed discovery into throughput engineering. The previous finding was that ESP-IDF GPSPI accepts actual 80 MHz only when the ESP32-P4 SPI device explicitly uses `SPI_CLK_SRC_SPLL`; beyond that, the driver enforces an 80 MHz normal SCLK ceiling. The new work therefore focused on reducing transaction overhead rather than requesting faster clocks.

The implemented optimization keeps the display at actual 80 MHz, raises the SPI bus maximum transfer size to 32 KiB, and replaces the old tiny 512-byte solid-fill chunks with a reusable 32 KiB internal DMA-capable buffer. On the live PicoCalc/Waveshare setup, full-screen RGB565 fill improved from roughly 32 ms to 21 ms per frame, which is close to the 20.48 ms raw wire-time floor for 204,800 bytes at 80 MHz.

### Prompt Context

**User prompt (verbatim):** "Ok, let's do more optimizations, create a detailed analysis guide explaining these to me, and then make a list of tasks to go through, and then implement and benchmark, and ofc also ask me for feedback to make sure things are displayed properly."

**Assistant interpretation:** Explain the display-throughput optimization path, write the analysis into the ticket, create a concrete task list, implement the next low-risk optimization, benchmark it on hardware, and ask the user to visually verify the screen output.

**Inferred user intent:** The user wants to continue improving the PicoCalc LCD path in a structured way: understand the engineering constraints, keep the ticket documentation useful, get measurable performance improvements, and avoid trusting serial logs alone when display correctness needs human visual confirmation.

**Commit (code):** 7bb4d1ac2554e894263b7fbce0c325777c389a08 — "0099: optimize LCD fill throughput"

### What I did

- Confirmed from ESP-IDF source that normal GPSPI SCLK is capped by `MIN(clock_source_hz / 2, 80 MHz)`.
- Confirmed from ESP32-P4 HAL source that the DMA transaction length register supports `(1 << 18)` bits, i.e. 32 KiB per transaction.
- Updated `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`:
  - kept `LCD_SPI_CLK_SRC` as `SPI_CLK_SRC_SPLL`;
  - kept the default requested LCD clock at 80 MHz;
  - added `LCD_SPI_MAX_TRANSFER_SZ` and `LCD_FILL_DMA_CHUNK_BYTES` at 32 KiB;
  - changed `spi_bus_config_t.max_transfer_sz` from 4096 bytes to 32 KiB;
  - changed the SPI LCD device queue size from 1 to 4 for future queued-transfer work;
  - added a reusable internal DMA-capable fill buffer allocated with `heap_caps_malloc(..., MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)`;
  - changed `lcd_fill_rect()` to transmit large DMA chunks instead of a 512-byte stack buffer;
  - extended `lcd fill`, `lcd bench`, and `status` output with throughput and DMA-buffer metrics.
- Updated `0099-esp32-p4-picocalc-display-keyboard/README.md` with the current 80 MHz/SPLL and 32 KiB DMA-buffer findings.
- Created `design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md` with:
  - the throughput model;
  - ESP-IDF clock and DMA limits;
  - completed and remaining tasks;
  - benchmark protocol;
  - visual feedback checklist;
  - phased next implementation plan.
- Updated `tasks.md` with completed LCD-throughput tasks and the next task queue.
- Related the optimization guide to the firmware, README, and ESP-IDF source files with `docmgr doc relate`.
- Built the optimized firmware with ESP-IDF v5.4.2.
- Flashed through the existing `0099_p4_dk_monitor` tmux monitor using `Ctrl-T A` after checking serial ownership.
- Ran:

```text
lcd init
lcd speed
lcd bench 5
lcd bench 50
lcd bars
status
```

### Why

At actual 80 MHz, a 320×320 RGB565 full-screen update has a raw payload floor of about 20.48 ms. The pre-optimization firmware measured about 32 ms per full-screen fill, so the remaining performance problem was not the SPI clock itself. It was the overhead of splitting the 204,800-byte frame into roughly 400 small pixel transactions.

ESP32-P4's SPI DMA transaction length limit makes 32 KiB a natural chunk size. A full frame then needs roughly seven pixel transactions, which is much closer to the raw SPI payload limit and does not require unsupported driver changes.

### What worked

- The optimized firmware built successfully with `idf.py build`.
- The optimized firmware flashed successfully through the CH343 UART monitor path.
- The LCD still initialized at actual 80 MHz:

```text
lcd speed requested=80000000 actual_khz=80000
```

- The first benchmark allocated the DMA buffer and improved full-screen fill time:

```text
I (...) p4_picocalc: LCD DMA buffer allocated: 32768 bytes
lcd bench loops=5 elapsed_ms=107 per_fill_ms=21 throughput_kib_s=9345 requested=80000000 actual_khz=80000 dma_chunk=32768
```

- The longer benchmark stayed stable:

```text
lcd bench loops=50 elapsed_ms=1071 per_fill_ms=21 throughput_kib_s=9337 requested=80000000 actual_khz=80000 dma_chunk=32768
```

- Color bars completed faster than before:

```text
lcd bars ok elapsed_ms=26
```

- `status` reported the active DMA buffer and chunk size:

```text
status ... lcd_actual_khz=80000 lcd_dma_chunk=32768 lcd_dma_buf=32768
```

### What didn't work

- No code failure occurred in this step after the optimization was implemented.
- A previous monitor scrollback still contained an attempted `lcd speed 90M` rejection, which remains expected and useful evidence:

```text
spi_master: spi_bus_add_device(432): invalid sclk speed
lcd speed requested=90000000 err=ESP_ERR_INVALID_ARG actual_khz=80000
```

- The current benchmark still cannot prove visual correctness. It proves that the firmware transmitted data without SPI errors; the user still needs to inspect the display.

### What I learned

- At actual 80 MHz, the 32 KiB DMA-buffer path is close to the theoretical full-frame payload floor: 21 ms measured versus about 20.48 ms raw wire time.
- The major win was reducing transaction count, not changing SCLK.
- A 32 KiB internal DMA allocation is acceptable in this lean firmware: internal free heap dropped from about 570 KiB to about 537 KiB after allocation.
- Solid-fill benchmarks are useful for measuring transport throughput, but they are weak signal-integrity tests because they do not create many high-frequency pixel transitions.

### What was tricky to build

The main tricky part was choosing a chunk size that was large enough to remove transaction overhead but still inside ESP32-P4 GPSPI's DMA transaction limits. ESP-IDF exposes two separate constraints: the bus-level `max_transfer_sz` and the hardware transaction bit-length check. Setting a large software buffer without raising `max_transfer_sz` would not be enough; raising `max_transfer_sz` above the hardware limit would fail later. The 32 KiB value matches `SPI_LL_DMA_MAX_BIT_LEN / 8`.

Another subtle point is byte order. The existing fill path wrote RGB565 as high byte then low byte. The optimized DMA buffer preserves that explicit byte order instead of treating the buffer as a native-endian `uint16_t[]`, which would be wrong on a little-endian CPU for this panel command stream.

### What warrants a second pair of eyes

- Visual inspection of the current `lcd bars` output at actual 80 MHz.
- Whether the GPIO-matrix same-position wiring is electrically stable at 80 MHz under longer display activity.
- Whether the 32 KiB internal DMA buffer is the right long-term size once the firmware grows beyond the lean bring-up app.
- Whether queued DMA is worth implementing now, given the full-screen fill path is already near the raw SPI payload floor.

### What should be done in the future

- Ask the user to confirm the current color bars are clean and correctly colored.
- Add checkerboard, stripe, and diagonal pattern commands for stronger signal-integrity testing.
- Add dirty-rectangle and terminal-cell benchmarks because real PicoCalc UI updates will not always be full-screen fills.
- Add a general RGB565 blit path for arbitrary pixel buffers.
- Evaluate queued DMA only after the simple 32 KiB polling baseline is visually confirmed.

### Code review instructions

- Start with `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`:
  - constants near `LCD_SPI_MAX_TRANSFER_SZ` and `LCD_FILL_DMA_CHUNK_BYTES`;
  - `lcd_tx()` chunking;
  - `lcd_ensure_dma_buffer()`;
  - `lcd_init_bus_gpio_and_host()` max transfer size;
  - `lcd_fill_rect()` byte-order and transfer loop;
  - `lcd bench` throughput output.
- Read `ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md` for the reasoning and next tasks.
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
```

Then in the monitor:

```text
lcd speed 80M
lcd bench 50
lcd bars
status
```

### Technical details

Current optimized parameters:

```c
#define LCD_DEFAULT_SPI_HZ        (80 * 1000 * 1000)
#define LCD_SPI_CLK_SRC           SPI_CLK_SRC_SPLL
#define LCD_SPI_MAX_TRANSFER_SZ   (32 * 1024)
#define LCD_FILL_DMA_CHUNK_BYTES  LCD_SPI_MAX_TRANSFER_SZ
```

Current observed benchmark:

```text
lcd bench loops=50 elapsed_ms=1071 per_fill_ms=21 throughput_kib_s=9337 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd bars ok elapsed_ms=26
```

Visual feedback request to the user: the display is currently left on `lcd bars` at actual 80 MHz. Please confirm whether the eight color bars are visible, stable, correctly colored, and free of flicker, random pixels, or partial-update artifacts.

## Step 9: Operator Visual Confirmation of 80 MHz LCD Bars

The optimized 80 MHz LCD path is now validated beyond serial logs. After the 32 KiB DMA-buffer optimization and benchmark run, the display was left on the `lcd bars` pattern at actual 80 MHz. The operator inspected the physical PicoCalc LCD and confirmed that the output looked good.

This closes the first throughput baseline: the same-position adapter wiring can run the current LCD smoke-test path at actual 80 MHz with a measured full-screen solid-fill time of about 21 ms and visually acceptable color-bar output.

### Prompt Context

**User prompt (verbatim):** "yes, it's good."

**Assistant interpretation:** The user visually inspected the PicoCalc LCD bars after the optimized 80 MHz run and confirmed that the display output is acceptable.

**Inferred user intent:** Record that the current high-speed display baseline is visually good and proceed to the next optimization/test phase.

**Commit (code):** N/A — this step records operator validation only.

### What I did

- Marked the task `LCD throughput: operator visual inspection of 80 MHz color bars` complete in `tasks.md`.
- Recorded the user's visual confirmation in the diary.

### Why

SPI logs and benchmark timings prove that transactions completed, but they do not prove that the panel displayed the result correctly. Visual confirmation is required before building more complex display tests on top of the 80 MHz/32 KiB DMA baseline.

### What worked

- The physical display output was confirmed good by the operator.
- The current baseline remains:

```text
lcd bench loops=50 elapsed_ms=1071 per_fill_ms=21 throughput_kib_s=9337 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd bars ok elapsed_ms=26
```

### What didn't work

- N/A

### What I learned

- The same-position GPIO-matrix LCD wiring is stable enough for the current 80 MHz color-bar smoke test.
- The 32 KiB DMA-buffer optimization can be treated as the accepted baseline for the next display experiments.

### What was tricky to build

The tricky part in this step was not implementation; it was validation discipline. The firmware's `lcd bars ok` result only says that SPI transactions completed. The meaningful validation was the human inspection that confirmed the glass looked correct.

### What warrants a second pair of eyes

- Future high-frequency patterns may still reveal signal-integrity problems that solid bars do not expose.
- Dirty-rectangle and arbitrary-pixel tests should be visually checked as they are added.

### What should be done in the future

- Add checkerboard, stripe, and diagonal pattern tests.
- Add dirty-rectangle and terminal-cell benchmarks.
- Add a general RGB565 blit path for arbitrary pixel data.

### Code review instructions

- Review `tasks.md` to confirm the visual-inspection task is marked complete.
- Continue from `design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md`, especially the Phase B and Phase C task descriptions.

### Technical details

Accepted baseline:

```text
clock_source=SPI_CLK_SRC_SPLL
requested_hz=80000000
actual_khz=80000
dma_chunk=32768
full_screen_fill≈21ms
lcd_bars≈26ms
```

## Step 10: High-Frequency LCD Patterns and Dirty-Rectangle Benchmarks

This step added the next layer of display validation on top of the accepted 80 MHz, 32 KiB DMA fill baseline. Solid fills and color bars are useful throughput tests, but they do not strongly exercise high-frequency pixel transitions. The new pattern commands generate checkerboard, one-pixel stripe, and diagonal patterns so the operator can look for shimmer, unstable columns, random pixels, or color/bit-order artifacts.

The same firmware update also added `lcd rectbench [w h loops]` for dirty-rectangle timing. This begins measuring workloads closer to terminal or UI updates, where the bottleneck is often rectangle command overhead rather than raw full-screen bandwidth.

### Prompt Context

**User prompt (verbatim):** "go ahead"

**Assistant interpretation:** Continue with the previously proposed next steps: implement checker/stripe/diagonal pattern tests and dirty-rectangle benchmarks, then run them on the board.

**Inferred user intent:** The user accepted the next phase of LCD optimization and validation after confirming the 80 MHz color bars looked good.

**Commit (code):** 9f7e979cf598f9f970242722f61b2c3a37b1e459 — "0099: add LCD pattern and rect benchmarks"

### What I did

- Added `lcd pattern checker|stripes|diagonal|all` to `0099`.
- Added a general pattern rectangle path that fills the reusable DMA buffer with generated RGB565 pixels and writes them in row-major order.
- Added `lcd rectbench [w h loops]` to measure repeated dirty-rectangle updates.
- Updated the README command list.
- Built with ESP-IDF v5.4.2.
- Flashed through the existing `0099_p4_dk_monitor` tmux session using `Ctrl-T A`.
- Ran:

```text
lcd init
lcd pattern checker
lcd pattern stripes
lcd pattern diagonal
lcd rectbench 16 16 500
lcd rectbench 80 24 200
lcd pattern stripes
status
```

### Why

The previous 80 MHz color-bar confirmation established that the panel could display large solid regions correctly. It did not prove that the same-position GPIO-matrix wiring is clean under high-frequency pixel changes. Alternating stripes and checkerboards are better stress patterns for visual signal-integrity checks.

Dirty-rectangle benchmarks are also necessary because real PicoCalc UI work will usually update small areas: character cells, cursors, status bars, and scroll regions. Small rectangles spend proportionally more time on `CASET`, `RASET`, `RAMWR`, GPIO DC changes, and transaction setup.

### What worked

- The firmware built successfully.
- The firmware flashed successfully.
- Pattern commands completed at actual 80 MHz:

```text
lcd pattern name=checker err=ESP_OK elapsed_ms=34 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd pattern name=stripes err=ESP_OK elapsed_ms=32 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd pattern name=diagonal err=ESP_OK elapsed_ms=33 requested=80000000 actual_khz=80000 dma_chunk=32768
```

- Dirty-rectangle benchmarks completed:

```text
lcd rectbench w=16 h=16 loops=500 elapsed_ms=427 rects_s=1170 payload_kib_s=585 requested=80000000 actual_khz=80000
lcd rectbench w=80 h=24 loops=200 elapsed_ms=237 rects_s=843 payload_kib_s=3164 requested=80000000 actual_khz=80000
```

- The display was left on `lcd pattern stripes`, a one-pixel black/white vertical stripe pattern, for operator visual feedback.

### What didn't work

- No runtime failures occurred in this step.
- Visual correctness is still pending for the new high-frequency stripe pattern. Serial logs prove the SPI transactions completed, not that the display is visually clean.

### What I learned

- Generated full-screen patterns cost more CPU time than solid fills because the firmware computes per-pixel colors before each DMA transaction. Pattern time is 32-34 ms versus about 21 ms for solid fills.
- Small dirty rectangles are dominated by command/setup overhead. A 16×16 rectangle benchmark achieved about 1170 rects/s, but only about 585 KiB/s payload throughput because each update sends little pixel data.
- Larger dirty rectangles improve payload efficiency. An 80×24 rectangle reached about 843 rects/s and 3164 KiB/s payload throughput.

### What was tricky to build

The pattern path had to preserve row-major geometry across DMA chunk boundaries. A 32 KiB chunk can split in the middle of a row, so the code tracks a pixel offset and reconstructs `x` and `y` for each generated pixel. This keeps checkerboards, one-pixel stripes, and diagonals continuous across transaction boundaries.

The dirty-rectangle benchmark had to avoid measuring a full-screen clear as part of the benchmark. It intentionally draws moving rectangles over the existing screen so the reported time reflects repeated dirty updates rather than full-frame repaint cost.

### What warrants a second pair of eyes

- The current one-pixel vertical stripe pattern should be inspected closely for shimmer, unstable columns, missing lines, or random pixels.
- The dirty-rectangle benchmark parameters should be reviewed against actual PicoCalc UI dimensions once font/cell sizes are chosen.
- The pattern generation path is CPU-bound enough that a future arbitrary blit path should separate render cost from SPI transfer cost.

### What should be done in the future

- Ask the user to confirm whether the stripe pattern is visually stable and correct.
- Add terminal-cell, row, and scroll-specific benchmark commands.
- Add a general RGB565 blit path with double-buffering or line-buffering.
- Consider queued DMA only after deciding which workload shape matters most: full-screen, rectangles, or terminal rows.

### Code review instructions

- Review `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`:
  - `lcd_pattern_t`, `pattern_pixel()`, and `lcd_pattern_rect()`;
  - `lcd pattern` command parsing;
  - `lcd rectbench` command and benchmark math.
- Validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
```

Then in the monitor:

```text
lcd pattern checker
lcd pattern stripes
lcd pattern diagonal
lcd rectbench 16 16 500
lcd rectbench 80 24 200
```

### Technical details

New command summary:

```text
lcd pattern checker
lcd pattern stripes
lcd pattern diagonal
lcd pattern all
lcd rectbench [w h loops]
```

Current visual-feedback request: the LCD is left on `lcd pattern stripes` at actual 80 MHz. Please inspect it for stable black/white one-pixel vertical stripes with no flicker, shimmer, random pixels, or unstable columns.

## Step 11: Terminal-Style LCD Workload Benchmarks

This step extended the LCD benchmark suite from generic dirty rectangles into terminal-shaped workloads. The firmware now measures small character-cell updates, full-width row repaints, and scroll-style redraws made from a sequence of row-height rectangle fills.

These results are important because a PicoCalc text UI will not behave like a full-screen graphics demo. Cursor blink, typed characters, status updates, line redraws, and scroll operations all have different command/payload ratios. The new commands make those costs visible before building a full text renderer.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Continue the display optimization task list by adding the terminal-cell, row, and scroll benchmarks proposed in the previous phase.

**Inferred user intent:** Keep moving through the LCD optimization plan and gather benchmark evidence for realistic PicoCalc terminal workloads.

**Commit (code):** 1414dfd6cd1e676bfa37eb7cf0921e57fb8b676d — "0099: add LCD terminal workload benchmarks"

### What I did

- Added `lcd cellbench [w h loops]` as a terminal-cell-oriented alias of the moving dirty-rectangle benchmark.
- Added `lcd rowbench [h loops]` to repeatedly repaint full-width rows.
- Added `lcd scrollbench [row_h loops]` to simulate a terminal scroll redraw by repainting all rows in row-height chunks for each scroll loop.
- Updated the README command list.
- Built the firmware with ESP-IDF v5.4.2.
- Flashed through the existing `0099_p4_dk_monitor` tmux session.
- Ran:

```text
lcd init
lcd cellbench 8 16 1000
lcd rowbench 16 200
lcd scrollbench 16 20
lcd scrollbench 8 20
lcd pattern checker
status
```

### Why

The previous `rectbench` command measured generic moving rectangles. Terminal work needs more specific shapes:

- character cells, such as 8×16 pixels;
- rows, such as 320×16 pixels;
- scroll redraws, which repaint many rows and expose command overhead across repeated `CASET`/`RASET`/`RAMWR` sequences.

These measurements help decide whether a future text renderer should optimize for single-cell writes, row batching, full-screen redraws, or hardware scroll/window features.

### What worked

- The firmware built and flashed successfully.
- The terminal workload commands completed at actual 80 MHz:

```text
lcd cellbench w=8 h=16 loops=1000 elapsed_ms=829 rects_s=1206 payload_kib_s=301 requested=80000000 actual_khz=80000
lcd rowbench h=16 loops=200 elapsed_ms=366 rows_s=546 payload_kib_s=5464 requested=80000000 actual_khz=80000
lcd scrollbench row_h=16 rows=20 loops=20 elapsed_ms=732 scrolls_s=27 row_updates_s=546 payload_kib_s=5464 requested=80000000 actual_khz=80000
lcd scrollbench row_h=8 rows=40 loops=20 elapsed_ms=1054 scrolls_s=18 row_updates_s=759 payload_kib_s=3795 requested=80000000 actual_khz=80000
```

- The display was left on `lcd pattern checker` for another visual inspection opportunity.

### What didn't work

- No build or runtime failures occurred after the terminal benchmark implementation.
- Earlier monitor scrollback still contains mistyped `lcd rectbenc` commands from manual experimentation; those are unrelated to the new implementation and returned the expected usage error.

### What I learned

- Single 8×16 cell updates reach about 1206 updates/s, but payload throughput is low because command/setup overhead dominates tiny rectangles.
- Full-width 16-pixel row repaint reaches about 546 rows/s and about 5464 KiB/s payload throughput.
- A 20-row, 16-pixel terminal scroll-style redraw reaches about 27 full scroll redraws/s.
- An 8-pixel row height increases row update rate to 759 row updates/s but lowers full scroll redraw rate to 18 scrolls/s because each scroll requires 40 row commands instead of 20.

### What was tricky to build

The key design detail was separating row update rate from scroll redraw rate. A scroll-style redraw is not one SPI transaction; it is many row rectangle updates. Reporting only payload throughput would hide command overhead, so `scrollbench` reports `scrolls_s`, `row_updates_s`, and `payload_kib_s` separately.

Another tricky point is interpreting cellbench results. A character-cell benchmark intentionally sends tiny payloads, so its KiB/s number looks poor even when update rate is useful. For text UI work, updates per second may be a more useful metric than raw payload throughput.

### What warrants a second pair of eyes

- Whether 8×16 is the right default terminal cell size for the future PicoCalc text renderer.
- Whether row-based redraw is good enough, or whether hardware vertical scrolling should be investigated for terminal scroll operations.
- Whether the checker pattern currently on screen is visually stable at actual 80 MHz.

### What should be done in the future

- Ask the user to visually confirm the checker pattern.
- Add a general RGB565 blit path so benchmarks can separate pixel generation from SPI transport.
- Start designing a minimal text renderer using row batching rather than per-cell writes for normal line redraws.
- Investigate ST7365P/ILI9488 vertical scroll commands if terminal scrolling becomes the performance bottleneck.

### Code review instructions

- Review `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`:
  - `cellbench`/`rectbench` shared command path;
  - `rowbench` command;
  - `scrollbench` command and metrics.
- Validate with:

```text
lcd cellbench 8 16 1000
lcd rowbench 16 200
lcd scrollbench 16 20
lcd scrollbench 8 20
```

### Technical details

Accepted terminal workload baseline at actual 80 MHz:

```text
cell 8x16: 1206 updates/s
row 320x16: 546 updates/s
scroll redraw 20 rows of 16 px: 27 scrolls/s
scroll redraw 40 rows of 8 px: 18 scrolls/s
```

The current visual feedback request is the checker pattern left on the display. Please confirm whether it is stable and crisp, with no shimmer, random pixels, or unstable squares.

## Step 12: Row-Batched Pseudo-Text LCD Benchmark

This step recorded the user's confirmation that the previous pattern and terminal benchmark output worked, then added the next practical LCD workload: row-batched pseudo-text rendering. The new path generates glyph-like RGB565 pixels into the existing 32 KiB DMA buffer and sends one complete text row at a time.

This is closer to a real PicoCalc terminal renderer than solid fills or colored rows. It includes CPU-side glyph/pixel generation, command overhead per row, and SPI payload transfer. The result is a useful baseline for deciding whether a future text UI should render row batches, individual cells, or use hardware scroll support.

### Prompt Context

**User prompt (verbatim):** "go ahead, this all works."

**Assistant interpretation:** The user confirmed the current display tests look good and asked to proceed with the next planned LCD workload work.

**Inferred user intent:** Accept the 80 MHz pattern/terminal benchmark baseline and continue toward a text-rendering path suitable for an interactive PicoCalc UI.

**Commit (code):** 749f254162cc774f23eac26dca8272ae8a4fe744 — "0099: add LCD pseudo text benchmark"

### What I did

- Recorded visual confirmation of the previous checker/pattern/terminal benchmark output in `tasks.md`.
- Added pseudo-glyph generation helpers to `0099`.
- Added a row-batched text-row rendering path that writes generated RGB565 pixels into the reusable 32 KiB DMA buffer.
- Added `lcd textbench [cell_w cell_h loops]`.
- Added `lcd text [cell_w cell_h]` to draw one pseudo-text screen and leave it visible.
- Updated the README command list.
- Built, flashed, and benchmarked the firmware.

### Why

Solid fills and row fills estimate transport performance, but a text UI also needs CPU-side glyph expansion. A row-batched pseudo-text benchmark approximates the future renderer shape without committing to a real font yet. It answers whether row-sized glyph generation plus SPI transfer is plausible for an interactive terminal.

### What worked

- Firmware build succeeded.
- Flash through tmux monitor succeeded.
- LCD initialized at actual 80 MHz.
- Pseudo-text benchmarks completed:

```text
lcd textbench cell_w=8 cell_h=16 cols=40 rows=20 loops=20 elapsed_ms=935 screens_s=21 cells_s=17112 payload_kib_s=4278 requested=80000000 actual_khz=80000
lcd textbench cell_w=8 cell_h=8 cols=40 rows=40 loops=20 elapsed_ms=980 screens_s=20 cells_s=32653 payload_kib_s=4081 requested=80000000 actual_khz=80000
lcd text cell_w=8 cell_h=16 cols=40 rows=20 loops=1 elapsed_ms=46 screens_s=21 cells_s=17391 payload_kib_s=4347 requested=80000000 actual_khz=80000
```

### What didn't work

- No build or runtime failure occurred in this step.
- The pseudo-glyphs are not a real font; they are deliberately synthetic and only model glyph-like pixel density and row batching.

### What I learned

- A 40×20 pseudo-text screen at 8×16 cells renders at about 21 screens/s.
- A 40×40 pseudo-text screen at 8×8 cells also renders at about 20 screens/s because it still covers the whole 320×320 display and doubles the row-command count.
- Row-batched text rendering is plausible for interactive UI work, but a production renderer should avoid full-screen redraws for every keypress.
- Dirty row batching plus selective cell updates will likely be the right next architecture.

### What was tricky to build

The pseudo-text row buffer must fit within the 32 KiB DMA chunk. A 320×16 RGB565 row is 10,240 bytes, which fits comfortably. The code therefore generates one row at a time instead of a full 204,800-byte frame. That matches a terminal renderer's natural update unit and avoids allocating a full internal framebuffer.

The benchmark also separates text-like rendering from real font design. The pseudo-glyph function is intentionally not a compatibility layer or font implementation; it is a workload model so performance decisions can be made before choosing the real glyph source.

### What warrants a second pair of eyes

- Whether 8×16 should remain the default PicoCalc text cell size.
- Whether the pseudo-text screen currently left visible is crisp and stable.
- Whether a production renderer should use per-row dirty tracking, per-cell dirty tracking, or hardware scroll commands.

### What should be done in the future

- Replace pseudo-glyph generation with a real bitmap font path.
- Add dirty row/cell tracking around the text renderer.
- Investigate ST7365P/ILI9488 vertical scroll commands for terminal scrolling.
- Consider queued DMA after the real renderer identifies whether row transfer time or CPU glyph generation dominates.

### Code review instructions

- Review `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`:
  - `pseudo_glyph_pixel()`;
  - `lcd_text_row()`;
  - `lcd_text_screen()`;
  - `lcd textbench` / `lcd text` command handling.
- Validate with:

```text
lcd textbench 8 16 20
lcd textbench 8 8 20
lcd text 8 16
```

### Technical details

Current pseudo-text baseline at actual 80 MHz:

```text
40x20 cells, 8x16: 21 screens/s, 17112 cells/s
40x40 cells, 8x8: 20 screens/s, 32653 cells/s
single 40x20 draw: 46 ms
```

The display is currently left on `lcd text 8 16`. Please confirm whether the pseudo-text-like pattern is stable and crisp.
