---
Title: Diary
Ticket: 0035-IMPROVE-NCP-LOGGING
Status: active
Topics:
    - logging
    - debugging
    - ncp
    - codegen
DocType: reference
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
Summary: Investigation diary for improving NCP logging by converting numeric constants to human-readable strings
LastUpdated: 2026-01-06T09:28:41.481246759-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Improve debug logging in NCP firmware by converting numerical constants (command IDs, status codes, state values) to their string names. This will make logs more readable and debugging faster, especially when analyzing protocol traces or error conditions.

## Step 1: Analyze Current Logging and Constant Definitions

This step establishes the baseline: understanding what constants need conversion and how logging currently works.

**Commit (code):** N/A — analysis phase

### What I did

1. Examined `esp_ncp_zb.c` and `esp_ncp_bus.c` for logging patterns
2. Identified constant definitions:
   - **Command IDs**: ~60+ `#define` macros in `esp_ncp_zb.h` (e.g., `ESP_NCP_NETWORK_INIT = 0x0000`, `ESP_NCP_ZCL_ATTR_READ = 0x0102`)
   - **Status codes**: `esp_ncp_status_t` enum in `esp_zb_ncp.h` (4 values: `ESP_NCP_SUCCESS`, `ESP_NCP_ERR_FATAL`, `ESP_NCP_BAD_ARGUMENT`, `ESP_NCP_ERR_NO_MEM`)
   - **Network states**: `esp_ncp_states_t` enum (6 values: `ESP_NCP_OFFLINES`, `ESP_NCP_JOINING`, `ESP_NCP_CONNECTED`, etc.)
   - **Security modes**: `esp_ncp_secur_t` enum (2 values)
3. Found current logging examples:
   - `ESP_LOGI(TAG, "Read attribute response: status(%d), cluster(0x%x)", ...)` — numeric values
   - `ESP_LOGI(TAG, "dst_addr_mode %0x, dst_short_addr %02x, ...", ...)` — hex dumps
   - `ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(...), ...)` — already uses string conversion!
4. Searched codebase for existing enum-to-string patterns:
   - `gw_event_id_to_str()` — switch/case pattern (small enum)
   - `bt_decode_*()` functions — lookup table pattern (larger sets)
   - `esp_err_to_name()` — ESP-IDF standard pattern

### Why

Need to understand the scope and existing patterns before choosing an approach. The codebase already has examples of both manual and lookup-table approaches.

### What worked

- Found clear constant definitions in header files
- Identified existing conversion patterns in the codebase
- Noted that ESP-IDF provides `esp_err_to_name()` as a standard pattern

### What didn't work

- `esp_zb_zdo_signal_to_string()` exists but returns a placeholder string ("The signal type of Zigbee") — not actually implemented

### What I learned

1. **Command IDs are `#define` macros**, not enums — this affects codegen options
2. **Status/state values are enums** — easier to handle with switch/case or lookup tables
3. **Existing patterns vary**:
   - Switch/case for small sets (<10 values)
   - Lookup tables for larger sets (>10 values)
4. **ESP-IDF logging** uses `esp_err_to_name()` pattern — could follow similar convention

### What was tricky to build

- Distinguishing between `#define` constants (command IDs) and enum constants (status/state) — they need different handling approaches

### What warrants a second pair of eyes

- Decision on whether to use codegen vs manual implementation
- Performance implications of lookup tables vs switch/case (likely negligible for logging)

### What should be done in the future

- Consider standardizing on a single pattern across the codebase
- If using codegen, ensure it integrates cleanly with the build system

### Code review instructions

- Review `esp_ncp_zb.h` lines 65-127 for command ID definitions
- Review `esp_zb_ncp.h` lines 28-55 for enum definitions
- Check logging calls in `esp_ncp_zb.c` (grep for `ESP_LOG*`)

### Technical details

**Command IDs** (`esp_ncp_zb.h`):
- Network commands: `0x0000` - `0x002F` (48 commands)
- ZCL commands: `0x0100` - `0x0108` (9 commands)
- ZDO commands: `0x0200` - `0x0202` (3 commands)
- APS commands: `0x0300` - `0x0302` (3 commands)
- Total: ~63 command IDs

**Enums** (`esp_zb_ncp.h`):
- `esp_ncp_status_t`: 4 values
- `esp_ncp_states_t`: 6 values
- `esp_ncp_secur_t`: 2 values

**Current logging locations**:
- `esp_ncp_zb.c`: ~28 `ESP_LOG*` calls
- `esp_ncp_bus.c`: ~16 `ESP_LOG*` calls

### What I'd do differently next time

- Start with a grep for existing conversion functions to find patterns faster

---

## Step 2: Evaluate Conversion Approaches

This step compares different approaches for converting constants to strings, weighing codegen options against manual implementation.

**Commit (code):** N/A — analysis phase

### What I did

1. **Manual switch/case approach**:
   - Pros: Simple, no build dependencies, easy to debug
   - Cons: Verbose for 60+ command IDs, manual maintenance
   - Example: `gw_event_id_to_str()` pattern

2. **Manual lookup table approach**:
   - Pros: Scales better, can include descriptions
   - Cons: Still manual maintenance, linear search overhead (negligible for logging)
   - Example: `bt_decode_*()` pattern

3. **Preprocessor stringification** (`#define STRINGIFY(x) #x`):
   - Pros: Automatic, no runtime overhead
   - Cons: Only works if you have the macro name at call site, doesn't work for runtime values
   - Limitation: Can't convert `uint16_t cmd_id` to string if `cmd_id` is a variable

4. **Code generation (Python script)**:
   - Pros: Automatic, stays in sync with headers, can generate both switch/case and lookup tables
   - Cons: Requires build integration, one more tool to maintain
   - Approach: Parse header files, extract `#define` and `enum` definitions, generate C code

5. **Tree-sitter AST transformation**:
   - Pros: More robust parsing, can handle complex C syntax
   - Cons: Significant complexity, requires tree-sitter C grammar, overkill for this use case
   - Verdict: Not worth it for this problem

6. **X-Macros pattern**:
   - Pros: Single source of truth, generates both constants and conversion functions
   - Cons: Requires refactoring existing `#define` macros, more invasive change
   - Example pattern:
     ```c
     #define ESP_NCP_CMD_LIST \
         X(ESP_NCP_NETWORK_INIT, 0x0000) \
         X(ESP_NCP_NETWORK_START, 0x0001) \
         ...
     ```

### Why

Need to choose an approach that balances:
- **Maintainability**: Easy to keep in sync with header changes
- **Simplicity**: Not over-engineered
- **Performance**: Negligible for logging (all approaches are fine)
- **Build integration**: Fits into existing ESP-IDF build system

### What worked

- Identified that codegen is the sweet spot for 60+ command IDs
- Found that manual switch/case is fine for small enums (4-6 values)
- Confirmed preprocessor stringification won't work for runtime values

### What didn't work

- Tree-sitter is overkill — Python regex/parsing is sufficient for header files
- X-Macros would require refactoring all existing `#define` macros — too invasive

### What I learned

1. **Hybrid approach is best**:
   - Codegen for command IDs (large set, `#define` macros)
   - Manual switch/case for enums (small sets, already defined)
2. **Python script is sufficient** — don't need full AST parsing for simple header extraction
3. **ESP-IDF build system** supports custom codegen via component CMakeLists.txt

### What was tricky to build

- Understanding limitations of preprocessor stringification (can't stringify runtime values)
- Evaluating whether codegen complexity is worth it (yes, for 60+ constants)

### What warrants a second pair of eyes

- Decision on codegen script location and build integration
- Whether to generate switch/case or lookup tables for command IDs

### What should be done in the future

- Consider X-Macros pattern for future constant definitions (prevents this problem)
- Document codegen script usage in component README

### Code review instructions

- Review existing codegen examples in ESP-IDF components
- Check if ESP-IDF has standard patterns for codegen

### Technical details

**Recommended approach**:

1. **Command IDs** → Codegen Python script:
   - Parse `esp_ncp_zb.h` for `#define ESP_NCP_*` patterns
   - Generate `esp_ncp_cmd_id_to_str(uint16_t id)` function
   - Use switch/case for performance (compiler optimizes to jump table)

2. **Status/State enums** → Manual switch/case:
   - Small sets (4-6 values), unlikely to change frequently
   - Simple functions: `esp_ncp_status_to_str()`, `esp_ncp_state_to_str()`

3. **Build integration**:
   - Add Python script to component
   - Use ESP-IDF `idf_component_register()` with custom target
   - Generate `.c` file, include in component sources

**Codegen script structure**:
```python
# scripts/gen_ncp_strings.py
import re

def extract_cmd_ids(header_path):
    # Parse #define ESP_NCP_* 0x... patterns
    # Return list of (name, value) tuples
    pass

def generate_switch_case(cmd_ids):
    # Generate C switch/case function
    pass
```

### What I'd do differently next time

- Research ESP-IDF codegen patterns first (might have standard approach)

---

## Step 3: Discover Lower-Level ESP_ZB Stack Symbols

This step expands the scope to include lower-level Zigbee stack symbols that are also logged but currently show as numeric values.

**Commit (code):** N/A — analysis phase

### What I did

1. Searched for ESP_ZB symbols used in logging:
   - Found `esp_zb_zdo_signal_to_string()` exists but is a stub (returns `"The signal type of Zigbee"`)
   - Found many lower-level enums and constants logged in `esp_ncp_zb.c`:
     - `esp_zb_app_signal_type_t` — ~60+ signal types (ZDO, BDB, NWK, SE signals)
     - `esp_zb_zdp_status_t` — 16 ZDP status codes
     - `esp_zb_zcl_status_t` — ~30 ZCL status codes
     - `esp_zb_zcl_attr_type_t` — ~50+ attribute types
     - `ESP_ZB_APS_ADDR_MODE_*` — APS addressing modes
     - `ESP_ZB_ZCL_CLUSTER_ID_*` — Many cluster IDs
2. Examined header files:
   - `esp_zigbee_zdo_common.h` — ZDO signals and status
   - `esp_zigbee_zcl_common.h` — ZCL status and attribute types
3. Found logging examples:
   - `ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), ...)` — already uses conversion (but stub)
   - `ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ...)` — status codes logged as numeric

### Why

The user correctly pointed out that lower-level ESP_ZB symbols are also logged and should be converted. This significantly expands the scope but improves debugging across the entire stack.

### What worked

- Found that `esp_zb_zdo_signal_to_string()` already exists (good API design) but needs implementation
- Identified all major lower-level symbol categories
- Found header file locations for codegen parsing

### What didn't work

- `esp_zb_zdo_signal_to_string()` is just a stub — needs full implementation

### What I learned

1. **Scope is much larger**: ~200+ constants total (NCP + lower-level stack)
2. **Some infrastructure exists**: `esp_zb_zdo_signal_to_string()` API already defined
3. **Codegen is even more valuable**: Manual implementation would be impractical for 200+ constants
4. **Priority matters**: ZDO signals and status codes are most critical (frequently logged, error debugging)

### What was tricky to build

- Finding all the lower-level symbol categories (scattered across multiple header files)
- Determining which symbols are actually logged vs just defined

### What warrants a second pair of eyes

- Decision on whether to implement all converters at once or phase them
- Whether cluster IDs need conversion (many values, less frequently logged)

### What should be done in the future

- Consider implementing `esp_zb_zdo_signal_to_string()` first (highest priority, API already exists)
- Phase implementation: NCP constants first, then lower-level stack symbols
- Document which symbols are converted vs which remain numeric

### Code review instructions

- Review `esp_zigbee_zdo_common.h` lines 19-107 for signal types and status codes
- Review `esp_zigbee_zcl_common.h` lines 143-241 for ZCL status and attribute types
- Check `esp_ncp_zb.c` for logging calls using these symbols

### Technical details

**Lower-level symbols to convert**:

1. **ZDO Signal Types** (`esp_zb_app_signal_type_t`): ~60+ values
   - `ESP_ZB_ZDO_SIGNAL_*` (ZDO signals)
   - `ESP_ZB_BDB_SIGNAL_*` (BDB signals)
   - `ESP_ZB_NWK_SIGNAL_*` (NWK signals)
   - `ESP_ZB_SE_SIGNAL_*` (SE signals)
   - **Priority**: High (frequently logged, stub function exists)

2. **ZDP Status** (`esp_zb_zdp_status_t`): 16 values
   - `ESP_ZB_ZDP_STATUS_SUCCESS`, `ESP_ZB_ZDP_STATUS_TIMEOUT`, etc.
   - **Priority**: High (error debugging critical)

3. **ZCL Status** (`esp_zb_zcl_status_t`): ~30 values
   - `ESP_ZB_ZCL_STATUS_SUCCESS`, `ESP_ZB_ZCL_STATUS_MALFORMED_CMD`, etc.
   - **Priority**: High (error debugging critical)

4. **ZCL Attribute Types** (`esp_zb_zcl_attr_type_t`): ~50+ values
   - `ESP_ZB_ZCL_ATTR_TYPE_U8`, `ESP_ZB_ZCL_ATTR_TYPE_U16`, etc.
   - **Priority**: Medium (useful for attribute debugging)

5. **APS Addressing Modes**: Constants (few values)
   - `ESP_ZB_APS_ADDR_MODE_64_ENDP_PRESENT`, etc.
   - **Priority**: Medium

6. **ZCL Cluster IDs**: Many constants
   - `ESP_ZB_ZCL_CLUSTER_ID_BASIC`, `ESP_ZB_ZCL_CLUSTER_ID_ON_OFF`, etc.
   - **Priority**: Low (many values, less frequently needed)

**Total scope**: ~200+ constants across NCP and lower-level stack

### What I'd do differently next time

- Ask about lower-level symbols upfront to get full scope
- Check for existing stub functions that need implementation
