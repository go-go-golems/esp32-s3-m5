# Changelog

## 2026-06-25

- Initial workspace created


## 2026-06-25

Created PicoOS supervisor architecture ticket and wrote intern-facing design/implementation guide for launcher, live scheduler, app switching, input routing, and REPL integration

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/design-doc/01-picoos-supervisor-design-and-implementation-guide.md — Primary supervisor design
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/reference/01-implementation-diary.md — Diary for design creation


## 2026-06-25

Validated PicoOS supervisor ticket with docmgr doctor and uploaded guide+diary bundle to reMarkable at /ai/2026/06/25/0102-PICOOS-SUPERVISOR

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/design-doc/01-picoos-supervisor-design-and-implementation-guide.md — Uploaded primary design
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/reference/01-implementation-diary.md — Uploaded diary


## 2026-06-25

Phase 1 implemented: added picoos_core supervisor skeleton, built-in app registry, picoos status/apps commands, and passing hardware probe (commit ac906dc)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Console integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picoos_core/include/picoos_core.h — Supervisor public API
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picoos_core/picoos_core.cpp — Supervisor registry/status implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/01-supervisor-phase1-probe.py — Validation probe


## 2026-06-25

Phase 2 implemented: picoos launch/launcher/repl commands now launch registered apps and track active surface/app; hardware launch probe passed (commit c687e03)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Console integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picoos_core/picoos_core.cpp — Launch implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/02-supervisor-launch-probe.py — Validation probe


## 2026-06-25

Phase 3 implemented: PicoOS live frame pump with start/stop/frame commands; Snake advances without serial frame commands and frame-pump hardware probe passed (commit b5378d1)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Console/render callback integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picoos_core/picoos_core.cpp — Frame pump implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/03-supervisor-frame-pump-probe.py — Validation probe


## 2026-06-25

Phase 4 implemented: PicoOS semantic input routing with global home/escape handling, physical keyboard routing, picoos key command, and passing input-router probe (commit e409fda)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Keyboard task integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picoos_core/picoos_core.cpp — Key routing implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/04-supervisor-input-router-probe.py — Validation probe


## 2026-06-25

Added visual REPL slash commands for PicoOS app control and mapped Break/Shift+Esc to escape-to-REPL; slash-command hardware probe passed

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Slash commands and key mapping
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/05-repl-slash-commands-probe.py — Validation probe


## 2026-06-25

Promoted ad-hoc /tmp launch-repro and serial crash-logger scripts into the ticket scripts directory, including a tmux wrapper using a fresh tmux server

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/06-repl-launch-crash-repro.py — Crash repro
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/07-serial-crash-logger.py — Serial logger
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/08-start-serial-crash-logger-tmux.sh — Tmux logger wrapper


## 2026-06-25

Increased keyboard task stack from 4096 to 12288 words after physical /launch crash report; UART screen-eval repro did not crash, physical validation still pending (commit e1c943e)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Keyboard task stack increase

