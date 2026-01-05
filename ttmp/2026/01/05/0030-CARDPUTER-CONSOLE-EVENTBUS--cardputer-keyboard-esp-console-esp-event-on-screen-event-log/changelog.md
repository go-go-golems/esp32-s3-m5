# Changelog

## 2026-01-05

- Initial workspace created


## 2026-01-05

Implement new Cardputer demo combining keyboard -> esp_event, esp_console REPL -> esp_event, and on-screen event log (commit 0a0ea42).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp — Core demo implementation.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/sdkconfig.defaults — Defaults enabling USB-Serial-JTAG console.


## 2026-01-05

Ticket closed


## 2026-01-05

Add console event monitor (evt monitor on|off) to mirror bus events to the console; document reusable esp_event-centric patterns for C/C++ components.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp — Event monitor queue + command.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/02-pattern-esp-event-centric-architecture-c-and-c.md — Reusable architecture writeup.

