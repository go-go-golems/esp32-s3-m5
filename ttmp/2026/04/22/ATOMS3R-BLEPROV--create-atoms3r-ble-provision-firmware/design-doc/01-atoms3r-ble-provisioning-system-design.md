---
title: "ATOMS3R BLE-Provisioning System Design"
tags:
  - article
  - design-doc
  - firmware
  - esp32
  - ble
  - provisioning
created: 2026-04-22
ticket: ATOMS3R-BLEPROV
status: active
type: design-doc
intent: long-term
topics:
  - firmware
  - esp32
  - ble
  - provisioning
  - ios
  - m5stack
---

# ATOMS3R BLE-Provisioning System Design

> **Purpose**: Define the architecture, components, and implementation details for adding BLE provisioning to the ATOMS3R (M5Stack ATOM thermal printer)  
> **Audience**: Firmware developers, technical reviewers  
> **Prerequisites**: Read `analysis/01-atoms3r-ble-provisioning-firmware-analysis.md`

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [System Architecture](#2-system-architecture)
3. [Component Specifications](#3-component-specifications)
4. [File Structure](#4-file-structure)
5. [API Specifications](#5-api-specifications)
6. [Data Structures](#6-data-structures)
7. [State Machines](#7-state-machines)
8. [Configuration](#8-configuration)
9. [Implementation Checklist](#9-implementation-checklist)
10. [Testing Procedures](#10-testing-procedures)
11. [Security Considerations](#11-security-considerations)

---

## 1. Executive Summary

### 1.1 Problem Statement

The current ATOM Printer firmware requires users to:
1. Disconnect from their WiFi network
2. Connect to the printer's SoftAP network (`ATOM-PRINTER-xxxx`)
3. Open a web browser to `192.168.4.1`
4. Enter their WiFi credentials
5. Wait for the printer to connect
6. Reconnect their phone to their WiFi

This process is **confusing on iOS** because iOS restricts apps from programmatically switching WiFi networks.

### 1.2 Proposed Solution

Add **BLE (Bluetooth LE) provisioning** capability so users can:
1. Stay on their existing WiFi
2. Scan for the printer via Bluetooth
3. Enter their WiFi credentials directly in the app
4. Let the printer connect automatically

### 1.3 Key Design Decisions

| Aspect | Decision | Rationale |
|--------|----------|-----------|
| **Primary Transport** | BLE | iOS compatible, no network switch needed |
| **Fallback Transport** | SoftAP HTTP | For devices without BLE |
| **Security** | Security 1 (Curve25519) | Good balance of security and simplicity |
| **Proof of Possession** | 6-digit PIN | Prevents unauthorized provisioning |
| **Device Name** | `ATOMS3R_XXXX` | Easy to identify during scanning |
| **BLE Service UUID** | Custom (see below) | Following Espressif convention |

---

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           iPhone / Android                              │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    ESP BLE Provisioning App                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │   │
│  │  │ Device Scan │  │ Credentials │  │   Connection Status     │ │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ BLE GATT
                                    │
┌─────────────────────────────────────────────────────────────────────────┐
│                           ATOMS3R Firmware                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                     Main Application Loop                        │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐               │   │
│  │  │ BLE Prov   │  │ MQTT Client│  │  Printer   │               │   │
│  │  │ Manager    │  │            │  │  Control   │               │   │
│  │  └─────┬──────┘  └─────┬──────┘  └──────┬─────┘               │   │
│  └────────┼───────────────┼─────────────────┼───────────────────────┘   │
│           │               │                 │                            │
│           ▼               ▼                 ▼                            │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────────────┐ │
│  │ Protocomm Layer │ │   WiFi Manager   │ │   ATOM_PRINTER Driver   │ │
│  │ (Security 1)    │ │   (BLE+SoftAP)   │ │   (Thermal printer)     │ │
│  └────────┬────────┘ └────────┬────────┘ └─────────────────────────┘ │
│           │                    │                                       │
│           ▼                    ▼                                       │
│  ┌─────────────────┐ ┌─────────────────┐                             │
│  │  BLE GATT       │ │   NVS Storage   │                             │
│  │  Server         │ │   (WiFi creds)  │                             │
│  └─────────────────┘ └─────────────────┘                             │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Component Interaction Diagram

```
                         ┌──────────────────┐
                         │   iPhone App     │
                         └────────┬─────────┘
                                  │
                    BLE GATT      │
                    ┌──────────────┴──────────────┐
                    │                             │
              ┌─────┴─────┐               ┌───────┴─────┐
              │  Version  │               │   Session   │
              │  Read     │               │   R/W/N     │
              └───────────┘               └───────┬─────┘
                                                   │
                                            ┌──────┴──────┐
                                            │  Protocomm  │
                                            │  Security 1 │
                                            └──────┬──────┘
                                                   │
                          ┌────────────────────────┴────────────┐
                          │                                     │
                   ┌──────┴──────┐                    ┌────────┴────────┐
                   │ wifi-config │                    │    custom-data   │
                   │  Endpoint   │                    │    Endpoint      │
                   └──────┬──────┘                    └────────┬────────┘
                          │                                 │
                          ▼                                 │
                   ┌──────────────────┐                     │
                   │  WiFi Manager    │                     │
                   │                  │                     │
                   │  - Store creds   │                     │
                   │  - Connect()     │                     │
                   │  - Status        │                     │
                   └────────┬─────────┘                     │
                            │                               │
                            ▼                               │
                   ┌──────────────────┐                     │
                   │    NVS Storage   │                     │
                   │                  │                     │
                   │  ssid            │                     │
                   │  password        │                     │
                   │  provisioned     │                     │
                   └──────────────────┘                     │
```

### 2.3 Data Flow Sequence

```
┌──────────┐     ┌───────────┐     ┌──────────┐     ┌───────────┐     ┌──────────┐
│   User   │     │  iPhone   │     │  GATT    │     │ Protocomm│     │   WiFi   │
│          │     │   App     │     │  Server  │     │          │     │  Manager │
└────┬─────┘     └─────┬─────┘     └─────┬─────┘     └─────┬─────┘     └────┬─────┘
     │                │                │                │                │
     │ Tap device     │                │                │                │
     │────────────────>│                │                │                │
     │                │ BLE Connect    │                │                │
     │                │───────────────>│                │                │
     │                │                │                │                │
     │                │<───────────────│ Connection OK  │                │
     │                │                │                │                │
     │                │ Key Exchange   │                │                │
     │                │───────────────>│ Curve25519     │                │
     │                │                │───────────────>│                │
     │                │                │<───────────────│ Public Key     │
     │                │<───────────────│                │                │
     │                │                │                │                │
     │ Enter PIN      │                │                │                │
     │<───────────────│                │                │                │
     │                │                │                │                │
     │ Enter SSID     │                │                │                │
     │ and Password   │                │                │                │
     │────────────────>│                │                │                │
     │                │                │                │                │
     │                │ Encrypted creds│                │                │
     │                │───────────────>│                │                │
     │                │                │───────────────>│                │
     │                │                │                │                │
     │                │                │<───────────────│ Decrypt OK     │
     │                │                │                │                │
     │                │                │                │ Store creds    │
     │                │                │                │───────────────>│
     │                │                │                │                │
     │                │                │                │    Connect     │
     │                │                │                │<───────────────│
     │                │                │                │                │
     │                │                │ Status: Trying │                │
     │                │<───────────────│<───────────────│                │
     │ "Connecting..."│                │                │                │
     │<──────────────│                │                │                │
     │                │                │                │                │
     │                │                │                │  Connected!    │
     │                │                │                │<───────────────│
     │                │<───────────────│ Status: OK     │                │
     │ "Printer       │                │                │                │
     │  connected!"   │                │                │                │
     │<───────────────│                │                │                │
     │                │                │                │                │
     │                │ Disconnect     │                │                │
     │                │───────────────>│                │                │
     │                │                │                │                │
```

---

## 3. Component Specifications

### 3.1 BLE GATT Server

#### Service Definition

```
Service Name: WiFi Provisioning
Service UUID: 0xFFFF (placeholder - will use actual Espressif UUID)
Primary Service: Yes

Characteristics:
├── Version Info (Read)
│   ├── UUID: 0xFF01
│   ├── Properties: Read
│   └── Value: "1.0" or similar version string
│
├── Session Info (Write/Notify)
│   ├── UUID: 0xFF02
│   ├── Properties: Write, Write No Response, Notify
│   ├── Description: Main provisioning communication channel
│   └── Max Length: 512 bytes (negotiated MTU)
│
└── Custom Data (Write/Read)
    ├── UUID: 0xFF03
    ├── Properties: Write, Read, Write No Response
    ├── Description: Application-specific data (MQTT config, etc.)
    └── Max Length: 256 bytes
```

#### BLE Advertisement Data

```cpp
// In advertising packet
esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,              // "ATOMS3R_XXXX" in scan response
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
};

// Device name in scan response
esp_ble_gap_set_device_name("ATOMS3R_531A");  // Last 4 hex of MAC
```

#### Connection Parameters

```cpp
esp_ble_gap_set_prefer_conn_params = {
    .min_conn_int = 0x10,      // 20ms minimum
    .max_conn_int = 0x20,      // 40ms maximum
    .latency = 0,              // No latency tolerance
    .supervision_timeout = 0x100,  // 4 seconds
};
```

### 3.2 Protocomm Layer

#### Session Protocol

```c
// Session established via characteristic 0xFF02
// Data format: [command_type:1 byte][payload:n bytes]

enum protocomm_cmd {
    PROTOCOMM_CMD_SECURITY = 0x00,
    PROTOCOMM_CMD_WIFI_CONFIG = 0x01,
    PROTOCOMM_CMD_WIFI_STATUS = 0x02,
    PROTOCOMM_CMD_CUSTOM = 0x03,
    PROTOCOMM_CMD_DISCONNECT = 0x04,
};
```

#### Security 1 Handshake

```cpp
// Step 1: Client sends public key
// Payload: [0x00][Curve25519 public key: 32 bytes]

// Step 2: Server responds with its public key
// Payload: [0x01][Curve25519 public key: 32 bytes]

// Step 3: Derive shared secret and continue with encrypted session
```

### 3.3 WiFi Provisioning Manager

#### Endpoint: wifi-config

```cpp
// Receive WiFi credentials
// Payload: protobuf WiFiConfig message

message WiFiConfig {
    string ssid = 1;
    string password = 2;
    // Optional:
    enum AuthMethod {
        WPA2 = 0;
        WPA3 = 1;
        OPEN = 2;
    }
    AuthMethod auth_method = 3;
}
```

#### Endpoint: wifi-status

```cpp
// Report connection status back to phone

message WiFiStatus {
    enum Status {
        CONNECTED = 0;
        DISCONNECTED = 1;
        CONNECTION_FAILED = 2;
    }
    Status status = 1;
    
    // If connected:
    string ip_addr = 2;
    int8 signal_strength = 3;  // RSSI in dBm
    
    // If failed:
    enum FailReason {
        WRONG_PASSWORD = 0;
        NETWORK_NOT_FOUND = 1;
        TIMEOUT = 2;
    }
    FailReason fail_reason = 4;
}
```

### 3.4 Custom Endpoint (Future Use)

```cpp
// Optional: For application-specific configuration
// Could be used for MQTT broker settings, device name, etc.

enum custom_cmd {
    CUSTOM_CMD_GET_CONFIG = 0x00,
    CUSTOM_CMD_SET_MQTT = 0x01,
    CUSTOM_CMD_SET_NAME = 0x02,
};

// Example: Set MQTT broker
{
    "cmd": 0x01,
    "mqtt": {
        "broker": "mqtt.example.com",
        "port": 1883,
        "username": "printer",
        "password": "secret"
    }
}
```

### 3.5 WiFi Manager Integration

```cpp
class WiFiManager {
public:
    enum class Mode {
        BLE_PROVISIONING,  // BLE active, waiting for credentials
        SOFTAP_CONFIG,     // SoftAP active (fallback)
        NORMAL,           // Connected to WiFi, normal operation
        OFFLINE           // No WiFi, printer mode only
    };
    
    // Connect using provisioned credentials
    bool connectWiFi() {
        // Load from NVS
        std::string ssid = nvs_get("wifi", "ssid");
        std::string password = nvs_get("wifi", "password");
        
        // Configure and connect
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &config);
        esp_wifi_connect();
    }
    
    // Status callback
    void onWiFiStatusChanged(WiFiStatus status) {
        // Notify provisioning manager
        wifi_prov_report_status(status);
    }
};
```

---

## 4. File Structure

### 4.1 New Firmware Directory Structure

```
atoms3r-ble-provision/
├── CMakeLists.txt
├── sdkconfig
├── sdkconfig.defaults
├── main/
│   ├── CMakeLists.txt
│   ├── main.cpp                 # Entry point
│   ├── main.h                    # Main header
│   │
│   ├── ble_provisioning/        # NEW: BLE provisioning module
│   │   ├── CMakeLists.txt
│   │   ├── ble_prov.cpp         # BLE GATT server implementation
│   │   ├── ble_prov.h           # BLE provisioning header
│   │   ├── gatt_service.cpp     # GATT service/characteristic handlers
│   │   └── gatt_service.h
│   │
│   ├── provisioning/            # NEW: Protocomm integration
│   │   ├── CMakeLists.txt
│   │   ├── prov_manager.cpp     # WiFi provisioning manager
│   │   ├── prov_manager.h
│   │   ├── security.cpp         # Security 1 implementation
│   │   └── endpoints.cpp        # Custom endpoint handlers
│   │
│   ├── wifi_manager/            # MODIFIED: Unified WiFi manager
│   │   ├── CMakeLists.txt
│   │   ├── wifi_manager.cpp
│   │   ├── wifi_manager.h
│   │   ├── softap_handler.cpp   # SoftAP fallback
│   │   └── softap_handler.h
│   │
│   ├── printer/                 # EXISTING: Keep as-is
│   │   ├── printer_main.cpp
│   │   ├── printer_mqtt.cpp
│   │   └── (other printer files)
│   │
│   └── config/
│       ├── app_config.cpp
│       ├── app_config.h
│       └── nvs_config.cpp
│
├── components/                  # External components
│   ├── wifi_provisioning/       # Espressif component
│   └── (other IDF components)
│
├── test/
│   ├── test_ble_prov.cpp
│   ├── test_wifi_connect.cpp
│   └── test_provisioning.cpp
│
└── README.md
```

### 4.2 Key File Responsibilities

| File | Responsibility |
|------|----------------|
| `main.cpp` | Application entry, initialize all components |
| `ble_prov.cpp` | BLE GATT server setup, advertising, connection handling |
| `gatt_service.cpp` | Handle read/write requests from phone app |
| `prov_manager.cpp` | WiFi provisioning logic, protocomm setup |
| `security.cpp` | Curve25519 key exchange, AES encryption |
| `wifi_manager.cpp` | Unified WiFi mode management |
| `softap_handler.cpp` | Fallback SoftAP provisioning |

### 4.3 CMakeLists.txt Dependencies

```cmake
# main/CMakeLists.txt
idf_component_register(
    SRCS "main.cpp"
         "ble_provisioning/ble_prov.cpp"
         "ble_provisioning/gatt_service.cpp"
         "provisioning/prov_manager.cpp"
         "provisioning/security.cpp"
         "provisioning/endpoints.cpp"
         "wifi_manager/wifi_manager.cpp"
         "wifi_manager/softap_handler.cpp"
    INCLUDE_DIRS "."
                 "ble_provisioning"
                 "provisioning"
                 "wifi_manager"
    REQUIRES
        esp_timer
        esp_ble_mesh
        nvs_flash
        wifi_provisioning
        esp_tls
)
```

---

## 5. API Specifications

### 5.1 BLE Provisioning API

```cpp
// ble_provisioning/ble_prov.h

namespace atoms3r {

class BLEProvManager {
public:
    // Initialize BLE stack and GATT server
    esp_err_t init();
    
    // Start BLE advertising and provisioning
    esp_err_t start_provisioning();
    
    // Stop BLE advertising
    esp_err_t stop_provisioning();
    
    // Deinitialize BLE
    esp_err_t deinit();
    
    // Set event callbacks
    void set_event_callbacks(BLEProvCallbacks* callbacks);
    
    // Check if device is currently being provisioned
    bool is_provisioning();
    
    // Check if provisioning has completed
    bool is_provisioned();
    
    // Reset provisioning state
    esp_err_t reset_provisioning();
};

class BLEProvCallbacks {
public:
    // Called when phone connects
    virtual void on_connected() = 0;
    
    // Called when phone disconnects
    virtual void on_disconnected() = 0;
    
    // Called when provisioning starts
    virtual void on_provisioning_started() = 0;
    
    // Called when provisioning completes
    virtual void on_provisioning_complete() = 0;
    
    // Called on error
    virtual void on_error(esp_err_t err) = 0;
};

} // namespace atoms3r
```

### 5.2 WiFi Provisioning API

```cpp
// provisioning/prov_manager.h

namespace atoms3r {

class WiFiProvManager {
public:
    // Initialize provisioning manager
    esp_err_t init();
    
    // Start WiFi provisioning (BLE or SoftAP mode)
    esp_err_t start_provisioning(ProvisioningMode mode);
    
    // Stop provisioning
    esp_err_t stop_provisioning();
    
    // Get provisioning status
    ProvisioningStatus get_status();
    
    // Set custom configuration endpoint
    esp_err_t add_custom_endpoint(const char* ep_name, 
                                   endpoint_handler_t handler,
                                   void* user_data);
    
    // Check if device is provisioned
    bool is_provisioned();
    
    // Reset provisioning state
    esp_err_t reset();
};

enum class ProvisioningMode {
    BLE_ONLY,      // BLE provisioning only
    SOFTAP_ONLY,   // SoftAP provisioning only
    BLE_FALLBACK_SOFTAP,  // Try BLE first, fallback to SoftAP
    SOFTAP_FALLBACK_BLE,  // Try SoftAP first, fallback to BLE
};

enum class ProvisioningStatus {
    NOT_STARTED,
    IN_PROGRESS,
    SUCCESS,
    FAILED,
};

} // namespace atoms3r
```

### 5.3 WiFi Manager API

```cpp
// wifi_manager/wifi_manager.h

namespace atoms3r {

class WiFiManager {
public:
    enum class Mode {
        OFFLINE,       // No WiFi, printer only
        BLE_PROV,      // BLE provisioning mode
        SOFTAP_CONFIG, // SoftAP configuration mode
        CONNECTING,    // Attempting to connect
        CONNECTED,     // Connected to WiFi
        DISCONNECTED,  // Connected but lost connection
    };
    
    // Initialize WiFi subsystem
    esp_err_t init();
    
    // Connect using stored credentials
    esp_err_t connect();
    
    // Disconnect from WiFi
    esp_err_t disconnect();
    
    // Get current mode
    Mode get_mode();
    
    // Get connection info
    ConnectionInfo get_connection_info();
    
    // Set mode change callback
    void set_mode_callback(ModeChangeCallback callback);
    
    // Clear stored credentials
    esp_err_t clear_credentials();
    
    // Check if credentials are stored
    bool has_credentials();

private:
    // Event handler for WiFi events
    static void wifi_event_handler(void* arg, 
                                   esp_event_base_t event_base,
                                   int32_t event_id,
                                   void* event_data);
};

struct ConnectionInfo {
    std::string ssid;
    std::string ip_address;
    std::string mac_address;
    int8_t rssi;
    uint8_t channel;
};

class ModeChangeCallback {
public:
    virtual void on_mode_changed(Mode old_mode, Mode new_mode) = 0;
    virtual void on_connected(const ConnectionInfo& info) = 0;
    virtual void on_disconnected() = 0;
};

} // namespace atoms3r
```

### 5.4 Configuration API

```cpp
// config/app_config.h

namespace atoms3r {

struct AppConfig {
    // Device name prefix (will become ATOMS3R_XXXX)
    std::string device_name_prefix = "ATOMS3R";
    
    // BLE provisioning
    bool ble_provisioning_enabled = true;
    uint16_t ble_mtu = 512;
    
    // SoftAP fallback
    bool softap_fallback_enabled = true;
    std::string softap_password = "";
    
    // Security
    uint8_t security_type = 1;  // Security 1 (Curve25519 + PoP)
    std::string pop_code = "123456";
    
    // WiFi
    uint32_t wifi_connect_timeout_ms = 30000;
    uint8_t wifi_max_retries = 5;
    
    // MQTT (from existing config)
    std::string mqtt_broker = "mqtt.m5stack.com";
    uint16_t mqtt_port = 1883;
};

class ConfigManager {
public:
    // Load configuration from NVS
    esp_err_t load();
    
    // Save configuration to NVS
    esp_err_t save();
    
    // Get current config
    const AppConfig& get() const;
    
    // Update config
    void update(const AppConfig& config);
    
    // Reset to defaults
    esp_err_t reset();
    
    // Factory reset (clear all configs)
    esp_err_t factory_reset();
};

} // namespace atoms3r
```

---

## 6. Data Structures

### 6.1 BLE GATT Data

```cpp
// GATT service UUIDs
// Following Espressif convention for provisioning
static const char* TAG = "ble_prov";

static const uint16_t PROV_SERVICE_UUID = 0xFFFF;  // TODO: Use actual UUID
static const uint16_t VERSION_CHAR_UUID = 0xFF01;
static const uint16_t SESSION_CHAR_UUID = 0xFF02;
static const uint16_t CUSTOM_CHAR_UUID = 0xFF03;

// Device info
struct ble_device_info_t {
    uint8_t flags;
    char device_name[32];
    uint16_t appearance;
} __attribute__((packed));

// BLE MTU sizes
static const uint16_t BLE_MIN_MTU = 23;
static const uint16_t BLE_DEFAULT_MTU = 512;
static const uint16_t BLE_MAX_MTU = 1024;
```

### 6.2 Protocomm Session Data

```cpp
// Session state machine
enum class session_state_t {
    SESSION_STATE_INIT,
    SESSION_STATE_SECURITY_HANDSHAKE,
    SESSION_STATE_PROVISIONING,
    SESSION_STATE_COMPLETE,
    SESSION_STATE_ERROR,
};

// Session context
struct protocomm_session_t {
    uint32_t session_id;
    session_state_t state;
    
    // Security context
    uint8_t client_pubkey[32];
    uint8_t session_key[32];
    
    // Data buffers
    uint8_t* in_buffer;
    size_t in_buffer_len;
    uint8_t* out_buffer;
    size_t out_buffer_len;
    
    // Timestamps
    uint64_t created_at;
    uint64_t last_activity;
    
    // Custom data
    void* user_data;
};
```

### 6.3 WiFi Credentials

```cpp
// NVS storage keys
namespace NVS_KEYS {
    static const char* NAMESPACE = "atoms3r";
    static const char* SSID = "wifi.ssid";
    static const char* PASSWORD = "wifi.password";
    static const char* PROVISIONED = "wifi.provisioned";
    static const char* DEVICE_NAME = "device.name";
    static const char* MQTT_BROKER = "mqtt.broker";
    static const char* MQTT_PORT = "mqtt.port";
}

// WiFi credentials structure
struct wifi_credentials_t {
    char ssid[32];
    char password[64];
    bool provisioned;
};

// Stored in NVS
// nvs_set_str(nvs, NVS_KEYS::SSID, ssid);
// nvs_set_str(nvs, NVS_KEYS::PASSWORD, password);
// nvs_set_u8(nvs, NVS_KEYS::PROVISIONED, 1);
```

### 6.4 Provisioning Event Data

```cpp
// Event types
enum class prov_event_type_t {
    START,
    CRED_RECV,
    CRED_FAIL,
    CRED_SUCCESS,
    END,
};

// Event data
struct prov_event_data_t {
    union {
        struct {
            char ssid[32];
            char password[64];
        } credentials;
        
        struct {
            esp_err_t error;
            char message[128];
        } error;
        
        struct {
            char ip_address[16];
            int8_t rssi;
        } connection_info;
    };
};
```

---

## 7. State Machines

### 7.1 Main Application State Machine

```
                    ┌─────────────────┐
                    │   INITIALIZE    │
                    │                 │
                    └────────┬────────┘
                             │
                             ▼
              ┌──────────────────────────────┐
              │      CHECK_PROVISIONED       │
              └──────────────┬───────────────┘
                             │
              ┌──────────────┼───────────────┐
              │              │               │
              ▼              ▼               ▼
        ┌───────────┐  ┌───────────┐  ┌───────────┐
        │ PROVISIONED│  │   NOT     │  │  ERROR    │
        │           │  │PROVISIONED│  │           │
        └─────┬─────┘  └─────┬─────┘  └───────────┘
              │              │
              ▼              ▼
        ┌───────────┐  ┌──────────────────────────┐
        │  CONNECT  │  │   START_PROVISIONING      │
        │   TO WIFI │  │                           │
        │           │  │  ┌─────────────────────┐ │
        └─────┬─────┘  │  │  START_BLE_ADVERT   │ │
              │        │  └──────────┬──────────┘ │
              │        │             │             │
              │        │  ┌─────────┴──────────┐  │
              │        │  ▼                    ▼  │
              │        │ WAIT_BLE_CONN    WAIT_SOFTAP │
              │        │      │              │      │
              │        │      └────────┬─────┘      │
              │        │                 │           │
              │        └─────────────────▼───────────┘
              │                    │
              │           ┌────────┴────────┐
              │           ▼                 ▼
              │     ┌────────────┐    ┌────────────┐
              │     │   RECEIVE  │    │   RECEIVE  │
              │     │  CREDENTIALS│   │  CREDENTIALS│
              │     │   (BLE)    │    │  (SoftAP)  │
              │     └──────┬─────┘    └──────┬─────┘
              │            │                  │
              │            └────────┬─────────┘
              │                     │
              │                     ▼
              │            ┌─────────────────┐
              │            │  STORE_CREDENTIALS│
              │            │  CONNECT_TO_WIFI  │
              │            └─────────┬─────────┘
              │                      │
              │           ┌──────────┴──────────┐
              │           ▼                      ▼
              │     ┌───────────┐          ┌───────────┐
              │     │ CONNECTED │          │  FAILED   │
              │     │           │          │           │
              │     └─────┬─────┘          └───────────┘
              │           │
              │           ▼
              │     ┌───────────┐
              │     │   NORMAL  │
              │     │   OPERATION│
              │     │  (MQTT etc)│
              │     └───────────┘
              │
              ▼ (on disconnect or reset)
        ┌───────────┐
        │ CHECK_AGAIN│
        └───────────┘
```

### 7.2 BLE Provisioning State Machine

```
                    ┌─────────────────┐
                    │  BLE_STOPPED    │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   BLE_STARTING  │
                    │  Initialize stack│
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
        ┌───────────┐ ┌───────────┐ ┌───────────┐
        │  SUCCESS  │ │  FAILED   │ │ TIMEOUT   │
        └─────┬─────┘ └───────────┘ └───────────┘
              │
              ▼
        ┌─────────────────┐
        │   BLE_RUNNING   │
        │   Start advert  │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │   ADVERTISING   │
        │                 │
        └────────┬────────┘
                 │
                 ├──────────────┬──────────────┐
                 ▼              ▼              ▼
          ┌───────────┐  ┌───────────┐  ┌───────────┐
          │ CONNECTED │  │  TIMEOUT  │  │  STOPPED  │
          │ by phone  │  │ (no conn) │  │  by app   │
          └─────┬─────┘  └───────────┘  └───────────┘
                │
                ▼
        ┌─────────────────┐
        │  HANDSHAKE      │
        │  Security 1    │
        │  Key exchange   │
        └────────┬────────┘
                 │
          ┌──────┴──────┐
          ▼             ▼
    ┌───────────┐ ┌───────────┐
    │  SUCCESS  │ │  FAILED   │
    └─────┬─────┘ └───────────┘
          │
          ▼
    ┌─────────────────┐
    │  PROVISIONING   │
    │  Receive WiFi   │
    │  credentials    │
    └────────┬────────┘
             │
      ┌──────┴──────┐
      ▼             ▼
┌───────────┐ ┌───────────┐
│ COMPLETE  │ │   ERROR    │
│ Success!  │ │           │
└───────────┘ └───────────┘
```

### 7.3 WiFi Connection State Machine

```
                    ┌─────────────────┐
                    │  WIFI_IDLE      │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   CONNECTING     │
                    │  esp_wifi_connect│
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
        ┌───────────┐  ┌───────────┐  ┌───────────┐
        │   GOT_IP  │  │ DISCONNECT│  │   ERROR   │
        │ Connected │  │ Lost conn │  │ Timeout   │
        └─────┬─────┘  └─────┬─────┘  └─────┬─────┘
              │              │              │
              ▼              ▼              ▼
        ┌───────────┐  ┌───────────┐  ┌───────────┐
        │  CONNECTED│  │RECONNECTING│ │  FAILED   │
        │  Normal   │  │ Retry conn │  │ Reset     │
        │  operation│  └─────┬─────┘  └─────┬─────┘
        └───────────┘        │              │
                              │              │
                              ▼              ▼
                        ┌───────────┐  ┌───────────┐
                        │ Retry N    │  │  Back to   │
                        │ times      │  │  WiFi Idle │
                        └───────────┘  └───────────┘
```

---

## 8. Configuration

### 8.1 sdkconfig Settings

```
# BLE Configuration
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=n        # Use NimBLE instead
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BLE_ENABLE=y
CONFIG_GATTS_ENABLE=y

# BLE Provisioning
CONFIG_WIFI_PROVING=y
CONFIG_WIFI_PROV_BLE=y
CONFIG_WIFI_PROV_BLE_MAX_CONN=6
CONFIG_WIFI_PROV_BLE_DEV_NAME="ATOMS3R"

# Security
CONFIG_WIFI_PROV_SECURITY_1=y
CONFIG_WIFI_PROV_SECURITY_MODE=1

# WiFi
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_STATIC_TX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32

# NVS
CONFIG_NVS_ENCRYPTION=y

# MQTT (from existing)
CONFIG_MQTT_TRANSPORT=TCP
CONFIG_MQTT_PROTOCOL_311=y
```

### 8.2 Application Configuration

```cpp
// config/app_config.cpp

#include "app_config.h"

namespace atoms3r {

AppConfig::AppConfig() {
    // Load defaults
    device_name_prefix = "ATOMS3R";
    ble_provisioning_enabled = true;
    softap_fallback_enabled = true;
    security_type = 1;
    pop_code = "123456";
    wifi_connect_timeout_ms = 30000;
    wifi_max_retries = 5;
    
    // MQTT defaults
    mqtt_broker = "mqtt.m5stack.com";
    mqtt_port = 1883;
}

} // namespace atoms3r
```

### 8.3 Build Configuration

```cmake
# CMakeLists.txt for main component

idf_component_register(
    SRCS "main.cpp"
         "ble_provisioning/ble_prov.cpp"
         "ble_provisioning/gatt_service.cpp"
         "provisioning/prov_manager.cpp"
         "provisioning/security.cpp"
         "provisioning/endpoints.cpp"
         "wifi_manager/wifi_manager.cpp"
         "wifi_manager/softap_handler.cpp"
    INCLUDE_DIRS "."
                 "ble_provisioning"
                 "provisioning"
                 "wifi_manager"
                 "config"
    REQUIRES 
        esp_timer
        nvs_flash
        wifi_provisioning
        esp_tls
        esp_netif
        esp_event
        esp_wifi
)

# Link against wifi_provisioning component
# (This is automatically handled by the component system)
```

---

## 9. Implementation Checklist

### Phase 1: BLE Stack Setup

- [ ] Enable BLE in sdkconfig
- [ ] Initialize NimBLE stack
- [ ] Create GATT service with three characteristics
- [ ] Implement advertising
- [ ] Handle connection/disconnection
- [ ] Test BLE scanning from phone

### Phase 2: Protocomm Integration

- [ ] Add wifi_provisioning component
- [ ] Initialize protocomm with BLE transport
- [ ] Implement Security 1 (Curve25519)
- [ ] Create wifi-config endpoint
- [ ] Create wifi-status endpoint
- [ ] Test key exchange with phone

### Phase 3: WiFi Provisioning Flow

- [ ] Implement credential storage to NVS
- [ ] Implement WiFi connection with credentials
- [ ] Handle connection status reporting
- [ ] Implement retry logic
- [ ] Handle connection failures gracefully

### Phase 4: SoftAP Fallback

- [ ] Create SoftAP fallback mode
- [ ] Implement HTTP provisioning endpoint
- [ ] Add mode selection logic
- [ ] Test dual-mode operation

### Phase 5: Integration & Testing

- [ ] Integrate with existing MQTT printer code
- [ ] Test complete provisioning flow
- [ ] Test iOS app integration
- [ ] Test Android app integration
- [ ] Memory optimization
- [ ] Stress testing

### Phase 6: Production

- [ ] Security audit
- [ ] Documentation
- [ ] Version numbering
- [ ] Release build

---

## 10. Testing Procedures

### 10.1 Unit Tests

```cpp
// test/test_ble_prov.cpp

#include "unity.h"
#include "ble_prov.h"

TEST_CASE("BLE initialization", "[ble_prov]")
{
    esp_err_t ret = ble_prov.init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

TEST_CASE("BLE advertising starts", "[ble_prov]")
{
    esp_err_t ret = ble_prov.start_provisioning();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Wait for advertising
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Check advertising state
    TEST_ASSERT_TRUE(ble_prov.is_advertising());
}

TEST_CASE("GATT service discovery", "[ble_prov]")
{
    // Simulate GATT discovery
    // Check service UUID matches
    // Check characteristics exist
}
```

### 10.2 Integration Tests

```cpp
// test/test_provisioning.cpp

TEST_CASE("Complete provisioning flow", "[integration]")
{
    // 1. Start provisioning
    esp_err_t ret = prov_manager.start_provisioning(BLE_ONLY);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // 2. Simulate phone connection
    // (In real test, use BLE test tool)
    
    // 3. Simulate credential exchange
    uint8_t test_ssid[] = "TestNetwork";
    uint8_t test_password[] = "TestPassword123";
    
    // 4. Verify credentials stored
    char stored_ssid[32];
    nvs_get_str(nvs, "wifi.ssid", stored_ssid, NULL);
    TEST_ASSERT_EQUAL_STRING((char*)test_ssid, stored_ssid);
}
```

### 10.3 Field Tests

| Test | Steps | Expected Result |
|------|-------|-----------------|
| iOS BLE provisioning | Use ESP BLE Provisioning app | Device connects to WiFi |
| Android BLE provisioning | Use Play Store app | Device connects to WiFi |
| SoftAP fallback | Disable BLE, trigger fallback | HTTP server starts |
| Connection retry | Break WiFi, restore | Reconnects automatically |
| Memory leak | Run 24 hours | No memory growth |
| Concurrent connections | Connect 3 phones | All handled correctly |

---

## 11. Security Considerations

### 11.1 Threat Matrix

| Threat | Likelihood | Impact | Mitigation |
|--------|------------|--------|------------|
| Eavesdropping BLE | Medium | High | Security 1 encryption |
| MITM attack | Low | High | Device authentication |
| Unauthorized provisioning | Medium | Medium | Proof of Possession |
| WiFi credential theft | Low | High | NVS encryption |
| Replay attacks | Low | Medium | Nonce in encryption |

### 11.2 Security Implementation Checklist

- [ ] Use LE Secure Connections (not Legacy)
- [ ] Implement Security 1 or 2 (not Security 0)
- [ ] Require Proof of Possession (6-digit PIN)
- [ ] Enable NVS encryption
- [ ] Clear credentials on factory reset
- [ ] Limit provisioning attempts (max 5)
- [ ] Use random connection intervals (prevent timing attacks)
- [ ] Validate SSID format (no special characters)
- [ ] Implement secure storage for passwords

### 11.3 Recommended PoP Code

For production, use a randomly generated 6-digit code:
- Printed on the device label
- Displayed on first boot
- QR code for easy entry

---

## 12. Appendices

### A. BLE UUID Reference

```
Service: WiFi Provisioning
UUID: To be assigned

Characteristics:
- Version: 0xFF01 (Read)
- Session: 0xFF02 (Write, Write No Response, Notify)
- Custom: 0xFF03 (Write, Read, Write No Response)
```

### B. Error Codes

```cpp
// Custom error codes for ATOMS3R provisioning
#define ERR_BLE_PROV_BASE         0x1000
#define ERR_BLE_INIT_FAILED       (ERR_BLE_PROV_BASE + 0x01)
#define ERR_BLE_ADV_FAILED        (ERR_BLE_PROV_BASE + 0x02)
#define ERR_BLE_CONN_FAILED       (ERR_BLE_PROV_BASE + 0x03)
#define ERR_BLE_SECURITY_FAILED   (ERR_BLE_PROV_BASE + 0x04)
#define ERR_BLE_TIMEOUT           (ERR_BLE_PROV_BASE + 0x05)

#define ERR_PROV_BASE             0x2000
#define ERR_PROV_INVALID_SSID     (ERR_PROV_BASE + 0x01)
#define ERR_PROV_WIFI_CONNECT     (ERR_PROV_BASE + 0x02)
#define ERR_PROV_TIMEOUT          (ERR_PROV_BASE + 0x03)
```

### C. Reference Documentation

- [ESP-IDF Provisioning API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/index.html)
- [Network Provisioning Component](https://github.com/espressif/idf-extra-components/tree/master/network_provisioning)
- [WiFi Provisioning Example](https://github.com/espressif/idf-extra-components/tree/master/network_provisioning/examples/wifi_prov)
- [iOS Provisioning App](https://github.com/espressif/esp-idf-provisioning-ios)
- [Android Provisioning App](https://github.com/espressif/esp-idf-provisioning-android)

---

*Document: ATOMS3R BLE-Provisioning System Design*  
*Ticket: ATOMS3R-BLEPROV*  
*Created: 2026-04-22*  
*Version: 1.0*