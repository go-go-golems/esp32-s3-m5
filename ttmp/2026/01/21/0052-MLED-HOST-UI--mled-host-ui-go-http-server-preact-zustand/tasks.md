# Tasks

## TODO

### Go: MLED protocol + engine
- [ ] Create `esp32-s3-m5/mled-server/` Go module + `cmd/mled-server` entrypoint.
- [ ] Implement MLED/1 codec (header + payload pack/unpack) matching `2026-01-21--mled-web/c/mledc/protocol.h`.
- [ ] Implement u32 show-time helpers (wrap-around-safe diff/duration/isDue) matching `2026-01-21--mled-web/c/mledc/time.c`.
- [ ] Implement UDP multicast transport:
  - bind local port (default `:4626`)
  - join multicast group (default `239.255.32.6`)
  - set TTL=1
  - select outbound interface/bind IP
- [ ] Implement controller epoch/show clock:
  - generate `epoch_id` at startup
  - `msg_id` counter
  - `show_ms()` based on monotonic clock, masked to u32
- [ ] Implement receive loop that:
  - parses datagrams and validates header/payload lengths
  - handles `PONG`, `ACK`, `TIME_REQ` (reply `TIME_RESP` to sender ip:port)
  - maintains node map keyed by `node_id`
- [ ] Implement node status tracking:
  - `last_seen`, `rssi_dbm`, `pattern_type`, `brightness_pct`, `uptime_ms`
  - derived status `online/weak/offline` per imported UI rules
  - offline sweeper ticker emitting `node.offline` events
- [ ] Implement “apply” operation as a protocol-compatible mapping:
  - per-target `CUE_PREPARE` (with optional `ACK_REQ`)
  - scheduled multicast `CUE_FIRE` with `lead_time` and `repeat` retransmits

### Go: HTTP + WS API (match imported spec)
- [ ] Implement HTTP server (default `localhost:8080`):
  - `GET /api/nodes` (list nodes)
  - `GET /api/presets` (list presets)
  - `POST /api/presets` (create)
  - `PUT /api/presets/:id` (update)
  - `DELETE /api/presets/:id` (delete)
  - `POST /api/apply` (apply PatternConfig to selected nodes / all)
  - `GET /api/settings` (read)
  - `PUT /api/settings` (update: bind ip, multicast, intervals, thresholds)
  - `GET /api/status` (epoch/show time + health)
- [ ] Implement WebSocket server at `GET /ws` sending events:
  - `node.update` (on PONG / status change)
  - `node.offline` (on offline transition)
  - `apply.ack` (when an ACK received for a tracked prepare)
  - `error` (server-side errors)
- [ ] Implement minimal persistence:
  - presets JSON file in configurable `--data-dir` (default `./var/`)
  - settings JSON file (same dir)

### Go: validation
- [ ] Unit tests for codec sizes and sample pack/unpack round-trips.
- [ ] Unit tests for u32 time helper behavior around wrap-around.
- [ ] Local smoke plan:
  - run Python simulator `2026-01-21--mled-web/tests/run_nodes.py`
  - start Go server, hit `POST /api/apply`, observe node updates/acks via `/ws`.
- [ ] Scaffold Go module under esp32-s3-m5/mled-server
- [ ] Implement MLED/1 codec + u32 show-time helpers (match mledc/protocol.h + time.c)
- [ ] Implement UDP engine (multicast join, PONG/ACK/TIME_REQ handling, node cache + offline sweeper)
- [ ] Implement HTTP API + WebSocket events per imported UI spec (/api/* and /ws)
