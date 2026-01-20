Below is a solid **ESP-IDF GPIO toggle (LED blink) demo** for the **Seeed Studio XIAO ESP32C6** (ESP32-C6). I’ll give you a plan, explain what’s happening, show a working demo project, and include references + links.

---

## Plan

1. **Pick the GPIO to toggle**

   * For **XIAO ESP32C6**, the on-board user LED is on **GPIO15**. ([Seeed Studio][1])
   * The user LED is **active-low** (drive LOW = on, HIGH = off). ([Seeed Studio][2])
2. **Create a minimal ESP-IDF project**
3. **Configure the GPIO as an output** with `gpio_config()` ([Espressif Systems][3])
4. **Toggle it in a loop** with a FreeRTOS delay
5. **Build / flash / monitor** with `idf.py`

---

## Fast path: use ESP-IDF’s built-in “blink” example (board-correct settings)

Seeed’s own ESP-IDF guide for XIAO uses the ESP-IDF `examples/get-started/blink` project and explicitly calls out:

* `idf.py set-target esp32c6` ([Seeed Studio][1])
* set **Blink GPIO number = 15** because “LED … is connected to GPIO15” ([Seeed Studio][1])

Commands (Linux/macOS shell):

```bash
# 0) Make sure your ESP-IDF environment is active in this shell
. $HOME/esp/esp-idf/export.sh

# 1) Go to the built-in example
cd ~/esp/esp-idf/examples/get-started/blink

# 2) Target ESP32-C6
idf.py set-target esp32c6

# 3) Configure:
idf.py menuconfig
# Example Configuration ->
#   Blink LED type: GPIO
#   Blink GPIO number: 15
#   Blink period in ms: 500  (or whatever)

# 4) Build/flash/monitor
idf.py -p /dev/ttyACM0 flash monitor
```

(Replace `/dev/ttyACM0` with your port.) ([Seeed Studio][1])

---

## Custom GPIO toggle demo project (minimal + clear)

### Folder layout

```text
xiao_esp32c6_gpio_toggle/
  CMakeLists.txt
  main/
    CMakeLists.txt
    gpio_toggle.c
```

### `CMakeLists.txt` (top-level)

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiao_esp32c6_gpio_toggle)
```

### `main/CMakeLists.txt`

```cmake
idf_component_register(SRCS "gpio_toggle.c"
                       INCLUDE_DIRS ".")
```

### `main/gpio_toggle.c`

```c
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "gpio_toggle";

/*
 * XIAO ESP32C6 on-board user LED is on GPIO15 (per Seeed docs).
 * Note: the user LED is active-low: LOW=ON, HIGH=OFF.
 */
#define LED_GPIO        GPIO_NUM_15
#define LED_ACTIVE_LOW  1

#define BLINK_PERIOD_MS 500

static inline int led_level(bool on)
{
#if LED_ACTIVE_LOW
    return on ? 0 : 1;
#else
    return on ? 1 : 0;
#endif
}

void app_main(void)
{
    // Configure LED_GPIO as a simple GPIO output.
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    bool on = false;
    while (true) {
        on = !on;
        gpio_set_level(LED_GPIO, led_level(on));
        ESP_LOGI(TAG, "LED %s", on ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}
```

### Build/flash

```bash
. $HOME/esp/esp-idf/export.sh
cd xiao_esp32c6_gpio_toggle
idf.py set-target esp32c6
idf.py -p /dev/ttyACM0 flash monitor
```

---

## How it works (what to look for)

* `gpio_config()` sets up the pin as a **simple GPIO output** (mode, pulls, interrupt). ([Espressif Systems][3])
* The loop flips a boolean and calls `gpio_set_level()` to drive the pin.
* `vTaskDelay(pdMS_TO_TICKS(ms))` yields to FreeRTOS and gives you a stable blink period.
* For XIAO ESP32C6 specifically:

  * LED is **GPIO15** ([Seeed Studio][1])
  * LED is **active-low** (LOW turns it on) ([Seeed Studio][2])

---

## Links (copy/paste)

```text
Seeed XIAO ESP-IDF guide (includes ESP32C6 LED=GPIO15 + set-target esp32c6):
https://wiki.seeedstudio.com/xiao_idf/

Seeed note about XIAO ESP32C6 user LED being active-low:
https://wiki.seeedstudio.com/xiao_wifi_usage_esp32c6/

ESP-IDF GPIO driver docs (ESP32-C6):
https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/gpio.html

ESP-IDF “Get Started” landing page:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html

ESP-IDF Linux/macOS toolchain setup (export.sh, etc.):
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-macos-setup.html
```

If you tell me **which exact Seeed ESP32-C6 board** you have (XIAO vs another Seeed C6 dev board) I’ll set the default GPIO to match that board’s on-board LED pin.

[1]: https://wiki.seeedstudio.com/xiao_idf/ "Developed on XIAO using Espressif ESP-IDF | Seeed Studio Wiki"
[2]: https://wiki.seeedstudio.com/xiao_wifi_usage_esp32c6/ "WiFi Usage | Seeed Studio Wiki"
[3]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/gpio.html "GPIO & RTC GPIO - ESP32-C6 -  — ESP-IDF Programming Guide v5.5.2 documentation"

