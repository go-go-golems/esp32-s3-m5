# Tasks

## TODO

### Milestone: “pair a keyboard, type, see keys over serial”

- [ ] Create new firmware project `esp32-s3-m5/0020-cardputer-ble-keyboard-host/` (CMakeLists, `main/`, `sdkconfig.defaults`, `partitions.csv`)
- [ ] Enable required SDK config defaults:
  - [ ] `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` (and disable UART/USB-CDC consoles)
  - [ ] Bluedroid BLE + GATTC enabled (Central mode)
  - [ ] Enable/verify `esp_hid` host component is included in build
- [ ] Implement BLE bring-up (NVS → controller → bluedroid → callbacks) in `ble_host.c`
  - [ ] `esp_ble_gap_register_callback(gap_cb)`
  - [ ] `esp_ble_gattc_register_callback(gattc_cb)` + `esp_ble_gattc_app_register(APP_ID)`
- [ ] Implement “device registry” for scan results (bounded list + last-seen + addr type + RSSI + name)
  - [ ] Parse ADV name via `esp_ble_resolve_adv_data_by_type(...)`
  - [ ] Add simple “kbd?” heuristic (appearance/service UUID when available)
- [ ] Implement scanning control:
  - [ ] `scan on [seconds]` starts scan (`esp_ble_gap_set_scan_params` + `esp_ble_gap_start_scanning`)
  - [ ] `scan off` stops scan (`esp_ble_gap_stop_scanning`)
  - [ ] `devices` prints the registry in a stable table format
- [ ] Integrate ESP-IDF HID Host (`esp_hidh`) in `hid_host.c`
  - [ ] `esp_hidh_init()` with an `esp_event_handler_t` callback for `ESP_HIDH_EVENTS`
  - [ ] In `gattc_cb`, forward events to HID host via `esp_hidh_gattc_event_handler(...)`
  - [ ] Handle `ESP_HIDH_OPEN_EVENT`, `ESP_HIDH_INPUT_EVENT`, `ESP_HIDH_CLOSE_EVENT` (`esp_hidh_dev_free(dev)` on close)
- [ ] Implement connect/disconnect:
  - [ ] `connect <index|addr>` calls `esp_hidh_dev_open(bda, ESP_HID_TRANSPORT_BLE, addr_type)`
  - [ ] `disconnect [addr]` calls `esp_hidh_dev_close(dev)` (plus cleanup state)
- [ ] Implement pairing + bonds:
  - [ ] Set baseline security policy via `esp_ble_gap_set_security_param(...)` (bonding + SC policy)
  - [ ] `pair <addr>` initiates encryption/pairing (`esp_ble_set_encryption(...)` or equivalent flow)
  - [ ] `bonds` lists bonded devices (`esp_ble_get_bond_device_num`, `esp_ble_get_bond_device_list`)
  - [ ] `unpair <addr>` removes bond (`esp_ble_remove_bond_device`) and disconnects if needed
  - [ ] Support interactive security prompts:
    - [ ] `sec-accept <addr> yes|no` → `esp_ble_gap_security_rsp(...)`
    - [ ] `passkey <addr> <6digits>` → `esp_ble_passkey_reply(...)`
    - [ ] `confirm <addr> yes|no` → `esp_ble_confirm_reply(...)`
- [ ] Implement key logging to serial:
  - [ ] Add a `keylog on|off` command to gate output spam
  - [ ] Parse common boot keyboard reports (modifiers + 6 keycodes) and print:
    - [ ] raw report dump (hex) for debugging
    - [ ] decoded key events (down/up)
    - [ ] optional ASCII when mapping is possible (US layout as first pass)
  - [ ] Avoid printing in BLE callbacks directly: enqueue key events to a logger task/queue
- [ ] Implement `info` command:
  - [ ] Current state (scanning/connecting/connected)
  - [ ] Selected device (addr, addr_type, name, bonded?, trusted?)
  - [ ] Security last result (auth complete status)
- [ ] Implement “trust” policy (optional for milestone, but useful):
  - [ ] `trust <addr>` / `untrust <addr>` sets an NVS flag keyed by address
  - [ ] Auto-reconnect only if trusted
- [ ] Write a minimal playbook for humans in `playbooks/`:
  - [ ] Flash + monitor commands
  - [ ] Example session: `scan on`, `devices`, `connect`, `pair`, `keylog on`
