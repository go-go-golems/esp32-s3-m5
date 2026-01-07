# Changelog

## 2026-01-06

- Initial workspace created


## 2026-01-06

Add tmux playbook; implement/flash Phase-1 firmware (esp_console over USB Serial/JTAG + 4x MAX7219 chain) and save non-TTY serial validation scripts

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console — New firmware project for MAX7219 bring-up
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/playbook/01-tmux-workflow-build-flash-monitor-esp-console-usb-serial-jtag.md — Repeatable tmux workflow for flash/monitor/REPL
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_send_matrix_commands.py — Validation script for matrix console commands


## 2026-01-06

Improve bring-up UX: slow MAX7219 SPI clock to 1MHz, add matrix blink mode, and make bare 'matrix' print help

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Add blink mode and friendlier command behavior
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c — Lower SPI clock for better signal integrity


## 2026-01-06

Add gamma-mapped brightness control: matrix bright 0..100 + matrix gamma 1.0..3.0 (maps to MAX7219 intensity 0..15)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Add brightness and gamma commands


## 2026-01-06

Step 5: Document how to identify MCU vs MAX7219/clone (espefuse.py vs physical marking)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record chip-identification procedure and limitations


## 2026-01-06

Revert perceptual brightness mapping (matrix bright/gamma) to return to a simpler, more stable baseline

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Remove bright/gamma commands


## 2026-01-06

Fix MAX7219 SPI reliability (max_transfer_sz) and add safe `matrix spi` clock control (commit 98e0a2c)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Add `matrix spi` command and status output
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c — Fix SPI transfer sizing; add device clock retune
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record regression fix


## 2026-01-07

Add RAM-based safe full-on/full-off test (matrix safe) and document that the observed “weird test mode” behavior was power-related (commit 125f6ab)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Add `matrix safe` command
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Recommend `matrix safe` for bring-up
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Backfill power root-cause notes


## 2026-01-07

Add frontmatter for local embedded text/animation reference source (commit 4149a54)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/sources/local/Embedded text blinking patterns.md — Add docmgr frontmatter and summary metadata
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/index.md — Link source from ticket index


## 2026-01-07

Add `matrix text` (4 chars) and `matrix flipv` orientation knob for the 4× 8×8 chain (commit fa2ae1c)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Add 5×7 font renderer + new commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document `matrix text` and `matrix flipv`


## 2026-01-07

Default `flipv` to on and add smooth scrolling text (`matrix scroll`) (commit 1698127)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Default vertical flip; add scroll task/commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document scroll usage and defaults
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record scroll bring-up step


## 2026-01-07

Make MAX7219 chain length configurable (default 12 modules, up to 16) via `matrix chain` (commit 1e11f22)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Chain length config + variable-width commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c — SPI tx sizing and chain max support
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.h — Define max/default chain length
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document `matrix chain` and new defaults
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record 12-module bring-up step

## 2026-01-06

Add drop-bounce + wave text animations via  (commit 8cc47ff)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document anim commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — New animation task + anim commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record animation step
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md — Add+check animation tasks


## 2026-01-06

Add scroll-wave and flipboard animations (commit 0d8cc04)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document new animation commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Add scroll-wave + flipboard commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record Step 14
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md — Add tasks for scroll-wave/flipboard


## 2026-01-06

Fix flipboard timing: hold each word then flip; cycle includes first word (commit 1e715a0)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Flipboard hold/flip segment logic
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record Step 15


## 2026-01-06

Stop other animations when switching between modes (scroll/blink now call stop_animations) and add validation task (commit 640c3fc)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Ensure starting scroll/blink stops other animation tasks
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record Step 16
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md — Add explicit mode-switching validation task


## 2026-01-06

Phase 2: add TCA8418 keyboard input (I2C+INT), kbd console commands, and render typed text to the MAX7219 strip (commit cda10a9)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document keyboard pins and kbd commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Keyboard ISR/queue/task
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/tca8418.c — Minimal TCA8418 register helper over esp_driver_i2c
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record Step 17
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md — Check off Phase 2 implementation tasks and add validation task

## 2026-01-06

Fix TCA8418 init failure by keeping esp_driver_i2c in synchronous mode (trans_queue_depth=0); avoids ops-list overflow during register init

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Disable I2C async mode for keyboard bus
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record Step 18


## 2026-01-06

Add deep-dive analysis doc explaining esp_driver_i2c async mode and why the internal ops list overflowed during TCA8418 init

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/analysis/01-esp-idf-esp-driver-i2c-async-mode-ops-list-overflow-explained.md — Root-cause explanation with diagrams/pseudocode


## 2026-01-06

Keyboard typing now updates active text modes (scroll/anim) live without stopping them; Enter clears (commit 0c22e33)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document typed text feeding into active modes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Route typed text into scroll/text anim state; add scroll mutex for safe live updates
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md — Record Step 19
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md — Check off task 28 and add validation task 29


## 2026-01-06

Step 20: Add spin letters text animation (matrix anim spin) (commit 6a671a8)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md — Document matrix anim spin
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Spin letters implementation


## 2026-01-06

Step 21: Tune drop-bounce speed + bounce height

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Adjust drop sequence and hold/gap timing

