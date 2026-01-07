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

Add matrix spi <hz> command to change MAX7219 SPI clock at runtime; lower default SPI clock to 100kHz

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Add 'matrix spi' command and show spi_hz in status
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c — Add close/reopen + set_spi_hz to reconfigure SPI clock

