# Changelog

## 2026-01-19

- Initial workspace created


## 2026-01-19

Step 1: create ticket + document wiring plan (commit 4966e5b)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/design-doc/01-ws2811-on-xiao-esp32c6-d1-wiring-firmware-plan.md — Wiring + firmware plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/reference/01-diary.md — Diary step 1


## 2026-01-19

Step 2: add WS2811 RMT firmware + build (commits 0ef3fbd, 69b78bd)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/main.c — Pattern + startup logs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c — RMT encoder
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/reference/01-diary.md — Diary step 2


## 2026-01-19

Step 3: add periodic serial log inside loop (commit 86290dd)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/main.c — Loop progress logs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/reference/01-diary.md — Diary step 3


## 2026-01-19

Step 4: port rainbow pattern + default 10 LEDs (commit c6dd2a9)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild — Defaults + rainbow knobs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/main.c — Rainbow rendering loop
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/reference/01-diary.md — Diary Step 4

## 2026-01-19

Step 5: smoother/faster rainbow + intensity modulation (commit 852e8af)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild — New animation/intensity knobs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/main.c — Frame timing + intensity modulation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/reference/01-diary.md — Diary Step 5

