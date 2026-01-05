# Tasks

## TODO

- [ ] Add tasks here

- [x] Define event base/IDs + payload contracts (commands vs notifications)
- [x] Design HTTP API (devices, commands, scenes, events stream) + example payloads
- [x] Decide event-stream transport (SSE vs WS) and backpressure strategy
- [ ] Plan persistence: device registry in-memory + NVS snapshot/debounce
- [x] Implement firmware skeleton (WiFi connect, http server, app event loop)
- [x] Implement device_sim (timers) + registry + scenes + SSE/WS event stream
- [ ] [Phase 3] Validate protobuf WS stream (multi-client, backpressure, drops)
- [x] [Phase 1] Disable WebSocket event stream (compile-time) and remove JSON broadcast hot path
- [x] [Phase 1] Clean up concurrent console output (minimize non-console printing; make hub quiet by default)
- [x] [Phase 1] Validate WiFi console + HTTP API without WS (curl smoke)
- [x] [Phase 2] Add nanopb toolchain component (FetchContent + early-expansion guard)
- [x] [Phase 2] Define hub_bus.proto mirroring hub_types.h payloads (bounded fields)
- [x] [Phase 2] Implement embedded hub->protobuf encoder + console dump command (no WS)
- [x] [Phase 2] Validate encoding with real hub events (hex dump + size)
- [x] [Phase 3] Remove JSON codepaths entirely; implement protobuf binary WS endpoint
- [x] [Phase 3] Decouple WS sending from hub event loop (queue + drop counters)
