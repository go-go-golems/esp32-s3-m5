# I2C backend source provenance

Retrieved 2026-08-21 for the ESP-60 transport comparison.

## ESP-IDF 5.5.4 new master driver

- Repository: https://github.com/espressif/esp-idf
- Tag: `v5.5.4`
- Local project IDF HEAD: `735507283d5b2f9fb363a1901172dbd9e847945d`
- Preserved files:
  - `esp-idf-v5.5.4-i2c_master.c`
  - `esp-idf-v5.5.4-i2c_master.h`
- Raw URLs:
  - https://raw.githubusercontent.com/espressif/esp-idf/v5.5.4/components/esp_driver_i2c/i2c_master.c
  - https://raw.githubusercontent.com/espressif/esp-idf/v5.5.4/components/esp_driver_i2c/include/driver/i2c_master.h

The source includes the April 2025 behavior associated with Espressif commit
`459b75f81a121dc83beb103a10aee8216c657fce`: after a synchronous transaction error,
the hardware FSM is reset without clearing the entire bus. The shallow local IDF checkout does not retain that
historical commit object, but the tagged source contains the resulting `s_i2c_hw_fsm_reset(..., false)` call.

## M5GFX 0.2.27 ESP32 I2C implementation

- Repository: https://github.com/m5stack/M5GFX
- Tag: `0.2.27`
- Commit: `93b480bb349749202c8a2a953065c8ae95f58320`
- Preserved file: `M5GFX-0.2.27-esp32-common.cpp`
- Raw URL: https://raw.githubusercontent.com/m5stack/M5GFX/0.2.27/src/lgfx/v1/platforms/esp32/common.cpp

This is the M5GFX version resolved by the successful official PlatformIO build. Its ESP32 I2C code directly manages
controller registers, uses a per-port mutex, resets the hardware FSM at transaction start, and has explicit forced-STOP
and bus-recovery paths.

## M5Unified 0.2.20 I2C adapter

- Repository: https://github.com/m5stack/M5Unified
- Tag: `0.2.20`
- Commit: `774d920cd6851a5231748b56ece1b073645f313f`
- Preserved file: `M5Unified-0.2.20-I2C_Class.cpp`
- Raw URL: https://raw.githubusercontent.com/m5stack/M5Unified/0.2.20/src/utility/I2C_Class.cpp

This is the M5Unified version used by the successful official build. `I2C_Class` delegates `start`, `restart`, `write`,
`read`, and `stop` to the M5GFX I2C implementation.
