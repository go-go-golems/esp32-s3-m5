# Changelog

## 2025-12-29

- Initial workspace created


## 2025-12-29

Created analysis documents and diary. Analyzed current firmware configuration and Cardputer port requirements. Identified key differences: console interface (UART vs USB Serial JTAG), flash size (2MB vs 8MB), partition table, and memory constraints.


## 2025-12-29

Added 8 tasks to track porting work: QEMU testing, implementation plan, configuration changes, code migration, memory verification, hardware testing, and optional enhancements


## 2025-12-29

Added MicroQuickJS native extension playbook + FreeRTOS multi-VM brainstorm; updated diary with research Steps 5-6

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/01-microquickjs-freertos-multi-vm-multi-task-architecture-communication-brainstorm.md — New design brainstorm
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md — Diary updated with new research steps
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md — New reference manual


## 2025-12-29

Validated QEMU boot reaches js> prompt after fixing partition-table config; discovered that interactive input does not reach the REPL (serial RX issue). Spinning off dedicated bug ticket for investigation.


## 2025-12-29

Closing as blocked on QEMU/serial input bug; investigation moved to new bug ticket.

