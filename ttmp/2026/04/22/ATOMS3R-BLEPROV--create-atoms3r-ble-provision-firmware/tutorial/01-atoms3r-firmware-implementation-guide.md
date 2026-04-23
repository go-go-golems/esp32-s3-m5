---
title: "ATOMS3R BLE-Provisioning Firmware Implementation Guide"
tags:
  - article
  - tutorial
  - firmware
  - esp32
  - ble
  - provisioning
  - implementation
created: 2026-04-22
ticket: ATOMS3R-BLEPROV
status: active
type: tutorial
intent: long-term
topics:
  - firmware
  - esp32
  - ble
  - provisioning
  - ios
  - m5stack
---

# ATOMS3R BLE-Provisioning Firmware Implementation Guide

> **Purpose**: Step-by-step tutorial for implementing BLE provisioning on the ATOMS3R (M5Stack ATOM thermal printer)  
> **Audience**: New firmware developer, intern  
> **Prerequisites**: C programming, basic embedded systems, ESP-IDF setup  
> **Goal**: Have a working BLE provisioning implementation by the end

---

## Table of Contents

1. [Overview and Learning Objectives](#1-overview-and-learning-objectives)
2. [Development Environment Setup](#2-development-environment-setup)
3. [Step 1: Create New Project from Scratch](#3-step-1-create-new-project-from-scratch)
4. [Step 2: Configure BLE and WiFi Provisioning](#4-step-2-configure-ble-and-wifi-provisioning)
5. [Step 3: Implement BLE Provisioning Manager](#5-step-3-implement-ble-provisioning-manager)
6. [Step 4: Implement WiFi Connection Handler](#6-step-4-implement-wifi-connection-handler)
7. [Step 5: Add SoftAP Fallback](#7-step-5-add-softap-fallback)
8. [Step 6: Integrate with Existing Printer Code](#8-step-6-integrate-with-existing-printer-code)
9. [Step 7: Build and Flash](#9-step-7-build-and-flash)
10. [Step 8: Test with iOS App](#10-step-8-test-with-ios-app)
11. [Troubleshooting Guide](#11-troubleshooting-guide)
12. [Reference Code Repository](#12-reference-code-repository)

---

## 1. Overview and Learning Objectives

### 1.1 What We're Building

This tutorial will guide you through creating a firmware that allows users to configure WiFi credentials using their iPhone via Bluetooth LE. No more disconnecting from WiFi and connecting to a printer's network!

**By the end of this tutorial, you will understand:**

- How ESP32's BLE stack works
- How Espressif's wifi_provisioning component works
- How to integrate BLE provisioning into existing firmware
- How to test and debug BLE provisioning

### 1.2 Architecture Preview

```
┌─────────────────────────────────────────────────────────────────┐
│                         iPhone                                   │
│              ESP BLE Provisioning App                            │
└─────────────────────────────────────────────────────────────────┘
                              │ BLE GATT
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      ATOMS3R Firmware                            │
│  ┌───────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │ BLE Provision │  │ WiFi Manager │  │    Printer Control    │ │
│  │   Manager     │  │              │  │    (keep existing)    │ │
│  └───────┬───────┘  └──────┬───────┘  └───────────────────────┘ │
│          │                 │                                    │
│          ▼                 ▼                                    │
│  ┌───────────────┐  ┌──────────────┐                             │
│  │ Protocomm     │  │ NVS Storage │                             │
│  │ (Security 1)  │  │ (WiFi creds)│                             │
│  └───────────────┘  └──────────────┘                             │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 What You'll Need

| Item | Description |
|------|-------------|
| **Hardware** | M5Stack ATOM or ATOMS3R device |
| **Computer** | macOS, Linux, or Windows with ESP-IDF installed |
| **iPhone/Android** | For testing provisioning |
| **USB Cable** | To flash the device |
| **WiFi Network** | For testing connection |

---

## 2. Development Environment Setup

### 2.1 Install ESP-IDF

If you don't have ESP-IDF installed, follow these steps:

```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Install tools
./install.sh

# Activate environment
source export.sh
```

### 2.2 Verify Installation

```bash
# Check ESP-IDF version
idf.py --version
# Should show: ESP-IDF v5.x.x

# Check tools
idf.py targets
# Should list: esp32, esp32s2, esp32s3, etc.
```

### 2.3 Create Working Directory

```bash
# Create project directory
mkdir -p ~/projects/atoms3r-ble-provision
cd ~/projects/atoms3r-ble-provision

# Initialize with ESP-IDF template
idf.py create-project --template simple .
```

---

## 3. Step 1: Create New Project from Scratch

### 3.1 Project Structure

Create the following directory structure:

```
atoms3r-ble-provision/
├── main/
│   ├── CMakeLists.txt
│   ├── main.cpp           # Main application
│   ├── main.h             # Header
│   ├── ble_prov.cpp       # BLE provisioning
│   ├── ble_prov.h
│   ├── wifi_manager.cpp   # WiFi handling
│   ├── wifi_manager.h
│   ├── prov_manager.cpp   # Provisioning manager
│   └── prov_manager.h
├── components/
│   └── wifi_provisioning/ # Will be added via idf.py
├── CMakeLists.txt
├── sdkconfig
└── Kconfig
```

### 3.2 Create Main CMakeLists.txt

```cmake
# CMakeLists.txt (project root)

cmake_minimum_required(VERSION 3.5)

# Enable IDF modules
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(atoms3r-ble-provision)
```

### 3.3 Create main/CMakeLists.txt

```cmake
# main/CMakeLists.txt

idf_component_register(
    SRCS 
        "main.cpp"
        "ble_prov.cpp"
        "wifi_manager.cpp"
        "prov_manager.cpp"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        esp_timer
        nvs_flash
        wifi_provisioning
        esp_tls
        esp_netif
        esp_event
        esp_wifi
)
```

### 3.4 Create Basic main.cpp

```cpp
// main/main.cpp

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char* TAG = "atoms3r";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ATOMS3R BLE Provisioning Firmware");
    ESP_LOGI(TAG, "================================");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized");

    // TODO: Initialize WiFi manager
    // TODO: Initialize BLE provisioning
    // TODO: Start main application

    ESP_LOGI(TAG, "System ready!");
}
```

### 3.5 Build Test

```bash
# Navigate to project directory
cd ~/projects/atoms3r-ble-provision

# Build to verify setup
idf.py build

# Should see "Project build complete."
```

---

## 4. Step 2: Configure BLE and WiFi Provisioning

### 4.1 Run Menuconfig

```bash
idf.py menuconfig
```

### 4.2 Enable Bluetooth (Component config → Bluetooth)

```
[*] Bluetooth
    [*] Bluetooth
    [ ] Bluetooth (Bluedroid)     ← Uncheck this
    [*] Bluetooth (NimBLE)       ← Check this
```

### 4.3 Enable WiFi Provisioning (Component config → WiFi Provisioning)

```
[*] Wi-Fi Provisioning
    [*] Enable BLE Provisioning
    [ ] Enable SoftAP Provisioning    (optional, we'll add later)
    [*] Security Mode 1 (Curve25519 + PoP)
    [*] Security Mode 2 (SRP6a)      (optional)
    (ATOMS3R) Device Name Prefix
    (123456) Proof of Possession Code
```

### 4.4 Configure WiFi (Component config → WiFi)

```
[*] Wi-Fi
    [*] WiFi Station
    [*] WiFi SoftAP
    (10) Max WiFi softAP clients
```

### 4.5 Save and Exit

Press `S` to save, then `Q` to quit.

### 4.6 Verify sdkconfig

Check that these lines appear in `sdkconfig`:

```bash
grep -E "CONFIG_BT|CONFIG_WIFI_PROV" sdkconfig
```

Expected output:
```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_WIFI_PROVING=y
CONFIG_WIFI_PROV_BLE=y
CONFIG_WIFI_PROV_SECURITY_1=y
```

---

## 5. Step 3: Implement BLE Provisioning Manager

### 5.1 Understanding wifi_provisioning Component

The `wifi_provisioning` component from Espressif handles all the BLE GATT complexity. We just need to:

1. Create a provisioning manager
2. Register event handlers
3. Start provisioning

### 5.2 Create prov_manager.h

```cpp
// main/prov_manager.h

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct wifi_prov_ctx wifi_prov_ctx_t;

/// WiFi provisioning event types
typedef enum {
    PROV_EVENT_START,
    PROV_EVENT_CRED_RECV,
    PROV_EVENT_CRED_FAIL,
    PROV_EVENT_CRED_SUCCESS,
    PROV_EVENT_END,
} prov_event_type_t;

/// WiFi provisioning event data
typedef struct {
    union {
        struct {
            char ssid[32];
            char password[64];
        } credentials;
        struct {
            int reason;
        } fail;
        struct {
            char ip[16];
        } connected;
    };
} prov_event_data_t;

/// Provisioning callbacks
typedef struct {
    void (*on_event)(prov_event_type_t event, prov_event_data_t* data, void* user_data);
    void* user_data;
} prov_callbacks_t;

/// WiFi provisioning manager class
class WiFiProvManager {
public:
    WiFiProvManager();
    ~WiFiProvManager();

    // Initialize the provisioning manager
    esp_err_t init();

    // Start BLE provisioning
    esp_err_t start_ble_provisioning(const char* device_name, 
                                     uint8_t security,
                                     const char* pop);

    // Start SoftAP provisioning (fallback)
    esp_err_t start_softap_provisioning(const char* softap_password);

    // Stop provisioning
    esp_err_t stop();

    // Check if device is already provisioned
    bool is_provisioned();

    // Reset provisioning state
    esp_err_t reset();

private:
    bool initialized_;
    wifi_prov_ctx_t* context_;
};
```

### 5.3 Create prov_manager.cpp

```cpp
// main/prov_manager.cpp

#include "prov_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

static const char* TAG = "prov_manager";

// Forward declaration
static void prov_event_handler(void* user_data, wifi_prov_event_t event, 
                                union wifi_prov_event_data* data);

// WiFi credentials storage
static char pending_ssid[32] = {0};
static char pending_password[64] = {0};

WiFiProvManager::WiFiProvManager() 
    : initialized_(false), context_(nullptr) {
}

WiFiProvManager::~WiFiProvManager() {
    if (context_) {
        wifi_prov_mgr_deinit();
    }
}

esp_err_t WiFiProvManager::init() {
    ESP_LOGI(TAG, "Initializing WiFi Provisioning Manager");

    // WiFi provisioning manager configuration
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,      // Use BLE for provisioning
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE,
        .app_event_handler = {
            .event_cb = prov_event_handler,
            .user_data = this,
        },
    };

    // Initialize the manager
    esp_err_t ret = wifi_prov_mgr_init(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize provisioning manager: %s", 
                 esp_err_to_name(ret));
        return ret;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "WiFi Provisioning Manager initialized");
    return ESP_OK;
}

esp_err_t WiFiProvManager::start_ble_provisioning(const char* device_name,
                                                   uint8_t security,
                                                   const char* pop) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting BLE provisioning");
    ESP_LOGI(TAG, "  Device name: %s", device_name);
    ESP_LOGI(TAG, "  Security: %d", security);
    ESP_LOGI(TAG, "  PoP: %s", pop);

    // Start provisioning
    // Security: 1 = Curve25519 + PoP
    // pop: 6-digit proof of possession code
    esp_err_t ret = wifi_prov_mgr_start_provisioning(
        (wifi_prov_security_t)security,
        pop,
        device_name,
        nullptr  // No custom data
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start BLE provisioning: %s", 
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "BLE provisioning started successfully");
    return ESP_OK;
}

esp_err_t WiFiProvManager::stop() {
    if (context_) {
        wifi_prov_mgr_stop_provisioning();
    }
    return ESP_OK;
}

bool WiFiProvManager::is_provisioned() {
    bool provisioned = false;
    if (initialized_) {
        esp_err_t ret = wifi_prov_mgr_is_provisioned(&provisioned);
        if (ret == ESP_OK) {
            return provisioned;
        }
    }
    return false;
}

esp_err_t WiFiProvManager::reset() {
    ESP_LOGI(TAG, "Resetting provisioning state");
    return wifi_prov_mgr_reset_provisioning();
}

// Event handler called by the provisioning manager
static void prov_event_handler(void* user_data, wifi_prov_event_t event,
                               union wifi_prov_event_data* data) {
    WiFiProvManager* mgr = (WiFiProvManager*)user_data;

    switch (event) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;

        case WIFI_PROV_CRED_RECV: {
            // WiFi credentials received from phone
            wifi_sta_config_t* creds = &data->cred_recv.creds[0];
            ESP_LOGI(TAG, "Received WiFi credentials:");
            ESP_LOGI(TAG, "  SSID: %.32s", (char*)creds->ssid);
            
            // Store credentials for later use
            memset(pending_ssid, 0, sizeof(pending_ssid));
            memset(pending_password, 0, sizeof(pending_password));
            memcpy(pending_ssid, creds->ssid, sizeof(creds->ssid));
            memcpy(pending_password, creds->password, sizeof(creds->password));
            break;
        }

        case WIFI_PROV_CRED_FAIL: {
            ESP_LOGE(TAG, "WiFi connection failed!");
            ESP_LOGE(TAG, "  Reason: %d", data->cred_fail.reason);
            break;
        }

        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "WiFi credentials applied successfully");
            ESP_LOGI(TAG, "Device will now connect to WiFi...");
            break;

        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning complete");
            // Deinitialize manager after provisioning
            wifi_prov_mgr_deinit();
            break;

        default:
            break;
    }
}
```

### 5.4 Understanding the BLE Service UUIDs

Espressif's wifi_provisioning component automatically creates a GATT service with these characteristics:

| Characteristic | UUID | Purpose |
|---------------|------|---------|
| **Version** | 0xFFFF | Device version info (readable) |
| **Session** | 0xFFFE | Main communication (write/notify) |
| **Custom** | 0xFFFD | Application-specific data (optional) |

The actual UUIDs depend on the device name prefix. The phone app uses these to communicate.

---

## 6. Step 4: Implement WiFi Connection Handler

### 6.1 Create wifi_manager.h

```cpp
// main/wifi_manager.h

#pragma once

#include <string>

// WiFi connection states
enum class WiFiState {
    IDLE,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    FAILED
};

// Connection information
struct ConnectionInfo {
    std::string ssid;
    std::string ip_address;
    std::string mac_address;
    int8_t rssi;
    uint8_t channel;
};

// WiFi manager class
class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();

    // Initialize WiFi subsystem
    esp_err_t init();

    // Connect to saved WiFi
    esp_err_t connect();

    // Disconnect from WiFi
    esp_err_t disconnect();

    // Get current state
    WiFiState get_state() const { return state_; }

    // Get connection info
    ConnectionInfo get_connection_info() const { return conn_info_; }

    // Check if connected
    bool is_connected() const { return state_ == WiFiState::CONNECTED; }

    // Clear saved credentials
    esp_err_t clear_credentials();

    // Check if credentials exist
    bool has_credentials();

    // Set state change callback
    void set_state_callback(void (*callback)(WiFiState));

private:
    WiFiState state_;
    ConnectionInfo conn_info_;
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);
};
```

### 6.2 Create wifi_manager.cpp

```cpp
// main/wifi_manager.cpp

#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char* TAG = "wifi_manager";
static void (*state_callback_)(WiFiState) = nullptr;

// Default WiFi config
wifi_config_t wifi_config = {0};

WiFiManager::WiFiManager() 
    : state_(WiFiState::IDLE), conn_info_() {
    memset(&conn_info_, 0, sizeof(conn_info_));
}

WiFiManager::~WiFiManager() {
    // Cleanup if needed
}

esp_err_t WiFiManager::init() {
    ESP_LOGI(TAG, "Initializing WiFi");

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        ESP_LOGE(TAG, "Failed to create WiFi station netif");
        return ESP_FAIL;
    }

    // WiFi init configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handler
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, this));

    // Set WiFi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    state_ = WiFiState::IDLE;
    ESP_LOGI(TAG, "WiFi initialized");
    return ESP_OK;
}

esp_err_t WiFiManager::connect() {
    ESP_LOGI(TAG, "Connecting to WiFi...");

    // Load credentials from NVS
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("wifi", NVS_READONLY, &nvs);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No WiFi credentials found (NVS error: %s)", 
                 esp_err_to_name(ret));
        state_ = WiFiState::IDLE;
        return ESP_ERR_NOT_FOUND;
    }

    char ssid[32] = {0};
    char password[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t password_len = sizeof(password);

    ret = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SSID from NVS");
        nvs_close(nvs);
        state_ = WiFiState::IDLE;
        return ret;
    }

    ret = nvs_get_str(nvs, "password", password, &password_len);
    nvs_close(nvs);

    // Configure WiFi
    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    // Set configuration
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        state_ = WiFiState::FAILED;
        return ret;
    }

    // Start WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        state_ = WiFiState::FAILED;
        return ret;
    }

    // Connect
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(ret));
        state_ = WiFiState::FAILED;
        return ret;
    }

    state_ = WiFiState::CONNECTING;
    ESP_LOGI(TAG, "WiFi connection initiated");
    return ESP_OK;
}

esp_err_t WiFiManager::disconnect() {
    ESP_LOGI(TAG, "Disconnecting from WiFi");
    esp_wifi_disconnect();
    state_ = WiFiState::DISCONNECTED;
    return ESP_OK;
}

bool WiFiManager::has_credentials() {
    nvs_handle_t nvs;
    if (nvs_open("wifi", NVS_READONLY, &nvs) != ESP_OK) {
        return false;
    }
    
    size_t ssid_len = 0;
    esp_err_t ret = nvs_get_str(nvs, "ssid", nullptr, &ssid_len);
    nvs_close(nvs);
    
    return (ret == ESP_OK && ssid_len > 0);
}

esp_err_t WiFiManager::clear_credentials() {
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("wifi", NVS_READWRITE, &nvs);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_erase_key(nvs, "ssid");
    ret = nvs_erase_key(nvs, "password");
    ret = nvs_erase_key(nvs, "provisioned");
    ret = nvs_commit(nvs);
    nvs_close(nvs);

    ESP_LOGI(TAG, "WiFi credentials cleared");
    return ret;
}

// Event handler
void WiFiManager::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data) {
    WiFiManager* mgr = (WiFiManager*)arg;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to WiFi (waiting for IP)");
                mgr->state_ = WiFiState::CONNECTING;
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t* data = 
                    (wifi_event_sta_disconnected_t*)event_data;
                ESP_LOGW(TAG, "Disconnected from WiFi (reason: %d)", 
                         data->reason);
                
                mgr->state_ = WiFiState::DISCONNECTED;
                
                // Notify callback
                if (state_callback_) {
                    state_callback_(WiFiState::DISCONNECTED);
                }
                break;
            }

            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
                ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                
                // Get connection info
                wifi_ap_record_t ap_info;
                esp_wifi_get_ap_info(&ap_info);
                
                mgr->conn_info_.rssi = ap_info.rssi;
                mgr->conn_info_.channel = ap_info.channel;
                
                // Convert IP to string
                sprintf(mgr->conn_info_.ip_address, 
                        IPSTR, IP2STR(&event->ip_info.ip));
                
                mgr->state_ = WiFiState::CONNECTED;
                
                ESP_LOGI(TAG, "WiFi connected! IP: %s, RSSI: %d dBm",
                         mgr->conn_info_.ip_address,
                         mgr->conn_info_.rssi);
                
                // Notify callback
                if (state_callback_) {
                    state_callback_(WiFiState::CONNECTED);
                }
                break;
            }

            default:
                break;
        }
    }
}

void WiFiManager::set_state_callback(void (*callback)(WiFiState)) {
    state_callback_ = callback;
}
```

---

## 7. Step 5: Add SoftAP Fallback

### 7.1 When to Use SoftAP

SoftAP is used as a fallback when:
- BLE provisioning fails
- User prefers web interface
- Device doesn't support BLE

### 7.2 SoftAP Configuration

```cpp
// main/softap_provisioning.cpp (new file)

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"

static const char* TAG = "softap_prov";

// HTTP handler for WiFi configuration
static esp_err_t wifi_config_handler(httpd_req_t* req) {
    char ssid[32] = {0};
    char password[64] = {0};
    
    // Parse query parameters
    // ssid=MyNetwork&password=MyPassword
    
    // TODO: Parse form data
    // For now, just return success
    const char* response = "OK";
    httpd_resp_send(req, response, strlen(response));
    
    return ESP_OK;
}

// Start SoftAP provisioning
esp_err_t start_softap_provisioning(const char* ap_password) {
    ESP_LOGI(TAG, "Starting SoftAP provisioning...");
    
    // Configure SoftAP
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ATOMS3R_CONFIG",
            .ssid_len = 13,
            .password = {0},
            .max_connection = 1,
            .authmode = WIFI_AUTH_OPEN,  // Or WPA2 if password provided
        },
    };
    
    if (ap_password && strlen(ap_password) > 0) {
        strncpy((char*)ap_config.ap.password, ap_password, 
                sizeof(ap_config.ap.password) - 1);
        ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }
    
    // Set configuration
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "SoftAP started: %s", ap_config.ap.ssid);
    ESP_LOGI(TAG, "Connect to %s and open http://192.168.4.1",
             ap_config.ap.ssid);
    
    // TODO: Start HTTP server
    
    return ESP_OK;
}
```

---

## 8. Step 6: Integrate with Existing Printer Code

### 8.1 Main Application Flow

```cpp
// main/main.cpp (complete version)

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "prov_manager.h"
#include "wifi_manager.h"

static const char* TAG = "atoms3r-main";

// Global instances
WiFiProvManager prov_manager;
WiFiManager wifi_manager;

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ATOMS3R BLE Provisioning Firmware v1.0");
    ESP_LOGI(TAG, "===========================================");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize WiFi
    ret = wifi_manager.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
    }

    // Check if already provisioned
    if (wifi_manager.has_credentials()) {
        ESP_LOGI(TAG, "WiFi credentials found, connecting...");
        
        ret = wifi_manager.connect();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to connect, will retry...");
        }
        
        // Wait for connection
        vTaskDelay(pdMS_TO_TICKS(5000));
        
        if (wifi_manager.is_connected()) {
            ESP_LOGI(TAG, "WiFi connected successfully!");
            ESP_LOGI(TAG, "IP: %s", wifi_manager.get_connection_info().ip_address);
            
            // Start normal application (MQTT, printer, etc.)
            // TODO: Call your existing app_main() or similar
            
        } else {
            ESP_LOGW(TAG, "WiFi not connected, starting provisioning...");
            start_provisioning_mode();
        }
    } else {
        ESP_LOGI(TAG, "No WiFi credentials found");
        start_provisioning_mode();
    }

    // Main loop (if not using FreeRTOS tasks)
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Monitor WiFi state
        if (wifi_manager.is_connected()) {
            // Normal operation
        } else {
            // Handle disconnection
        }
    }
}

void start_provisioning_mode() {
    ESP_LOGI(TAG, "Entering provisioning mode...");

    // Initialize provisioning manager
    esp_err_t ret = prov_manager.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init provisioning manager: %s", 
                 esp_err_to_name(ret));
        return;
    }

    // Get device name (use last 4 chars of MAC for uniqueness)
    char device_name[32] = "ATOMS3R";
    
    // Start BLE provisioning
    // Security 1 = Curve25519 + PoP
    // PoP = "123456" (should match what's on device/box)
    ret = prov_manager.start_ble_provisioning(
        device_name,
        1,              // Security 1
        "123456"        // Proof of Possession
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start BLE provisioning: %s", 
                 esp_err_to_name(ret));
        
        // Fallback to SoftAP
        ESP_LOGI(TAG, "Falling back to SoftAP provisioning...");
        // TODO: Start SoftAP
    }

    ESP_LOGI(TAG, "Provisioning mode active");
    ESP_LOGI(TAG, "Use ESP BLE Provisioning app to configure");
}
```

### 8.2 Integration Points

When integrating with existing printer code, find these sections:

```cpp
// In your existing PRINTER_FW.ino or printer_main.cpp:

// 1. Find setup() function
void setup() {
    // Add: Check for provisioning mode
    if (check_provisioning_button()) {
        enter_provisioning_mode();
        return;  // Don't start printer yet
    }
    
    // Keep existing initialization
    init_printer();
    init_mqtt();
}

// 2. Find loop() function
void loop() {
    // If in provisioning mode, don't run printer loop
    if (is_in_provisioning_mode()) {
        vTaskDelay(pdMS_TO_TICKS(100));
        return;
    }
    
    // Keep existing printer loop
    printer_loop();
}
```

### 8.3 Handling Button Press for Reset

Add a button handler to reset WiFi and enter provisioning mode:

```cpp
// Reset button handler (e.g., hold for 5 seconds)

#include "driver/gpio.h"
#include "esp_timer.h"

#define RESET_BUTTON_GPIO GPIO_NUM_39  // ATOM button

static bool button_pressed = false;
static int64_t press_start_time = 0;

void init_reset_button() {
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << RESET_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&config);
}

void check_reset_button() {
    int level = gpio_get_level(RESET_BUTTON_GPIO);
    
    if (level == 0 && !button_pressed) {
        // Button pressed
        button_pressed = true;
        press_start_time = esp_timer_get_time();
    } else if (level == 1 && button_pressed) {
        // Button released
        int64_t press_duration = esp_timer_get_time() - press_start_time;
        
        if (press_duration > 5000000) {  // 5 seconds
            ESP_LOGI(TAG, "Reset button held for 5 seconds!");
            ESP_LOGI(TAG, "Clearing WiFi credentials and entering provisioning mode");
            
            wifi_manager.clear_credentials();
            prov_manager.reset();
            
            esp_restart();
        }
        
        button_pressed = false;
    }
}
```

---

## 9. Step 7: Build and Flash

### 9.1 Build the Project

```bash
cd ~/projects/atoms3r-ble-provision

# Clean build (recommended)
idf.py fullclean

# Build
idf.py build
```

### 9.2 Expected Output

```
-- Building for esp32
-- Project is not using ESP-IDF's wifi-provisioning component.
-- Found static library: /path/to/libwifi_provisioning.a
-- Found static library: /path/to/libesp-tls.a
...
-- Build complete.
-- To flash, run this command:
/path/to/esptool.py --chip esp32 write_flash 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/boot_app0.bin 0x10000 build/atoms3r-ble-provision.bin
```

### 9.3 Flash to Device

```bash
# Connect device via USB

# Find serial port (Linux/macOS)
ls /dev/ttyUSB*   # Linux
ls /dev/tty.usbserial*  # macOS

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

### 9.4 Monitor Serial Output

```bash
idf.py monitor
```

Expected output:
```
I (0) cpu_start: App starting up...
I (123) app_main: ===========================================
I (124) app_main: ATOMS3R BLE Provisioning Firmware v1.0
I (125) app_main: ===========================================
I (126) app_main: NVS initialized
I (127) wifi_manager: WiFi initialized
I (128) app_main: No WiFi credentials found
I (129) app_main: Entering provisioning mode...
I (130) prov_manager: Initializing WiFi Provisioning Manager
I (131) prov_manager: WiFi Provisioning Manager initialized
I (132) prov_manager: Starting BLE provisioning
I (133) prov_manager: BLE provisioning started successfully
I (134) app_main: Provisioning mode active
I (135) app_main: Use ESP BLE Provisioning app to configure
```

---

## 10. Step 8: Test with iOS App

### 10.1 Download the App

1. Open **App Store** on iPhone
2. Search for **"ESP BLE Provisioning"**
3. Download and install

### 10.2 Test Procedure

```
Step 1: Power on the ATOMS3R
│
├─► The device should start advertising "ATOMS3R"
│
Step 2: Open the ESP BLE Provisioning app
│
├─► App should show "Scanning for devices..."
│
Step 3: Wait for "ATOMS3R_XXXX" to appear
│
├─► Tap on the device name to connect
│
Step 4: Enter Proof of Possession (PoP) code
│
├─► Default is "123456" (as configured in code)
│
Step 5: App shows available WiFi networks
│
├─► Select your WiFi network
│
Step 6: Enter WiFi password
│
Step 7: Tap "Connect"
│
├─► App sends credentials via BLE
│
Step 8: Wait for device to connect
│
├─► App should show "Device Connected" when successful
│
Step 9: Note the IP address shown in app
│
└─► Use this IP to send print commands via MQTT/HTTP
```

### 10.3 Troubleshooting Test Failures

| Issue | Cause | Solution |
|-------|-------|----------|
| Device not found | BLE not advertising | Check serial output for errors |
| Wrong PoP | Code mismatch | Verify code in sdkconfig |
| Connection fails | Wrong password | Re-enter password |
| Timeout | Device busy | Power cycle device |
| App crashes | iOS permissions | Allow Bluetooth access |

---

## 11. Troubleshooting Guide

### 11.1 Common Build Errors

**Error: `wifi_provisioning` component not found**

```bash
# Add the component via idf.py
idf.py add-dependency espressif/wifi_provisioning
idf.py reconfigure
```

**Error: `BTDM_CONTROLLER_BB` undefined**

Check sdkconfig has:
```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
```

**Error: `esp_tls` not found**

```bash
idf.py add-dependency espressif/esp-tls
```

### 11.2 Common Runtime Errors

**Device not advertising**

Check serial output for:
```
E (123) ble_prov: BLE init failed
```

Fix: Check BLE configuration in menuconfig

**Phone can't connect**

1. Check device is advertising (use nRF Connect app)
2. Check device name matches expected prefix
3. Check no other device with similar name

**Credentials stored but not connecting**

Check NVS:
```bash
idf.py monitor
> nvs_print wifi
```

### 11.3 Debug BLE

Enable BLE debug logs:

```bash
# In menuconfig
Component config → Bluetooth → NimBLE Host → NimBLE Log Level → Debug

# Or at runtime
esp_log_level_set("nimble", ESP_LOG_DEBUG);
esp_log_level_set("bt_atm", ESP_LOG_DEBUG);
```

---

## 12. Reference Code Repository

### 12.1 Complete Source Files

All source files for this tutorial are available in the ticket workspace:

```
ttmp/ATOMS3R-BLEPROV/
├── analysis/          # Firmware analysis document
├── design-doc/        # System design document
├── tutorial/          # This implementation guide
├── reference/         # Reference documentation
│   ├── 01-esp32-provisioning-index.md
│   ├── 07-wifi_prov_example.md
│   └── 12-wifi_provisioning_api.md
└── scripts/           # Build and test scripts
```

### 12.2 Espressif Reference Code

| Component | Location |
|-----------|----------|
| wifi_provisioning | `idf-extra-components/network_provisioning/` |
| wifi_prov example | `network_provisioning/examples/wifi_prov/` |
| iOS app source | [github.com/espressif/esp-idf-provisioning-ios](https://github.com/espressif/esp-idf-provisioning-ios) |
| Android app source | [github.com/espressif/esp-idf-provisioning-android](https://github.com/espressif/esp-idf-provisioning-android) |

### 12.3 Quick Reference: Key API Calls

```cpp
// 1. Initialize provisioning
wifi_prov_mgr_init(config);

// 2. Check if provisioned
wifi_prov_mgr_is_provisioned(&provisioned);

// 3. Start BLE provisioning
wifi_prov_mgr_start_provisioning(
    SECURITY_1,    // or SECURITY_0, SECURITY_2
    "123456",      // PoP code
    "ATOMS3R",     // device name prefix
    nullptr        // custom data
);

// 4. Handle events
esp_event_handler_register(
    WIFI_PROV_EVENT,
    ESP_EVENT_ANY_ID,
    &prov_event_handler,
    nullptr
);

// 5. Get WiFi credentials
nvs_get_str(nvs, "ssid", ssid, &len);
nvs_get_str(nvs, "password", password, &len);

// 6. Reset provisioning
wifi_prov_mgr_reset_provisioning();
```

---

## 13. Glossary

| Term | Definition |
|------|-----------|
| **BLE** | Bluetooth Low Energy - power-efficient Bluetooth variant |
| **GATT** | Generic Attribute Profile - BLE communication protocol |
| **UUID** | Universally Unique Identifier - 128-bit identifier for BLE services |
| **Protocomm** | Espressif's protocol communication abstraction layer |
| **PoP** | Proof of Possession - security code to authorize provisioning |
| **SoftAP** | Software Access Point - device creates WiFi network |
| **NVS** | Non-Volatile Storage - persistent storage on ESP32 |
| **MTU** | Maximum Transmission Unit - max BLE packet size |

---

*Document: ATOMS3R BLE-Provisioning Firmware Implementation Guide*  
*Ticket: ATOMS3R-BLEPROV*  
*Created: 2026-04-22*  
*Version: 1.0*  
*Author: For new firmware developers and interns*