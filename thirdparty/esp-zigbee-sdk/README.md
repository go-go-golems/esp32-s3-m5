# Vendored `esp-zigbee-sdk` subset (H2 NCP firmware)

This repository vendors a minimal subset of Espressif's Zigbee SDK needed to
build the **ESP32‑H2 NCP firmware** used by the host+NCP Zigbee setup
(Cardputer/ESP32‑S3 host ↔ Unit Gateway H2/ESP32‑H2 NCP).

## What is included

- `components/esp-zigbee-ncp/`
  - The NCP transport + Zigbee request handler component (modified in this repo
    for bring-up/debugging).
- `examples/esp_zigbee_ncp/`
  - The ESP-IDF project that builds the NCP firmware image for ESP32‑H2.
  - Includes `managed_components/` so it can build without fetching components
    online.

## Upstream origin

- Upstream repository: `https://github.com/espressif/esp-zigbee-sdk`
- Vendored from local checkout at commit: `3d86dd0c41923c68f5fa0aae519c3fe157e8641c`

The example project uses ESP-IDF Component Manager dependencies declared in:

- `examples/esp_zigbee_ncp/main/idf_component.yml`

This vendor snapshot includes the corresponding `managed_components/` directory
and `dependencies.lock` from the upstream example so builds do not require
network access.

## Build

Prerequisites:

- ESP-IDF environment exported (`IDF_PATH` set, `idf.py` available)

Build the H2 NCP firmware:

```bash
idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp build
```

Flash (adjust port to your H2 USB Serial/JTAG device):

```bash
idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 flash monitor
```

