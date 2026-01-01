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


## 2025-12-29

Checked off completed meta-tasks (analysis docs, build wrapper, qemu-xtensa install, and spin-off bug tickets 0015 + 0016). Leaving Cardputer port implementation tasks open/deferred.


## 2025-12-31

Backfilled diary with stdlib generator provenance (+ -m32 plan), wrote C++ REPL split design + stdlib analysis docs, uploaded PDFs to reMarkable, and added tasks for REPL-only milestone + esp32 stdlib generation.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/analysis/03-microquickjs-stdlib-atom-table-split-why-var-should-parse-current-state-ideal-structure.md — Recorded where 64-bit stdlib comes from and how to generate 32-bit via -m32
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md — Design for C++ split and RepeatEvaluator-first REPL-only bring-up
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md — Added Steps 7-9 with commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/tasks.md — Added task breakdown for REPL-only firmware and esp32 stdlib generation


## 2025-12-31

Step 10: Cardputer flash/CPU defaults + partition sizing (commit 881a761)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/partitions.csv — Enlarge app/storage partitions to Cardputer-friendly sizes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/sdkconfig.defaults — Board defaults to avoid 2MB flash partition-table failures


## 2025-12-31

Step 11: start C++ split + REPL-only RepeatEvaluator bring-up (commit 1a25a10)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/CMakeLists.txt — Switch main component to new C++ modules
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp — REPL loop now dispatches to RepeatEvaluator and supports :help/:mode/:prompt


## 2025-12-31

Step 12: add tmux smoke tests for RepeatEvaluator REPL (commit 5aeb539)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh — Automates device monitor-driven REPL echo checks
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh — Automates QEMU monitor-driven REPL echo checks


## 2025-12-31

Step 13: tmux QEMU script sees prompt but no input echo (still 0015) (commit 167c629)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh — Same harness for device bring-up once hardware is connected
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh — Now reliably waits for prompt; still fails at :mode due to missing QEMU UART RX


## 2025-12-31

Step 14: UART-direct QEMU tests still show no RX echo (commit bf99dc4)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh — Proves missing input echo even with QEMU UART on stdio
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh — Proves missing input echo even without idf_monitor

