# BLE/WiFi Provisioning Research - Source Manifest

**Created**: 2026-04-22
**Research Focus**: Espressif BLE/WiFi provisioning framework for ESP32 devices

---

## Collected Sources

### Official Espressif Documentation

| # | File | Source URL | Description |
|---|------|------------|-------------|
| 01 | 01-esp32-provisioning-index.md | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/index.html | Provisioning API index |
| 02 | 02-esp_blufi.md | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/esp_blufi.html | BLUFi protocol |
| 03 | 03-wifi-provisioning.md | https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/wifi.html#wi-fi-provisioning | WiFi provisioning guide |
| 04 | 04-esp_smartconfig.md | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_smartconfig.html | SmartConfig protocol |
| 05 | 05-esp_dpp.md | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_dpp.html | Wi-Fi Easy Connect (DPP) |
| 06 | 06-network_provisioning_repo.md | https://github.com/espressif/idf-extra-components/tree/master/network_provisioning | Network provisioning library |
| 07 | 07-wifi_prov_example.md | https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/examples/wifi_prov/README.md | WiFi provisioning example |
| 08 | 08-wifi_prov_app_main.md | https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/examples/wifi_prov/main/app_main.c | Example source code |
| 12 | 12-wifi_provisioning_api.md | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html | Unified Provisioning API |
| 13 | 13-protocomm.md | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html | Protocol Communication |

### Mobile Apps

| # | File | Source URL | Description |
|---|------|------------|-------------|
| 11 | 11-esp32_prov_android.md | https://play.google.com/store/apps/details?id=com.espressif.provble | Android BLE Provisioning app |

### Key GitHub Repositories

| Repository | URL |
|-----------|-----|
| wifi_provisioning | https://github.com/espressif/idf-extra-components/tree/master/network_provisioning |
| wifi_prov example | https://github.com/espressif/idf-extra-components/tree/master/network_provisioning/examples/wifi_prov |
| ESP BLE Provisioning iOS App | https://apps.apple.com/us/app/esp-ble-provisioning/id1465017836 |
| ESP Provisioning Android | https://play.google.com/store/apps/details?id=com.espressif.provble |

---

## Key Findings

### Provisioning Methods

1. **BLE Provisioning** (Recommended for iPhone)
   - Stay on your WiFi while configuring
   - ~110KB memory usage
   - Full iOS support
   - Security 0/1/2 options

2. **SoftAP Provisioning**
   - Creates WiFi AP on device
   - Low memory usage
   - Requires WiFi network switch on iOS

3. **SmartConfig**
   - Legacy method
   - Uses WiFi broadcast packets
   - Less secure

4. **WiFi Easy Connect (DPP)**
   - Modern standard
   - WiFi Alliance certified

### iOS Apps

- **ESP BLE Provisioning** - Available on App Store
- **ESP SoftAP Provisioning** - Available on App Store
- Both require manual network switching for SoftAP mode

### Security Schemes

| Security | Method | PoP | Recommended For |
|----------|--------|-----|----------------|
| 0 | None | No | Testing only |
| 1 | Curve25519 + AES-256-CTR | Optional | Production |
| 2 | SRP6a + AES-256-GCM | Required | High security |

---

## Search History

1. `Espressif BLE WiFi provisioning framework ESP32`
2. `ESP32 BLE provisioning iPhone app`
3. `Espressif wifi_provisioning github`
4. `esp-idf provisioning examples`
5. `ESP BLE Provisioning Android Play Store`

---

## Next Steps

1. Test current printer with BLE provisioning apps
2. Modify printer firmware to add BLE provisioning support
3. Create hybrid SoftAP + BLE provisioning
4. Test with iPhone ESP BLE Provisioning app
5. Consider adding custom data endpoint for MQTT config
