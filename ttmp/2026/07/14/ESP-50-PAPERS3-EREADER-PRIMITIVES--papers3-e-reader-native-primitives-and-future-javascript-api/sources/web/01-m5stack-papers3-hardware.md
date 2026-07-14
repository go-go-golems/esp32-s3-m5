```
English
```

[![pdf-icon](https://docs.m5stack.com/assets/pdf.svg)](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static.pdf)

CONTENTS

## PaperS3

SKU:C139

## Description

**PaperS3** is a uniquely designed low-power touch-enabled e-ink screen main control device launched by M5Stack. The controller uses the **ESP32-S3**. The front of the device is embedded with a **960 x 540 @ 4.7"** (touch and screen integrated) e-ink display, supporting **16-level grayscale display**. Paired with the **GT911** capacitive touch panel, it supports two-point touch and various gesture operations. Compared to previous products, PaperS3 adopts a full-screen structure. Compared to ordinary LCD screens, e-ink screens not only provide users with a better text reading experience but also offer advantages such as low power consumption and image retention when powered off.

In terms of functional configuration, it integrates a **gyroscope sensor, onboard buzzer, and physical buttons**, enabling interactive operations such as wake-up on lift and power on/off operations. For data storage, PaperS3 is equipped with a **microSD** interface, and the **ESP32-S3R8** chip comes with **8MB** of **PSRAM**. Additionally, it has an external **16MB** flash memory chip for expanded storage, ensuring higher storage capacity and faster data access speeds.

It comes with a built-in **1800mAh** lithium battery and charging functionality, combined with the internal **RTC (BM8563)**, enabling sleep and wake-up functions, providing the device with strong battery life. At the same time, the onboard **battery detection circuit** can monitor the battery status in real-time, ensuring battery health management.

The back of the device features an open **HC1.25-4PLT** peripheral interface, which can be used to expand various sensor devices, offering endless possibilities for subsequent application development. It also supports **OTG functionality**, providing more options for connecting external devices and data exchange.

In terms of performance optimization, compared to previous products, PaperS3 has enhanced antenna performance, resulting in better wireless performance and signal stability. In design, it features a **hanging ear design**, making it convenient for users to carry and hang, enhancing the product's convenience and practicality, and the overall device is thinner. Additionally, PaperS3 has a **magnetic function**, allowing users to fix the device on metal surfaces, further enhancing the convenience and flexibility of device usage.

PaperS3 is suitable for various low-power display and interactive application scenarios such as IoT monitoring, smart home control, environmental monitoring, health monitoring, electronic labels, and data logging.

## Tutorial

![Arduino IDE](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/assets/img/arduino/arduino_banner_01.png) [Arduino IDE](https://docs.m5stack.com/en/arduino/m5papers3/program)

[Arduino IDE](https://docs.m5stack.com/en/arduino/m5papers3/program)

This tutorial will introduce you to how to program and control the PaperS3 device via Arduino IDE

![UiFlow2](https://static-cdn.m5stack.com/resource/docs/static/assets/img/uiflow2/uiflow2.0_banner_01.png) [UiFlow2](https://docs.m5stack.com/en/uiflow2/m5papers3/program)

[UiFlow2](https://docs.m5stack.com/en/uiflow2/m5papers3/program)

This tutorial will introduce you to how to control the PaperS3 device via the UiFlow2 graphical programming platform

## Note

The back structure of the PaperS3 uses a large flat surface molding process. Due to the material characteristics, slight natural bending may occur. This is a normal structural feature, not a battery issue, and does not affect the product’s functionality or usage safety.

## Features

- ESP32-S3R8 SoC
- Built-in gyroscope, buzzer, physical buttons
- 8MB PSRAM, 16MB external flash
- 4.7" touch e-ink screen 960×540 resolution
- Back magnetic attachment
- 1800mAh lithium battery
- RTC chip sleep wake-up
- Low power consumption
- Development platforms:
	- UiFlow2
		- Arduino IDE
		- ESP-IDF
		- PlatformIO

## Includes

- 1 x PaperS3

## Applications

- IoT monitoring
- Smart home control panel
- Electronic labels
- Education and learning tools

## Specifications

| Specification | Parameter |
| --- | --- |
| SoC | ESP32S3R8@Xtensa 32-bit LX7 Dual-Core Processor, Main Frequency 240MHz |
| Flash | 16MB |
| PSRAM | 8MB Quad |
| Wi-Fi | 2.4 GHz Wi-Fi |
| Storage | Supports microSD card expansion |
| Display | 4.7" touch e-ink screen (full screen) @EPD\_ED047TC1   Resolution: 960x540 pixels   16-level grayscale display |
| Touch Function | Supports two-point touch and various gesture operations (GT911 capacitive touch panel) |
| Sensor | Built-in gyroscope sensor BMI270 @ communication address: 0x68 |
| USB Function | OTG/CDC/MSC/Firmware Flashing |
| Power Input | 5V@500mA |
| Peripheral Interface | HC1.25-4PLT (3v3 + GND + 2 x GPIO) peripheral interface (used for expanding sensors and devices), specification: 1 x 4P 1.25 pitch |
| Battery | 3.7V@1800mAh lithium battery @ charging chip: LGS4056H |
| Battery Interface | HY1.25-2P |
| Charging Current | DC 5V@331.5mA |
| Power Management | PMS150G (power on/off and program download control)   Built-in BM8563 RTC chip (supports sleep and wake-up functions) @ communication address: 0x51 |
| Button | 1x physical button (for device control, power on/off, reset, download mode) |
| Buzzer | Onboard passive buzzer |
| Wi-Fi | Communication distance 111 meters (open area, antenna perpendicular to the ground at 90°) |
| Power Consumption | Low power mode: DC4.2V/9.28uA (main power off, gyroscope in low power mode)   Standby mode: DC4.2V/949.58uA (main power off, gyroscope on)   Operating mode: DC4.2V/154.02mA (main power on) |
| Operating Temperature | 0 ~ 40°C |
| Product Size | 121.5 x 67.7 x 7.7mm |
| Product Weight | 89.0g |
| Packaging Dimensions | 132.2 x 77.4 x 19.7mm |
| Gross Weight | 111.7g |

## Learn

### Power On/Off

Click the side button to power on, double-click the side button to power off.

### Download Mode

Connect the device to a computer via USB cable, long press the power button on the M5PaperS3, when the back status light flashes red, it indicates the device has entered download mode.

### IMU Triaxial Direction Schematic Diagram

![](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/IMU-PaperS3.jpg)

### Version Information

PaperS3 v1.1, v1.2, and later versions can check the version information through the sticker on the back of the product; the v1.0 version sticker does not print version information.

## Certifications

- CE/FCC/MIC certification

## Schematics

- [PaperS3 Schematics PDF](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/sch_papers3_V1.0.pdf)
![schematics](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/sch_papers3_V1.0_page_01.png)

## PinMap

### EPD\_ED047TC1

| EPD\_ED047TC1 | ESP32S3R8 |
| --- | --- |
| DB0 | G6 |
| DB1 | G14 |
| DB2 | G7 |
| DB3 | G12 |
| DB4 | G9 |
| DB5 | G11 |
| DB6 | G8 |
| DB7 | G10 |
| XSTL | G13 |
| XLE | G15 |
| SPV | G17 |
| CKV | G18 |
| PWR | G45 |

### GT911 & BM8563 & BMI270 & BAT\_ADC & BUZZER

| ESP32S3R8 | G41 | G42 | G48 | PMS150GU06-PA6/CIN- | G3 | G21 |
| --- | --- | --- | --- | --- | --- | --- |
| GT911 | SDA | SCL | INT |  |  |  |
| BM8563 | SDA | SCL |  | INT |  |  |
| Battery Detect |  |  |  |  | ADC\_VBAT |  |
| Buzzer |  |  |  |  |  | BUZ\_PWM |

### microSD

| microSD | CS | SCK | MOSI | MISO |
| --- | --- | --- | --- | --- |
| ESP32S3R8 | G47 | G39 | G38 | G40 |

### USB Power Supply Detection

| ESP32S3R8 | G5 |
| --- | --- |
| USB Power Supply Detection | USB\_DET |

When the voltage at the USB\_DET pin exceeds 0.2V, it indicates that the USB is connected and supplying power. This detection mechanism ensures that the device correctly recognizes the USB power status.

### HC1.25-4PLT Grove

| HC1.25-4PLT | Black | Red | Yellow | White |
| --- | --- | --- | --- | --- |
| PORT.CUSTOM | GND | 3V3 | G1 | G2 |

## Model Size

[PaperS3 Model Size PDF](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/C139.pdf)

![](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/C139_page_01.png)

## Structure

- [PaperS3 Structure Files](https://github.com/m5stack/M5_Hardware/tree/master/Products/C139_PaperS3/Structures)

## Datasheets

- [ESP32-S3](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/472/esp32-s3_datasheet_en.pdf)
- [GT911](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/m5paper/gt911_datasheet.pdf)
- [EPD\_ED047TC1](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/C139_ED047TC1_datasheet.pdf)

## Softwares

PaperS3 Development Notes

1\. PSRAM needs to be enabled  
2\. PSRAM needs to be set to Octal mode  
3\. Requires EPDIY library version 2.0.0 or higher

### Arduino

- [PaperS3 Arduino Quick Start](https://docs.m5stack.com/en/arduino/m5papers3/program)
- [PaperS3 Arduino M5Unified Driver Library](https://github.com/m5stack/M5Unified)
- [PaperS3 Arduino M5GFX Driver Library](https://github.com/m5stack/M5GFX)

### UiFlow2

- [PaperS3 UiFlow2 Quick Start](https://docs.m5stack.com/en/uiflow2/m5papers3/program)

### ESP-IDF

- [PaperS3 Factory Firmware](https://github.com/m5stack/M5PaperS3-UserDemo)

### PlatformIO

- PlatformIO Configuration

```bash
[env:PaperS3]
platform = espressif32
board = esp32-s3-devkitm-1
framework = arduino
board_build.partitions = default_16MB.csv
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.arduino.memory_type = qio_opi
build_flags =
    -DESP32S3
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=5
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
lib_deps =
    epdiy=https://github.com/vroland/epdiy.git#d84d26ebebd780c4c9d4218d76fbe2727ee42b47
    M5Unified=https://github.com/m5stack/M5Unified
```

### Easyloader

| Easyloader | Download Link | Notes |
| --- | --- | --- |
| PaperS3 Factory Firmware | [download](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/core/PaperS3/PaperS3%20Factory%20Firmware.exe) | / |

## Video

- PaperS3 Product Introduction and Case Demonstration

<video controls=""><source src="https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/core/PaperS3/papers3%20video.mp4" type="video/mp4"></video>

## Product Comparison

| Product Compare | [PaperS3](https://docs.m5stack.com/en/core/PaperS3) ![PaperS3](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/core/PaperS3/12.webp) | [Paper](https://docs.m5stack.com/en/core/m5paper_v1.1) ![Paper V1.1](https://static-cdn.m5stack.com/resource/docs/products/core/m5paper_v1.1/m5paper_v1.1_03.webp) |
| --- | --- | --- |
| Chip Solution | ESP32-S3R8 | ESP32-D0WDQ6-V3 |
| E-ink Screen Control | ESP32S3R8 direct drive | IT8951 |
| Sensor | BMI270, BM8563 | BM8653 |
| Screen | Full screen | Bezel screen |
| Power On/Off Method | Click the side button to power on, double-click to power off, download mode: long press the button, the red light at the bottom right of the back flashes to enter download mode. | PWR button (press the dial switch) as the power button (long press 2s), if you need to power off the device, you need to use the software API or press the reset button on the back. Download program: install the driver, recognize the port before downloading the program |
| Storage | Flash:16MB PSRAM:8MB | Flash:16MB PSRAM:8MB |
| Battery | 1800mAh | 1150mAh |
| Download Connection | Direct connection to ESP32S3, automatic port recognition | CH9102 serial chip |
| Antenna | Upgraded antenna, more stable signal | First-generation antenna |
| Dimensions | 121.5 x 67.7 x 7.7mm | 118 x 66 x 10mm |

To compare information on the Paper / CoreInk series products, you can visit the [Product Selection Table](https://docs.m5stack.com/en/products_selector/m5paper_compare?select=C139), check the target products, and get the comparison results. The selection table covers key information such as core parameters and functional features, and supports comparison of multiple products simultaneously.