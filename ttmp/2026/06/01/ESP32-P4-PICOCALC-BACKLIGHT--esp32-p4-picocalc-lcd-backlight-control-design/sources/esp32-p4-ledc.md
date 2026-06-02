---
Title: Source - esp32-p4-ledc
Ticket: ESP32-P4-PICOCALC-BACKLIGHT
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-BACKLIGHT"
---

## LED Control (LEDC)

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-reference/peripherals/ledc.html)

## Introduction

The LED control (LEDC) peripheral is primarily designed to control the intensity of LEDs, although it can also be used to generate PWM signals for other purposes. It has 8 channels which can generate independent waveforms that can be used, for example, to drive RGB LED devices.

The PWM controller can automatically increase or decrease the duty cycle gradually, allowing for fades without any processor interference.

## Functionality Overview

Setting up a channel of the LEDC is done in three steps. Note that unlike ESP32, ESP32-P4 only supports configuring channels in "low speed" mode.

1. by specifying the PWM signal's frequency and duty cycle resolution.
2. by associating it with the timer and GPIO to output the PWM signal.
3. that drives the output in order to change LED's intensity. This can be done under the full control of software or with hardware fading functions.

As an optional step, it is also possible to set up an interrupt on fade end.

![Key Settings of LED PWM Controller's API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/_images/ledc-api-settings.jpg)

Key Settings of LED PWM Controller's API 

Note

For an initial setup, it is recommended to configure for the timers first (by calling ), and then for the channels (by calling ). This ensures the PWM frequency is at the desired value since the appearance of the PWM signal from the IO pad.

### Timer Configuration

Setting the timer is done by calling the function and passing the data structure that contains the following configuration settings:

- Speed mode (value must be `LEDC_LOW_SPEED_MODE`)
- Timer number
- PWM signal frequency in Hz
- Resolution of PWM duty
- Source clock

The frequency and the duty resolution are interdependent. The higher the PWM frequency, the lower the duty resolution which is available, and vice versa. This relationship might be important if you are planning to use this API for purposes other than changing the intensity of LEDs. For more details, see Section.

The source clock can also limit the PWM frequency. The higher the source clock frequency, the higher the maximum PWM frequency can be configured.

| Clock name | Clock freq | Clock capabilities |
| --- | --- | --- |
| PLL\_80M\_CLK | 80 MHz | / |
| RC\_FAST\_CLK | ~ 17.5 MHz | Dynamic Frequency Scaling compatible, Light-sleep compatible |
| XTAL\_CLK | 40 MHz | Dynamic Frequency Scaling compatible |

Note

1. On ESP32-P4, if RC\_FAST\_CLK is chosen as the LEDC clock source, an internal calibration will be performed to get the exact frequency of the clock. This ensures the accuracy of output PWM signal frequency.
1. For ESP32-P4, all timers share one clock source. In other words, it is impossible to use different clock sources for different timers.

The LEDC driver offers a helper function to find the maximum possible resolution for the timer, given the source clock frequency and the desired PWM signal frequency.

When a timer is no longer needed by any channel, it can be deconfigured by calling the same function. The configuration structure passes in should be:

- The speed mode of the timer which wants to be deconfigured belongs to ()
- The ID of the timers which wants to be deconfigured ()
- Set this to true so that the timer specified can be deconfigured

### Channel Configuration

When the timer is set up, configure the desired channel (one out of ). This is done by calling the function.

Similar to the timer configuration, the channel setup function should be passed a structure that contains the channel's configuration parameters.

At this point, the channel should start operating and generating the PWM signal on the selected GPIO, as configured in, with the frequency specified in the timer settings and the given duty cycle. The channel operation (signal generation) can be suspended at any time by calling the function.

### Change PWM Signal

Once the channel starts operating and generating the PWM signal with the constant duty cycle and frequency, there are a couple of ways to change this signal. When driving LEDs, primarily the duty cycle is changed to vary the light intensity.

The following two sections describe how to change the duty cycle using software and hardware fading. If required, the signal's frequency can also be changed; it is covered in Section.

Note

All the timers and channels in the ESP32-P4's LED PWM Controller only support low speed mode. Any change of PWM settings must be explicitly triggered by software (see below).

#### Change PWM Duty Cycle Using Software

To set the duty cycle, use the dedicated function. After that, call to activate the changes. To check the currently set value, use the corresponding `_get_` function.

Another way to set the duty cycle, as well as some other channel parameters, is by calling covered in Section.

The range of the duty cycle values passed to functions depends on selected `duty_resolution` and should be from `0` to `(2 ** duty_resolution)`. For example, if the selected duty resolution is 10, then the duty cycle values can range from 0 to 1024. This provides the resolution of ~ 0.1%.

Warning

On ESP32-P4, when channel's binded timer selects its maximum duty resolution, the duty cycle value cannot be set to `(2 ** duty_resolution)`. Otherwise, the internal duty counter in the hardware will overflow and be messed up.

The hardware limitation above only applies to chip revision before v3.0.

#### Change PWM Duty Cycle Using Hardware

The LEDC hardware provides the means to gradually transition from one duty cycle value to another. To use this functionality, enable fading with and then configure it by calling one of the available fading functions:

On ESP32-P4, the hardware additionally allows to perform up to 16 consecutive linear fades without CPU intervention. This feature can be useful if you want to do a fade with gamma correction.

The luminance perceived by human eyes does not have a linear relationship with the PWM duty cycle. In order to make human feel the LED is dimming or lighting linearly, the change in duty cycle should be non-linear, which is the so-called gamma correction. The LED controller can simulate a gamma curve fading by piecewise linear approximation. is a function that can help to construct the parameters for the piecewise linear fades. First, you need to allocate a memory block for saving the fade parameters, then by providing start/end PWM duty cycle values, gamma correction function, and the total number of desired linear segments to the helper function, it will fill the calculation results into the allocated space. You can also construct the array of manually. Once the fade parameter structs are prepared, a consecutive fading can be configured by passing the pointer to the prepared list and the total number of fade ranges to.

Start fading with. A fade can be operated in blocking or non-blocking mode, please check for the difference between the two available fade modes. Note that with either fade mode, the next fade or fixed-duty update will not take effect until the last fade finishes or is stopped. has to be called to stop a fade that is in progress.

To get a notification about the completion of a fade operation, a fade end callback function can be registered for each channel by calling after the fade service being installed. The fade end callback prototype is defined in, where you should return a boolean value from the callback function, indicating whether a high priority task is woken up by this callback function. It is worth mentioning, the callback and the function invoked by itself should be placed in IRAM, as the interrupt service routine is in IRAM. will print a warning message if it finds the addresses of callback and user context are incorrect.

If not required anymore, fading and an associated interrupt can be disabled with.

#### Change PWM Frequency

The LEDC API provides several ways to change the PWM frequency "on the fly":

> - Set the frequency by calling. There is a corresponding function to check the current frequency.
> - Change the frequency and the duty resolution by calling to bind some other timer to the channel.
> - Change the channel's timer by calling.

## LEDC's ETM Events and Tasks

LEDC can generate various events that can be connected to the [ETM](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/etm.html) module. Timer's supported events are listed in, and channel's supported events are listed in. Users can create an `ETM event` handle by calling or respectively. LEDC also supports some tasks that can be triggered by other events and executed automatically. Timer's supported tasks are listed in, and channel's supported tasks are listed in. Users can create an `ETM task` handle by calling or respectively.

Some useful applications of ETM with LEDC are:

> - To generate a PWM signal with certain number of pulses
> - To synchronize the PWM period with an external signal
> - To start / stop the PWM signal output or a fading without CPU intervention

For how to connect the LEDC events and tasks to the ETM channel, please refer to the [ETM](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/etm.html) documentation.

## Power Management

LEDC driver does not utilize power management lock to prevent the system from going into Light-sleep. Instead, the LEDC peripheral power domain state and the PWM signal output behavior during sleep can be chosen by configuring. The default mode is, which stands for no signal output and LEDC power domain will not be powered down during sleep.

If signal output needs to be maintained in Light-sleep, then select. As long as the binded LEDC timer clock source is Light-sleep compatible, the PWM signal can continue its output even the system enters Light-sleep. The cost is a higher power consumption in sleep, since the clock source and the power domain where LEDC belongs to cannot be powered down. Note that, if there is an unfinished fade before entering sleep, the fade can also continue during sleep, but the target duty might not be reached exactly. It will adjust to the target duty after wake-up.

There is another sleep mode,, can save some power consumption in sleep, but at the expense of more memory being consumed. The system retains LEDC register context before entering Light-sleep and restores them after waking up, so that the LEDC power domain can be powered down during sleep. Any unfinished fade will not resume upon waking up from sleep, instead, it will output a PWM signal with a fixed duty cycle that matches the duty cycle just before entering sleep.

## Supported Range of Frequency and Duty Resolutions

The LED PWM Controller is designed primarily to drive LEDs. It provides a large flexibility of PWM duty cycle settings. For instance, the PWM frequency of 5 kHz can have the maximum duty resolution of 13 bits. This means that the duty can be set anywhere from 0 to 100% with a resolution of ~ 0.012% (2 \*\* 13 = 8192 discrete levels of the LED intensity). Note, however, that these parameters depend on the clock signal clocking the LED PWM Controller timer which in turn clocks the channel (see and the **ESP32-P4 Technical Reference Manual** > **LED PWM Controller (LEDC)** \[[PDF](https://www.espressif.com/sites/default/files/documentation/esp32-p4_technical_reference_manual_en.pdf#ledpwm)\]).

The LEDC can be used for generating signals at much higher frequencies that are sufficient enough to clock other devices, e.g., a digital camera module. In this case, the maximum available frequency is 40 MHz with duty resolution of 1 bit. This means that the duty cycle is fixed at 50% and cannot be adjusted.

The LEDC API is designed to report an error when trying to set a frequency and a duty resolution that exceed the range of LEDC's hardware. For example, an attempt to set the frequency to 20 MHz and the duty resolution to 3 bits results in the following error reported on a serial monitor:

```
E (196) ledc: requested frequency and duty resolution cannot be achieved, try reducing freq_hz or duty_resolution. div_param=128
```

In such a situation, either the duty resolution or the frequency must be reduced. For example, setting the duty resolution to 2 resolves this issue and makes it possible to set the duty cycle at 25% steps, i.e., at 25%, 50% or 75%.

The LEDC driver also captures and reports attempts to configure frequency/duty resolution combinations that are below the supported minimum, e.g.,:

```
E (196) ledc: requested frequency and duty resolution cannot be achieved, try increasing freq_hz or duty_resolution. div_param=128000000
```

The duty resolution is normally set using. This enumeration covers the range from 10 to 15 bits. If a smaller duty resolution is required (from 10 down to 1), enter the equivalent numeric values directly.

## Application Example

- [peripherals/ledc/ledc\_basic](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/ledc/ledc_basic) demonstrates how to use the LEDC to generate a PWM signal in LOW SPEED mode.
- [peripherals/ledc/ledc\_fade](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/ledc/ledc_fade) demonstrates how to control the intensity of LEDs using the LEDC fade functionality.
- [peripherals/ledc/ledc\_gamma\_curve\_fade](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/ledc/ledc_gamma_curve_fade) demonstrates how to use the LEDC for color control of RGB LEDs with gamma correction.
- [peripherals/ledc/ledc\_dimmer](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/peripherals/ledc/ledc_dimmer) demonstrates how to use the LEDC and ETM to generate TRIAC gate trigger pulses that are synchronized to the mains zero‑cross.

## API Reference

### Header File

- [components/esp\_driver\_ledc/include/driver/ledc.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_ledc/include/driver/ledc.h)
- This header file can be included with:
	> ```c
	> #include "driver/ledc.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_ledc` component. To declare that your component depends on `esp_driver_ledc`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_ledc
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_ledc
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_channel\_config(const \*ledc\_conf) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv419ledc_channel_configPK21ledc_channel_config_t "Permalink to this definition")  

LEDC channel configuration Configure LEDC channel with the given channel/output gpio\_num/interrupt/source timer/frequency(Hz)/LEDC duty.

Parameters:

**ledc\_conf** -- Pointer of LEDC channel configure struct

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error

uint32\_t ledc\_find\_suitable\_duty\_resolution(uint32\_t src\_clk\_freq, uint32\_t timer\_freq) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv434ledc_find_suitable_duty_resolution8uint32_t8uint32_t "Permalink to this definition")  

Helper function to find the maximum possible duty resolution in bits for ledc\_timer\_config()

Parameters:

- **src\_clk\_freq** -- LEDC timer source clock frequency (Hz) (See doxygen comments of `ledc_clk_cfg_t` or get from `esp_clk_tree_src_get_freq_hz`)
- **timer\_freq** -- Desired LEDC timer frequency (Hz)

Returns:

- 0 The timer frequency cannot be achieved
- Others The largest duty resolution value to be set

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_timer\_config(const \*timer\_conf) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv417ledc_timer_configPK19ledc_timer_config_t "Permalink to this definition")  

LEDC timer configuration Configure LEDC timer with the given source timer/frequency(Hz)/duty\_resolution.

Parameters:

**timer\_conf** -- Pointer of LEDC timer configure struct

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_FAIL Can not find a proper pre-divider number base on the given frequency and the current duty\_resolution.
- ESP\_ERR\_INVALID\_STATE Timer cannot be de-configured because timer is not configured or is not paused

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_update\_duty( speed\_mode, channel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv416ledc_update_duty11ledc_mode_t14ledc_channel_t "Permalink to this definition")  

LEDC update channel parameters.

Note

Call this function to activate the LEDC updated parameters. After ledc\_set\_duty, we need to call this function to update the settings. And the new LEDC parameters don't take effect until the next PWM cycle.

Note

ledc\_set\_duty, ledc\_set\_duty\_with\_hpoint and ledc\_update\_duty are not thread-safe, do not call these functions to control one LEDC channel in different tasks at the same time. A thread-safe version of API is ledc\_set\_duty\_and\_update

Note

If `CONFIG_LEDC_CTRL_FUNC_IN_IRAM` is enabled, this function will be placed in the IRAM by linker, makes it possible to execute even when the Cache is disabled.

Note

This function is allowed to run within ISR context.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_pin(int gpio\_num, speed\_mode, channel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv412ledc_set_pini11ledc_mode_t14ledc_channel_t "Permalink to this definition")  

Set LEDC output gpio.

Note

This function only routes the LEDC signal to GPIO through matrix, other LEDC resources initialization are not involved. Please use `ledc_channel_config()` instead to fully configure a LEDC channel.

Parameters:

- **gpio\_num** -- The LEDC output gpio
- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_stop( speed\_mode, channel, uint32\_t idle\_level) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv49ledc_stop11ledc_mode_t14ledc_channel_t8uint32_t "Permalink to this definition")  

LEDC stop. Disable LEDC output, and set idle level.

Note

If `CONFIG_LEDC_CTRL_FUNC_IN_IRAM` is enabled, this function will be placed in the IRAM by linker, makes it possible to execute even when the Cache is disabled.

Note

This function is allowed to run within ISR context.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **idle\_level** -- Set output idle level after LEDC stops.

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_freq( speed\_mode, timer\_num, uint32\_t freq\_hz) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv413ledc_set_freq11ledc_mode_t12ledc_timer_t8uint32_t "Permalink to this definition")  

LEDC set channel frequency (Hz)

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **timer\_num** -- LEDC timer index (0-3), select from ledc\_timer\_t
- **freq\_hz** -- Set the LEDC frequency

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_FAIL Can not find a proper pre-divider number base on the given frequency and the current duty\_resolution.

uint32\_t ledc\_get\_freq( speed\_mode, timer\_num) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv413ledc_get_freq11ledc_mode_t12ledc_timer_t "Permalink to this definition")  

LEDC get channel frequency (Hz)

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **timer\_num** -- LEDC timer index (0-3), select from ledc\_timer\_t

Returns:

- 0 error
- Others Current LEDC frequency

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_duty\_with\_hpoint( speed\_mode, channel, uint32\_t duty, uint32\_t hpoint) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv425ledc_set_duty_with_hpoint11ledc_mode_t14ledc_channel_t8uint32_t8uint32_t "Permalink to this definition")  

LEDC set duty and hpoint value Only after calling ledc\_update\_duty will the duty update.

Note

ledc\_set\_duty, ledc\_set\_duty\_with\_hpoint and ledc\_update\_duty are not thread-safe, do not call these functions to control one LEDC channel in different tasks at the same time. A thread-safe version of API is ledc\_set\_duty\_and\_update

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **duty** -- Set the LEDC duty, the range of duty setting is \[0, (2\*\*duty\_resolution)\]
- **hpoint** -- Set the LEDC hpoint value, the range is \[0, (2\*\*duty\_resolution)-1\]

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error

int ledc\_get\_hpoint( speed\_mode, channel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv415ledc_get_hpoint11ledc_mode_t14ledc_channel_t "Permalink to this definition")  

LEDC get hpoint value, the counter value when the output is set high level.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t

Returns:

- LEDC\_ERR\_VAL if parameter error
- Others Current hpoint value of LEDC channel

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_duty( speed\_mode, channel, uint32\_t duty) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv413ledc_set_duty11ledc_mode_t14ledc_channel_t8uint32_t "Permalink to this definition")  

LEDC set duty This function do not change the hpoint value of this channel. if needed, please call ledc\_set\_duty\_with\_hpoint. only after calling ledc\_update\_duty will the duty update.

Note

ledc\_set\_duty, ledc\_set\_duty\_with\_hpoint and ledc\_update\_duty are not thread-safe, do not call these functions to control one LEDC channel in different tasks at the same time. A thread-safe version of API is ledc\_set\_duty\_and\_update.

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **duty** -- Set the LEDC duty, the range of duty setting is \[0, (2\*\*duty\_resolution)\]

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error

uint32\_t ledc\_get\_duty( speed\_mode, channel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv413ledc_get_duty11ledc_mode_t14ledc_channel_t "Permalink to this definition")  

LEDC get duty This function returns the duty at the present PWM cycle. You shouldn't expect the function to return the new duty in the same cycle of calling ledc\_update\_duty, because duty update doesn't take effect until the next cycle.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t

Returns:

- LEDC\_ERR\_DUTY if parameter error
- Others Current LEDC duty

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_fade( speed\_mode, channel, uint32\_t duty, fade\_direction, uint32\_t step\_num, uint32\_t duty\_cycle\_num, uint32\_t duty\_scale) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv413ledc_set_fade11ledc_mode_t14ledc_channel_t8uint32_t21ledc_duty_direction_t8uint32_t8uint32_t8uint32_t "Permalink to this definition")  

LEDC set gradient Set LEDC gradient, After the function calls the ledc\_update\_duty function, the function can take effect.

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **duty** -- Set the start of the gradient duty, the range of duty setting is \[0, (2\*\*duty\_resolution)\]
- **fade\_direction** -- Set the direction of the gradient
- **step\_num** -- Set the number of the gradient
- **duty\_cycle\_num** -- Set how many LEDC tick each time the gradient lasts
- **duty\_scale** -- Set gradient change amplitude

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error

Register LEDC interrupt handler, the handler is an ISR. The handler will be attached to the same CPU core that this function is running on.

Parameters:

- **fn** -- Interrupt handler function.
- **arg** -- User-supplied argument passed to the handler function.
- **intr\_alloc\_flags** -- Flags used to allocate the interrupt. One or multiple (ORred) ESP\_INTR\_FLAG\_\* values. See esp\_intr\_alloc.h for more info.
- **handle** -- Pointer to return handle. If non-NULL, a handle for the interrupt will be returned here.

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_NOT\_FOUND Failed to find available interrupt source

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_timer\_rst( speed\_mode, timer\_sel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv414ledc_timer_rst11ledc_mode_t12ledc_timer_t "Permalink to this definition")  

Reset LEDC timer.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **timer\_sel** -- LEDC timer index (0-3), select from ledc\_timer\_t

Returns:

- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_OK Success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_timer\_pause( speed\_mode, timer\_sel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv416ledc_timer_pause11ledc_mode_t12ledc_timer_t "Permalink to this definition")  

Pause LEDC timer counter.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **timer\_sel** -- LEDC timer index (0-3), select from ledc\_timer\_t

Returns:

- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_OK Success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_timer\_resume( speed\_mode, timer\_sel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv417ledc_timer_resume11ledc_mode_t12ledc_timer_t "Permalink to this definition")  

Resume LEDC timer.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **timer\_sel** -- LEDC timer index (0-3), select from ledc\_timer\_t

Returns:

- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_OK Success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_bind\_channel\_timer( speed\_mode, channel, timer\_sel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv423ledc_bind_channel_timer11ledc_mode_t14ledc_channel_t12ledc_timer_t "Permalink to this definition")  

Bind LEDC channel with the selected timer.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **timer\_sel** -- LEDC timer index (0-3), select from ledc\_timer\_t

Returns:

- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_OK Success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_channel\_configure\_maximum\_timer\_ovf\_cnt( speed\_mode, channel, uint32\_t max\_ovf\_cnt) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv444ledc_channel_configure_maximum_timer_ovf_cnt11ledc_mode_t14ledc_channel_t8uint32_t "Permalink to this definition")  

Configure the maximum timer overflow times for the LEDC channel to be used to trigger `LEDC_ETM_EVENT_CHANNEL_REACH_MAX_OVF_CNT` ETM event.

When the overflow counter maximum value is re-configured, the counter will also be reset. Timer can be paused before calling this API by calling `ledc_timer_pause()`, and resumed afterwards by calling `ledc_timer_resume()`.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **max\_ovf\_cnt** -- The timer overflow counter maximum value. To disable the timer overflow count, set this parameter to 0.

Returns:

- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_OK Success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_fade\_with\_step( speed\_mode, channel, uint32\_t target\_duty, uint32\_t scale, uint32\_t cycle\_num) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv423ledc_set_fade_with_step11ledc_mode_t14ledc_channel_t8uint32_t8uint32_t8uint32_t "Permalink to this definition")  

Set LEDC fade function.

Note

Call ledc\_fade\_func\_install() once before calling this function. Call ledc\_fade\_start() after this to start fading.

Note

ledc\_set\_fade\_with\_step, ledc\_set\_fade\_with\_time and ledc\_fade\_start are not thread-safe, do not call these functions to control one LEDC channel in different tasks at the same time. A thread-safe version of API is ledc\_set\_fade\_step\_and\_start

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **target\_duty** -- Target duty of fading \[0, (2\*\*duty\_resolution)\]
- **scale** -- Controls the increase or decrease step scale.
- **cycle\_num** -- increase or decrease the duty every cycle\_num cycles

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_fade\_with\_time( speed\_mode, channel, uint32\_t target\_duty, int desired\_fade\_time\_ms) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv423ledc_set_fade_with_time11ledc_mode_t14ledc_channel_t8uint32_ti "Permalink to this definition")  

Set LEDC fade function, with a limited time.

Note

Call ledc\_fade\_func\_install() once before calling this function. Call ledc\_fade\_start() after this to start fading.

Note

ledc\_set\_fade\_with\_step, ledc\_set\_fade\_with\_time and ledc\_fade\_start are not thread-safe, do not call these functions to control one LEDC channel in different tasks at the same time. A thread-safe version of API is ledc\_set\_fade\_step\_and\_start

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **target\_duty** -- Target duty of fading \[0, (2\*\*duty\_resolution)\]
- **desired\_fade\_time\_ms** -- The intended time of the fading ( ms ). Note that the actual time it takes to complete the fade could vary by a factor of up to 2x shorter or longer than the expected time due to internal rounding errors in calculations. Specifically:
	- The total number of cycles (total\_cycle\_num = desired\_fade\_time\_ms \* freq / 1000)
		- The difference in duty cycle (duty\_delta = |target\_duty - current\_duty|) The fade may complete faster than expected if total\_cycle\_num larger than duty\_delta. Conversely, it may take longer than expected if total\_cycle\_num is less than duty\_delta. The closer the ratio of total\_cycle\_num/duty\_delta (or its inverse) is to a whole number (the floor value), the more accurately the actual fade duration will match the intended time. If an exact fade time is expected, please consider to split the entire fade into several smaller linear fades. The split should make each fade step has a divisible total\_cycle\_num/duty\_delta (or its inverse) ratio.

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_fade\_func\_install(int intr\_alloc\_flags) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv422ledc_fade_func_installi "Permalink to this definition")  

Install LEDC fade function. This function will occupy interrupt of LEDC module.

Parameters:

**intr\_alloc\_flags** -- Flags used to allocate the interrupt. One or multiple (ORred) ESP\_INTR\_FLAG\_\* values. See esp\_intr\_alloc.h for more info.

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Intr flag error
- ESP\_ERR\_NOT\_FOUND Failed to find available interrupt source
- ESP\_ERR\_INVALID\_STATE Fade function already installed

void ledc\_fade\_func\_uninstall(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv424ledc_fade_func_uninstallv "Permalink to this definition")  

Uninstall LEDC fade function.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_fade\_start( speed\_mode, channel, fade\_mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv415ledc_fade_start11ledc_mode_t14ledc_channel_t16ledc_fade_mode_t "Permalink to this definition")  

Start LEDC fading.

Note

Call ledc\_fade\_func\_install() once before calling this function. Call this API right after ledc\_set\_fade\_with\_time or ledc\_set\_fade\_with\_step before to start fading.

Note

Starting fade operation with this API is not thread-safe, use with care.

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel number
- **fade\_mode** -- Whether to block until fading done. See ledc\_types.h ledc\_fade\_mode\_t for more info. Note that this function will not return until fading to the target duty if LEDC\_FADE\_WAIT\_DONE mode is selected.

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_STATE Channel not initialized or fade function not installed.
- ESP\_ERR\_INVALID\_ARG Parameter error.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_fade\_stop( speed\_mode, channel) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv414ledc_fade_stop11ledc_mode_t14ledc_channel_t "Permalink to this definition")  

Stop LEDC fading. The duty of the channel is guaranteed to be fixed at most one PWM cycle after the function returns.

Note

This API can be called if a new fixed duty or a new fade want to be set while the last fade operation is still running in progress.

Note

Call this API will abort the fading operation only if it was started by calling ledc\_fade\_start with LEDC\_FADE\_NO\_WAIT mode.

Note

If a fade was started with LEDC\_FADE\_WAIT\_DONE mode, calling this API afterwards has no use in stopping the fade. Fade will continue until it reaches the target duty.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel number

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_duty\_and\_update( speed\_mode, channel, uint32\_t duty, uint32\_t hpoint) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv424ledc_set_duty_and_update11ledc_mode_t14ledc_channel_t8uint32_t8uint32_t "Permalink to this definition")  

A thread-safe API to set duty for LEDC channel and return when duty updated.

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **duty** -- Set the LEDC duty, the range of duty setting is \[0, (2\*\*duty\_resolution)\]
- **hpoint** -- Set the LEDC hpoint value, the range is \[0, (2\*\*duty\_resolution)-1\]

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_fade\_time\_and\_start( speed\_mode, channel, uint32\_t target\_duty, uint32\_t desired\_fade\_time\_ms, fade\_mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv428ledc_set_fade_time_and_start11ledc_mode_t14ledc_channel_t8uint32_t8uint32_t16ledc_fade_mode_t "Permalink to this definition")  

A thread-safe API to set and start LEDC fade function, with a limited time.

Note

Call ledc\_fade\_func\_install() once, before calling this function.

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **target\_duty** -- Target duty of fading \[0, (2\*\*duty\_resolution)\]
- **desired\_fade\_time\_ms** -- The intended time of the fading ( ms ). Note that the actual time it takes to complete the fade could vary by a factor of up to 2x shorter or longer than the expected time due to internal rounding errors in calculations. Specifically:
	- The total number of cycles (total\_cycle\_num = desired\_fade\_time\_ms \* freq / 1000)
		- The difference in duty cycle (duty\_delta = |target\_duty - current\_duty|) The fade may complete faster than expected if total\_cycle\_num larger than duty\_delta. Conversely, it may take longer than expected if total\_cycle\_num is less than duty\_delta. The closer the ratio of total\_cycle\_num/duty\_delta (or its inverse) is to a whole number (the floor value), the more accurately the actual fade duration will match the intended time. If an exact fade time is expected, please consider to split the entire fade into several smaller linear fades. The split should make each fade step has a divisible total\_cycle\_num/duty\_delta (or its inverse) ratio.
- **fade\_mode** -- choose blocking or non-blocking mode

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_fade\_step\_and\_start( speed\_mode, channel, uint32\_t target\_duty, uint32\_t scale, uint32\_t cycle\_num, fade\_mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv428ledc_set_fade_step_and_start11ledc_mode_t14ledc_channel_t8uint32_t8uint32_t8uint32_t16ledc_fade_mode_t "Permalink to this definition")  

A thread-safe API to set and start LEDC fade function.

Note

Call ledc\_fade\_func\_install() once before calling this function.

Note

For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. Other duty operations will have to wait until the fade operation has finished.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **target\_duty** -- Target duty of fading \[0, (2\*\*duty\_resolution)\]
- **scale** -- Controls the increase or decrease step scale.
- **cycle\_num** -- increase or decrease the duty every cycle\_num cycles
- **fade\_mode** -- choose blocking or non-blocking mode

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Fade function init error

LEDC callback registration function.

Note

The callback is called from an ISR, it must never attempt to block, and any FreeRTOS API called must be ISR capable.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **cbs** -- Group of LEDC callback functions
- **user\_arg** -- user registered data for the callback function

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_multi\_fade( speed\_mode, channel, uint32\_t start\_duty, const \*fade\_params\_list, uint32\_t list\_len) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv419ledc_set_multi_fade11ledc_mode_t14ledc_channel_t8uint32_tPK24ledc_fade_param_config_t8uint32_t "Permalink to this definition")  

Set a LEDC multi-fade.

Note

Call `ledc_fade_func_install()` once before calling this function. Call `ledc_fade_start()` after this to start fading.

Note

This function is not thread-safe, do not call it to control one LEDC channel in different tasks at the same time. A thread-safe version of API is ledc\_set\_multi\_fade\_and\_start

Note

This function does not prohibit from duty overflow. User should take care of this by themselves. If duty overflow happens, the PWM signal will suddenly change from 100% duty cycle to 0%, or the other way around.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **start\_duty** -- Set the start of the gradient duty, the range of duty setting is \[0, (2\*\*duty\_resolution)\]
- **fade\_params\_list** -- Pointer to the array of fade parameters for a multi-fade
- **list\_len** -- Length of the fade\_params\_list, i.e. number of fade ranges for a multi-fade (1 - SOC\_LEDC\_GAMMA\_CURVE\_FADE\_RANGE\_MAX)

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_set\_multi\_fade\_and\_start( speed\_mode, channel, uint32\_t start\_duty, const \*fade\_params\_list, uint32\_t list\_len, fade\_mode) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv429ledc_set_multi_fade_and_start11ledc_mode_t14ledc_channel_t8uint32_tPK24ledc_fade_param_config_t8uint32_t16ledc_fade_mode_t "Permalink to this definition")  

A thread-safe API to set and start LEDC multi-fade function.

Note

Call `ledc_fade_func_install()` once before calling this function.

Note

Fade will always begin from the current duty cycle. Make sure it is stable and synchronized to the desired initial value before calling this function. Otherwise, you may see unexpected duty change.

Note

This function does not prohibit from duty overflow. User should take care of this by themselves. If duty overflow happens, the PWM signal will suddenly change from 100% duty cycle to 0%, or the other way around.

Parameters:

- **speed\_mode** -- Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **start\_duty** -- Set the start of the gradient duty, the range of duty setting is \[0, (2\*\*duty\_resolution)\]
- **fade\_params\_list** -- Pointer to the array of fade parameters for a multi-fade
- **list\_len** -- Length of the fade\_params\_list, i.e. number of fade ranges for a multi-fade (1 - SOC\_LEDC\_GAMMA\_CURVE\_FADE\_RANGE\_MAX)
- **fade\_mode** -- Choose blocking or non-blocking mode

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Fade function init error

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_fill\_multi\_fade\_param\_list( speed\_mode, channel, uint32\_t start\_duty, uint32\_t end\_duty, uint32\_t linear\_phase\_num, uint32\_t max\_fade\_time\_ms, uint32\_t (\*gamma\_correction\_operator)(uint32\_t), uint32\_t fade\_params\_list\_size, \*fade\_params\_list, uint32\_t \*hw\_fade\_range\_num) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv431ledc_fill_multi_fade_param_list11ledc_mode_t14ledc_channel_t8uint32_t8uint32_t8uint32_t8uint32_tPF8uint32_t8uint32_tE8uint32_tP24ledc_fade_param_config_tP8uint32_t "Permalink to this definition")  

Helper function to fill the fade params for a multi-fade. Useful if desires a gamma curve fading.

Note

The fade params are calculated based on the given start\_duty and end\_duty. If the duty is not at the start duty (gamma-corrected) when the fade begins, you may see undesired brightness change. Therefore, please always remember thet when passing the fade\_params to either `ledc_set_multi_fade` or `ledc_set_multi_fade_and start`, the start\_duty argument has to be the gamma-corrected start\_duty.

Parameters:

- **speed\_mode** -- **\[in\]** Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- **\[in\]** LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **start\_duty** -- **\[in\]** Duty cycle \[0, (2\*\*duty\_resolution)\] where the multi-fade begins with. This value should be a non-gamma-corrected duty cycle.
- **end\_duty** -- **\[in\]** Duty cycle \[0, (2\*\*duty\_resolution)\] where the multi-fade ends with. This value should be a non-gamma-corrected duty cycle.
- **linear\_phase\_num** -- **\[in\]** Number of linear fades to simulate a gamma curved fade (1 - SOC\_LEDC\_GAMMA\_CURVE\_FADE\_RANGE\_MAX)
- **max\_fade\_time\_ms** -- **\[in\]** The maximum time of the fading ( ms ).
- **gamma\_correction\_operator** -- **\[in\]** User provided gamma correction function. The function argument should be able to take any value within \[0, (2\*\*duty\_resolution)\]. And returns the gamma-corrected duty cycle.
- **fade\_params\_list\_size** -- **\[in\]** The size of the fade\_params\_list user allocated (1 - SOC\_LEDC\_GAMMA\_CURVE\_FADE\_RANGE\_MAX)
- **fade\_params\_list** -- **\[out\]** Pointer to the array of structure
- **hw\_fade\_range\_num** -- **\[out\]** Number of fade ranges for this multi-fade

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized
- ESP\_FAIL Required number of hardware ranges exceeds the size of the array user allocated

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_read\_fade\_param( speed\_mode, channel, uint32\_t range, uint32\_t \*dir, uint32\_t \*cycle, uint32\_t \*scale, uint32\_t \*step) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv420ledc_read_fade_param11ledc_mode_t14ledc_channel_t8uint32_tP8uint32_tP8uint32_tP8uint32_tP8uint32_t "Permalink to this definition")  

Get the fade parameters that are stored in gamma ram for a certain fade range.

Gamma ram is where saves the fade parameters for each fade range. The fade parameters are written in during fade configuration. When fade begins, the duty will change according to the parameters in gamma ram.

Parameters:

- **speed\_mode** -- **\[in\]** Select the LEDC channel group with specified speed mode. Note that not all targets support high speed mode.
- **channel** -- **\[in\]** LEDC channel index (0 - LEDC\_CHANNEL\_MAX-1), select from ledc\_channel\_t
- **range** -- **\[in\]** Range index (0 - (SOC\_LEDC\_GAMMA\_CURVE\_FADE\_RANGE\_MAX-1)), it specifies to which range in gamma ram to read
- **dir** -- **\[out\]** Pointer to accept fade direction value
- **cycle** -- **\[out\]** Pointer to accept fade cycle value
- **scale** -- **\[out\]** Pointer to accept fade scale value
- **step** -- **\[out\]** Pointer to accept fade step value

Returns:

- ESP\_OK Success
- ESP\_ERR\_INVALID\_ARG Parameter error
- ESP\_ERR\_INVALID\_STATE Channel not initialized

### Structures

struct ledc\_channel\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv421ledc_channel_config_t "Permalink to this definition")  

Configuration parameters of LEDC channel for ledc\_channel\_config function.

Public Members

int gpio\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t8gpio_numE "Permalink to this definition")  

the LEDC output gpio\_num, if you want to use gpio16, gpio\_num = 16

speed\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t10speed_modeE "Permalink to this definition")  

LEDC speed speed\_mode, high-speed mode (only exists on esp32) or low-speed mode

channel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t7channelE "Permalink to this definition")  

LEDC channel (0 - LEDC\_CHANNEL\_MAX-1)

intr\_type [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t9intr_typeE "Permalink to this definition")  

*Deprecated:*

, no need to explicitly configure interrupt, handled in the driver

timer\_sel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t9timer_selE "Permalink to this definition")  

Select the timer source of channel (0 - LEDC\_TIMER\_MAX-1)

uint32\_t duty [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t4dutyE "Permalink to this definition")  

LEDC channel duty, the range of duty setting is \[0, (2\*\*duty\_resolution)\]

int hpoint [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t6hpointE "Permalink to this definition")  

LEDC channel hpoint value, the range is \[0, (2\*\*duty\_resolution)-1\]

sleep\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t10sleep_modeE "Permalink to this definition")  

choose the desired behavior for the LEDC channel in Light-sleep

struct:: flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t5flagsE "Permalink to this definition")  

Extra configuration flags for LEDC channel

bool deconfigure [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t11deconfigureE "Permalink to this definition")  

Set this field to de-configure a LEDC channel which has been configured before The driver only does limited action to release the pins occupied by this channel only. When this field is set, gpio\_num, timer\_sel, duty, hpoint, sleep\_mode, flags fields are ignored.

struct ledc\_channel\_flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t18ledc_channel_flagsE "Permalink to this definition")  

Extra configuration flags for LEDC channel.

Public Members

unsigned int output\_invert [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_channel_config_t18ledc_channel_flags13output_invertE "Permalink to this definition")  

Enable (1) or disable (0) gpio output invert

struct ledc\_timer\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv419ledc_timer_config_t "Permalink to this definition")  

Configuration parameters of LEDC timer for ledc\_timer\_config function.

Public Members

speed\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_timer_config_t10speed_modeE "Permalink to this definition")  

LEDC speed speed\_mode, high-speed mode (only exists on esp32) or low-speed mode

duty\_resolution [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_timer_config_t15duty_resolutionE "Permalink to this definition")  

LEDC channel duty resolution

timer\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_timer_config_t9timer_numE "Permalink to this definition")  

The timer source of channel (0 - LEDC\_TIMER\_MAX-1)

uint32\_t freq\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_timer_config_t7freq_hzE "Permalink to this definition")  

LEDC timer frequency (Hz)

clk\_cfg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_timer_config_t7clk_cfgE "Permalink to this definition")  

Configure LEDC source clock from ledc\_clk\_cfg\_t. Note that LEDC\_USE\_RC\_FAST\_CLK and LEDC\_USE\_XTAL\_CLK are non-timer-specific clock sources. You can not have one LEDC timer uses RC\_FAST\_CLK as the clock source and have another LEDC timer uses XTAL\_CLK as its clock source. All chips except esp32 and esp32s2 do not have timer-specific clock sources, which means clock source for all timers must be the same one.

bool deconfigure [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_timer_config_t11deconfigureE "Permalink to this definition")  

Set this field to de-configure a LEDC timer which has been configured before Note that it will not check whether the timer wants to be de-configured is binded to any channel. Also, the timer has to be paused first before it can be de-configured. When this field is set, duty\_resolution, freq\_hz, clk\_cfg fields are ignored.

struct ledc\_cb\_param\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv415ledc_cb_param_t "Permalink to this definition")  

LEDC callback parameter.

Public Members

event [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N15ledc_cb_param_t5eventE "Permalink to this definition")  

Event name

uint32\_t speed\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N15ledc_cb_param_t10speed_modeE "Permalink to this definition")  

Speed mode of the LEDC channel group

uint32\_t channel [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N15ledc_cb_param_t7channelE "Permalink to this definition")  

LEDC channel (0 - LEDC\_CHANNEL\_MAX-1)

uint32\_t duty [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N15ledc_cb_param_t4dutyE "Permalink to this definition")  

LEDC current duty of the channel, the range of duty is \[0, (2\*\*duty\_resolution)\]

struct ledc\_cbs\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv410ledc_cbs_t "Permalink to this definition")  

Group of supported LEDC callbacks.

Note

The callbacks are all running under ISR environment

Public Members

fade\_cb [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N10ledc_cbs_t7fade_cbE "Permalink to this definition")  

LEDC fade\_end callback function

struct ledc\_fade\_param\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv424ledc_fade_param_config_t "Permalink to this definition")  

Structure for the fade parameters for one hardware fade to be written to gamma wr register.

```cpp
*                                  duty ^                 ONE HW LINEAR FADE
*                                       |
*                                       |
*                                       |
*                                       |
*     start_duty + scale * n = end_duty |. . . . . . . . . . . . . . . . . . . . . . . . . .+-
*                                       |                                                   |
*                                       |                                                   |
*                                       |                                          +--------+
*                                       |                                          |        .
*                                       |                                          |        .
*                                       |                                   -------+        .
*                                       |                                  .                .
*                                       |                                .                  .
*                                       |                              .                    .
*                                       |                            .                      .
*                                 ^ --- |. . . . . . . . . .+--------                       .
*                            scale|     |                   |                               .
*                                 |     |                   |                               .
*                                 v --- |. . . . .+---------+                               .
*                                       |         |         .                               .
*                                       |         |         .                               .
*                            start_duty +---------+         .                               .
*                                       |         .         .                               .
*                                       |         .         .                               .
*                                       +----------------------------------------------------------->
*                                                                                                  PWM cycle
*                                       |         |         |                               |
*                                       | 1 step  | 1 step  |                               |
*                                       |<------->|<------->|                               |
*                                       | m cycles  m cycles                                |
*                                       |                                                   |
*                                       <--------------------------------------------------->
*                                                           n total steps
*                                                          cycles = m * n
*
```

Note

Be aware of the maximum value available on each element

Public Members

uint32\_t dir [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N24ledc_fade_param_config_t3dirE "Permalink to this definition")  

Duty change direction. Set 1 as increase, 0 as decrease

uint32\_t cycle\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N24ledc_fade_param_config_t9cycle_numE "Permalink to this definition")  

Number of PWM cycles of each step \[0, 2\*\*SOC\_LEDC\_FADE\_PARAMS\_BIT\_WIDTH-1\]

uint32\_t scale [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N24ledc_fade_param_config_t5scaleE "Permalink to this definition")  

Duty change of each step \[0, 2\*\*SOC\_LEDC\_FADE\_PARAMS\_BIT\_WIDTH-1\]

uint32\_t step\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N24ledc_fade_param_config_t8step_numE "Permalink to this definition")  

Total number of steps in one hardware fade \[0, 2\*\*SOC\_LEDC\_FADE\_PARAMS\_BIT\_WIDTH-1\]

### Type Definitions

typedef [intr\_handle\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/intr_alloc.html#_CPPv413intr_handle_t "intr_handle_t") ledc\_isr\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv417ledc_isr_handle_t "Permalink to this definition")  

typedef bool (\*ledc\_cb\_t)(const \*param, void \*user\_arg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv49ledc_cb_t "Permalink to this definition")  

Type of LEDC event callback.

Param param:

LEDC callback parameter

Param user\_arg:

User registered data

Return:

Whether a high priority task has been waken up by this function

### Enumerations

enum ledc\_sleep\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv417ledc_sleep_mode_t "Permalink to this definition")  

Strategies to be applied to the LEDC channel during system Light-sleep period.

*Values:*

enumerator LEDC\_SLEEP\_MODE\_NO\_ALIVE\_NO\_PD [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N17ledc_sleep_mode_t30LEDC_SLEEP_MODE_NO_ALIVE_NO_PDE "Permalink to this definition")  

The default mode: no LEDC output, and no power off the LEDC power domain.

enumerator LEDC\_SLEEP\_MODE\_NO\_ALIVE\_ALLOW\_PD [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N17ledc_sleep_mode_t33LEDC_SLEEP_MODE_NO_ALIVE_ALLOW_PDE "Permalink to this definition")  

The low-power-consumption mode: no LEDC output, and allow to power off the LEDC power domain. This can save power, but at the expense of more RAM being consumed to save register context. This option is only available on targets that support TOP domain to be powered down.

enumerator LEDC\_SLEEP\_MODE\_KEEP\_ALIVE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N17ledc_sleep_mode_t26LEDC_SLEEP_MODE_KEEP_ALIVEE "Permalink to this definition")  

The high-power-consumption mode: keep LEDC output when the system enters Light-sleep.

enumerator LEDC\_SLEEP\_MODE\_INVALID [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N17ledc_sleep_mode_t23LEDC_SLEEP_MODE_INVALIDE "Permalink to this definition")  

Invalid LEDC sleep mode strategy

enum ledc\_cb\_event\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv415ledc_cb_event_t "Permalink to this definition")  

LEDC callback event type.

*Values:*

enumerator LEDC\_FADE\_END\_EVT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N15ledc_cb_event_t17LEDC_FADE_END_EVTE "Permalink to this definition")  

LEDC fade end event

### Header File

- [components/esp\_hal\_ledc/include/hal/ledc\_types.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_hal_ledc/include/hal/ledc_types.h)
- This header file can be included with:
	> ```c
	> #include "hal/ledc_types.h"
	> ```
- This header file is a part of the API provided by the `esp_hal_ledc` component. To declare that your component depends on `esp_hal_ledc`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_hal_ledc
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_hal_ledc
	> ```

### Type Definitions

typedef [soc\_periph\_ledc\_clk\_src\_legacy\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/clk_tree.html#_CPPv432soc_periph_ledc_clk_src_legacy_t "soc_periph_ledc_clk_src_legacy_t") ledc\_clk\_cfg\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv414ledc_clk_cfg_t "Permalink to this definition")  

LEDC clock source configuration struct.

In theory, the following enumeration shall be placed in LEDC driver's header. However, as the next enumeration, `ledc_clk_src_t`, makes the use of some of these values and to avoid mutual inclusion of the headers, we must define it here.

### Enumerations

enum ledc\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv411ledc_mode_t "Permalink to this definition")  

*Values:*

enumerator LEDC\_LOW\_SPEED\_MODE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N11ledc_mode_t19LEDC_LOW_SPEED_MODEE "Permalink to this definition")  

LEDC low speed speed\_mode

enumerator LEDC\_SPEED\_MODE\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N11ledc_mode_t19LEDC_SPEED_MODE_MAXE "Permalink to this definition")  

LEDC speed limit

enum ledc\_intr\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv416ledc_intr_type_t "Permalink to this definition")  

*Values:*

enumerator LEDC\_INTR\_DISABLE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_intr_type_t17LEDC_INTR_DISABLEE "Permalink to this definition")  

Disable LEDC interrupt

enumerator LEDC\_INTR\_FADE\_END [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_intr_type_t18LEDC_INTR_FADE_ENDE "Permalink to this definition")  

Enable LEDC interrupt

enumerator LEDC\_INTR\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_intr_type_t13LEDC_INTR_MAXE "Permalink to this definition")  

enum ledc\_duty\_direction\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv421ledc_duty_direction_t "Permalink to this definition")  

*Values:*

enumerator LEDC\_DUTY\_DIR\_DECREASE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_duty_direction_t22LEDC_DUTY_DIR_DECREASEE "Permalink to this definition")  

LEDC duty decrease direction

enumerator LEDC\_DUTY\_DIR\_INCREASE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_duty_direction_t22LEDC_DUTY_DIR_INCREASEE "Permalink to this definition")  

LEDC duty increase direction

enumerator LEDC\_DUTY\_DIR\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N21ledc_duty_direction_t17LEDC_DUTY_DIR_MAXE "Permalink to this definition")  

enum ledc\_slow\_clk\_sel\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv419ledc_slow_clk_sel_t "Permalink to this definition")  

LEDC global clock sources.

*Values:*

enumerator LEDC\_SLOW\_CLK\_RC\_FAST [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_slow_clk_sel_t21LEDC_SLOW_CLK_RC_FASTE "Permalink to this definition")  

LEDC low speed timer clock source is RC\_FAST clock

enumerator LEDC\_SLOW\_CLK\_PLL\_DIV [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_slow_clk_sel_t21LEDC_SLOW_CLK_PLL_DIVE "Permalink to this definition")  

LEDC low speed timer clock source is a PLL\_DIV clock

enumerator LEDC\_SLOW\_CLK\_XTAL [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N19ledc_slow_clk_sel_t18LEDC_SLOW_CLK_XTALE "Permalink to this definition")  

LEDC low speed timer clock source XTAL clock

enum ledc\_clk\_src\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv414ledc_clk_src_t "Permalink to this definition")  

LEDC timer-specific clock sources.

Note: Setting numeric values to match ledc\_clk\_cfg\_t values are a hack to avoid collision with LEDC\_AUTO\_CLK in the driver, as these enums have very similar names and user may pass one of these by mistake.

*Values:*

enumerator LEDC\_SCLK [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_clk_src_t9LEDC_SCLKE "Permalink to this definition")  

Selecting this value for LEDC\_TICK\_SEL\_TIMER let the hardware take its source clock from LEDC\_CLK\_SEL

enum ledc\_timer\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv412ledc_timer_t "Permalink to this definition")  

*Values:*

enumerator LEDC\_TIMER\_0 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N12ledc_timer_t12LEDC_TIMER_0E "Permalink to this definition")  

LEDC timer 0

enumerator LEDC\_TIMER\_1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N12ledc_timer_t12LEDC_TIMER_1E "Permalink to this definition")  

LEDC timer 1

enumerator LEDC\_TIMER\_2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N12ledc_timer_t12LEDC_TIMER_2E "Permalink to this definition")  

LEDC timer 2

enumerator LEDC\_TIMER\_3 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N12ledc_timer_t12LEDC_TIMER_3E "Permalink to this definition")  

LEDC timer 3

enumerator LEDC\_TIMER\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N12ledc_timer_t14LEDC_TIMER_MAXE "Permalink to this definition")  

enum ledc\_channel\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv414ledc_channel_t "Permalink to this definition")  

*Values:*

enumerator LEDC\_CHANNEL\_0 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_0E "Permalink to this definition")  

LEDC channel 0

enumerator LEDC\_CHANNEL\_1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_1E "Permalink to this definition")  

LEDC channel 1

enumerator LEDC\_CHANNEL\_2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_2E "Permalink to this definition")  

LEDC channel 2

enumerator LEDC\_CHANNEL\_3 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_3E "Permalink to this definition")  

LEDC channel 3

enumerator LEDC\_CHANNEL\_4 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_4E "Permalink to this definition")  

LEDC channel 4

enumerator LEDC\_CHANNEL\_5 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_5E "Permalink to this definition")  

LEDC channel 5

enumerator LEDC\_CHANNEL\_6 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_6E "Permalink to this definition")  

LEDC channel 6

enumerator LEDC\_CHANNEL\_7 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t14LEDC_CHANNEL_7E "Permalink to this definition")  

LEDC channel 7

enumerator LEDC\_CHANNEL\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N14ledc_channel_t16LEDC_CHANNEL_MAXE "Permalink to this definition")  

enum ledc\_timer\_bit\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv416ledc_timer_bit_t "Permalink to this definition")  

*Values:*

enumerator LEDC\_TIMER\_1\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_1_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 1 bits

enumerator LEDC\_TIMER\_2\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_2_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 2 bits

enumerator LEDC\_TIMER\_3\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_3_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 3 bits

enumerator LEDC\_TIMER\_4\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_4_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 4 bits

enumerator LEDC\_TIMER\_5\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_5_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 5 bits

enumerator LEDC\_TIMER\_6\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_6_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 6 bits

enumerator LEDC\_TIMER\_7\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_7_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 7 bits

enumerator LEDC\_TIMER\_8\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_8_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 8 bits

enumerator LEDC\_TIMER\_9\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t16LEDC_TIMER_9_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 9 bits

enumerator LEDC\_TIMER\_10\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_10_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 10 bits

enumerator LEDC\_TIMER\_11\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_11_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 11 bits

enumerator LEDC\_TIMER\_12\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_12_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 12 bits

enumerator LEDC\_TIMER\_13\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_13_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 13 bits

enumerator LEDC\_TIMER\_14\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_14_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 14 bits

enumerator LEDC\_TIMER\_15\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_15_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 15 bits

enumerator LEDC\_TIMER\_16\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_16_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 16 bits

enumerator LEDC\_TIMER\_17\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_17_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 17 bits

enumerator LEDC\_TIMER\_18\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_18_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 18 bits

enumerator LEDC\_TIMER\_19\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_19_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 19 bits

enumerator LEDC\_TIMER\_20\_BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t17LEDC_TIMER_20_BITE "Permalink to this definition")  

LEDC PWM duty resolution of 20 bits

enumerator LEDC\_TIMER\_BIT\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_timer_bit_t18LEDC_TIMER_BIT_MAXE "Permalink to this definition")  

enum ledc\_fade\_mode\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv416ledc_fade_mode_t "Permalink to this definition")  

*Values:*

enumerator LEDC\_FADE\_NO\_WAIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_fade_mode_t17LEDC_FADE_NO_WAITE "Permalink to this definition")  

LEDC fade function will return immediately

enumerator LEDC\_FADE\_WAIT\_DONE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_fade_mode_t19LEDC_FADE_WAIT_DONEE "Permalink to this definition")  

LEDC fade function will block until fading to the target duty

enumerator LEDC\_FADE\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N16ledc_fade_mode_t13LEDC_FADE_MAXE "Permalink to this definition")  

enum ledc\_channel\_etm\_task\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv428ledc_channel_etm_task_type_t "Permalink to this definition")  

LEDC channel related specific tasks that supported by the ETM module.

*Values:*

enumerator LEDC\_CHANNEL\_ETM\_TASK\_FADE\_SCALE\_UPDATE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_channel_etm_task_type_t39LEDC_CHANNEL_ETM_TASK_FADE_SCALE_UPDATEE "Permalink to this definition")  

Update newly configured scale for the fade on the channel

enumerator LEDC\_CHANNEL\_ETM\_TASK\_SIG\_OUT\_DIS [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_channel_etm_task_type_t33LEDC_CHANNEL_ETM_TASK_SIG_OUT_DISE "Permalink to this definition")  

Disable signal output on the channel

enumerator LEDC\_CHANNEL\_ETM\_TASK\_OVF\_CNT\_RST [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_channel_etm_task_type_t33LEDC_CHANNEL_ETM_TASK_OVF_CNT_RSTE "Permalink to this definition")  

Reset channel's timer overflow counter

enumerator LEDC\_CHANNEL\_ETM\_TASK\_FADE\_RESTART [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_channel_etm_task_type_t34LEDC_CHANNEL_ETM_TASK_FADE_RESTARTE "Permalink to this definition")  

enumerator LEDC\_CHANNEL\_ETM\_TASK\_FADE\_PAUSE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_channel_etm_task_type_t32LEDC_CHANNEL_ETM_TASK_FADE_PAUSEE "Permalink to this definition")  

Pause fading on the channel

enumerator LEDC\_CHANNEL\_ETM\_TASK\_FADE\_RESUME [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_channel_etm_task_type_t33LEDC_CHANNEL_ETM_TASK_FADE_RESUMEE "Permalink to this definition")  

Resume fading on the channel

enumerator LEDC\_CHANNEL\_ETM\_TASK\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_channel_etm_task_type_t25LEDC_CHANNEL_ETM_TASK_MAXE "Permalink to this definition")  

enum ledc\_timer\_etm\_task\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv426ledc_timer_etm_task_type_t "Permalink to this definition")  

LEDC timer related specific tasks that supported by the ETM module.

*Values:*

enumerator LEDC\_TIMER\_ETM\_TASK\_RST [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N26ledc_timer_etm_task_type_t23LEDC_TIMER_ETM_TASK_RSTE "Permalink to this definition")  

Reset the timer When timer is reset, a timer overflow equivalent signal is sent to the channel, so any newly configured parameter is able to get updated This is not the case on C6/H2/P4/C5/H4/H21, on such targets, no timer overflow equivalent signal is sent to the channel, i.e. newly configured parameter is not able to get updated when this ETM task is triggered

enumerator LEDC\_TIMER\_ETM\_TASK\_RESUME [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N26ledc_timer_etm_task_type_t26LEDC_TIMER_ETM_TASK_RESUMEE "Permalink to this definition")  

Resume the timer Note that the ETM timer pause/resume task must be used in pair Pause by ETM task and resume by calling `ledc_timer_resume` is not allowed, vice versa

enumerator LEDC\_TIMER\_ETM\_TASK\_PAUSE [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N26ledc_timer_etm_task_type_t25LEDC_TIMER_ETM_TASK_PAUSEE "Permalink to this definition")  

Pause the timer

enumerator LEDC\_TIMER\_ETM\_TASK\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N26ledc_timer_etm_task_type_t23LEDC_TIMER_ETM_TASK_MAXE "Permalink to this definition")  

enum ledc\_channel\_etm\_event\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv429ledc_channel_etm_event_type_t "Permalink to this definition")  

LEDC channel related specific events that supported by the ETM module.

*Values:*

enumerator LEDC\_CHANNEL\_ETM\_EVENT\_FADE\_END [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N29ledc_channel_etm_event_type_t31LEDC_CHANNEL_ETM_EVENT_FADE_ENDE "Permalink to this definition")  

Channel fading ended

enumerator LEDC\_CHANNEL\_ETM\_EVENT\_REACH\_MAX\_OVF\_CNT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N29ledc_channel_etm_event_type_t40LEDC_CHANNEL_ETM_EVENT_REACH_MAX_OVF_CNTE "Permalink to this definition")  

Channel has reached the maximum timer overflow count The maximum overflow count value can be set with `ledc_channel_configure_maximum_timer_ovf_cnt`

enumerator LEDC\_CHANNEL\_ETM\_EVENT\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N29ledc_channel_etm_event_type_t26LEDC_CHANNEL_ETM_EVENT_MAXE "Permalink to this definition")  

enum ledc\_timer\_etm\_event\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv427ledc_timer_etm_event_type_t "Permalink to this definition")  

LEDC timer related specific events that supported by the ETM module.

*Values:*

enumerator LEDC\_TIMER\_ETM\_EVENT\_OVF [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N27ledc_timer_etm_event_type_t24LEDC_TIMER_ETM_EVENT_OVFE "Permalink to this definition")  

Timer overflow happened

enumerator LEDC\_TIMER\_ETM\_EVENT\_MAX [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N27ledc_timer_etm_event_type_t24LEDC_TIMER_ETM_EVENT_MAXE "Permalink to this definition")  

### Header File

- [components/esp\_driver\_ledc/include/driver/ledc\_etm.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_ledc/include/driver/ledc_etm.h)
- This header file can be included with:
	> ```c
	> #include "driver/ledc_etm.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_ledc` component. To declare that your component depends on `esp_driver_ledc`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_ledc
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_ledc
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_timer\_new\_etm\_event( speed\_mode, timer\_sel, const \*config, [esp\_etm\_event\_handle\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/etm.html#_CPPv422esp_etm_event_handle_t "esp_etm_event_handle_t") \*out\_event) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv424ledc_timer_new_etm_event11ledc_mode_t12ledc_timer_tPK29ledc_timer_etm_event_config_tP22esp_etm_event_handle_t "Permalink to this definition")  

Get the ETM timer event for LEDC.

Note

The created ETM event object can be deleted later by calling `esp_etm_del_event`

Parameters:

- **speed\_mode** -- **\[in\]** Select the LEDC channel group with specified speed mode
- **timer\_sel** -- **\[in\]** LEDC timer index, select from ledc\_timer\_t
- **config** -- **\[in\]** LEDC timer ETM event configuration
- **out\_event** -- **\[out\]** Returned ETM event handle

Returns:

- ESP\_OK: Get ETM event successfully
- ESP\_ERR\_INVALID\_ARG: Get ETM event failed because of invalid argument
- ESP\_ERR\_NO\_MEM: Get ETM event failed because of no memory

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_channel\_new\_etm\_event( speed\_mode, channel, const \*config, [esp\_etm\_event\_handle\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/etm.html#_CPPv422esp_etm_event_handle_t "esp_etm_event_handle_t") \*out\_event) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv426ledc_channel_new_etm_event11ledc_mode_t14ledc_channel_tPK31ledc_channel_etm_event_config_tP22esp_etm_event_handle_t "Permalink to this definition")  

Get the ETM channel event for LEDC.

Note

The created ETM event object can be deleted later by calling `esp_etm_del_event`

Parameters:

- **speed\_mode** -- **\[in\]** Select the LEDC channel group with specified speed mode
- **channel** -- **\[in\]** LEDC channel index, select from ledc\_channel\_t
- **config** -- **\[in\]** LEDC channel ETM event configuration
- **out\_event** -- **\[out\]** Returned ETM event handle

Returns:

- ESP\_OK: Get ETM event successfully
- ESP\_ERR\_INVALID\_ARG: Get ETM event failed because of invalid argument
- ESP\_ERR\_NO\_MEM: Get ETM event failed because of no memory

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_timer\_new\_etm\_task( speed\_mode, timer\_sel, const \*config, [esp\_etm\_task\_handle\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/etm.html#_CPPv421esp_etm_task_handle_t "esp_etm_task_handle_t") \*out\_task) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv423ledc_timer_new_etm_task11ledc_mode_t12ledc_timer_tPK28ledc_timer_etm_task_config_tP21esp_etm_task_handle_t "Permalink to this definition")  

Get the ETM timer task for LEDC.

Note

The created ETM task object can be deleted later by calling `esp_etm_del_task`

Parameters:

- **speed\_mode** -- **\[in\]** Select the LEDC channel group with specified speed mode
- **timer\_sel** -- **\[in\]** LEDC timer index, select from ledc\_timer\_t
- **config** -- **\[in\]** LEDC timer ETM task configuration
- **out\_task** -- **\[out\]** Returned ETM task handle

Returns:

- ESP\_OK: Get ETM task successfully
- ESP\_ERR\_INVALID\_ARG: Get ETM task failed because of invalid argument
- ESP\_ERR\_NO\_MEM: Get ETM task failed because of no memory

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") ledc\_channel\_new\_etm\_task( speed\_mode, channel, const \*config, [esp\_etm\_task\_handle\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/etm.html#_CPPv421esp_etm_task_handle_t "esp_etm_task_handle_t") \*out\_task) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv425ledc_channel_new_etm_task11ledc_mode_t14ledc_channel_tPK30ledc_channel_etm_task_config_tP21esp_etm_task_handle_t "Permalink to this definition")  

Get the ETM channel task for LEDC.

Note

The created ETM task object can be deleted later by calling `esp_etm_del_task`

Parameters:

- **speed\_mode** -- **\[in\]** Select the LEDC channel group with specified speed mode
- **channel** -- **\[in\]** LEDC channel index, select from ledc\_channel\_t
- **config** -- **\[in\]** LEDC channel ETM task configuration
- **out\_task** -- **\[out\]** Returned ETM task handle

Returns:

- ESP\_OK: Get ETM task successfully
- ESP\_ERR\_INVALID\_ARG: Get ETM task failed because of invalid argument
- ESP\_ERR\_NO\_MEM: Get ETM task failed because of no memory

### Structures

struct ledc\_timer\_etm\_event\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv429ledc_timer_etm_event_config_t "Permalink to this definition")  

LEDC timer ETM event configuration.

Public Members

event\_type [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N29ledc_timer_etm_event_config_t10event_typeE "Permalink to this definition")  

LEDC timer ETM event type

struct ledc\_channel\_etm\_event\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv431ledc_channel_etm_event_config_t "Permalink to this definition")  

LEDC channel ETM event configuration.

Public Members

event\_type [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N31ledc_channel_etm_event_config_t10event_typeE "Permalink to this definition")  

LEDC channel ETM event type

struct ledc\_timer\_etm\_task\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv428ledc_timer_etm_task_config_t "Permalink to this definition")  

LEDC timer ETM task configuration.

Public Members

task\_type [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N28ledc_timer_etm_task_config_t9task_typeE "Permalink to this definition")  

LEDC timer ETM task type

struct ledc\_channel\_etm\_task\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv430ledc_channel_etm_task_config_t "Permalink to this definition")  

LEDC channel ETM task configuration.

Public Members

task\_type [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ledc.html#_CPPv4N30ledc_channel_etm_task_config_t9task_typeE "Permalink to this definition")  

LEDC channel ETM task type