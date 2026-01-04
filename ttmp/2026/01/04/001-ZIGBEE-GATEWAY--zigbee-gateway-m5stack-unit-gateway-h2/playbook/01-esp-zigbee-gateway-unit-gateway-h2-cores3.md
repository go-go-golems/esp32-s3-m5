---
Title: ESP Zigbee Gateway (Unit Gateway H2 + CoreS3)
Ticket: 001-ZIGBEE-GATEWAY
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32
    - esp32h2
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_gateway.txt
      Note: Text dump of the M5Stack guide content used to build this playbook
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_gateway.html
      Note: Archived HTML of the M5Stack guide
ExternalSources:
    - https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway:M5Stack guide for running ESP Zigbee Gateway using Unit Gateway H2 + CoreS3
    - https://github.com/espressif/esp-zigbee-sdk/tree/master/examples/esp_zigbee_gateway:Upstream example README (RCP + gateway, auto RCP update behavior)
    - https://github.com/espressif/esp-idf/tree/v5.4.1/examples/openthread/ot_rcp:ESP-IDF `ot_rcp` example used as the 802.15.4 RCP firmware
Summary: Build + flash the ESP32-H2 RCP firmware (ot_rcp) and the ESP32-S3 Zigbee gateway firmware (esp_zigbee_gateway), then verify logs for Wi-Fi + Zigbee network formation.
LastUpdated: 2026-01-04T20:51:00Z
WhatFor: "Repeatable build/flash/run procedure for the M5Stack Unit Gateway H2 + CoreS3 Zigbee gateway setup."
WhenToUse: "When bringing up the gateway for the first time, or when updating either the ESP32-H2 RCP firmware or the CoreS3 gateway firmware."
---

# ESP Zigbee Gateway (Unit Gateway H2 + CoreS3)

## Purpose

Run the ESP Zigbee Gateway example with:

- **RCP (802.15.4 / Zigbee radio):** ESP32-H2 inside **Unit Gateway H2** running the `ot_rcp` firmware.
- **Gateway (Wi-Fi + Zigbee stack):** ESP32-S3 inside **CoreS3** running the `esp_zigbee_gateway` firmware (from `esp-zigbee-sdk`).

## Environment Assumptions

- Host OS has USB serial access (prefer `/dev/serial/by-id/...` paths).
- ESP-IDF 5.4.1 installed at `/home/manuel/esp/esp-5.4.1` (symlink to `/home/manuel/esp/esp-idf-5.4.1`).
- `esp-zigbee-sdk` cloned at `/home/manuel/esp/esp-zigbee-sdk`.
- Hardware:
  - M5Stack **CoreS3** (ESP32-S3)
  - M5Stack **Unit Gateway H2** (ESP32-H2)
  - Physical connection between CoreS3 ↔ Unit Gateway H2 matching your configured UART pins (defaults in this playbook follow the M5 guide).

## Commands

```bash
set -euo pipefail

# ESP-IDF 5.4.1 (as requested)
export IDF_PATH="$HOME/esp/esp-5.4.1"

# esp-zigbee-sdk clone (example location)
export ZIGBEE_SDK="$HOME/esp/esp-zigbee-sdk"

# Choose stable ports if available (recommended).
ls -la /dev/serial/by-id/ || true

# Set these before flashing:
PORT_RCP='/dev/serial/by-id/REPLACE_ME_unit_gateway_h2'
PORT_GW='/dev/serial/by-id/REPLACE_ME_cores3'
```

### 1) Activate ESP-IDF environment

```bash
source "$IDF_PATH/export.sh"
```

### 2) Build + flash the RCP firmware to Unit Gateway H2 (ESP32-H2)

The M5 guide uses the ESP-IDF `ot_rcp` example as the RCP firmware.

```bash
source "$IDF_PATH/export.sh"
cd "$IDF_PATH/examples/openthread/ot_rcp"

idf.py set-target esp32h2
idf.py menuconfig
```

In `menuconfig`:

- `Component config` → `OpenThread` → `Thread Radio Co-Processor Feature` → ensure `Enable vendor command for RCP` is enabled (`CONFIG_OPENTHREAD_NCP_VENDOR_HOOK=y`).
- If your UART wiring differs from defaults: `OpenThread RCP Example` → `Configure RCP UART pin manually`.

Optional (recommended by M5 if you see UART issues with long cables): lower the UART baud rate:

```bash
perl -0pe 's/\\.baud_rate\\s*=\\s*\\s*460800/\\.baud_rate =  230400/' -i main/esp_ot_config.h
rg -n "\\.baud_rate" main/esp_ot_config.h | head
```

Build and flash the RCP:

```bash
idf.py build

# Put Unit Gateway H2 in download mode:
# - open case
# - hold/press BOOT
# - connect USB power/data cable
idf.py -p "$PORT_RCP" flash
```

### 3) Build + flash the gateway firmware to CoreS3 (ESP32-S3)

```bash
source "$IDF_PATH/export.sh"
cd "$ZIGBEE_SDK/examples/esp_zigbee_gateway"

idf.py set-target esp32s3
idf.py menuconfig
```

In `menuconfig`:

- `ESP Zigbee gateway rcp update` → `Board Configuration`
  - `Pin to RCP reset`: `-1`
  - `Pin to RCP boot`: `-1`
  - `Pin to RCP TX`: `18`
  - `Pin to RCP RX`: `17`
- If `Pin to RCP reset/boot` are `-1`, consider disabling `ESP Zigbee gateway rcp update` → `Update RCP automatically` (avoids attempting an update sequence without reset/boot control).
- `Example Connection Configuration`
  - `WiFi SSID`
  - `WiFi Password`

CoreS3 power enable (M5 guide): add this helper and call it at the beginning of `app_main()` (file: `main/esp_zigbee_gateway.c`) to enable Grove power output.

```c
#include "driver/i2c.h"

void fix_aw9523_p0_pull_up(void)
{
    /* AW9523 P0 is in push-pull mode */
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_12,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = GPIO_NUM_11,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400000
    };
    i2c_param_config(I2C_NUM_1, &i2c_conf);
    i2c_driver_install(I2C_NUM_1, i2c_conf.mode, 0, 0, 0);

    uint8_t data[2];
    data[0] = 0x11;
    data[1] = 0x10;
    i2c_master_write_to_device(I2C_NUM_1, 0x58, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
    i2c_driver_delete(I2C_NUM_1);
}
```

Build and flash the gateway:

```bash
idf.py build

# WARNING: wipes NVS and all flash contents on the CoreS3.
idf.py -p "$PORT_GW" erase-flash

idf.py -p "$PORT_GW" flash monitor
```

## Exit Criteria

- CoreS3 serial monitor shows all of:
  - RCP firmware version check (and no spinel/UART errors)
  - Wi-Fi connects successfully (IP acquired)
  - Zigbee stack initialized and network formation succeeds
  - Network opens for device joining (steering started / open for N seconds)

## Notes

- The M5Stack guide recommends ESP-IDF v5.3.1; this playbook uses ESP-IDF **v5.4.1** per ticket requirements.
- If UART between CoreS3 ↔ Unit Gateway H2 is flaky, reduce the `ot_rcp` baud rate (see Step 2) and ensure your wiring/pins match the configured values.
- To exit `idf.py monitor`, use `Ctrl-]`.
