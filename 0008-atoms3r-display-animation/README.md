## AtomS3R Tutorial 0008 — Display Animation (GC9107 + ESP-IDF 5.4.1)

This tutorial brings up the **AtomS3R SPI LCD (GC9107)** using **ESP-IDF `esp_lcd`** and renders a simple looping animation.

### Goals

- Show a minimal **`esp_lcd` panel init** for ATOMS3R
- Show a simple **full-frame animation loop** (RGB565 framebuffer → `esp_lcd_panel_draw_bitmap`)

### Hardware wiring (AtomS3R)

From the ATOMS3R schematic crop (`atoms3r_display_pins_crop.png`) and the pin summary doc:

- `DISP_CS` = GPIO14
- `SPI_SCK` = GPIO15
- `SPI_MOSI` = GPIO21
- `DISP_RS/DC` = GPIO42
- `DISP_RST` = GPIO48

### Panel controller (GC9107)

AtomS3R uses **GC9107**. The visible area is **128×128**, but the controller RAM is **128×160**, and M5’s stack maps the visible window with a **Y offset of 32**. This tutorial defaults `TUTORIAL_0008_LCD_Y_OFFSET=32`.

### Backlight control (AtomS3R revisions)

M5GFX indicates AtomS3R backlight is controlled via an **I2C brightness device** (address `0x30`, register `0x0e`) on pins **SCL=GPIO0 / SDA=GPIO45**.

This tutorial defaults to that I2C backlight mode, but also provides:

- **A gate GPIO in I2C mode** (some revisions gate LED_BL via a GPIO-controlled FET; commonly **GPIO7 active-low**)
- **A pure GPIO fallback** in menuconfig (on/off)

### Menuconfig knobs

`idf.py menuconfig` → `Tutorial 0008: ATOMS3R Display Animation`

- Resolution (defaults 128×128)
- X/Y offsets (use if image is shifted)
- RGB565 byte-swap (recommended for SPI LCDs; fixes “grid/noise” artifacts if byte order is wrong)
- BGR vs RGB color order
- SPI clock
- Frame delay (fps)
- Backlight mode + parameters (I2C vs GPIO fallback)
- Debug heartbeat logging (every N frames)
- GC9107 init compatibility: use M5GFX-style vendor init cmds (helpful if the panel stays black)

### Dependencies

This tutorial uses Espressif’s component-manager driver for GC9107:

- `main/idf_component.yml`: `espressif/esp_lcd_gc9107`

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation && idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation && idf.py flash && idf.py monitor
```

If you need to specify the serial port:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation && idf.py -p /dev/ttyACM0 flash monitor
```

### Expected output

- The display shows a smooth, shifting color gradient animation.
- Serial logs include LCD init parameters (pclk, gap offsets, colorspace).
- You will also see:
  - backlight init/write logs (I2C addr/reg/value), and
  - a periodic “heartbeat” log (configurable; can be disabled).

### Desktop preview (expected animation)

Open `animation-preview.html` in a browser. It reproduces the same per-pixel math as the ESP-IDF `draw_frame()` function so you can compare what the LCD *should* look like.

| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- | ----- |

# Hello World Example

Starts a FreeRTOS task to print "Hello World".

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## How to use example

Follow detailed instructions provided specifically for this example.

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)


## Example folder contents

The project **hello_world** contains one source file in C language [hello_world_main.c](main/hello_world_main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt` files that provide set of directives and instructions describing the project's source files and targets (executable, library, or both).

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── pytest_hello_world.py      Python script used for automated testing
├── main
│   ├── CMakeLists.txt
│   └── hello_world_main.c
└── README.md                  This is the file you are currently reading
```

For more information on structure and contents of ESP-IDF projects, please refer to Section [Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html) of the ESP-IDF Programming Guide.

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

## Technical support and feedback

Please use the following feedback channels:

* For technical queries, go to the [esp32.com](https://esp32.com/) forum
* For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-idf/issues)

We will get back to you as soon as possible.
