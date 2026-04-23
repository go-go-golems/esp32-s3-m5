## 🚫 Deprecated — Use M5GFX & M5Unified

- **M5GFX**  
	High-performance, lightweight graphics and display driver library for M5 devices.  
	[https://github.com/m5stack/M5GFX](https://github.com/m5stack/M5GFX)
- **M5Unified**  
	Unified base library for M5 devices (IO/peripherals, power management, audio, etc.).  
	[https://github.com/m5stack/M5Unified](https://github.com/m5stack/M5Unified)

## ATOM Library

[![Arduino Compile](https://github.com/m5stack/M5Atom/actions/workflows/arduino-action-atom-compile.yml/badge.svg)](https://github.com/m5stack/M5Atom/actions/workflows/arduino-action-paper-compile.yml) [![Arduino Lint](https://github.com/m5stack/M5Atom/actions/workflows/Arduino-Lint-Check.yml/badge.svg)](https://github.com/m5stack/M5Atom/actions/workflows/Arduino-Lint-Check.yml) [![Clang Format](https://github.com/m5stack/M5Atom/actions/workflows/clang-format-check.yml/badge.svg)](https://github.com/m5stack/M5Atom/actions/workflows/clang-format-check.yml)

English | [中文](https://github.com/m5stack/M5Atom/blob/master/README_cn.md)

[![M5Atom Lite](https://camo.githubusercontent.com/2f1ffa3363f1d897c4f4e70111df624e8f27828da957cd5fe90c9d741aa341ac/68747470733a2f2f6d35737461636b2e6f73732d636e2d7368656e7a68656e2e616c6979756e63732e636f6d2f696d6167652f6d352d646f63735f686f6d65706167652f636f72652f61746f6d5f6c6974655f30312e77656270)](https://camo.githubusercontent.com/2f1ffa3363f1d897c4f4e70111df624e8f27828da957cd5fe90c9d741aa341ac/68747470733a2f2f6d35737461636b2e6f73732d636e2d7368656e7a68656e2e616c6979756e63732e636f6d2f696d6167652f6d352d646f63735f686f6d65706167652f636f72652f61746f6d5f6c6974655f30312e77656270)

[![M5Atom Matrix](https://camo.githubusercontent.com/b51248d5e54d4b79922d7c459b7cab0b24d97fbe7b696a2e33f3691ce84ef35d/68747470733a2f2f6d35737461636b2e6f73732d636e2d7368656e7a68656e2e616c6979756e63732e636f6d2f696d6167652f6d352d646f63735f686f6d65706167652f636f72652f61746f6d5f6d61747269785f30312e77656270)](https://camo.githubusercontent.com/b51248d5e54d4b79922d7c459b7cab0b24d97fbe7b696a2e33f3691ce84ef35d/68747470733a2f2f6d35737461636b2e6f73732d636e2d7368656e7a68656e2e616c6979756e63732e636f6d2f696d6167652f6d352d646f63735f686f6d65706167652f636f72652f61746f6d5f6d61747269785f30312e77656270)

- **For the Detailed documentation of ATOM Lite, please [Click here](https://docs.m5stack.com/en/core/atom_lite)**
- **For the Detailed documentation of ATOM Matrix, please [Click here](https://docs.m5stack.com/en/core/atom_matrix)**
- **In order to buy ATOM Lite, please [Click here](https://shop.m5stack.com/collections/m5-controllers/products/atom-lite-esp32-development-kit)**
- **In order to buy ATOM Matrix, please [Click here](https://shop.m5stack.com/collections/m5-controllers/products/atom-matrix-esp32-development-kit)**

## Description

==**ATOM Matrix**== and ==**ATOM Lite**== are ESP32 development board with a size of only 24 \* 24mm.It provides more GPIO for user customization which is very suitable for embedded smart home devices and in making smart toys. The main control adopts the ESP32-PICO chip which comes integrated with Wi-Fi and Bluetooth technologies and has a 4MB of integrated SPI flash memory. ATOM board provides an Infra-Red LED, RGB LED, buttons, and a PH2.0 interface. In addition, it can connect to external sensors and actuators through 6 GPIOs. The on-board Type-C USB interface enables rapid program upload and execution. ATOM Matrix have 5 \* 5 RGB LED matrix, built-in IMU sensor (MPU6886).

## Applications

- Internet of things terminal controller
- IoT node
- Wearable peripherals

## Driver Installation

Connect the device to the PC, open the device manager to install [FTDI driver](https://ftdichip.com/drivers/vcp-drivers/) for the device. Take the win10 environment as an example, download the driver file that matches the operating system, unzip it, and install it through the device manager. (Note: In some system environments, the driver needs to be installed twice for the driver to take effect. The unrecognized device name is usually `M5Stack` or `USB Serial`. Windows recommends using the driver file to install directly in the device manager (custom Update), the executable file installation method may not work properly). [Click here to download FTDI driver](https://ftdichip.com/drivers/vcp-drivers/)

## Peripherals Pin Map

| Peripherals | Pin |
| --- | --- |
| RGB Led | G27 |
| Btn | G39 |
| IR | G12 |
| SCL | G21 |
| SDA | G25 |

[![](https://camo.githubusercontent.com/90dbc06a7dffb0a14bbe57351324fcc84edb59f4c2315a57c09db5d65455a3a1/68747470733a2f2f7374617469632d63646e2e6d35737461636b2e636f6d2f7265736f757263652f646f63732f7374617469632f6173736574732f696d672f70726f647563745f706963732f636f72652f6d696e69636f72652f61746f6d2f61746f6d5f70696e5f6d61705f30312e77656270)](https://camo.githubusercontent.com/90dbc06a7dffb0a14bbe57351324fcc84edb59f4c2315a57c09db5d65455a3a1/68747470733a2f2f7374617469632d63646e2e6d35737461636b2e636f6d2f7265736f757263652f646f63732f7374617469632f6173736574732f696d672f70726f647563745f706963732f636f72652f6d696e69636f72652f61746f6d2f61746f6d5f70696e5f6d61705f30312e77656270)

## Schematic

[![](https://camo.githubusercontent.com/748f14eb1cce84c700c144ed8e1e2ca5ebf37e30e66ef4ac3c6ff66874bce875/68747470733a2f2f7374617469632d63646e2e6d35737461636b2e636f6d2f7265736f757263652f646f63732f7374617469632f6173736574732f696d672f70726f647563745f706963732f636f72652f6d696e69636f72652f61746f6d2f41544f4d5f4c4954455f53494d504c455f4349524355545f32303230303531342e77656270)](https://camo.githubusercontent.com/748f14eb1cce84c700c144ed8e1e2ca5ebf37e30e66ef4ac3c6ff66874bce875/68747470733a2f2f7374617469632d63646e2e6d35737461636b2e636f6d2f7265736f757263652f646f63732f7374617469632f6173736574732f696d672f70726f647563745f706963732f636f72652f6d696e69636f72652f61746f6d2f41544f4d5f4c4954455f53494d504c455f4349524355545f32303230303531342e77656270)

**Atom pixel tool**

[Click here to download](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/software/AtomPixTool.exe)

## More Information

**UIFlow Quick Start**: [Click Here](https://docs.m5stack.com/en/quick_start/atom/uiflow)

**MicroPython API**: [Click Here](https://docs.m5stack.com/en/quick_start/atom/mpy)

**Arduino IDE Development**: [Click Here](https://docs.m5stack.com/en/quick_start/atom/arduino)

**Atom Arduino API**: [Click Here](https://docs.m5stack.com/en/api/atom/system)