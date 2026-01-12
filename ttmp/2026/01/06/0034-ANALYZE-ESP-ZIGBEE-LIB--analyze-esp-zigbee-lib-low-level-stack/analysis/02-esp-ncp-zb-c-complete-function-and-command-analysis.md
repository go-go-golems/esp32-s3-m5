---
Title: Complete Function and Command Analysis of esp_ncp_zb.c
Ticket: 0034-ANALYZE-ESP-ZIGBEE-LIB
Status: active
Topics:
    - zigbee
    - ncp
    - esp-zigbee-lib
    - command-handlers
    - function-analysis
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Complete source file analysis
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_zb.h
      Note: Command ID definitions
ExternalSources: []
Summary: Comprehensive analysis of every function and ESP_NCP_* command in esp_ncp_zb.c, including low-level API calls and Zigbee bus impact
LastUpdated: 2026-01-06T14:30:00.000000000-05:00
WhatFor: Understanding the complete implementation of NCP command handlers and their interaction with the Zigbee stack
WhenToUse: When implementing application host-NCP communication, debugging command handlers, or understanding Zigbee stack integration
---

# Complete Function and Command Analysis of esp_ncp_zb.c

## Table of Contents

1. [Glossary](#glossary)
2. [Overview](#overview)
3. [File Structure and Architecture](#file-structure-and-architecture)
4. [Helper Functions and Callbacks](#helper-functions-and-callbacks)
5. [Network Commands (0x0000-0x002F)](#network-commands-0x0000-0x002f)
6. [ZCL Commands (0x0100-0x0108)](#zcl-commands-0x0100-0x0108)
7. [ZDO Commands (0x0200-0x0202)](#zdo-commands-0x0200-0x0202)
8. [APS Commands (0x0300-0x0302)](#aps-commands-0x0300-0x0302)
9. [Command Dispatch System](#command-dispatch-system)
10. [State Management](#state-management)
11. [Zigbee Bus Impact Analysis](#zigbee-bus-impact-analysis)

---

## Glossary

This glossary defines key terms used throughout this document to avoid confusion, especially around the dual meaning of "host" in different contexts.

### Architecture Terms

**Host** (in NCP context, also called "application host")
: The main application processor that controls the NCP via serial protocol (UART/SPI). In this project, the **application host** is typically an ESP32-S3 (e.g., Cardputer) running application code. The application host sends commands to the NCP and receives responses/notifications.

**NCP (Network Co-Processor)**
: A separate chip running Zigbee stack firmware that handles all Zigbee protocol operations. In this project, the **NCP** is typically an ESP32-H2 running the NCP firmware. The NCP receives commands from the application host, executes them via the Zigbee stack, and sends responses/notifications back.

**Host Connection Mode** (in platform config)
: A configuration option in `esp_zb_platform_config_t` that specifies how the Zigbee stack connects to a host application. Options include:
- `ZB_HOST_CONNECTION_MODE_NONE`: No host connection (stack runs standalone)
- `ZB_HOST_CONNECTION_MODE_CLI_UART`: CLI interface over UART
- `ZB_HOST_CONNECTION_MODE_RCP_UART`: RCP (Remote Co-Processor) mode over UART

**Note**: In NCP firmware, this is set to `ZB_HOST_CONNECTION_MODE_NONE` because the NCP chip itself runs the Zigbee stack directly. The "host connection" here refers to the Zigbee stack's internal host interface, not the NCP's serial connection to the application host.

**Native Radio Mode**
: The Zigbee radio is on the same chip as the Zigbee stack. This is the normal mode for NCP firmware (ESP32-H2 has both the stack and radio on-chip).

**RCP (Remote Co-Processor) Mode**
: The Zigbee radio is on a separate chip connected via UART. The stack runs on one chip and controls a remote radio chip. This is different from NCP mode - RCP is about radio separation, while NCP is about stack separation.

**Stack** (Zigbee Stack)
: The complete Zigbee protocol implementation (`esp-zigbee-lib`), including all layers (PHY, MAC, NWK, APS, ZDO, ZCL, BDB). The stack handles network formation, device joining, routing, security, and application data transmission.

**Platform**
: The hardware abstraction layer that interfaces the Zigbee stack with the physical radio and system resources. Platform configuration (`esp_zb_platform_config()`) sets up radio mode, host connection mode (platform config), and MAC parameters.

### Protocol Layer Terms

**APS (Application Support Sublayer)**
: Zigbee layer responsible for end-to-end data delivery between application endpoints. APS handles addressing, fragmentation, security, and reliable delivery. APS commands in NCP protocol: `ESP_NCP_APS_DATA_REQUEST`, `ESP_NCP_APS_DATA_CONFIRM`, `ESP_NCP_APS_DATA_INDICATION`.

**ZDO (Zigbee Device Object)**
: Zigbee layer that manages device-level operations: network discovery, device discovery, binding, network management (permit join, device leave). ZDO commands in NCP protocol: `ESP_NCP_ZDO_IEEE_ADDR_REQ`, `ESP_NCP_ZDO_NODE_DESC_REQ`, `ESP_NCP_ZDO_SIMPLE_DESC_REQ`.

**ZCL (Zigbee Cluster Library)**
: Application-layer protocol defining standard clusters (groups of attributes and commands) for common device types (lights, sensors, switches, etc.). ZCL commands in NCP protocol: `ESP_NCP_ZCL_READ_ATTR`, `ESP_NCP_ZCL_WRITE_ATTR`, `ESP_NCP_ZCL_REPORT_ATTR`.

**BDB (Base Device Behavior)**
: Zigbee 3.0 commissioning framework that standardizes network formation, joining, and finding/binding procedures. BDB provides high-level APIs like `esp_zb_bdb_start_top_level_commissioning()`.

### Communication Terms

**Command**
: A request sent from application host to NCP. Commands have a command ID (`ESP_NCP_*`), optional payload, and expect a response. Commands are synchronous (application host waits for response).

**Response**
: The immediate answer to a command, sent from NCP to application host. Contains status code and optional result data. Responses match the sequence number (`sn`) of the request.

**Notification**
: An asynchronous event sent from NCP to application host, not in response to a command. Notifications indicate Zigbee stack events (network formed, device joined, APS data received, etc.). Notifications have their own sequence numbers.

**Signal**
: An internal Zigbee stack event delivered to the application via `esp_zb_app_signal_handler()`. Signals represent all stack events (network formation, device joins, APS data, etc.). The NCP firmware converts signals to notifications for the application host.

**Callback**
: A function registered with the Zigbee stack that gets invoked when specific events occur (e.g., APS data indication callback, ZDO command response callback). Callbacks are called from stack context and must be thread-safe.

### State Terms

**Initialized** (`s_init_flag`)
: Platform has been configured via `esp_zb_platform_config()`. Radio and host connection mode (platform config) are set.

**Stack Initialized** (`s_zb_stack_inited`)
: Zigbee stack has been initialized via `esp_zb_init()`. Stack data structures are ready, but network operations not yet started.

**Started** (`s_start_flag`)
: Zigbee stack has been started via `esp_zb_start()`. Stack main loop is running and network operations are active.

**Connected** (network state)
: Device has successfully joined a Zigbee network and can communicate with other devices.

---

## Overview

`esp_ncp_zb.c` is the core file that bridges NCP protocol commands from the application host to the Zigbee stack (`esp-zigbee-lib`). It contains:

- **60+ command handler functions** - One per ESP_NCP_* command ID
- **Callback handlers** - For asynchronous Zigbee stack events
- **Response formatters** - Convert Zigbee stack responses to NCP protocol format
- **State management** - Tracks initialization, channel masks, security settings
- **Command dispatch table** - Maps command IDs to handler functions

### Key Design Patterns

1. **Command-Response Pattern**: Application host sends command → NCP processes → NCP sends response
2. **Notification Pattern**: Zigbee stack events → NCP sends async notifications to application host
3. **Deferred Configuration**: Some settings stored until stack initialization completes
4. **Queue-Based APS Events**: APS indications/confirms use FreeRTOS queues for synchronization

---

## File Structure and Architecture

### Static State Variables

```c
static bool s_init_flag = false;                    // Platform initialized
static bool s_start_flag = false;                  // Stack started
static uint32_t s_primary_channel = 0;            // Primary channel mask
static bool s_zb_stack_inited = false;             // Stack initialized
static uint32_t s_channel_mask = 0;                // General channel mask
static bool s_nvram_erase_at_start = false;        // NVRAM erase policy
static bool s_link_key_exchange_required = true;   // Security mode
static uint8_t s_tc_preconfigure_key[16] = {...}; // Trust center key
static QueueHandle_t s_aps_data_confirm;            // APS confirm queue
static QueueHandle_t s_aps_data_indication;        // APS indication queue
```

### Internal Data Structures

```c
// Cluster function table for endpoint registration
typedef struct {
    uint16_t cluster_id;
    esp_err_t (*add_cluster_fn)(...);
    esp_err_t (*del_cluster_fn)(...);
} esp_ncp_zb_cluster_fn_t;

// Context for queue-based APS events
typedef struct {
    uint16_t id;      // Event ID
    uint16_t size;    // Data size
    void *data;       // Data pointer
} esp_ncp_zb_ctx_t;
```

### Command Handler Function Signature

All command handlers follow this pattern:

```c
static esp_err_t esp_ncp_zb_<command>_fn(
    const uint8_t *input,      // Request payload
    uint16_t inlen,            // Payload length
    uint8_t **output,          // Response payload (allocated by handler)
    uint16_t *outlen           // Response length
)
```

---

## Helper Functions and Callbacks

### APS Data Handling Functions

#### `esp_ncp_zb_aps_data_handle()` (Lines 68-96)

**Purpose**: Routes APS data events (indications/confirms) to either a FreeRTOS queue or direct notification.

**Function**:
```c
static esp_err_t esp_ncp_zb_aps_data_handle(uint16_t id, const void *buffer, uint16_t len)
```

**Behavior**:
- If queue exists (`s_aps_data_confirm` or `s_aps_data_indication`): Allocates memory, copies data, sends to queue
- If no queue: Sends notification directly via `esp_ncp_noti_input()`
- Handles ISR context: Uses `xQueueSendFromISR()` if called from interrupt

**Zigbee Bus Impact**: None directly - this is a routing/dispatch function.

**Low-Level Calls**:
- `calloc()` - Memory allocation
- `memcpy()` - Data copying
- `xQueueSend()` / `xQueueSendFromISR()` - FreeRTOS queue operations
- `esp_ncp_noti_input()` - NCP notification sending

---

#### `esp_ncp_zb_aps_data_indication_handler()` (Lines 98-160)

**Purpose**: Callback registered with Zigbee stack to receive incoming APS data frames.

**Function**:
```c
static bool esp_ncp_zb_aps_data_indication_handler(esp_zb_apsde_data_ind_t ind)
```

**Called By**: Zigbee stack when APS frame received over the air.

**Behavior**:
1. Allocates buffer for NCP notification payload
2. Converts `esp_zb_apsde_data_ind_t` to `esp_ncp_zb_aps_data_ind_t` format
3. Handles address mode conversion (short vs IEEE)
4. Copies ASDU (Application Service Data Unit) payload
5. Calls `esp_ncp_zb_aps_data_handle()` to route to application host

**Zigbee Bus Impact**: 
- **Receives** APS frames from Zigbee network
- **Decodes** APS headers (source/destination addresses, endpoints, clusters)
- **Extracts** application payload (ZCL commands, attribute reports, etc.)

**Low-Level Calls**:
- `calloc()` - Buffer allocation
- `memcpy()` - Data structure conversion
- `esp_ncp_zb_aps_data_handle()` - Event routing

**Payload Structure**:
```c
typedef struct {
    uint8_t states;              // ESP_NCP_INDICATION
    uint8_t dst_addr_mode;
    esp_zb_addr_u dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_addr_mode;
    esp_zb_addr_u src_addr;
    uint8_t src_endpoint;
    uint16_t profile_id;
    uint16_t cluster_id;
    uint8_t indication_status;
    uint8_t security_status;
    uint8_t lqi;                 // Link Quality Indicator
    int rx_time;
    uint32_t asdu_length;
    uint8_t asdu[asdu_length];   // Application payload
} esp_ncp_zb_aps_data_ind_t;
```

---

#### `esp_ncp_zb_aps_data_confirm_handler()` (Lines 162-199)

**Purpose**: Callback registered with Zigbee stack to receive APS transmission confirmations.

**Function**:
```c
static void esp_ncp_zb_aps_data_confirm_handler(esp_zb_apsde_data_confirm_t confirm)
```

**Called By**: Zigbee stack after APS frame transmission completes (success or failure).

**Behavior**:
1. Allocates buffer for confirmation payload
2. Converts `esp_zb_apsde_data_confirm_t` to `esp_ncp_zb_aps_data_confirm_t`
3. Copies destination address and endpoints
4. Includes transmission status and ASDU (if any)
5. Routes via `esp_ncp_zb_aps_data_handle()`

**Zigbee Bus Impact**:
- **Confirms** APS frame transmission status
- **Reports** delivery success/failure to application host
- **Includes** ASDU if transmission failed (for retry)

**Low-Level Calls**:
- `calloc()` - Buffer allocation
- `memcpy()` - Structure conversion
- `esp_ncp_zb_aps_data_handle()` - Event routing

**Payload Structure**:
```c
typedef struct {
    uint8_t states;              // ESP_NCP_CONFIRM
    uint8_t dst_addr_mode;
    esp_zb_zcl_basic_cmd_t basic_cmd;
    int tx_time;
    uint8_t confirm_status;      // 0 = success, non-zero = error
    uint32_t asdu_length;
    uint8_t asdu[asdu_length];  // Original ASDU if failed
} esp_ncp_zb_aps_data_confirm_t;
```

---

### ZDO Callback Functions

#### `esp_ncp_zb_bind_cb()` (Lines 206-228)

**Purpose**: Callback for ZDO bind request completion.

**Function**:
```c
static void esp_ncp_zb_bind_cb(esp_zb_zdp_status_t zdo_status, void *user_ctx)
```

**Called By**: Zigbee stack after bind request completes.

**Behavior**:
- Extracts user context (callback info)
- Formats notification payload with status
- Sends notification with `ESP_NCP_ZDO_BIND_SET` ID

**Zigbee Bus Impact**:
- **Confirms** binding creation between endpoints
- **Reports** ZDP status (success/failure codes)

**Low-Level Calls**:
- `esp_ncp_noti_input()` - Send notification to application host

---

#### `esp_ncp_zb_unbind_cb()` (Lines 230-252)

**Purpose**: Callback for ZDO unbind request completion.

**Function**:
```c
static void esp_ncp_zb_unbind_cb(esp_zb_zdp_status_t zdo_status, void *user_ctx)
```

**Behavior**: Same as bind callback, but for unbind operations.

**Zigbee Bus Impact**:
- **Confirms** binding removal
- **Reports** ZDP status

---

#### `esp_ncp_zb_find_match_cb()` (Lines 254-280)

**Purpose**: Callback for ZDO match descriptor request completion.

**Function**:
```c
static void esp_ncp_zb_find_match_cb(esp_zb_zdp_status_t zdo_status, uint16_t addr, 
                                     uint8_t endpoint, void *user_ctx)
```

**Behavior**:
- Extracts matched device address and endpoint
- Formats notification with match results
- Sends notification with `ESP_NCP_ZDO_FIND_MATCH` ID

**Zigbee Bus Impact**:
- **Reports** device discovery results
- **Identifies** matching endpoints for cluster communication

**Payload Structure**:
```c
typedef struct {
    esp_zb_zdp_status_t zdo_status;
    uint16_t addr;              // Matched device short address
    uint8_t endpoint;           // Matched endpoint
    esp_ncp_zb_user_cb_t zdo_cb; // User callback context
} esp_ncp_zb_find_parameters_t;
```

---

#### `esp_ncp_zb_zdo_scan_complete_handler()` (Lines 282-310)

**Purpose**: Callback for active scan completion.

**Function**:
```c
static void esp_ncp_zb_zdo_scan_complete_handler(esp_zb_zdp_status_t zdo_status, 
                                                  uint8_t count, 
                                                  esp_zb_network_descriptor_t *nwk_descriptor)
```

**Called By**: Zigbee stack after active scan finishes.

**Behavior**:
1. Allocates buffer for scan results
2. Copies status and network count
3. Appends array of network descriptors
4. Sends notification with `ESP_NCP_NETWORK_SCAN_COMPLETE_HANDLER` ID

**Zigbee Bus Impact**:
- **Reports** discovered Zigbee networks
- **Includes** PAN IDs, extended PAN IDs, channels, security info
- **Enables** network selection for joining

**Low-Level Calls**:
- `calloc()` - Buffer allocation
- `memcpy()` - Network descriptor copying
- `esp_ncp_noti_input()` - Notification sending

**Payload Structure**:
```c
typedef struct {
    esp_zb_zdp_status_t zdo_status;
    uint8_t count;
    // Followed by count * sizeof(esp_zb_network_descriptor_t)
} esp_ncp_zb_scan_parameters_t;

// Each network descriptor:
typedef struct {
    uint16_t short_pan_id;
    bool permit_joining;
    esp_zb_ieee_addr_t extended_pan_id;  // 8 bytes
    uint8_t logic_channel;
    bool router_capacity;
    bool end_device_capacity;
} esp_zb_network_descriptor_t;
```

---

### ZCL Response Handlers

These functions convert Zigbee stack ZCL responses into NCP notification format.

#### `esp_ncp_zb_read_attr_resp_handler()` (Lines 312-352)

**Purpose**: Formats ZCL read attribute response for application host notification.

**Function**:
```c
static esp_err_t esp_ncp_zb_read_attr_resp_handler(
    const esp_zb_zcl_cmd_read_attr_resp_message_t *message,
    uint8_t **output, 
    uint16_t *outlen
)
```

**Called By**: `esp_ncp_zb_action_handler()` when `ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID` received.

**Behavior**:
1. Validates message and status
2. Iterates through attribute response variables
3. Builds payload: command info + attribute count + (attribute ID + type + size + value) for each
4. Allocates and formats output buffer

**Zigbee Bus Impact**:
- **Formats** attribute read responses from remote devices
- **Includes** attribute IDs, data types, and values

**Low-Level Calls**:
- `calloc()` / `realloc()` - Dynamic buffer allocation
- `memcpy()` - Data copying

**Payload Format**:
```
[esp_zb_zcl_cmd_info_t]  // Status, cluster, endpoints
[uint8_t count]          // Number of attributes
[For each attribute:]
  [uint16_t attr_id]
  [esp_zb_zcl_attr_type_t type]
  [uint8_t size]
  [uint8_t value[size]]
```

---

#### `esp_ncp_zb_write_attr_resp_handler()` (Lines 354-387)

**Purpose**: Formats ZCL write attribute response.

**Function**:
```c
static esp_err_t esp_ncp_zb_write_attr_resp_handler(
    const esp_zb_zcl_cmd_write_attr_resp_message_t *message,
    uint8_t **output,
    uint16_t *outlen
)
```

**Behavior**: Similar to read handler, but includes status per attribute instead of values.

**Payload Format**:
```
[esp_zb_zcl_cmd_info_t]
[uint8_t count]
[For each attribute:]
  [esp_zb_zcl_status_t status]
  [uint16_t attr_id]
```

---

#### `esp_ncp_zb_report_configure_resp_handler()` (Lines 389-421)

**Purpose**: Formats ZCL configure report response.

**Payload Format**:
```
[esp_zb_zcl_cmd_info_t]
[uint8_t count]
[For each attribute:]
  [esp_zb_zcl_status_t status]
  [uint8_t direction]
  [uint16_t attr_id]
```

---

#### `esp_ncp_zb_disc_attr_resp_handler()` (Lines 423-456)

**Purpose**: Formats ZCL discover attributes response.

**Payload Format**:
```
[esp_zb_zcl_cmd_info_t]
[uint8_t count]
[For each attribute:]
  [uint16_t attr_id]
  [esp_zb_zcl_attr_type_t data_type]
```

---

#### `esp_ncp_zb_report_attr_handler()` (Lines 458-499)

**Purpose**: Formats ZCL attribute report (unsolicited report from device).

**Function**:
```c
static esp_err_t esp_ncp_zb_report_attr_handler(
    const esp_zb_zcl_report_attr_message_t *message,
    uint8_t **output,
    uint16_t *outlen
)
```

**Zigbee Bus Impact**:
- **Receives** unsolicited attribute reports from devices
- **Formats** for application host consumption
- **Includes** source address, endpoint, cluster, attribute ID, type, value

**Payload Format**:
```
[esp_zb_zcl_status_t status]
[esp_zb_zcl_addr_t src_address]
[uint8_t src_endpoint]
[uint8_t dst_endpoint]
[uint16_t cluster]
[uint16_t attr_id]
[uint8_t attr_type]
[uint8_t attr_size]
[uint8_t attr_value[attr_size]]
```

---

#### `esp_ncp_zb_action_handler()` (Lines 501-543)

**Purpose**: Central dispatcher for ZCL action callbacks from Zigbee stack.

**Function**:
```c
static esp_err_t esp_ncp_zb_action_handler(
    esp_zb_core_action_callback_id_t callback_id,
    const void *message
)
```

**Called By**: Zigbee stack core via `esp_zb_core_action_handler_register()`.

**Behavior**:
- Routes callbacks by ID to appropriate response handler
- Sends notifications with correct command ID
- Handles: read attr resp, write attr resp, report config resp, disc attr resp, report attr

**Low-Level Calls**:
- Response handler functions (read/write/disc/report)
- `esp_ncp_noti_input()` - Notification sending

---

### Utility Functions

#### `esp_ncp_apply_channel_config()` (Lines 545-559)

**Purpose**: Applies deferred channel configuration after stack initialization.

**Function**:
```c
static void esp_ncp_apply_channel_config(void)
```

**Called By**: `esp_zb_app_signal_handler()` on `ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START`.

**Behavior**:
- Checks if stack initialized
- Applies general channel mask if set
- Applies primary channel mask if set

**Zigbee Bus Impact**:
- **Configures** radio channel selection
- **Affects** network formation and scanning

**Low-Level Calls**:
- `esp_zb_set_channel_mask()` - Set general channel mask
- `esp_zb_set_primary_network_channel_set()` - Set primary channel mask

---

#### `esp_ncp_zb_task()` (Lines 561-566)

**Purpose**: FreeRTOS task that runs the Zigbee stack main loop.

**Function**:
```c
static void esp_ncp_zb_task(void *pvParameters)
```

**Created By**: `esp_ncp_zb_start_fn()` when stack starts.

**Behavior**:
1. Registers ZCL action handler
2. Calls `esp_zb_stack_main_loop()` (blocks forever)
3. Deletes task if loop exits (shouldn't happen)

**Zigbee Bus Impact**:
- **Runs** Zigbee stack event processing
- **Handles** all stack callbacks and timers
- **Processes** incoming frames and state machines

**Low-Level Calls**:
- `esp_zb_core_action_handler_register()` - Register ZCL callback
- `esp_zb_stack_main_loop()` - Main stack loop (blocks)

---

#### `esp_zb_app_signal_handler()` (Lines 568-752)

**Purpose**: Application signal handler - receives all Zigbee stack events.

**Function**:
```c
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
```

**Called By**: Zigbee stack for all application-level events.

**Behavior**: Large switch statement handling 30+ signal types:

**Key Signal Handlers**:

1. **ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP** (Line 581):
   - Starts BDB initialization mode
   - **Low-Level**: `esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION)`

2. **ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE** (Line 585):
   - Device joined/rejoined network
   - Sends `ESP_NCP_NETWORK_JOINNETWORK` notification
   - **Low-Level**: `esp_zb_app_signal_get_params()` - Extract device info

3. **ESP_ZB_ZDO_SIGNAL_LEAVE** (Line 606):
   - Device left network
   - Sends `ESP_NCP_NETWORK_LEAVENETWORK` notification

4. **ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START** / **ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT** (Line 666):
   - Applies channel config
   - Starts network formation
   - **Low-Level**: `esp_ncp_apply_channel_config()`, `esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION)`

5. **ESP_ZB_BDB_SIGNAL_FORMATION** (Line 687):
   - Network formation completed
   - Sends `ESP_NCP_NETWORK_FORMNETWORK` notification with PAN ID, extended PAN ID, channel
   - Starts network steering
   - **Low-Level**: `esp_zb_get_pan_id()`, `esp_zb_get_current_channel()`, `esp_zb_get_extended_pan_id()`, `esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING)`

6. **ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS** (Line 729):
   - Permit join status changed
   - Sends `ESP_NCP_NETWORK_PERMIT_JOINING` notification with duration

**Zigbee Bus Impact**:
- **Receives** all Zigbee stack events
- **Translates** stack signals to NCP notifications
- **Manages** network lifecycle (formation, joining, leaving)
- **Handles** device announcements and updates

**Low-Level Calls**:
- `esp_zb_bdb_start_top_level_commissioning()` - Start commissioning modes
- `esp_zb_get_pan_id()` - Get PAN ID
- `esp_zb_get_current_channel()` - Get channel
- `esp_zb_get_extended_pan_id()` - Get extended PAN ID
- `esp_zb_app_signal_get_params()` - Extract signal parameters
- `esp_zb_scheduler_alarm()` - Schedule delayed actions
- `esp_ncp_noti_input()` - Send notifications

---

## Network Commands (0x0000-0x002F)

### 0x0000: ESP_NCP_NETWORK_INIT

**Handler**: `esp_ncp_zb_network_init_fn()` (Lines 755-780)

**Purpose**: Initialize Zigbee platform (radio and host connection mode).

**Request Payload**: None (ignored)

**Response Payload**:
```c
uint8_t status;  // ESP_NCP_SUCCESS or ESP_NCP_ERR_FATAL
```

**Behavior**:
1. Checks `s_init_flag` to prevent double initialization
2. Configures platform with native radio mode and no host connection mode (platform config)
3. Sets `s_init_flag = true` on success

**Note on "Host Connection"**: The `host_connection_mode` parameter in platform config refers to the Zigbee stack's internal host interface (for CLI or RCP scenarios), not the NCP's serial connection to the application host. In NCP firmware, this is set to `ZB_HOST_CONNECTION_MODE_NONE` because the stack runs directly on the NCP chip - there's no separate host application from the stack's perspective. The NCP's UART/SPI connection to the application host is handled separately by the NCP transport layer (`esp_ncp_bus.c`).

**Zigbee Bus Impact**:
- **Initializes** Zigbee platform
- **Configures** radio in native mode (radio on same chip as stack, not RCP mode)
- **Sets** host connection mode to NONE (stack runs standalone on NCP chip)

**Low-Level Calls**:
- `esp_zb_platform_config()` - Platform initialization
  - Configures: `radio_mode = ZB_RADIO_MODE_NATIVE`
  - Configures: `host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE`

**State Changes**:
- `s_init_flag = true` (if successful)

---

### 0x0001: ESP_NCP_NETWORK_START

**Handler**: `esp_ncp_zb_start_fn()` (Lines 963-976)

**Purpose**: Start the Zigbee stack commissioning process.

**Request Payload**:
```c
bool autostart;  // true = autostart, false = no-autostart
```

**Response Payload**:
```c
uint8_t status;
```

**Behavior**:
1. Calls `esp_zb_start()` with autostart flag
2. Creates `esp_ncp_zb_task` FreeRTOS task if not already created
3. Sets `s_start_flag = true`

**Zigbee Bus Impact**:
- **Starts** Zigbee stack
- **Begins** commissioning process (formation or joining)
- **Creates** stack main loop task
- **Registers** application signal handler

**Low-Level Calls**:
- `esp_zb_start()` - Start Zigbee stack
- `xTaskCreate()` - Create FreeRTOS task for stack loop

**State Changes**:
- `s_start_flag = true`
- Creates `esp_ncp_zb_task` task

---

### 0x0002: ESP_NCP_NETWORK_STATE

**Handler**: `esp_ncp_zb_network_state_fn()` (Lines 1031-1041)

**Purpose**: Get current network connection state.

**Request Payload**: None

**Response Payload**:
```c
uint8_t state;  // Always ESP_NCP_CONNECTED (hardcoded)
```

**Behavior**: Returns hardcoded `ESP_NCP_CONNECTED` value.

**Zigbee Bus Impact**: None - informational only.

**Note**: This is a stub implementation - doesn't query actual stack state.

---

### 0x0003: ESP_NCP_NETWORK_STACK_STATUS_HANDLER

**Handler**: `esp_ncp_zb_stack_status_fn()` (Lines 1043-1051)

**Purpose**: Register stack status change handler (notification only, no request handler).

**Request Payload**: None (ignored)

**Response Payload**:
```c
uint8_t status;
```

**Behavior**: Stub - always returns success.

**Note**: Stack status changes are sent as notifications via `esp_zb_app_signal_handler()`.

---

### 0x0004: ESP_NCP_NETWORK_FORMNETWORK

**Handler**: `esp_ncp_zb_form_network_fn()` (Lines 904-938)

**Purpose**: Form a new Zigbee network (become coordinator).

**Request Payload**:
```c
typedef struct {
    esp_zb_nwk_device_type_t esp_zb_role;  // Coordinator/Router/End Device
    bool install_code_policy;
    union {
        esp_zb_zczr_cfg_t zczr_cfg;        // Coordinator/Router config
        esp_zb_zed_cfg_t zed_cfg;          // End Device config
    } nwk_cfg;
} esp_zb_cfg_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Behavior**:
1. Sets NVRAM erase policy
2. Copies configuration to static buffer (avoids pointer issues)
3. Calls `esp_zb_init()` with configuration
4. Sets `s_zb_stack_inited = true`
5. Configures security (TC key, link key exchange)
6. Applies deferred channel masks

**Zigbee Bus Impact**:
- **Initializes** Zigbee stack with device role
- **Configures** security keys and policies
- **Applies** channel masks (affects network formation)
- **Prepares** for network formation (triggered by BDB signals)

**Low-Level Calls**:
- `esp_zb_nvram_erase_at_start()` - Set NVRAM erase policy
- `esp_zb_init()` - Initialize Zigbee stack
- `esp_zb_secur_TC_standard_preconfigure_key_set()` - Set trust center key
- `esp_zb_secur_link_key_exchange_required_set()` - Set security mode
- `esp_zb_set_channel_mask()` - Apply channel mask
- `esp_zb_set_primary_network_channel_set()` - Apply primary channel mask

**State Changes**:
- `s_zb_stack_inited = true`
- Network formation triggered via BDB signals

**Notification**: On successful formation, `ESP_NCP_NETWORK_FORMNETWORK` notification sent with PAN ID, extended PAN ID, and channel.

---

### 0x0005: ESP_NCP_NETWORK_PERMIT_JOINING

**Handler**: `esp_ncp_zb_permit_joining_fn()` (Lines 978-993)

**Purpose**: Allow or disallow devices to join the network.

**Request Payload**:
```c
uint8_t permit_duration;  // Seconds (0x00 = disable, 0xFF = enable indefinitely)
```

**Response Payload**:
```c
uint8_t status;
```

**Behavior**:
1. Extracts permit duration from payload
2. Calls `esp_zb_bdb_open_network()` with duration

**Zigbee Bus Impact**:
- **Opens** network for joining (if duration > 0)
- **Closes** network (if duration = 0)
- **Broadcasts** permit join beacon frames
- **Affects** device association behavior

**Low-Level Calls**:
- `esp_zb_bdb_open_network()` - Open/close network for joining

**Notification**: `ESP_NCP_NETWORK_PERMIT_JOINING` notification sent when status changes (via `ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS` signal).

---

### 0x0006: ESP_NCP_NETWORK_JOINNETWORK

**Handler**: NULL (notification only)

**Purpose**: Notification sent when device joins network.

**Notification Payload**:
```c
typedef struct {
    uint16_t device_short_addr;
    esp_zb_ieee_addr_t ieee_addr;  // 8 bytes
    uint8_t capability;
} esp_zb_zdo_signal_device_annce_params_t;
```

**Sent By**: `esp_zb_app_signal_handler()` on `ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE`.

**Zigbee Bus Impact**:
- **Indicates** new device joined or rejoined
- **Includes** device addresses and capabilities
- **Enables** application host to track network membership

---

### 0x0007: ESP_NCP_NETWORK_LEAVENETWORK

**Handler**: NULL (notification only)

**Purpose**: Notification sent when device leaves network.

**Notification Payload**:
```c
typedef struct {
    uint16_t short_addr;
    esp_zb_ieee_addr_t device_addr;  // 8 bytes
    uint8_t rejoin;
} esp_zb_zdo_signal_leave_indication_params_t;
```

**Sent By**: `esp_zb_app_signal_handler()` on `ESP_ZB_ZDO_SIGNAL_LEAVE`.

---

### 0x0008: ESP_NCP_NETWORK_START_SCAN

**Handler**: `esp_ncp_zb_start_scan_fn()` (Lines 940-951)

**Purpose**: Start active scan for available Zigbee networks.

**Request Payload**:
```c
typedef struct {
    uint32_t channel_mask;     // Channels to scan (0x00000800 - 0x07FFF800)
    uint8_t scan_duration;     // Scan duration per channel
} esp_ncp_zb_scan_parameters_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Behavior**:
1. Extracts channel mask and duration from payload
2. Calls `esp_zb_zdo_active_scan_request()` with callback

**Zigbee Bus Impact**:
- **Initiates** active scan on specified channels
- **Transmits** beacon request frames
- **Receives** beacon responses from coordinators/routers
- **Discovers** available networks

**Low-Level Calls**:
- `esp_zb_zdo_active_scan_request()` - Start active scan
  - Parameters: `channel_mask`, `scan_duration`, `esp_ncp_zb_zdo_scan_complete_handler`

**Notification**: `ESP_NCP_NETWORK_SCAN_COMPLETE_HANDLER` sent when scan completes.

---

### 0x0009: ESP_NCP_NETWORK_SCAN_COMPLETE_HANDLER

**Handler**: `esp_ncp_zb_scan_complete_fn()` (Lines 953-956)

**Purpose**: Stub handler (scan completion sent as notification).

**Behavior**: Returns success immediately.

**Note**: Actual scan results sent via `esp_ncp_zb_zdo_scan_complete_handler()` callback.

---

### 0x000A: ESP_NCP_NETWORK_STOP_SCAN

**Handler**: `esp_ncp_zb_stop_scan_fn()` (Lines 958-961)

**Purpose**: Stop active scan (stub implementation).

**Behavior**: Returns success immediately.

**Note**: No actual stop functionality implemented.

---

### 0x000B: ESP_NCP_NETWORK_PAN_ID_GET

**Handler**: `esp_ncp_zb_pan_id_get_fn()` (Lines 891-902)

**Purpose**: Get current PAN ID.

**Request Payload**: None

**Response Payload**:
```c
uint16_t pan_id;
```

**Zigbee Bus Impact**: None - read-only query.

**Low-Level Calls**:
- `esp_zb_get_pan_id()` - Get PAN ID from stack

---

### 0x000C: ESP_NCP_NETWORK_PAN_ID_SET

**Handler**: `esp_ncp_zb_pan_id_set_fn()` (Lines 877-889)

**Purpose**: Set PAN ID (before network formation).

**Request Payload**:
```c
uint16_t pan_id;
```

**Response Payload**:
```c
uint8_t status;
```

**Zigbee Bus Impact**:
- **Sets** PAN ID for network formation
- **Must** be called before `ESP_NCP_NETWORK_FORMNETWORK`
- **Affects** network identifier

**Low-Level Calls**:
- `esp_zb_set_pan_id()` - Set PAN ID in stack

---

### 0x000D: ESP_NCP_NETWORK_EXTENDED_PAN_ID_GET

**Handler**: `esp_ncp_zb_extended_pan_id_get_fn()` (Lines 865-875)

**Purpose**: Get extended PAN ID.

**Request Payload**: None

**Response Payload**:
```c
esp_zb_ieee_addr_t extended_pan_id;  // 8 bytes
```

**Low-Level Calls**:
- `esp_zb_get_extended_pan_id()` - Get extended PAN ID

---

### 0x000E: ESP_NCP_NETWORK_EXTENDED_PAN_ID_SET

**Handler**: `esp_ncp_zb_extended_pan_id_set_fn()` (Lines 851-863)

**Purpose**: Set extended PAN ID.

**Request Payload**:
```c
esp_zb_ieee_addr_t extended_pan_id;  // 8 bytes
```

**Low-Level Calls**:
- `esp_zb_set_extended_pan_id()` - Set extended PAN ID

---

### 0x000F: ESP_NCP_NETWORK_PRIMARY_CHANNEL_GET

**Handler**: `esp_ncp_zb_primary_channel_get_fn()` (Lines 1444-1455)

**Purpose**: Get primary channel mask.

**Request Payload**: None

**Response Payload**:
```c
uint32_t channel_mask;
```

**Behavior**: Returns cached `s_primary_channel` value.

**Low-Level Calls**: None (returns cached value).

---

### 0x0010: ESP_NCP_NETWORK_PRIMARY_CHANNEL_SET

**Handler**: `esp_ncp_zb_network_primary_channel_set_fn()` (Lines 782-804)

**Purpose**: Set primary channel mask.

**Request Payload**:
```c
uint32_t channel_mask;  // 0x00000800 (ch 11) to 0x07FFF800 (ch 11-26)
```

**Response Payload**:
```c
uint8_t status;
```

**Behavior**:
1. Stores mask in `s_primary_channel`
2. Applies immediately if stack initialized, otherwise deferred

**Zigbee Bus Impact**:
- **Restricts** network formation to specified channels
- **Affects** channel selection during formation
- **Must** be set before network formation

**Low-Level Calls**:
- `esp_zb_set_primary_network_channel_set()` - Set primary channel mask (if stack initialized)

**State Changes**:
- `s_primary_channel = mask`

---

### 0x0011: ESP_NCP_NETWORK_SECONDARY_CHANNEL_GET

**Handler**: Not implemented (stub).

---

### 0x0012: ESP_NCP_NETWORK_SECONDARY_CHANNEL_SET

**Handler**: `esp_ncp_zb_network_secondary_channel_set_fn()` (Lines 806-814)

**Purpose**: Set secondary channel mask.

**Request Payload**:
```c
uint32_t channel_mask;
```

**Low-Level Calls**:
- `esp_zb_set_secondary_network_channel_set()` - Set secondary channel mask

---

### 0x0013: ESP_NCP_NETWORK_CHANNEL_GET

**Handler**: `esp_ncp_zb_current_channel_fn()` (Lines 1432-1442)

**Purpose**: Get current operating channel.

**Request Payload**: None

**Response Payload**:
```c
uint8_t channel;
```

**Low-Level Calls**:
- `esp_zb_get_current_channel()` - Get current channel

---

### 0x0014: ESP_NCP_NETWORK_CHANNEL_SET

**Handler**: `esp_ncp_zb_channel_set_fn()` (Lines 816-835)

**Purpose**: Set general channel mask.

**Request Payload**:
```c
uint32_t channel_mask;
```

**Behavior**:
1. Stores mask in `s_channel_mask`
2. Applies immediately if stack initialized, otherwise deferred

**Zigbee Bus Impact**:
- **Restricts** channel selection for scanning and formation
- **Broader** than primary channel mask

**Low-Level Calls**:
- `esp_zb_set_channel_mask()` - Set channel mask (if stack initialized)

**State Changes**:
- `s_channel_mask = mask`

---

### 0x0015: ESP_NCP_NETWORK_TXPOWER_GET

**Handler**: Not implemented (stub).

---

### 0x0016: ESP_NCP_NETWORK_TXPOWER_SET

**Handler**: `esp_ncp_zb_tx_power_set_fn()` (Lines 837-849)

**Purpose**: Set transmit power.

**Request Payload**:
```c
int8_t power;  // dB
```

**Low-Level Calls**:
- `esp_zb_set_tx_power()` - Set TX power

**Zigbee Bus Impact**:
- **Adjusts** radio transmit power
- **Affects** range and interference

---

### 0x0017: ESP_NCP_NETWORK_PRIMARY_KEY_GET

**Handler**: `esp_ncp_zb_primary_key_get_fn()` (Lines 1457-1470)

**Purpose**: Get primary network key.

**Request Payload**: None

**Response Payload**:
```c
uint8_t key[16];  // Network key
```

**Low-Level Calls**:
- `esp_zb_secur_primary_network_key_get()` - Get network key

---

### 0x0018: ESP_NCP_NETWORK_PRIMARY_KEY_SET

**Handler**: `esp_ncp_zb_primary_key_set_fn()` (Lines 1472-1480)

**Purpose**: Set primary network key.

**Request Payload**:
```c
uint8_t key[16];  // Network key
```

**Low-Level Calls**:
- `esp_zb_secur_network_key_set()` - Set network key

**Zigbee Bus Impact**:
- **Sets** network encryption key
- **Affects** security of all network communications
- **Must** match across all network devices

---

### 0x0019: ESP_NCP_NETWORK_FRAME_COUNT_GET

**Handler**: `esp_ncp_zb_nwk_frame_counter_get_fn()` (Lines 1482-1493)

**Purpose**: Get network frame counter (stub - returns hardcoded value).

**Response Payload**:
```c
uint32_t frame_count;  // Always 0x00001388 (hardcoded)
```

**Note**: Stub implementation - doesn't query actual counter.

---

### 0x001A: ESP_NCP_NETWORK_FRAME_COUNT_SET

**Handler**: `esp_ncp_zb_nwk_frame_counter_set_fn()` (Lines 1495-1503)

**Purpose**: Set network frame counter (stub - no-op).

**Note**: Stub implementation.

---

### 0x001B: ESP_NCP_NETWORK_ROLE_GET

**Handler**: `esp_ncp_zb_nwk_role_get_fn()` (Lines 1505-1515)

**Purpose**: Get network role (stub - always returns Coordinator).

**Response Payload**:
```c
uint8_t role;  // Always ESP_ZB_DEVICE_TYPE_COORDINATOR
```

**Note**: Stub implementation.

---

### 0x001C: ESP_NCP_NETWORK_ROLE_SET

**Handler**: `esp_ncp_zb_nwk_role_set_fn()` (Lines 1517-1525)

**Purpose**: Set network role (stub - no-op).

**Note**: Stub implementation.

---

### 0x001D: ESP_NCP_NETWORK_SHORT_ADDRESS_GET

**Handler**: `esp_ncp_zb_short_addr_fn()` (Lines 1407-1418)

**Purpose**: Get device short address.

**Request Payload**: None

**Response Payload**:
```c
uint16_t short_addr;
```

**Low-Level Calls**:
- `esp_zb_get_short_address()` - Get short address

---

### 0x001E: ESP_NCP_NETWORK_SHORT_ADDRESS_SET

**Handler**: `esp_ncp_zb_short_addr_set_fn()` (Lines 1527-1535)

**Purpose**: Set device short address (stub - no-op).

**Note**: Short address assigned by coordinator during joining.

---

### 0x001F: ESP_NCP_NETWORK_LONG_ADDRESS_GET

**Handler**: `esp_ncp_zb_long_addr_fn()` (Lines 1420-1430)

**Purpose**: Get device IEEE address.

**Request Payload**: None

**Response Payload**:
```c
esp_zb_ieee_addr_t long_addr;  // 8 bytes
```

**Low-Level Calls**:
- `esp_zb_get_long_address()` - Get IEEE address

---

### 0x0020: ESP_NCP_NETWORK_LONG_ADDRESS_SET

**Handler**: `esp_ncp_zb_long_addr_set_fn()` (Lines 1537-1545)

**Purpose**: Set device IEEE address.

**Request Payload**:
```c
esp_zb_ieee_addr_t long_addr;  // 8 bytes
```

**Low-Level Calls**:
- `esp_zb_set_long_address()` - Set IEEE address

**Zigbee Bus Impact**:
- **Sets** device IEEE address
- **Must** be unique on network
- **Affects** device identification

---

### 0x0021: ESP_NCP_NETWORK_CHANNEL_MASKS_GET

**Handler**: Not implemented (stub).

---

### 0x0022: ESP_NCP_NETWORK_CHANNEL_MASKS_SET

**Handler**: Uses `esp_ncp_zb_network_primary_channel_set_fn()` (same as 0x0010).

---

### 0x0023: ESP_NCP_NETWORK_UPDATE_ID_GET

**Handler**: `esp_ncp_zb_nwk_update_id_fn()` (Lines 1547-1557)

**Purpose**: Get network update ID (stub - returns 1).

**Response Payload**:
```c
uint8_t update_id;  // Always 1
```

---

### 0x0024: ESP_NCP_NETWORK_UPDATE_ID_SET

**Handler**: `esp_ncp_zb_nwk_update_id_set_fn()` (Lines 1559-1567)

**Purpose**: Set network update ID (stub - no-op).

---

### 0x0025: ESP_NCP_NETWORK_TRUST_CENTER_ADDR_GET

**Handler**: `esp_ncp_zb_nwk_trust_center_addr_fn()` (Lines 1569-1580)

**Purpose**: Get trust center address (stub - returns hardcoded value).

**Response Payload**:
```c
esp_zb_ieee_addr_t trust_center_addr;  // Hardcoded: 0xab:0x98:0x09:0xff:0xff:0x2e:0x21:0x00
```

---

### 0x0026: ESP_NCP_NETWORK_TRUST_CENTER_ADDR_SET

**Handler**: `esp_ncp_zb_nwk_trust_center_addr_set_fn()` (Lines 1582-1590)

**Purpose**: Set trust center address (stub - no-op).

---

### 0x0027: ESP_NCP_NETWORK_LINK_KEY_GET

**Handler**: `esp_ncp_zb_nwk_link_key_fn()` (Lines 1592-1605)

**Purpose**: Get link key (returns device IEEE + TC preconfigure key).

**Response Payload**:
```c
[esp_zb_ieee_addr_t device_addr]  // 8 bytes
[uint8_t link_key[16]]            // TC preconfigure key
```

**Low-Level Calls**:
- `esp_zb_get_long_address()` - Get device IEEE address

---

### 0x0028: ESP_NCP_NETWORK_LINK_KEY_SET

**Handler**: `esp_ncp_zb_nwk_link_key_set_fn()` (Lines 1607-1622)

**Purpose**: Set link key (updates TC preconfigure key).

**Request Payload**:
```c
uint8_t link_key[16];
```

**Behavior**:
1. Updates `s_tc_preconfigure_key`
2. Applies to stack if initialized

**Low-Level Calls**:
- `esp_zb_secur_TC_standard_preconfigure_key_set()` - Set TC key (if stack initialized)

**State Changes**:
- `s_tc_preconfigure_key = link_key`

---

### 0x0029: ESP_NCP_NETWORK_SECURE_MODE_GET

**Handler**: `esp_ncp_zb_nwk_security_mode_fn()` (Lines 1624-1634)

**Purpose**: Get security mode.

**Response Payload**:
```c
uint8_t mode;  // ESP_NCP_PRECONFIGURED_NETWORK_KEY or ESP_NCP_NO_SECURITY
```

**Behavior**: Returns based on `s_link_key_exchange_required`.

---

### 0x002A: ESP_NCP_NETWORK_SECURE_MODE_SET

**Handler**: `esp_ncp_zb_nwk_security_mode_set_fn()` (Lines 1636-1652)

**Purpose**: Set security mode.

**Request Payload**:
```c
uint8_t mode;  // 0 = no security, non-zero = preconfigured key
```

**Behavior**:
1. Updates `s_link_key_exchange_required`
2. Applies to stack if initialized

**Low-Level Calls**:
- `esp_zb_secur_link_key_exchange_required_set()` - Set security mode (if stack initialized)

**State Changes**:
- `s_link_key_exchange_required = (mode != 0)`

---

### 0x002B: ESP_NCP_NETWORK_PREDEFINED_PANID

**Handler**: `esp_ncp_zb_use_predefined_nwk_panid_set_fn()` (Lines 1683-1691)

**Purpose**: Enable/disable predefined PAN ID (stub - no-op).

---

### 0x002C: ESP_NCP_NETWORK_SHORT_TO_IEEE

**Handler**: `esp_ncp_zb_ieee_address_by_short_get_fn()` (Lines 1712-1733)

**Purpose**: Get IEEE address by short address.

**Request Payload**:
```c
uint16_t short_addr;
```

**Response Payload**:
```c
esp_zb_ieee_addr_t ieee_addr;  // 8 bytes
```

**Low-Level Calls**:
- `esp_zb_ieee_address_by_short()` - Lookup IEEE by short address

**Zigbee Bus Impact**:
- **Queries** network address table
- **Resolves** short address to IEEE address

---

### 0x002D: ESP_NCP_NETWORK_IEEE_TO_SHORT

**Handler**: `esp_ncp_zb_address_short_by_ieee_get_fn()` (Lines 1693-1710)

**Purpose**: Get short address by IEEE address.

**Request Payload**:
```c
esp_zb_ieee_addr_t ieee_addr;  // 8 bytes
```

**Response Payload**:
```c
uint16_t short_addr;
```

**Low-Level Calls**:
- `esp_zb_address_short_by_ieee()` - Lookup short by IEEE address

---

### 0x002E: ESP_NCP_NETWORK_NVRAM_ERASE_AT_START_SET

**Handler**: `esp_ncp_zb_nvram_erase_at_start_set_fn()` (Lines 1666-1681)

**Purpose**: Enable/disable NVRAM erase at startup.

**Request Payload**:
```c
bool erase;
```

**Behavior**:
1. Updates `s_nvram_erase_at_start`
2. Applies to stack if initialized

**Low-Level Calls**:
- `esp_zb_nvram_erase_at_start()` - Set erase policy (if stack initialized)

**State Changes**:
- `s_nvram_erase_at_start = erase`

**Zigbee Bus Impact**:
- **Controls** whether `zb_storage` is cleared before loading parameters
- **Affects** network persistence across reboots

---

### 0x002F: ESP_NCP_NETWORK_NVRAM_ERASE_AT_START_GET

**Handler**: `esp_ncp_zb_nvram_erase_at_start_get_fn()` (Lines 1654-1664)

**Purpose**: Get NVRAM erase policy.

**Response Payload**:
```c
uint8_t erase;  // 1 = enabled, 0 = disabled
```

---

## ZCL Commands (0x0100-0x0108)

### 0x0100: ESP_NCP_ZCL_ENDPOINT_ADD

**Handler**: `esp_ncp_zb_add_endpoint_fn()` (Lines 1102-1179)

**Purpose**: Register an endpoint with clusters on the Zigbee device.

**Request Payload**:
```c
typedef struct {
    uint8_t endpoint;
    uint16_t profileId;
    uint16_t deviceId;
    uint8_t appFlags;
    uint8_t inputClusterCount;
    uint16_t inputClusterList[inputClusterCount];
    uint8_t outputClusterCount;
    uint16_t outputClusterList[outputClusterCount];
} esp_ncp_zb_endpoint_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Behavior**:
1. Validates payload length
2. Extracts cluster lists from payload
3. Creates cluster list using `cluster_list_fn_table` lookup
4. Adds clusters with appropriate roles (server/client)
5. Creates endpoint list and registers device

**Zigbee Bus Impact**:
- **Registers** endpoint with Zigbee stack
- **Exposes** clusters for ZCL communication
- **Enables** device to receive/send ZCL commands
- **Affects** device descriptor and discovery

**Low-Level Calls**:
- `esp_zb_zcl_cluster_list_create()` - Create cluster list
- `esp_zb_zcl_attr_list_create()` - Create attribute list for cluster
- Cluster-specific add functions (e.g., `esp_zb_cluster_list_add_on_off_cluster()`)
- `esp_zb_ep_list_create()` - Create endpoint list
- `esp_zb_ep_list_add_ep()` - Add endpoint to list
- `esp_zb_device_register()` - Register device with stack

**Cluster Function Table**: `cluster_list_fn_table[]` (Lines 1053-1100) maps cluster IDs to add functions for 47+ standard clusters.

---

### 0x0101: ESP_NCP_ZCL_ENDPOINT_DEL

**Handler**: `esp_ncp_zb_del_endpoint_fn()` (Lines 1181-1184)

**Purpose**: Remove endpoint (stub - no-op).

**Note**: Stub implementation - endpoint removal not supported.

---

### 0x0102: ESP_NCP_ZCL_ATTR_READ

**Handler**: `esp_ncp_zb_read_attr_fn()` (Lines 1186-1227)

**Purpose**: Read ZCL attributes from remote device.

**Request Payload**:
```c
typedef struct {
    esp_zb_zcl_basic_cmd_t zcl_basic_cmd;  // Destination, endpoints
    uint8_t address_mode;
    uint16_t cluster_id;
    uint8_t attr_number;
    uint16_t attr_id[attr_number];  // Attribute IDs to read
} esp_ncp_zb_read_attr_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Behavior**:
1. Extracts attribute IDs from payload
2. Allocates attribute field array
3. Calls `esp_zb_zcl_read_attr_cmd_req()`

**Zigbee Bus Impact**:
- **Sends** ZCL read attribute command
- **Transmits** APS frame to destination device
- **Requests** attribute values from remote cluster
- **Triggers** response via callback

**Low-Level Calls**:
- `calloc()` - Allocate attribute ID array
- `esp_zb_zcl_read_attr_cmd_req()` - Send read attribute command

**Notification**: Response sent via `esp_ncp_zb_read_attr_resp_handler()` → `ESP_NCP_ZCL_ATTR_READ` notification.

---

### 0x0103: ESP_NCP_ZCL_ATTR_WRITE

**Handler**: `esp_ncp_zb_write_attr_fn()` (Lines 1229-1309)

**Purpose**: Write ZCL attributes to remote device.

**Request Payload**:
```c
typedef struct {
    esp_zb_zcl_basic_cmd_t zcl_basic_cmd;
    uint8_t address_mode;
    uint16_t cluster_id;
    uint8_t attr_number;
    // Followed by attr_number * (attr_id + type + size + value)
} esp_ncp_zb_write_attr_t;
```

**Behavior**:
1. Parses attribute data from payload
2. Allocates attribute structures
3. Copies attribute values
4. Calls `esp_zb_zcl_write_attr_cmd_req()`
5. Frees temporary buffers

**Zigbee Bus Impact**:
- **Sends** ZCL write attribute command
- **Modifies** remote device attributes
- **Triggers** write response

**Low-Level Calls**:
- `calloc()` - Allocate attribute structures
- `esp_zb_zcl_write_attr_cmd_req()` - Send write attribute command

**Notification**: Response sent via `esp_ncp_zb_write_attr_resp_handler()` → `ESP_NCP_ZCL_ATTR_WRITE` notification.

---

### 0x0104: ESP_NCP_ZCL_ATTR_REPORT

**Handler**: `esp_ncp_zb_report_attr_fn()` (Lines 1311-1323)

**Purpose**: Send ZCL attribute report command.

**Request Payload**:
```c
esp_zb_zcl_report_attr_cmd_t;  // Forwarded directly to stack
```

**Low-Level Calls**:
- `esp_zb_zcl_report_attr_cmd_req()` - Send report command

**Zigbee Bus Impact**:
- **Sends** unsolicited attribute report
- **Notifies** remote device of attribute change

---

### 0x0105: ESP_NCP_ZCL_ATTR_DISC

**Handler**: `esp_ncp_zb_disc_attr_fn()` (Lines 1325-1337)

**Purpose**: Discover attributes on remote cluster.

**Request Payload**:
```c
esp_zb_zcl_disc_attr_cmd_t;  // Forwarded directly to stack
```

**Low-Level Calls**:
- `esp_zb_zcl_disc_attr_cmd_req()` - Send discover attributes command

**Zigbee Bus Impact**:
- **Queries** remote device for supported attributes
- **Discovers** attribute IDs and types

**Notification**: Response sent via `esp_ncp_zb_disc_attr_resp_handler()` → `ESP_NCP_ZCL_ATTR_DISC` notification.

---

### 0x0106: ESP_NCP_ZCL_READ

**Handler**: `esp_ncp_zb_zcl_read_fn()` (Lines 1339-1342)

**Purpose**: Generic ZCL read command (stub - no-op).

**Note**: Stub implementation.

---

### 0x0107: ESP_NCP_ZCL_WRITE

**Handler**: `esp_ncp_zb_zcl_write_fn()` (Lines 1344-1405)

**Purpose**: Send generic ZCL custom cluster command.

**Request Payload**:
```c
typedef struct {
    esp_zb_zcl_basic_cmd_t zcl_basic_cmd;
    uint8_t address_mode;
    uint16_t profile_id;
    uint16_t cluster_id;
    uint16_t custom_cmd_id;
    uint8_t direction;
    uint8_t type;
    uint16_t size;
    uint8_t payload[size];
} esp_ncp_zb_zcl_data_t;
```

**Behavior**:
1. Parses command structure
2. Handles array/structure types (prepends length)
3. Calls `esp_zb_zcl_custom_cluster_cmd_req()`

**Zigbee Bus Impact**:
- **Sends** custom ZCL command
- **Enables** vendor-specific or non-standard commands
- **Transmits** arbitrary payload

**Low-Level Calls**:
- `esp_zb_zcl_custom_cluster_cmd_req()` - Send custom command

---

### 0x0108: ESP_NCP_ZCL_REPORT_CONFIG

**Handler**: NULL (notification only)

**Purpose**: Configure attribute reporting (response sent via callback).

**Note**: Request handled by stack, response formatted by `esp_ncp_zb_report_configure_resp_handler()`.

---

## ZDO Commands (0x0200-0x0202)

### 0x0200: ESP_NCP_ZDO_BIND_SET

**Handler**: `esp_ncp_zb_set_bind_fn()` (Lines 995-1011)

**Purpose**: Create binding between two endpoints.

**Request Payload**:
```c
typedef struct {
    esp_zb_ieee_addr_t src_address;  // 8 bytes
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t dst_addr_mode;
    esp_zb_addr_u dst_address;
    uint8_t dst_endpoint;
    uint16_t req_dst_addr;
    esp_ncp_zb_user_cb_t user_cb;  // At end of payload
} esp_zb_zdo_bind_req_param_t;
```

**Behavior**:
1. Extracts user callback context from end of payload
2. Calls `esp_zb_zdo_device_bind_req()` with callback

**Zigbee Bus Impact**:
- **Sends** ZDO bind request
- **Creates** binding table entry
- **Enables** automatic message forwarding
- **Affects** group addressing and direct communication

**Low-Level Calls**:
- `calloc()` - Allocate user context
- `esp_zb_zdo_device_bind_req()` - Send bind request
  - Parameters: bind params, `esp_ncp_zb_bind_cb`, user context

**Notification**: Completion sent via `esp_ncp_zb_bind_cb()` → `ESP_NCP_ZDO_BIND_SET` notification.

---

### 0x0201: ESP_NCP_ZDO_UNBIND_SET

**Handler**: `esp_ncp_zb_set_unbind_fn()` (Lines 1013-1029)

**Purpose**: Remove binding between endpoints.

**Request Payload**: Same as bind (0x0200)

**Low-Level Calls**:
- `esp_zb_zdo_device_unbind_req()` - Send unbind request

**Notification**: Completion sent via `esp_ncp_zb_unbind_cb()` → `ESP_NCP_ZDO_UNBIND_SET` notification.

---

### 0x0202: ESP_NCP_ZDO_FIND_MATCH

**Handler**: `esp_ncp_zb_find_match_fn()` (Lines 1735-1774)

**Purpose**: Find devices matching descriptor (profile, clusters).

**Request Payload**:
```c
typedef struct {
    esp_ncp_zb_user_cb_t user_ctx;
    uint16_t dst_nwk_addr;
    uint16_t addr_of_interest;
    uint16_t profile_id;
    uint8_t num_in_clusters;
    uint8_t num_out_clusters;
    uint16_t cluster_list[num_in_clusters + num_out_clusters];
} esp_zb_zdo_match_desc_t;
```

**Behavior**:
1. Extracts match descriptor parameters
2. Extracts cluster list from payload
3. Calls `esp_zb_zdo_match_cluster()` with callback

**Zigbee Bus Impact**:
- **Sends** ZDO match descriptor request
- **Discovers** devices with matching profile/clusters
- **Enables** device discovery for application logic

**Low-Level Calls**:
- `calloc()` - Allocate user context
- `esp_zb_zdo_match_cluster()` - Send match descriptor request
  - Parameters: descriptor params, `esp_ncp_zb_find_match_cb`, user context

**Notification**: Results sent via `esp_ncp_zb_find_match_cb()` → `ESP_NCP_ZDO_FIND_MATCH` notification.

---

## APS Commands (0x0300-0x0302)

### 0x0300: ESP_NCP_APS_DATA_REQUEST

**Handler**: `esp_ncp_zb_aps_data_request_fn()` (Lines 1776-1829)

**Purpose**: Send raw APS data frame (low-level APS transmission).

**Request Payload**:
```c
typedef struct {
    esp_zb_zcl_basic_cmd_t basic_cmd;
    uint8_t dst_addr_mode;
    uint16_t profile_id;
    uint16_t cluster_id;
    uint8_t tx_options;
    bool use_alias;
    esp_zb_addr_u alias_src_addr;
    uint8_t alias_seq_num;
    uint8_t radius;
    uint32_t asdu_length;
    uint8_t asdu[asdu_length];
} esp_zb_aps_data_t;
```

**Behavior**:
1. Parses APS data request structure
2. Extracts ASDU from payload
3. Registers APS handlers (if not already registered)
4. Calls `esp_zb_aps_data_request()`

**Zigbee Bus Impact**:
- **Sends** APS frame directly
- **Bypasses** ZCL layer (raw APS)
- **Enables** custom protocols or non-ZCL communication
- **Transmits** over Zigbee network with specified addressing

**Low-Level Calls**:
- `esp_zb_aps_data_indication_handler_register()` - Register indication handler
- `esp_zb_aps_data_confirm_handler_register()` - Register confirm handler
- `esp_zb_aps_data_request()` - Send APS frame
  - Parameters: destination, endpoints, profile, cluster, ASDU, options

**Notifications**:
- Confirm: `ESP_NCP_APS_DATA_CONFIRM` via `esp_ncp_zb_aps_data_confirm_handler()`
- Indication: `ESP_NCP_APS_DATA_INDICATION` via `esp_ncp_zb_aps_data_indication_handler()` (if response received)

---

### 0x0301: ESP_NCP_APS_DATA_INDICATION

**Handler**: `esp_ncp_zb_aps_data_indication_fn()` (Lines 1831-1851)

**Purpose**: Poll for incoming APS data indication (queue-based).

**Request Payload**: None

**Response Payload**:
```c
// Either:
esp_ncp_zb_aps_data_ind_t;  // If data available in queue
// Or:
uint8_t ESP_NCP_CONNECTED;  // If queue empty/timeout
```

**Behavior**:
1. Creates queue if not exists
2. Waits up to 100ms for data from queue
3. Returns indication data if available
4. Returns `ESP_NCP_CONNECTED` if timeout

**Zigbee Bus Impact**: None directly - this is a polling mechanism for received frames.

**Low-Level Calls**:
- `xQueueCreate()` - Create FreeRTOS queue
- `xQueueReceive()` - Receive from queue (100ms timeout)

**Note**: Actual indications are queued by `esp_ncp_zb_aps_data_indication_handler()` callback.

---

### 0x0302: ESP_NCP_APS_DATA_CONFIRM

**Handler**: `esp_ncp_zb_aps_data_confirm_fn()` (Lines 1853-1873)

**Purpose**: Poll for APS data transmission confirmation (queue-based).

**Request Payload**: None

**Response Payload**:
```c
// Either:
esp_ncp_zb_aps_data_confirm_t;  // If confirm available in queue
// Or:
uint8_t ESP_NCP_CONNECTED;  // If queue empty/timeout
```

**Behavior**: Same as indication handler, but for confirmations.

**Low-Level Calls**:
- `xQueueCreate()` - Create FreeRTOS queue
- `xQueueReceive()` - Receive from queue (100ms timeout)

**Note**: Actual confirmations are queued by `esp_ncp_zb_aps_data_confirm_handler()` callback.

---

## Command Dispatch System

### Dispatch Function

#### `esp_ncp_zb_output()` (Lines 1938-1967)

**Purpose**: Main command dispatcher - routes incoming commands to handlers.

**Function**:
```c
esp_err_t esp_ncp_zb_output(esp_ncp_header_t *ncp_header, const void *buffer, uint16_t len)
```

**Behavior**:
1. Iterates through `ncp_zb_func_table[]`
2. Matches `ncp_header->id` to table entry
3. Calls handler function if exists
4. Sends response via `esp_ncp_resp_input()` if successful
5. Returns error if handler is NULL

**Low-Level Calls**:
- Handler functions (one per command)
- `esp_ncp_resp_input()` - Send response frame

---

### Command Function Table

**Location**: Lines 1875-1936

**Structure**:
```c
static const esp_ncp_zb_func_t ncp_zb_func_table[] = {
    {ESP_NCP_NETWORK_INIT, esp_ncp_zb_network_init_fn},
    {ESP_NCP_NETWORK_START, esp_ncp_zb_start_fn},
    // ... 60+ entries ...
    {ESP_NCP_APS_DATA_CONFIRM, esp_ncp_zb_aps_data_confirm_fn},
};
```

**Total Commands**: 61 entries

**NULL Handlers**: Commands with NULL handlers are notification-only:
- `ESP_NCP_NETWORK_JOINNETWORK` (0x0006)
- `ESP_NCP_NETWORK_LEAVENETWORK` (0x0007)
- `ESP_NCP_ZCL_REPORT_CONFIG` (0x0108)

---

## State Management

### Initialization States

1. **Platform Initialized** (`s_init_flag`):
   - Set by `ESP_NCP_NETWORK_INIT`
   - Required before stack operations

2. **Stack Started** (`s_start_flag`):
   - Set by `ESP_NCP_NETWORK_START`
   - Creates `esp_ncp_zb_task` FreeRTOS task

3. **Stack Initialized** (`s_zb_stack_inited`):
   - Set by `ESP_NCP_NETWORK_FORMNETWORK`
   - Enables deferred configuration application

### Deferred Configuration

Some settings are stored and applied later:

- **Channel Masks** (`s_channel_mask`, `s_primary_channel`):
  - Set before stack init → stored
  - Applied during `esp_ncp_zb_form_network_fn()` or `esp_ncp_apply_channel_config()`

- **Security Settings** (`s_tc_preconfigure_key`, `s_link_key_exchange_required`):
  - Set before stack init → stored
  - Applied during `esp_ncp_zb_form_network_fn()`

- **NVRAM Policy** (`s_nvram_erase_at_start`):
  - Set before stack init → stored
  - Applied during `esp_ncp_zb_form_network_fn()`

---

## Zigbee Bus Impact Analysis

### Command Categories by Bus Impact

#### High Impact (Network Formation/Joining)

- **ESP_NCP_NETWORK_INIT**: Initializes platform, no bus activity
- **ESP_NCP_NETWORK_FORMNETWORK**: Initializes stack, triggers formation
- **ESP_NCP_NETWORK_START**: Starts stack, begins commissioning
- **ESP_NCP_NETWORK_PERMIT_JOINING**: Broadcasts permit join beacons
- **ESP_NCP_NETWORK_START_SCAN**: Transmits beacon requests, receives responses

#### Medium Impact (Configuration)

- **Channel/PAN ID Settings**: Affect network formation parameters
- **Security Settings**: Affect encryption and key exchange
- **TX Power**: Affects range and interference

#### Low Impact (Queries)

- **GET Commands**: Read-only queries, no bus activity
- **State Queries**: Return cached or queried values

#### Data Transmission Impact

- **ZCL Commands**: Generate APS frames with ZCL headers
- **APS Data Request**: Raw APS frames, bypasses ZCL
- **ZDO Commands**: Generate ZDO frames for network management

### Frame Flow Examples

#### Example 1: Read Attribute

```
Application Host → NCP: ESP_NCP_ZCL_ATTR_READ (0x0102)
  ↓
esp_ncp_zb_read_attr_fn()
  ↓
esp_zb_zcl_read_attr_cmd_req()
  ↓
Zigbee Stack → APS → NWK → MAC → Radio
  ↓
[Frame transmitted over air]
  ↓
Remote Device → Response Frame
  ↓
Radio → MAC → NWK → APS → ZCL
  ↓
esp_ncp_zb_action_handler() → ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID
  ↓
esp_ncp_zb_read_attr_resp_handler()
  ↓
NCP → Application Host: ESP_NCP_ZCL_ATTR_READ notification
```

#### Example 2: Network Formation

```
Application Host → NCP: ESP_NCP_NETWORK_FORMNETWORK (0x0004)
  ↓
esp_ncp_zb_form_network_fn()
  ↓
esp_zb_init() → Sets s_zb_stack_inited = true
  ↓
[BDB signals triggered]
  ↓
esp_zb_app_signal_handler() → ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START
  ↓
esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION)
  ↓
[Stack selects channel, generates PAN ID]
  ↓
[Beacon frames transmitted]
  ↓
esp_zb_app_signal_handler() → ESP_ZB_BDB_SIGNAL_FORMATION
  ↓
NCP → Application Host: ESP_NCP_NETWORK_FORMNETWORK notification (PAN ID, channel)
```

#### Example 3: APS Data Transmission

```
Application Host → NCP: ESP_NCP_APS_DATA_REQUEST (0x0300)
  ↓
esp_ncp_zb_aps_data_request_fn()
  ↓
esp_zb_aps_data_request()
  ↓
[APS frame built with ASDU]
  ↓
[Frame routed through network]
  ↓
[Transmission completes]
  ↓
esp_ncp_zb_aps_data_confirm_handler() → Queue confirm
  ↓
Application Host → NCP: ESP_NCP_APS_DATA_CONFIRM (0x0302) [poll]
  ↓
[Confirm dequeued and returned]
```

---

## Summary

`esp_ncp_zb.c` serves as the critical bridge between the NCP protocol and the Zigbee stack. It:

1. **Translates** 60+ NCP commands to Zigbee stack API calls
2. **Formats** Zigbee stack responses/events into NCP notifications
3. **Manages** state and deferred configuration
4. **Handles** asynchronous events via callbacks and queues
5. **Provides** complete Zigbee network control from application host

### Key Design Decisions

- **Deferred Configuration**: Channel masks and security settings stored until stack init
- **Queue-Based APS**: Indications/confirms use queues for application host polling
- **Notification Pattern**: Stack events automatically sent to application host
- **Stub Handlers**: Some commands have minimal implementation (role, frame counter, etc.)

### Zigbee Bus Impact Summary

- **Network Formation**: Commands trigger beacon transmission and network creation
- **Device Joining**: Permit join commands broadcast beacons, device announcements sent as notifications
- **Data Transmission**: ZCL/APS commands generate frames transmitted over air
- **Configuration**: Channel, security, and power settings affect radio behavior
- **Discovery**: Scan and match descriptor commands query network for devices/networks

This file is essential for understanding how application host applications control Zigbee networks through the NCP firmware.

