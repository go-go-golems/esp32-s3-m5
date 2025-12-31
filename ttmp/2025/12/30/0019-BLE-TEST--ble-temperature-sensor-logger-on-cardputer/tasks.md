# Tasks

## TODO

- [x] Add tasks here

- [x] Decide firmware project location (new ESP-IDF app under esp32-s3-m5/) and pin ESP-IDF version (Cardputer baseline is v5.4.1)
- [x] Implement BLE bring-up (Bluedroid): NVS init, controller init/enable (BLE-only), bluedroid init/enable, register GAP + GATTS callbacks, start advertising.
- [x] Define custom GATT service for temp logger: Service UUID + Temperature characteristic (READ + NOTIFY) + optional Control characteristic (WRITE) for rate/enable, and CCCD handling (subscribe/unsubscribe).
- [x] Mock sensor v1: implement a deterministic temperature source (e.g., ramp/sine) behind a sensor interface; update value at 1 Hz; send notifications only when subscribed; ensure read returns latest value.
- [x] Add firmware instrumentation: structured EVT logs (adv/start, connect, mtu, subscribe, notify rc), health counters (connects, disconnects, notifies_sent/fail, heap min free, largest block), periodic stat dump.
- [x] Add interactive controls: UART commands or BLE control characteristic to set notify rate, toggle mock sensor, force disconnect, dump counters.
- [x] Host-side client: add a small Python bleak script (scan by name, connect, read, start_notify, print decoded temperature). Include modes: --scan, --connect-once, --subscribe --duration, --stress --reconnects.
- [x] End-to-end bring-up with mock sensor: verify discovery, connect, reads, notifications at 1 Hz; capture logs and btmon traces; document known-good baseline commands.
- [ ] Stress testing with mock sensor: 100 reconnects; 30 min streaming; disconnect/reconnect while notifying; verify no heap regression (min free stable) and notify_fail stays 0.
- [ ] Hardware research: verify Cardputer I2C pins + voltage/pullups; decide sensor module (SHT30/SHT31) and address (0x44/0x45).
- [ ] Implement real sensor driver (SHT3x): I2C init + probe + read + CRC check; expose as same sensor interface used by mock.
- [ ] Swap mock→real: stream real temperature; decide behavior when sensor missing (fail fast vs fallback to mock) and log explicitly.
- [ ] Finalize: update docs (ticket index + analysis links), add a short playbook for flashing/monitoring + Linux client usage, and record results (baseline + stress).
