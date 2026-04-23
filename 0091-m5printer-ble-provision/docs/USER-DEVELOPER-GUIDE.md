---
title: "BLE/WiFi Provisioning with ESP32 - User & Developer Guide"
tags:
  - article
  - guide
  - esp32
  - ble
  - wifi
  - provisioning
  - m5stack
  - iot
created: 2026-04-22
repo: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0091-m5printer-ble-provision
status: active
type: article
---

# BLE/WiFi Provisioning with ESP32 - User & Developer Guide

A comprehensive guide to using Espressif's BLE/WiFi provisioning framework to configure the M5Stack ATOM Printer (and other ESP32 devices) directly from your iPhone, without needing the device to connect to a router first.

---

## Table of Contents

1. [What is BLE Provisioning?](#1-what-is-ble-provisioning)
2. [The Three Provisioning Methods](#2-the-three-provisioning-methods)
3. [iPhone/Android Apps](#3-iphoneandroid-apps)
4. [How BLE Provisioning Works](#4-how-ble-provisioning-works)
5. [Current Printer Firmware Limitation](#5-current-printer-firmware-limitation)
6. [How to Add BLE Provisioning to Your Firmware](#6-how-to-add-ble-provisioning-to-your-firmware)
7. [Example Code: Adding BLE Provisioning to ATOM Printer](#7-example-code-adding-ble-provisioning-to-atom-printer)
8. [Security Considerations](#8-security-considerations)
9. [Troubleshooting](#9-troubleshooting)

---

## 1. What is BLE Provisioning?

BLE Provisioning allows you to send WiFi credentials to an ESP32 device over Bluetooth LE, instead of:

- Having the device create its own WiFi AP (which requires you to disconnect from your network)
- Using a serial cable
- Pre-programming credentials at the factory

**Use case for your printer**: Instead of the current approach where the printer broadcasts `ATOM-PRINTER-xxxx` and you have to connect your phone to it, BLE provisioning lets you stay on your iPhone's WiFi and push credentials directly to the printer via Bluetooth.

---

## 2. The Three Provisioning Methods

Espressif provides three provisioning methods:

### 2.1 BLE Provisioning (Recommended for iPhone)

| Aspect | Details |
|--------|---------|
| **Transport** | Bluetooth LE (GATT) |
| **Memory usage** | ~110 KB |
| **iOS support** | ✅ Full support |
| **Security** | Security 0 (none), Security 1 (Curve25519 + PoP), Security 2 (SRP6a) |
| **App** | "ESP BLE Provisioning" (App Store) |

**Advantages:**
- Stay on your WiFi while configuring the device
- No need to disconnect/reconnect
- Reliable connection feedback
- Better UX on iOS

### 2.2 SoftAP Provisioning

| Aspect | Details |
|--------|---------|
| **Transport** | WiFi (device hosts HTTP server) |
| **Memory usage** | Low (~10 KB) |
| **iOS support** | ⚠️ Limited (requires manual WiFi switch) |
| **Security** | WPA2 with per-device passphrase |

**Disadvantage on iOS:** iOS doesn't allow apps to programmatically switch WiFi networks, so users must manually go to Settings to connect to the device's AP.

### 2.3 SmartConfig (Legacy)

| Aspect | Details |
|--------|---------|
| **Transport** | WiFi (broadcast packets) |
| **Memory usage** | Low |
| **iOS support** | ✅ Via Espressif App |
| **Security** | AES encryption with PoP |

This is the older method - still works but BLE provisioning is preferred.

### 2.4 WiFi Easy Connect (DPP)

Modern standard (Device Provisioning Protocol) for WiFi Alliance certification.

---

## 3. iPhone/Android Apps

### 3.1 iOS Apps

| App | Store | Description |
|-----|-------|-------------|
| **ESP BLE Provisioning** | [App Store](https://apps.apple.com/in/app/esp-ble-provisioning/id1473590141) | BLE-based WiFi provisioning |
| **ESP SoftAP Provisioning** | [App Store](https://apps.apple.com/in/app/esp-softap-provisioning/id1474040630) | SoftAP-based provisioning |

### 3.2 Android Apps

| App | Store | Description |
|-----|-------|-------------|
| **ESP BLE Provisioning** | [Play Store](https://play.google.com/store/apps/details?id=com.espressif.provble) | BLE-based WiFi provisioning |
| **ESP SoftAP Provisioning** | Play Store | SoftAP-based provisioning |

### 3.3 Command Line Tool

```bash
# Python-based provisioning tool for Linux/macOS/Windows
# Part of esp-idf-extra-components
```

---

## 4. How BLE Provisioning Works

### 4.1 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      iPhone                                │
│  ┌─────────────────────────────────────────────────┐    │
│  │           ESP BLE Provisioning App               │    │
│  └─────────────────────────────────────────────────┘    │
│                         │                                │
│                    BLE GATT                             │
└─────────────────────────┼────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 (ATOM Printer)                      │
│  ┌─────────────────────────────────────────────────┐      │
│  │            BLE GATT Server                       │      │
│  │  Service UUID: xxxxxxxx-xxxx-xxxx-xxxx        │      │
│  └─────────────────────────────────────────────────┘      │
│                         │                                │
│                    protocomm                               │
│  ┌─────────────────────────────────────────────────┐    │
│  │         Network Provisioning Manager              │      │
│  └─────────────────────────────────────────────────┘    │
│                         │                                │
│  ┌─────────────────────────────────────────────────┐    │
│  │         WiFi Station Mode                       │      │
│  └─────────────────────────────────────────────────┘    │
└───────────────────────────────────────────────────────────┘
```

### 4.2 Protocol Flow

```
1. Phone scans for BLE devices
   ↓
2. Phone connects to device (matched by name/prefix)
   ↓
3. Security handshake (if PoP configured)
   ↓
4. Phone sends WiFi credentials (SSID + password)
   ↓
5. Device attempts WiFi connection
   ↓
6. Device reports success/failure back to phone
   ↓
7. Provisioning complete
```

### 4.3 Security Schemes

#### Security 0: No Security
```protobuf
// No encryption - only for testing
```

#### Security 1: Curve25519 + PoP (Recommended)
```protobuf
// 1. Exchange public keys
// 2. Derive shared secret
// 3. Encrypt with AES-256-CTR
// 4. Optional: Proof of Possession (6-digit PIN)
```

#### Security 2: SRP6a + AES-GCM
```protobuf
// 1. Password-based authentication
// 2. Salt + Verifier stored on device
// 3. Full encryption with AES-GCM
```

---

## 5. Current Printer Firmware Limitation

Looking at the current ATOM Printer firmware:

**What it does:**
- Creates WiFi AP (`ATOM-PRINTER-xxxx`)
- Runs HTTP server on `192.168.4.1`
- User must connect phone to printer's AP, configure WiFi credentials
- After config, printer connects to user's WiFi

**What it doesn't do:**
- BLE provisioning
- iPhone-friendly provisioning
- Stay-connected experience

**The problem:** On iOS, users must manually switch WiFi networks to configure the printer.

---

## 6. How to Add BLE Provisioning to Your Firmware

### 6.1 Required Components

Add these to your `idf_component.yml` or `CMakeLists.txt`:

```
espressif/wifi_provisioning (from idf-extra-components)
espressif/esp-tls
espressif/protocomm
nlohmann/json (for protobuf alternative)
```

### 6.2 Architecture Changes

```
Current:                          With BLE Provisioning:

WiFi AP ──┐                        BLE ◄───── iPhone App
           │                        │
           ▼                        ▼
       HTTP Server ──► WiFi      protocomm ──► WiFi
           ▲                        │
           │                        │
  Web UI ◄─┘                       ▼
                              WiFi Station
```

### 6.3 Key Components Needed

1. **BLE GATT Server** - Advertises service, handles connections
2. **Protocomm** - Protocol layer for secure communication
3. **WiFi Provisioning Manager** - Handles WiFi credential storage
4. **Custom Handlers** - Optional: add custom config endpoints

---

## 7. Example Code: Adding BLE Provisioning to ATOM Printer

### 7.1 Dependencies

In `idf_component.yml`:

```yaml
## idf_component.yml
version: "1.0.0"
description: ATOM Printer with BLE Provisioning
targets:
  - esp32
dependencies:
  espressif/wifi_provisioning:
    version: "^1.0.0"
  espressif/esp-tls: "^1.0.0"
  ## BLE support
  idf: ">=4.4"
```

### 7.2 Main Application Code

```cpp
// main/ble_provisioning_main.c

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

// BLE device name prefix (how phone app finds device)
#define BLE_PROV_NAME "ATOM_PRINTER"

// Proof of Possession (optional security)
#define POP_SALT "123456"

// Custom data endpoint (for future use)
#define CUSTOM_DATA_EP "custom-data"

wifi_prov_config_t wifi_prov_config = {
    .scheme = wifi_prov_scheme_ble,
    .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE,
    .app_info = {
        .fw_version = "1.0.0",
        .custom_data = NULL,
        .service_name = BLE_PROV_NAME,
        .service_key = NULL,  // or POP if using security
    },
};

// Custom handler for application-specific data
static void custom_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    // Process custom data from phone app
    // This could be MQTT broker settings, device name, etc.
    
    printf("Received custom data: %.*s\n", inlen, inbuf);
    
    // Echo back acknowledgment
    *outbuf = malloc(16);
    *outlen = sprintf((char*)*outbuf, "OK");
}

// Event handler for provisioning events
static void prov_event_handler(void *user_data, wifi_prov_event_t event,
                                union wifi_prov_event_data *event_data)
{
    switch (event) {
        case WIFI_PROV_START:
            printf("Provisioning started\n");
            break;
        case WIFI_PROV_CRED_RECV:
            wifi_sta_config_t *cred = &event_data->cred_recv.creds[0];
            printf("WiFi credentials received: SSID=%s\n", cred->ssid);
            break;
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t reason = event_data->cred_fail.reason;
            printf("Provisioning failed! Reason: %d\n", reason);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            printf("WiFi credentials applied successfully\n");
            break;
        case WIFI_PROV_END:
            printf("Provisioning complete\n");
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
    }
}

void app_main()
{
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // Initialize network stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Start provisioning
    wifi_prov_mgr_config_t prov_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE,
        .app_event_handler = {
            .event_cb = prov_event_handler,
            .user_data = NULL,
        },
    };
    
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));
    
    // Check if already provisioned
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    
    if (!provisioned) {
        printf("Starting BLE provisioning...\n");
        
        // Create BLE advertisement
        wifi_prov_ble_gap_register_callbacks_t ble_gap_callbacks = {
            // Optional: custom device name or data in advertisement
        };
        
        // Start provisioning (this never returns unless prov is done)
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
            SECURITY_0,  // Or SECURITY_1 with POP
            POP_SALT,     // Optional proof of possession
            BLE_PROV_NAME,
            NULL
        ));
        
        // Wait for provisioning to complete
        // (app_main doesn't return if using default config)
    } else {
        printf("Already provisioned, connecting to WiFi...\n");
        esp_wifi_start();
        esp_wifi_connect();
    }
}
```

### 7.3 Configuration Options

In `sdkconfig.defaults`:

```
# Enable BLE
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y

# Enable BLE provisioning
CONFIG_WIFI_PROVING=y
CONFIG_WIFI_PROV_BLE=y

# Security (choose one)
CONFIG_WIFI_PROV_SECURITY_0=y    # No security
CONFIG_WIFI_PROV_SECURITY_1=y   # Curve25519 + PoP

# BLE device name
CONFIG_WIFI_PROV_BLE_DEV_NAME="ATOM_PRINTER"

# Disable auto-start of provisioning on boot
CONFIG_WIFI_PROV_MGMT_NOT_RECV_CONFIG=y
```

### 7.4 Phone App Usage

1. Download "ESP BLE Provisioning" from App Store
2. Open the app
3. The app scans for BLE devices with name `ATOM_PRINTER*`
4. Tap to connect
5. Enter WiFi credentials (SSID + password)
6. If using PoP security, enter the 6-digit code
7. Wait for success
8. Device connects to your WiFi

---

## 8. Security Considerations

### 8.1 Proof of Possession (PoP)

PoP adds a 6-digit PIN that must be entered on the phone app to provision the device:

```cpp
// Security 1 with PoP
ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
    SECURITY_1,        // Curve25519 + PoP
    "123456",          // 6-digit PIN (printed on device/box)
    BLE_PROV_NAME,
    NULL
));
```

### 8.2 BLE Security

| Aspect | Recommendation |
|--------|---------------|
| Bonding | Enable BLE bonding to prevent MITM |
| LE Secure Connections | Enable for strongest security |
| Device Name | Don't include sensitive data in name |
| Advertising Data | Keep minimal |

### 8.3 WiFi Security

| Aspect | Recommendation |
|--------|---------------|
| WPA3 | Enable if supported |
| WPA2-Enterprise | Consider for corporate networks |
| Hidden SSIDs | May cause provisioning issues |

---

## 9. Troubleshooting

### 9.1 Device Not Found by App

**Causes:**
- BLE not enabled on device
- Device already provisioned
- BLE not started
- Name prefix mismatch

**Solutions:**
```bash
# Check device is advertising
# Use nRF Connect app to scan for BLE devices
# Look for device name starting with "ATOM_PRINTER"
```

### 9.2 Provisioning Fails

**Causes:**
- Wrong WiFi password
- WiFi network not in range
- Security mismatch (PoP code wrong)

**Solutions:**
```bash
# Check serial output for error codes
# Reset provisioning state:
idf.py erase-flash
```

### 9.3 iOS App Crashes

**Causes:**
- App not compatible with iOS version
- BLE permissions not granted

**Solutions:**
- Check App Store for updates
- Ensure Bluetooth permissions granted
- Try Android app as alternative

### 9.4 Resetting Provisioned State

```cpp
// In your app, provide a way to reset:
#include <wifi_provisioning/manager.h>

void reset_provisioning() {
    wifi_prov_mgr_reset_provisioning();
    esp_restart();
}

// Or from serial/button:
// Hold button for 10 seconds to reset
```

---

## 10. Resources

### Documentation
- [Espressif Provisioning API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/index.html)
- [Network Provisioning Examples](https://github.com/espressif/idf-extra-components/tree/master/network_provisioning/examples)
- [Protocol Communication (Protocomm)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html)

### iOS/Android Apps
- [ESP BLE Provisioning (iOS)](https://apps.apple.com/in/app/esp-ble-provisioning/id1473590141)
- [ESP SoftAP Provisioning (iOS)](https://apps.apple.com/in/app/esp-softap-provisioning/id1474040630)
- [ESP BLE Provisioning (Android)](https://play.google.com/store/apps/details?id=com.espressif.provble)

### Source Code
- [idf-extra-components](https://github.com/espressif/idf-extra-components) - BLE provisioning library
- [esp-idf-provisioning-android](https://github.com/espressif/esp-idf-provisioning-android) - Android app source
- [esp-idf-provisioning-ios](https://github.com/espressif/esp-idf-provisioning-ios) - iOS app source

### M5Stack Specific
- Current printer firmware: [ATOM-PRINTER](https://github.com/m5stack/ATOM-PRINTER)
- For M5Stack devices with BLE: Check if `wifi_provisioning` component is included in your build

---

## 11. Alternative: SoftAP + BLE Hybrid

For the best user experience, support both methods:

```cpp
// Initialize both BLE and SoftAP
wifi_prov_mgr_init(mgr_config);

// BLE will be tried first
// Falls back to SoftAP if BLE fails or not supported
```

---

*Research date: 2026-04-22*
*Sources: Espressif IDF documentation, GitHub repositories, App Store listings*
