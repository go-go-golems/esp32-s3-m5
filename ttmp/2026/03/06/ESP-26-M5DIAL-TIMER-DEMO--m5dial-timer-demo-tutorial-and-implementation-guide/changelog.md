# Changelog

## 2026-03-06

- Initial workspace created


## 2026-03-06

Created a detailed M5Dial timer demo design guide and investigation diary, combining M5Dial-UserDemo board knowledge with modern esp32-s3-m5 LVGL and timer patterns.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/hal.cpp — Hardware reference for the proposed tutorial
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp — LVGL porting reference
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/timer_engine.cpp — Timer engine reference
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-26-M5DIAL-TIMER-DEMO--m5dial-timer-demo-tutorial-and-implementation-guide/design-doc/01-m5dial-timer-demo-analysis-design-and-implementation-guide.md — Primary design deliverable


## 2026-03-06

Validated the ticket with docmgr doctor and uploaded the full guide bundle to reMarkable under the dated ESP-26 folder.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-26-M5DIAL-TIMER-DEMO--m5dial-timer-demo-tutorial-and-implementation-guide/design-doc/01-m5dial-timer-demo-analysis-design-and-implementation-guide.md — Uploaded design guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-26-M5DIAL-TIMER-DEMO--m5dial-timer-demo-tutorial-and-implementation-guide/index.md — Bundle entry document
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-26-M5DIAL-TIMER-DEMO--m5dial-timer-demo-tutorial-and-implementation-guide/reference/01-investigation-diary.md — Uploaded diary


## 2026-03-06

Expanded the ticket task list from high-level setup items into a detailed implementation checklist for the future 0072 M5Dial timer demo.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-26-M5DIAL-TIMER-DEMO--m5dial-timer-demo-tutorial-and-implementation-guide/tasks.md — Detailed implementation checklist for executing the guide


## 2026-03-06

Step 2: scaffolded 0072-m5dial-timer-demo, added initial M5Dial board wrapper, and verified the first clean esp32s3 build under IDF 5.4.1 (commit b39be1f)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp — Smoke-test firmware used for the first hardware bring-up pass
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp — Board layer introduced for the new tutorial


## 2026-03-06

Step 3: implemented the first usable LVGL M5Dial timer demo, fixed the watchdog-starvation loop shape, and hardware-validated clean boot on /dev/ttyACM0 (commit f91abc2)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp — Application loop moved to a dedicated task and wired to the timer UI
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp — New round-screen timer presentation and state-driven styling


## 2026-03-06

Step 4: stabilized the M5Dial panel path by aligning the board wrapper with the known-good display config, then replaced the software encoder decoder with a PCNT-backed path that feels materially better on hardware (commit d04e6e0)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp — Display wrapper corrections and hardware-backed encoder migration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.h — Board state updated to own the PCNT-backed encoder
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/CMakeLists.txt — Reused encoder/button support sources compiled into the new tutorial


## 2026-03-06

Step 5: added FT3267 touch-swipe handling, mapped gestures to theme cycling on the round timer UI, and restored the project-level LovyanGFX legacy-I2C define after the first touch build reintroduced the old/new driver conflict (commit 1a9e006)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/CMakeLists.txt — Restores the LovyanGFX legacy-I2C build define required for clean boot on IDF 5.4
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp — FT3267 initialization, raw touch reads, and swipe detection
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp — Maps swipe events into theme changes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp — Adds multiple visible color themes for touch-swipe cycling

## 2026-03-06

Step 7: reduced visible timer-screen tearing by enabling LVGL double buffering and cutting unnecessary redraw work (commit a1c6dc3)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp — Enables double buffering and larger draw stripes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp — Quantizes label and arc updates to reduce panel churn
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.h — Stores cached visible state for redraw suppression


## 2026-03-06

Step 6: split hardware polling from LVGL with a queue-backed input event bus and ISR-assisted button wakeups (commit 2a4ddba)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp — Creates the dedicated I/O and UI tasks
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h — Introduces the normalized event payload used across the refactor
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp — Posts queue events and uses ISR wakeups for the center button


## 2026-03-06

Step 8: reverted the LVGL tearing mitigation after the user rejected the RAM and update-style tradeoff; rebuilt and reflashed the simpler rendering path (commit 12a7fb9)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp — Restores the original single-buffer LVGL configuration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp — Restores unconditional timer-screen updates from before Step 7
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.h — Removes cached redraw state introduced by the tearing experiment

