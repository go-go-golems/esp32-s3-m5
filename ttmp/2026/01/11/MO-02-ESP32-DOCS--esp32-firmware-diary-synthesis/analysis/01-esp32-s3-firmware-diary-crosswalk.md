---
Title: ESP32-S3 Firmware Diary Crosswalk
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md
      Note: Representative input/console diary
    - Path: esp32-s3-m5/ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/reference/01-diary.md
      Note: Representative web UI diary
    - Path: esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md
      Note: Representative BLE diary
    - Path: esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/01-diary.md
      Note: Representative graphics diary
    - Path: esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md
      Note: Representative Zigbee architecture diary
ExternalSources: []
Summary: Crosswalk of esp32-s3-m5 firmware directories to ticket diaries, plus cross-firmware themes and proposed long-term docs.
LastUpdated: 2026-01-11T18:52:55.273238308-05:00
WhatFor: Plan documentation extraction from esp32-s3-m5 experiments and diaries.
WhenToUse: Use when prioritizing long-term docs or onboarding developers to the firmware catalog.
---


# ESP32-S3 Firmware Diary Crosswalk

## Scope

This document maps esp32-s3-m5 firmware directories to related ticket diaries, then highlights cross-firmware topics and proposes a small set of long-term docs worth extracting.

## Revision

v2: Added granular doc-topic candidates per firmware and per ticket-only diary, plus a tighter shortlist.

## 1) Firmware directories and related diaries (with candidate doc topics)

| Firmware dir | Focus | Related diary tickets | Candidate long-term doc topics (specific) |
| --- | --- | --- | --- |
| `0001-cardputer-hello_world` | Cardputer base bring-up | none found | Cardputer ESP-IDF baseline: `sdkconfig.defaults` + `partitions.csv` + 8MB flash<br>Reproducible build/flash checklist for ESP-IDF 5.4.1 |
| `0002-freertos-queue-demo` | FreeRTOS queue patterns | none found | Producer/consumer queue pipeline with timestamps and log latency |
| `0003-gpio-isr-queue-demo` | GPIO ISR -> queue patterns | none found | GPIO ISR -> queue -> task pattern (minimal ISR work, BOOT button example) |
| `0004-i2c-rolling-average` | I2C sampling + smoothing | none found | I2C polling via `esp_timer` + queue fan-out + rolling average (SHT30/31) |
| `0005-wifi-event-loop` | WiFi event loop | none found | WiFi STA event loop wiring + reconnect logic + disconnect reason codes |
| `0006-wifi-http-client` | HTTP client | none found | ESP-IDF `esp_http_client` GET loop + TLS time sync caveat |
| `0007-cardputer-keyboard-serial` | Keyboard input + serial echo | `esp32-s3-m5/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md` | Cardputer keyboard matrix scan: pins, scan cadence, debounce<br>Realtime serial echo and line-buffered terminal pitfalls |
| `0008-atoms3r-display-animation` | AtomS3R display animation | none found | AtomS3R GC9107 bring-up with `esp_lcd`: offsets, color order, backlight modes |
| `0009-atoms3r-m5gfx-display-animation` | M5GFX display bring-up | `esp32-s3-m5/ttmp/2025/12/22/001-MAKE-GFX-EXAMPLE-WORK--make-gc9107-m5gfx-style-display-example-work-atoms3r/reference/01-diary.md` | AtomS3R GC9107 bring-up with M5GFX + `EXTRA_COMPONENT_DIRS` |
| `0010-atoms3r-m5gfx-canvas-animation` | M5GFX canvas animation | `esp32-s3-m5/ttmp/2025/12/22/002-GFX-CANVAS-EXAMPLE--m5gfx-canvas-animation-example-atoms3r/reference/01-diary.md` | M5GFX `M5Canvas` + DMA `waitDMA()` usage (flicker-free animation)<br>ESP-IDF 5.4 compatibility patch for M5GFX I2C module |
| `0011-cardputer-m5gfx-plasma-animation` | Cardputer plasma animation | `esp32-s3-m5/ttmp/2025/12/22/004-CARDPUTER-M5GFX--cardputer-using-m5gfx-bring-up-plasma-animation/reference/01-diary.md` | Cardputer ST7789 M5GFX autodetect + visible offset notes<br>Stable USB/JTAG flash with by-id path |
| `0012-cardputer-typewriter` | Keyboard-driven UI text | `esp32-s3-m5/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2025/12/23/0011-HOST-DEBUG-COMPILE--host-debug-compile-run-typewriter-on-host-device/reference/01-diary.md` | Keyboard -> on-screen text pipeline (typewriter UI loop)<br>Host-compile workflow to speed iteration |
| `0013-atoms3r-gif-console` | Serial-driven GIF display | `esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/02-diary.md`<br>`esp32-s3-m5/ttmp/2025/12/23/009-INVESTIGATE-GIF-LIBRARIES--investigate-gif-decoding-libraries-for-esp-idf-esp32-atoms3r/reference/01-diary.md` | GIF console architecture: serial control + FATFS asset pack<br>GIF asset pipeline: crop/trim scripts and storage image |
| `0014-atoms3r-animatedgif-single` | Single GIF playback | `esp32-s3-m5/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2025/12/23/009-INVESTIGATE-GIF-LIBRARIES--investigate-gif-decoding-libraries-for-esp-idf-esp32-atoms3r/reference/01-diary.md` | AnimatedGIF (bitbank2) ESP-IDF component integration<br>Single-GIF playback harness: embedded bytes vs partition-mmap |
| `0015-cardputer-serial-terminal` | Serial terminal UX | `esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/reference/01-diary.md` | Dual-transport serial terminal: USB Serial/JTAG vs Grove UART<br>On-screen terminal buffer + echo modes |
| `0016-atoms3r-grove-gpio-signal-tester` | Grove GPIO signal test | `esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2025/12/26/0012-0016-NEWLIB-LOCK-ABORT--0016-abort-in-newlib-lock-acquire-generic-during-gpio-reset-pin/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2025/12/26/0013-GROVE-SERIAL-DEBUG--grove-serial-debug-gpio-spikes-wiring-coupling-analysis/reference/01-diary.md` | Grove GPIO tester REPL: TX/RX modes + UART patterns + pull-ups<br>Signal integrity + GPIO spike debugging + newlib lock abort |
| `0017-atoms3r-web-ui` | Web UI + WebSocket terminal | `esp32-s3-m5/ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/reference/01-diary.md` | Device-hosted UI: SoftAP + HTTP server + WebSocket terminal<br>PNG upload -> FATFS -> display render |
| `0018-qemu-uart-echo-firmware` | QEMU UART echo | `esp32-s3-m5/ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2025/12/29/0017-QEMU-NET-GFX--qemu-esp32-s3-exploration-networking-graphics-project/reference/01-diary.md` | ESP32-S3 QEMU UART RX diagnostics + serial backend choices |
| `0019-cardputer-ble-temp-logger` | BLE temperature logger | `esp32-s3-m5/ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/reference/02-diary.md` | BLE GATT server: service/characteristic layout + notify period control |
| `0020-cardputer-ble-keyboard-host` | BLE keyboard host | `esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md` | BLE HID host: scan, pair, connect, key logging commands |
| `0021-atoms3-memo-website` | Memo website + audio | `esp32-s3-m5/ttmp/2025/12/31/CLINTS-MEMO-WEBSITE--audio-recorder-with-web-control-for-m5atoms3/reference/01-diary.md` | Device-hosted memo website: STA vs SoftAP config + build workflow |
| `0022-cardputer-m5gfx-demo-suite` | Cardputer M5GFX demo suite | `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/02-implementation-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/01/0026-B3-SCREENSHOT-STACKOVERFLOW--cardputer-demo-screenshot-png-send-triggers-stack-overflow-in-main/reference/01-diary.md` | M5GFX demo suite UI scaffolding (menus, HUD/footer)<br>Screenshot-over-serial framing + WDT/stack overflow pitfalls |
| `0023-cardputer-kb-scancode-calibrator` | Keyboard scancode calibration | `esp32-s3-m5/ttmp/2026/01/02/0023-CARDPUTER-KB-SCANCODES--cardputer-keyboard-scancode-calibration-firmware/reference/01-diary.md` | Scancode calibration workflow: collect chords + CFG -> header |
| `0025-cardputer-lvgl-demo` | LVGL demo | `esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md` | LVGL bring-up on Cardputer using M5GFX + `esp_console` commands |
| `0028-cardputer-esp-event-demo` | esp_event demo | `esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md` | `esp_event` fan-out demo with UI event log + keyboard controls |
| `0029-mock-zigbee-http-hub` | Mock Zigbee HTTP hub | `esp32-s3-m5/ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/05/0029a-ADD-WIFI-CONSOLE--add-esp-console-wifi-config-scan-to-mock-zigbee-hub/reference/01-diary.md` | Protobuf HTTP API + WebSocket event stream on ESP-IDF<br>WiFi console commands + NVS credentials flow |
| `0030-cardputer-console-eventbus` | Console + event bus | `esp32-s3-m5/ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/reference/01-diary.md` | `esp_console` REPL that posts into `esp_event` bus + UI log |
| `0031-zigbee-orchestrator` | Zigbee orchestration | `esp32-s3-m5/ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/06/0032-ANALYZE-NCP-FIRMWARE--analyze-ncp-h2-gateway-firmware/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/06/0034-ANALYZE-ESP-ZIGBEE-LIB--analyze-esp-zigbee-lib-low-level-stack/reference/01-diary.md`<br>`esp32-s3-m5/ttmp/2026/01/06/0035-IMPROVE-NCP-LOGGING--improve-ncp-logging-convert-constants-to-strings/reference/01-diary.md` | Zigbee orchestrator contract: async HTTP/WS + event bus + NCP split |
| `0036-cardputer-adv-led-matrix-console` | LED matrix console | `esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md` | MAX7219 LED matrix console: chain config, SPI clock tuning, animations |
| `0037-cardputer-adv-fan-control-console` | Fan control console | `esp32-s3-m5/ttmp/2026/01/07/001-FAN-CONTROL--cardputer-adv-fan-control-gpio3-relay/reference/01-diary.md` | GPIO3 fan relay control: REPL commands + safe patterns |
| `0038-cardputer-adv-serial-terminal` | Advanced serial terminal | none found | Cardputer-ADV TCA8418 keyboard terminal: USB/JTAG vs UART routing |

### Diaries without a firmware directory in esp32-s3-m5 (doc candidates still relevant)

| Ticket diary | Focus | Candidate long-term doc topics (specific) |
| --- | --- | --- |
| `esp32-s3-m5/ttmp/2025/12/21/001-ANALYZE-ECHO-BASE--analyze-echo-base-openai-realtime-embedded-sdk/reference/01-research-diary.md` | OpenAI Realtime SDK analysis | ESP32-S3 WebRTC audio pipeline overview (components, tasks, data flow) |
| `esp32-s3-m5/ttmp/2025/12/21/001-ANALYZE-ECHO-BASE--analyze-echo-base-openai-realtime-embedded-sdk/reference/02-sdk-configuration-research-diary.md` | SDK config research | Annotated `sdkconfig.defaults` for realtime audio (event loop ISR, TLS verification, watchdogs) |
| `esp32-s3-m5/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/01-diary.md` | Tutorial research guide | How to scaffold a new ESP-IDF tutorial project with `sdkconfig.defaults` |
| `esp32-s3-m5/ttmp/2025/12/21/003-ANALYZE-ATOMS3R-USERDEMO--analyze-m5atoms3-userdemo-project/reference/01-diary.md` | AtomS3R user demo analysis | AtomS3R app framework structure and display refresh flow |
| `esp32-s3-m5/ttmp/2025/12/22/007-CARDPUTER-SYNTH--cardputer-synth-speaker-keyboard-driven-audio/reference/01-diary.md` | Audio synth experiment | Cardputer speaker I2S pin map + mic contention notes |
| `esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md` | MicroQuickJS REPL | Porting MicroQuickJS REPL to ESP-IDF + SPIFFS autoload strategy |
| `esp32-s3-m5/ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/reference/01-diary.md` | SPIFFS autoload issue | SPIFFS first-boot format + autoload failure modes and fixes |
| `esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/reference/01-diary.md` | M5PaperS3 EPD drawing | M5PaperS3 EPD update pipeline (M5GFX Panel_EPD, refresh vs redraw) |
| `esp32-s3-m5/ttmp/2026/01/02/0027-ESP-HELPER-TOOLING--esp-helper-tooling-go-debugging-flashing-helper/reference/01-diary.md` | Tooling | Flash/monitor tmux workflows + stable serial port selection |
| `esp32-s3-m5/ttmp/2026/01/06/0033-ANALYZE-IDF-PY--analyze-esp-idf-idf-py-build-tool/reference/01-diary.md` | IDF build tooling | `idf.py` extension system + command/task graph overview |
| `esp32-s3-m5/ttmp/2026/01/11/MO-01-JS-GPIO-EXERCIZER--cardputer-adv-microjs-gpio-exercizer/reference/01-diary.md` | MicroJS GPIO exercizer | JS-driven GPIO/protocol exercizer architecture + signal plan VM |

## 2) Cross-firmware doc topics (granular, didactic)

- Cardputer keyboard matrix scanning: pins, scan cadence, debounce, Fn combos (0007, 0012, 0023, 0038)
- USB Serial/JTAG `esp_console` setup: config flags, REPL conflicts, recovery steps (0029, 0030, 0037, 0038)
- `esp_event` UI log pattern: multi-producer events with keyboard controls (0028, 0030)
- AtomS3R GC9107 bring-up with `esp_lcd`: offsets, color order, backlight modes (0008)
- M5GFX bring-up on AtomS3R/Cardputer: board autodetect, offsets, `EXTRA_COMPONENT_DIRS` (0009, 0011)
- M5GFX canvas/sprite DMA patterns: when to `waitDMA()` for flicker-free animation (0010, 0011)
- GIF pipeline on ESP32-S3: AnimatedGIF component + FATFS assets + crop/trim scripts (0013, 0014)
- Serial terminal dual backend: USB Serial/JTAG vs Grove UART routing + on-screen buffer (0015, 0038)
- ESP-IDF WiFi STA + HTTP client loop + TLS time sync caveat (0005, 0006)
- Device-hosted UI pattern: SoftAP/STA + HTTP server + WebSocket streams + upload (0017, 0021, 0029)
- BLE recipes: GATT notify server + HID host pairing flow (0019, 0020)
- ESP32-S3 QEMU UART RX diagnostics + serial backend choices (0018 + QEMU diaries)
- Screenshot-over-serial framing + failure modes (0022 diaries, 0025 tools)
- MAX7219 LED matrix console: chain config, SPI clock tuning, animations (0036)
- GPIO3 relay control patterns + safe defaults (0037)
- Zigbee orchestrator contract: async HTTP/WS + event bus + NCP split (0031 + 0032-0035)
- M5PaperS3 EPD refresh pipeline with M5GFX Panel_EPD (M5PS3 EPD diary)
- ESP-IDF build tooling map: `idf.py` extension architecture + tmux flash/monitor patterns (0033, 0027)

## 3) Proposed long-term docs to extract (shortlist v2)

- Cardputer keyboard input guide: GPIO matrix scan + scancode calibration + Fn combos (0007, 0023, 0038)
- USB Serial/JTAG console guide: `esp_console` config + REPL conflict checklist (0029, 0030, 0038)
- AtomS3R GC9107 display bring-up with `esp_lcd` (offsets/backlight/color order) (0008)
- M5GFX bring-up + DMA-safe canvas rendering on AtomS3R/Cardputer (0009-0011)
- GIF asset pipeline on ESP32-S3: AnimatedGIF component + FATFS pack + conversion scripts (0013, 0014)
- Cardputer serial terminal dual-backend (USB/JTAG vs Grove UART) with on-screen buffer (0015, 0038)
- Device-hosted UI pattern: SoftAP/STA + HTTP + WebSocket + upload (0017, 0021, 0029)
- `esp_event`-driven UI log pattern with REPL injection (0028, 0030)
- BLE recipes: GATT notify server + HID host pairing flow (0019, 0020)
- Zigbee orchestrator contract: async HTTP/WS + NCP split + event bus (0031 + 0032-0035)
- Cardputer audio output: I2S speaker pins + keyboard-to-audio synth flow (007 diary)
- M5PaperS3 EPD refresh pipeline with M5GFX Panel_EPD (M5PS3 EPD diary)
