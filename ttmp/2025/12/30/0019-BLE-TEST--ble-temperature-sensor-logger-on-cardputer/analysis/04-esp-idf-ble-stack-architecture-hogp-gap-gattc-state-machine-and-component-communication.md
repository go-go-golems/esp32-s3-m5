---
Title: ESP-IDF BLE Stack Architecture - HOGP, GAP, GATTC, State Machine, and Component Communication
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - cardputer
    - sensors
    - esp32s3
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Deep dive into ESP-IDF Bluedroid BLE stack architecture: HOGP vs GAP/GATTC, state machine location, and communication patterns between ble_host, hid_host, esp_console, esp_hidh, and esp_ble components.
LastUpdated: 2025-12-30T11:25:06.619233048-05:00
WhatFor: Understanding the layered architecture of ESP-IDF BLE stack and how components communicate for implementing custom BLE applications
WhenToUse: When debugging BLE issues, understanding stack interactions, or implementing custom profiles beyond HID
---

# ESP-IDF BLE Stack Architecture - HOGP, GAP, GATTC, State Machine, and Component Communication

## Executive Summary

This document provides a comprehensive analysis of the ESP-IDF Bluedroid BLE stack architecture, answering:
1. **What is HOGP vs GAP/GATTC?** - Profile layers vs protocol layers
2. **Where does the state machine live?** - Globally in Bluedroid host stack
3. **How do components communicate?** - Callback-driven event system with layered abstractions

**Key Finding**: ESP-IDF uses a **layered callback-driven architecture** where:
- **GAP/GATTC** are **protocol layers** (Bluedroid core)
- **HOGP** is a **profile layer** (built on top of GATT)
- **esp_hidd** is an **application abstraction** (HID device wrapper)
- **State machines** live **globally in Bluedroid host stack** (not per-component)
- **Communication** flows via **event callbacks** through the stack layers

---

## 1. HOGP vs GAP/GATTC: Understanding the Layers

### 1.1 GAP (Generic Access Profile) - Protocol Layer

**What it is**: GAP is the **foundational BLE protocol layer** that defines how devices:
- **Advertise** their presence
- **Scan** for other devices
- **Establish connections**
- **Manage security/pairing**
- **Handle disconnections**

**Location in stack**: **Bluedroid host stack** (`esp_gap_ble_api.h`)

**Key responsibilities**:
- Device discovery (advertising/scanning)
- Connection management (connect/disconnect)
- Security management (pairing, bonding, encryption)
- Connection parameter negotiation

**APIs**:
```c
// Register GAP callback (receives all GAP events)
esp_ble_gap_register_callback(ble_gap_event_handler);

// Configure advertising
esp_ble_gap_config_adv_data(&adv_data);
esp_ble_gap_start_advertising(&adv_params);

// Security/pairing
esp_ble_gap_set_security_param(...);
```

**Events handled**:
- `ESP_GAP_BLE_ADV_START_COMPLETE_EVT` - Advertising started
- `ESP_GAP_BLE_CONNECT_EVT` - Connection established
- `ESP_GAP_BLE_DISCONNECT_EVT` - Disconnected
- `ESP_GAP_BLE_AUTH_CMPL_EVT` - Authentication complete
- `ESP_GAP_BLE_SCAN_RESULT_EVT` - Scan result received

### 1.2 GATTC (GATT Client) - Protocol Layer

**What it is**: GATTC is the **GATT Client protocol layer** that:
- **Discovers** services and characteristics on a GATT server
- **Reads** characteristic values
- **Writes** characteristic values
- **Subscribes** to notifications/indications
- **Manages** service discovery

**Location in stack**: **Bluedroid host stack** (`esp_gattc_api.h`)

**Key responsibilities**:
- Service/characteristic discovery
- Read/write operations
- Notification/indication subscriptions
- MTU negotiation

**When used**: When your ESP32 acts as a **BLE Central** (client) connecting to other devices.

**APIs**:
```c
// Register GATTC callback
esp_ble_gattc_register_callback(gattc_event_handler);

// Connect to server
esp_ble_gattc_open(gattc_if, remote_bda, BLE_ADDR_TYPE_PUBLIC, true);

// Discover services
esp_ble_gattc_search_service(gattc_if, conn_id, NULL);

// Read characteristic
esp_ble_gattc_read_char(gattc_if, conn_id, char_handle, ESP_GATT_AUTH_REQ_NONE);

// Write characteristic
esp_ble_gattc_write_char(gattc_if, conn_id, char_handle, len, value, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
```

### 1.3 GATTS (GATT Server) - Protocol Layer

**What it is**: GATTS is the **GATT Server protocol layer** that:
- **Exposes** services and characteristics
- **Handles** read/write requests from clients
- **Sends** notifications/indications to clients
- **Manages** attribute database

**Location in stack**: **Bluedroid host stack** (`esp_gatts_api.h`)

**When used**: When your ESP32 acts as a **BLE Peripheral** (server) exposing data/services.

**APIs**:
```c
// Register GATTS callback
esp_ble_gatts_register_callback(gatts_event_handler);

// Create service
esp_ble_gatts_create_service(gatts_if, &service_uuid, handle_num, &service_handle);

// Add characteristic
esp_ble_gatts_add_char(service_handle, &char_uuid, ...);

// Send notification
esp_ble_gatts_send_indicate(gatts_if, conn_id, char_handle, len, value, false);
```

### 1.4 HOGP (HID Over GATT Profile) - Profile Layer

**What it is**: HOGP is a **BLE profile** (application layer specification) that defines:
- How **HID devices** (keyboards, mice, game controllers) communicate over BLE
- **Standard GATT service structure** for HID devices
- **Report maps** (descriptors of HID data formats)
- **Protocol modes** (boot vs report mode)

**Location in stack**: **Application layer** (built on top of GATTS)

**Key characteristics**:
- Uses **standard HID service UUID** (`0x1812`)
- Defines **standard characteristics**:
  - HID Information (`0x2A4A`)
  - Report Map (`0x2A4B`)
  - HID Control Point (`0x2A4C`)
  - Report characteristics (Input/Output/Feature)
- Uses **CCCD** (Client Characteristic Configuration Descriptor) for notifications

**Relationship to GATTS**: HOGP is **implemented using GATTS**. The HOGP profile defines **what** GATT services/characteristics to expose, and GATTS provides the **how** (the protocol implementation).

**In ESP-IDF**: The `esp_hidd` component implements HOGP by:
1. Creating GATT services/characteristics using GATTS APIs
2. Handling HID-specific protocol (report maps, protocol modes)
3. Providing a simplified API (`esp_hidd_dev_input_set`) that abstracts GATTS details

### 1.5 Comparison Table

| Layer | Type | Purpose | Location | APIs |
|-------|------|---------|----------|------|
| **GAP** | Protocol | Device discovery, connection, security | Bluedroid host | `esp_gap_ble_api.h` |
| **GATTC** | Protocol | Client-side GATT operations | Bluedroid host | `esp_gattc_api.h` |
| **GATTS** | Protocol | Server-side GATT operations | Bluedroid host | `esp_gatts_api.h` |
| **HOGP** | Profile | HID device standard (uses GATTS) | Application layer | `esp_hidd.h` (wrapper) |

**Key Insight**: GAP/GATTC/GATTS are **protocol layers** (part of Bluedroid core). HOGP is a **profile layer** (application specification built on GATTS).

---

## 2. Where Does the State Machine Live?

### 2.1 Global State Machine in Bluedroid Host Stack

**Answer**: The BLE state machine lives **globally in the Bluedroid host stack**, not per-component.

**Architecture**:

```
┌─────────────────────────────────────────┐
│     Application Layer                    │
│  (esp_hidd, custom GATT apps)            │
└──────────────┬──────────────────────────┘
               │ Callbacks
┌──────────────▼──────────────────────────┐
│     Bluedroid Host Stack                 │
│  ┌──────────────────────────────────┐   │
│  │  GAP State Machine (GLOBAL)       │   │
│  │  - Advertising state              │   │
│  │  - Connection state               │   │
│  │  - Security/pairing state         │   │
│  └──────────────────────────────────┘   │
│  ┌──────────────────────────────────┐   │
│  │  GATT State Machine (GLOBAL)      │   │
│  │  - Service discovery state        │   │
│  │  - Characteristic access state    │   │
│  │  - Notification state             │   │
│  └──────────────────────────────────┘   │
└──────────────┬──────────────────────────┘
               │ HCI
┌──────────────▼──────────────────────────┐
│     Bluetooth Controller                 │
│  (Hardware, Link Layer)                  │
└──────────────────────────────────────────┘
```

### 2.2 State Machine Scope

**Global states** (managed by Bluedroid):
- **Controller state**: Uninitialized → Initialized → Enabled
- **GAP state**: Idle → Advertising → Connected → Disconnected
- **GATT state**: Idle → Service Discovery → Ready → Notifying
- **Security state**: Unpaired → Pairing → Bonded

**Per-connection states** (tracked by application):
- Connection ID (`conn_id`)
- Connection handle (`conn_handle`)
- Service/characteristic handles
- Notification subscription status

**Example from codebase** (`ble_keyboard_wrap.cpp`):
```cpp
// Application-level state (not Bluedroid state machine)
static BleKbWrapState_t _keyboard_current_state = ble_kb_wrap_state_wait_connect;

// Bluedroid manages connection state internally
// Application tracks it via callbacks:
case ESP_HIDD_CONNECT_EVENT:
    _keyboard_current_state = ble_kb_wrap_state_connected;  // App state
    // Bluedroid already transitioned to "connected" internally
    break;
```

### 2.3 State Machine Transitions

**GAP State Transitions** (managed by Bluedroid):
```
[Idle]
  │ esp_ble_gap_start_advertising()
  ▼
[Advertising]
  │ Peer connects
  ▼
[Connected]
  │ esp_ble_gap_disconnect() or peer disconnects
  ▼
[Disconnected]
  │ (auto-restart advertising if configured)
  ▼
[Advertising]
```

**GATT State Transitions** (managed by Bluedroid):
```
[Idle]
  │ esp_ble_gatts_create_service()
  ▼
[Service Created]
  │ esp_ble_gatts_start_service()
  ▼
[Service Active]
  │ Client connects + discovers
  ▼
[Ready for Operations]
  │ Client subscribes to notifications
  ▼
[Notifying]
```

**Application observes** these transitions via **callbacks**, but **does not control** the state machine directly.

### 2.4 State Machine Location in Code

**Bluedroid host stack** (closed source, provided by Espressif):
- Lives in ESP-IDF components: `components/bt/host/bluedroid/`
- State machines are **internal to Bluedroid**
- Application interacts via **APIs and callbacks**

**Application state tracking** (your code):
- Track connection status, subscription status, etc.
- Use callbacks to synchronize with Bluedroid state

**Example** (`esp_hid_gap.c`):
```c
// Application tracks scan results (not Bluedroid state)
static esp_hid_scan_result_t *ble_scan_results = NULL;

// Bluedroid manages scan state internally
// Application observes via callback:
case ESP_GAP_BLE_SCAN_RESULT_EVT:
    // Bluedroid is in "scanning" state
    // Application processes result
    add_ble_scan_result(...);
    break;
```

---

## 3. Component Communication Architecture

### 3.1 Component Overview

**Key components**:
1. **ble_host** - Bluedroid BLE host stack (GAP/GATT protocols)
2. **hid_host** - HID host abstraction (for HID client functionality)
3. **esp_console** - ESP-IDF console subsystem (UART/USB serial)
4. **esp_hidh** - HID host APIs (client-side HID)
5. **esp_hidd** - HID device APIs (server-side HID, implements HOGP)
6. **esp_ble** - BLE APIs (GAP/GATTC/GATTS)

**Note**: In the codebase, we see:
- `esp_hidd` (HID device/server) - used in `ble_keyboard_wrap.cpp`
- `esp_hid_gap` - wrapper around GAP for HID use cases
- No direct `esp_hidh` usage (that's for HID client/host)

### 3.2 Communication Flow: HID Device (HOGP) Example

**Architecture diagram**:

```
┌─────────────────────────────────────────────────────────┐
│  Application (ble_keyboard_wrap.cpp)                    │
│  - ble_keyboard_wrap_init()                             │
│  - ble_keyboard_wrap_update_input()                     │
│  - State: _keyboard_current_state                       │
└──────────────┬──────────────────────────────────────────┘
               │ Calls
┌──────────────▼──────────────────────────────────────────┐
│  esp_hidd (HID Device Abstraction)                       │
│  - esp_hidd_dev_init()                                   │
│  - esp_hidd_dev_input_set()                             │
│  - Events: ESP_HIDD_START_EVENT, ESP_HIDD_CONNECT_EVENT│
└──────────────┬──────────────────────────────────────────┘
               │ Uses GATTS APIs
┌──────────────▼──────────────────────────────────────────┐
│  esp_hid_gap (GAP Wrapper for HID)                      │
│  - esp_hid_gap_init()                                    │
│  - esp_hid_ble_gap_adv_init()                           │
│  - esp_hid_ble_gap_adv_start()                          │
│  - Registers: ble_gap_event_handler()                   │
└──────────────┬──────────────────────────────────────────┘
               │ Calls
┌──────────────▼──────────────────────────────────────────┐
│  esp_ble (GAP/GATTS APIs)                                │
│  - esp_ble_gap_register_callback()                      │
│  - esp_ble_gap_config_adv_data()                        │
│  - esp_ble_gap_start_advertising()                       │
│  - esp_ble_gatts_register_callback()                    │
│  - esp_ble_gatts_create_service()                       │
└──────────────┬──────────────────────────────────────────┘
               │ HCI Commands/Events
┌──────────────▼──────────────────────────────────────────┐
│  Bluedroid Host Stack (ble_host)                         │
│  - GAP state machine                                     │
│  - GATT state machine                                    │
│  - HCI layer                                            │
└──────────────┬──────────────────────────────────────────┘
               │ HCI
┌──────────────▼──────────────────────────────────────────┐
│  Bluetooth Controller (Hardware)                         │
│  - Radio management                                      │
│  - Link layer                                           │
└─────────────────────────────────────────────────────────┘
```

### 3.3 Communication Pattern: Callback-Driven Events

**Key principle**: **All communication is callback-driven**. Components don't call each other directly; they register callbacks and receive events.

**Initialization flow** (`ble_keyboard_wrap_init`):

```c
// 1. Initialize Bluedroid stack (low-level)
esp_hid_gap_init(HID_DEV_MODE);
  └─> init_low_level()
      ├─> esp_bt_controller_init()
      ├─> esp_bt_controller_enable()
      ├─> esp_bluedroid_init()
      ├─> esp_bluedroid_enable()
      └─> init_ble_gap()
          └─> esp_ble_gap_register_callback(ble_gap_event_handler)

// 2. Configure advertising (GAP layer)
esp_hid_ble_gap_adv_init(...);
  └─> esp_ble_gap_config_adv_data(...)
  └─> esp_ble_gap_set_security_param(...)

// 3. Register GATTS callback
esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler);

// 4. Initialize HID device (creates GATT services)
esp_hidd_dev_init(&ble_hid_config, ESP_HID_TRANSPORT_BLE, ...);
  └─> (internally calls GATTS APIs to create HID service)
  └─> (registers event handler for HID events)
```

**Event flow** (when client connects):

```
1. Bluetooth Controller detects connection
   └─> Sends HCI event to Bluedroid

2. Bluedroid GAP state machine transitions to "connected"
   └─> Calls registered callback: ble_gap_event_handler()
       └─> Event: ESP_GAP_BLE_CONNECT_EVT

3. Bluedroid GATTS processes connection
   └─> Calls registered callback: esp_hidd_gatts_event_handler()
       └─> Event: ESP_GATTS_CONNECT_EVT
       └─> (esp_hidd internally processes this)

4. esp_hidd processes GATTS event
   └─> Calls application callback: ble_hidd_event_callback()
       └─> Event: ESP_HIDD_CONNECT_EVENT
       └─> Application updates: _keyboard_current_state = connected

5. Application can now send HID reports
   └─> ble_keyboard_wrap_update_input()
       └─> esp_hidd_dev_input_set()
           └─> (internally calls esp_ble_gatts_send_indicate())
               └─> Bluedroid sends notification to client
```

### 3.4 Component Responsibilities

#### esp_ble (GAP/GATTS APIs)

**Role**: **Direct interface to Bluedroid host stack**

**Responsibilities**:
- Register callbacks for GAP/GATTS events
- Configure advertising parameters
- Create GATT services/characteristics
- Send notifications/indications
- Manage security/pairing

**APIs**:
- `esp_ble_gap_register_callback()` - Register GAP callback
- `esp_ble_gap_config_adv_data()` - Configure advertising
- `esp_ble_gap_start_advertising()` - Start advertising
- `esp_ble_gatts_register_callback()` - Register GATTS callback
- `esp_ble_gatts_create_service()` - Create GATT service
- `esp_ble_gatts_send_indicate()` - Send notification

**Communication**: **Direct HCI interface** with Bluetooth controller

#### esp_hid_gap (GAP Wrapper)

**Role**: **Simplified GAP interface for HID use cases**

**Responsibilities**:
- Initialize Bluedroid stack
- Configure HID-specific advertising (HID service UUID)
- Handle GAP events (connect/disconnect/security)
- Provide HID-specific GAP helpers

**APIs**:
- `esp_hid_gap_init()` - Initialize stack + register GAP callback
- `esp_hid_ble_gap_adv_init()` - Configure HID advertising
- `esp_hid_ble_gap_adv_start()` - Start advertising

**Communication**: **Calls esp_ble APIs**, registers GAP callback

**Code location**: `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/esp_hid_gap.c`

#### esp_hidd (HID Device)

**Role**: **HOGP implementation wrapper**

**Responsibilities**:
- Create HID GATT service (using GATTS APIs)
- Manage HID report maps
- Handle HID protocol modes (boot vs report)
- Provide simplified API for sending HID reports
- Translate HID events to application callbacks

**APIs**:
- `esp_hidd_dev_init()` - Initialize HID device, create GATT service
- `esp_hidd_dev_input_set()` - Send HID input report
- `esp_hidd_dev_deinit()` - Cleanup

**Communication**: **Calls esp_ble GATTS APIs**, registers GATTS callback

**Code location**: ESP-IDF component (not in our codebase, but used via `esp_hidd.h`)

#### Application Layer (ble_keyboard_wrap)

**Role**: **Application-specific logic**

**Responsibilities**:
- Initialize HID device
- Track application state (`_keyboard_current_state`)
- Process keyboard input
- Send HID reports when keys pressed

**APIs**:
- `ble_keyboard_wrap_init()` - Initialize BLE HID keyboard
- `ble_keyboard_wrap_update_input()` - Send keyboard report
- `ble_keyboard_wrap_get_current_state()` - Get connection state

**Communication**: **Calls esp_hidd APIs**, receives HID events via callback

**Code location**: `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/ble_keyboard_wrap.cpp`

### 3.5 esp_console (Not Directly Related to BLE)

**Role**: **ESP-IDF console subsystem** (UART/USB serial)

**Responsibilities**:
- UART/USB serial communication
- Command-line interface
- Logging output

**Relationship to BLE**: **Independent**. Console is for **host communication via serial**, BLE is for **wireless communication**. They don't interact directly.

**Note**: `esp_console` is **not** part of the BLE stack. It's a separate subsystem for serial I/O.

### 3.6 Communication Summary Table

| Component | Role | Communicates With | Method |
|-----------|------|-------------------|--------|
| **Application** | App logic | `esp_hidd` | Direct API calls + callbacks |
| **esp_hidd** | HOGP wrapper | `esp_ble` (GATTS) | Direct API calls + callbacks |
| **esp_hid_gap** | GAP wrapper | `esp_ble` (GAP) | Direct API calls + callbacks |
| **esp_ble** | BLE APIs | Bluedroid host | Direct API calls + callbacks |
| **Bluedroid host** | Protocol stack | Bluetooth controller | HCI commands/events |
| **Controller** | Hardware | Radio | Hardware interface |

**Key pattern**: **Each layer calls APIs of the layer below, and receives events via callbacks from the layer below**.

---

## 4. Detailed Communication Examples

### 4.1 Example 1: Sending HID Report (Keyboard Input)

**Flow**:

```c
// Application layer
ble_keyboard_wrap_update_input(buffer);
  │
  ├─> Calls esp_hidd API
  │
  ▼
// esp_hidd layer
esp_hidd_dev_input_set(hid_dev, conn_id, report_id, buffer, len);
  │
  ├─> Validates parameters
  ├─> Finds characteristic handle for report_id
  ├─> Calls esp_ble GATTS API
  │
  ▼
// esp_ble GATTS layer
esp_ble_gatts_send_indicate(gatts_if, conn_id, char_handle, len, buffer, false);
  │
  ├─> Validates connection state
  ├─> Queues notification
  ├─> Sends HCI command to controller
  │
  ▼
// Bluedroid host stack
  │
  ├─> Processes HCI command
  ├─> Sends BLE packet to peer device
  ├─> Receives confirmation
  ├─> Calls callback: ESP_GATTS_CONF_EVT
  │
  ▼
// Application receives confirmation (if registered)
gatts_event_handler(ESP_GATTS_CONF_EVT, ...);
```

### 4.2 Example 2: Client Connects

**Flow**:

```
1. Peer device initiates connection
   │
   ▼
2. Bluetooth Controller receives connection request
   │
   ├─> Sends HCI event: LE Connection Complete
   │
   ▼
3. Bluedroid GAP processes event
   │
   ├─> Updates GAP state machine: Idle → Connected
   ├─> Calls registered callback
   │
   ▼
4. esp_hid_gap receives GAP callback
   ble_gap_event_handler(ESP_GAP_BLE_CONNECT_EVT, ...)
   │
   ├─> Logs connection
   ├─> (Doesn't notify application directly)
   │
   ▼
5. Bluedroid GATTS processes connection
   │
   ├─> Updates GATT state machine
   ├─> Calls registered callback
   │
   ▼
6. esp_hidd receives GATTS callback
   esp_hidd_gatts_event_handler(ESP_GATTS_CONNECT_EVT, ...)
   │
   ├─> Processes connection internally
   ├─> Calls application callback
   │
   ▼
7. Application receives HID event
   ble_hidd_event_callback(ESP_HIDD_CONNECT_EVENT, ...)
   │
   ├─> Updates application state
   ├─> _keyboard_current_state = connected
   └─> (Application can now send reports)
```

### 4.3 Example 3: Client Subscribes to Notifications

**Flow**:

```
1. Client writes to CCCD (Client Characteristic Configuration Descriptor)
   │
   ▼
2. Bluetooth Controller receives write
   │
   ├─> Sends HCI event: Attribute Write
   │
   ▼
3. Bluedroid GATTS processes write
   │
   ├─> Validates handle (is it CCCD?)
   ├─> Updates subscription state
   ├─> Calls registered callback
   │
   ▼
4. esp_hidd receives GATTS callback
   esp_hidd_gatts_event_handler(ESP_GATTS_WRITE_EVT, ...)
   │
   ├─> Checks if handle is CCCD
   ├─> Updates notification enabled state
   ├─> (esp_hidd_dev_input_set will now send notifications)
   │
   ▼
5. Application doesn't receive direct callback
   │
   └─> (But can check notification state via esp_hidd APIs if needed)
```

---

## 5. State Machine Details

### 5.1 GAP State Machine (Bluedroid Internal)

**States** (simplified):
- **IDLE** - Not advertising, not connected
- **ADVERTISING** - Broadcasting presence
- **CONNECTED** - Link established
- **DISCONNECTED** - Link terminated

**Transitions**:
```
IDLE
  └─> esp_ble_gap_start_advertising()
      └─> ADVERTISING
          └─> Peer connects
              └─> CONNECTED
                  └─> Disconnect (peer or local)
                      └─> DISCONNECTED
                          └─> (Auto-restart advertising if configured)
                              └─> ADVERTISING
```

**Application observes** via `ESP_GAP_BLE_CONNECT_EVT` / `ESP_GAP_BLE_DISCONNECT_EVT`.

### 5.2 GATT State Machine (Bluedroid Internal)

**States** (simplified):
- **IDLE** - No service created
- **SERVICE_CREATED** - Service created but not started
- **SERVICE_ACTIVE** - Service started, ready for connections
- **CONNECTED** - Client connected
- **DISCOVERING** - Client discovering services
- **READY** - Ready for read/write operations
- **NOTIFYING** - Sending notifications

**Transitions**:
```
IDLE
  └─> esp_ble_gatts_create_service()
      └─> SERVICE_CREATED
          └─> esp_ble_gatts_start_service()
              └─> SERVICE_ACTIVE
                  └─> Client connects
                      └─> CONNECTED
                          └─> Client discovers services
                              └─> DISCOVERING
                                  └─> Discovery complete
                                      └─> READY
                                          └─> Client subscribes
                                              └─> NOTIFYING
```

**Application observes** via `ESP_GATTS_CONNECT_EVT`, `ESP_GATTS_READ_EVT`, `ESP_GATTS_WRITE_EVT`, etc.

### 5.3 Application State Tracking

**Application maintains its own state** (separate from Bluedroid):

**Example** (`ble_keyboard_wrap.cpp`):
```c
enum BleKbWrapState_t {
    ble_kb_wrap_state_wait_connect = 0,
    ble_kb_wrap_state_connected,
};

static BleKbWrapState_t _keyboard_current_state = ble_kb_wrap_state_wait_connect;
```

**Why separate state?**
- Bluedroid state is **low-level** (connection, GATT operations)
- Application state is **high-level** (business logic: "can I send keyboard input?")
- Application state **synchronizes** with Bluedroid state via callbacks

**Synchronization**:
```c
// Bluedroid transitions to connected
case ESP_HIDD_CONNECT_EVENT:
    _keyboard_current_state = ble_kb_wrap_state_connected;  // Sync app state
    break;

// Bluedroid transitions to disconnected
case ESP_HIDD_DISCONNECT_EVENT:
    _keyboard_current_state = ble_kb_wrap_state_wait_connect;  // Sync app state
    break;
```

---

## 6. Key Takeaways

### 6.1 Architecture Principles

1. **Layered abstraction**: Each layer provides a simpler API than the layer below
2. **Callback-driven**: All communication is event-based via callbacks
3. **State machines are global**: Bluedroid manages state internally
4. **Application tracks high-level state**: Sync with Bluedroid via callbacks

### 6.2 Component Roles

- **GAP/GATTC/GATTS**: **Protocol layers** (Bluedroid core)
- **HOGP**: **Profile layer** (application specification)
- **esp_hidd**: **Profile implementation** (HOGP wrapper)
- **esp_hid_gap**: **Convenience wrapper** (GAP for HID)
- **Application**: **Business logic** (your code)

### 6.3 Communication Pattern

**Always**: Application → API call → Lower layer → Callback → Application

**Never**: Direct component-to-component calls (except via APIs)

### 6.4 State Management

- **Bluedroid**: Manages low-level state (connection, GATT operations)
- **Application**: Manages high-level state (business logic)
- **Synchronization**: Via callbacks

---

## 7. Related Files

- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/ble_keyboard_wrap.cpp` - Application layer using esp_hidd
- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/esp_hid_gap.c` - GAP wrapper implementation
- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/esp_hid_gap.h` - GAP wrapper API

## 8. External References

- [ESP-IDF Bluedroid Architecture](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/bluetooth/architecture.html)
- [ESP-IDF GAP Guide](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/bluetooth/esp-ble-gap.html)
- [ESP-IDF GATT Guide](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/bluetooth/esp-gatt.html)
- [HOGP Specification](https://www.bluetooth.com/specifications/specs/hid-over-gatt-profile-1-0/) (Bluetooth SIG)

---

## 9. Summary

**HOGP vs GAP/GATTC**:
- **GAP/GATTC/GATTS** are **protocol layers** (Bluedroid core)
- **HOGP** is a **profile layer** (application specification built on GATTS)

**State Machine Location**:
- Lives **globally in Bluedroid host stack**
- Application observes state via **callbacks**
- Application maintains **separate high-level state**

**Component Communication**:
- **Callback-driven event system**
- **Layered abstraction**: Application → esp_hidd → esp_ble → Bluedroid → Controller
- **No direct component-to-component calls** (except via APIs)

This architecture provides **separation of concerns**, **modularity**, and **clear interfaces** between layers, making it easier to implement custom BLE applications while leveraging the robust Bluedroid stack.
