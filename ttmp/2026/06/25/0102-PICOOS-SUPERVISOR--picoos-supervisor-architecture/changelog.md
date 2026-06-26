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

