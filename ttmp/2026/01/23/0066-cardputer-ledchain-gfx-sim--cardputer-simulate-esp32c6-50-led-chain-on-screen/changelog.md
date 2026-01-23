# Changelog

## 2026-01-23

- Initial workspace created


## 2026-01-23

Initial ticket setup + deep-research mapping of WS281x pattern engine, MLED/1 wire config, and Cardputer M5GFX render-loop idioms.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/design-doc/01-deep-research-led-pattern-engine-cardputer-gfx-rendering.md — Analysis document
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/reference/01-diary.md — Research diary


## 2026-01-23

MVP implemented: Cardputer firmware renders 50-LED virtual chain, driven by esp_console over USB Serial/JTAG; flashed and smoke-tested on /dev/ttyACM0 (commits eb39f4b, 78efc2e, c6ac579, 71ca90c, a9e6c4b).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp — Firmware entrypoint
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_console.cpp — Console-driven pattern control
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp — On-screen rendering of 50 LEDs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/scripts/serial_smoke_0066.py — Non-interactive REPL smoke test

