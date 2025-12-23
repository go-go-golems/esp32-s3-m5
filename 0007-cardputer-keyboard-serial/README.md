## Cardputer Tutorial 0007 — Keyboard → Serial Echo (ESP-IDF 5.4.1)

This tutorial reads the **built-in Cardputer keyboard** (GPIO matrix scan) and **echoes what you type** to the serial console in realtime.

### Goals

- Demonstrate **GPIO matrix scanning** (polling-based) on the Cardputer keyboard
- Provide a minimal “text input” building block for future UI/network tutorials

### Hardware notes

The keyboard is scanned using **3 output pins** and **7 input pins**.

- Defaults are taken from the vendor implementation in `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h`:
  - OUT: `GPIO8, GPIO9, GPIO11`
  - IN: `GPIO13, GPIO15, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7`
- Schematics referenced:
  - [Sch_M5Cardputer.pdf](file:///home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/datasheets/Sch_M5Cardputer.pdf)
  - [Sch_M5cardputer_Base.pdf](file:///home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/datasheets/Sch_M5cardputer_Base.pdf)

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial && idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial && idf.py flash && idf.py monitor
```

If you need to specify the serial port:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial && idf.py -p /dev/ttyACM0 flash monitor
```

#### Note on “realtime” echo vs host line buffering

This firmware echoes typed characters **inline** after the prompt (it does **not** print a newline until you press `enter`).

- With `idf.py monitor`, you should usually see inline characters immediately.
- Some IDE consoles / log capture layers are **line-buffered** and won’t display partial lines until a newline appears. In that case, the typed characters may “show up all at once” only after you press `enter`.
- On some Cardputer setups, the ESP-IDF console may be configured as **UART primary + USB-Serial-JTAG secondary**, which can make `stdout`/inline echo behavior confusing when you’re only watching `/dev/ttyACM*`.
  - This tutorial now writes inline echo bytes directly to the **USB-Serial-JTAG** driver as well (when enabled), so you should see realtime echo over `/dev/ttyACM*` with both `idf.py monitor` and `screen`.

Workarounds:

- **Use per-key event logging**: enable `Tutorial 0007: ... -> Debug: log a line for every key press event` (`CONFIG_TUTORIAL_0007_LOG_KEY_EVENTS=y`)
- **Use `screen`** (shows bytes as they arrive):

```bash
screen -L -U /dev/ttyACM0 115200
```

To exit `screen`: `Ctrl-A` then `k` then `y`.

### Configure pins / scan period (menuconfig)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial && idf.py menuconfig
```

Go to:

- `Tutorial 0007: Cardputer Keyboard -> Serial Echo`

### Expected output

- You’ll see a prompt `> `
- As you type, characters echo **inline** immediately (no newline until `enter`)
- `enter` prints the captured line via `ESP_LOGI(...)` and shows a new prompt
  - (Backspace is supported as `del`)

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
