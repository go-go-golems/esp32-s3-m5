# Changelog

## 2026-01-24

- Initial workspace created


## 2026-01-24

Imported UI specs and wrote keyboard+screen design docs; created implementation task list (commit 99b46ae).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/analysis/01-cardputer-adv-keyboard-matrix-setup.md — Keyboard setup analysis
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/design/01-ui-screen-designs-per-screen.md — Per-screen design spec
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/tasks.md — Implementation tasks


## 2026-01-24

Implemented 0066 keyboard scan task + UI event queue plumbing (commit 99002e2).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp — Event queue integration stub
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp — Keyboard scan and event emission


## 2026-01-24

Implemented keyboard-driven overlay UI in 0066 (menu/pattern/params/color sliders/help) (commit 9e6ae46).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp — Overlay integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp — UI mode machine and rendering


## 2026-01-24

Implemented Presets (RAM slots) and JS Lab (examples + output) on-device overlays (commit 584f586).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp — Presets/JS overlays


## 2026-01-24

Implement web UI multi-screen layout + add /api/frame,/api/js/reset,/api/js/mem,/api/gpio/status; add ticket scripts; flashed to /dev/ttyACM0 (monitor requires TTY). (commits c9b833b, fde611c)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/assets/index.html — Multi-screen web UI scaffold
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp — Web endpoints for frames/JS reset/gpio status
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/scripts/04-web-smoke.sh — Web smoke-test script


## 2026-01-24

Fix keyboard on Cardputer-ADV: replace matrix scanner with TCA8418 (GPIO8/9 + INT11) and make overlays usable via Tab back + HJKL navigation; status bar shows kb:ok/err. (commit 97616a5)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp — ADV keyboard driver using TCA8418 events (commit 97616a5)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp — Tab back + HJKL navigation + kb status display (commit 97616a5)


## 2026-01-24

Fix browser JS parse error in /assets/app.js (remove invalid '(void)reason' and avoid catch-without-binding); add /favicon.ico handler to stop 404 noise. (commit 954da2c)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/assets/app.js — Fix syntax error at scheduleApply()
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp — Serve /favicon.ico


## 2026-01-24

Fix boot abort (I2C driver conflict): M5GFX links legacy i2c driver (driver/i2c.h) which has a startup conflict check; avoid linking driver_ng by switching TCA8418 keyboard code to legacy i2c API. (commit 32e9728)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/tca8418.c — Switch to legacy i2c transactions
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp — Use i2c_driver_install+i2c_master_write_read_device


## 2026-01-24

Web UI improvements: don't overwrite focused inputs during polling (fixes bri/frame fields resetting while typing); add trailing-slash aliases for /api/js/eval,/api/js/reset,/api/js/mem and /ws to avoid 404 from URL variants. Flash currently blocked because /dev/ttyACM0 is held by a python process; close monitor and reflash. (commit 60ac447)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/assets/app.js — Skip status->form sync when input focused
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp — Alias handlers for /api/js/* and /ws

