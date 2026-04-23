---
title: "ATOMS3R ESP-IDF BLE Provisioning System Design"
tags:
  - design-doc
  - firmware
  - esp32
  - ble
  - provisioning
  - esp-idf
created: 2026-04-22
ticket: ATOMS3R-ESPPROV
status: active
type: design-doc
intent: long-term
topics:
  - firmware
  - esp32
  - ble
  - provisioning
  - esp-idf
  - ios
  - m5stack
---

# ATOMS3R ESP-IDF BLE Provisioning System Design

> **Purpose**: Design an ESP-IDF-based firmware using the official Espressif `wifi_provisioning` component with the "ESP BLE Provisioning" iOS app  
> **Audience**: Firmware developers  
> **Ticket**: ATOMS3R-ESPPROV

---

## 1. Executive Summary

### 1.1 The Goal

Create a new firmware for the M5Stack ATOM Printer that uses the **official Espressif ESP-IDF BLE provisioning framework**. This allows users to configure WiFi using the **"ESP BLE Provisioning" iOS app** from the App Store.

### 1.2 Why ESP-IDF Instead of Arduino

| Aspect | Arduino + NimBLE (current) | ESP-IDF + wifi_provisioning (new) |
|--------|---------------------------|-----------------------------------|
| **iOS App** | Generic BLE apps (nRF Connect) | Official "ESP BLE Provisioning" app |
| **Protocol** | Custom GATT characteristics | Standard Espressif protocol |
| **Security** | None (plaintext) | Curve25519 + AES-256-CTR + PoP |
| **Protobuf** | No | Yes (compact binary protocol) |
| **Complexity** | Low | Medium |
| **Code reuse** | All existing code | Must rewrite printer/MQTT drivers |

### 1.3 Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Framework** | ESP-IDF | Required for wifi_provisioning component |
| **Provisioning** | BLE (Security 1) | Works with official iOS app |
| **Proof of Possession** | 6-digit PIN | Printed on device label |
| **Device Name** | `ATOMS3R_XXXX` | Easy to identify |
| **Printer Driver** | Rewrite in C | ESP-IDF doesn't have M5Atom lib |
| **MQTT** | ESP-MQTT | Native ESP-IDF component |
| **Web Server** | esp_http_server | Native ESP-IDF component |

---

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     iPhone / iPad                           │
│         "ESP BLE Provisioning" App (App Store)               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  - Scan for BLE devices                              │   │
│  │  - Enter PoP (Proof of Possession)                   │   │
│  │  - Select WiFi network from scan list                │   │
│  │  - Enter WiFi password                               │   │
│  │  - Send credentials over BLE                         │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ BLE GATT (Protobuf over
                              │ Security 1 encrypted channel)
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    ATOMS3R Firmware (ESP-IDF)               │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │         WiFi Provisioning Manager                    │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ │   │
│  │  │ Protocomm   │  │  Security 1 │  │   BLE GATT  │ │   │
│  │  │ (protobuf)  │  │(Curve25519) │  │   Server    │ │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘ │   │
│  │         └─────────────────┴─────────────────┘        │   │
│  │                      │                               │   │
│  │                      ▼                               │   │
│  │         ┌─────────────────────┐                    │   │
│  │         │   WiFi Manager       │                    │   │
│  │         │   (connect/sta)      │                    │   │
│  │         └──────────┬──────────┘                    │   │
│  └────────────────────┼────────────────────────────────┘   │
│                       │                                    │
│                       ▼                                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Application Layer                       │   │
│  │  ┌────────────┐  ┌────────────┐  ┌──────────────┐  │   │
│  │  │ Thermal    │  │   MQTT     │  │   HTTP       │  │   │
│  │  │ Printer    │  │   Client   │  │   Server     │  │   │
│  │  │ Driver     │  │            │  │              │  │   │
│  │  └────────────┘  └────────────┘  └──────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Protocol Stack

```
Application Layer
├─ WiFi Provisioning Manager (wifi_provisioning component)
│  ├─ protocomm (protocol abstraction)
│  │  ├─ Security 1 (Curve25519 key exchange + AES-256-CTR)
│  │  └─ Transport: BLE (GATT)
│  └─ Network Config Endpoint (wifi-config)
│     └─ Protobuf messages (wifi_config.proto)
│
├─ Application Code
│  ├─ Thermal printer driver (rewritten in C)
│  ├─ MQTT client (esp-mqtt)
│  └─ HTTP server (esp_http_server)
│
ESP-IDF Framework
├─ Bluetooth (Bluedroid or NimBLE)
├─ WiFi (esp_wifi)
├─ NVS (nvs_flash)
└─ FreeRTOS
```

---

## 3. Component Specifications

### 3.1 WiFi Provisioning Component

The `wifi_provisioning` component from `idf-extra-components` provides:

```c
#include "wifi_provisioning/manager.h"

// Initialize with BLE scheme
wifi_prov_mgr_config_t config = {
    .scheme = wifi_prov_scheme_ble,
    .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE,
    .app_event_handler = {
        .event_cb = app_prov_event_handler,
        .user_data = NULL
    }
};
wifi_prov_mgr_init(config);

// Start provisioning
wifi_prov_mgr_start_provisioning(
    SECURITY_1,           // Security scheme
    "123456",             // Proof of Possession (6-digit PIN)
    "ATOMS3R_8700",       // Device name
    NULL                  // Custom service key (optional)
);
```

### 3.2 Protocomm Protocol

The provisioning protocol uses Google Protocol Buffers:

```protobuf
// Session commands
message SessionData {
    oneof proto_case {
        Sec1Payload sec1 = 1;    // Security 1 handshake
        Sec2Payload sec2 = 2;    // Security 2 handshake
    }
}

// WiFi configuration
message WiFiConfigPayload {
    oneof payload {
        CmdGetStatus cmd_get_status = 1;
        RespGetStatus resp_get_status = 2;
        CmdSetConfig cmd_set_config = 3;
        RespSetConfig resp_set_config = 4;
        CmdApplyConfig cmd_apply_config = 5;
        RespApplyConfig resp_apply_config = 6;
    }
}

// WiFi credentials
message WiFiConfigMsg {
    bytes ssid = 1;
    bytes passphrase = 2;
    int32 bssid = 3;
    int32 channel = 4;
}
```

### 3.3 Security 1 Handshake

```
Phone                              Device
  │                                  │
  │────── SessionData (step0) ─────►│  "I want to start provisioning"
  │                                  │
  │◄───── SessionData (step1) ──────│  "Here's my public key"
  │                                  │
  │────── SessionData (step2) ─────►│  "Here's my public key + auth data"
  │                                  │  (if PoP is configured, verify it)
  │                                  │
  │◄───── SessionData (step3) ──────│  "Here's my response"
  │                                  │
  │                                  │  Derive shared key from DH exchange
  │                                  │  All subsequent communication is
  │                                  │  encrypted with AES-256-CTR
```

### 3.4 BLE Service

The ESP-IDF provisioning component creates these BLE characteristics:

| UUID | Name | Properties | Purpose |
|------|------|------------|---------|
| `0xFF52` | Version | Read | Protocol version |
| `0xFF53` | Session | Write, Notify | Main command/response channel |

The service UUID is generated from the device name using a hash function.

### 3.5 iOS App Integration

The "ESP BLE Provisioning" app expects:

1. **Device Discovery**: Scans for BLE devices with name prefix matching the configured prefix
2. **PoP Verification**: Prompts user for 6-digit PIN before proceeding
3. **WiFi Scan**: Requests device to scan for available WiFi networks
4. **Credential Send**: Sends selected network + password
5. **Status Monitoring**: Polls connection status until success/failure

---

## 4. Implementation Plan

### Phase 1: Project Setup

**Task 1.1: Create ESP-IDF Project**
- Create new ESP-IDF project using `idf.py create-project`
- Configure for ESP32-PICO-D4 target
- Set up partition table for 4MB flash

**Task 1.2: Add wifi_provisioning Component**
- Add `espressif/wifi_provisioning` dependency to `idf_component.yml`
- Add `espressif/esp-tls` for TLS support
- Configure `sdkconfig` for BLE and WiFi provisioning

**Task 1.3: Configure sdkconfig**
```
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y
CONFIG_WIFI_PROVING=y
CONFIG_WIFI_PROV_BLE=y
CONFIG_WIFI_PROV_SECURITY_1=y
CONFIG_WIFI_PROV_BLE_DEV_NAME="ATOMS3R"
```

### Phase 2: Provisioning Implementation

**Task 2.1: Implement Provisioning Manager**
- Initialize `wifi_prov_mgr` with BLE scheme
- Register event handlers for provisioning events
- Handle credential reception and storage to NVS

**Task 2.2: Implement Boot Logic**
- Check NVS for existing WiFi credentials
- If provisioned: connect to WiFi, start application
- If not provisioned: start BLE provisioning
- Handle factory reset (button hold)

**Task 2.3: LED Status Indication**
- Map provisioning/WiFi/MQTT states to LED colors
- Use M5Atom's SK6812 RGB LED via GPIO

### Phase 3: Application Layer

**Task 3.1: Thermal Printer Driver**
- Port existing printer logic from Arduino to ESP-IDF C
- UART communication with thermal printer module
- Commands: init, print text, print QR, print barcode, feed

**Task 3.2: MQTT Client**
- Use `esp-mqtt` component
- Connect to mqtt.m5stack.com:1883
- Subscribe to device MAC topic
- Handle print commands (TEXT, QR, BAR)

**Task 3.3: HTTP Server**
- Use `esp_http_server` component
- Implement endpoints: /print, /device_status, /wifi_config
- Serve web UI for direct browser access

### Phase 4: Testing & Integration

**Task 4.1: Unit Testing**
- Test provisioning flow with esp_prov.py (Python CLI tool)
- Test WiFi connection with various networks
- Test printer commands

**Task 4.2: iOS App Testing**
- Install "ESP BLE Provisioning" from App Store
- Test complete provisioning flow
- Test with different WiFi networks

**Task 4.3: Integration Testing**
- Test print via MQTT
- Test print via HTTP
- Test web UI
- Test factory reset

---

## 5. File Structure

```
atoms3r-esp-idf/
├── CMakeLists.txt
├── sdkconfig
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                    # Entry point, app_main()
│   ├── app_prov.c/h              # Provisioning manager wrapper
│   ├── app_wifi.c/h              # WiFi connection logic
│   ├── app_led.c/h               # LED status indication
│   ├── app_button.c/h            # Button handling (reset)
│   ├── app_printer.c/h           # Thermal printer driver
│   ├── app_mqtt.c/h              # MQTT client
│   ├── app_http.c/h              # HTTP server
│   └── app_nvs.c/h               # NVS storage helpers
├── components/
│   └── (managed by idf_component.yml)
├── test/
│   └── test_provisioning.c
└── README.md
```

---

## 6. API Reference

### 6.1 wifi_provisioning API

```c
// Initialize manager
esp_err_t wifi_prov_mgr_init(wifi_prov_mgr_config_t config);

// Check if provisioned
esp_err_t wifi_prov_mgr_is_provisioned(bool *provisioned);

// Start provisioning
esp_err_t wifi_prov_mgr_start_provisioning(
    wifi_prov_security_t security,
    const char *pop,              // Proof of Possession
    const char *service_name,     // BLE device name
    const char *service_key       // SoftAP password (optional)
);

// Stop provisioning
esp_err_t wifi_prov_mgr_stop_provisioning(void);

// Reset provisioning state
esp_err_t wifi_prov_mgr_reset_provisioning(void);

// Deinitialize
void wifi_prov_mgr_deinit(void);
```

### 6.2 Event Handlers

```c
// Provisioning event handler
void app_prov_event_handler(void *user_data,
                            wifi_prov_event_t event,
                            union wifi_prov_event_data *data)
{
    switch (event) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
            ESP_LOGI(TAG, "Credentials received");
            break;
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "WiFi connected successfully");
            break;
        case WIFI_PROV_CRED_FAIL:
            ESP_LOGE(TAG, "WiFi connection failed");
            break;
        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning complete");
            wifi_prov_mgr_deinit();
            break;
    }
}
```

---

## 7. Security Considerations

### 7.1 Proof of Possession (PoP)

- 6-digit PIN printed on device label
- Required before provisioning can begin
- Prevents unauthorized users from provisioning the device

### 7.2 Encryption

- Curve25519 key exchange for session establishment
- AES-256-CTR for all provisioning data
- WiFi credentials never transmitted in plaintext

### 7.3 NVS Storage

- Credentials stored in encrypted NVS partition (optional)
- Consider enabling `CONFIG_NVS_ENCRYPTION`

---

## 8. Resources

### 8.1 Official Documentation
- [WiFi Provisioning API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html)
- [network_provisioning component](https://github.com/espressif/idf-extra-components/tree/master/network_provisioning)
- [wifi_prov example](https://github.com/espressif/idf-extra-components/tree/master/network_provisioning/examples/wifi_prov)

### 8.2 iOS App
- [ESP BLE Provisioning on App Store](https://apps.apple.com/us/app/esp-ble-provisioning/id1465017836)
- [ESP-IDF Provisioning iOS Source](https://github.com/espressif/esp-idf-provisioning-ios)

### 8.3 Existing Research
- `0091-m5printer-ble-provision/reference/` - Contains provisioning documentation
- `0090-m5printer-research/` - Contains original firmware analysis

---

## 9. Open Questions

1. **Printer UART pins**: Need to verify which GPIOs the thermal printer is connected to on the ATOM
2. **LED GPIO**: Need to confirm SK6812 LED pin on ATOM (likely GPIO 27)
3. **Button GPIO**: Need to confirm button pin on ATOM (likely GPIO 39)
4. **Power management**: The thermal printer needs 12V - need to verify power circuit
5. **Partition table**: May need custom partition table for OTA updates

---

## 10. Related Tickets

- **ATOMS3R-BLEPROV** - Previous ticket with Arduino+NimBLE approach (simpler, works with nRF Connect)
- **0090-m5printer-research** - Original ATOM Printer firmware analysis

---

*Document: ATOMS3R ESP-IDF BLE Provisioning System Design*  
*Ticket: ATOMS3R-ESPPROV*  
*Created: 2026-04-22*
