---
Title: ESP-Zigbee-Lib Low-Level Stack Analysis
Ticket: 0034-ANALYZE-ESP-ZIGBEE-LIB
Status: active
Topics:
    - zigbee
    - esp-zigbee-lib
    - stack
    - low-level
    - uml
    - esp32-h2
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h
      Note: Core API header analyzed
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/platform/esp_zigbee_platform.h
      Note: Platform configuration API analyzed
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/aps/esp_zigbee_aps.h
      Note: APS layer API analyzed
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_command.h
      Note: ZDO command API analyzed
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_common.h
      Note: ZDO signal types analyzed
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: H2 firmware integration with esp-zigbee-lib analyzed
ExternalSources: []
Summary: Comprehensive low-level analysis of esp-zigbee-lib stack, API architecture, signal/callback system, and integration with H2 firmware, including UML timing diagrams
LastUpdated: 2026-01-06T13:52:20.000000000-05:00
WhatFor: Understanding the low-level Zigbee stack implementation, API structure, event handling, and how the H2 firmware uses it
WhenToUse: When developing Zigbee applications, debugging stack behavior, or understanding the interaction between NCP firmware and the Zigbee stack
---

# ESP-Zigbee-Lib Low-Level Stack Analysis

## Executive Summary

This document provides a comprehensive analysis of **esp-zigbee-lib**, the low-level Zigbee protocol stack library used by Espressif's ESP32-H2 NCP firmware. The library provides a complete Zigbee 3.0 stack implementation, including MAC, NWK, APS, ZDO, and ZCL layers, with an event-driven callback architecture.

**Key Findings:**

- **Binary Library Architecture**: The core stack (`libzboss_port.*.a`) is provided as precompiled binaries, with a C API wrapper layer (`libesp_zb_api.*.a`) exposing the functionality
- **Event-Driven Design**: Uses a signal/callback system (`esp_zb_app_signal_handler`) for asynchronous event notification
- **Main Loop Pattern**: Requires continuous execution of `esp_zb_stack_main_loop()` in a dedicated task
- **Platform Abstraction**: Platform-specific configuration via `esp_zb_platform_config()` for radio and host connection modes
- **Layered API Structure**: APIs organized by protocol layer (Core, APS, ZDO, ZCL, BDB, Platform)

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [API Structure and Organization](#api-structure-and-organization)
3. [Initialization and Startup Sequence](#initialization-and-startup-sequence)
4. [Signal and Callback System](#signal-and-callback-system)
5. [Integration with H2 Firmware](#integration-with-h2-firmware)
6. [UML Timing Diagrams](#uml-timing-diagrams)
7. [Key Data Structures](#key-data-structures)
8. [API Reference](#api-reference)

---

## Architecture Overview

### Component Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│              (NCP Firmware / Host Application)              │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        │ C API Calls
                        │
┌───────────────────────▼─────────────────────────────────────┐
│              ESP-Zigbee-Lib API Layer                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐     │
│  │  Core    │ │   APS     │ │   ZDO    │ │   ZCL    │     │
│  │  API     │ │   API     │ │   API    │ │   API    │     │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘     │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                  │
│  │  BDB     │ │ Platform │ │ Security │                  │
│  │  API     │ │   API     │ │   API    │                  │
│  └──────────┘ └──────────┘ └──────────┘                  │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        │ Internal Calls
                        │
┌───────────────────────▼─────────────────────────────────────┐
│            ZBOSS Stack (Binary Library)                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Application Layer (ZCL)                             │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  Application Support Layer (APS)                     │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  Network Layer (NWK)                                │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  MAC Layer (IEEE 802.15.4)                           │   │
│  └──────────────────────────────────────────────────────┘   │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        │ HAL Calls
                        │
┌───────────────────────▼─────────────────────────────────────┐
│              Platform HAL (ESP-IDF)                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                  │
│  │  Radio   │ │   NVS     │ │  Timer   │                  │
│  │  Driver │ │  Storage  │ │  Sched   │                  │
│  └──────────┘ └──────────┘ └──────────┘                  │
└─────────────────────────────────────────────────────────────┘
```

### Library Organization

The esp-zigbee-lib is organized into several components:

1. **Core Library** (`libesp_zb_api.*.a`): Main API wrapper
2. **ZBOSS Port** (`libzboss_port.*.a`): Platform-specific port layer
3. **Header Files** (`include/`): Public API definitions organized by layer:
   - `esp_zigbee_core.h` - Core initialization and main loop
   - `aps/esp_zigbee_aps.h` - Application Support Layer
   - `zdo/esp_zigbee_zdo_*.h` - Zigbee Device Object
   - `zcl/esp_zigbee_zcl_*.h` - Zigbee Cluster Library
   - `bdb/esp_zigbee_bdb_*.h` - Base Device Behavior
   - `platform/esp_zigbee_platform.h` - Platform configuration

### Key Design Principles

1. **Event-Driven Architecture**: All stack events are delivered via `esp_zb_app_signal_handler()`
2. **Non-Blocking API**: Most API calls return immediately; results delivered via callbacks
3. **Thread Safety**: Requires `esp_zb_lock_acquire()`/`esp_zb_lock_release()` for multi-threaded access
4. **Main Loop Requirement**: `esp_zb_stack_main_loop()` must run continuously in a dedicated task
5. **Platform Abstraction**: Radio and host connection modes configurable at initialization

---

## API Structure and Organization

### Core API (`esp_zigbee_core.h`)

**Initialization and Control:**

```c
// Stack initialization
void esp_zb_init(esp_zb_cfg_t *nwk_cfg);
esp_err_t esp_zb_start(bool autostart);
void esp_zb_stack_main_loop(void);

// Configuration (must be called before esp_zb_init)
esp_err_t esp_zb_overall_network_size_set(uint16_t size);
esp_err_t esp_zb_io_buffer_size_set(uint16_t size);
esp_err_t esp_zb_scheduler_queue_size_set(uint16_t size);
esp_err_t esp_zb_set_primary_network_channel_set(uint32_t channel_mask);
```

**Signal Handler (Required):**

```c
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_s);
void *esp_zb_app_signal_get_params(uint32_t *signal_p);
```

**Scheduler:**

```c
void esp_zb_scheduler_alarm(esp_zb_callback_t cb, uint8_t param, uint32_t time);
void esp_zb_scheduler_alarm_cancel(esp_zb_callback_t cb, uint8_t param);
```

**Thread Safety:**

```c
bool esp_zb_lock_acquire(TickType_t block_ticks);
void esp_zb_lock_release(void);
```

### APS API (`aps/esp_zigbee_aps.h`)

**Data Transmission:**

```c
esp_err_t esp_zb_aps_data_request(esp_zb_apsde_data_req_t *req);
```

**Callbacks:**

```c
void esp_zb_aps_data_indication_handler_register(esp_zb_apsde_data_indication_callback_t cb);
void esp_zb_aps_data_confirm_handler_register(esp_zb_apsde_data_confirm_callback_t cb);
```

**Data Structures:**

```c
typedef struct esp_zb_apsde_data_req_s {
    uint8_t dst_addr_mode;
    esp_zb_addr_u dst_addr;
    uint8_t dst_endpoint;
    uint16_t profile_id;
    uint16_t cluster_id;
    uint8_t src_endpoint;
    uint32_t asdu_length;
    uint8_t *asdu;
    uint8_t tx_options;
    // ... additional fields
} esp_zb_apsde_data_req_t;

typedef struct esp_zb_apsde_data_ind_s {
    uint8_t status;
    uint8_t dst_addr_mode;
    uint16_t dst_short_addr;
    uint8_t dst_endpoint;
    uint16_t src_short_addr;
    uint8_t src_endpoint;
    uint16_t profile_id;
    uint16_t cluster_id;
    uint32_t asdu_length;
    uint8_t *asdu;
    uint8_t security_status;
    int lqi;
} esp_zb_apsde_data_ind_t;
```

### ZDO API (`zdo/esp_zigbee_zdo_command.h`)

**Network Discovery:**

```c
void esp_zb_zdo_active_scan_request(uint32_t channel_mask, uint8_t scan_duration, 
                                     esp_zb_zdo_scan_complete_callback_t user_cb);
void esp_zb_zdo_energy_detect_request(uint32_t channel_mask, uint8_t duration,
                                      esp_zb_zdo_energy_detect_callback_t cb);
```

**Device Management:**

```c
void esp_zb_zdo_ieee_addr_req(esp_zb_zdo_ieee_addr_req_param_t *cmd_req,
                               esp_zb_zdo_ieee_addr_callback_t user_cb, void *user_ctx);
void esp_zb_zdo_nwk_addr_req(esp_zb_zdo_nwk_addr_req_param_t *cmd_req,
                              esp_zb_zdo_nwk_addr_callback_t user_cb, void *user_ctx);
void esp_zb_zdo_node_desc_req(esp_zb_zdo_node_desc_req_param_t *cmd_req,
                               esp_zb_zdo_node_desc_callback_t user_cb, void *user_ctx);
void esp_zb_zdo_simple_desc_req(esp_zb_zdo_simple_desc_req_param_t *cmd_req,
                                esp_zb_zdo_simple_desc_callback_t user_cb, void *user_ctx);
```

**Binding:**

```c
void esp_zb_zdo_device_bind_req(esp_zb_zdo_bind_req_param_t *cmd_req,
                                 esp_zb_zdo_bind_callback_t user_cb, void *user_ctx);
void esp_zb_zdo_binding_table_req(esp_zb_zdo_mgmt_bind_param_t *cmd_req,
                                  esp_zb_zdo_binding_table_callback_t user_cb, void *user_ctx);
```

**Network Control:**

```c
void esp_zb_zdo_permit_joining_req(esp_zb_zdo_permit_joining_req_param_t *cmd_req,
                                    esp_zb_zdo_permit_join_callback_t user_cb, void *user_ctx);
void esp_zb_zdo_device_leave_req(esp_zb_zdo_mgmt_leave_req_param_t *cmd_req,
                                  esp_zb_zdo_leave_callback_t user_cb, void *user_ctx);
```

### BDB API (`bdb/esp_zigbee_bdb_commissioning.h`)

**Commissioning:**

```c
esp_err_t esp_zb_bdb_start_top_level_commissioning(uint8_t mode_mask);
esp_err_t esp_zb_bdb_open_network(uint8_t permit_duration);
esp_err_t esp_zb_bdb_close_network(void);
```

**Commissioning Modes:**

```c
typedef enum {
    ESP_ZB_BDB_MODE_INITIALIZATION          = 0x00,
    ESP_ZB_BDB_MODE_TOUCHLINK_COMMISSIONING  = 0x01,
    ESP_ZB_BDB_MODE_NETWORK_STEERING        = 0x02,
    ESP_ZB_BDB_MODE_NETWORK_FORMATION       = 0x04,
    ESP_ZB_BDB_MODE_FINDING_N_BINDING       = 0x08,
    ESP_ZB_BDB_MODE_TOUCHLINK_TARGET        = 0x40,
} esp_zb_bdb_commissioning_mode_mask_t;
```

### Platform API (`platform/esp_zigbee_platform.h`)

**Configuration:**

```c
esp_err_t esp_zb_platform_config(esp_zb_platform_config_t *config);
```

**Configuration Structure:**

```c
typedef struct {
    esp_zb_radio_config_t    radio_config;   // Radio mode (native/RCP)
    esp_zb_host_config_t     host_config;    // Host connection mode
} esp_zb_platform_config_t;

typedef struct {
    esp_zb_radio_mode_t      radio_mode;         // ZB_RADIO_MODE_NATIVE or ZB_RADIO_MODE_UART_RCP
    esp_zb_uart_config_t      radio_uart_config; // UART config for RCP mode
} esp_zb_radio_config_t;
```

---

## Initialization and Startup Sequence

### Standard Initialization Flow

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Platform Configuration                                   │
│    esp_zb_platform_config(&platform_config)                 │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ 2. Optional Pre-Init Configuration                          │
│    esp_zb_overall_network_size_set(size)                    │
│    esp_zb_io_buffer_size_set(size)                          │
│    esp_zb_set_primary_network_channel_set(channel_mask)     │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ 3. Stack Initialization                                     │
│    esp_zb_init(&nwk_cfg)                                    │
│    - Sets default IB parameters                             │
│    - Does NOT start stack                                   │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ 4. Start Stack                                               │
│    esp_zb_start(autostart)                                  │
│    - autostart=true: Loads NVRAM, proceeds with startup     │
│    - autostart=false: Initializes framework only            │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ 5. Create Main Loop Task                                     │
│    xTaskCreate(esp_zb_stack_main_loop_task, ...)            │
│    - Must run continuously                                  │
│    - Processes stack events                                 │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ 6. Signal Handler Receives Events                            │
│    esp_zb_app_signal_handler()                              │
│    - ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP                         │
│    - ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START                  │
│    - ESP_ZB_BDB_SIGNAL_FORMATION                            │
│    - etc.                                                    │
└─────────────────────────────────────────────────────────────┘
```

### Code Example: Basic Initialization

```c
void zigbee_init_task(void *pvParameters)
{
    // 1. Platform configuration
    esp_zb_platform_config_t platform_config = {
        .radio_config = {
            .radio_mode = ZB_RADIO_MODE_NATIVE,
        },
        .host_config = {
            .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
        },
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform_config));

    // 2. Optional: Set channel mask
    esp_zb_set_primary_network_channel_set(0x07FFF800); // Channels 11-26

    // 3. Stack initialization
    esp_zb_cfg_t nwk_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,
        .install_code_policy = false,
        .nwk_cfg = {
            .zczr_cfg = {
                .max_children = 20,
            },
        },
    };
    esp_zb_init(&nwk_cfg);

    // 4. Start stack (autostart mode)
    ESP_ERROR_CHECK(esp_zb_start(true));

    // 5. Main loop (runs forever)
    esp_zb_stack_main_loop();
}
```

---

## Signal and Callback System

### Signal Types Overview

The stack uses a unified signal system (`esp_zb_app_signal_type_t`) to notify the application of all events:

**ZDO Signals (Zigbee Device Object):**
- `ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP` - Stack framework ready
- `ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE` - Device joined/rejoined
- `ESP_ZB_ZDO_SIGNAL_LEAVE` - Device left network
- `ESP_ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED` - Device authorized
- `ESP_ZB_ZDO_SIGNAL_DEVICE_UPDATE` - Device information updated

**BDB Signals (Base Device Behavior):**
- `ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START` - Factory new device initialized
- `ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT` - Device rebooted/rejoined
- `ESP_ZB_BDB_SIGNAL_FORMATION` - Network formation complete
- `ESP_ZB_BDB_SIGNAL_STEERING` - Network steering complete
- `ESP_ZB_BDB_SIGNAL_TOUCHLINK` - Touchlink commissioning result
- `ESP_ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED` - F&B target finished
- `ESP_ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED` - F&B initiator finished

**NWK Signals (Network Layer):**
- `ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS` - Permit join status changed
- `ESP_ZB_NWK_SIGNAL_DEVICE_ASSOCIATED` - Device associated
- `ESP_ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED` - PAN ID conflict detected

**Common Signals:**
- `ESP_ZB_COMMON_SIGNAL_CAN_SLEEP` - Device can enter sleep mode

### Signal Handler Structure

```c
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;

    switch (sig_type) {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            // Stack framework ready, can start commissioning
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
            break;

        case ESP_ZB_BDB_SIGNAL_FORMATION:
            if (err_status == ESP_OK) {
                // Network formed successfully
                uint16_t pan_id = esp_zb_get_pan_id();
                uint8_t channel = esp_zb_get_current_channel();
                // ... handle formation success
            }
            break;

        case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
            // Device joined/rejoined network
            esp_zb_zdo_signal_device_annce_params_t *params =
                (esp_zb_zdo_signal_device_annce_params_t *)esp_zb_app_signal_get_params(p_sg_p);
            if (params) {
                uint16_t short_addr = params->device_short_addr;
                esp_zb_ieee_addr_t ieee_addr = params->ieee_addr;
                // ... handle device announcement
            }
            break;
        }

        case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
            if (err_status == ESP_OK) {
                uint8_t *duration = esp_zb_app_signal_get_params(p_sg_p);
                // ... handle permit join status change
            }
            break;

        // ... handle other signals
    }
}
```

### Callback Registration Pattern

**APS Data Indication:**

```c
bool my_aps_data_indication_handler(esp_zb_apsde_data_ind_t ind)
{
    // Process incoming APS data
    // Return true if handled, false to let stack process
    return false;
}

// Register callback
esp_zb_aps_data_indication_handler_register(my_aps_data_indication_handler);
```

**APS Data Confirm:**

```c
void my_aps_data_confirm_handler(esp_zb_apsde_data_confirm_t confirm)
{
    // Process transmission confirmation
    if (confirm.status == 0) {
        // Success
    } else {
        // Failure
    }
}

// Register callback
esp_zb_aps_data_confirm_handler_register(my_aps_data_confirm_handler);
```

**ZDO Command Callbacks:**

```c
void my_ieee_addr_callback(esp_zb_zdp_status_t zdo_status,
                          esp_zb_zdo_ieee_addr_rsp_t *resp,
                          void *user_ctx)
{
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
        // Process IEEE address response
    }
}

// Use callback in request
esp_zb_zdo_ieee_addr_req_param_t req = {
    .dst_nwk_addr = 0x0000,
    .addr_of_interest = 0x0000,
    .request_type = 0x00,
    .start_index = 0,
};
esp_zb_zdo_ieee_addr_req(&req, my_ieee_addr_callback, NULL);
```

---

## Integration with H2 Firmware

### How NCP Firmware Uses esp-zigbee-lib

The H2 NCP firmware (`esp_ncp_zb.c`) acts as a bridge between the host protocol (ZNSP/EGSP) and esp-zigbee-lib:

```
┌─────────────────────────────────────────────────────────────┐
│                    Host (ESP32-S3)                          │
│              Sends ZNSP Commands via UART                   │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        │ UART + SLIP + ZNSP Frames
                        │
┌───────────────────────▼─────────────────────────────────────┐
│              NCP Firmware (ESP32-H2)                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  esp_ncp_frame.c                                      │  │
│  │  - SLIP decode                                        │  │
│  │  - Frame validation                                   │  │
│  └───────────────────────┬──────────────────────────────┘  │
│                          │                                   │
│  ┌───────────────────────▼──────────────────────────────┐  │
│  │  esp_ncp_zb.c                                         │  │
│  │  - Command dispatch table                            │  │
│  │  - Maps ZNSP commands → esp-zigbee-lib APIs          │  │
│  │  - Maps esp-zigbee-lib signals → ZNSP notifications  │  │
│  └───────────────────────┬──────────────────────────────┘  │
│                          │                                   │
│  ┌───────────────────────▼──────────────────────────────┐  │
│  │  esp-zigbee-lib                                      │  │
│  │  - Zigbee stack execution                            │  │
│  │  - Signal generation                                 │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Command Mapping

**Network Commands:**

```c
// ZNSP_NETWORK_INIT → esp_zb_init()
static esp_err_t esp_ncp_zb_network_init_fn(const uint8_t *input, uint16_t inlen,
                                            uint8_t **output, uint16_t *outlen)
{
    esp_zb_cfg_t nwk_cfg;
    memcpy(&nwk_cfg, input, sizeof(esp_zb_cfg_t));
    esp_zb_init(&nwk_cfg);
    // ... return status
}

// ZNSP_NETWORK_START → esp_zb_start()
static esp_err_t esp_ncp_zb_network_start_fn(const uint8_t *input, uint16_t inlen,
                                             uint8_t **output, uint16_t *outlen)
{
    bool autostart = *(bool *)input;
    esp_err_t ret = esp_zb_start(autostart);
    // ... return status
}

// ZNSP_NETWORK_FORMNETWORK → esp_zb_bdb_start_top_level_commissioning()
static esp_err_t esp_ncp_zb_form_network_fn(const uint8_t *input, uint16_t inlen,
                                            uint8_t **output, uint16_t *outlen)
{
    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
    // ... return status
}
```

**Signal to Notification Mapping:**

```c
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    esp_zb_app_signal_type_t sig_type = *signal_struct->p_app_signal;
    
    switch (sig_type) {
        case ESP_ZB_BDB_SIGNAL_FORMATION:
            // Convert to ZNSP notification
            esp_ncp_header_t ncp_header = {
                .id = ESP_NCP_NETWORK_FORMNETWORK,
                .sn = esp_random() % 0xFF,
            };
            esp_ncp_zb_formnetwork_parameters_t params = {
                .panId = esp_zb_get_pan_id(),
                .radioChannel = esp_zb_get_current_channel(),
            };
            esp_zb_get_extended_pan_id(params.extendedPanId);
            esp_ncp_noti_input(&ncp_header, &params, sizeof(params));
            break;

        case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            // Convert to ZNSP notification
            esp_zb_zdo_signal_device_annce_params_t *params =
                (esp_zb_zdo_signal_device_annce_params_t *)
                esp_zb_app_signal_get_params(signal_struct->p_app_signal);
            ncp_header.id = ESP_NCP_NETWORK_JOINNETWORK;
            esp_ncp_noti_input(&ncp_header, params, sizeof(*params));
            break;
    }
}
```

### Task Structure

The NCP firmware runs esp-zigbee-lib in a dedicated task:

```c
static void esp_ncp_zb_task(void *pvParameters)
{
    // Register action handler (if needed)
    esp_zb_core_action_handler_register(esp_ncp_zb_action_handler);
    
    // Run stack main loop (blocks forever)
    esp_zb_stack_main_loop();
    
    vTaskDelete(NULL);
}
```

This task is created during NCP initialization:

```c
esp_err_t esp_ncp_start(void)
{
    // ... create event queue ...
    
    // Create Zigbee stack task
    xTaskCreate(esp_ncp_zb_task, "ncp_zb", 6144, NULL, 8, NULL);
    
    // ... start bus task ...
    
    return ESP_OK;
}
```

---

## UML Timing Diagrams

### Diagram 1: Stack Initialization Sequence

```
┌──────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│Application│    │esp-zigbee-lib│    │  ZBOSS Stack │    │  Platform    │
└─────┬─────┘    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
      │                  │                    │                   │
      │ esp_zb_platform_ │                    │                   │
      │ config()         │                    │                   │
      ├─────────────────>│                    │                   │
      │                  │                    │                   │
      │                  │ Initialize Radio   │                   │
      │                  ├───────────────────>│                   │
      │                  │                    │                   │
      │                  │                    │ Radio Init OK     │
      │                  │<───────────────────┤                   │
      │                  │                    │                   │
      │                  │ ESP_OK             │                   │
      │<─────────────────┤                    │                   │
      │                  │                    │                   │
      │ esp_zb_init()    │                    │                   │
      ├─────────────────>│                    │                   │
      │                  │ Initialize Stack   │                   │
      │                  ├───────────────────>│                   │
      │                  │                    │                   │
      │                  │                    │ Stack Init OK     │
      │                  │<───────────────────┤                   │
      │                  │                    │                   │
      │                  │ Return             │                   │
      │<─────────────────┤                    │                   │
      │                  │                    │                   │
      │ esp_zb_start()   │                    │                   │
      ├─────────────────>│                    │                   │
      │                  │ Start Stack        │                   │
      │                  ├───────────────────>│                   │
      │                  │                    │                   │
      │                  │                    │ Load NVRAM        │
      │                  │                    ├───────────────────>│
      │                  │                    │                   │
      │                  │                    │ NVRAM Loaded      │
      │                  │                    │<───────────────────┤
      │                  │                    │                   │
      │                  │ ESP_OK             │                   │
      │<─────────────────┤                    │                   │
      │                  │                    │                   │
      │                  │                    │ Signal:           │
      │                  │                    │ SKIP_STARTUP      │
      │                  │<───────────────────┤                   │
      │                  │                    │                   │
      │                  │ esp_zb_app_signal_ │                   │
      │                  │ handler()          │                   │
      ├──────────────────<───────────────────┤                   │
      │                  │                    │                   │
```

### Diagram 2: Network Formation Sequence

```
┌──────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│Application│    │esp-zigbee-lib│    │  ZBOSS Stack │    │   Network    │
└─────┬─────┘    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
      │                  │                    │                   │
      │ esp_zb_bdb_start │                    │                   │
      │ _top_level_      │                    │                   │
      │ commissioning(   │                    │                   │
      │ FORMATION)       │                    │                   │
      ├─────────────────>│                    │                   │
      │                  │                    │                   │
      │                  │ Start Formation    │                   │
      │                  ├───────────────────>│                   │
      │                  │                    │                   │
      │                  │                    │ Energy Scan       │
      │                  │                    ├───────────────────>│
      │                  │                    │                   │
      │                  │                    │ Scan Results      │
      │                  │                    │<───────────────────┤
      │                  │                    │                   │
      │                  │                    │ Select Channel    │
      │                  │                    │                   │
      │                  │                    │ Form Network      │
      │                  │                    ├───────────────────>│
      │                  │                    │                   │
      │                  │                    │ Network Formed    │
      │                  │                    │<───────────────────┤
      │                  │                    │                   │
      │                  │                    │ Signal:           │
      │                  │                    │ FORMATION         │
      │                  │<───────────────────┤                   │
      │                  │                    │                   │
      │                  │ esp_zb_app_signal_ │                   │
      │                  │ handler()          │                   │
      ├──────────────────<───────────────────┤                   │
      │                  │                    │                   │
      │                  │                    │ Signal:           │
      │                  │                    │ STEERING          │
      │                  │<───────────────────┤                   │
      │                  │                    │                   │
      │                  │ esp_zb_app_signal_ │                   │
      │                  │ handler()          │                   │
      ├──────────────────<───────────────────┤                   │
      │                  │                    │                   │
```

### Diagram 3: Device Join Sequence

```
┌──────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│Coordinator│    │esp-zigbee-lib│    │  ZBOSS Stack │    │  New Device  │
└─────┬─────┘    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
      │                  │                    │                   │
      │ esp_zb_bdb_open_ │                    │                   │
      │ network(60)       │                    │                   │
      ├─────────────────>│                    │                   │
      │                  │                    │                   │
      │                  │ Permit Join        │                   │
      │                  ├───────────────────>│                   │
      │                  │                    │                   │
      │                  │                    │ Broadcast         │
      │                  │                    │ Permit Join       │
      │                  │                    ├───────────────────>│
      │                  │                    │                   │
      │                  │                    │                   │ Join Request
      │                  │                    │<───────────────────┤
      │                  │                    │                   │
      │                  │                    │ Process Join      │
      │                  │                    │                   │
      │                  │                    │ Join Response     │
      │                  │                    ├───────────────────>│
      │                  │                    │                   │
      │                  │                    │ Device Joined     │
      │                  │                    │                   │
      │                  │                    │ Signal:           │
      │                  │                    │ DEVICE_ANNCE     │
      │                  │<───────────────────┤                   │
      │                  │                    │                   │
      │                  │ esp_zb_app_signal_ │                   │
      │                  │ handler()          │                   │
      ├──────────────────<───────────────────┤                   │
      │                  │                    │                   │
```

### Diagram 4: APS Data Transmission Sequence

```
┌──────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│Application│    │esp-zigbee-lib│    │  ZBOSS Stack │    │ Remote Device│
└─────┬─────┘    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
      │                  │                    │                   │
      │ esp_zb_aps_data_ │                    │                   │
      │ request()         │                    │                   │
      ├─────────────────>│                    │                   │
      │                  │                    │                   │
      │                  │ APSDE-DATA.request │                   │
      │                  ├───────────────────>│                   │
      │                  │                    │                   │
      │                  │                    │ Encrypt & Route  │
      │                  │                    │                   │
      │                  │                    │ Transmit Frame   │
      │                  │                    ├───────────────────>│
      │                  │                    │                   │
      │                  │                    │                   │ ACK
      │                  │                    │<───────────────────┤
      │                  │                    │                   │
      │                  │                    │ APSDE-DATA.confirm│
      │                  │<───────────────────┤                   │
      │                  │                    │                   │
      │                  │ esp_zb_aps_data_   │                   │
      │                  │ confirm_handler()  │                   │
      ├──────────────────<───────────────────┤                   │
      │                  │                    │                   │
      │                  │                    │                   │ Process
      │                  │                    │                   │ Frame
      │                  │                    │                   │
      │                  │                    │ APSDE-DATA.indication│
      │                  │                    │<───────────────────┤
      │                  │                    │                   │
      │                  │ esp_zb_aps_data_   │                   │
      │                  │ indication_handler │                   │
      │                  │ ()                 │                   │
      ├──────────────────<───────────────────┤                   │
      │                  │                    │                   │
```

---

## Key Data Structures

### Stack Configuration

```c
typedef struct esp_zb_cfg_s {
    esp_zb_nwk_device_type_t esp_zb_role;  // Coordinator/Router/End Device
    bool install_code_policy;               // Install code security policy
    union {
        esp_zb_zczr_cfg_t zczr_cfg;        // Coordinator/Router config
        esp_zb_zed_cfg_t zed_cfg;          // End Device config
    } nwk_cfg;
} esp_zb_cfg_t;

typedef struct {
    uint8_t max_children;                   // Max number of children
} esp_zb_zczr_cfg_t;

typedef struct {
    uint8_t ed_timeout;                    // End device timeout
    uint32_t keep_alive;                   // Keep alive timeout (ms)
} esp_zb_zed_cfg_t;
```

### Signal Structure

```c
typedef struct esp_zb_app_signal_s {
    uint32_t *p_app_signal;                // Pointer to signal type
    esp_err_t esp_err_status;              // Error status
} esp_zb_app_signal_t;
```

### APS Data Request

```c
typedef struct esp_zb_apsde_data_req_s {
    uint8_t dst_addr_mode;                 // Address mode
    esp_zb_addr_u dst_addr;                // Destination address
    uint8_t dst_endpoint;                  // Destination endpoint
    uint16_t profile_id;                   // Profile ID
    uint16_t cluster_id;                   // Cluster ID
    uint8_t src_endpoint;                  // Source endpoint
    uint32_t asdu_length;                 // ASDU length
    uint8_t *asdu;                        // ASDU data
    uint8_t tx_options;                  // Transmission options
    bool use_alias;                       // Use alias address
    uint16_t alias_src_addr;             // Alias source address
    int alias_seq_num;                   // Alias sequence number
    uint8_t radius;                      // Transmission radius
} esp_zb_apsde_data_req_t;
```

### APS Data Indication

```c
typedef struct esp_zb_apsde_data_ind_s {
    uint8_t status;                       // Processing status
    uint8_t dst_addr_mode;                // Destination address mode
    uint16_t dst_short_addr;              // Destination short address
    uint8_t dst_endpoint;                 // Destination endpoint
    uint8_t src_addr_mode;                // Source address mode
    uint16_t src_short_addr;              // Source short address
    uint8_t src_endpoint;                 // Source endpoint
    uint16_t profile_id;                  // Profile ID
    uint16_t cluster_id;                  // Cluster ID
    uint32_t asdu_length;                // ASDU length
    uint8_t *asdu;                       // ASDU data
    uint8_t security_status;             // Security status
    int lqi;                             // Link quality indicator
    int rx_time;                         // Receive time
} esp_zb_apsde_data_ind_t;
```

---

## API Reference

### Core Functions

#### `esp_zb_init()`

Initialize the Zigbee stack.

**Signature:**
```c
void esp_zb_init(esp_zb_cfg_t *nwk_cfg);
```

**Parameters:**
- `nwk_cfg`: Pointer to network configuration structure

**Notes:**
- Must be called before `esp_zb_start()`
- Sets default Information Base (IB) parameters
- Does not start the stack; call `esp_zb_start()` after

#### `esp_zb_start()`

Start the Zigbee stack.

**Signature:**
```c
esp_err_t esp_zb_start(bool autostart);
```

**Parameters:**
- `autostart`: 
  - `true`: Loads NVRAM and proceeds with startup (formation/rejoin/join)
  - `false`: Initializes framework only (scheduler, buffers), no network operations

**Returns:**
- `ESP_OK`: Success
- Other: Error code

**Notes:**
- Must be called after `esp_zb_init()`
- Does not loop; caller must run `esp_zb_stack_main_loop()` after

#### `esp_zb_stack_main_loop()`

Main loop for stack execution.

**Signature:**
```c
void esp_zb_stack_main_loop(void);
```

**Notes:**
- Blocks forever; must run in dedicated task
- Processes stack events and callbacks
- Must be called after `esp_zb_init()` and `esp_zb_start()`

#### `esp_zb_app_signal_handler()`

Application signal handler (required).

**Signature:**
```c
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_s);
```

**Parameters:**
- `signal_s`: Pointer to signal structure

**Notes:**
- Must be implemented by application
- Called by stack for all events
- Use `esp_zb_app_signal_get_params()` to extract signal-specific data

### APS Functions

#### `esp_zb_aps_data_request()`

Send APS data frame.

**Signature:**
```c
esp_err_t esp_zb_aps_data_request(esp_zb_apsde_data_req_t *req);
```

**Parameters:**
- `req`: Pointer to APS data request structure

**Returns:**
- `ESP_OK`: Request queued successfully
- `ESP_ERR_INVALID_ARG`: Invalid arguments
- `ESP_ERR_NO_MEM`: Insufficient memory
- `ESP_FAIL`: Other failure

**Notes:**
- Result delivered via `esp_zb_aps_data_confirm_handler()`
- Incoming data delivered via `esp_zb_aps_data_indication_handler()`

### BDB Functions

#### `esp_zb_bdb_start_top_level_commissioning()`

Start BDB commissioning procedure.

**Signature:**
```c
esp_err_t esp_zb_bdb_start_top_level_commissioning(uint8_t mode_mask);
```

**Parameters:**
- `mode_mask`: Commissioning mode mask (see `esp_zb_bdb_commissioning_mode_mask_t`)

**Returns:**
- `ESP_OK`: Success

**Notes:**
- Results delivered via `esp_zb_app_signal_handler()`
- Common modes:
  - `ESP_ZB_BDB_MODE_NETWORK_FORMATION`: Form new network
  - `ESP_ZB_BDB_MODE_NETWORK_STEERING`: Join existing network
  - `ESP_ZB_BDB_MODE_INITIALIZATION`: Initialize BDB

---

## Conclusion

The esp-zigbee-lib provides a comprehensive, event-driven Zigbee 3.0 stack implementation with a well-structured API organized by protocol layer. The library's design emphasizes:

1. **Asynchronous Operation**: All operations are non-blocking with results delivered via callbacks
2. **Event-Driven Architecture**: Unified signal system for all stack events
3. **Platform Abstraction**: Configurable radio and host connection modes
4. **Thread Safety**: Lock mechanisms for multi-threaded access

The H2 NCP firmware integrates seamlessly with esp-zigbee-lib, translating between the host protocol (ZNSP/EGSP) and the stack's API, demonstrating the library's flexibility for different application architectures.

---

## References

- ESP-Zigbee-Lib Header Files: `managed_components/espressif__esp-zigbee-lib/include/`
- NCP Firmware Integration: `components/esp-zigbee-ncp/src/esp_ncp_zb.c`
- Zigbee 3.0 Specification: Zigbee Alliance
- IEEE 802.15.4 Specification: IEEE Standards Association

