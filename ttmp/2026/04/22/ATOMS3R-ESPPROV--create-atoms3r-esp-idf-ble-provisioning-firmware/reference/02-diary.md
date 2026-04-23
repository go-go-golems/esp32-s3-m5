---
title: "ATOMS3R-ESPPROV Implementation Diary"
ticket: ATOMS3R-ESPPROV
created: 2026-04-22
---

# ATOMS3R-ESPPROV Implementation Diary

## Goal
Build an ESP-IDF-based firmware for the M5Stack ATOM Printer using the official Espressif `wifi_provisioning` component with Security 1 (Curve25519 + AES-256-CTR + PoP). This will work with the "ESP BLE Provisioning" iOS app from the App Store.

## Approach
Unlike the previous Arduino+NimBLE approach, this is a ground-up rewrite using ESP-IDF. We will:
1. Create a fresh ESP-IDF project
2. Add the `wifi_provisioning` component from `idf-extra-components`
3. Implement provisioning with BLE transport and Security 1
4. Port the printer driver, MQTT client, and HTTP server to ESP-IDF C
5. Flash and test with the official iOS app

## Hardware
- M5Stack ATOM (ESP32-PICO-D4, 4MB flash, 520KB RAM)
- Thermal printer module (UART)
- USB-C for programming

---

## 2026-04-22: Phase 1 Complete - ESP-IDF Project Setup

### Tasks Completed
- 1.1: Created ESP-IDF project structure with idf.py create-project
- 1.2: Added wifi_provisioning component (built into ESP-IDF 5.1)
- 1.3: Configured sdkconfig for BLE, WiFi provisioning, and Security 1
- 1.4: Set up custom partition table (1.9MB factory partition)
- 1.5: Configured build for ESP32-PICO-D4 target
- 1.6: Verified clean build succeeds

### Build Results
- Binary size: 0x13fcb0 (1,294,512 bytes, ~1.29 MB)
- Partition size: 0x1f0000 (1,990,656 bytes, ~1.9 MB)
- Free space: 0xb0350 (721,744 bytes, 36% free)
- Status: BUILD SUCCESS

### Compilation Fixes
1. app_prov.c: Added esp_mac.h include for esp_read_mac()
2. app_prov.c: Changed event type from wifi_prov_event_t to wifi_prov_cb_event_t
3. app_prov.c: Fixed event data pointer (void* instead of union)
4. app_wifi.c: Added string.h include for strncpy()
5. app_led.c: Removed unused s_colors array

### Git Commits
- 2691886 Phase 1: ESP-IDF project setup with BLE provisioning
- 3d4bda2 Phase 1: ESP-IDF project with BLE provisioning (.gitignore)

### Next Steps
Flash to device and test with ESP BLE Provisioning iOS app

## Flashing and Testing Status

### Flash Results
- Erased entire flash chip (5.5 seconds)
- Flashed bootloader at 0x1000
- Flashed partition table at 0x8000
- Flashed app at 0x10000 (1.29MB, 72.6 seconds at 115200 baud)
- Hash verification: PASSED
- Device MAC: 14:08:08:53:87:00

### Serial Output Issue
After flashing, the device shows boot ROM signature at 74880 baud:
  ets Jun  8 2016 00:22:57

But no output at 115200 baud. Possible causes:
1. Bootloader or app crash during init
2. UART pin configuration conflict
3. Need to enable early boot messages in sdkconfig

### Next Steps to Debug
1. Enable CONFIG_ESP_CONSOLE_UART_DEFAULT and verbose boot messages
2. Add early ESP_EARLY_LOGx at start of app_main
3. Check if device is advertising over BLE
4. Try flashing ESP-IDF hello_world example to verify toolchain

### Code Status
Phase 1 (Setup): COMPLETE
Phase 2 (Provisioning): IMPLEMENTED but needs testing
Phase 3 (Application): NOT STARTED (printer, MQTT, HTTP)
Phase 4 (Testing): BLOCKED by serial output issue
