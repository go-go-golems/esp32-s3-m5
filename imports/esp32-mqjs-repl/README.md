# ESP32-S3 MicroQuickJS REPL

A JavaScript REPL (Read-Eval-Print Loop) running on ESP32-S3 using the MicroQuickJS embedded JavaScript engine.

## Features

- **Embedded JavaScript Engine**: MicroQuickJS - a minimal JavaScript engine designed for embedded systems
- **Interactive REPL**: Serial console interface for interactive JavaScript execution
- **ESP-IDF Integration**: Built on ESP-IDF v5.5.2 framework
- **QEMU Support**: Can be tested in QEMU emulator before deploying to hardware
- **Minimal Footprint**: Only 281KB firmware size with 64KB JavaScript heap

## Requirements

- ESP-IDF v5.5.2
- ESP32-S3 development board (or QEMU for testing)
- Python 3.11+

## Building

```bash
# Navigate to project directory
cd esp32-mqjs-repl/mqjs-repl

# Set up ESP-IDF environment
source ~/esp-idf-v5.5.2/export.sh

# Configure for ESP32-S3 (already done)
# idf.py set-target esp32s3

# Build the firmware
idf.py build
```

## Running in QEMU

```bash
# Build and run in QEMU with monitor
idf.py qemu monitor

# The REPL will be accessible on the console
# You'll see the prompt: js>
```

## Flashing to Hardware

```bash
# Flash to ESP32-S3 board
idf.py -p /dev/ttyUSB0 flash monitor

# Or use the generated flash command
python -m esptool --chip esp32s3 -b 460800 \
  --before default_reset --after hard_reset write_flash \
  --flash_mode dio --flash_size 2MB --flash_freq 80m \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/mqjs-repl.bin
```

## Usage

Once the firmware is running, you can type JavaScript expressions at the `js>` prompt:

```javascript
js> 1+2
3
js> var x = 10
js> x * 5
50
js> function factorial(n) { return n <= 1 ? 1 : n * factorial(n-1); }
js> factorial(5)
120
```

## Project Structure

```
esp32-mqjs-repl/
├── mqjs-repl/
│   ├── main/
│   │   ├── main.c              # Main REPL implementation
│   │   ├── minimal_stdlib.h    # Minimal JavaScript stdlib
│   │   └── CMakeLists.txt
│   ├── components/
│   │   └── mquickjs/           # MicroQuickJS engine
│   │       ├── mquickjs.c
│   │       ├── mquickjs.h
│   │       ├── cutils.c
│   │       ├── dtoa.c
│   │       ├── libm.c
│   │       └── CMakeLists.txt
│   ├── CMakeLists.txt
│   └── sdkconfig
└── README.md
```

## Technical Details

### Memory Configuration
- **JavaScript Heap**: 64 KB
- **Total Firmware Size**: ~281 KB
- **Free RAM at startup**: ~300 KB

### UART Configuration
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **TX Pin**: GPIO 44
- **RX Pin**: GPIO 43

### Supported JavaScript Features
- Variables (`var`)
- Functions
- Basic operators (+, -, *, /, %)
- Comparison operators (==, !=, <, >, <=, >=)
- Logical operators (&&, ||, !)
- Control flow (if, while, for, return)
- Objects and arrays (basic support)

## Limitations

- **No Standard Library**: The current implementation uses a minimal stdlib. Built-in objects like `Math`, `String`, `Array` methods are not available.
- **Limited Memory**: 64KB heap may be insufficient for complex programs
- **No Persistent Storage**: Code is not saved between resets

## Future Enhancements

- [ ] Full JavaScript standard library integration
- [ ] ESP32-specific APIs (GPIO, WiFi, I2C, SPI)
- [ ] File system support for loading/saving scripts
- [ ] Increased memory allocation
- [ ] WebSocket or network REPL access

## License

This project integrates MicroQuickJS which is licensed under the MIT License.

## Credits

- **MicroQuickJS**: https://github.com/bellard/mquickjs by Fabrice Bellard
- **ESP-IDF**: https://github.com/espressif/esp-idf by Espressif Systems

## Author

Built as a demonstration of embedded JavaScript on ESP32-S3.
