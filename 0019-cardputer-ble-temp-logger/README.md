## Cardputer Tutorial 0019 — BLE Temperature Logger (ESP-IDF 5.4.1)

This tutorial exposes a simple **BLE GATT server** that streams a mock temperature value via **notifications**.

### GATT layout

- Service UUID: `0xFFF0`
- Temperature characteristic UUID: `0xFFF1`
  - Properties: READ + NOTIFY
  - Format: int16 little-endian, temperature in centi-degC (e.g. 2150 = 21.50°C)
- Control characteristic UUID: `0xFFF2`
  - Properties: WRITE
  - Format: uint16 little-endian, notify period in milliseconds (0 disables notify loop)

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger && \
idf.py flash monitor
```

### Linux client (bleak)

See `tools/ble_client.py`.


