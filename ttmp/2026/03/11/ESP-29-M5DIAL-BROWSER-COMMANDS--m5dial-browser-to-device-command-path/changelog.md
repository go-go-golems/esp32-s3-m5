# Changelog

## 2026-03-11

- Initial workspace created
- Firmware command queue, inbound `ui_command` parsing, and `ui_command_ack` support landed in commit `c549955`.
- Go hub device routing, browser `ui_command_result`, and per-socket write serialization landed in commit `69a922f`.
- React command controls, browser feedback handling, and the refreshed embedded bundle landed in commit `f9857ea`.
- Live validation succeeded with device `m5dial-b76a94`; request `9001` received `ui_command_result=queued` and `ui_command_ack=applied`.
