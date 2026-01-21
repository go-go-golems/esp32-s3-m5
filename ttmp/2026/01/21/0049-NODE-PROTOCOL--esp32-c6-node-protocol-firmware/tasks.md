# Tasks

## TODO

- [x] Relate protocol sources + prior firmware tickets to design doc
- [x] Write textbook-style ESP32-C6 node firmware design (Wi‑Fi + console + MLED/1)
- [x] Add implementation plan and validation playbook (host + device)
- [x] Upload final analysis to reMarkable

## Implementation (Firmware)

- [ ] Create new ESP32-C6 project `0049-xiao-esp32c6-mled-node` (CMake + sdkconfig.defaults)
- [ ] Port Wi‑Fi provisioning via `esp_console` (scan/join/save) and print browse IP
- [ ] Implement UDP multicast socket lifecycle (start on got-IP; stop on lost-IP; join group 239.255.32.6:4626)
- [ ] Implement MLED/1 wire structs + pack/unpack helpers (header + payloads)
- [ ] Implement node protocol task main loop (recv + timeout scheduler) with epoch gating
- [ ] Implement `PING`→`PONG` discovery + status snapshot (uptime/rssi/pattern/etc)
- [ ] Implement `BEACON` handling (epoch establish + coarse time sync) + optional TIME_REQ refinement
- [ ] Implement `CUE_PREPARE` store + `ACK` replies (target filtering, bounded cue store)
- [ ] Implement `CUE_FIRE` scheduling (bounded pending list, wrap-around-safe due checks)
- [ ] Implement `CUE_CANCEL` (remove cue + pending fires)
- [ ] (Optional) Implement `TIME_REQ/TIME_RESP` refinement (RTT + server-proc correction, min-of-N heuristic)
- [ ] Map `PatternConfig` to an effect engine (initially log-only; later integrate `0044` LED task)

## Implementation (Host Tools)

- [ ] Add Python smoke script to send `PING` and print `PONG` (and optionally send `BEACON` + cue prepare/fire)

## Validation

- [ ] Write/finish playbook for flashing + python smoke test
- [ ] Ask user to flash + run python; record observed results in diary
