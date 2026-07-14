# m5stack/M5GFX issue #160: Not draw correctly when using ESP-IDF on PaperS3, some werid stripes on screen.

- URL: https://github.com/m5stack/M5GFX/issues/160
- State: `closed`
- Created: 2025-10-28T13:49:58Z
- Updated: 2025-11-08T20:29:08Z
- Author: `tyan-boot`

## Issue body

When use M5GFX 0.2.8 and above in ESP-IDF framework, the screen have weird stripe on top:

![Image](https://github.com/user-attachments/assets/01e1e37b-9a17-4ac9-88a3-5fcf0079f94f)

The code:
```cpp
#include <stdio.h>
#include <M5GFX.h>

M5GFX display;

extern "C" {

void app_main(void)
{

  display.init();
}

}
```

Basically do nothing except just init the display, i tried almost every version from 0.2.8 to 0.2.16, all have the same behavior. If i draw some rect on screen using this code:

```cpp
#include <stdio.h>
#include <M5GFX.h>

M5GFX display;

extern "C" {

void app_main(void)
{

  display.init();
  display.setColor(128, 128, 128);
  display.fillRect(100, 100, 200, 110);
}

}
```

All background will be filled with that weird stripes:

![Image](https://github.com/user-attachments/assets/92981252-f98a-4661-9ffb-063015ef61e5)

When in this situation, it needs clear screen about five or more times to make screen return to white like first picture.

Also i tried IDF v5.4.2 and IDF v5.5.1, nothing changed, both have this issue, here is my `sdkconfig` copied from https://github.com/m5stack/uiflow-micropython/blob/master/m5stack/boards/M5STACK_PaperS3/mpconfigboard.cmake

```
CONFIG_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHMODE="qio"
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_DETECT=y
CONFIG_ESPTOOLPY_AFTER_NORESET=y

CONFIG_SPIRAM_MEMTEST=

CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_SPIRAM_MODE_QUAD=
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_FREERTOS_ENABLE_BACKWARD_COMPATIBILITY=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_SPEED=80

# epdiy
CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y
CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE=64

# Compiler options: use -O2 and disable assertions to improve performance
CONFIG_COMPILER_OPTIMIZATION_PERF=y
CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE=y


# ESP System Settings
# Only on: ESP32, ESP32S3
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=n
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=n

# Power Management
CONFIG_PM_ENABLE=y

# Memory protection
# This is required to allow allocating IRAM
CONFIG_ESP_SYSTEM_MEMPROT_FEATURE=n

# FreeRTOS
CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS=2
CONFIG_FREERTOS_SUPPORT_STATIC_ALLOCATION=y
CONFIG_FREERTOS_ENABLE_STATIC_TASK_CLEAN_UP=n
CONFIG_FREERTOS_TASK_PRE_DELETION_HOOK=n

# To reduce iRAM usage
CONFIG_ESP32_WIFI_IRAM_OPT=n
CONFIG_ESP32_WIFI_RX_IRAM_OPT=n
CONFIG_SPI_MASTER_ISR_IN_IRAM=n
CONFIG_SPI_SLAVE_ISR_IN_IRAM=n
CONFIG_ESP_EVENT_POST_FROM_IRAM_ISR=n
CONFIG_PERIPH_CTRL_FUNC_IN_IRAM=n

# UART Configuration
CONFIG_UART_ISR_IN_IRAM=y

# IDF 5 deprecated
CONFIG_ADC_SUPPRESS_DEPRECATE_WARN=y
CONFIG_RMT_SUPPRESS_DEPRECATE_WARN=y

CONFIG_NEWLIB_NANO_FORMAT=n

CONFIG_ESP_SYSTEM_PMP_IDRAM_SPLIT=n

# MicroPython on ESP32, ESP IDF configuration with 240MHz CPU
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_40=
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_80=
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160=
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240

CONFIG_SPI_SLAVE_IN_IRAM=n
CONFIG_SPI_SLAVE_ISR_IN_IRAM=n
CONFIG_SPI_MASTER_IN_IRAM=n
CONFIG_SPI_MASTER_ISR_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBCHAR_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBENV_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBFILE_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBIO_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBJMP_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBMATH_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBMEM_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBMISC_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBNUMPARSER_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBRAND_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBSTR_IN_IRAM=n
CONFIG_SPIRAM_CACHE_LIBTIME_IN_IRAM=n

# For cmake build
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_16mb.csv"

CONFIG_FREERTOS_HZ=1000
```

**However, use M5GFX in arduino platform, all work well, no stripes**

After dig and try tons of hours still have no idea about this.

## Update
I tried v5.3.4, nothing wrong and all goods, but v5.4 and above still not. I notice some lcd driver refactor on v5.4 but don't know what actually effect this.

## Comments

### Comment 1: tyan-boot at 2025-10-28T18:58:45Z

Permalink: https://github.com/m5stack/M5GFX/issues/160#issuecomment-3458042682

After more hours i found that `bus_speed` may effect stripe wides.

https://github.com/m5stack/M5GFX/blob/6c93c6836f11a5abad67c0ff35ff75050e636abc/src/M5GFX.cpp#L1465

So this is what screen looks like when `bus_speed` is set to `16000000` which is default value in version 0.2.16

![Image](https://github.com/user-attachments/assets/1e734f6e-6366-44e1-8d4e-621f7a71ed75)

After reduce bus_speed to `10000000`, the stripe narrowed

![Image](https://github.com/user-attachments/assets/46818286-630e-40dd-9d64-aca7acf8397f)

And `bus_speed` on 5000000

![Image](https://github.com/user-attachments/assets/815bbf9d-8f0e-48e4-a1b0-4dd28d461a50)

### Comment 2: lovyan03 at 2025-10-29T01:34:45Z

Permalink: https://github.com/m5stack/M5GFX/issues/160#issuecomment-3459285448

Hello, @tyan-boot
Thank you for the information.
Just to confirm, does this mean the results vary depending on the ESP-IDF version?

ESP-IDF v5.3.x = OK
ESP-IDF v5.4.x = NG
ESP-IDF v5.5.x = NG

Is this correct?

### Comment 3: tyan-boot at 2025-10-29T11:58:08Z

Permalink: https://github.com/m5stack/M5GFX/issues/160#issuecomment-3461146198

@lovyan03 Yes, i also test with patch versions:

v5.3.4 = OK, same with arduino, no error
v5.4 = NG
v5.4.1 = NG
v5.4.2 = NG
v5.5.1 = NG

Not sure why but i can not reproduce the second picture, now it looks:

![Image](https://github.com/user-attachments/assets/e25fe332-442f-4d91-84d1-76b369afeb99)

And i draw some text when run those tests, with version that not works:

![Image](https://github.com/user-attachments/assets/149adf26-d520-46d9-8f81-f7b0eeccd803)

The code
```cpp
  display.begin();
  display.setRotation(1);
  display.setTextSize(10);
  display.setCursor(100,100);
  display.print("Hello!");

  display.setTextSize(5);
  display.setCursor(100,300);
  display.print("Hello!");
  display.fillRect(500, 100, 100, 50, TFT_BLACK);
```

Another things is when i post this issue, i already use `print` to display some text but the character itself is not solid, they are also composed of stripes with stripe gray background just like the picture in this comment. After a sleep now the character are solid but those background still exist.

### Comment 4: tyan-boot at 2025-10-31T23:00:15Z

Permalink: https://github.com/m5stack/M5GFX/issues/160#issuecomment-3475139960

After run bisect between v5.3.4 and v5.4, i finally found the commit introduce this bug: https://github.com/espressif/esp-idf/commit/611fb654ce44966c6e24f1e08d47ac0bc60adf19

In this commit ESP-IDF remove `gpio_set_direction` on those pins, the most important things `gpio_set_direction` done is disable open-drain mode on pin if not requests explicitly, that is data pins will only be output mode but not OD mode even if already is OD mode.

For somehow unknown reason, GPIO11 and GPIO12 are in OD mode when we initialize i80 bus, on older ESP-IDF, all data pins are called with `gpio_set_direction(pin, GPIO_MODE_OUTPUT)` which then calls `gpio_od_disable`, on ESP-IDF version 5.4 and above(more specifically https://github.com/espressif/esp-idf/commit/611fb654ce44966c6e24f1e08d47ac0bc60adf19), there is no `gpio_set_direction` call leaving some data pins are OD mode and cause this problem.

### Comment 5: tyan-boot at 2025-10-31T23:04:01Z

Permalink: https://github.com/m5stack/M5GFX/issues/160#issuecomment-3475157310

@lovyan03 Hi, i open a pr https://github.com/m5stack/M5GFX/pull/162 that should fix this problem.

Whenever it’s convenient, could you review my PR?
