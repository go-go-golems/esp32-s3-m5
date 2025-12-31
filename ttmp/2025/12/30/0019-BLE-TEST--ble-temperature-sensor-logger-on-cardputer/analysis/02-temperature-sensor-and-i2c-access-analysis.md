---
Title: Temperature Sensor and I2C Access Analysis
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
Summary: Analysis of temperature sensor options, I2C access patterns, and integration with Cardputer hardware for BLE temperature logger.
LastUpdated: 2025-12-30T11:25:06.619233048-05:00
WhatFor: Understanding sensor options and I2C implementation patterns for temperature sensing
WhenToUse: When selecting and implementing temperature sensor hardware and drivers
---

# Temperature Sensor and I2C Access Analysis

## Executive Summary

The Cardputer does **not have a built-in temperature sensor**. We need to connect an external I2C temperature sensor. The codebase contains an **SHT3x** (temperature + humidity) sensor example that demonstrates I2C access patterns using ESP-IDF v5.x master driver APIs. This document analyzes sensor options, I2C implementation patterns, and integration requirements.

## Cardputer Hardware Constraints

### I2C Availability

**Cardputer I2C Ports**: Not explicitly documented in analyzed files, but ESP32-S3 has multiple I2C peripherals.

**Reference Implementation**: `esp32-s3-m5/0004-i2c-rolling-average/` uses:
- `I2C_NUM_0`
- GPIO8 (SDA) and GPIO9 (SCL) - **These are example pins, verify for Cardputer**

**Important**: Cardputer pin assignments need verification. Check:
- Cardputer schematic/datasheet
- M5Stack Cardputer documentation
- Existing Cardputer projects for I2C pin usage

### Memory Constraints

**Cardputer SRAM**: 512KB total (no PSRAM)
- Display buffers: ~64KB
- BLE stack: ~100KB
- **Available for sensor + application**: ~350KB

**I2C Driver Memory**: Minimal (~1-2KB for buffers)

## Temperature Sensor Options

### Option 1: SHT3x (SHT30/SHT31/SHT35) - Recommended

**Status**: Example implementation exists in codebase

**Reference**: `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c`

**Characteristics**:
- **Interface**: I2C (address 0x44 default, 0x45 alternate)
- **Measurement**: Temperature + Humidity (dual sensor)
- **Accuracy**: ±0.3°C (SHT30), ±0.2°C (SHT31), ±0.1°C (SHT35)
- **Range**: -40°C to +125°C
- **Resolution**: 0.01°C (16-bit)
- **Update Rate**: Single-shot or periodic (configurable)
- **Power**: Low power, good for battery operation

**Pros**:
- Example code already exists
- High accuracy
- Humidity sensor bonus
- Well-documented protocol
- Common breakout boards available

**Cons**:
- More expensive than basic sensors
- Requires CRC validation

**I2C Protocol**:
```c
// Single-shot measurement, high repeatability
uint8_t cmd[2] = {0x2C, 0x06};  // Clock stretching disabled
i2c_master_transmit(dev, cmd, 2, timeout);
vTaskDelay(pdMS_TO_TICKS(20));  // Measurement time
uint8_t data[6];  // 2 bytes temp + CRC + 2 bytes humidity + CRC
i2c_master_receive(dev, data, 6, timeout);

// Temperature conversion
uint16_t raw_temp = (data[0] << 8) | data[1];
float temp_c = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);
```

### Option 2: DS18B20 (1-Wire)

**Status**: Not I2C - uses 1-Wire protocol

**Characteristics**:
- **Interface**: 1-Wire (not I2C)
- **Accuracy**: ±0.5°C
- **Range**: -55°C to +125°C
- **Resolution**: 9-12 bit configurable

**Note**: Requires different driver (1-Wire, not I2C). Not recommended if we want to stick with I2C.

### Option 3: LM75 / TMP102 (Simple I2C)

**Status**: Not in codebase, but simple to implement

**Characteristics**:
- **Interface**: I2C
- **Address**: LM75 (0x48-0x4F), TMP102 (0x48 default)
- **Accuracy**: ±2°C (LM75), ±0.5°C (TMP102)
- **Range**: -55°C to +125°C
- **Resolution**: 0.125°C (9-bit) or 0.0625°C (12-bit)

**Pros**:
- Very simple protocol (read 2-byte register)
- Low cost
- Common breakout boards

**Cons**:
- Lower accuracy than SHT3x
- No humidity sensor

**I2C Protocol**:
```c
// Read temperature register (0x00)
uint8_t reg = 0x00;
i2c_master_write_read_device(bus, addr, &reg, 1, data, 2, timeout);
int16_t temp_raw = (data[0] << 8) | data[1];
float temp_c = (float)(temp_raw >> 7) * 0.5f;  // 9-bit resolution
```

### Option 4: MCP9808 (High Accuracy)

**Status**: Not in codebase

**Characteristics**:
- **Interface**: I2C (address 0x18-0x1F)
- **Accuracy**: ±0.25°C
- **Range**: -40°C to +125°C
- **Resolution**: 0.0625°C (16-bit)

**Pros**:
- High accuracy
- Simple protocol

**Cons**:
- More expensive
- No humidity sensor

## Recommended Choice: SHT3x

**Rationale**:
1. Example code exists in codebase (`0004-i2c-rolling-average`)
2. High accuracy suitable for logging
3. Humidity sensor provides additional value
4. Well-tested implementation pattern

## I2C Implementation Analysis

### ESP-IDF I2C Driver Versions

**ESP-IDF v4.4.6** (Cardputer current): Uses legacy I2C driver
- `driver/i2c.h` - Legacy API
- `i2c_param_config()`, `i2c_driver_install()`
- `i2c_master_cmd_begin()` - Command pattern

**ESP-IDF v5.x** (Example code): Uses new master driver
- `driver/i2c_master.h` - New API
- `i2c_new_master_bus()` - Bus creation
- `i2c_master_bus_add_device()` - Device attachment
- `i2c_master_transmit()`, `i2c_master_receive()` - Simplified API

**Note**: The example (`0004-i2c-rolling-average`) uses ESP-IDF v5.x APIs. For Cardputer (v4.4.6), we need to adapt to legacy APIs or verify if v5.x APIs are available.

### Legacy I2C API Pattern (v4.4.6)

```c
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO    8    // GPIO number for SCL
#define I2C_MASTER_SDA_IO    9    // GPIO number for SDA
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   100000

static void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

static esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}
```

### New I2C Master Driver API Pattern (v5.x)

**Reference**: `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c`

```c
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t sht_dev = NULL;

static void i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_8,
        .scl_io_num = GPIO_NUM_9,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&bus_cfg, &i2c_bus);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x44,  // SHT3x default
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(i2c_bus, &dev_cfg, &sht_dev);
}

static esp_err_t sht3x_read(float *temp, float *humidity)
{
    const uint8_t cmd[2] = {0x2C, 0x06};
    uint8_t buf[6] = {0};

    esp_err_t err = i2c_master_transmit(sht_dev, cmd, sizeof(cmd), 100);
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(20));

    err = i2c_master_receive(sht_dev, buf, sizeof(buf), 100);
    if (err != ESP_OK) return err;

    // CRC validation (omitted for brevity)
    uint16_t raw_t = (buf[0] << 8) | buf[1];
    uint16_t raw_rh = (buf[3] << 8) | buf[4];

    *temp = -45.0f + 175.0f * ((float)raw_t / 65535.0f);
    *humidity = 100.0f * ((float)raw_rh / 65535.0f);
    return ESP_OK;
}
```

## SHT3x Implementation Reference

### Complete SHT3x Driver Pattern

**Source**: `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c`

**Key Functions**:
1. `sht_crc8()` - CRC-8 validation (polynomial 0x31)
2. `sht3x_read()` - Single-shot measurement
3. Timer callback → Queue → Task pattern for periodic reading

**Measurement Command**:
- `0x2C, 0x06` - Single-shot, high repeatability, clock stretching disabled
- Measurement time: ~15ms typical
- Response: 6 bytes (temp MSB, temp LSB, temp CRC, hum MSB, hum LSB, hum CRC)

**Temperature Conversion**:
```c
uint16_t raw_temp = (data[0] << 8) | data[1];
float temp_c = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);
```

**Humidity Conversion**:
```c
uint16_t raw_hum = (data[3] << 8) | data[4];
float humidity = 100.0f * ((float)raw_hum / 65535.0f);
```

## Integration Architecture

### Task-Based Architecture (Recommended)

**Pattern**: Timer → Queue → Task → BLE Notification

```
esp_timer (periodic)
    ↓ (event)
FreeRTOS Queue (poll_evt_t)
    ↓ (dequeue)
I2C Poll Task
    ↓ (read sensor)
FreeRTOS Queue (sample_t)
    ↓ (dequeue)
BLE Notification Task
    ↓ (send)
BLE GATT Notify
```

**Benefits**:
- Non-blocking I2C operations
- Decoupled sensor reading from BLE transmission
- Easy to adjust sample rate
- Can buffer samples if BLE is temporarily unavailable

### Simple Polling Architecture (Alternative)

**Pattern**: Single task with delay

```c
void sensor_task(void *arg)
{
    while (1) {
        float temp;
        sht3x_read(&temp, NULL);
        
        if (ble_connected) {
            ble_notify_temperature(temp);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1Hz
    }
}
```

**Benefits**:
- Simpler code
- Lower memory (no queues)

**Drawbacks**:
- Blocking I2C can delay BLE events
- Less flexible

## Cardputer Pin Assignment Research Needed

**Critical**: Verify I2C pins for Cardputer before implementation.

**Research Steps**:
1. Check Cardputer schematic/datasheet
2. Check M5Stack Cardputer documentation
3. Check existing Cardputer projects for I2C usage
4. Use I2C scanner to probe bus if needed

**Likely Candidates** (ESP32-S3 typical):
- I2C_NUM_0: GPIO8 (SDA), GPIO9 (SCL) - Common default
- I2C_NUM_1: GPIO10 (SDA), GPIO11 (SCL) - Alternative

## Data Format for BLE Transmission

### Option 1: Fixed-Point Integer (Recommended)

**Format**: `int16_t` (2 bytes)
- Units: 0.01°C
- Example: 2500 = 25.00°C, -500 = -5.00°C
- Range: -327.68°C to +327.67°C (sufficient)

**Pros**:
- No floating-point conversion on host
- Predictable size (2 bytes)
- Simple encoding/decoding

**Cons**:
- Limited precision (0.01°C, but sufficient)

### Option 2: IEEE 754 Float

**Format**: `float` (4 bytes)
- Standard IEEE 754 single precision
- Range: -3.4e38 to +3.4e38
- Precision: ~7 decimal digits

**Pros**:
- Native float representation
- Higher precision

**Cons**:
- Larger payload (4 bytes vs 2 bytes)
- Endianness considerations
- Host must parse float

### Recommendation: Fixed-Point Integer

Use `int16_t` for temperature (2 bytes, 0.01°C units) to minimize BLE payload size and simplify host-side parsing.

## Related Files

- `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c` - SHT3x implementation reference
- `esp32-s3-m5/0004-i2c-rolling-average/README.md` - Example documentation
- `ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp` - I2C initialization pattern (M5Unified wrapper)

## External References

- [SHT3x Datasheet](https://sensirion.com/products/catalog/SHT3x-DIS/)
- [ESP-IDF I2C Driver (v4.4)](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/peripherals/i2c.html)
- [ESP-IDF I2C Master Driver (v5.x)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c_master.html)

## Next Steps

1. **Verify Cardputer I2C pins** (critical)
2. **Choose ESP-IDF version compatibility** (v4.4.6 legacy API vs v5.x new API)
3. **Implement SHT3x driver** (adapt from example)
4. **Integrate with BLE notification task**
5. **Test sensor reading accuracy**
6. **Implement sample rate control**
