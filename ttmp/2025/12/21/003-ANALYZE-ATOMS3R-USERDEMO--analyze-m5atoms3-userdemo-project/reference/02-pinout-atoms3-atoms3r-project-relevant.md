---
Title: 'Pinout: AtomS3 / AtomS3R (project-relevant)'
Ticket: 003-ANALYZE-ATOMS3R-USERDEMO
Status: active
Topics:
    - embedded-systems
    - esp32
    - m5stack
    - arduino
    - analysis
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Project-relevant pinout reference for AtomS3 and AtomS3R, with detailed notes and code patterns for display, I2C, UART, IR, PWM/ADC, and button handling"
LastUpdated: 2025-12-22T09:24:48.872988547-05:00
WhatFor: "Quickly answer 'which GPIO does what?' for this repo’s demos and for M5Unified/M5GFX-driven LCD bring-up"
WhenToUse: "When wiring peripherals, debugging display/backlight/I2C/UART issues, or porting between AtomS3 and AtomS3R"
---

# Pinout: AtomS3 / AtomS3R (project-relevant)

## Goal

Provide a **project-relevant pinout** for the M5Stack **AtomS3** (used by `M5AtomS3-UserDemo`) and **AtomS3R** (included because M5Unified/M5GFX treat it as a distinct board with different display wiring). The structure is:

- **Reference first**: tables you can scan/copy quickly
- **Details**: what each signal means and what the pitfalls are
- **How to interface in code**: copy/paste patterns

## Context

In this repo, “the pinout” is spread across three layers:

- **Demo app code** (`M5AtomS3-UserDemo/src/main.cpp`) defines pins for IR and chooses which Arduino buses/pins to use for scanning, UART bridging, PWM/ADC demos, etc.
- **M5Unified** defines *logical* pins/buses (`M5.In_I2C`, `M5.Ex_I2C`, `M5.BtnA`) and maps them to GPIOs based on board detection.
- **M5GFX (LovyanGFX)** contains the low-level LCD wiring, panel controller detection, offsets, and backlight strategy.

This document focuses on the pins that matter for the demo and for “how the LCD is actually driven”.

## Quick Reference (pin tables)

### AtomS3 (as used by `M5AtomS3-UserDemo`)

#### Display (M5GFX auto-detect)

| Signal | GPIO | Notes |
|---|---:|---|
| LCD MOSI | 21 | SPI data-out to LCD |
| LCD SCLK | 17 | SPI clock to LCD |
| LCD D/C | 33 | Data/Command select |
| LCD CS | 15 | Chip select |
| LCD RST | 34 | Panel reset |
| Backlight | 16 | PWM backlight via `lgfx::Light_PWM` (LEDC ch 7, freq=256, offset=48) |
| Panel | — | **GC9107**, visible 128×128 with `offset_y=32` into 128×160 memory |

#### Internal I2C (`M5.In_I2C`) / IMU bus

| Bus | SDA | SCL | Notes |
|---|---:|---:|---|
| Internal I2C (In_I2C) | 38 | 39 | Demo IMU uses `Wire1.begin(38,39)` |

#### External I2C (`M5.Ex_I2C`) / Grove Port

| Bus | SDA | SCL | Notes |
|---|---:|---:|---|
| External I2C (Ex_I2C / Port.A) | 2 | 1 | Demo I2C scan uses `Wire.begin(2,1)` |

#### UART (Grove ↔ USB bridge demo)

| Mode | Baud | RX | TX | Notes |
|---:|---:|---:|---:|---|
| 0 | 9600 | 2 | 1 | `Serial.begin(baud, SERIAL_8N1, rx, tx)` |
| 1 | 115200 | 2 | 1 |  |
| 2 | 9600 | 1 | 2 | swapped pins |
| 3 | 115200 | 1 | 2 | swapped pins |

#### IR (TX)

| Subsystem | GPIO | Notes |
|---|---:|---|
| IR TX | 4 | RMT channel 2; NEC (`infrared_tools`) |

#### PWM + ADC (demo reuses the same GPIOs)

| Subsystem | GPIO(s) | Notes |
|---|---|---|
| PWM output | 1, 2 | `ledcAttachPin(1,0)` + `ledcAttachPin(2,1)` |
| ADC input | 1, 2 | `analogRead(1)` and `analogRead(2)` |

#### Button

| Button | GPIO | Notes |
|---|---:|---|
| BtnA | 41 | Read by M5Unified; use `M5.BtnA` |

---

### AtomS3R (from M5Unified/M5GFX board support)

AtomS3R is included because M5GFX/M5Unified treat it as a distinct board with different LCD wiring.

#### Display (M5GFX auto-detect)

| Signal | GPIO | Notes |
|---|---:|---|
| LCD MOSI | 21 | SPI data-out to LCD |
| LCD SCLK | 15 | SPI clock to LCD |
| LCD D/C | 42 | Data/Command select |
| LCD CS | 14 | Chip select |
| LCD RST | 48 | Panel reset |
| Backlight | (I2C) | M5GFX uses an I2C-controlled light device (addr 0x30) |
| Panel | — | **GC9107**, visible 128×128 with `offset_y=32` into 128×160 memory |

#### Internal I2C (`M5.In_I2C`)

| Bus | SDA | SCL | Notes |
|---|---:|---:|---|
| Internal I2C (In_I2C) | 45 | 0 | Used by AtomS3R backlight control in M5GFX |

#### External I2C (`M5.Ex_I2C`) / Grove Port

| Bus | SDA | SCL |
|---|---:|---:|
| External I2C (Ex_I2C / Port.A) | 2 | 1 |

#### Button

| Button | GPIO | Notes |
|---|---:|---|
| BtnA | 41 | Same as AtomS3 in M5Unified |

## Details (what each signal is + why it matters)

### Display (SPI LCD + backlight)

For AtomS3 and AtomS3R, M5GFX probes the panel using `RDDID` (0x04) and matches a GC9107 signature; then it creates a `Panel_GC9107` and configures it.

Two practical implications:

- **Offset matters**: GC9107 is configured as a 128×160 memory panel but the visible area is 128×128; M5GFX uses `offset_y = 32`. If you reimplement the display in pure ESP-IDF (`esp_lcd`), this offset is a prime suspect if graphics appear shifted.
- **Backlight differs by board**:
  - AtomS3 uses PWM on GPIO16 (`lgfx::Light_PWM`).
  - AtomS3R uses an I2C-controlled device at address 0x30 (48 decimal); M5GFX writes brightness to register 0x0e after initializing I2C on SDA=45/SCL=0.

### Internal vs external I2C

M5Unified provides:

- `M5.In_I2C`: internal bus for on-board devices (IMU; and for AtomS3R, backlight control)
- `M5.Ex_I2C`: external bus on Port.A / Grove connector

The demo uses Arduino `Wire1` (IMU) and `Wire` (I2C scan) directly; those align with M5Unified’s mapping.

### UART bridging (Grove ↔ USB)

The demo uses Arduino `Serial` with configurable RX/TX pins. The code cycles between RX/TX=(2,1) and RX/TX=(1,2) specifically because TX/RX are commonly swapped in real wiring.

### PWM + ADC pin reuse

GPIO1 and GPIO2 are used by multiple demos (PWM, ADC, and also external I2C/UART modes). This is fine for a single-function demo, but it’s a **real conflict** if you try to enable multiple subsystems at once.

### IR transmitter

IR TX uses ESP-IDF RMT on GPIO4. The demo builds NEC frames via `infrared_tools` and writes the resulting waveform items using `rmt_write_items`.

### Button A

On ESP32-S3 AtomS3-family boards, M5Unified reads BtnA from GPIO41. In application code you should use `M5.BtnA` (debouncing + higher-level events like `wasClicked()`/`wasHold()`).

## How to interface with these pins in code

### Display (recommended: M5Unified/M5GFX)

```cpp
#include <M5Unified.h>

void setup() {
  M5.begin();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.printf("Hello LCD");
}

void loop() {
  M5.update();
}
```

### Canvas / sprite rendering (double-buffer)

```cpp
#include <M5Unified.h>

M5Canvas canvas(&M5.Display);

void setup() {
  M5.begin();
  canvas.setColorDepth(16);
  canvas.createSprite(M5.Display.width(), M5.Display.height() - 30);
  canvas.fillSprite(TFT_BLACK);
  canvas.drawCenterString("Sprite!", canvas.width() / 2, canvas.height() / 2);
  canvas.pushSprite(0, 30);
}
```

### Internal/external I2C (recommended: M5Unified)

```cpp
#include <M5Unified.h>

void setup() {
  M5.begin();
  // M5.In_I2C and M5.Ex_I2C are configured by M5.begin().
  // Use them instead of hard-coded Wire/Wire1 when writing new code.
}
```

### External I2C scan (demo style)

```cpp
#include <Wire.h>

void setup() {
  Wire.begin(/*sda=*/2, /*scl=*/1);
}
```

### UART (ESP32 Arduino)

```cpp
// ESP32 Arduino: Serial.begin(baud, config, rxPin, txPin)
Serial.begin(115200, SERIAL_8N1, /*rx=*/2, /*tx=*/1);
```

### PWM (LEDC) + ADC

```cpp
// PWM (LEDC)
ledcSetup(/*channel=*/0, /*freq=*/1000, /*resolution_bits=*/8);
ledcAttachPin(/*gpio=*/1, /*channel=*/0);
ledcWrite(/*channel=*/0, /*duty=*/128);

// ADC
int raw = analogRead(/*gpio=*/1);
float volts = (raw / 4095.0f) * 3.3f;
```

## Sources (authoritative references)

### Demo application pins (IR, IMU, UART modes)

```16:21:M5AtomS3-UserDemo/src/main.cpp
#define VERSION         0.1
#define LAYOUT_OFFSET_Y 30

#define IR_RMT_TX_CHANNEL RMT_CHANNEL_2
#define IR_GPIO           4
```

```249:276:M5AtomS3-UserDemo/src/main.cpp
class func_uart_t : public func_base_t {
    const unsigned char* uart_mon_img_list[4] = {
        uart_mon_01_img, uart_mon_02_img, uart_mon_03_img, uart_mon_04_img};
    const uint8_t uart_io_list[4][2] = {{2, 1}, {2, 1}, {1, 2}, {1, 2}};
    const uint32_t uart_baud_list[4] = {9600, 115200, 9600, 115200};
    uint8_t uart_mon_mode_index      = 0;

    void start() {
        Serial.begin(uart_baud_list[uart_mon_mode_index], SERIAL_8N1,
                     uart_io_list[uart_mon_mode_index][0],
                     uart_io_list[uart_mon_mode_index][1]);
        M5.Display.drawPng(uart_mon_img_list[uart_mon_mode_index], ~0u, 0, 0);
```

```513:516:M5AtomS3-UserDemo/src/main.cpp
    void start() {
        Wire1.begin(38, 39);
        imu.begin();
        filter.begin(20);  // 20hz
```

### Internal/external I2C mapping (M5Unified pin tables)

```58:72:M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5Unified/src/M5Unified.cpp
static constexpr const uint8_t _pin_table_i2c_ex_in[][5] = {
                            // In CL,DA, EX CL,DA
{ board_t::board_M5AtomS3R    , GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  }, // AtomS3R
{ board_t::board_unknown      , GPIO_NUM_39,GPIO_NUM_38 , GPIO_NUM_1 ,GPIO_NUM_2  }, // AtomS3,AtomS3Lite,AtomS3U
```

### Button A GPIO for AtomS3 family on ESP32-S3

```1277:1305:M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5Unified/src/M5Unified.cpp
#elif defined (CONFIG_IDF_TARGET_ESP32S3)

    switch (_board)
    {
    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
    case board_t::board_M5AtomS3R:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_41)) & 1;
      break;
```

### AtomS3/AtomS3R display wiring + panel config (M5GFX)

```1258:1338:M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5GFX/src/M5GFX.cpp
      if (board == 0 || board == board_t::board_M5AtomS3R)
      {
        _pin_reset(GPIO_NUM_48, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_21;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_15;
        bus_cfg.pin_dc   = GPIO_NUM_42;
        // ...
            cfg.pin_cs  = GPIO_NUM_14;
            cfg.pin_rst = GPIO_NUM_48;
            cfg.panel_width = 128;
            cfg.panel_height = 128;
            cfg.offset_y = 32;
        // ...
          _set_backlight(new Light_M5StackAtomS3R());
      }

      if (board == 0 || board == board_t::board_M5AtomS3)
      {
        _pin_reset(GPIO_NUM_34, use_reset); // LCD RST
        bus_cfg.pin_mosi = GPIO_NUM_21;
        bus_cfg.pin_miso = (gpio_num_t)-1; //GPIO_NUM_NC;
        bus_cfg.pin_sclk = GPIO_NUM_17;
        bus_cfg.pin_dc   = GPIO_NUM_33;
        // ...
            cfg.pin_cs  = GPIO_NUM_15;
            cfg.pin_rst = GPIO_NUM_34;
            cfg.panel_width = 128;
            cfg.panel_height = 128;
            cfg.offset_y = 32;
        // ...
          _set_pwm_backlight(GPIO_NUM_16, 7, 256, false, 48);
      }
```

## Related

- `analysis/01-m5atoms3-userdemo-project-analysis.md`
- `analysis/02-m5unified-m5gfx-deep-dive-display-stack-and-low-level-lcd-communication.md`

## Related

<!-- Link to related documents or resources -->
