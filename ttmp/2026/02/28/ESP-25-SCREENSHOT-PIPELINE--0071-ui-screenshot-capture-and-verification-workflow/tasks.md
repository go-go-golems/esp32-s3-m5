# Tasks

## TODO

- [x] `T01` Capture current failure mode and choose QOI as replacement for on-device PNG
- [x] `T02` Implement on-device QOI encoder + USB Serial/JTAG framing (`QOI_BEGIN/QOI_END`)
- [x] `T03` Switch console screenshot command and runtime wiring from PNG to QOI path
- [x] `T04` Replace host capture script to read framed QOI bytes and write `.qoi`
- [x] `T05` Add host-side QOI decode helper output for visual verification (PPM/PNG when available)
- [x] `T06` Validate on hardware: command succeeds, file produced, no watchdog/reset
- [x] `T07` Use captured output to verify UI scrollbar behavior and settings/IP visibility
- [x] `T08` Update ticket docs/diary/changelog and commit QOI migration in focused commits
