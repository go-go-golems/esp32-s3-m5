# Changelog

## 2025-12-26

- Initial workspace created


## 2025-12-26

Add analysis + diary explaining tx-low spikes; correlate user log with 0016 pin reset/apply ordering; add external references for safe GPIO config / glitch avoidance.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-GROVE-SERIAL-DEBUG--grove-serial-debug-gpio-spikes-wiring-coupling-analysis/analysis/01-why-does-tx-low-on-gpio2-create-a-spike-on-gpio1-gpio2-0016-signal-tester-hardware-coupling.md — Updated with log evidence + mitigation guidance
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-GROVE-SERIAL-DEBUG--grove-serial-debug-gpio-spikes-wiring-coupling-analysis/reference/01-diary.md — Recorded investigation steps


## 2025-12-26

Design: propose UART peripheral test modes (uart_tx/uart_rx) for 0016, including verbs (uart baud/map/tx/rx echo) and file touch list.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-GROVE-SERIAL-DEBUG--grove-serial-debug-gpio-spikes-wiring-coupling-analysis/design-doc/01-0016-add-uart-tx-rx-modes-for-grove-uart-functionality-speed-testing.md — UART mode design doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-GROVE-SERIAL-DEBUG--grove-serial-debug-gpio-spikes-wiring-coupling-analysis/reference/01-diary.md — Diary updated with UART design step


## 2025-12-26

Design update: uart_rx buffers bytes (no echo), LCD shows buffered count, add REPL verbs uart rx get/clear; lock uart tx stop stays in uart_tx.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-GROVE-SERIAL-DEBUG--grove-serial-debug-gpio-spikes-wiring-coupling-analysis/design-doc/01-0016-add-uart-tx-rx-modes-for-grove-uart-functionality-speed-testing.md — Updated UART RX contract
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-GROVE-SERIAL-DEBUG--grove-serial-debug-gpio-spikes-wiring-coupling-analysis/reference/01-diary.md — Recorded Step 4 (UART RX buffer requirement)


## 2025-12-26

Implement 0016 UART test modes (uart_tx/uart_rx) with RX buffering + uart rx get/clear; add 0016 build.sh helper and update 0016 README; build verified via ./build.sh build.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/build.sh — New build helper
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp — New uart verbs and uart modes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/uart_tester.cpp — UART driver + TX/RX tasks + buffering

