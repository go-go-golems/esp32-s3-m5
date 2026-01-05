# Tasks

## TODO

- [ ] Add tasks here

- [x] Define event base/IDs + payload contracts (commands vs notifications)
- [x] Design HTTP API (devices, commands, scenes, events stream) + example payloads
- [x] Decide event-stream transport (SSE vs WS) and backpressure strategy
- [ ] Plan persistence: device registry in-memory + NVS snapshot/debounce
- [x] Implement firmware skeleton (WiFi connect, http server, app event loop)
- [x] Implement device_sim (timers) + registry + scenes + SSE/WS event stream
