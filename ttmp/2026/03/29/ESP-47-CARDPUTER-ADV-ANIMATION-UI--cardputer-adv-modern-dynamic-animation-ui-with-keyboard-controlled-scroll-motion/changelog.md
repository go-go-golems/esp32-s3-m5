# Changelog

## 2026-03-29

- Initial workspace created


## 2026-03-29

Imported the retro Mac minimap donor, analyzed 0066/0022/0030 firmware patterns, and wrote the detailed intern-facing design guide plus investigation diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp — Scroll-state handling reference
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp — ADV input reference
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/imports/retro_macos_line_minimap.html — Imported donor prototype
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/reference/01-investigation-diary.md — Chronological investigation record


## 2026-03-29

Validated the ticket with docmgr doctor, completed the reMarkable dry-run and upload, and verified the remote listing.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/changelog.md — Delivery history
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/design-doc/01-cardputer-adv-dynamic-animation-ui-analysis-design-and-implementation-guide.md — Uploaded in the final bundle
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/tasks.md — Upload task marked complete


## 2026-03-29

Scaffolded and built the first implementation of `0083-cardputer-adv-animation-ui`, including the ADV-aware keyboard task, animated scroll model, full-screen canvas renderer, and tmux-oriented build helper. Recorded in commit `44798eb`.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0083-cardputer-adv-animation-ui/main/app_main.cpp — Main render loop and display ownership
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0083-cardputer-adv-animation-ui/main/ui_kb.cpp — ADV semantic keyboard layer
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0083-cardputer-adv-animation-ui/main/ui_model.cpp — Animated scroll state and motion rules
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0083-cardputer-adv-animation-ui/main/ui_render.cpp — Minimap, scrollbar, and viewport rendering
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0083-cardputer-adv-animation-ui/build.sh — tmux flash/monitor helper
