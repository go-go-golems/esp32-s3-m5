---
Title: Source - esp32-p4-sdmmc-host
Ticket: ESP32-P4-PICOCALC-SDCARD
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-SDCARD"
---

## SDMMC Host Driver

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-reference/peripherals/sdmmc_host.html)

## Overview

ESP32-P4's SDMMC host peripheral has two slots. Each slot can be used independently to connect to an SD card, SDIO device, or eMMC chip.

- `SDMMC_HOST_SLOT_1` is routed via GPIO Matrix. This means that any GPIO may be used for each of the SD card signals. It is for non UHS-I usage.
- `SDMMC_HOST_SLOT_0` is dedicated to UHS-I mode.

On ESP32-P4, SDMMC host requires an external power supply for the IO voltage. Please refer to for details.

## Supported Speed Modes

SDMMC Host driver supports the following speed modes:

- Default Speed (20 MHz): 1-line or 4-line with SD cards, and 1-line, 4-line, or 8-line with 3.3 V eMMC
- High Speed (40 MHz): 1-line or 4-line with SD cards, and 1-line, 4-line, or 8-line with 3.3 V eMMC
- UHS-I 1.8 V, SDR104 (200 MHz): 4-line with SD cards
- UHS-I 1.8 V, SDR50 (100 MHz): 4-line with SD cards
- UHS-I 1.8 V, DDR50 (50 MHz): 4-line with SD cards
- High Speed DDR (40 MHz): 4-line with 3.3 V eMMC

Speed modes not supported at present:

- High Speed DDR mode: 8-line eMMC

## Using the SDMMC Host Driver

Of all the functions listed below, only the following ones will be used directly by most applications:

Other functions, such as the ones given below, will be called by the SD/MMC protocol layer via function pointers in the [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t") structure:

## Configuring Bus Width and Frequency

With the default initializers for [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t") and, i.e., `SDMMC_HOST_DEFAULT` and `SDMMC_SLOT_CONFIG_DEFAULT`, SDMMC Host driver will attempt to use the widest bus supported by the card (4 lines for SD, 8 lines for eMMC) and the frequency of 20 MHz.

In the designs where communication at 40 MHz frequency can be achieved, it is possible to increase the bus frequency by changing the `max_freq_khz` field of [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t"):

```cpp
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
```

If you need a specific frequency other than standard speeds, you are free to use any value from within an appropriate range of the SD interface given (SDMMC or SDSPI). However, the real clock frequency shall be calculated by the underlying driver and the value can be different from the one required.

For the SDMMC, `max_freq_khz` works as the upper limit so the final frequency value shall be always lower or equal. For the SDSPI, the nearest fitting frequency is supplied and thus the value can be greater than/equal to/lower than `max_freq_khz`.

To configure the bus width, set the `width` field of. For example, to set 1-line mode:

```cpp
sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
slot.width = 1;
```

## Configuring GPIOs

ESP32-P4 SDMMC Host can be configured to use arbitrary GPIOs for each of the signals. Configuration is performed by setting members of structure.

For example, to use GPIOs 1-6 for CLK, CMD, and D0-D3 signals respectively:

```c
sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
slot.clk = GPIO_NUM_1;
slot.cmd = GPIO_NUM_2;
slot.d0 = GPIO_NUM_3;
slot.d1 = GPIO_NUM_4;
slot.d2 = GPIO_NUM_5;
slot.d3 = GPIO_NUM_6;
```

It is also possible to configure Card Detect and Write Protect pins. Similar to other signals, set `cd` and `wp` members of the same structure:

```c
slot.cd = GPIO_NUM_7;
slot.wp = GPIO_NUM_8;
```

`SDMMC_SLOT_CONFIG_DEFAULT` sets both to `GPIO_NUM_NC`, meaning that by default the signals are not used.

Once structure is initialized this way, you can use it when calling or one of the higher level functions (such as [`esp_vfs_fat_sdmmc_mount()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/fatfs.html#_CPPv423esp_vfs_fat_sdmmc_mountPKcPK12sdmmc_host_tPKvPK26esp_vfs_fat_mount_config_tPP12sdmmc_card_t "esp_vfs_fat_sdmmc_mount")).

## Configuring Voltage Level

ESP32-P4 SDMMC Host requires the IO voltage to be supplied externally via the VDDPST\_5 (SD\_VREF) pin. If the design doesn't require the higher speed SD modes, this pin can be simply connected to the 3.3V supply.

If the design does require higher speed SD modes (which only work at 1.8V IO levels), there are two options available:

- Use the on-chip programmable LDO. In this case, connect the desired LDO output channel to VDDPST\_5 (SD\_VREF) pin. Call to initialize the SD power control driver, then set [`sdmmc_host_t::pwr_ctrl_handle`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t15pwr_ctrl_handleE "sdmmc_host_t::pwr_ctrl_handle") to the resulting handle.
- Use an external programmable LDO. Likewise, connect the LDO output to the VDDPST\_5 (SD\_VREF) pin. Then implement a custom sd\_pwr\_ctrl driver to control your LDO. Finally, assign [`sdmmc_host_t::pwr_ctrl_handle`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t15pwr_ctrl_handleE "sdmmc_host_t::pwr_ctrl_handle") to the handle of your driver instance.

## DDR Mode for eMMC Chips

By default, DDR mode will be used if:

- SDMMC host frequency is set to [`SDMMC_FREQ_HIGHSPEED`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_HIGHSPEED "SDMMC_FREQ_HIGHSPEED") in [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t") structure, and
- eMMC chip reports DDR mode support in its CSD register

DDR mode places higher requirements for signal integrity. To disable DDR mode while keeping the [`SDMMC_FREQ_HIGHSPEED`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_HIGHSPEED "SDMMC_FREQ_HIGHSPEED") frequency, clear the [`SDMMC_HOST_FLAG_DDR`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_DDR "SDMMC_HOST_FLAG_DDR") bit in [`sdmmc_host_t::flags`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t5flagsE "sdmmc_host_t::flags") field of the [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t"):

```c
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
host.flags &= ~SDMMC_HOST_FLAG_DDR;
```

## See also

- [SD/SDIO/MMC Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html): introduces the higher-level driver which implements the protocol layer.
- [SD SPI Host Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html): introduces a similar driver that uses the SPI controller and is limited to SD protocol's SPI mode.
- [SD Pull-up Requirements](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sd_pullup_requirements.html): introduces pull-up support and compatibility of modules and development kits.

## API Reference

### Header File

- [components/esp\_driver\_sdmmc/legacy/include/driver/sdmmc\_host.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_sdmmc/legacy/include/driver/sdmmc_host.h)
- This header file can be included with:
	> ```c
	> #include "driver/sdmmc_host.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_sdmmc` component. To declare that your component depends on `esp_driver_sdmmc`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_sdmmc
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_sdmmc
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_init(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv415sdmmc_host_initv "Permalink to this definition")  

Initialize SDMMC host peripheral.

Note

This function is not thread safe

Returns:

- ESP\_OK on success or if sdmmc\_host\_init was already initialized with this function
- ESP\_ERR\_NO\_MEM if memory can not be allocated

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_init\_slot(int slot, const \*slot\_config) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv420sdmmc_host_init_slotiPK19sdmmc_slot_config_t "Permalink to this definition")  

Initialize given slot of SDMMC peripheral.

On the ESP32, SDMMC peripheral has two slots:

- Slot 0: 8-bit wide, maps to HS1\_\* signals in PIN MUX
- Slot 1: 4-bit wide, maps to HS2\_\* signals in PIN MUX

Card detect and write protect signals can be routed to arbitrary GPIOs using GPIO matrix.

Note

This function is not thread safe

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **slot\_config** -- additional configuration for the slot

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_STATE if host has not been initialized using sdmmc\_host\_init
- ESP\_ERR\_INVALID\_ARG if GPIO pins from slot\_config are not valid

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_set\_bus\_width(int slot, size\_t width) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv424sdmmc_host_set_bus_widthi6size_t "Permalink to this definition")  

Select bus width to be used for data transfer.

SD/MMC card must be initialized prior to this command, and a command to set bus width has to be sent to the card (e.g. SD\_APP\_SET\_BUS\_WIDTH)

Note

This function is not thread safe

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **width** -- bus width (1, 4, or 8 for slot 0; 1 or 4 for slot 1)

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if slot number or width is not valid

size\_t sdmmc\_host\_get\_slot\_width(int slot) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv425sdmmc_host_get_slot_widthi "Permalink to this definition")  

Get bus width configured in `sdmmc_host_init_slot` to be used for data transfer.

Parameters:

**slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)

Returns:

configured bus width of the specified slot.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_set\_card\_clk(int slot, uint32\_t freq\_khz) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv423sdmmc_host_set_card_clki8uint32_t "Permalink to this definition")  

Set card clock frequency.

Currently only integer fractions of 40MHz clock can be used. For High Speed cards, 40MHz can be used. For Default Speed cards, 20MHz can be used.

Note

This function is not thread safe

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **freq\_khz** -- card clock frequency, in kHz

Returns:

- ESP\_OK on success
- other error codes may be returned in the future

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_set\_bus\_ddr\_mode(int slot, bool ddr\_enabled) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv427sdmmc_host_set_bus_ddr_modeib "Permalink to this definition")  

Enable or disable DDR mode of SD interface.

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **ddr\_enabled** -- enable or disable DDR mode

Returns:

- ESP\_OK on success
- ESP\_ERR\_NOT\_SUPPORTED if DDR mode is not supported on this slot

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_set\_cclk\_always\_on(int slot, bool cclk\_always\_on) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv429sdmmc_host_set_cclk_always_onib "Permalink to this definition")  

Enable or disable always-on card clock When cclk\_always\_on is false, the host controller is allowed to shut down the card clock between the commands. When cclk\_always\_on is true, the clock is generated even if no command is in progress.

Parameters:

- **slot** -- slot number
- **cclk\_always\_on** -- enable or disable always-on clock

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if the slot number is invalid

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_do\_transaction(int slot, [sdmmc\_command\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv415sdmmc_command_t "sdmmc_command_t") \*cmdinfo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv425sdmmc_host_do_transactioniP15sdmmc_command_t "Permalink to this definition")  

Send command to the card and get response.

This function returns when command is sent and response is received, or data is transferred, or timeout occurs.

**Attention**

Data buffer passed in cmdinfo->data must be in DMA capable memory and aligned to 4 byte boundary. If it's behind the cache, both cmdinfo->data and cmdinfo->buflen need to be aligned to cache line boundary.

Note

This function is not thread safe w.r.t. init/deinit functions, and bus width/clock speed configuration functions. Multiple tasks can call sdmmc\_host\_do\_transaction as long as other sdmmc\_host\_\* functions are not called.

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **cmdinfo** -- pointer to structure describing command and data to transfer

Returns:

- ESP\_OK on success
- ESP\_ERR\_TIMEOUT if response or data transfer has timed out
- ESP\_ERR\_INVALID\_CRC if response or data transfer CRC check has failed
- ESP\_ERR\_INVALID\_RESPONSE if the card has sent an invalid response
- ESP\_ERR\_INVALID\_SIZE if the size of data transfer is not valid in SD protocol
- ESP\_ERR\_INVALID\_ARG if the data buffer is not in DMA capable memory

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_io\_int\_enable(int slot) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv424sdmmc_host_io_int_enablei "Permalink to this definition")  

Enable IO interrupts.

This function configures the host to accept SDIO interrupts.

Parameters:

**slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)

Returns:

returns ESP\_OK, other errors possible in the future

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_io\_int\_wait(int slot, uint32\_t timeout\_ticks) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv422sdmmc_host_io_int_waiti8uint32_t "Permalink to this definition")  

Block until an SDIO interrupt is received, or timeout occurs.

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **timeout\_ticks** -- number of RTOS ticks to wait for the interrupt

Returns:

- ESP\_OK on success (interrupt received)
- ESP\_ERR\_TIMEOUT if the interrupt did not occur within timeout\_ticks

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_deinit\_slot(int slot) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv422sdmmc_host_deinit_sloti "Permalink to this definition")  

Disable SDMMC host and release allocated resources gracefully.

Note

If there are more than 1 active slots, this function will just decrease the reference count and won't actually disable the host until the last slot is disabled

Note

This function is not thread safe

Parameters:

**slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_STATE if SDMMC host has not been initialized
- ESP\_ERR\_INVALID\_ARG if invalid slot number is used

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_deinit(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv417sdmmc_host_deinitv "Permalink to this definition")  

Disable SDMMC host and release allocated resources forcefully.

Note

This function will deinitialize the host immediately, regardless of the number of active slots

Note

This function is not thread safe

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_STATE if SDMMC host has not been initialized

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_get\_real\_freq(int slot, int \*real\_freq\_khz) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv424sdmmc_host_get_real_freqiPi "Permalink to this definition")  

Provides a real frequency used for an SD card installed on specific slot of SD/MMC host controller.

This function calculates real working frequency given by current SD/MMC host controller setup for required slot: it reads associated host and card dividers from corresponding SDMMC registers, calculates respective frequency and stores the value into the 'real\_freq\_khz' parameter

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **real\_freq\_khz** -- **\[out\]** output parameter for the result frequency (in kHz)

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG on real\_freq\_khz == NULL or invalid slot number used

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_set\_input\_delay(int slot, sdmmc\_delay\_phase\_t delay\_phase) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv426sdmmc_host_set_input_delayi19sdmmc_delay_phase_t "Permalink to this definition")  

set input delay

- This API sets delay when the SDMMC Host samples the signal from the SD Slave.
- This API will check if the given `delay_phase` is valid or not.
- This API will print out the delay time, in picosecond (ps)

Note

ESP32 doesn't support this feature, you will get an `ESP_ERR_NOT_SUPPORTED`

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **delay\_phase** -- delay phase, this API will convert the phase into picoseconds and print it out

Returns:

- ESP\_OK: ON success.
- ESP\_ERR\_INVALID\_ARG: Invalid argument.
- ESP\_ERR\_NOT\_SUPPORTED: ESP32 doesn't support this feature.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_set\_input\_delayline(int slot, sdmmc\_delay\_line\_t delay\_line) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv430sdmmc_host_set_input_delaylinei18sdmmc_delay_line_t "Permalink to this definition")  

set input delayline

- This API sets delay when the SDMMC Host samples the signal from the SD Slave.
- This API will check if the given `delay_line` is valid or not.
- This API will print out the delay time, in picosecond (ps)

Parameters:

- **slot** -- slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **delay\_line** -- delay line, this API will convert the line into picoseconds and print it out

Returns:

- ESP\_OK: ON success.
- ESP\_ERR\_INVALID\_ARG: Invalid argument.
- ESP\_ERR\_NOT\_SUPPORTED: Some chips don't support this feature.

bool sdmmc\_host\_check\_buffer\_alignment(int slot, const void \*buf, size\_t size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv433sdmmc_host_check_buffer_alignmentiPKv6size_t "Permalink to this definition")  

Check if the buffer meets the alignment requirements.

Parameters:

- **slot** -- **\[in\]** slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **buf** -- **\[in\]** buffer pointer
- **size** -- **\[in\]** buffer size

Returns:

True for aligned buffer, false for not aligned buffer

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_is\_slot\_set\_to\_uhs1(int slot, bool \*is\_uhs1) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv430sdmmc_host_is_slot_set_to_uhs1iPb "Permalink to this definition")  

Check if the slot is set to uhs1 or not.

Parameters:

- **slot** -- **\[in\]** Slot id
- **is\_uhs1** -- **\[out\]** Is uhs1 or not

Returns:

- ESP\_OK: on success
- ESP\_ERR\_INVALID\_STATE: driver not in correct state

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_host\_get\_state( \*state) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv420sdmmc_host_get_stateP18sdmmc_host_state_t "Permalink to this definition")  

Get the state of SDMMC host.

Parameters:

**state** -- **\[out\]** output parameter for SDMMC host state structure

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG on invalid argument

### Structures

struct sdmmc\_slot\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv419sdmmc_slot_config_t "Permalink to this definition")  

Extra configuration for SDMMC peripheral slot

Public Members

gpio\_num\_t clk [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t3clkE "Permalink to this definition")  

GPIO number of CLK signal.

gpio\_num\_t cmd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t3cmdE "Permalink to this definition")  

GPIO number of CMD signal.

gpio\_num\_t d0 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d0E "Permalink to this definition")  

GPIO number of D0 signal.

gpio\_num\_t d1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d1E "Permalink to this definition")  

GPIO number of D1 signal.

gpio\_num\_t d2 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d2E "Permalink to this definition")  

GPIO number of D2 signal.

gpio\_num\_t d3 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d3E "Permalink to this definition")  

GPIO number of D3 signal.

gpio\_num\_t d4 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d4E "Permalink to this definition")  

GPIO number of D4 signal. Ignored in 1- or 4- line mode.

gpio\_num\_t d5 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d5E "Permalink to this definition")  

GPIO number of D5 signal. Ignored in 1- or 4- line mode.

gpio\_num\_t d6 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d6E "Permalink to this definition")  

GPIO number of D6 signal. Ignored in 1- or 4- line mode.

gpio\_num\_t d7 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2d7E "Permalink to this definition")  

GPIO number of D7 signal. Ignored in 1- or 4- line mode.

gpio\_num\_t gpio\_cd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t7gpio_cdE "Permalink to this definition")  

GPIO number of card detect signal.

gpio\_num\_t cd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2cdE "Permalink to this definition")  

GPIO number of card detect signal; shorter name.

gpio\_num\_t gpio\_wp [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t7gpio_wpE "Permalink to this definition")  

GPIO number of write protect signal.

gpio\_num\_t wp [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t2wpE "Permalink to this definition")  

GPIO number of write protect signal; shorter name.

uint8\_t width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t5widthE "Permalink to this definition")  

Bus width used by the slot (might be less than the max width supported)

uint32\_t flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N19sdmmc_slot_config_t5flagsE "Permalink to this definition")  

Features used by this slot.

struct sdmmc\_host\_state\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv418sdmmc_host_state_t "Permalink to this definition")  

SD/MMC host state structure

Public Members

bool host\_initialized [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N18sdmmc_host_state_t16host_initializedE "Permalink to this definition")  

Whether the host is initialized.

int num\_of\_init\_slots [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N18sdmmc_host_state_t17num_of_init_slotsE "Permalink to this definition")  

Number of initialized slots.

### Macros

SDMMC\_SLOT\_FLAG\_INTERNAL\_PULLUP [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#c.SDMMC_SLOT_FLAG_INTERNAL_PULLUP "Permalink to this definition")  

Enable internal pullups on enabled pins. The internal pullups are insufficient however, please make sure external pullups are connected on the bus. This is for debug / example purpose only.

SDMMC\_SLOT\_FLAG\_WP\_ACTIVE\_HIGH [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#c.SDMMC_SLOT_FLAG_WP_ACTIVE_HIGH "Permalink to this definition")  

GPIO write protect polarity. 0 means "active low", i.e. card is protected when the GPIO is low; 1 means "active high", i.e. card is protected when GPIO is high.

SDMMC\_SLOT\_FLAG\_UHS1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#c.SDMMC_SLOT_FLAG_UHS1 "Permalink to this definition")  

Enable UHS-I mode for this slot

### Header File

- [components/sdmmc/include/sd\_pwr\_ctrl.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/sdmmc/include/sd_pwr_ctrl.h)
- This header file can be included with:
	> ```c
	> #include "sd_pwr_ctrl.h"
	> ```
- This header file is a part of the API provided by the `sdmmc` component. To declare that your component depends on `sdmmc`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES sdmmc
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES sdmmc
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sd\_pwr\_ctrl\_set\_io\_voltage( handle, int voltage\_mv) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv426sd_pwr_ctrl_set_io_voltage20sd_pwr_ctrl_handle_ti "Permalink to this definition")  

Set SD IO voltage by a registered SD power control driver handle.

Parameters:

- **handle** -- **\[in\]** SD power control driver handle
- **voltage\_mv** -- **\[in\]** Voltage in mV

### Type Definitions

typedef struct sd\_pwr\_ctrl\_drv\_t \*sd\_pwr\_ctrl\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv420sd_pwr_ctrl_handle_t "Permalink to this definition")  

SD power control handle.

### Header File

- [components/sdmmc/include/sd\_pwr\_ctrl\_by\_on\_chip\_ldo.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/sdmmc/include/sd_pwr_ctrl_by_on_chip_ldo.h)
- This header file can be included with:
	> ```c
	> #include "sd_pwr_ctrl_by_on_chip_ldo.h"
	> ```
- This header file is a part of the API provided by the `sdmmc` component. To declare that your component depends on `sdmmc`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES sdmmc
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES sdmmc
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sd\_pwr\_ctrl\_new\_on\_chip\_ldo(const \*configs, \*ret\_drv) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv427sd_pwr_ctrl_new_on_chip_ldoPK24sd_pwr_ctrl_ldo_config_tP20sd_pwr_ctrl_handle_t "Permalink to this definition")  

New an SD power control driver via on-chip LDO.

Parameters:

- **configs** -- **\[in\]** On-chip LDO power control driver configurations
- **ret\_drv** -- **\[out\]** Created power control driver handle

Returns:

- ESP\_OK
- ESP\_ERR\_INVALID\_ARG Invalid arguments
- ESP\_ERR\_NO\_MEM Out of memory

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sd\_pwr\_ctrl\_del\_on\_chip\_ldo( ctrl\_handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv427sd_pwr_ctrl_del_on_chip_ldo20sd_pwr_ctrl_handle_t "Permalink to this definition")  

Delete a previously created on-chip LDO power control driver.

Parameters:

**ctrl\_handle** -- **\[in\]** Power control driver handle

Returns:

- ESP\_OK
- ESP\_ERR\_INVALID\_ARG Invalid arguments

### Structures

struct sd\_pwr\_ctrl\_ldo\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv424sd_pwr_ctrl_ldo_config_t "Permalink to this definition")  

LDO configurations.

Public Members

int ldo\_chan\_id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv4N24sd_pwr_ctrl_ldo_config_t11ldo_chan_idE "Permalink to this definition")  

On-chip LDO channel ID, e.g. set to `4` is the `LDO_VO4` is connected to power the SDMMC IO.