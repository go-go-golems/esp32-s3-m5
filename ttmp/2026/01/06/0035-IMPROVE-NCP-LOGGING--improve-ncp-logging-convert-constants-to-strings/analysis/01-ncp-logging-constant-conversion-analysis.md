---
Title: NCP Logging Constant Conversion Analysis
Ticket: 0035-IMPROVE-NCP-LOGGING
Status: active
Topics:
    - logging
    - debugging
    - ncp
    - codegen
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Main NCP Zigbee implementation with command handlers and logging
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c
      Note: UART bus implementation with logging
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_zb.h
      Note: Command ID constant definitions
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/include/esp_zb_ncp.h
      Note: Status and state enum definitions
ExternalSources: []
Summary: Analysis of approaches for converting NCP constants (command IDs, status codes, states) to strings for improved debug logging
LastUpdated: 2026-01-06T09:29:23.123456789-05:00
WhatFor: ""
WhenToUse: ""
---

# NCP Logging Constant Conversion Analysis

## Problem Statement

Current NCP logging outputs numeric values (hex/dec) for command IDs, status codes, and state values:

```c
ESP_LOGI(TAG, "Read attribute response: status(%d), cluster(0x%x)", 
         message->info.status, message->info.cluster);
ESP_LOGI(TAG, "dst_addr_mode %0x, dst_short_addr %02x", ...);
```

This makes logs harder to read and debug. Converting these to string names (e.g., `ESP_NCP_SUCCESS` instead of `0x00`) would improve readability.

## Current State

### Constants to Convert

1. **Command IDs** (`esp_ncp_zb.h`): ~63 `#define` macros
   - Network: `0x0000` - `0x002F` (48 commands)
   - ZCL: `0x0100` - `0x0108` (9 commands)
   - ZDO: `0x0200` - `0x0202` (3 commands)
   - APS: `0x0300` - `0x0302` (3 commands)

2. **Status Codes** (`esp_zb_ncp.h`): `esp_ncp_status_t` enum (4 values)
   - `ESP_NCP_SUCCESS = 0x00`
   - `ESP_NCP_ERR_FATAL = 0x01`
   - `ESP_NCP_BAD_ARGUMENT = 0x02`
   - `ESP_NCP_ERR_NO_MEM = 0x03`

3. **Network States** (`esp_zb_ncp.h`): `esp_ncp_states_t` enum (6 values)
   - `ESP_NCP_OFFLINES`, `ESP_NCP_JOINING`, `ESP_NCP_CONNECTED`, etc.

4. **Security Modes** (`esp_zb_ncp.h`): `esp_ncp_secur_t` enum (2 values)

### Lower-Level ESP_ZB Stack Symbols

The NCP code also logs lower-level Zigbee stack symbols from the **esp-zigbee-lib managed component** (`managed_components/espressif__esp-zigbee-lib`) that should be converted:

5. **ZDO Signal Types** (`managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_common.h`): `esp_zb_app_signal_type_t` enum (~60+ values)
   - `ESP_ZB_ZDO_SIGNAL_*` (ZDO signals)
   - `ESP_ZB_BDB_SIGNAL_*` (BDB signals)
   - `ESP_ZB_NWK_SIGNAL_*` (NWK signals)
   - `ESP_ZB_SE_SIGNAL_*` (SE signals)
   - **Note**: `esp_zb_zdo_signal_to_string()` exists but is a stub returning `"The signal type of Zigbee"` — needs implementation!

6. **ZDP Status Codes** (`managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_common.h`): `esp_zb_zdp_status_t` enum (16 values)
   - `ESP_ZB_ZDP_STATUS_SUCCESS`, `ESP_ZB_ZDP_STATUS_TIMEOUT`, etc.
   - Used in bind/unbind/find callbacks

7. **ZCL Status Codes** (`managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_common.h`): `esp_zb_zcl_status_t` enum (~30 values)
   - `ESP_ZB_ZCL_STATUS_SUCCESS`, `ESP_ZB_ZCL_STATUS_MALFORMED_CMD`, etc.
   - Logged in attribute read/write handlers

8. **ZCL Attribute Types** (`managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_common.h`): `esp_zb_zcl_attr_type_t` enum (~50+ values)
   - `ESP_ZB_ZCL_ATTR_TYPE_U8`, `ESP_ZB_ZCL_ATTR_TYPE_U16`, etc.
   - Logged when reading/writing attributes

9. **APS Addressing Modes** (`managed_components/espressif__esp-zigbee-lib/include/aps/esp_zigbee_aps.h`): `esp_zb_aps_address_mode_t` enum (5 values)
   - `ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT`, `ESP_ZB_APS_ADDR_MODE_64_ENDP_PRESENT`, etc.
   - Used in APS data indication/confirm handlers

10. **ZCL Cluster IDs** (`managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_common.h`): `esp_zb_zcl_cluster_id_t` enum (~50+ values)
    - `ESP_ZB_ZCL_CLUSTER_ID_BASIC`, `ESP_ZB_ZCL_CLUSTER_ID_ON_OFF`, etc.
    - Used in cluster list and attribute operations

### Logging Locations

- `esp_ncp_zb.c`: ~28 `ESP_LOG*` calls
- `esp_ncp_bus.c`: ~16 `ESP_LOG*` calls
- Lower-level stack components: Additional logging of ESP_ZB symbols

## Approaches Evaluated

### 1. Manual Switch/Case Functions

**Pattern**: Simple switch/case returning string literals

**Example** (from `gw_bus.c`):
```c
const char *gw_event_id_to_str(int32_t id) {
    switch (id) {
        case GW_CMD_PERMIT_JOIN: return "GW_CMD_PERMIT_JOIN";
        case GW_CMD_ONOFF: return "GW_CMD_ONOFF";
        default: return "GW_EVT_UNKNOWN";
    }
}
```

**Pros**:
- Simple, no build dependencies
- Easy to debug
- Compiler optimizes to jump table (fast)

**Cons**:
- Verbose for 60+ command IDs
- Manual maintenance (must update when constants change)
- Error-prone (easy to miss a case)

**Verdict**: Good for small enums (4-6 values), not ideal for 60+ command IDs

---

### 2. Manual Lookup Tables

**Pattern**: Array of structs with linear search

**Example** (from `bt_decode.c`):
```c
typedef struct {
    uint32_t value;
    const char *name;
    const char *desc;
} bt_decode_entry_t;

static const bt_decode_entry_t k_gatt_status[] = {
    {ESP_GATT_OK, "ESP_GATT_OK", "Operation successful"},
    {ESP_GATT_INVALID_HANDLE, "ESP_GATT_INVALID_HANDLE", "Invalid handle"},
    // ...
};

const char *bt_decode_gatt_status_name(esp_gatt_status_t status) {
    for (size_t i = 0; i < sizeof(k_gatt_status)/sizeof(k_gatt_status[0]); i++) {
        if (k_gatt_status[i].value == status) {
            return k_gatt_status[i].name;
        }
    }
    return "ESP_GATT_STATUS_UNKNOWN";
}
```

**Pros**:
- Scales better than switch/case
- Can include descriptions
- Easy to extend

**Cons**:
- Still manual maintenance
- Linear search overhead (negligible for logging, but slower than switch/case)
- More memory usage

**Verdict**: Better than switch/case for large sets, but still manual

---

### 3. Preprocessor Stringification

**Pattern**: Use `#define STRINGIFY(x) #x` macro

**Example**:
```c
#define STRINGIFY(x) #x
#define CMD_ID_NAME(cmd_id) STRINGIFY(cmd_id)

ESP_LOGI(TAG, "Command: %s", CMD_ID_NAME(ESP_NCP_NETWORK_INIT));
// Output: "Command: ESP_NCP_NETWORK_INIT"
```

**Pros**:
- Automatic, no runtime overhead
- No code generation needed

**Cons**:
- **Critical limitation**: Only works if you have the macro name at the call site
- **Doesn't work for runtime values**: Can't convert `uint16_t cmd_id` variable to string
- Example of what **doesn't work**:
  ```c
  uint16_t cmd_id = receive_command();  // Runtime value
  ESP_LOGI(TAG, "Command: %s", CMD_ID_NAME(cmd_id));  // ❌ Won't work!
  ```

**Verdict**: Not suitable for this use case (we need runtime conversion)

---

### 4. Code Generation (Python Script)

**Pattern**: Parse header files, generate C conversion functions

**Approach**:
1. Python script parses `esp_ncp_zb.h` for `#define ESP_NCP_*` patterns
2. Extracts name-value pairs
3. Generates C switch/case function
4. Integrates into ESP-IDF build system

**Example generated code**:
```c
// Generated by scripts/gen_ncp_strings.py
const char *esp_ncp_cmd_id_to_str(uint16_t id) {
    switch (id) {
        case ESP_NCP_NETWORK_INIT: return "ESP_NCP_NETWORK_INIT";
        case ESP_NCP_NETWORK_START: return "ESP_NCP_NETWORK_START";
        // ... 60+ more cases ...
        default: return "ESP_NCP_UNKNOWN";
    }
}
```

**Pros**:
- Automatic — stays in sync with header changes
- No manual maintenance
- Can generate optimized switch/case (fast)
- Single source of truth (header file)

**Cons**:
- Requires build integration
- One more tool to maintain
- Must handle edge cases (comments, multi-line defines, etc.)

**Verdict**: **Best approach for 60+ command IDs**

---

### 5. Tree-sitter AST Transformation

**Pattern**: Parse C code with tree-sitter, transform AST, generate code

**Pros**:
- Robust parsing (handles complex C syntax)
- Can transform existing code (not just generate new code)

**Cons**:
- Significant complexity
- Requires tree-sitter C grammar dependency
- Overkill for this problem
- Python regex/parsing is sufficient for header files

**Verdict**: Not worth the complexity for this use case

---

### 6. X-Macros Pattern

**Pattern**: Single source of truth that generates both constants and conversion functions

**Example**:
```c
#define ESP_NCP_CMD_LIST \
    X(ESP_NCP_NETWORK_INIT, 0x0000) \
    X(ESP_NCP_NETWORK_START, 0x0001) \
    ...

// Generate constants
#define X(name, value) name = value,
enum { ESP_NCP_CMD_LIST };
#undef X

// Generate conversion function
const char *esp_ncp_cmd_id_to_str(uint16_t id) {
    switch (id) {
        #define X(name, value) case value: return #name;
        ESP_NCP_CMD_LIST
        #undef X
        default: return "UNKNOWN";
    }
}
```

**Pros**:
- Single source of truth
- No code generation needed
- Automatic stringification

**Cons**:
- **Requires refactoring all existing `#define` macros** — very invasive change
- Would break existing code that uses `ESP_NCP_NETWORK_INIT` directly
- Not backward compatible

**Verdict**: Good pattern for new code, but too invasive for existing codebase

---

## Recommended Approach

### Hybrid Strategy

1. **NCP Command IDs** → **Codegen Python script**
   - Parse `esp_ncp_zb.h` for `#define ESP_NCP_*` patterns
   - Generate `esp_ncp_cmd_id_to_str(uint16_t id)` switch/case function
   - Integrate into ESP-IDF build system

2. **NCP Status/State enums** → **Manual switch/case**
   - Small sets (4-6 values), unlikely to change frequently
   - Simple functions: `esp_ncp_status_to_str()`, `esp_ncp_state_to_str()`
   - Low maintenance burden

3. **Lower-Level ESP_ZB Symbols** → **Codegen Python script**
   - **ZDO Signal Types**: Implement `esp_zb_zdo_signal_to_string()` (currently stub)
     - Parse `managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_common.h` for `esp_zb_app_signal_type_t` enum
     - Generate switch/case function
   - **ZDP Status**: Generate `esp_zb_zdp_status_to_str()` function
     - Parse `managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_common.h` for `esp_zb_zdp_status_t` enum
   - **ZCL Status**: Generate `esp_zb_zcl_status_to_str()` function
     - Parse `managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_common.h` for `esp_zb_zcl_status_t` enum
   - **ZCL Attribute Types**: Generate `esp_zb_zcl_attr_type_to_str()` function
     - Parse `managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_common.h` for `esp_zb_zcl_attr_type_t` enum
   - **APS Address Modes**: Generate `esp_zb_aps_addr_mode_to_str()` function
     - Parse `managed_components/espressif__esp-zigbee-lib/include/aps/esp_zigbee_aps.h` for `esp_zb_aps_address_mode_t` enum
   - **ZCL Cluster IDs**: Consider lookup table (many values)
     - Could generate from cluster list in `esp_ncp_zb.c` or header files

### Implementation Plan

1. **Create codegen script** (`scripts/gen_ncp_strings.py`):
   - Parse header file with regex
   - Extract `#define ESP_NCP_* 0x...` patterns
   - Generate C switch/case function
   - Handle edge cases (comments, multi-line, etc.)

2. **Integrate into build** (component `CMakeLists.txt`):
   - Add custom target to run codegen
   - Generate `esp_ncp_strings.c` (or `.h` with inline functions)
   - Include generated file in component sources

3. **Create manual enum converters**:
   - `esp_ncp_status_to_str()` — 4 values
   - `esp_ncp_state_to_str()` — 6 values
   - `esp_ncp_secur_to_str()` — 2 values

4. **Update logging calls**:
   - Replace numeric format specifiers with string functions
   - Example: `status(%d)` → `status(%s)`, `esp_ncp_status_to_str(status)`

### Codegen Script Structure

```python
#!/usr/bin/env python3
"""Generate NCP command ID to string conversion function."""

import re
import sys

def extract_cmd_ids(header_path):
    """Parse header file for #define ESP_NCP_* patterns."""
    cmd_ids = []
    with open(header_path, 'r') as f:
        for line in f:
            # Match: #define ESP_NCP_NETWORK_INIT 0x0000
            match = re.match(r'#define\s+(ESP_NCP_\w+)\s+(0x[0-9A-Fa-f]+)', line)
            if match:
                name, value = match.groups()
                cmd_ids.append((name, int(value, 16)))
    return sorted(cmd_ids, key=lambda x: x[1])  # Sort by value

def generate_switch_case(cmd_ids, function_name='esp_ncp_cmd_id_to_str'):
    """Generate C switch/case function."""
    lines = [
        f'const char *{function_name}(uint16_t id) {{',
        '    switch (id) {'
    ]
    for name, value in cmd_ids:
        lines.append(f'        case {name}: return "{name}";')
    lines.extend([
        '        default: return "ESP_NCP_UNKNOWN";',
        '    }',
        '}'
    ])
    return '\n'.join(lines)

if __name__ == '__main__':
    header_path = sys.argv[1]
    output_path = sys.argv[2]
    cmd_ids = extract_cmd_ids(header_path)
    code = generate_switch_case(cmd_ids)
    with open(output_path, 'w') as f:
        f.write(code)
```

## Comparison Table

| Approach | Maintenance | Performance | Complexity | Suitability |
|----------|-------------|-------------|------------|-------------|
| Manual switch/case | High (manual) | Fast (jump table) | Low | Small enums only |
| Manual lookup table | High (manual) | Medium (linear search) | Low | Medium sets |
| Preprocessor stringify | Low | Fast (compile-time) | Low | ❌ Doesn't work for runtime |
| **Codegen (Python)** | **Low (automatic)** | **Fast (jump table)** | **Medium** | **✅ Large sets** |
| Tree-sitter AST | Low (automatic) | Fast | High | Overkill |
| X-Macros | Low (automatic) | Fast | Medium | ❌ Too invasive |

## Next Steps

### Phase 1: NCP-Level Constants
1. Implement codegen script for NCP command IDs
2. Create manual enum converters for NCP status/state
3. Integrate codegen into ESP-IDF build system
4. Update NCP logging calls to use new conversion functions

### Phase 2: Lower-Level ESP_ZB Stack Symbols
5. Implement `esp_zb_zdo_signal_to_string()` (currently stub)
6. Generate converters for ZDP status, ZCL status, ZCL attribute types
7. Generate converter for APS addressing modes
8. Consider cluster ID lookup table (many values)
9. Update logging calls throughout stack components
10. Test and validate output

### Priority
- **High**: ZDO signal types (already has stub function, frequently logged)
- **High**: ZDP/ZCL status codes (error debugging critical)
- **Medium**: ZCL attribute types (useful for attribute debugging)
- **Low**: Cluster IDs (many values, less frequently needed)

## References

- Existing patterns in codebase:
  - `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_bus.c` — switch/case pattern
  - `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/bt_decode.c` — lookup table pattern
- ESP-IDF logging: `esp_err_to_name()` pattern
- See diary for detailed investigation notes
