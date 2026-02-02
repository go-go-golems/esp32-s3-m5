---
Title: 'ESP32-H2 NCP Firmware and ZNSP Protocol: Host Integration Reference'
Ticket: 0069-ANALYZE-PAST-FIRMWARE
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32s3
    - esp32h2
    - esp32
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0031-zigbee-orchestrator/components/zb_host/src/esp_host_frame.c
      Note: Host-side SLIP/CRC framing and multi-frame parsing.
    - Path: 0031-zigbee-orchestrator/components/zb_host/src/esp_host_zb.c
      Note: Host request/response dispatch and notify callback handling.
    - Path: 0031-zigbee-orchestrator/main/gw_zb.c
      Note: Host usage pattern
    - Path: thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_frame.c
      Note: Defines ZNSP framing
    - Path: thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Implements NCP command handlers
    - Path: thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_zb.h
      Note: Authoritative ZNSP command ID definitions.
    - Path: thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/main/esp_zigbee_ncp.c
      Note: NCP firmware entry point (app_main).
ExternalSources: []
Summary: Protocol-level reference for the ESP32-H2 Zigbee NCP firmware and ZNSP host integration, derived from source.
LastUpdated: 2026-02-02T11:32:40-05:00
WhatFor: Host integration work, protocol debugging, and firmware bring-up against the ESP32-H2 Zigbee NCP.
WhenToUse: When you need the actual ZNSP framing, command IDs, and host/NCP behavior from the firmware source.
---


# ESP32-H2 NCP Firmware and ZNSP Protocol: Host Integration Reference

## Goal

Provide a detailed, code-derived reference for the ESP32-H2 Zigbee NCP firmware and its host-facing ZNSP protocol, including framing, command IDs, payload expectations, and host integration patterns (ESP32-S3 or any external host that can speak UART + SLIP).

## Context

The ESP32-H2 runs Espressif's Zigbee NCP firmware (from the esp-zigbee-sdk). It exposes a host-facing protocol referred to in the code as ZNSP, transported over UART using SLIP framing and a CRC16 checksum. A host (e.g., ESP32-S3 running the 0031 Zigbee orchestrator) uses the companion host component to encode/decode frames and issue ZNSP requests. This document is derived from the actual firmware and host sources in this repository, not from high-level marketing docs.

The host must keep UART pins clean (avoid console noise), handle SLIP framing, and respect buffer sizes and response timeouts. The H2 ROM prints a boot banner on UART, which can disrupt framing if not accounted for.

## Quick Reference

### Architecture Map (Firmware + Host)

- NCP firmware (ESP32-H2): `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/`
  - `esp_ncp_main.c` - event loop + bus dispatch
  - `esp_ncp_bus.c` - UART framing (pattern detect) + SLIP boundary
  - `esp_ncp_frame.c` - SLIP decode/encode + CRC validation
  - `esp_ncp_zb.c` - ZNSP command handlers, Zigbee stack integration
- Host component (ESP32-S3): `0031-zigbee-orchestrator/components/zb_host/`
  - `esp_host_main.c` - event loop + bus dispatch
  - `esp_host_bus.c` - UART framing (pattern detect) + SLIP boundary
  - `esp_host_frame.c` - SLIP decode/encode + CRC validation
  - `esp_host_zb.c` - ZNSP request/response dispatch + notify callbacks
- Host application example (0031): `0031-zigbee-orchestrator/main/gw_zb.c`
- NCP example firmware entry point: `thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/main/esp_zigbee_ncp.c`

### Firmware Entry Points and Startup Sequence

NCP firmware (ESP32-H2):

- `app_main()` in `esp_zigbee_ncp.c`:
  - `nvs_flash_init()`
  - `esp_ncp_init(NCP_HOST_CONNECTION_MODE_UART)`
  - `esp_ncp_start()`
- The NCP event loop runs in `esp_ncp_main_task()` and dispatches UART frames into `esp_ncp_frame_output()`.

Host (ESP32-S3, 0031):

- `gw_zb_stack_task_main()`
  - Delay ~1500 ms to absorb H2 ROM UART banner
  - `esp_zb_platform_config()` with `RADIO_MODE_UART_NCP` + `HOST_CONNECTION_MODE_UART`
  - `esp_zb_init()` + `esp_zb_start(false)`
  - `esp_zb_stack_main_loop()` (notification loop)
- Low-level ZNSP requests use `esp_host_zb_output()` via `gw_zb_znsp_request()`.

### UART / Bus Configuration (Keep UART Clean)

Host (0031 defaults):

- UART: `CONFIG_HOST_BUS_UART_NUM=1`
- Pins: `RX=GPIO1`, `TX=GPIO2` (Cardputer Grove G1/G2)
- Baud: `115200`
- Console: USB Serial/JTAG, not UART (avoids protocol corruption)

NCP example defaults (H2):

- UART pins: `CONFIG_NCP_BUS_UART_RX_PIN=23`, `CONFIG_NCP_BUS_UART_TX_PIN=24`
- Console: USB Serial/JTAG (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`), UART console disabled
- Bootloader log level set to none (avoid UART noise)

### ZNSP Frame Format

Each ZNSP frame is serialized as:

- SLIP encoded bytes containing:
  - Header (packed, 7 bytes total):
    - `flags` (uint16):
      - `version` (4 bits)
      - `type` (4 bits) - 0=request, 1=response, 2=notify
      - `reserved` (8 bits)
    - `id` (uint16) - ZNSP command ID
    - `sn` (uint8) - sequence number
    - `len` (uint16) - payload length
  - Payload: `len` bytes
  - CRC16 (uint16), computed over header+payload

CRC and endianness:

- CRC = `esp_crc16_le(UINT16_MAX, header+payload)`
- CRC is appended as a 16-bit value. On ESP targets this is little-endian on the wire.

SLIP encoding:

- Special bytes:
  - `SLIP_END = 0xC0`
  - `SLIP_ESC = 0xDB`
  - `SLIP_ESC_END = 0xDC`
  - `SLIP_ESC_ESC = 0xDD`
- Encoder behavior:
  - Prefixes an initial `SLIP_END` (flush noise)
  - Escapes `SLIP_END` and `SLIP_ESC` in payload
  - Terminates with a final `SLIP_END`

Host vs NCP parsing differences:

- Host (`esp_host_frame_input`) can parse multiple frames from a single SLIP decode buffer (loop over `offerset`).
- NCP (`esp_ncp_frame_output`) expects exactly one frame per SLIP decode buffer.

### Frame Types

- `type=0`: request (host -> NCP)
- `type=1`: response (NCP -> host)
- `type=2`: notify (NCP -> host asynchronous)

The host implementation matches responses by `id` only (not by `sn`), and uses a single outstanding request lock (recursive mutex). If you roll your own host, you may want to correlate by `(id, sn)` instead.

### Buffer Sizes and UART Framing Behavior

- Both host and NCP use:
  - `*_BUS_BUF_SIZE = 1024` bytes (single frame buffer)
  - `*_BUS_RINGBUF_SIZE = 20480`
- The UART driver uses pattern detection on `SLIP_END` to identify frame boundaries.
- Host side will drop/flush frames larger than 1024 bytes (logs `frame too large`).

### Timing and Concurrency Notes

- Host and NCP event queues are length 60 (`HOST_EVENT_QUEUE_LEN`, `NCP_EVENT_QUEUE_LEN`).
- The host request path (`esp_host_zb_output`) waits up to ~2 seconds for a response (`pdMS_TO_TICKS(2000)`).
- Host serialization: a recursive mutex enforces one outstanding ZNSP request at a time in the stock host component.

### ZNSP Command ID Reference (Host <-> NCP)

Network commands (0x0000 - 0x002F):

| ID | Name | Notes |
| --- | --- | --- |
| 0x0000 | NETWORK_INIT | Resume network after reboot |
| 0x0001 | NETWORK_START | Start commissioning |
| 0x0002 | NETWORK_STATE | State query |
| 0x0003 | NETWORK_STACK_STATUS_HANDLER | Stack status notify |
| 0x0004 | NETWORK_FORMNETWORK | Form coordinator network |
| 0x0005 | NETWORK_PERMIT_JOINING | Allow joining (payload: uint8 seconds) |
| 0x0006 | NETWORK_JOINNETWORK | Join network (NCP handler is NULL) |
| 0x0007 | NETWORK_LEAVENETWORK | Leave network (NCP handler is NULL) |
| 0x0008 | NETWORK_START_SCAN | Active scan (payload: u32 mask + u8 duration) |
| 0x0009 | NETWORK_SCAN_COMPLETE_HANDLER | Scan complete notify |
| 0x000A | NETWORK_STOP_SCAN | Stop scan |
| 0x000B | NETWORK_PAN_ID_GET | Get PAN ID |
| 0x000C | NETWORK_PAN_ID_SET | Set PAN ID |
| 0x000D | NETWORK_EXTENDED_PAN_ID_GET | Get ext PAN ID |
| 0x000E | NETWORK_EXTENDED_PAN_ID_SET | Set ext PAN ID |
| 0x000F | NETWORK_PRIMARY_CHANNEL_GET | Get primary channel mask |
| 0x0010 | NETWORK_PRIMARY_CHANNEL_SET | Set primary channel mask (payload: u32 mask) |
| 0x0011 | NETWORK_SECONDARY_CHANNEL_GET | Get secondary channel mask |
| 0x0012 | NETWORK_SECONDARY_CHANNEL_SET | Set secondary channel mask (payload: u32 mask) |
| 0x0013 | NETWORK_CHANNEL_GET | Get current channel |
| 0x0014 | NETWORK_CHANNEL_SET | Set channel mask (payload: u32 mask) |
| 0x0015 | NETWORK_TXPOWER_GET | Get TX power |
| 0x0016 | NETWORK_TXPOWER_SET | Set TX power |
| 0x0017 | NETWORK_PRIMARY_KEY_GET | Get network key |
| 0x0018 | NETWORK_PRIMARY_KEY_SET | Set network key (payload: 16 bytes) |
| 0x0019 | NETWORK_FRAME_COUNT_GET | Get frame counter |
| 0x001A | NETWORK_FRAME_COUNT_SET | Set frame counter |
| 0x001B | NETWORK_ROLE_GET | Get role (0 coord / 1 router) |
| 0x001C | NETWORK_ROLE_SET | Set role |
| 0x001D | NETWORK_SHORT_ADDRESS_GET | Get short addr |
| 0x001E | NETWORK_SHORT_ADDRESS_SET | Set short addr |
| 0x001F | NETWORK_LONG_ADDRESS_GET | Get IEEE addr |
| 0x0020 | NETWORK_LONG_ADDRESS_SET | Set IEEE addr (payload: 8 bytes) |
| 0x0021 | NETWORK_CHANNEL_MASKS_GET | Get channel masks |
| 0x0022 | NETWORK_CHANNEL_MASKS_SET | Set channel masks |
| 0x0023 | NETWORK_UPDATE_ID_GET | Get update id |
| 0x0024 | NETWORK_UPDATE_ID_SET | Set update id |
| 0x0025 | NETWORK_TRUST_CENTER_ADDR_GET | Get trust center addr |
| 0x0026 | NETWORK_TRUST_CENTER_ADDR_SET | Set trust center addr |
| 0x0027 | NETWORK_LINK_KEY_GET | Get link key (returns IEEE + key) |
| 0x0028 | NETWORK_LINK_KEY_SET | Set link key (payload: 16 bytes) |
| 0x0029 | NETWORK_SECURE_MODE_GET | Get security mode (0/1) |
| 0x002A | NETWORK_SECURE_MODE_SET | Set security mode (payload: u8 mode) |
| 0x002B | NETWORK_PREDEFINED_PANID | Enable/disable predef PAN ID (payload: u8) |
| 0x002C | NETWORK_SHORT_TO_IEEE | Short -> IEEE lookup |
| 0x002D | NETWORK_IEEE_TO_SHORT | IEEE -> short lookup |
| 0x002E | NETWORK_NVRAM_ERASE_AT_START_SET | Set nvram erase flag (payload: u8) |
| 0x002F | NETWORK_NVRAM_ERASE_AT_START_GET | Get nvram erase flag |

ZCL commands (0x0100 - 0x0108):

| ID | Name | Notes |
| --- | --- | --- |
| 0x0100 | ZCL_ENDPOINT_ADD | Configure endpoint (payload: esp_ncp_zb_endpoint_t) |
| 0x0101 | ZCL_ENDPOINT_DEL | Remove endpoint |
| 0x0102 | ZCL_ATTR_READ | Read attribute |
| 0x0103 | ZCL_ATTR_WRITE | Write attribute |
| 0x0104 | ZCL_ATTR_REPORT | Report attribute |
| 0x0105 | ZCL_ATTR_DISC | Discover attribute |
| 0x0106 | ZCL_READ | Read ZCL command |
| 0x0107 | ZCL_WRITE | Write ZCL command |
| 0x0108 | ZCL_REPORT_CONFIG | Report configure (NCP handler is NULL) |

ZDO commands (0x0200 - 0x0202):

| ID | Name | Notes |
| --- | --- | --- |
| 0x0200 | ZDO_BIND_SET | Bind request |
| 0x0201 | ZDO_UNBIND_SET | Unbind request |
| 0x0202 | ZDO_FIND_MATCH | Match descriptor request |

APS commands (0x0300 - 0x0302):

| ID | Name | Notes |
| --- | --- | --- |
| 0x0300 | APS_DATA_REQUEST | Send APS data |
| 0x0301 | APS_DATA_INDICATION | APS receive indication (notify) |
| 0x0302 | APS_DATA_CONFIRM | APS confirm (notify) |

### Status Codes (Responses)

NCP status values (used by many SET operations):

- `0x00` success
- `0x01` fatal error
- `0x02` bad argument
- `0x03` out of memory

Host-side mapping example (0031):

- `0x00` -> `ESP_OK`
- `0x02` -> `ESP_ERR_INVALID_ARG`
- `0x03` -> `ESP_ERR_NO_MEM`
- `0x01` or other -> `ESP_FAIL`

### Selected Payload Notes (Observed in Firmware)

These payload sizes/structures are explicitly enforced in `esp_ncp_zb.c`:

- `NETWORK_PERMIT_JOINING`: payload is `uint8_t seconds`.
- `NETWORK_CHANNEL_SET` / `PRIMARY_CHANNEL_SET` / `SECONDARY_CHANNEL_SET`: payload `uint32_t mask`.
- `NETWORK_LONG_ADDRESS_SET`: payload `esp_zb_ieee_addr_t` (8 bytes).
- `NETWORK_LINK_KEY_SET`: payload is 16-byte key.
- `NETWORK_SECURE_MODE_SET`: payload `uint8_t mode` (0 = no security, 1 = preconfigured key).
- `NETWORK_NVRAM_ERASE_AT_START_SET`: payload `uint8_t flag`.
- `NETWORK_START_SCAN`: payload = `uint32_t channel_mask` + `uint8_t scan_duration`.

### Notification Behavior

- NCP uses `esp_ncp_noti_input()` to send notify frames (type=2).
- Host handles a subset of notifications via `host_zb_func_table`:
  - FORMNETWORK
  - JOINNETWORK
  - PERMIT_JOINING
  - LEAVENETWORK
  - ZDO_BIND_SET / ZDO_UNBIND_SET / ZDO_FIND_MATCH
- APS notifications (0x0301/0x0302) are produced by NCP; the host sample does not register handlers for these IDs by default. If you need APS data on the host, add handlers to the host table and queue.

### Known Limitations (from Firmware Table)

These command IDs are present but unimplemented in the NCP table (they return `ESP_ERR_INVALID_ARG`):

- `NETWORK_JOINNETWORK` (0x0006)
- `NETWORK_LEAVENETWORK` (0x0007)
- `ZCL_REPORT_CONFIG` (0x0108)

### Boot ROM UART Noise and Console Strategy

- The H2 ROM prints a boot banner on UART; host firmware in 0031 waits 1500 ms before bringing up the bus to avoid SLIP desync.
- The NCP example disables UART console and bootloader logs, and uses USB Serial/JTAG instead.
- When integrating with other hosts, emulate the same behavior: avoid UART console on the NCP bus pins, and either reset/flush or delay at host startup.

## Usage Examples

### 1) H2 NCP Firmware (minimal)

Use the esp-zigbee-sdk example as-is:

```c
#include "nvs_flash.h"
#include "esp_zb_ncp.h"

void app_main(void)
{
    nvs_flash_init();
    esp_ncp_init(NCP_HOST_CONNECTION_MODE_UART);
    esp_ncp_start();
}
```

Ensure these defaults are applied on H2:

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
CONFIG_BOOTLOADER_LOG_LEVEL_NONE=y
CONFIG_NCP_BUS_UART_RX_PIN=23
CONFIG_NCP_BUS_UART_TX_PIN=24
```

### 2) ESP32-S3 Host (0031-style ZNSP request)

```c
uint8_t out = 0xFF;
uint16_t outlen = sizeof(out);
uint8_t seconds = 60;

// Send permit-join request (0x0005).
const esp_err_t err = gw_zb_znsp_request(0x0005,
                                        &seconds,
                                        sizeof(seconds),
                                        &out,
                                        &outlen,
                                        pdMS_TO_TICKS(3000));
// out contains ZNSP status.
```

### 3) Raw Host Implementation (Pseudo-code)

If you are not using Espressif's host component, you must implement SLIP + CRC + framing:

```text
payload = ...
header.flags.version = 0
header.flags.type = REQUEST
header.id = ZNSP_ID
header.sn = seq
header.len = payload_len
frame = header || payload
crc = crc16_le(0xFFFF, frame)
frame_with_crc = frame || crc_le
slip_out = slip_encode(0xC0-prefix + frame_with_crc + 0xC0-suffix)
write_uart(slip_out)
```

On receive:

- Buffer until you see `SLIP_END (0xC0)`.
- SLIP-decode the frame.
- Validate CRC16 over header+payload.
- Dispatch by `id` and `type`.

### 4) Console Debugging (0031)

The orchestrator firmware exposes a `znsp req` command for direct testing:

```
znsp req 0x0005 3c
```

This sends `permit_join` with `0x3c` (60 seconds) and prints the raw response bytes.

### 5) Security/Link Key Example

Read the current link key exchange requirement:

- Request: `ZNSP_NETWORK_SECURE_MODE_GET (0x0029)` with empty payload
- Response payload: `uint8_t` (0 = no security, 1 = preconfigured key required)

Disable link key exchange requirement:

- Request: `ZNSP_NETWORK_SECURE_MODE_SET (0x002A)` with payload `uint8_t 0x00`
- Response payload: status byte

## Related

- `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/01-0031-zigbee-orchestrator-deep-dive-full-retrospective.md`
- `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/analysis/02-technical-memo-parc-0031-zigbee-orchestrator-retrospective.md`
- `ttmp/2026/01/06/0032-ANALYZE-NCP-FIRMWARE--analyze-ncp-h2-gateway-firmware/analysis/01-ncp-firmware-architecture-and-protocol-analysis.md`
- `ttmp/2026/01/06/0034-ANALYZE-ESP-ZIGBEE-LIB--analyze-esp-zigbee-lib-low-level-stack/analysis/02-esp-ncp-zb-c-complete-function-and-command-analysis.md`
