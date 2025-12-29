# Changelog

## 2025-12-24

- Initial workspace created


## 2025-12-24

Created build-plan analysis + initial diary; linked prior art (0007 keyboard/serial + 0012 typewriter UI) and added initial implementation task list.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/analysis/01-build-plan-cardputer-serial-terminal-keyboard-echo-uart-usb-backend.md — New implementation-ready analysis doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/reference/01-diary.md — New investigation diary with steps and findings
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/tasks.md — Initial task breakdown for implementation


## 2025-12-24

Implemented chapter 0015-cardputer-serial-terminal (keyboard echo to screen + TX to USB-Serial-JTAG or UART via menuconfig); updated docs/tasks to reference 0015.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild — Config knobs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp — Core implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/analysis/01-build-plan-cardputer-serial-terminal-keyboard-echo-uart-usb-backend.md — Updated references to 0015
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/reference/01-diary.md — Recorded implementation step


## 2025-12-24

Refined the serial interface selection to **USB vs GROVE UART (G1/G2)**, added a menuconfig “speed” knob (baud) for GROVE, and guarded against a pin conflict between GROVE UART (GPIO1/2) and keyboard IN0/IN1 autodetect.

**Commit:** c8c6f30e51f0502398bb7dd1f69e6965724b0521

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild — USB vs GROVE selection + GROVE baud + default RX/TX pins
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp — Runtime guard disabling keyboard autodetect when it conflicts with GROVE UART pins


## 2025-12-25

Ticket complete: implemented chapter 0015-cardputer-serial-terminal with USB-Serial-JTAG and GROVE UART backend selection, keyboard echo to screen, and configurable baud rate. Fixed USB driver install to prevent host write timeouts.

