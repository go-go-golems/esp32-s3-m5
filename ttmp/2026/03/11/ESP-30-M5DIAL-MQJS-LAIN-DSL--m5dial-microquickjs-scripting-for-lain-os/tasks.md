# Tasks

## Research and design

- [x] Create ticket `ESP-30-M5DIAL-MQJS-LAIN-DSL`
- [x] Map the current 0074 firmware/server/web architecture
- [x] Inspect the existing `0048-cardputer-js-web` MicroQuickJS integration
- [x] Write the primary analysis / design / implementation guide
- [x] Write the implementation diary

## Recommended implementation phases

- [x] Add `mquickjs` and `mqjs_service` dependencies to `0074-m5dial-web-remote/firmware`
- [x] Create a dedicated JS service module in the 0074 firmware
- [x] Define a Lain OS JS DSL layer that flushes queued commands/logs/events out of the VM into firmware-owned queues
- [x] Replace the narrow `RemoteUiCommand` path with a broader app-command bus that can be fed by both websocket commands and JS runtime output
- [x] Add websocket message types for script execution requests and results
- [x] Extend the Go hub to route `script_eval` frames to the selected device
- [x] Extend the Go hub to broadcast `script_result`, `script_console`, and `script_event` frames back to browsers
- [x] Add a firmware console command to enable/disable remote script execution
- [ ] Add watchdog, timeout, and memory-budget guardrails to the JS service
- [ ] Validate the end-to-end flow on hardware with `/dev/ttyACM0`

## Explicitly out of scope for system implementation

- [ ] Browser-side script editor and interaction design owned by UX/frontend
- [ ] Browser presentation of script status, logs, and errors owned by UX/frontend
- [ ] Visual affordances for script presets, history, and layout owned by UX/frontend

## Explicit open questions to resolve during implementation

- [x] Whether remote script execution should default to disabled
- [ ] Whether scripts may mutate station definitions persistently or only in RAM
- [ ] Whether multi-line reveal text should be a first-class device primitive
- [x] Whether the protocol accepts raw source only in v1 or also named script references later
