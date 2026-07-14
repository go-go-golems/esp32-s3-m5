[ESPHome](https://community.home-assistant.io/c/esphome/36)

Hi everyone,

I wanted to share my updated fork of the **epdiy library** that now works with **ESP-IDF 5.5.1**
and the **M5Stack PaperS3** on ESPHome. The original library had several compilation errors with
the latest ESP-IDF version, which I’ve fixed.

### What Was Fixed

ESP-IDF 5.5.1 introduced breaking changes that prevented the epdiy library from compiling:

1. **`gpio_hal_iomux_func_sel` removal** - Replaced with inline function using `PIN_FUNC_SELECT`
2. **`__DECLARE_RCC_ATOMIC_ENV` macro removal** - Added compatibility shim for the removed RCC
atomic environment macro
3. **GDMA API deprecations** - The old API still works but shows warnings (harmless)

All fixes maintain **backward compatibility** with ESP-IDF 4.x and 5.0-5.4.

### Tested and Working

- **ESP-IDF**: 5.5.1 (also works with 4.x and 5.0-5.4)
- **ESPHome**: 2024.11.0+
- **Hardware**: M5Stack PaperS3 (ESP32-S3 with 4.7" e-paper display)
- **Home Assistant**: Full integration via ESPHome text entities

### Repository

**[GitHub - Frogy76/epdiy · GitHub](https://github.com/Frogy76/epdiy)**

### Quick Start Example

Here’s a minimal ESPHome configuration to get started:

```
esphome:
  name: m5papers3
  libraries:
    - epidy=https://github.com/Frogy76/epdiy

external_components:
  - source: github://patrick3399/esphome_components

esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf
    version: latest  # ESP-IDF 5.5.1 works!

psram:
  mode: octal
  speed: 80MHz

display:
  - platform: ed047tc1
    id: epaper_display
    # M5Stack PaperS3 pin configuration
    pwr_pin: GPIO45
    bst_en_pin: GPIO46
    xstl_pin: GPIO13
    xle_pin: GPIO15
    spv_pin: GPIO17
    ckv_pin: GPIO18
    pclk_pin: GPIO16
    d0_pin: GPIO6
    d1_pin: GPIO14
    d2_pin: GPIO7
    d3_pin: GPIO12
    d4_pin: GPIO9
    d5_pin: GPIO11
    d6_pin: GPIO8
    d7_pin: GPIO10
    rotation: 90
    update_interval: never
    lambda: |-
      it.print(240, 50, id(my_font), TextAlign::CENTER, id(line1).state.c_str());
      it.print(240, 150, id(my_font), TextAlign::CENTER, id(line2).state.c_str());

font:
  - file: "gfonts://Roboto"
    id: my_font
    size: 48

text:
  - platform: template
    name: "Display Line 1"
    id: line1
    optimistic: true
    max_length: 30
    on_value:
      - component.update: epaper_display

  - platform: template
    name: "Display Line 2"
    id: line2
    optimistic: true
    max_length: 30
    on_value:
      - component.update: epaper_display
```

### Control from Home Assistant

Once the device is added to Home Assistant, you can update the display using services:

**Via UI:**

- Go to Developer Tools → Services
- Select `text.set_value`
- Choose entity: `text.display_line_1`
- Enter your text

**Via Automation:**

```
service: text.set_value
target:
  entity_id: text.display_line_1
data:
  value: "Temperature: {{ states('sensor.living_room_temperature') }}°C"
```

**Example Dashboard Card:**

```
type: entities
entities:
  - entity: text.display_line_1
  - entity: text.display_line_2
title: E-Paper Display Control
```

### Full Example

A complete working example with 12 text lines, touchscreen, battery monitoring, and RTC is
available in the repository:
[papers3.yaml](https://github.com/Frogy76/epdiy/blob/5640651b18f36005c280d9382da837b2b1c09dbf/papers
3.yaml)

### Use Cases

- **Information Display**: Weather, calendar, to-do lists
- **Sensor Dashboard**: Room temperatures, energy usage
- **Notifications**: Doorbell alerts, delivery notifications
- **Status Board**: Home Assistant automation status

The e-paper display is perfect for low-power, always-visible information that doesn’t need
frequent updates.

### Tips

- Set `update_interval: never` and trigger updates manually via `component.update`
- Use a `system_initialized` flag to prevent updates during boot
- The display has excellent readability in bright light (no backlight)
- Battery life is excellent due to e-paper’s ultra-low power consumption

Hope this helps anyone trying to use M5Stack PaperS3 with the latest ESPHome/ESP-IDF!

[ESP32-S3 compatibily and M5 Paper
S3](https://community.home-assistant.io/t/esp32-s3-compatibily-and-m5-paper-s3/876573/11)

Followed your Quick Start Example and got the response:

INFO ESPHome 2025.11.0
INFO Reading configuration /config/esphome/display1.yaml…
INFO Unable to import component ed047tc1.display: No module named ‘esphome.components.ed047tc1’

Failed config

Platform not found: ‘display.ed047tc1’

You are missing:

```
external_components:
  - source: github://patrick3399/esphome_components
```

Thanks I updated the Quick Start Example

Just wanted to say thanks for this, it got me started once I’d added the wifi credentials etc.

I am looking for an e-paper battery-powered device that can show a touchable HA dashboard (see
pic). I went down the Kindle rabbit hole, and it was a fail because battery life would only last
24-36 hours… so long story short, is this epdiy fork what I am looking for? Will an M5Stack
PaperS3 work on ESPHome as an interactive dashboard? Sorry if this is a stupid question, but I
don’t want to buy any more stuff until I know it will work like I (kind of) want it to.

Thx

[![Bildschirmfoto 2026-01-11 um
15.30.00](https://community-assets.home-assistant.io/optimized/4X/3/8/4/3841baa22272ad8af6376c7ad123
51b6ee714582_2_567x500.jpeg)](https://community-assets.home-assistant.io/original/4X/3/8/4/3841baa22
272ad8af6376c7ad12351b6ee714582.jpeg "Bildschirmfoto 2026-01-11 um 15.30.00")

i get the following error during compiling:

```yaml
src/esphome/components/ed047tc1/ed047tc1.cpp: In member function 'virtual void
esphome::ed047tc1::ED047TC1Display::dump_config()':
src/esphome/components/ed047tc1/ed047tc1.cpp:208:58: error: 'get_pin_name' was not declared in this
scope
  208 |         if (d_pins_[i] != nullptr && hasattr(d_pins_[i], get_pin_name)) {
      |                                                          ^~~~~~~~~~~~
src/esphome/components/ed047tc1/ed047tc1.cpp:208:38: error: 'hasattr' was not declared in this scope
  208 |         if (d_pins_[i] != nullptr && hasattr(d_pins_[i], get_pin_name)) {
      |                                      ^~~~~~~
In file included from src/esphome/core/component.h:9,
                 from src/esphome/components/ed047tc1/ed047tc1.h:3,
                 from src/esphome/components/ed047tc1/ed047tc1.cpp:1:
src/esphome/components/ed047tc1/ed047tc1.cpp:209:65: error: 'class esphome::GPIOPin' has no member
named 'get_pin_name'
  209 |              ESP_LOGCONFIG(TAG, "  D%d Pin: %s", i, d_pins_[i]->get_pin_name().c_str());
      |                                                                 ^~~~~~~~~~~~
src/esphome/core/log.h:98:101: note: in definition of macro 'esph_log_config'
   98 |   ::esphome::esp_log_printf_(ESPHOME_LOG_LEVEL_CONFIG, tag, __LINE__,
ESPHOME_LOG_FORMAT(format), ##__VA_ARGS__)
      |
        ^~~~~~~~~~~
src/esphome/components/ed047tc1/ed047tc1.cpp:209:14: note: in expansion of macro 'ESP_LOGCONFIG'
  209 |              ESP_LOGCONFIG(TAG, "  D%d Pin: %s", i, d_pins_[i]->get_pin_name().c_str());
      |              ^~~~~~~~~~~~~
*** [.pioenvs/m5paper/src/esphome/components/ed047tc1/ed047tc1.cpp.o] Error 1
========================= [FAILED] Took 191.84 seconds =========================
```

yeah, unfortunately this seems to have stopped working. I get the same error messages

I had the same Problem.
I´ve tried to compile with an older ESPHome Version (2025.8) and it worked.
[Legacy ESPHome versions
add-on](https://community.home-assistant.io/t/legacy-esphome-versions-add-on/568448)

```
for (int i=0; i<8; ++i) {
    #if ESPHOME_VERSION_CODE >= VERSION_CODE(2023, 1, 0)
    if (d_pins_[i] != nullptr) {
        ESP_LOGCONFIG(TAG, "  D%d Pin: Configured (Pin: %d)", i,
esphome_pin_to_gpio_num(d_pins_[i]));
    } else { ESP_LOGCONFIG(TAG, "  D%d Pin: NONE", i); }
    #else
    ESP_LOGCONFIG(TAG, "  D%d Pin: %s", i, d_pins_[i] ? "Configured" : "NONE");
    #endif
}
```

as temp solution

I’m not getting compilation errors with ESPHome 2026.2.4, though after flashing and booting
nothing seems to happen.
This is my config:

```yaml
esphome:
  name: m5papers3-office
  friendly_name: "M5 Paper S3 Dashboard"
  platformio_options:
    build_flags: "-DBOARD_HAS_PSRAM"
  libraries:
    - epidy=https://github.com/Frogy76/epdiy
  on_boot:
    then:
      - rtttl.play: 'siren:d=8,o=5,b=100:d,e,d,e,d,e,d,e'
      - delay: 5s
      - component.update: ed047tc1_display
      - lambda: |-
          id(system_initialized) = true;

esp32:
  board: esp32-s3-devkitc-1
  flash_size: 16MB
  framework:
    type: esp-idf
    version: latest
    sdkconfig_options:
      CONFIG_EPD_OUTPUT_LUT_ASSEMBLY: "y"
      CONFIG_ESP32S3_INSTRUCTION_SET_AI_DSP: "y"
      CONFIG_ESP32S3_VECTOR_INSTRUCTIONS_SUPPORT: "y"

globals:
  - id: system_initialized
    type: bool
    restore_value: no
    initial_value: 'false'

psram:
  mode: octal
  speed: 80MHz

interval:
  - interval: 8s
    then:
      - light.turn_on: statled
      - delay: 1s
      - light.turn_off: statled


logger:
  level: DEBUG
  baud_rate: 0
  logs:
    api.service: WARN
    wifi: WARN
    esp-idf: NONE
    light: WARN
debug:
  update_interval: 10s

api:
  encryption:
    key: !secret api_key
  reboot_timeout: 6h

ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  reboot_timeout: 6h
  #power_save_mode: LIGHT

external_components:
  - source: github://patrick3399/esphome_components
  # - source: github://n-serrette/esphome_sd_card   #If need SD Card Support
i2c:
  sda: GPIO41
  scl: GPIO42
  scan: true
  id: bus_internal
  frequency: 200kHz

# sd_mmc_card:  #If need SD Card Support
#   id: micro_sd
#   mode_1bit: true
#   clk_pin: GPIO7
#   cmd_pin: GPIO8
#   data0_pin: GPIO9
#   data3_pin: GPIO10

# sd_file_server:  #If need SD Card Support
#   id: file_server
#   url_prefix: file
#   root_path: "/"
#   enable_deletion: true
#   enable_download: true
#   enable_upload: true

touchscreen:
  platform: gt911
  id: my_touchscreen
  interrupt_pin: GPIO48
  transform:
    mirror_x: true
    mirror_y: true
    swap_xy: false
  on_touch:
    - lambda: |-
          ESP_LOGI("cal", "x=%d, y=%d, x_raw=%d, y_raw=%0d",
              touch.x,
              touch.y,
              touch.x_raw,
              touch.y_raw
              );
light:
  - platform: monochromatic
    id: statled
    name: "Status LED"
    output: gpio_0
    restore_mode: ALWAYS_OFF
    internal: True
    disabled_by_default: True

output:
  - platform: ledc
    id: rtttl_pin
    pin: 21
  - platform: ledc
    id: gpio_0
    pin: 0
    inverted: True
rtttl:
  output: rtttl_pin
  id: rtttl_buzz
  gain: 60%

text_sensor:
  - platform: debug
    device:
      name: "Device Info"
    reset_reason:
      name: "Reset Reason"

sensor:
  - platform: debug
    free:
      name: "Heap Free"
    block:
      name: "Heap Max Block"
    loop_time:
      name: "Loop Time"
    psram:
      name: "Free PSRAM"
  - platform: internal_temperature
    name: "Chip Temp"
  - platform: adc
    pin: GPIO3
    name: "Battery Voltage"
    update_interval: 30s
    attenuation: 12db
    filters:
      - multiply: 2.0
  - platform: adc
    pin: GPIO5
    name: "USB DET"
    update_interval: 30s
    attenuation: 12db
    filters:
      - multiply: 2.0
  #- platform: bmi270
  #  address: 0x68
  #  accel_x:
  #    name: "BMI270 Accel X"
  #  accel_y:
  #    name: "BMI270 Accel Y"
  #  accel_z:
  #    name: "BMI270 Accel Z"
  #  gyro_x:
  #    name: "BMI270 Gyro X"
  #  gyro_y:
  #    name: "BMI270 Gyro Y"
  #  gyro_z:
  #    name: "BMI270 Gyro Z"
  #  temperature:
  #    name: "BMI270 Temperature"
  #  power_save_mode: LOW_POWER # OR NORMAL
  #  update_interval: 60s
    # Add new configuration options specific to BMI270 if needed
    # For example, accelerometer and gyroscope range selection:
    # accel_range: '8g' # Options: '2g', '4g', '8g', '16g' [cite: 5]
    # gyro_range: '2000dps' # Options: '125dps', '250dps', '500dps', '1000dps', '2000dps' [cite: 5]

  # - platform: sd_mmc_card  #If need SD Card Support
  #   type: used_space
  #   name: "SD card used space"
  # - platform: sd_mmc_card
  #   type: total_space
  #   name: "SD card total space"

# text_sensor:
  # - platform: sd_mmc_card  #If need SD Card Support
  #   sd_card_type:
  #     name: "SD card type"

binary_sensor:
  - platform: gpio
    pin:
      number: 4
      inverted: True
    name: "Charge Status"
    device_class: battery_charging

time:
  - platform: pcf8563  #BM8563
    id: pcf8563_time
    address: 0x51
    timezone: Asia/Taipei
    update_interval: never

  - platform: homeassistant
    id: homeassistant_time
    timezone: Asia/Taipei
    update_interval: 1h
    on_time_sync:
      then:
        pcf8563.write_time:
          id: pcf8563_time

display:
  - platform: ed047tc1
    id: ed047tc1_display
    pwr_pin: GPIO45
    bst_en_pin: GPIO46
    xstl_pin: GPIO13
    xle_pin: GPIO15
    spv_pin: GPIO17
    ckv_pin: GPIO18
    pclk_pin: GPIO16
    d0_pin: GPIO6
    d1_pin: GPIO14
    d2_pin: GPIO7
    d3_pin: GPIO12
    d4_pin: GPIO9
    d5_pin: GPIO11
    d6_pin: GPIO8
    d7_pin: GPIO10
    update_interval: never
    rotation: 90
    lambda: |-
       it.print(it.get_width() / 2, it.get_height() / 12 *1 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline01).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *2 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline02).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *3 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline03).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *4 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline04).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *5 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline05).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *6 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline06).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *7 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline07).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *8 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline08).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *9 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline09).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *10 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline10).state.c_str());
       it.print(it.get_width() / 2, it.get_height() / 12 *11 , id(Roboto70),
TextAlign::BOTTOM_CENTER, id(textline11).state.c_str());
       it.print(it.get_width() / 2, it.get_height() , id(Roboto70), TextAlign::BOTTOM_CENTER,
id(textline12).state.c_str());
font:
  - file: "gfonts://Roboto"
    id: Roboto70
    size: 70

text:
  - platform: template
    name: "Text Line 01"
    id: textline01
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 01'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 02"
    id: textline02
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 02'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 03"
    id: textline03
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 03'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 04"
    id: textline04
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 04'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 05"
    id: textline05
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 05'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 06"
    id: textline06
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 06'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 07"
    id: textline07
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 07'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 08"
    id: textline08
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 08'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 09"
    id: textline09
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 09'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 10"
    id: textline10
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 10'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 11"
    id: textline11
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 11'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
  - platform: template
    name: "Text Line 12"
    id: textline12
    optimistic: true
    min_length: 0
    max_length: 26
    initial_value: 'TEXT 12'
    mode: text
    on_value:
      then:
        - if:
            condition:
              lambda: 'return id(system_initialized) == true;'
            then:
              - component.update: ed047tc1_display
```

And when I look at the logs and reset the device, this is as far as it gets:

```
[15:12:36]ESP-ROM:esp32s3-20210327
[15:12:36]Build:Mar 27 2021
[15:12:36]rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
[15:12:36]Saved PC:0x40377ef2
[15:12:36]SPIWP:0xee
[15:12:36]mode:DIO, clock div:1
[15:12:36]load:0x3fce2820,len:0x158c
[15:12:36]load:0x403c8700,len:0xd24
[15:12:36]load:0x403cb700,len:0x2f28
[15:12:36]entry 0x403c891c
[15:12:36]I (24) boot: ESP-IDF 5.5.2 2nd stage bootloader
[15:12:36]I (24) boot: compile time Mar 10 2026 13:12:56
[15:12:36]I (25) boot: Multicore bootloader
[15:12:36]I (25) boot: chip revision: v0.2
[15:12:36]I (27) boot: efuse block revision: v1.3
[15:12:36]I (31) boot.esp32s3: Boot SPI Speed : 80MHz
[15:12:36]I (35) boot.esp32s3: SPI Mode       : DIO
[15:12:36]I (39) boot.esp32s3: SPI Flash Size : 4MB
[15:12:36]I (42) boot: Enabling RNG early entropy source...
[15:12:36]I (47) boot: Partition Table:
[15:12:36]I (49) boot: ## Label            Usage          Type ST Offset   Length
[15:12:36]I (56) boot:  0 otadata          OTA data         01 00 00009000 00002000
[15:12:36]I (62) boot:  1 phy_init         RF data          01 01 0000b000 00001000
[15:12:36]I (69) boot:  2 app0             OTA app          00 10 00010000 001c0000
[15:12:36]I (75) boot:  3 app1             OTA app          00 11 001d0000 001c0000
[15:12:36]I (82) boot:  4 nvs              WiFi data        01 02 00390000 0006d000
[15:12:36]I (88) boot: End of partition table
[15:12:36]I (92) esp_image: segment 0: paddr=00010020 vaddr=3c030020 size=1a7f8h (108536) map
[15:12:36]I (118) esp_image: segment 1: paddr=0002a820 vaddr=3fc90300 size=02e44h ( 11844) load
[15:12:36]I (121) esp_image: segment 2: paddr=0002d66c vaddr=40374000 size=029ach ( 10668) load
[15:12:36]I (125) esp_image: segment 3: paddr=00030020 vaddr=42000020 size=2321ch (143900) map
[15:12:36]I (155) esp_image: segment 4: paddr=00053244 vaddr=403769ac size=09878h ( 39032) load
[15:12:36]I (164) esp_image: segment 5: paddr=0005cac4 vaddr=50000000 size=00020h (    32) load
[15:12:36]I (170) boot: Loaded app from partition at offset 0x10000
[15:12:36]I (170) boot: Disabling RNG early entropy source...
```

The screen is totally blank

Okay scratch that! I guess my device wasn’t powered down or something? Just turned it on today
after leaving for 24hrs and it booted straight up, played the little song, and is showing all 12
text lines!

Your readme mentions support for other displays such as the ED133UT2, but I’m unable to compile
it with errors such as `Platform not found: 'display.ed133ut2'`. Do you have any examples for other
displays just use the epdiy v7 without a touchscreen, etc?
