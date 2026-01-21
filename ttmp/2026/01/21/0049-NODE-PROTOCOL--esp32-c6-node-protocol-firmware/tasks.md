# Tasks

## TODO

- [x] Relate protocol sources + prior firmware tickets to design doc
- [x] Write textbook-style ESP32-C6 node firmware design (Wi‑Fi + console + MLED/1)
- [x] Add implementation plan and validation playbook (host + device)
- [x] Upload final analysis to reMarkable

## Implementation (Firmware)

- [x] Create new ESP32-C6 project `0049-xiao-esp32c6-mled-node` (CMake + sdkconfig.defaults)
- [x] Port Wi‑Fi provisioning via `esp_console` (scan/join/save) and print browse IP
- [ ] Implement UDP multicast socket lifecycle (start on got-IP; stop on lost-IP; join group 239.255.32.6:4626)
- [x] Implement MLED/1 wire structs + pack/unpack helpers (header + payloads)
- [x] Implement node protocol task main loop (recv + timeout scheduler) with epoch gating
- [x] Implement `PING`→`PONG` discovery + status snapshot (uptime/rssi/pattern/etc)
- [x] Implement `BEACON` handling (epoch establish + coarse time sync) + optional TIME_REQ refinement
- [x] Implement `CUE_PREPARE` store + `ACK` replies (target filtering, bounded cue store)
- [x] Implement `CUE_FIRE` scheduling (bounded pending list, wrap-around-safe due checks)
- [x] Implement `CUE_CANCEL` (remove cue + pending fires)
- [x] (Optional) Implement `TIME_REQ/TIME_RESP` refinement (RTT + server-proc correction)
- [x] Map `PatternConfig` to an effect engine (log-only state + APPLY logs)
- [ ] Integrate cue application with `0044` WS281x `led_task` (queue boundary)

## Implementation (Host Tools)

- [x] Add Python smoke script to send `PING` and print `PONG` (std-lib only)
- [ ] Extend Python smoke script: `BEACON` + `CUE_PREPARE` + scheduled `CUE_FIRE`

## Validation

- [x] Write/finish playbook for flashing + python smoke test
- [ ] Ask user to flash + run python; record observed results in diary
