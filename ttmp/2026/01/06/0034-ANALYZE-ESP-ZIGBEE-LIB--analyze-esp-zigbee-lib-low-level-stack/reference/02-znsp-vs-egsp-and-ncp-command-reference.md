---
Title: ZNSP vs EGSP and Complete NCP Command Reference
Ticket: 0034-ANALYZE-ESP-ZIGBEE-LIB
Status: active
Topics:
    - zigbee
    - znsp
    - egsp
    - ncp
    - protocol
    - commands
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_zb.h
      Note: Complete NCP command ID definitions
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_frame.h
      Note: Frame header structure (EGSP)
    - Path: ../../../../../../../../0031-zigbee-orchestrator/components/zb_host/src/priv/esp_host_zb.h
      Note: Host-side protocol definitions (ZNSP)
    - Path: ../../../../../../../../thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Command handler implementations and payload structures
ExternalSources: []
Summary: Explanation of ZNSP vs EGSP naming and complete reference of all NCP commands with payload structures
LastUpdated: 2026-01-06T14:00:00.000000000-05:00
WhatFor: Understanding protocol naming conventions and complete NCP command reference for implementation
WhenToUse: When implementing host-NCP communication or debugging protocol issues
---

# ZNSP vs EGSP and Complete NCP Command Reference

## ZNSP vs EGSP: What's the Difference?

**Short Answer**: They're the **same protocol**, just different naming conventions used on different sides of the communication.

### ZNSP (Zigbee NCP Serial Protocol)

- **Used on**: **Host side** (ESP32-S3, application processor)
- **Naming convention**: `ESP_ZNSP_*` defines
- **Location**: `components/zb_host/src/priv/esp_host_zb.h`
- **Frame structure**: `esp_host_header_t` (same format as NCP side)

### EGSP (Espressif Zigbee Serial Protocol)

- **Used on**: **NCP side** (ESP32-H2, network co-processor)
- **Naming convention**: `ESP_NCP_*` defines, frame header mentions "EGSP protocol version"
- **Location**: `components/esp-zigbee-ncp/src/priv/esp_ncp_frame.h`
- **Frame structure**: `esp_ncp_header_t` (same format as host side)

### Frame Format (Same on Both Sides)

```c
typedef struct {
    struct {
        uint16_t version:4;      // EGSP protocol version (on NCP side)
        uint16_t type:4;         // Request/Response/Notify
        uint16_t reserved:8;
    } flags;
    uint16_t id;                 // Frame ID (command/event ID)
    uint8_t  sn;                 // Sequence number
    uint16_t len;                // Payload length
} __attribute__((packed)) esp_ncp_header_t;  // or esp_host_header_t
```

### Why Two Names?

- **ZNSP**: Host-side naming emphasizes "Zigbee NCP" perspective
- **EGSP**: NCP-side naming emphasizes "Espressif Zigbee" branding
- **Reality**: Same wire protocol, same frame format, same command IDs

### Protocol Flow

```
┌─────────────────┐                    ┌─────────────────┐
│   Host (S3)     │                    │    NCP (H2)     │
│                 │                    │                 │
│  ZNSP Protocol  │◄─── UART/SLIP ───►│  EGSP Protocol  │
│  (ESP_ZNSP_*)   │                    │  (ESP_NCP_*)    │
└─────────────────┘                    └─────────────────┘
```

**Key Point**: The command IDs are identical on both sides. `ESP_ZNSP_NETWORK_INIT` (host) = `ESP_NCP_NETWORK_INIT` (NCP) = `0x0000`.

---

## Complete NCP Command Reference

### Command ID Ranges

- **Network Commands**: `0x0000` - `0x002F`
- **ZCL Commands**: `0x0100` - `0x0108`
- **ZDO Commands**: `0x0200` - `0x0202`
- **APS Commands**: `0x0300` - `0x0302`

### Frame Types

```c
typedef enum {
    ESP_ZNSP_TYPE_REQUEST = 0,   // Host → NCP command
    ESP_ZNSP_TYPE_RSPONSE = 1,   // NCP → Host response
    ESP_ZNSP_TYPE_NOTIFY  = 2,   // NCP → Host notification (async event)
} esp_znsp_type_t;
```

### Status Codes

```c
typedef enum {
    ESP_ZNSP_SUCCESS = 0x00,         // Success
    ESP_ZNSP_ERR_FATAL = 0x01,       // Fatal error
    ESP_ZNSP_BAD_ARGUMENT = 0x02,    // Invalid argument
    ESP_ZNSP_ERR_NO_MEM = 0x03,      // Out of memory
} esp_znsp_status_t;
```

---

## Network Commands (0x0000 - 0x002F)

### 0x0000: ESP_NCP_NETWORK_INIT

**Description**: Resume network operation after a reboot

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
uint8_t status;  // ESP_NCP_SUCCESS or error code
```

**Handler**: `esp_ncp_zb_network_init_fn()` → calls `esp_zb_init()`

---

### 0x0001: ESP_NCP_NETWORK_START

**Description**: Start the commissioning process

**Request Payload**:
```c
bool autostart;  // true = autostart, false = no-autostart
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_start_fn()` → calls `esp_zb_start()`

---

### 0x0002: ESP_NCP_NETWORK_STATE

**Description**: Returns network state (joining/joined/leaving)

**Request Payload**: None

**Response Payload**:
```c
uint8_t state;  // ESP_NCP_DISCONNECTED, ESP_NCP_CONNECTED, etc.
```

**Handler**: `esp_ncp_zb_network_state_fn()`

---

### 0x0003: ESP_NCP_NETWORK_STACK_STATUS_HANDLER

**Description**: Notify when stack status changes (notification only, no request)

**Notification Payload**:
```c
uint8_t status;  // Stack status code
```

**Handler**: `esp_ncp_zb_stack_status_fn()`

---

### 0x0004: ESP_NCP_NETWORK_FORMNETWORK

**Description**: Forms a new network by becoming the coordinator

**Request Payload**: None (uses previously configured parameters)

**Response Payload**:
```c
uint8_t status;
```

**Notification Payload** (on success):
```c
typedef struct {
    esp_zb_ieee_addr_t extendedPanId;  // 8 bytes
    uint16_t panId;
    uint8_t radioChannel;
} esp_ncp_zb_formnetwork_parameters_t;
```

**Handler**: `esp_ncp_zb_form_network_fn()` → calls `esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION)`

---

### 0x0005: ESP_NCP_NETWORK_PERMIT_JOINING

**Description**: Allow other nodes to join the network

**Request Payload**:
```c
uint8_t permit_duration;  // Seconds (0x00 = disable, 0xFF = enable indefinitely)
```

**Response Payload**:
```c
uint8_t status;
```

**Notification Payload** (status change):
```c
uint8_t permit_duration;  // Current permit join duration
```

**Handler**: `esp_ncp_zb_permit_joining_fn()` → calls `esp_zb_bdb_open_network()`

---

### 0x0006: ESP_NCP_NETWORK_JOINNETWORK

**Description**: Associate with network using specified parameters (notification only)

**Notification Payload**:
```c
typedef struct {
    uint16_t device_short_addr;
    esp_zb_ieee_addr_t ieee_addr;  // 8 bytes
    uint8_t capability;
} esp_zb_zdo_signal_device_annce_params_t;
```

**Handler**: Sent via `esp_zb_app_signal_handler()` when `ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE` occurs

---

### 0x0007: ESP_NCP_NETWORK_LEAVENETWORK

**Description**: Causes stack to leave current network (notification only)

**Notification Payload**:
```c
typedef struct {
    uint16_t short_addr;
    esp_zb_ieee_addr_t device_addr;  // 8 bytes
    uint8_t rejoin;
} esp_zb_zdo_signal_leave_indication_params_t;
```

**Handler**: Sent via `esp_zb_app_signal_handler()` when `ESP_ZB_ZDO_SIGNAL_LEAVE` occurs

---

### 0x0008: ESP_NCP_NETWORK_START_SCAN

**Description**: Active scan for available networks

**Request Payload**:
```c
typedef struct {
    uint32_t channel_mask;     // Channels to scan (0x00000800 - 0x07FFF800)
    uint8_t scan_duration;     // Scan duration
} esp_ncp_zb_scan_parameters_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_start_scan_fn()` → calls `esp_zb_zdo_active_scan_request()`

---

### 0x0009: ESP_NCP_NETWORK_SCAN_COMPLETE_HANDLER

**Description**: Signals that scan has completed (notification only)

**Notification Payload**:
```c
typedef struct {
    esp_zb_zdp_status_t zdo_status;
    uint8_t count;
    // Followed by count * sizeof(esp_zb_network_descriptor_t)
} esp_ncp_zb_scan_parameters_t;

typedef struct {
    uint16_t short_pan_id;
    bool permit_joining;
    esp_zb_ieee_addr_t extended_pan_id;  // 8 bytes
    uint8_t logic_channel;
    bool router_capacity;
    bool end_device_capacity;
} esp_zb_network_descriptor_t;
```

**Handler**: `esp_ncp_zb_scan_complete_fn()` → callback from `esp_zb_zdo_active_scan_request()`

---

### 0x000A: ESP_NCP_NETWORK_STOP_SCAN

**Description**: Terminates a scan in progress

**Request Payload**: None

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_stop_scan_fn()`

---

### 0x000B: ESP_NCP_NETWORK_PAN_ID_GET

**Description**: Get the Zigbee network PAN ID

**Request Payload**: None

**Response Payload**:
```c
uint16_t pan_id;
```

**Handler**: `esp_ncp_zb_pan_id_get_fn()` → calls `esp_zb_get_pan_id()`

---

### 0x000C: ESP_NCP_NETWORK_PAN_ID_SET

**Description**: Set the Zigbee network PAN ID

**Request Payload**:
```c
uint16_t pan_id;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_pan_id_set_fn()` → calls `esp_zb_set_pan_id()`

---

### 0x000D: ESP_NCP_NETWORK_EXTENDED_PAN_ID_GET

**Description**: Get the Zigbee network extended PAN ID

**Request Payload**: None

**Response Payload**:
```c
esp_zb_ieee_addr_t extended_pan_id;  // 8 bytes
```

**Handler**: `esp_ncp_zb_extended_pan_id_get_fn()` → calls `esp_zb_get_extended_pan_id()`

---

### 0x000E: ESP_NCP_NETWORK_EXTENDED_PAN_ID_SET

**Description**: Set the Zigbee network extended PAN ID

**Request Payload**:
```c
esp_zb_ieee_addr_t extended_pan_id;  // 8 bytes
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_extended_pan_id_set_fn()` → calls `esp_zb_set_extended_pan_id()`

---

### 0x000F: ESP_NCP_NETWORK_PRIMARY_CHANNEL_GET

**Description**: Get the primary channel mask

**Request Payload**: None

**Response Payload**:
```c
uint32_t channel_mask;
```

**Handler**: `esp_ncp_zb_primary_channel_get_fn()` → calls `esp_zb_get_primary_network_channel_set()`

---

### 0x0010: ESP_NCP_NETWORK_PRIMARY_CHANNEL_SET

**Description**: Set the primary channel mask

**Request Payload**:
```c
uint32_t channel_mask;  // 0x00000800 (ch 11) to 0x07FFF800 (ch 11-26)
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_network_primary_channel_set_fn()` → calls `esp_zb_set_primary_network_channel_set()`

---

### 0x0011: ESP_NCP_NETWORK_SECONDARY_CHANNEL_GET

**Description**: Get the secondary channel mask

**Request Payload**: None

**Response Payload**:
```c
uint32_t channel_mask;
```

**Handler**: `esp_ncp_zb_secondary_channel_get_fn()` → calls `esp_zb_get_secondary_network_channel_set()`

---

### 0x0012: ESP_NCP_NETWORK_SECONDARY_CHANNEL_SET

**Description**: Set the secondary channel mask

**Request Payload**:
```c
uint32_t channel_mask;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_secondary_channel_set_fn()` → calls `esp_zb_set_secondary_network_channel_set()`

---

### 0x0013: ESP_NCP_NETWORK_CHANNEL_GET

**Description**: Get the current 2.4G channel

**Request Payload**: None

**Response Payload**:
```c
uint8_t channel;
```

**Handler**: `esp_ncp_zb_current_channel_fn()` → calls `esp_zb_get_current_channel()`

---

### 0x0014: ESP_NCP_NETWORK_CHANNEL_SET

**Description**: Set the 2.4G channel mask

**Request Payload**:
```c
uint32_t channel_mask;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_channel_set_fn()` → calls `esp_zb_set_channel_mask()`

---

### 0x0015: ESP_NCP_NETWORK_TXPOWER_GET

**Description**: Get the TX power

**Request Payload**: None

**Response Payload**:
```c
int8_t power;  // dB
```

**Handler**: `esp_ncp_zb_tx_power_get_fn()` → calls `esp_zb_get_tx_power()`

---

### 0x0016: ESP_NCP_NETWORK_TXPOWER_SET

**Description**: Set the TX power

**Request Payload**:
```c
int8_t power;  // dB
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_tx_power_set_fn()` → calls `esp_zb_set_tx_power()`

---

### 0x0017: ESP_NCP_NETWORK_PRIMARY_KEY_GET

**Description**: Get the primary security network key

**Request Payload**: None

**Response Payload**:
```c
uint8_t key[16];  // Network key
```

**Handler**: `esp_ncp_zb_primary_key_get_fn()` → calls `esp_zb_get_primary_network_key()`

---

### 0x0018: ESP_NCP_NETWORK_PRIMARY_KEY_SET

**Description**: Set the primary security network key

**Request Payload**:
```c
uint8_t key[16];  // Network key
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_primary_key_set_fn()` → calls `esp_zb_set_primary_network_key()`

---

### 0x0019: ESP_NCP_NETWORK_FRAME_COUNT_GET

**Description**: Get the network frame counter

**Request Payload**: None

**Response Payload**:
```c
uint32_t frame_count;
```

**Handler**: `esp_ncp_zb_nwk_frame_counter_get_fn()`

---

### 0x001A: ESP_NCP_NETWORK_FRAME_COUNT_SET

**Description**: Set the network frame counter

**Request Payload**:
```c
uint32_t frame_count;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_nwk_frame_counter_set_fn()`

---

### 0x001B: ESP_NCP_NETWORK_ROLE_GET

**Description**: Get the network role (Coordinator/Router)

**Request Payload**: None

**Response Payload**:
```c
uint8_t role;  // 0 = Coordinator, 1 = Router
```

**Handler**: `esp_ncp_zb_nwk_role_get_fn()`

---

### 0x001C: ESP_NCP_NETWORK_ROLE_SET

**Description**: Set the network role

**Request Payload**:
```c
uint8_t role;  // 0 = Coordinator, 1 = Router
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_nwk_role_set_fn()`

---

### 0x001D: ESP_NCP_NETWORK_SHORT_ADDRESS_GET

**Description**: Get the Zigbee device short address

**Request Payload**: None

**Response Payload**:
```c
uint16_t short_addr;
```

**Handler**: `esp_ncp_zb_short_addr_fn()` → calls `esp_zb_get_short_address()`

---

### 0x001E: ESP_NCP_NETWORK_SHORT_ADDRESS_SET

**Description**: Set the Zigbee device short address

**Request Payload**:
```c
uint16_t short_addr;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_short_addr_set_fn()` → calls `esp_zb_set_short_address()`

---

### 0x001F: ESP_NCP_NETWORK_LONG_ADDRESS_GET

**Description**: Get the Zigbee device long (IEEE) address

**Request Payload**: None

**Response Payload**:
```c
esp_zb_ieee_addr_t long_addr;  // 8 bytes
```

**Handler**: `esp_ncp_zb_long_addr_fn()` → calls `esp_zb_get_long_address()`

---

### 0x0020: ESP_NCP_NETWORK_LONG_ADDRESS_SET

**Description**: Set the Zigbee device long (IEEE) address

**Request Payload**:
```c
esp_zb_ieee_addr_t long_addr;  // 8 bytes
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_long_addr_set_fn()` → calls `esp_zb_set_long_address()`

---

### 0x0021: ESP_NCP_NETWORK_CHANNEL_MASKS_GET

**Description**: Get the channel masks

**Request Payload**: None

**Response Payload**:
```c
uint32_t channel_mask;
```

**Handler**: `esp_ncp_zb_channel_masks_get_fn()`

---

### 0x0022: ESP_NCP_NETWORK_CHANNEL_MASKS_SET

**Description**: Set the channel masks

**Request Payload**:
```c
uint32_t channel_mask;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_network_primary_channel_set_fn()` (same as 0x0010)

---

### 0x0023: ESP_NCP_NETWORK_UPDATE_ID_GET

**Description**: Get the network update ID

**Request Payload**: None

**Response Payload**:
```c
uint8_t update_id;
```

**Handler**: `esp_ncp_zb_nwk_update_id_fn()`

---

### 0x0024: ESP_NCP_NETWORK_UPDATE_ID_SET

**Description**: Set the network update ID

**Request Payload**:
```c
uint8_t update_id;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_nwk_update_id_set_fn()`

---

### 0x0025: ESP_NCP_NETWORK_TRUST_CENTER_ADDR_GET

**Description**: Get the network trust center address

**Request Payload**: None

**Response Payload**:
```c
esp_zb_ieee_addr_t trust_center_addr;  // 8 bytes
```

**Handler**: `esp_ncp_zb_nwk_trust_center_addr_fn()`

---

### 0x0026: ESP_NCP_NETWORK_TRUST_CENTER_ADDR_SET

**Description**: Set the network trust center address

**Request Payload**:
```c
esp_zb_ieee_addr_t trust_center_addr;  // 8 bytes
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_nwk_trust_center_addr_set_fn()`

---

### 0x0027: ESP_NCP_NETWORK_LINK_KEY_GET

**Description**: Get the network link key

**Request Payload**: None

**Response Payload**:
```c
uint8_t link_key[16];
```

**Handler**: `esp_ncp_zb_nwk_link_key_fn()`

---

### 0x0028: ESP_NCP_NETWORK_LINK_KEY_SET

**Description**: Set the network link key

**Request Payload**:
```c
uint8_t link_key[16];
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_nwk_link_key_set_fn()`

---

### 0x0029: ESP_NCP_NETWORK_SECURE_MODE_GET

**Description**: Get the network security mode

**Request Payload**: None

**Response Payload**:
```c
uint8_t security_mode;
```

**Handler**: `esp_ncp_zb_nwk_security_mode_fn()`

---

### 0x002A: ESP_NCP_NETWORK_SECURE_MODE_SET

**Description**: Set the network security mode

**Request Payload**:
```c
uint8_t security_mode;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_nwk_security_mode_set_fn()`

---

### 0x002B: ESP_NCP_NETWORK_PREDEFINED_PANID

**Description**: Enable or disable predefined network PAN ID

**Request Payload**:
```c
bool enable;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_use_predefined_nwk_panid_set_fn()`

---

### 0x002C: ESP_NCP_NETWORK_SHORT_TO_IEEE

**Description**: Get the network IEEE address by the short address

**Request Payload**:
```c
uint16_t short_addr;
```

**Response Payload**:
```c
esp_zb_ieee_addr_t ieee_addr;  // 8 bytes
```

**Handler**: `esp_ncp_zb_ieee_address_by_short_get_fn()` → calls `esp_zb_address_short_by_ieee()`

---

### 0x002D: ESP_NCP_NETWORK_IEEE_TO_SHORT

**Description**: Get the network short address by the IEEE address

**Request Payload**:
```c
esp_zb_ieee_addr_t ieee_addr;  // 8 bytes
```

**Response Payload**:
```c
uint16_t short_addr;
```

**Handler**: `esp_ncp_zb_address_short_by_ieee_get_fn()` → calls `esp_zb_address_short_by_ieee()`

---

### 0x002E: ESP_NCP_NETWORK_NVRAM_ERASE_AT_START_SET

**Description**: Enable/disable erasing zb_storage at next start

**Request Payload**:
```c
bool erase;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_nvram_erase_at_start_set_fn()` → calls `esp_zb_nvram_erase_at_start()`

---

### 0x002F: ESP_NCP_NETWORK_NVRAM_ERASE_AT_START_GET

**Description**: Get the nvram-erase-at-start flag

**Request Payload**: None

**Response Payload**:
```c
bool erase;
```

**Handler**: `esp_ncp_zb_nvram_erase_at_start_get_fn()`

---

## ZCL Commands (0x0100 - 0x0108)

### 0x0100: ESP_NCP_ZCL_ENDPOINT_ADD

**Description**: Configures endpoint information on the NCP

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

**Handler**: `esp_ncp_zb_add_endpoint_fn()` → calls `esp_zb_ep_list_add_endpoint()`

---

### 0x0101: ESP_NCP_ZCL_ENDPOINT_DEL

**Description**: Remove endpoint information on the NCP

**Request Payload**:
```c
uint8_t endpoint;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_del_endpoint_fn()` → calls `esp_zb_ep_list_remove_endpoint()`

---

### 0x0102: ESP_NCP_ZCL_ATTR_READ

**Description**: Read attribute data on NCP endpoints

**Request Payload**:
```c
typedef struct {
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t attr_count;
    uint16_t attr_id[attr_count];
} esp_ncp_zb_read_attr_t;
```

**Response Payload** (via notification):
```c
typedef struct {
    esp_zb_zcl_cmd_info_t info;  // Contains status, cluster, etc.
    uint8_t attr_count;
    // Followed by attr_count * (uint16_t attr_id + esp_zb_zcl_attr_type_t type + uint8_t size + value[size])
} esp_ncp_zb_read_attr_resp_t;
```

**Handler**: `esp_ncp_zb_read_attr_fn()` → calls `esp_zb_zcl_read_attr_cmd()`

---

### 0x0103: ESP_NCP_ZCL_ATTR_WRITE

**Description**: Write attribute data on NCP endpoints

**Request Payload**:
```c
typedef struct {
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t attr_count;
    // Followed by attr_count * (uint16_t attr_id + esp_zb_zcl_attr_type_t type + uint8_t size + value[size])
} esp_ncp_zb_write_attr_t;
```

**Response Payload** (via notification):
```c
typedef struct {
    esp_zb_zcl_cmd_info_t info;
    uint8_t attr_count;
    // Followed by attr_count * (esp_zb_zcl_status_t status + uint16_t attr_id)
} esp_ncp_zb_write_attr_resp_t;
```

**Handler**: `esp_ncp_zb_write_attr_fn()` → calls `esp_zb_zcl_write_attr_cmd()`

---

### 0x0104: ESP_NCP_ZCL_ATTR_REPORT

**Description**: Report attribute data on NCP endpoints (notification only)

**Notification Payload**:
```c
typedef struct {
    esp_zb_zcl_status_t status;
    esp_zb_zcl_addr_t src_address;
    uint8_t src_endpoint;
    uint8_t dst_endpoint;
    uint16_t cluster;
    uint16_t attr_id;
    uint8_t attr_type;
    uint8_t attr_size;
    uint8_t attr_value[attr_size];
} esp_ncp_zb_report_attr_t;
```

**Handler**: `esp_ncp_zb_report_attr_fn()` → callback from ZCL report handler

---

### 0x0105: ESP_NCP_ZCL_ATTR_DISC

**Description**: Discover attribute data on NCP endpoints

**Request Payload**:
```c
typedef struct {
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint16_t start_attr_id;
    uint8_t max_attr_count;
} esp_ncp_zb_disc_attr_t;
```

**Response Payload** (via notification):
```c
typedef struct {
    esp_zb_zcl_cmd_info_t info;
    uint8_t attr_count;
    // Followed by attr_count * (uint16_t attr_id + esp_zb_zcl_attr_type_t type)
} esp_ncp_zb_disc_attr_resp_t;
```

**Handler**: `esp_ncp_zb_disc_attr_fn()` → calls `esp_zb_zcl_discover_attributes_cmd()`

---

### 0x0106: ESP_NCP_ZCL_READ

**Description**: Read ZCL command (generic ZCL command)

**Request Payload**:
```c
typedef struct {
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint16_t profile_id;
    uint8_t cmd_id;
    uint8_t direction;  // 0 = client to server, 1 = server to client
    uint16_t payload_len;
    uint8_t payload[payload_len];
} esp_ncp_zb_zcl_data_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_zcl_read_fn()` → calls `esp_zb_zcl_custom_cluster_cmd()`

---

### 0x0107: ESP_NCP_ZCL_WRITE

**Description**: Write ZCL command (generic ZCL command)

**Request Payload**:
```c
typedef struct {
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint16_t profile_id;
    uint8_t cmd_id;
    uint8_t direction;
    uint16_t payload_len;
    uint8_t payload[payload_len];
} esp_ncp_zb_zcl_data_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_zcl_write_fn()` → calls `esp_zb_zcl_custom_cluster_cmd()`

---

### 0x0108: ESP_NCP_ZCL_REPORT_CONFIG

**Description**: Report configure on NCP endpoints

**Request Payload**:
```c
typedef struct {
    uint16_t dst_addr;
    uint8_t dst_endpoint;
    uint8_t src_endpoint;
    uint16_t cluster_id;
    uint8_t direction;  // 0 = reporting, 1 = receiving reports
    uint16_t attr_id;
    uint8_t attr_type;
    uint16_t min_interval;
    uint16_t max_interval;
    uint16_t reportable_change;
} esp_ncp_zb_report_config_t;
```

**Response Payload** (via notification):
```c
typedef struct {
    esp_zb_zcl_cmd_info_t info;
    uint8_t attr_count;
    // Followed by attr_count * (esp_zb_zcl_status_t status + uint8_t direction + uint16_t attr_id)
} esp_ncp_zb_report_config_resp_t;
```

**Handler**: `esp_ncp_zb_report_config_fn()` → calls `esp_zb_zcl_config_report_cmd()`

---

## ZDO Commands (0x0200 - 0x0202)

### 0x0200: ESP_NCP_ZDO_BIND_SET

**Description**: Create a binding between two endpoints on two nodes

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
} esp_zb_zdo_bind_req_param_t;
```

**Response Payload** (via notification):
```c
typedef struct {
    esp_zb_zdp_status_t zdo_status;
    esp_ncp_zb_user_cb_t zdo_cb;  // User callback info
} esp_ncp_zb_bind_parameters_t;
```

**Handler**: `esp_ncp_zb_set_bind_fn()` → calls `esp_zb_zdo_device_bind_req()`

---

### 0x0201: ESP_NCP_ZDO_UNBIND_SET

**Description**: Remove a binding between two endpoints on two nodes

**Request Payload**: Same as bind (0x0200)

**Response Payload** (via notification):
```c
typedef struct {
    esp_zb_zdp_status_t zdo_status;
    esp_ncp_zb_user_cb_t zdo_cb;
} esp_ncp_zb_unbind_parameters_t;
```

**Handler**: `esp_ncp_zb_set_unbind_fn()` → calls `esp_zb_zdo_device_unbind_req()`

---

### 0x0202: ESP_NCP_ZDO_FIND_MATCH

**Description**: Send match desc request to find matched Zigbee device

**Request Payload**:
```c
typedef struct {
    uint16_t dst_nwk_addr;
    uint16_t addr_of_interest;
    uint16_t profile_id;
    uint8_t num_in_clusters;
    uint8_t num_out_clusters;
    uint16_t cluster_list[num_in_clusters + num_out_clusters];
} esp_zb_zdo_match_desc_req_param_t;
```

**Response Payload** (via notification):
```c
typedef struct {
    esp_zb_zdp_status_t zdo_status;
    uint16_t addr;
    uint8_t endpoint;
    esp_ncp_zb_user_cb_t zdo_cb;
} esp_ncp_zb_find_parameters_t;
```

**Handler**: `esp_ncp_zb_find_match_fn()` → calls `esp_zb_zdo_match_cluster()`

---

## APS Commands (0x0300 - 0x0302)

### 0x0300: ESP_NCP_APS_DATA_REQUEST

**Description**: Request APS data transmission

**Request Payload**:
```c
typedef struct {
    uint8_t dst_addr_mode;
    esp_zb_addr_u dst_addr;
    uint8_t dst_endpoint;
    uint16_t profile_id;
    uint16_t cluster_id;
    uint8_t src_endpoint;
    uint32_t asdu_length;
    uint8_t asdu[asdu_length];
    uint8_t tx_options;
    bool use_alias;
    uint16_t alias_src_addr;
    int alias_seq_num;
    uint8_t radius;
} esp_zb_apsde_data_req_t;
```

**Response Payload**:
```c
uint8_t status;
```

**Handler**: `esp_ncp_zb_aps_data_request_fn()` → calls `esp_zb_aps_data_request()`

---

### 0x0301: ESP_NCP_APS_DATA_INDICATION

**Description**: APS data indication (notification only)

**Notification Payload**:
```c
typedef struct {
    uint8_t states;  // ESP_NCP_INDICATION
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
    uint8_t lqi;
    int rx_time;
    uint32_t asdu_length;
    uint8_t asdu[asdu_length];
} esp_ncp_zb_aps_data_ind_t;
```

**Handler**: `esp_ncp_zb_aps_data_indication_fn()` → callback from `esp_zb_aps_data_indication_handler_register()`

---

### 0x0302: ESP_NCP_APS_DATA_CONFIRM

**Description**: APS data confirm (notification only)

**Notification Payload**:
```c
typedef struct {
    uint8_t states;  // ESP_NCP_CONFIRM
    uint8_t dst_addr_mode;
    esp_zb_zcl_basic_cmd_t basic_cmd;
    int tx_time;
    uint8_t confirm_status;
    uint32_t asdu_length;
    uint8_t asdu[asdu_length];
} esp_ncp_zb_aps_data_confirm_t;
```

**Handler**: `esp_ncp_zb_aps_data_confirm_fn()` → callback from `esp_zb_aps_data_confirm_handler_register()`

---

## Payload Decoding Reference

### Common Data Types

```c
// IEEE Address (64-bit)
typedef uint8_t esp_zb_ieee_addr_t[8];

// Address Union
typedef union {
    uint16_t addr_short;
    esp_zb_ieee_addr_t addr_long;
} esp_zb_addr_u;

// ZCL Command Info
typedef struct {
    esp_zb_zcl_status_t status;
    uint16_t cluster;
    uint8_t src_endpoint;
    uint8_t dst_endpoint;
    // ... other fields
} esp_zb_zcl_cmd_info_t;
```

### Frame Structure

All frames follow this structure:

```
┌─────────────────────────────────────────┐
│ Frame Header (6 bytes)                  │
├─────────────────────────────────────────┤
│ Flags (2 bytes)                          │
│   - version:4                           │
│   - type:4                              │
│   - reserved:8                           │
├─────────────────────────────────────────┤
│ ID (2 bytes) - Command/Event ID         │
├─────────────────────────────────────────┤
│ SN (1 byte) - Sequence Number           │
├─────────────────────────────────────────┤
│ Length (2 bytes) - Payload Length        │
├─────────────────────────────────────────┤
│ Payload (variable length)               │
└─────────────────────────────────────────┘
```

### SLIP Encoding

Frames are wrapped in SLIP encoding:
- `SLIP_END` (0xC0) - Frame delimiter
- `SLIP_ESC` (0xDB) - Escape character
- `SLIP_ESC_END` (0xDC) - Escaped END
- `SLIP_ESC_ESC` (0xDD) - Escaped ESC

---

## Command Dispatch Table

The NCP firmware uses a function table to dispatch commands:

```c
static const esp_ncp_zb_func_t ncp_zb_func_table[] = {
    {ESP_NCP_NETWORK_INIT, esp_ncp_zb_network_init_fn},
    {ESP_NCP_NETWORK_START, esp_ncp_zb_start_fn},
    // ... all commands listed above
    {ESP_NCP_APS_DATA_REQUEST, esp_ncp_zb_aps_data_request_fn},
    {ESP_NCP_APS_DATA_INDICATION, esp_ncp_zb_aps_data_indication_fn},
    {ESP_NCP_APS_DATA_CONFIRM, esp_ncp_zb_aps_data_confirm_fn},
};
```

Commands are dispatched by iterating through this table and matching the frame ID.

---

## References

- **NCP Command Definitions**: `components/esp-zigbee-ncp/src/priv/esp_ncp_zb.h`
- **Frame Structure**: `components/esp-zigbee-ncp/src/priv/esp_ncp_frame.h`
- **Host Protocol Definitions**: `components/zb_host/src/priv/esp_host_zb.h`
- **Command Handlers**: `components/esp-zigbee-ncp/src/esp_ncp_zb.c` (lines 1875-1936)

