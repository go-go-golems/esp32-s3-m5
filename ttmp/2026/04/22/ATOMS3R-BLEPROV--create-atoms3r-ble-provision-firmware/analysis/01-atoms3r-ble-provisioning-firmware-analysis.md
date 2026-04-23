---
title: "ATOMS3R BLE-Provisioning Firmware Analysis"
tags:
  - article
  - analysis
  - firmware
  - esp32
  - ble
  - provisioning
  - ios
created: 2026-04-22
ticket: ATOMS3R-BLEPROV
status: active
type: analysis
intent: long-term
topics:
  - firmware
  - esp32
  - ble
  - provisioning
  - ios
  - m5stack
---

# ATOMS3R BLE-Provisioning Firmware Analysis

> **Audience**: New firmware developer joining the ATOMS3R BLE provisioning project  
> **Purpose**: Provide comprehensive background knowledge before writing any code  
> **Prerequisites**: Basic embedded C, some networking knowledge helpful but not required

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [What is "Provisioning" and Why Do We Need It?](#2-what-is-provisioning-and-why-do-we-need-it)
3. [The Problem with Traditional WiFi Setup](#3-the-problem-with-traditional-wifi-setup)
4. [How BLE Provisioning Solves This](#4-how-ble-provisioning-solves-this)
5. [ESP32 BLE Architecture Deep Dive](#5-esp32-ble-architecture-deep-dive)
6. [The Protocomm Protocol](#6-the-protocomm-protocol)
7. [Security: How Credentials Are Protected](#7-security-how-credentials-are-protected)
8. [Current ATOM Printer Firmware Analysis](#8-current-atom-printer-firmware-analysis)
9. [Gap Analysis: What We Need to Build](#9-gap-analysis-what-we-need-to-build)
10. [iOS/Android App Integration](#10-iosandroid-app-integration)
11. [File Structure and References](#11-file-structure-and-references)

---

## 1. Executive Summary

### 1.1 The Goal

We need to create a new firmware for the ATOMS3R (M5Stack ATOM-based thermal printer) that allows users to configure WiFi credentials using their iPhone, **without having to disconnect from their existing WiFi network**.

Currently, the printer works like this:

```
Current Flow:
┌─────────┐         WiFi AP         ┌─────────────┐
│  Phone  │ ◄──────────────────────►│  Printer    │
│         │    "ATOM-PRINTER-xxx"   │  (creates   │
│         │                         │   its own   │
│  WiFi   │                         │   network)  │
└────┬────┘                         └─────────────┘
     │ User must manually disconnect from their WiFi
     │ and connect to the printer's network
```

What we want:

```
Desired Flow:
┌─────────┐         BLE          ┌─────────────┐
│  Phone  │ ◄───────────────────►│  Printer    │
│         │    Stays on normal   │  (still on  │
│         │    WiFi while        │   normal    │
│  WiFi   │    configuring)      │   WiFi)      │
└─────────┘                     └──────────────┘
                                    │
                                    ▼
                              ┌───────────┐
                              │ User's    │
                              │ WiFi      │
                              └───────────┘
```

### 1.2 Key Technical Decisions

| Decision | Choice | Reason |
|----------|--------|--------|
| **Transport** | BLE (Bluetooth LE) | Works on iOS without network switching |
| **Security** | Security 1 (Curve25519) | Good balance of security and simplicity |
| **Proof of Possession** | 6-digit PIN | Prevents unauthorized provisioning |
| **Device Name** | `ATOMS3R_XXXX` | Easy to identify during scanning |

### 1.3 What This Document Covers

This analysis covers:
- The networking concepts behind BLE provisioning
- How ESP32's BLE stack works
- The Espressif provisioning protocol stack
- How to integrate with existing printer firmware
- Security considerations and tradeoffs

---

## 2. What is "Provisioning" and Why Do We Need It?

### 2.1 Definition

**Provisioning** in the IoT context means: *the process of configuring a device with the network credentials (WiFi SSID and password) it needs to connect to the internet.*

Without provisioning, a device has no way to know:
- Which WiFi network to connect to
- What the password is
- What security type to use (WPA2, WPA3, etc.)

### 2.2 Why Can't We Just Pre-configure?

Some devices do come pre-configured (like setting a default WiFi password at the factory), but this has problems:

| Problem | Description |
|---------|-------------|
| **Factory Reset** | Users can reset devices, losing the pre-configured credentials |
| **Different Networks** | Users have different WiFi networks at home vs. work |
| **Security Risk** | Default credentials are well-known and can be exploited |
| **No User Control** | Users can't change the WiFi without reflashing firmware |

Therefore, most IoT devices need a **user-initiated provisioning process**.

### 2.3 What a Provisioning Process Must Do

A complete provisioning flow must accomplish these steps:

```
1. DISCOVER: User's phone finds the device
2. AUTHENTICATE: Verify the device is the right one (Proof of Possession)
3. TRANSMIT: Send WiFi credentials (SSID + password) securely
4. VERIFY: Confirm the device received the credentials correctly
5. CONNECT: Device connects to the WiFi network
6. CONFIRM: Report success/failure back to the phone
```

---

## 3. The Problem with Traditional WiFi Setup

### 3.1 The SoftAP Method (Current Implementation)

The current ATOM Printer uses the **SoftAP (Software Access Point)** method:

```
┌──────────────────────────────────────────────────────────────┐
│                     CURRENT PRINTER SETUP                     │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│   ┌─────────┐         WiFi         ┌──────────────────┐     │
│   │  Phone  │ ◄──────────────────►│  ATOM Printer     │     │
│   │         │    192.168.4.1       │                   │     │
│   └────┬────┘                     │  Creates its own   │     │
│        │                          │  WiFi network      │     │
│        │ SSID: ATOM-PRINTER-xxxx  │ with DHCP server   │     │
│        │                          │                   │     │
│        ▼                          └───────────────────┘     │
│   User must:                                                 │
│   1. Disconnect from home WiFi                               │
│   2. Connect to printer's network                            │
│   3. Open browser to 192.168.4.1                            │
│   4. Enter home WiFi credentials                            │
│   5. Wait for printer to connect                            │
│   6. Reconnect phone to home WiFi                            │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

### 3.2 Why This is Problematic on iOS

On iOS, the WiFi network management APIs are **restricted**:

1. **No WiFi Switching API**: Apps cannot programmatically disconnect from one WiFi network and connect to another. The user must do this manually through Settings.
2. **Background WiFi**: iOS deprioritizes WiFi scanning when an app is backgrounded, making network discovery unreliable.
3. **User Experience**: This multi-step process confuses users and leads to support requests.

### 3.3 BLE to the Rescue

Bluetooth LE operates **independently** from WiFi:

```
┌──────────────────────────────────────────────────────────────┐
│                     BLE PROVISIONING SETUP                     │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│   ┌─────────┐         BLE          ┌──────────────────┐     │
│   │  Phone  │ ◄───────────────────►│  ATOM Printer     │     │
│   │         │                       │                   │     │
│   └────┬────┘                       │  No WiFi AP      │     │
│        │                            │  required!       │     │
│        │                            │                   │     │
│        ▼                            └───────────────────┘     │
│   User:                                                     │
│   1. Open app (stays on home WiFi)                          │
│   2. App scans for BLE devices                               │
│   3. Tap "ATOMS3R_1234" to connect                           │
│   4. Enter home WiFi credentials                             │
│   5. Device connects automatically                           │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

---

## 4. How BLE Provisioning Solves This

### 4.1 Overview of the BLE Provisioning Flow

```
┌──────────┐                           ┌──────────────┐
│   iPhone │                           │  ATOMS3R     │
│          │                           │  (Printer)   │
└────┬─────┘                           └───────┬──────┘
     │                                        │
     │  1. Scan for BLE devices               │
     │───────────────────────────────────────►│
     │                                        │
     │  2. BLE Connect                        │
     │─────────────────────────────────────────►
     │                                        │
     │  3. Security Handshake (optional)      │
     │  <───────────────────────────────────────
     │                                        │
     │  4. Send WiFi credentials              │
     │─────────────────────────────────────────►
     │                                        │
     │                                        │ Store in NVS
     │                                        │ Connect to WiFi
     │                                        │
     │  5. Connection status                 │
     │  <───────────────────────────────────────
     │                                        │
     │  6. Disconnect                        │
     │  <───────────────────────────────────────
     │                                        │
     ▼                                        ▼
```

### 4.2 Key Components

| Component | Purpose | Location |
|-----------|---------|----------|
| **BLE Stack** | Handle Bluetooth communication | ESP-IDF built-in |
| **GATT Server** | Expose provisioning service/characteristics | We implement |
| **Protocomm** | Handle protocol encoding/decoding | From IDF |
| **WiFi Prov Manager** | Manage WiFi connection lifecycle | From IDF |
| **NVS** | Store credentials persistently | ESP-IDF built-in |

### 4.3 BLE Service Structure

The ESP32 exposes a GATT service with these characteristics:

```
GATT Service: WiFi Provisioning (UUID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)
│
├─── Characteristic: Version (Read)
│    Value: "1.0"
│
├─── Characteristic: Session (Write/Notify)
│    Purpose: Exchange provisioning data
│    Properties: Write, Write No Response, Notify
│
└─── Characteristic: Custom Data (Write/Read)
     Purpose: Application-specific data (MQTT config, etc.)
     Properties: Write, Read, Write No Response
```

### 4.4 Data Flow During Provisioning

```
Phone App                          ESP32
   │                                 │
   │──── Connect Request ──────────►│
   │                                 │
   │◄─── Connection Complete ────────│
   │                                 │
   │──── Handshake Start ──────────►│ Curve25519 key exchange
   │◄─── Public Key ──────────────────│
   │                                 │
   │──── Encrypted Session ─────────►│ Derive shared key
   │◄─── Session Token ───────────────│
   │                                 │
   │──── WiFi Config (encrypted) ──►│ SSID: "MyNetwork"
   │                                 │ Password: "MyPassword123"
   │◄─── Config Ack ──────────────────│
   │                                 │
   │◄─── Connection Status ───────────│ SUCCESS/FAILURE
   │                                 │
   │──── Disconnect ────────────────►│
```

---

## 5. ESP32 BLE Architecture Deep Dive

### 5.1 ESP32 BLE Stack Layers

```
┌─────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                        │
│  (Our BLE Provisioning Code)                                │
├─────────────────────────────────────────────────────────────┤
│                     GATT LAYER                              │
│  (Attributes, Services, Characteristics)                     │
├─────────────────────────────────────────────────────────────┤
│                     ATT LAYER                                │
│  (Attribute Protocol - read/write requests)                 │
├─────────────────────────────────────────────────────────────┤
│                     SMP LAYER                                │
│  (Security Manager Protocol - pairing/bonding)             │
├─────────────────────────────────────────────────────────────┤
│                     L2CAP LAYER                              │
│  (Logical Link Control - channels, MTU negotiation)         │
├─────────────────────────────────────────────────────────────┤
│                     HCI LAYER                                │
│  (Host Controller Interface)                                │
├─────────────────────────────────────────────────────────────┤
│                     LINK LAYER                               │
│  (Physical radio, advertising, connections)                │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 BLE Roles in Our Application

| Role | Description | Who Does This |
|------|-------------|---------------|
| **GATT Server** | Exposes data to clients | ESP32 (our device) |
| **GATT Client** | Reads/writes data | iPhone app |
| **Advertiser** | Broadcasts presence | ESP32 |
| **Scanner** | Finds devices | iPhone app |

### 5.3 ESP32 BLE Configuration

In `sdkconfig`, we need these settings:

```
# Enable Bluetooth
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y

# BLE only (not Classic Bluetooth)
CONFIG_BT_BLE_ENABLED=y

# BLE GATT server support
CONFIG_GATTS_ENABLE=y

# BLE security
CONFIG_BLE_SECURITY_ENABLED=y

# Default MTU size (for larger packets)
CONFIG_BLE_MAX_CONN=6
CONFIG_BLE_MAX_CONN_INT=6
```

### 5.4 BLE Memory Footprint

| Component | Memory Usage |
|-----------|-------------|
| BLE Stack (running) | ~60 KB |
| GATT Server | ~30 KB |
| Protocomm | ~20 KB |
| **Total** | **~110 KB** |

This is acceptable for most ESP32 applications. The memory is reclaimed when BLE is disabled after provisioning.

### 5.5 BLE Advertising

The device advertises with these parameters:

```c
esp_ble_adv_params_t adv_params = {
    .adv_type = ADV_TYPE_IND,           // Connectable
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .adv_channel_map = ADV_CHNL_37_38_39,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .interval_min = 0x100,              // ~100ms
    .interval_max = 0x200,               // ~200ms
};

// In advertisement data:
uint8_t device_name[] = "ATOMS3R_1234";
esp_ble_gap_config_adv_data(adv_data);
```

### 5.6 BLE Connection Parameters

```c
esp_gatt_conn_params_t conn_params = {
    .interval_min = 0x10,      // 20ms
    .interval_max = 0x20,      // 40ms
    .latency = 0,
    .supervision_timeout = 0x100,  // 4 seconds
};
```

---

## 6. The Protocomm Protocol

### 6.1 What is Protocomm?

**Protocomm** is Espressif's abstraction layer for secure device communication. It sits on top of various transports (BLE, SoftAP HTTP, UART) and provides:

- Session establishment
- Security handshake
- Endpoint-based communication

```
┌─────────────────────────────────────────────┐
│              Application Code                │
├─────────────────────────────────────────────┤
│           Protocomm Endpoints                │
│  ┌─────────────┐  ┌─────────────┐           │
│  │ wifi-config │  │ custom-data │           │
│  └─────────────┘  └─────────────┘           │
├─────────────────────────────────────────────┤
│         Protocomm Security Layer             │
│  (Security 0/1/2 implementation)            │
├─────────────────────────────────────────────┤
│       Protocomm Transport Layer              │
│  ┌───────┐ ┌───────┐ ┌───────┐             │
│  │ BLE   │ │ HTTP  │ │ UART  │             │
│  └───────┘ └───────┘ └───────┘             │
└─────────────────────────────────────────────┘
```

### 6.2 Endpoints

Endpoints are logical channels for specific data types:

| Endpoint Name | Purpose | Data Format |
|--------------|---------|-------------|
| `wifi-config` | Receive WiFi credentials | protobuf |
| `wifi-status` | Report WiFi connection status | protobuf |
| `custom-data` | Application-specific config | Any format |
| `security-sec1` | Security handshake | Custom |

### 6.3 Protocomm Security Schemes

#### Security 0: No Security
```python
# Simple request/response, no encryption
# FOR TESTING ONLY
{
    "type": "wifi_credentials",
    "ssid": "MyNetwork",
    "password": "MyPassword"
}
```

#### Security 1: Curve25519 + AES-256-CTR (Recommended)

```
┌──────────┐                                    ┌──────────┐
│   Phone  │                                    │  ESP32   │
└────┬─────┘                                    └────┬─────┘
     │                                             │
     │  1. Generate ephemeral key pair             │
     │     (private: sk_ph, public: pk_ph)          │
     │                                             │
     │─────── pk_ph ─────────────────────────────► │
     │                                             │ 2. Generate key pair
     │                                             │    (private: sk_dev, public: pk_dev)
     │                                             │
     │◄────── pk_dev ───────────────────────────── │
     │                                             │
     │  3. Shared key derivation                   │
     │     DH(sk_ph, pk_dev) = shared_secret       │
     │                                             │
     │  4. Encrypt with AES-256-CTR               │
     │     ciphertext = AES-CTR(shared_secret,    │
     │                   nonce, plaintext)         │
     │                                             │
     │─────── ciphertext ─────────────────────────► │
     │                                             │
     │                                             │ 5. Decrypt with shared_secret
     │                                             │    plaintext = AES-CTR(shared_secret,
     │                                             │                   nonce, ciphertext)
```

#### Security 2: SRP6a (Password-based)

More complex but provides mutual authentication. Uses Salt + Verifier stored on device.

### 6.4 Proof of Possession (PoP)

PoP adds a PIN code that the user must enter on their phone:

```python
# The 6-digit PIN is combined with the shared key
combined_key = HKDF(
    key=shared_secret,
    salt=POP,
    info="poffds-proto-comms-key-gen"
)
```

This prevents someone nearby from:
- Discovering the device
- Attempting to send credentials
- Potentially connecting to the wrong network

---

## 7. Security: How Credentials Are Protected

### 7.1 Threat Model

We need to protect against these threats:

| Threat | Description | Mitigation |
|--------|-------------|------------|
| **Eavesdropping** | Someone sniff BLE traffic | Encryption (Security 1/2) |
| **Man-in-the-Middle** | Rogue device impersonating printer | Device authentication |
| **Unauthorized Provisioning** | Unauthorized user sending WiFi | Proof of Possession PIN |
| **Credential Storage** | WiFi password stored insecurely | NVS encryption |
| **Replay Attacks** | Replay captured packets | Nonce/sequence numbers |

### 7.2 BLE Security Levels

| Level | Name | Description | Use Case |
|-------|------|-------------|----------|
| 1 | No security | Open BLE connection | Testing |
| 2 | "Just works" | Encrypted but no authentication | Consumer products |
| 3 | Passkey entry | 6-digit PIN on both devices | Higher security |
| 4 | Numeric comparison | Display numbers to compare | High security |
| 5 | Out-of-band | External channel (NFC, QR) | Maximum security |

For BLE provisioning, we typically use **LE Secure Connections** with **Security 1** (Curve25519) and **PoP**.

### 7.3 Credential Storage

WiFi credentials are stored in **NVS (Non-Volatile Storage)**:

```c
#include "nvs_flash.h"
#include "nvs.h"

// Store WiFi credentials
nvs_handle_t nvs;
nvs_open("wifi", NVS_READWRITE, &nvs);
nvs_set_str(nvs, "ssid", "MyNetwork");
nvs_set_str(nvs, "password", "MyPassword123");
nvs_commit(nvs);
nvs_close(nvs);
```

For production, consider enabling **NVS encryption**:

```
CONFIG_NVS_ENCRYPTION=y
CONFIG_NVS_SEC贞_KEY_PROTECT_USING_EFUSE=y
```

### 7.4 Security Checklist

- [ ] Enable BLE secure connections
- [ ] Use Security 1 or 2 (not Security 0)
- [ ] Implement Proof of Possession (PoP)
- [ ] Encrypt NVS storage
- [ ] Validate SSID format
- [ ] Limit provisioning attempts
- [ ] Clear credentials on factory reset

---

## 8. Current ATOM Printer Firmware Analysis

### 8.1 Current Architecture

Looking at the existing ATOM Printer firmware:

```
Current ATOM Printer Architecture:
┌────────────────────────────────────────────────────┐
│                   PRINTER_FW.ino                    │
├────────────────────────────────────────────────────┤
│                                                    │
│  ┌──────────────┐    ┌─────────────────────────┐  │
│  │ WiFi Manager │    │   HTTP Server           │  │
│  │              │    │                         │  │
│  │ - CreateAP() │    │ GET /                    │  │
│  │ - Connect()  │    │ GET /Connect?ssid=...   │  │
│  │ - HandleWiFi │    │ GET /device_status      │  │
│  │ - CheckConn()│    │ POST /wifi_config       │  │
│  └──────────────┘    └─────────────────────────┘  │
│                                                    │
│  ┌──────────────┐    ┌─────────────────────────┐  │
│  │ MQTT Client  │    │   Printer Control        │  │
│  │              │    │                         │  │
│  │ - Connect()  │    │ - Print Text            │  │
│  │ - Subscribe()│    │ - Print QR/Barcode       │  │
│  │ - Publish()  │    │ - Feed/Paper control    │  │
│  └──────────────┘    └─────────────────────────┘  │
│                                                    │
└────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│         ATOM_PRINTER.cpp            │
│   (Thermal printer hardware driver) │
└─────────────────────────────────────┘
```

### 8.2 Key Files

| File | Purpose |
|------|---------|
| `PRINTER_FW.ino` | Main application, setup/loop |
| `ATOM_PRINTER_WIFI.cpp/.h` | WiFi SoftAP mode, connection logic |
| `ATOM_PRINTER_WEB.cpp/.h` | HTTP server, web interface |
| `ATOM_PRINTER_MQTT.cpp/.h` | MQTT client, print commands |
| `ATOM_PRINTER_HTML.h` | Web UI HTML/CSS |
| `ATOM_PRINTER_CONFIG.h` | Configuration constants |
| `src/ATOM_PRINTER.cpp/.h` | Thermal printer driver |
| `src/ATOM_PRINTER_CMD.h` | Printer command definitions |

### 8.3 Current WiFi Flow

```cpp
// Simplified from ATOM_PRINTER_WIFI.cpp

void setup() {
    // Check if WiFi is already configured
    if (hasWiFiConfig()) {
        // Try to connect to saved network
        connectWiFi();
    } else {
        // Start SoftAP for configuration
        startConfigMode();
    }
}

void startConfigMode() {
    // Create access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ATOM-PRINTER-" + MAC_LAST4);
    
    // Start HTTP server
    server.on("/Connect", handleWiFiConnect);
    server.begin();
    
    // Show web interface at http://192.168.4.1
}

void handleWiFiConnect() {
    String ssid = server.arg("ssid");
    String password = server.arg("pass");
    
    // Save to NVS
    saveWiFiConfig(ssid, password);
    
    // Try to connect
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection...
    // If success, switch to normal mode
    // If fail, restart config mode
}
```

### 8.4 What's Missing for BLE Provisioning

| Component | Current Status | Needed for BLE |
|-----------|---------------|----------------|
| **BLE Stack** | ❌ Not included | ✅ Add esp_bt controller |
| **GATT Server** | ❌ None | ✅ Implement provisioning service |
| **Protocomm** | ❌ Not used | ✅ Add wifi_provisioning component |
| **Security** | ❌ None | ✅ Implement Curve25519 + PoP |
| **NVS WiFi storage** | ✅ Existing | ✅ Reuse |
| **MQTT** | ✅ Existing | ✅ Keep |
| **Print functions** | ✅ Existing | ✅ Keep |

### 8.5 Proposed Architecture Addition

```
New ATOMS3R Architecture:
┌─────────────────────────────────────────────────────────────┐
│                     MAIN APPLICATION                         │
├────────────────┬────────────────┬───────────────────────────┤
│  BLE Provider  │  MQTT Client   │   Printer Control         │
│  (NEW)          │  (existing)    │   (existing)              │
└───────┬─────────┴───────┬────────┴────────────┬─────────────┘
        │                 │                      │
        ▼                 ▼                      ▼
┌──────────────┐   ┌──────────────┐     ┌──────────────────┐
│ Protocomm    │   │ WiFi Manager │     │  ATOM_PRINTER    │
│ (NEW)        │   │ (modified)   │     │  (unchanged)     │
└───────┬──────┘   └──────────────┘     └──────────────────┘
        │
        ▼
┌──────────────┐   ┌──────────────┐
│ WiFi Prov    │   │ NVS Storage  │
│ Manager (NEW)│   │ (existing)   │
└──────────────┘   └──────────────┘
```

---

## 9. Gap Analysis: What We Need to Build

### 9.1 Components to Add

| # | Component | Description | Complexity |
|---|-----------|-------------|------------|
| 1 | BLE Stack | Enable BLE, initialize controller | Low |
| 2 | GATT Server | Implement provisioning service | Medium |
| 3 | Protocomm | Add wifi_provisioning component | Medium |
| 4 | Security | Implement Security 1 with PoP | Medium |
| 5 | Provisioning Flow | Handle WiFi credentials, verify connection | Medium |
| 6 | App Integration | Test with iOS app | Low |

### 9.2 Components to Modify

| # | Component | Changes Needed |
|---|-----------|----------------|
| 1 | WiFi Manager | Add BLE provisioning mode trigger |
| 2 | Configuration Flow | Support dual modes (BLE + SoftAP fallback) |
| 3 | Boot Sequence | Check provisioning state on startup |

### 9.3 Components to Keep

- ATOM_PRINTER.cpp/h (thermal printer driver)
- MQTT client
- Print functions (text, QR, barcode)
- NVS storage (reused for WiFi credentials)

### 9.4 Estimated Effort

| Task | Time |
|------|------|
| BLE stack setup | 2-4 hours |
| Protocomm integration | 4-6 hours |
| Security implementation | 4-8 hours |
| Provisioning flow | 4-6 hours |
| Testing & debugging | 4-8 hours |
| **Total** | **18-32 hours** |

---

## 10. iOS/Android App Integration

### 10.1 Available Apps

Espressif provides free provisioning apps:

| Platform | App Name | App Store Link |
|----------|----------|----------------|
| iOS | ESP BLE Provisioning | [Link](https://apps.apple.com/us/app/esp-ble-provisioning/id1465017836) |
| iOS | ESP SoftAP Provisioning | [Link](https://apps.apple.com/us/app/esp-softap-provisioning/id1474040630) |
| Android | ESP BLE Provisioning | [Link](https://play.google.com/store/apps/details?id=com.espressif.provble) |

### 10.2 App Features

The ESP BLE Provisioning app provides:
- BLE device scanning (filters by name prefix)
- WiFi network scanning
- Credentials transmission
- Connection status feedback
- Proof of Possession input

### 10.3 App Configuration

We configure the app behavior through the firmware:

```c
// Device advertisement configuration
#define BLE_DEVICE_NAME "ATOMS3R"  // App looks for this prefix

// Security configuration
#define SECURITY_TYPE 1            // Curve25519 + PoP
#define POP_CODE "123456"          // 6-digit PIN

// WiFi capabilities
#define WIFI_SCAN_METHOD WIFI_FAST_SCAN
```

### 10.4 Custom App Development

If we need a custom app (for branding, additional features):

```swift
// iOS using CoreBluetooth

import CoreBluetooth

class ProvisioningManager: NSObject {
    var centralManager: CBCentralManager!
    var discoveredDevice: CBPeripheral?
    var provisioningCharacteristic: CBCharacteristic?
    
    func scan() {
        centralManager.scanForPeripherals(
            withServices: nil,
            options: [CBCentralManagerScanOptionAllowDuplicatesKey: false]
        )
    }
    
    func connect(to device: CBPeripheral) {
        centralManager.connect(device, options: nil)
    }
}
```

The app must implement the **WiFi Provisioning protocol** defined by Espressif. This is documented in:
- `esp-idf/provisioning.md`
- Source code: [esp-idf-provisioning-ios](https://github.com/espressif/esp-idf-provisioning-ios)

---

## 11. File Structure and References

### 11.1 Key Documentation Files

| File | Description |
|------|-------------|
| `reference/01-esp32-provisioning-index.md` | Official ESP-IDF provisioning documentation |
| `reference/12-wifi_provisioning_api.md` | Unified Provisioning API reference |
| `reference/13-protocomm.md` | Protocomm protocol reference |
| `reference/07-wifi_prov_example.md` | Example WiFi provisioning code |

### 11.2 Key Espressif Resources

| Resource | URL |
|----------|-----|
| Provisioning Guide | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/index.html |
| WiFi Provisioning API | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html |
| network_provisioning component | https://github.com/espressif/idf-extra-components/tree/master/network_provisioning |
| wifi_prov example | https://github.com/espressif/idf-extra-components/tree/master/network_provisioning/examples/wifi_prov |
| iOS App Source | https://github.com/espressif/esp-idf-provisioning-ios |
| Android App Source | https://github.com/espressif/esp-idf-provisioning-android |

### 11.3 Related ATOM Printer Files

| File | Path | Relevance |
|------|------|----------|
| PRINTER_FW.ino | `ATOM-PRINTER/examples/PRINTER_FW/PRINTER_FW.ino` | Main firmware file to modify |
| ATOM_PRINTER_WIFI.cpp | `ATOM-PRINTER/examples/PRINTER_FW/ATOM_PRINTER_WIFI.cpp` | Current WiFi implementation |
| ATOM_PRINTER_CONFIG.h | `ATOM-PRINTER/examples/PRINTER_FW/ATOM_PRINTER_CONFIG.h` | Configuration constants |

### 11.4 Required ESP-IDF Components

Add to `idf_component.yml`:

```yaml
## idf_component.yml
dependencies:
  espressif/wifi_provisioning:
    version: "^1.0.0"
  espressif/esp-tls:
    version: "^1.0.0"
```

---

## 12. Summary and Next Steps

### 12.1 Key Takeaways

1. **BLE Provisioning** allows iPhone users to configure WiFi without disconnecting
2. **Protocomm** abstracts the complexity of BLE communication and security
3. **Security 1 with PoP** provides good security while remaining user-friendly
4. **The existing printer firmware** has most infrastructure we need; we just need to add BLE provisioning
5. **Espressif provides free iOS/Android apps** that can be used immediately

### 12.2 Design Decisions Made

| Decision | Rationale |
|----------|-----------|
| BLE over SoftAP | Works on iOS without network switching |
| Security 1 over Security 2 | Simpler implementation, good security |
| Keep existing MQTT/print | Reuse proven functionality |
| Dual-mode (BLE + SoftAP) | Maximum compatibility |

### 12.3 Next Steps (Design Document)

The accompanying design document will cover:
- Detailed component specifications
- File structure for the new firmware
- API specifications
- Implementation checklist
- Testing procedures

---

*Document: ATOMS3R BLE-Provisioning Firmware Analysis*  
*Ticket: ATOMS3R-BLEPROV*  
*Created: 2026-04-22*  
*Version: 1.0*