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

