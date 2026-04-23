# Tasks

## TODO

- [ ] Add tasks here

- [x] PHASE 1.1: Create ESP-IDF project structure with idf.py create-project
- [x] PHASE 1.2: Add wifi_provisioning component dependency to idf_component.yml
- [x] PHASE 1.3: Configure sdkconfig for BLE, WiFi provisioning, and Security 1
- [x] PHASE 1.4: Set up partition table for 4MB flash with NVS encryption
- [x] PHASE 1.5: Configure build for ESP32-PICO-D4 target
- [x] PHASE 1.6: Verify clean build succeeds before adding application code
- [ ] PHASE 2.1: Implement app_prov.c - wifi_provisioning manager wrapper
- [ ] PHASE 2.2: Implement provisioning event handler (CRED_RECV, CRED_SUCCESS, CRED_FAIL, END)
- [ ] PHASE 2.3: Implement app_wifi.c - WiFi station mode connection with stored credentials
- [ ] PHASE 2.4: Implement boot logic: check NVS provisioning state, auto-connect or start provisioning
- [ ] PHASE 2.5: Implement app_nvs.c - NVS storage for WiFi credentials and device config
- [ ] PHASE 2.6: Test provisioning flow with esp_prov.py Python CLI tool
- [ ] PHASE 2.7: Test Security 1 handshake with Curve25519 key exchange
- [ ] PHASE 2.8: Test Proof of Possession (PoP) with 6-digit PIN
- [ ] PHASE 3.1: Implement app_led.c - SK6812 RGB LED control for status indication
- [ ] PHASE 3.2: Implement app_button.c - GPIO button handling with factory reset (5s hold)
- [ ] PHASE 3.3: Implement app_printer.c - Thermal printer UART driver (port from Arduino)
- [ ] PHASE 3.4: Implement printer commands: init, text print, QR code, barcode, feed
- [ ] PHASE 3.5: Implement app_mqtt.c - ESP-MQTT client connecting to mqtt.m5stack.com
- [ ] PHASE 3.6: Implement MQTT callback handler for TEXT/QR/BAR print commands
- [ ] PHASE 3.7: Implement app_http.c - esp_http_server with /print, /device_status, /wifi_config endpoints
- [ ] PHASE 3.8: Implement web UI HTML for browser-based configuration and printing
- [ ] PHASE 4.1: Test complete provisioning flow with ESP BLE Provisioning iOS app
- [ ] PHASE 4.2: Test WiFi connection with multiple networks (2.4GHz, WPA2, WPA3)
- [ ] PHASE 4.3: Test print via MQTT: TEXT, QR code, barcode commands
- [ ] PHASE 4.4: Test print via HTTP API endpoints
- [ ] PHASE 4.5: Test factory reset via button hold (clear NVS, restart provisioning)
- [ ] PHASE 4.6: Test reconnection after power cycle (auto-connect with saved credentials)
- [ ] PHASE 4.7: Memory profiling - verify RAM/flash usage is within ESP32-PICO-D4 limits
- [ ] PHASE 4.8: Stability test - run 24h with continuous MQTT connection
- [ ] DOC: Write README with build instructions, iOS app setup, and usage guide
- [ ] DOC: Document BLE protocol for custom app developers
- [ ] DOC: Create troubleshooting guide for common issues
