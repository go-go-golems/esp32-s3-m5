---
Title: Source - waveshare-esp32-p4-wifi6
Ticket: ESP32-P4-PICOCALC-BACKLIGHT
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-BACKLIGHT"
---

The **ESP32-P4-WIFI6** is a multimedia development board based on ESP32-P4 with integrated ESP32-C6, and supports Wi-Fi 6 and BLE 5 wireless connections. It provides a rich set of human-machine interfaces, including MIPI-CSI (integrated image signal processor ISP), MIPI-DSI, SPI, I2S, I2C, LED PWM, MCPWM, RMT, ADC, UART, TWAI, etc. Additionally, it supports USB OTG 2.0 HS, and has a built-in 40PIN GPIO expansion interface on board that is compatible with some Raspberry Pi HAT expansion boards, enabling a wider range of application adaptability. The ESP32-P4 uses a 360MHz dual-core RISC-V processor and supports up to 32MB PSRAM, featuring USB 2.0, MIPI-CSI/DSI, H.264 encoding, and other peripherals, meeting the needs for low-cost, high-performance, and low-power multimedia development. In addition, the ESP32-P4 integrates a digital signature peripheral and a dedicated key management unit to ensure data and operation security. The ESP32-P4-WIFI6 is specially designed for high-performance and high-security applications, meeting the needs of embedded systems in human-machine interaction, edge computing, and IO expansion.

| SKU | Product |
| --- | --- |
| 32020 | ESP32-P4-WIFI6 |
| 31647 | ESP32-P4-WIFI6-M |
| 32021 | ESP32-P4-WIFI6-KIT-A |
| 32022 | ESP32-P4-WIFI6-KIT-B |

## Features

- Processor
	- Equipped with a RISC-V 32-bit dual-core processor (HP system), featuring DSP and ISA extensions, a Floating-Point Unit (FPU), with a main frequency of up to 360 MHz.
		- Equipped with a RISC-V 32-bit single-core processor (LP system), with a main frequency of up to 40 MHz.
		- Equipped with an ESP32-C6 WIFI/BT co-processor, expanding WIFI 6/Bluetooth 5 and other functions through SDIO
- Memory
	- 128 KB of high-performance (HP) system read-only memory (ROM).
		- 16 KB of low-power (LP) system read-only memory (ROM).
		- 768 KB of high-performance (HP) L2 memory (L2MEM).
		- 32 KB of low-power (LP) SRAM.
		- 8 KB of system tightly coupled memory (TCM).
		- 32 MB PSRAM stacked in the package, onboard 32MB Nor Flash
- Peripheral Interfaces
	- 2 × 20 pin headers onboard, exposing 27 remaining programmable GPIOs
		- Onboard speaker interface and microphone, enabling ideal audio functionality using the Codec chip and amplifier chip
		- Onboard MIPI-CSI high-definition camera interface, supporting Full HD 1080P image capture and encoding, integrated Image Signal Processor (ISP), H.264 video encoder, supports H.264 & JPEG video encoding (1080P @30fps), facilitating applications in computer vision, machine vision, and other fields
		- Onboard MIPI-DSI high-definition display interface, integrated Pixel Processing Accelerator (PPA), 2D graphics acceleration controller (2D DMA), supports JPEG image decoding (1080P @30fps), providing strong support for high-definition display and smooth HMI experiences, suitable for applications such as smart home control panels, industrial control panels, and vending machines

## Hardware Description

![ESP32-P4-WIFI6 Hardware Description 1](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-details-intro-911fb2f4c7e9661438776e729cb02b40.webp)

1. **ESP32-P4NRW32** ESP32-P4 with 32MB stacked PSRAM
2. **ESP32-C6-MINI-1** SDIO interface protocol, expands ESP32-P4-WIFI6 with Wi-Fi 6 and Bluetooth 5
3. **32MB Nor Flash**
4. **Display Interface** MIPI-DSI (2-lane), compatible with [`5`](https://www.waveshare.com/5-dsi-touch-a.htm) / [`7`](https://www.waveshare.com/7-dsi-touch-a.htm) / [`8`](https://www.waveshare.com/8-dsi-touch-a.htm) / [`10.1`](https://www.waveshare.com/10.1-dsi-touch-a.htm) inch DSI screens
5. **Camera Interface** MIPI-CSI (2-lane), compatible with cameras such as [`OV5647`](https://www.waveshare.com/rpi-camera-b.htm)
6. **Type-C Interface** For power supply, programming, and debugging
7. **SMD Microphone**
8. **Speaker Interface** MX1.25 2P connector, supports 8Ω 2W speaker
9. **4PIN USB Interface** USB OTG 2.0 High Speed interface
10. **ESP32-C6 UART Pads**
11. **BOOT Button** Press during power-up or reset to enter download mode
12. **RESET Button** reset button
13. **Power indicator**
14. **TF Card Slot** SDIO 3.0 interface protocol

## Pinout Definition

![ESP32-P4-WIFI6 Pin Definition](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-details-inter-3472c9ea8e7f1e2665531f8d7e954e06.webp)

## Dimensions

![ESP32-P4-WIFI6 Product Dimensions](https://docs.waveshare.com/assets/images/ESP32-P4-WIFI6-details-size-b78b796cd14da18002ab1244855a89a9.webp)

## Development Tools

Each of these two development approaches has its own advantages, and developers can choose according to their needs and skill levels. Arduino is suitable for beginners and non-professionals because they are easy to learn and quick to get started. ESP-IDF is a better choice for developers with a professional background or high performance requirements, as it provides more advanced development tools and greater control capabilities for the development of complex projects.

- **ESP-IDF**, or full name Espressif IDE, is a professional development framework introduced by Espressif Technology for the ESP series chips. It is developed using the C language, including a compiler, debugger, and flashing tool, etc., and can be developed via the command lines or through an integrated development environment (such as Visual Studio Code with the Espressif IDF plugin). The plugin offers features such as code navigation, project management, and debugging, etc. We recommend using VS Code for development. For the specific configuration process, please refer to the **[Working with ESP-IDF](https://docs.waveshare.com/ESP32-P4-WIFI6/Development-Environment-Setup-IDF)**. The tutorial also provides relevant example programs for reference.