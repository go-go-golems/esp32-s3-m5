---
Title: Source - esp32-p4-sleep-modes
Ticket: ESP32-P4-PICOCALC-SLEEP
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-SLEEP"
---

## Sleep Modes

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-reference/system/sleep_modes.html)

## Overview

ESP32-P4 supports two major power saving modes: Light-sleep and Deep-sleep. According to the features used by an application, there are some sub sleep modes. See for these sleep modes and sub sleep modes. Additionally, there are some power-down options that can be configured to further reduce the power consumption. See for more details.

There are several wakeup sources in the sleep modes. These sources can also be combined so that the chip will wake up when any of the sources are triggered. describes these wakeup sources and configuration APIs in detail.

The configuration of power-down options and wakeup sources are optional. They can be configured at any moment before entering the sleep modes.

Then the application can call sleep start APIs to enter one of the sleep modes. See for more details. When the wakeup condition is met, the application is awoken from sleep. See on how to get the wakeup cause, and on how to handle the wakeup sources after wakeup.

## Sleep Modes

In Light-sleep mode, the digital peripherals, most of the RAM, and CPUs are clock-gated and their supply voltage is reduced. Upon exit from Light-sleep, the digital peripherals, RAM, and CPUs resume operation and their internal states are preserved.

In Deep-sleep mode, the CPUs, most of the RAM, and all digital peripherals that are clocked from APB\_CLK are powered off. The only parts of the chip that remain powered on are:

> - RTC controller
> - ULP coprocessor
> - RTC FAST memory

## Wakeup Sources

Wakeup sources can be enabled using `esp_sleep_enable_X_wakeup` APIs. Wakeup sources are not disabled after wakeup, you can disable them using API if you do not need them any more. See.

Following are the wakeup sources supported on ESP32-P4.

### Timer

The RTC controller has a built-in timer which can be used to wake up the chip after a predefined amount of time. Time is specified at microsecond precision, but the actual resolution depends on the clock source selected for RTC\_SLOW\_CLK.

For details on RTC clock options, see **ESP32-P4 Technical Reference Manual** > **ULP Coprocessor** \[[PDF](https://www.espressif.com/sites/default/files/documentation/esp32-p4_technical_reference_manual_en.pdf#ulp)\].

RTC peripherals or RTC memories do not need to be powered on during sleep in this wakeup mode.

function can be used to enable sleep wakeup using a timer.

### Touchpad

The RTC IO module contains the logic to trigger wakeup when a touch sensor interrupt occurs. To wakeup from a touch sensor interrupt, users need to configure the touch pad interrupt before the chip enters Deep-sleep or Light-sleep modes.

function can be used to enable this wakeup source.

### External Wakeup (ext1)

The RTC controller contains the logic to trigger wakeup using multiple RTC GPIOs. One of the following two logic functions can be used to trigger ext1 wakeup:

- wake up if any of the selected pins is high (`ESP_EXT1_WAKEUP_ANY_HIGH`)
- wake up if any of the selected pins is low (`ESP_EXT1_WAKEUP_ANY_LOW`)

This wakeup source is controlled by the RTC controller. Unlike `ext0`, this wakeup source supports wakeup even when the RTC peripheral is powered down. Although the power domain of the RTC peripheral, where RTC IOs are located, is powered down during sleep modes, ESP-IDF will automatically lock the state of the wakeup pin before the system enters sleep modes and unlock upon exiting sleep modes. Therefore, the internal pull-up or pull-down resistors can still be configured for the wakeup pin:

```
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
rtc_gpio_pullup_dis(gpio_num);
rtc_gpio_pulldown_en(gpio_num);
```

If we turn off the `RTC_PERIPH` domain, we will use the HOLD feature to maintain the pull-up and pull-down on the pins during sleep. HOLD feature will be acted on the pin internally before the system enters sleep modes, and this can further reduce power consumption:

```
rtc_gpio_pullup_dis(gpio_num);
rtc_gpio_pulldown_en(gpio_num);
```

If certain chips lack the `RTC_PERIPH` domain, we can only use the HOLD feature to maintain the pull-up and pull-down on the pins during sleep modes:

```
gpio_pullup_dis(gpio_num);
gpio_pulldown_en(gpio_num);
```

function can be used to append ext1 wakeup IO and set corresponding wakeup level.

function can be used to remove ext1 wakeup IO.

The RTC controller also supports triggering wakeup, allowing configurable IO to use different wakeup levels simultaneously. This can be configured with.

Warning

- To use the EXT1 wakeup, the IO pad(s) are configured as RTC IO. Therefore, before using these pads as digital GPIOs, users need to reconfigure them by calling the [`rtc_gpio_deinit()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/gpio.html#_CPPv415rtc_gpio_deinit10gpio_num_t "rtc_gpio_deinit") function.
- If the RTC peripherals are configured to be powered down (which is by default), the wakeup IOs will be set to the holding state before entering sleep. Therefore, after the chip wakes up from Light-sleep, please call `rtc_gpio_hold_dis` to disable the hold function to perform any pin re-configuration. For Deep-sleep wakeup, this is already being handled at the application startup stage.

### ULP Coprocessor Wakeup

ULP coprocessor can run while the chip is in sleep mode, and may be used to poll sensors, monitor ADC or GPIO states, and wake up the chip when a specific event is detected. ULP coprocessor is part of the RTC peripherals power domain, and it runs the program stored in RTC SLOW memory. RTC SLOW memory will be powered on during sleep if this wakeup mode is requested. RTC peripherals will be automatically powered on before ULP coprocessor starts running the program; once the program stops running, RTC peripherals are automatically powered down again.

function can be used to enable this wakeup source.

### GPIO Wakeup (Light-sleep Only)

In addition to EXT0 and EXT1 wakeup sources described above, one more method of wakeup from external inputs is available in Light-sleep mode. With this wakeup source, each pin can be individually configured to trigger wakeup on high or low level using [`gpio_wakeup_enable()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/gpio.html#_CPPv418gpio_wakeup_enable10gpio_num_t15gpio_int_type_t "gpio_wakeup_enable") function. Unlike EXT0 and EXT1 wakeup sources, which can only be used with RTC IOs, this wakeup source can be used with any IO (RTC or digital).

function can be used to enable this wakeup source.

Warning

Before entering Light-sleep mode, check if any GPIO pin to be driven is part of the VDD\_SPI power domain. If so, this power domain must be configured to remain ON during sleep.

For example, on ESP32-WROOM-32 board, GPIO16 and GPIO17 are linked to VDD\_SPI power domain. If they are configured to remain high during Light-sleep, the power domain should be configured to remain powered ON. This can be done with:

```cpp
esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
```

Note

In Light-sleep mode, if you set Kconfig option [CONFIG\_PM\_POWER\_DOWN\_PERIPHERAL\_IN\_LIGHT\_SLEEP](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-pm-power-down-peripheral-in-light-sleep) ， to continue using [`gpio_wakeup_enable()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/gpio.html#_CPPv418gpio_wakeup_enable10gpio_num_t15gpio_int_type_t "gpio_wakeup_enable") for GPIO wakeup, you need to first call [`rtc_gpio_init()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/gpio.html#_CPPv413rtc_gpio_init10gpio_num_t "rtc_gpio_init") and [`rtc_gpio_set_direction()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/gpio.html#_CPPv422rtc_gpio_set_direction10gpio_num_t15rtc_gpio_mode_t "rtc_gpio_set_direction"), setting the RTCIO to input mode.

Alternatively，you can use directly in that condition for GPIO wakeup, because the digital IO power domain is being powered off.

### UART Wakeup (Light-sleep Only)

When ESP32-P4 receives UART input from external devices, it is often necessary to wake up the chip when input data is available. The UART peripheral contains a feature which allows waking up the chip from Light-sleep when a certain number of positive edges on RX pin are seen. This number of positive edges can be set using [`uart_set_wakeup_threshold()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/uart.html#_CPPv425uart_set_wakeup_threshold11uart_port_ti "uart_set_wakeup_threshold") function. Note that the character which triggers wakeup (and any characters before it) will not be received by the UART after wakeup. This means that the external device typically needs to send an extra character to the ESP32-P4 to trigger wakeup before sending the data.

function can be used to enable this wakeup source.

After waking-up from UART, you should send some extra data through the UART port in Active mode, so that the internal wakeup indication signal can be cleared. Otherwises, the next UART wake-up would trigger with two less rising edges than the configured threshold value.

> Note
> 
> In Light-sleep mode, setting Kconfig option [CONFIG\_PM\_POWER\_DOWN\_PERIPHERAL\_IN\_LIGHT\_SLEEP](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-pm-power-down-peripheral-in-light-sleep) will invalidate UART wakeup.

### Disable Sleep Wakeup Source

Previously configured wakeup sources can be disabled later using API. This function deactivates trigger for the given wakeup source. Additionally, it can disable all triggers if the argument is `ESP_SLEEP_WAKEUP_ALL`.

## Power-down Options

The application can force specific powerdown modes for RTC peripherals and RTC memories. In Deep-sleep mode, we can also isolate some IOs to further reduce current consumption.

### Power-down of RTC Peripherals and Memories

By default, and functions power down all RTC power domains which are not needed by the enabled wakeup sources. To override this behaviour, function is provided.

In ESP32-P4, there is only RTC FAST memory, so if some variables in the program are marked by `RTC_DATA_ATTR`, `RTC_SLOW_ATTR` or `RTC_FAST_ATTR` attributes, all of them go to RTC FAST memory. It will be kept powered on by default. This can be overridden using function, if desired.

### Power-down of Flash

By default, to avoid potential issues, function does **not** power down flash. To be more specific, it takes time to power down the flash and during this period the system may be woken up, which then actually powers up the flash before this flash could be powered down completely. As a result, there is a chance that the flash may not work properly.

So, in theory, it is ok if you only wake up the system after the flash is completely powered down. However, in reality, the flash power-down period can be hard to predict (for example, this period can be much longer when you add filter capacitors to the flash's power supply circuit) and uncontrollable (for example, the asynchronous wake-up signals make the actual sleep time uncontrollable).

Warning

If a filter capacitor is added to your flash power supply circuit, please do everything possible to avoid powering down flash.

Therefore, it is recommended not to power down flash when using ESP-IDF. For power-sensitive applications, it is recommended to use Kconfig option [CONFIG\_ESP\_SLEEP\_FLASH\_LEAKAGE\_WORKAROUND](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-esp-sleep-flash-leakage-workaround) to reduce the power consumption of the flash during Light-sleep, instead of powering down the flash.

It is worth mentioning that PSRAM has a similar Kconfig option [CONFIG\_ESP\_SLEEP\_PSRAM\_LEAKAGE\_WORKAROUND](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-esp-sleep-psram-leakage-workaround).

However, for those who have fully understood the risk and are still willing to power down the flash to further reduce the power consumption, please check the following mechanisms:

> - Setting Kconfig option [CONFIG\_ESP\_SLEEP\_POWER\_DOWN\_FLASH](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-esp-sleep-power-down-flash) only powers down the flash when the RTC timer is the only wake-up source **and** the sleep time is longer than the flash power-down period.
> - Calling `esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF)` powers down flash when the RTC timer is not enabled as a wakeup source **or** the sleep time is longer than the flash power-down period.

Note

- ESP-IDF does not provide any mechanism that can power down the flash in all conditions when Light-sleep.
- function forces power down flash regardless of user configuration.

### Configuring IOs (Deep-sleep Only)

Some ESP32-P4 IOs have internal pullups or pulldowns, which are enabled by default. If an external circuit drives this pin in Deep-sleep mode, current consumption may increase due to current flowing through these pullups and pulldowns.

To isolate a pin to prevent extra current draw, call [`rtc_gpio_isolate()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/gpio.html#_CPPv416rtc_gpio_isolate10gpio_num_t "rtc_gpio_isolate") function.

For example, on ESP32-WROVER module, GPIO12 is pulled up externally, and it also has an internal pulldown in the ESP32 chip. This means that in Deep-sleep, some current flows through these external and internal resistors, increasing Deep-sleep current above the minimal possible value.

Add the following code before to remove such extra current:

```cpp
rtc_gpio_isolate(GPIO_NUM_12);
```

## Entering Sleep

or functions can be used to enter Light-sleep or Deep-sleep modes correspondingly. After that, the system configures the parameters of RTC controller according to the requested wakeup sources and power-down options.

It is also possible to enter sleep modes with no wakeup sources configured. In this case, the chip will be in sleep modes indefinitely until external reset is applied.

### UART Output Handling

Before entering sleep, the sleep flow prepares the **console UART** (the UART used for debug output, selected by [CONFIG\_ESP\_CONSOLE\_UART\_NUM](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-esp-console-uart-num)) so that APB clock changes or power-down do not cause garbled output or undefined behavior. The strategy applied is configurable and affects data integrity, sleep entry time, and power consumption.

**Default behavior (auto mode)**

If you do not call, the following default strategy is used:

- **Deep-sleep**: Always wait until all data in the console UART FIFO has been transmitted before entering sleep, so that all debug output is sent and no data is lost.
- **Light-sleep**: Behavior depends on whether the UART power domain is powered down:
	- If the UART remains powered (e.g. HP peripheral domain not powered down): UART output is **suspended** after the current frame completes; after wakeup it is resumed and any remaining data in the UART TX FIFO before sleep continues to be sent.
	- If the UART power domain is powered down: The sleep flow waits until all data in the console UART TX FIFO has been transmitted before entering sleep; data in other UARTs is discarded to enter sleep faster.

**Configuring console UART handling**

You can override the default by calling and choosing one of the following modes (see ):

- (default): Automatically choose flush or suspend based on sleep type and power domain, as described above.
- : Always wait until all data in the console UART TX FIFO has been transmitted before entering sleep. Use when you must guarantee that all debug output is visible; sleep entry will take longer and the chip will stay in Active state longer, increasing power consumption.
- : Wait for the current UART frame to complete, then suspend the UART. If the UART stays powered during Light-sleep, transmission continues after wake. If the UART power domain is powered down, unsent data will be lost.
- : Discard all unsent data in the console UART FIFO and enter sleep immediately. Use for the fastest sleep entry and lowest power when debug output can be discarded.
- : Do not perform any handling on the console UART before sleep. Use only when the UART state is known to be safe (e.g. no pending output or the console UART is disabled).

Note

The sleep flow runs in a critical section. When using a mode that flushes the console UART (e.g., or the default behavior for Light-sleep/Deep-sleep when the HP peripheral domain is powered down), set [CONFIG\_ESP\_INT\_WDT\_TIMEOUT\_MS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/kconfig-reference.html#config-esp-int-wdt-timeout-ms) to be **greater than** `SOC_UART_FIFO_LEN` × (time to send one character at the current baud rate). Otherwise, if too much data is queued in the TX FIFO, the flush may take longer than the interrupt watchdog timeout and trigger a watchdog reset during sleep entry.

Example: ensure all debug output is sent before every sleep:

```
.. code-block:: C
```

> fflush(stdout); esp\_sleep\_set\_console\_uart\_handling\_mode(ESP\_SLEEP\_ALWAYS\_FLUSH\_UART); esp\_light\_sleep\_start();

Example: minimize sleep entry time and allow discarding console output:

```
.. code-block:: C
```

> esp\_sleep\_set\_console\_uart\_handling\_mode(ESP\_SLEEP\_ALWAYS\_DISCARD\_UART); esp\_deep\_sleep\_start();

## Checking Sleep Wakeup Cause

function can be used to check which wakeup source has triggered wakeup from sleep mode.

For touchpad, it is possible to identify which touch pin has caused wakeup using functions.

For ext1 wakeup sources, it is possible to identify which GPIO has caused wakeup using functions.

## Application Examples

- [protocols/sntp](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/protocols/sntp) demonstrates the implementation of basic functionality of Deep-sleep, where ESP module is periodically waken up to retrieve time from NTP server.
- [system/deep\_sleep](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/system/deep_sleep) demonstrates the usage of various Deep-sleep wakeup triggers and ULP coprocessor programming.
- [system/light\_sleep](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/system/light_sleep) demonstrates the usage of Light-sleep wakeup triggered by various sources, such as the timer, GPIOs, supported by ESP32-P4.
- [peripherals/touch\_sensor/touch\_sens\_sleep](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/touch_sensor/touch_sens_sleep) demonstrates the usage of Light-sleep and Deep-sleep wakeup triggered by the touch sensor.

## API Reference

### Header File

- [components/esp\_hw\_support/include/esp\_sleep.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_hw_support/include/esp_sleep.h)
- This header file can be included with:
	> ```c
	> #include "esp_sleep.h"
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_disable\_wakeup\_source( source) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv431esp_sleep_disable_wakeup_source18esp_sleep_source_t "Permalink to this definition")  

Disable wakeup source.

This function is used to deactivate wake up trigger for source defined as parameter of the function.

See docs/sleep-modes.rst for details.

Note

This function does not modify wake up configuration in RTC. It will be performed in esp\_deep\_sleep\_start/esp\_light\_sleep\_start function.

Parameters:

**source** -- - number of source to disable of type esp\_sleep\_source\_t

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_STATE if trigger was not active

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_ulp\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv427esp_sleep_enable_ulp_wakeupv "Permalink to this definition")  

Enable wakeup by ULP coprocessor.

Note

On ESP32, ULP wakeup source cannot be used when RTC\_PERIPH power domain is forced, to be powered on (ESP\_PD\_OPTION\_ON) or when ext0 wakeup source is used.

Returns:

- ESP\_OK on success
- ESP\_ERR\_NOT\_SUPPORTED if additional current by touch (CONFIG\_RTC\_EXT\_CRYST\_ADDIT\_CURRENT) is enabled.
- ESP\_ERR\_INVALID\_STATE if ULP co-processor is not enabled or if wakeup triggers conflict

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_timer\_wakeup(uint64\_t time\_in\_us) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv429esp_sleep_enable_timer_wakeup8uint64_t "Permalink to this definition")  

Enable wakeup by timer.

Note

The valid `time_in_us` value depends on the bit width of the lp\_timer/rtc\_timer counter and the current slow clock source selection (Refer RTC clock source configuration in menuconfig). Valid values should be positive values less than RTC slow clock period \* (2 ^ RTC timer bitwidth).

Parameters:

**time\_in\_us** -- time before wakeup, in microseconds

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if value is out of range.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_vad\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv427esp_sleep_enable_vad_wakeupv "Permalink to this definition")  

Enable wakeup by VAD.

Returns:

- ESP\_OK on success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_vbat\_under\_volt\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv439esp_sleep_enable_vbat_under_volt_wakeupv "Permalink to this definition")  

Wakeup chip is VBAT power voltage is lower than the configured brownout\_threshold value.

Returns:

- ESP\_OK on success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_touchpad\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv432esp_sleep_enable_touchpad_wakeupv "Permalink to this definition")  

Enable wakeup by touch sensor.

Note

On ESP32, touch wakeup source can not be used when RTC\_PERIPH power domain is forced to be powered on (ESP\_PD\_OPTION\_ON) or when ext0 wakeup source is used.

Note

The FSM mode of the touch button should be configured as the timer trigger mode.

Returns:

- ESP\_OK on success
- ESP\_ERR\_NOT\_SUPPORTED if additional current by touch (CONFIG\_RTC\_EXT\_CRYST\_ADDIT\_CURRENT) is enabled.
- ESP\_ERR\_INVALID\_STATE if wakeup triggers conflict

int esp\_sleep\_get\_touchpad\_wakeup\_status(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv436esp_sleep_get_touchpad_wakeup_statusv "Permalink to this definition")  

Get the touch pad which caused wakeup.

If wakeup was caused by another source, this function will return TOUCH\_PAD\_MAX;

Returns:

touch pad which caused wakeup

bool esp\_sleep\_is\_valid\_wakeup\_gpio(gpio\_num\_t gpio\_num) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv430esp_sleep_is_valid_wakeup_gpio10gpio_num_t "Permalink to this definition")  

Returns true if a GPIO number is valid for use as wakeup source.

Note

For SoCs with RTC IO capability, this can be any valid RTC IO input pin.

Parameters:

**gpio\_num** -- Number of the GPIO to test for wakeup source capability

Returns:

True if this GPIO number will be accepted as a sleep wakeup source.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_ext1\_wakeup(uint64\_t io\_mask, level\_mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_enable_ext1_wakeup8uint64_t28esp_sleep_ext1_wakeup_mode_t "Permalink to this definition")  

Enable wakeup using multiple pins.

This function uses external wakeup feature of RTC controller. It will work even if RTC peripherals are shut down during sleep.

This feature can monitor any number of pins which are in RTC IOs. Once selected pins go into the state given by level\_mode argument, the chip will be woken up.

Note

This function does not modify pin configuration. The pins are configured in esp\_deep\_sleep\_start/esp\_light\_sleep\_start, immediately before entering sleep mode.

Note

Internal pullups and pulldowns don't work when RTC peripherals are shut down. In this case, external resistors need to be added. Alternatively, RTC peripherals (and pullups/pulldowns) may be kept enabled using esp\_sleep\_pd\_config function. If we turn off the `RTC_PERIPH` domain or certain chips lack the `RTC_PERIPH` domain, we will use the HOLD feature to maintain the pull-up and pull-down on the pins during sleep. HOLD feature will be acted on the pin internally before the system entering sleep, and this can further reduce power consumption.

Note

Call this func will reset the previous ext1 configuration.

Note

This function will be deprecated in release/v6.0. Please switch to use `esp_sleep_enable_ext1_wakeup_io` and `esp_sleep_disable_ext1_wakeup_io`

Note

On ESP32-H2, although GPIO7 is an RTC GPIO, it is not led out for external wakeup.

Parameters:

- **io\_mask** -- Bit mask of GPIO numbers which will cause wakeup. Only GPIOs which have RTC functionality can be used in this bit map. For different SoCs, the related GPIOs are:
	- ESP32: 0, 2, 4, 12-15, 25-27, 32-39
		- ESP32-S2: 0-21
		- ESP32-S3: 0-21
		- ESP32-C6: 0-7
		- ESP32-H2: 7-14
- **level\_mode** -- Select logic function used to determine wakeup condition:
	- When target chip is ESP32:
		- ESP\_EXT1\_WAKEUP\_ALL\_LOW: wake up when all selected GPIOs are low
				- ESP\_EXT1\_WAKEUP\_ANY\_HIGH: wake up when any of the selected GPIOs is high
		- When target chip is ESP32-S2, ESP32-S3, ESP32-C6 or ESP32-H2:
		- ESP\_EXT1\_WAKEUP\_ANY\_LOW: wake up when any of the selected GPIOs is low
				- ESP\_EXT1\_WAKEUP\_ANY\_HIGH: wake up when any of the selected GPIOs is high

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if io\_mask is zero, or mode is invalid

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_ext1\_wakeup\_io(uint64\_t io\_mask, level\_mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv431esp_sleep_enable_ext1_wakeup_io8uint64_t28esp_sleep_ext1_wakeup_mode_t "Permalink to this definition")  

Enable ext1 wakeup pins with IO masks.

This will append selected IOs to the wakeup IOs, it will not reset previously enabled IOs. To reset specific previously enabled IOs, call esp\_sleep\_disable\_ext1\_wakeup\_io with the io\_mask. To reset all the enabled IOs, call esp\_sleep\_disable\_ext1\_wakeup\_io(0).

This function uses external wakeup feature of RTC controller. It will work even if RTC peripherals are shut down during sleep.

This feature can monitor any number of pins which are in RTC IOs. Once selected pins go into the state given by level\_mode argument, the chip will be woken up.

Note

This function does not modify pin configuration. The pins are configured in esp\_deep\_sleep\_start/esp\_light\_sleep\_start, immediately before entering sleep mode.

Note

Internal pullups and pulldowns don't work when RTC peripherals are shut down. In this case, external resistors need to be added. Alternatively, RTC peripherals (and pullups/pulldowns) may be kept enabled using esp\_sleep\_pd\_config function. If we turn off the `RTC_PERIPH` domain or certain chips lack the `RTC_PERIPH` domain, we will use the HOLD feature to maintain the pull-up and pull-down on the pins during sleep. HOLD feature will be acted on the pin internally before the system entering sleep, and this can further reduce power consumption.

Note

On ESP32-H2, although GPIO7 is an RTC GPIO, it is not led out for external wakeup.

Parameters:

- **io\_mask** -- Bit mask of GPIO numbers which will cause wakeup. Only GPIOs which have RTC functionality can be used in this bit map. For different SoCs, the related GPIOs are:
	- ESP32: 0, 2, 4, 12-15, 25-27, 32-39
		- ESP32-S2: 0-21
		- ESP32-S3: 0-21
		- ESP32-C6: 0-7
		- ESP32-H2: 7-14
- **level\_mode** -- Select logic function used to determine wakeup condition:
	- When target chip is ESP32:
		- ESP\_EXT1\_WAKEUP\_ALL\_LOW: wake up when all selected GPIOs are low
				- ESP\_EXT1\_WAKEUP\_ANY\_HIGH: wake up when any of the selected GPIOs is high
		- When target chip is ESP32-S2, ESP32-S3, ESP32-C6 or ESP32-H2:
		- ESP\_EXT1\_WAKEUP\_ANY\_LOW: wake up when any of the selected GPIOs is low
				- ESP\_EXT1\_WAKEUP\_ANY\_HIGH: wake up when any of the selected GPIOs is high

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if any of the selected GPIOs is not an RTC GPIO, or mode is invalid
- ESP\_ERR\_NOT\_ALLOWED when wakeup level will become different between ext1 IOs if!SOC\_PM\_SUPPORT\_EXT1\_WAKEUP\_MODE\_PER\_PIN

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_disable\_ext1\_wakeup\_io(uint64\_t io\_mask) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv432esp_sleep_disable_ext1_wakeup_io8uint64_t "Permalink to this definition")  

Disable ext1 wakeup pins with IO masks. This will remove selected IOs from the wakeup IOs.

Note

On ESP32-H2, although GPIO7 is an RTC GPIO, it is not led out for external wakeup.

Parameters:

**io\_mask** -- Bit mask of GPIO numbers which will cause wakeup. Only GPIOs which have RTC functionality can be used in this bit map. If value is zero, this func will remove all previous ext1 configuration. For different SoCs, the related GPIOs are:

- ESP32: 0, 2, 4, 12-15, 25-27, 32-39
- ESP32-S2: 0-21
- ESP32-S3: 0-21
- ESP32-C6: 0-7
- ESP32-H2: 7-14

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if any of the selected GPIOs is not an RTC GPIO.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_ext1\_wakeup\_with\_level\_mask(uint64\_t io\_mask, uint64\_t level\_mask) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv444esp_sleep_enable_ext1_wakeup_with_level_mask8uint64_t8uint64_t "Permalink to this definition")  

Enable wakeup using multiple pins, allows different trigger mode per pin.

This function uses external wakeup feature of RTC controller. It will work even if RTC peripherals are shut down during sleep.

This feature can monitor any number of pins which are in RTC IOs. Once selected pins go into the state given by level\_mode argument, the chip will be woken up.

Note

This function does not modify pin configuration. The pins are configured in esp\_deep\_sleep\_start/esp\_light\_sleep\_start, immediately before entering sleep mode.

Note

Internal pullups and pulldowns don't work when RTC peripherals are shut down. In this case, external resistors need to be added. Alternatively, RTC peripherals (and pullups/pulldowns) may be kept enabled using esp\_sleep\_pd\_config function. If we turn off the `RTC_PERIPH` domain or certain chips lack the `RTC_PERIPH` domain, we will use the HOLD feature to maintain the pull-up and pull-down on the pins during sleep. HOLD feature will be acted on the pin internally before the system entering sleep, and this can further reduce power consumption.

Note

On ESP32-H2, although GPIO7 is an RTC GPIO, it is not led out for external wakeup.

Parameters:

- **io\_mask** -- Bit mask of GPIO numbers which will cause wakeup. Only GPIOs which have RTC functionality can be used in this bit map. For different SoCs, the related GPIOs are:
	- ESP32-C6: 0-7
		- ESP32-H2: 7-14
- **level\_mask** -- Select logic function used to determine wakeup condition per pin. Each bit of the level\_mask corresponds to the respective GPIO. Each bit's corresponding position is set to 0, the wakeup level will be low, on the contrary, each bit's corresponding position is set to 1, the wakeup level will be high.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if any of the selected GPIOs is not an RTC GPIO, or mode is invalid

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_gpio\_wakeup\_on\_hp\_periph\_powerdown(uint64\_t gpio\_pin\_mask, mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv451esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown8uint64_t29esp_sleep_gpio_wake_up_mode_t "Permalink to this definition")  

Enable wakeup using specific gpio pins.

This function enables an IO pin to wake up the chip from peripheral powerdowned sleep. (including deepsleep and peripheral powerdowned lightsleep).

Note

1.This function does not modify pin configuration. The pins are configured inside `esp_sleep_start`, immediately before entering sleep mode. 2.This function is also applicable to waking up the lightsleep when the peripheral power domain is powered off, see PM\_POWER\_DOWN\_PERIPHERAL\_IN\_LIGHT\_SLEEP in menuconfig.

Note

You don't need to worry about pull-up or pull-down resistors before using this function because the ESP\_SLEEP\_GPIO\_ENABLE\_INTERNAL\_RESISTORS option is enabled by default. It will automatically set pull-up or pull-down resistors internally in esp\_deep\_sleep\_start based on the wakeup mode. However, when using external pull-up or pull-down resistors, please be sure to disable the ESP\_SLEEP\_GPIO\_ENABLE\_INTERNAL\_RESISTORS option, as the combination of internal and external resistors may cause interference. BTW, when you use low level to wake up the chip, we strongly recommend you to add external resistors (pull-up).

Parameters:

- **gpio\_pin\_mask** -- Bit mask of GPIO numbers which will cause wakeup. Only GPIOs which have RTC functionality (pads that powered by VDD3P3\_RTC) can be used in this bit map.
- **mode** -- Select logic function used to determine wakeup condition:
	- ESP\_GPIO\_WAKEUP\_GPIO\_LOW: wake up when the gpio turn to low.
		- ESP\_GPIO\_WAKEUP\_GPIO\_HIGH: wake up when the gpio turn to high.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if the mask contains any invalid wakeup pin or wakeup mode is invalid

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_gpio\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_enable_gpio_wakeupv "Permalink to this definition")  

Enable wakeup from light sleep using GPIOs.

Each GPIO supports wakeup function, which can be triggered on either low level or high level. Unlike EXT0 and EXT1 wakeup sources, this method can be used both for all IOs: RTC IOs and digital IOs. It can only be used to wakeup from light sleep though.

To enable wakeup, first call gpio\_wakeup\_enable, specifying gpio number and wakeup level, for each GPIO which is used for wakeup. Then call this function to enable wakeup feature.

Note

1\. On ESP32, GPIO wakeup source can not be used together with touch or ULP wakeup sources.

1. If PM\_POWER\_DOWN\_PERIPHERAL\_IN\_LIGHT\_SLEEP is enabled (if target supported), this API is unavailable since the GPIO module is powered down during sleep. You can use `esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown` instead, or use EXT1 wakeup source by `esp_sleep_enable_ext1_wakeup_io` to achieve the same function. (Only GPIOs which have RTC functionality can be used)

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_STATE if wakeup triggers conflict

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_uart\_wakeup(int uart\_num) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_enable_uart_wakeupi "Permalink to this definition")  

Enable wakeup from light sleep using UART.

Use uart\_set\_wakeup\_threshold function to configure UART wakeup threshold.

Wakeup from light sleep takes some time, so not every character sent to the UART can be received by the application.

Note

1\. ESP32 does not support wakeup from UART2.

1. If PM\_POWER\_DOWN\_PERIPHERAL\_IN\_LIGHT\_SLEEP is enabled (if target supported), this API is unavailable since the UART module is powered down during sleep.

Parameters:

**uart\_num** -- UART port to wake up from

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if wakeup from given UART is not supported

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_bt\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv426esp_sleep_enable_bt_wakeupv "Permalink to this definition")  

Enable wakeup by bluetooth.

Returns:

- ESP\_OK on success
- ESP\_ERR\_NOT\_SUPPORTED if wakeup from bluetooth is not supported

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_disable\_bt\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv427esp_sleep_disable_bt_wakeupv "Permalink to this definition")  

Disable wakeup by bluetooth.

Returns:

- ESP\_OK on success
- ESP\_ERR\_NOT\_SUPPORTED if wakeup from bluetooth is not supported

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_wifi\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_enable_wifi_wakeupv "Permalink to this definition")  

Enable wakeup by WiFi MAC.

Returns:

- ESP\_OK on success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_disable\_wifi\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv429esp_sleep_disable_wifi_wakeupv "Permalink to this definition")  

Disable wakeup by WiFi MAC.

Returns:

- ESP\_OK on success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_enable\_wifi\_beacon\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv435esp_sleep_enable_wifi_beacon_wakeupv "Permalink to this definition")  

Enable beacon wakeup by WiFi MAC, it will wake up the system into modem state.

Returns:

- ESP\_OK on success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_disable\_wifi\_beacon\_wakeup(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv436esp_sleep_disable_wifi_beacon_wakeupv "Permalink to this definition")  

Disable beacon wakeup by WiFi MAC.

Returns:

- ESP\_OK on success

uint64\_t esp\_sleep\_get\_ext1\_wakeup\_status(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv432esp_sleep_get_ext1_wakeup_statusv "Permalink to this definition")  

Get the bit mask of GPIOs which caused wakeup (ext1)

If wakeup was caused by another source, this function will return 0.

Returns:

bit mask, if GPIOn caused wakeup, BIT(n) will be set

uint64\_t esp\_sleep\_get\_gpio\_wakeup\_status(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv432esp_sleep_get_gpio_wakeup_statusv "Permalink to this definition")  

Get the bit mask of GPIOs which caused wakeup (gpio)

If wakeup was caused by another source, this function will return 0.

Returns:

bit mask, if GPIOn caused wakeup, BIT(n) will be set

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_pd\_config( domain, option) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv419esp_sleep_pd_config21esp_sleep_pd_domain_t21esp_sleep_pd_option_t "Permalink to this definition")  

Configure power domain options for sleep mode.

This function provides power domain management for sleep mode, allowing users to control which power domains remain active during sleep to maintain required functionality. The function supports:

- Automatic power management (ESP\_PD\_OPTION\_AUTO): Power domains are controlled automatically based on system requirements and other peripheral usage.
- Manual power control (ESP\_PD\_OPTION\_ON/ESP\_PD\_OPTION\_OFF): User can force specific power domains to stay on or off during sleep.

The management strategy uses reference counting when in manual mode, allowing multiple subsystems to request and release power domain resources safely without interfering with each other. Power domains will only change state when all users have released their requirements.

Parameters:

- **domain** -- power domain to configure
- **option** -- power down option (ESP\_PD\_OPTION\_OFF, ESP\_PD\_OPTION\_ON, or ESP\_PD\_OPTION\_AUTO)

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if either of the arguments is out of range

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_deep\_sleep\_try\_to\_start(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv427esp_deep_sleep_try_to_startv "Permalink to this definition")  

Enter deep sleep with the configured wakeup options.

The reason for the rejection can be such as a short sleep time.

Note

In general, the function does not return, but if the sleep is rejected, then it returns from it.

Returns:

- No return - If the sleep is not rejected.
- ESP\_ERR\_INVALID\_STATE VBAT power does not meet the requirements for entering deepsleep
- ESP\_ERR\_SLEEP\_REJECT sleep request is rejected(wakeup source set before the sleep request)

void esp\_deep\_sleep\_start(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv420esp_deep_sleep_startv "Permalink to this definition")  

Enter deep sleep with the configured wakeup options.

Note

The function does not do a return (no rejection). Even if wakeup source set before the sleep request it goes to deep sleep anyway.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_light\_sleep\_start(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv "Permalink to this definition")  

Enter light sleep with the configured wakeup options.

Returns:

- ESP\_OK on success (returned after wakeup)
- ESP\_ERR\_SLEEP\_REJECT sleep request is rejected(wakeup source set before the sleep request)
- ESP\_ERR\_SLEEP\_TOO\_SHORT\_SLEEP\_DURATION after deducting the sleep flow overhead, the final sleep duration is too short to cover the minimum sleep duration of the chip, when rtc timer wakeup source enabled

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_deep\_sleep\_try(uint64\_t time\_in\_us) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv418esp_deep_sleep_try8uint64_t "Permalink to this definition")  

Enter deep-sleep mode.

The device will automatically wake up after the deep-sleep time Upon waking up, the device calls deep sleep wake stub, and then proceeds to load application.

Call to this function is equivalent to a call to esp\_deep\_sleep\_enable\_timer\_wakeup followed by a call to esp\_deep\_sleep\_start.

The reason for the rejection can be such as a short sleep time.

Note

In general, the function does not return, but if the sleep is rejected, then it returns from it.

Parameters:

**time\_in\_us** -- deep-sleep time, unit: microsecond

Returns:

- No return - If the sleep is not rejected.
- ESP\_ERR\_SLEEP\_REJECT sleep request is rejected(wakeup source set before the sleep request)

void esp\_deep\_sleep(uint64\_t time\_in\_us) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv414esp_deep_sleep8uint64_t "Permalink to this definition")  

Enter deep-sleep mode.

The device will automatically wake up after the deep-sleep time Upon waking up, the device calls deep sleep wake stub, and then proceeds to load application.

Call to this function is equivalent to a call to esp\_deep\_sleep\_enable\_timer\_wakeup followed by a call to esp\_deep\_sleep\_start.

Note

The function does not do a return (no rejection).. Even if wakeup source set before the sleep request it goes to deep sleep anyway.

Parameters:

**time\_in\_us** -- deep-sleep time, unit: microsecond

Register a callback to be called from the deep sleep prepare.

Warning

deepsleep callbacks should without parameters, and MUST NOT, UNDER ANY CIRCUMSTANCES, CALL A FUNCTION THAT MIGHT BLOCK.

Parameters:

**new\_dslp\_cb** -- Callback to be called

Returns:

- ESP\_OK: Callback registered to the deepsleep misc\_modules\_sleep\_prepare
- ESP\_ERR\_NO\_MEM: No more hook space for register the callback

Unregister an deepsleep callback.

Parameters:

**old\_dslp\_cb** -- Callback to be unregistered

esp\_sleep\_get\_wakeup\_cause(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv426esp_sleep_get_wakeup_causev "Permalink to this definition")  

Get the wakeup source which caused wakeup from sleep.

Note

!!! This API will only return one wakeup source. If multiple wakeup sources wake up at the same time, the wakeup source information may be lost.

Returns:

cause of wake up from last sleep (deep sleep or light sleep)

uint32\_t esp\_sleep\_get\_wakeup\_causes(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv427esp_sleep_get_wakeup_causesv "Permalink to this definition")  

Get all wakeup sources bitmap which caused wakeup from sleep.

Returns:

The bitmap of the wakeup sources of the last wakeup from sleep. (deep sleep or light sleep)

void esp\_wake\_deep\_sleep(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv419esp_wake_deep_sleepv "Permalink to this definition")  

Default stub to run on wake from deep sleep.

Allows for executing code immediately on wake from sleep, before the software bootloader or ESP-IDF app has started up.

This function is weak-linked, so you can implement your own version to run code immediately when the chip wakes from sleep.

See docs/deep-sleep-stub.rst for details.

void esp\_set\_deep\_sleep\_wake\_stub( new\_stub) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_set_deep_sleep_wake_stub29esp_deep_sleep_wake_stub_fn_t "Permalink to this definition")  

Install a new stub at runtime to run on wake from deep sleep.

If implementing esp\_wake\_deep\_sleep() then it is not necessary to call this function.

However, it is possible to call this function to substitute a different deep sleep stub. Any function used as a deep sleep stub must be marked RTC\_IRAM\_ATTR, and must obey the same rules given for esp\_wake\_deep\_sleep().

void esp\_set\_deep\_sleep\_wake\_stub\_default\_entry(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv442esp_set_deep_sleep_wake_stub_default_entryv "Permalink to this definition")  

Set wake stub entry to default `esp_wake_stub_entry`

esp\_get\_deep\_sleep\_wake\_stub(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_get_deep_sleep_wake_stubv "Permalink to this definition")  

Get current wake from deep sleep stub.

Returns:

Return current wake from deep sleep stub, or NULL if no stub is installed.

void esp\_default\_wake\_deep\_sleep(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv427esp_default_wake_deep_sleepv "Permalink to this definition")  

The default esp-idf-provided esp\_wake\_deep\_sleep() stub.

See docs/deep-sleep-stub.rst for details.

void esp\_deep\_sleep\_disable\_rom\_logging(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv434esp_deep_sleep_disable_rom_loggingv "Permalink to this definition")  

Disable logging from the ROM code after deep sleep.

Using LSB of RTC\_STORE4.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_set\_console\_uart\_handling\_mode( handling\_mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv440esp_sleep_set_console_uart_handling_mode30esp_sleep_uart_handling_mode_t "Permalink to this definition")  

Configure how the console UART is handled when entering sleep.

This function configures the handling behavior for the console UART (CONFIG\_ESP\_CONSOLE\_UART\_NUM) during sleep modes. The console UART is typically used for debug output, so its handling mode affects whether debug messages are preserved or discarded before sleep.

Note

When CONFIG\_ESP\_SLEEP\_CACHE\_SAFE\_ASSERTION is enabled, the console UART will always be flushed regardless of the configured mode to ensure debug output is visible even when cache is disabled.

Parameters:

**handling\_mode** -- Handling method, one of the following strategies:

- ESP\_SLEEP\_AUTO\_FLUSH\_SUSPEND\_UART (default): Automatically selects the appropriate strategy based on sleep type and power domain:
	- Deep sleep: Always flush to avoid data loss
		- Light sleep: Suspend if UART remains powered, flush if UART power domain is powered down
- ESP\_SLEEP\_ALWAYS\_FLUSH\_UART: Wait for all data in TX FIFO to be fully transmitted before entering sleep. Ensures all debug output is visible but increases sleep entry time and power consumption.
- ESP\_SLEEP\_ALWAYS\_SUSPEND\_UART: Wait for current UART frame to complete, then suspend the UART state machine. If UART remains powered during light sleep, transmission resumes after wake. If UART power domain is powered down, unsent data will be lost.
- ESP\_SLEEP\_ALWAYS\_DISCARD\_UART: Discard all unsent data in UART FIFO and enter sleep immediately. Fastest sleep entry and lowest power, but all unsent debug output is lost.
- ESP\_SLEEP\_NO\_HANDLING: Do not perform any handling on the console UART before sleep. Can be used to disable the default UART handling behavior.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if handling\_mode is not valid
- ESP\_ERR\_NOT\_SUPPORTED if no console UART is configured (CONFIG\_ESP\_CONSOLE\_UART\_NUM == -1)

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_cpu\_retention\_init(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_cpu_retention_initv "Permalink to this definition")  

CPU Power down initialize.

Returns:

- ESP\_OK on success
- ESP\_ERR\_NO\_MEM not enough retention memory

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_sleep\_cpu\_retention\_deinit(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv430esp_sleep_cpu_retention_deinitv "Permalink to this definition")  

CPU Power down de-initialize.

Release system retention memory.

Returns:

- ESP\_OK on success

void esp\_sleep\_config\_gpio\_isolate(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv429esp_sleep_config_gpio_isolatev "Permalink to this definition")  

Configure to isolate all GPIO pins in sleep state.

void esp\_sleep\_enable\_gpio\_switch(bool enable) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_enable_gpio_switchb "Permalink to this definition")  

Enable or disable GPIO pins status switching between slept status and waked status.

Parameters:

**enable** -- decide whether to switch status or not

### Macros

ESP\_PD\_DOMAIN\_RTC8M [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#c.ESP_PD_DOMAIN_RTC8M "Permalink to this definition")  

### Type Definitions

typedef void (\*esp\_deep\_sleep\_cb\_t)(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv419esp_deep_sleep_cb_t "Permalink to this definition")  

typedef esp\_sleep\_wakeup\_cause\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv424esp_sleep_wakeup_cause_t "Permalink to this definition")  

typedef void (\*esp\_deep\_sleep\_wake\_stub\_fn\_t)(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv429esp_deep_sleep_wake_stub_fn_t "Permalink to this definition")  

Function type for stub to run on wake from sleep.

### Enumerations

enum esp\_sleep\_ext1\_wakeup\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_ext1_wakeup_mode_t "Permalink to this definition")  

Logic function used for EXT1 wakeup mode.

*Values:*

enumerator ESP\_EXT1\_WAKEUP\_ANY\_LOW [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N28esp_sleep_ext1_wakeup_mode_t23ESP_EXT1_WAKEUP_ANY_LOWE "Permalink to this definition")  

Wake the chip when any of the selected GPIOs go low.

enumerator ESP\_EXT1\_WAKEUP\_ANY\_HIGH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N28esp_sleep_ext1_wakeup_mode_t24ESP_EXT1_WAKEUP_ANY_HIGHE "Permalink to this definition")  

Wake the chip when any of the selected GPIOs go high.

enumerator ESP\_EXT1\_WAKEUP\_ALL\_LOW [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N28esp_sleep_ext1_wakeup_mode_t23ESP_EXT1_WAKEUP_ALL_LOWE "Permalink to this definition")  

enum esp\_sleep\_gpio\_wake\_up\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv429esp_sleep_gpio_wake_up_mode_t "Permalink to this definition")  

*Values:*

enumerator ESP\_GPIO\_WAKEUP\_GPIO\_LOW [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N29esp_sleep_gpio_wake_up_mode_t24ESP_GPIO_WAKEUP_GPIO_LOWE "Permalink to this definition")  

enumerator ESP\_GPIO\_WAKEUP\_GPIO\_HIGH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N29esp_sleep_gpio_wake_up_mode_t25ESP_GPIO_WAKEUP_GPIO_HIGHE "Permalink to this definition")  

enum esp\_sleep\_pd\_domain\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv421esp_sleep_pd_domain_t "Permalink to this definition")  

Power domains which can be powered down in sleep mode.

*Values:*

enumerator ESP\_PD\_DOMAIN\_RTC\_PERIPH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t24ESP_PD_DOMAIN_RTC_PERIPHE "Permalink to this definition")  

RTC IO, sensors and ULP co-processor.

enumerator ESP\_PD\_DOMAIN\_XTAL [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t18ESP_PD_DOMAIN_XTALE "Permalink to this definition")  

XTAL oscillator.

enumerator ESP\_PD\_DOMAIN\_XTAL32K [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t21ESP_PD_DOMAIN_XTAL32KE "Permalink to this definition")  

External 32 kHz XTAL oscillator.

enumerator ESP\_PD\_DOMAIN\_RC32K [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t19ESP_PD_DOMAIN_RC32KE "Permalink to this definition")  

Internal 32 kHz RC oscillator.

enumerator ESP\_PD\_DOMAIN\_RC\_FAST [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t21ESP_PD_DOMAIN_RC_FASTE "Permalink to this definition")  

Internal Fast oscillator.

enumerator ESP\_PD\_DOMAIN\_CPU [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t17ESP_PD_DOMAIN_CPUE "Permalink to this definition")  

CPU core.

enumerator ESP\_PD\_DOMAIN\_VDDSDIO [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t21ESP_PD_DOMAIN_VDDSDIOE "Permalink to this definition")  

VDD\_SDIO.

enumerator ESP\_PD\_DOMAIN\_TOP [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t17ESP_PD_DOMAIN_TOPE "Permalink to this definition")  

SoC TOP.

enumerator ESP\_PD\_DOMAIN\_CNNT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t18ESP_PD_DOMAIN_CNNTE "Permalink to this definition")  

Hight-speed connect peripherals power domain.

enumerator ESP\_PD\_DOMAIN\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_domain_t17ESP_PD_DOMAIN_MAXE "Permalink to this definition")  

Number of domains.

enum esp\_sleep\_pd\_option\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv421esp_sleep_pd_option_t "Permalink to this definition")  

Power down options.

*Values:*

enumerator ESP\_PD\_OPTION\_OFF [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_option_t17ESP_PD_OPTION_OFFE "Permalink to this definition")  

Power down the power domain in sleep mode.

enumerator ESP\_PD\_OPTION\_ON [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_option_t16ESP_PD_OPTION_ONE "Permalink to this definition")  

Keep power domain enabled during sleep mode.

enumerator ESP\_PD\_OPTION\_AUTO [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N21esp_sleep_pd_option_t18ESP_PD_OPTION_AUTOE "Permalink to this definition")  

Keep power domain enabled in sleep mode, if it is needed by one of the wakeup options. Otherwise power it down.

enum esp\_sleep\_source\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv418esp_sleep_source_t "Permalink to this definition")  

Sleep wakeup cause.

*Values:*

enumerator ESP\_SLEEP\_WAKEUP\_UNDEFINED [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t26ESP_SLEEP_WAKEUP_UNDEFINEDE "Permalink to this definition")  

In case of deep sleep, reset was not caused by exit from deep sleep.

enumerator ESP\_SLEEP\_WAKEUP\_ALL [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t20ESP_SLEEP_WAKEUP_ALLE "Permalink to this definition")  

Not a wakeup cause, used to disable all wakeup sources with esp\_sleep\_disable\_wakeup\_source.

enumerator ESP\_SLEEP\_WAKEUP\_EXT0 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t21ESP_SLEEP_WAKEUP_EXT0E "Permalink to this definition")  

Wakeup caused by external signal using RTC\_IO.

enumerator ESP\_SLEEP\_WAKEUP\_EXT1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t21ESP_SLEEP_WAKEUP_EXT1E "Permalink to this definition")  

Wakeup caused by external signal using RTC\_CNTL.

enumerator ESP\_SLEEP\_WAKEUP\_TIMER [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t22ESP_SLEEP_WAKEUP_TIMERE "Permalink to this definition")  

Wakeup caused by timer.

enumerator ESP\_SLEEP\_WAKEUP\_TOUCHPAD [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t25ESP_SLEEP_WAKEUP_TOUCHPADE "Permalink to this definition")  

Wakeup caused by touchpad.

enumerator ESP\_SLEEP\_WAKEUP\_ULP [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t20ESP_SLEEP_WAKEUP_ULPE "Permalink to this definition")  

Wakeup caused by ULP program.

enumerator ESP\_SLEEP\_WAKEUP\_GPIO [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t21ESP_SLEEP_WAKEUP_GPIOE "Permalink to this definition")  

Wakeup caused by GPIO (light sleep only on ESP32, S2 and S3)

enumerator ESP\_SLEEP\_WAKEUP\_UART [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t21ESP_SLEEP_WAKEUP_UARTE "Permalink to this definition")  

Wakeup caused by UART0 (light sleep only)

enumerator ESP\_SLEEP\_WAKEUP\_UART1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t22ESP_SLEEP_WAKEUP_UART1E "Permalink to this definition")  

Wakeup caused by UART1 (light sleep only)

enumerator ESP\_SLEEP\_WAKEUP\_UART2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t22ESP_SLEEP_WAKEUP_UART2E "Permalink to this definition")  

Wakeup caused by UART2 (light sleep only)

enumerator ESP\_SLEEP\_WAKEUP\_WIFI [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t21ESP_SLEEP_WAKEUP_WIFIE "Permalink to this definition")  

Wakeup caused by WIFI (light sleep only)

enumerator ESP\_SLEEP\_WAKEUP\_COCPU [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t22ESP_SLEEP_WAKEUP_COCPUE "Permalink to this definition")  

Wakeup caused by COCPU int.

enumerator ESP\_SLEEP\_WAKEUP\_COCPU\_TRAP\_TRIG [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t32ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIGE "Permalink to this definition")  

Wakeup caused by COCPU crash.

enumerator ESP\_SLEEP\_WAKEUP\_BT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t19ESP_SLEEP_WAKEUP_BTE "Permalink to this definition")  

Wakeup caused by BT (light sleep only)

enumerator ESP\_SLEEP\_WAKEUP\_VAD [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t20ESP_SLEEP_WAKEUP_VADE "Permalink to this definition")  

Wakeup caused by VAD.

enumerator ESP\_SLEEP\_WAKEUP\_VBAT\_UNDER\_VOLT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N18esp_sleep_source_t32ESP_SLEEP_WAKEUP_VBAT_UNDER_VOLTE "Permalink to this definition")  

Wakeup caused by VDD\_BAT under voltage.

enum esp\_sleep\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv416esp_sleep_mode_t "Permalink to this definition")  

Sleep mode.

*Values:*

enumerator ESP\_SLEEP\_MODE\_LIGHT\_SLEEP [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N16esp_sleep_mode_t26ESP_SLEEP_MODE_LIGHT_SLEEPE "Permalink to this definition")  

light sleep mode

enumerator ESP\_SLEEP\_MODE\_DEEP\_SLEEP [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N16esp_sleep_mode_t25ESP_SLEEP_MODE_DEEP_SLEEPE "Permalink to this definition")  

deep sleep mode

enum esp\_sleep\_uart\_handling\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv430esp_sleep_uart_handling_mode_t "Permalink to this definition")  

UART handling mode before entering sleep.

These modes define how UARTs are handled when the chip enters sleep mode. The behavior affects data integrity, power consumption, and sleep entry time.

*Values:*

enumerator ESP\_SLEEP\_AUTO\_FLUSH\_SUSPEND\_UART [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N30esp_sleep_uart_handling_mode_t33ESP_SLEEP_AUTO_FLUSH_SUSPEND_UARTE "Permalink to this definition")  

Automatically select flush or suspend based on sleep type and power domain configuration. For deep sleep, always flush. For light sleep, suspend if UART remains powered, flush/discard if UART power domain is powered down.

enumerator ESP\_SLEEP\_ALWAYS\_FLUSH\_UART [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N30esp_sleep_uart_handling_mode_t27ESP_SLEEP_ALWAYS_FLUSH_UARTE "Permalink to this definition")  

Always wait for all data in TX FIFO to be transmitted before sleep. Ensures data integrity but increases power consumption and sleep entry time.

enumerator ESP\_SLEEP\_ALWAYS\_SUSPEND\_UART [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N30esp_sleep_uart_handling_mode_t29ESP_SLEEP_ALWAYS_SUSPEND_UARTE "Permalink to this definition")  

Suspend UART transmission after current frame completes. If UART remains powered during sleep, transmission resumes after wake. If UART power domain is powered down, unsent data will be lost.

enumerator ESP\_SLEEP\_ALWAYS\_DISCARD\_UART [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N30esp_sleep_uart_handling_mode_t29ESP_SLEEP_ALWAYS_DISCARD_UARTE "Permalink to this definition")  

Discard all data in TX/RX FIFOs and enter sleep immediately. Fastest sleep entry and lowest power, but all unsent data is lost.

enumerator ESP\_SLEEP\_NO\_HANDLING [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/sleep_modes.html#_CPPv4N30esp_sleep_uart_handling_mode_t21ESP_SLEEP_NO_HANDLINGE "Permalink to this definition")  

Do not perform any UART handling before sleep. UART state is not modified.