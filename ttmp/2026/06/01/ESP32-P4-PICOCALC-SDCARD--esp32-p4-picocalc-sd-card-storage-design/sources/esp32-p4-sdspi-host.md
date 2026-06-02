---
Title: Source - esp32-p4-sdspi-host
Ticket: ESP32-P4-PICOCALC-SDCARD
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-SDCARD"
---

## SD SPI Host Driver

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-reference/peripherals/sdspi_host.html)

## Overview

The SD SPI host driver allows communication with one or more SD cards using the SPI Master driver, which utilizes the SPI host. Each card is accessed through an SD SPI device, represented by an SD SPI handle, which returns when the device is attached to an SPI bus by calling. It is important to note that the SPI bus should be initialized beforehand by [`spi_bus_initialize()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_initialize17spi_host_device_tPK16spi_bus_config_t14spi_dma_chan_t "spi_bus_initialize").

With the help of [SPI Master Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/spi_master.html) the SD SPI host driver based on, the SPI bus can be shared among SD cards and other SPI devices. The SPI Master driver will handle exclusive access from different tasks.

The SD SPI driver uses software-controlled CS signal.

## How to Use

Firstly, use the macro to initialize the structure, which is used to initialize an SD SPI device. This macro will also fill in the default pin mappings, which are the same as the pin mappings of the SDMMC host driver. Modify the host and pins of the structure to desired value. Then call `sdspi_host_init_device` to initialize the SD SPI device and attach to its bus.

Then use the macro to initialize the [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t") structure, which is used to store the state and configurations of the upper layer (SD/SDIO/MMC driver). Modify the `slot` parameter of the structure to the SD SPI device SD SPI handle just returned from `sdspi_host_init_device`. Call `sdmmc_card_init` with the [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t") to probe and initialize the SD card.

Now you can use SD/SDIO/MMC driver functions to access your card!

## Other Details

Only the following driver's API functions are normally used by most applications:

Other functions are mostly used by the protocol level SD/SDIO/MMC driver via function pointers in the [`sdmmc_host_t`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "sdmmc_host_t") structure. For more details, see [SD/SDIO/MMC Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html).

Note

SD over SPI does not support speeds above [`SDMMC_FREQ_DEFAULT`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_DEFAULT "SDMMC_FREQ_DEFAULT") due to the limitations of the SPI driver.

Warning

If you want to share the SPI bus among SD card and other SPI devices, there are some restrictions, see [Sharing the SPI Bus Among SD Cards and Other SPI Devices](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_share.html).

## API Reference

### Header File

- [components/esp\_driver\_sdspi/include/driver/sdspi\_host.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_sdspi/include/driver/sdspi_host.h)
- This header file can be included with:
	> ```c
	> #include "driver/sdspi_host.h"
	> ```
- This header file is a part of the API provided by the `esp_driver_sdspi` component. To declare that your component depends on `esp_driver_sdspi`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_driver_sdspi
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_driver_sdspi
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_init(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv415sdspi_host_initv "Permalink to this definition")  

Initialize SD SPI driver.

Note

This function is not thread safe

Returns:

- ESP\_OK on success
- other error codes may be returned in future versions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_init\_device(const \*dev\_config, \*out\_handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv422sdspi_host_init_devicePK21sdspi_device_config_tP18sdspi_dev_handle_t "Permalink to this definition")  

Attach and initialize an SD SPI device on the specific SPI bus.

Note

This function is not thread safe

Note

Initialize the SPI bus by `spi_bus_initialize()` before calling this function.

Note

The SDIO over sdspi needs an extra interrupt line. Call `gpio_install_isr_service()` before this function.

Parameters:

- **dev\_config** -- pointer to device configuration structure
- **out\_handle** -- Output of the handle to the sdspi device.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if sdspi\_host\_init\_device has invalid arguments
- ESP\_ERR\_NO\_MEM if memory can not be allocated
- other errors from the underlying spi\_master and gpio drivers

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_remove\_device( handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv424sdspi_host_remove_device18sdspi_dev_handle_t "Permalink to this definition")  

Remove an SD SPI device.

Parameters:

**handle** -- Handle of the SD SPI device

Returns:

Always ESP\_OK

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_do\_transaction( handle, [sdmmc\_command\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv415sdmmc_command_t "sdmmc_command_t") \*cmdinfo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv425sdspi_host_do_transaction18sdspi_dev_handle_tP15sdmmc_command_t "Permalink to this definition")  

Send command to the card and get response.

This function returns when command is sent and response is received, or data is transferred, or timeout occurs.

Note

This function is not thread safe w.r.t. init/deinit functions, and bus width/clock speed configuration functions. Multiple tasks can call sdspi\_host\_do\_transaction as long as other sdspi\_host\_\* functions are not called.

Parameters:

- **handle** -- Handle of the sdspi device
- **cmdinfo** -- pointer to structure describing command and data to transfer

Returns:

- ESP\_OK on success
- ESP\_ERR\_TIMEOUT if response or data transfer has timed out
- ESP\_ERR\_INVALID\_CRC if response or data transfer CRC check has failed
- ESP\_ERR\_INVALID\_RESPONSE if the card has sent an invalid response

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_set\_card\_clk( host, uint32\_t freq\_khz) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv423sdspi_host_set_card_clk18sdspi_dev_handle_t8uint32_t "Permalink to this definition")  

Set card clock frequency.

Currently only integer fractions of 40MHz clock can be used. For High Speed cards, 40MHz can be used. For Default Speed cards, 20MHz can be used.

Note

This function is not thread safe

Parameters:

- **host** -- Handle of the sdspi device
- **freq\_khz** -- card clock frequency, in kHz

Returns:

- ESP\_OK on success
- other error codes may be returned in the future

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_get\_real\_freq( handle, int \*real\_freq\_khz) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv424sdspi_host_get_real_freq18sdspi_dev_handle_tPi "Permalink to this definition")  

Calculate working frequency for specific device.

Parameters:

- **handle** -- SDSPI device handle
- **real\_freq\_khz** -- **\[out\]** output parameter to hold the calculated frequency (in kHz)

Returns:

- ESP\_ERR\_INVALID\_ARG: `handle` is NULL or invalid or `real_freq_khz` parameter is NULL
- ESP\_OK: Success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_deinit(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv417sdspi_host_deinitv "Permalink to this definition")  

Release resources allocated using sdspi\_host\_init.

Note

This function is not thread safe

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_STATE if sdspi\_host\_init function has not been called

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_io\_int\_enable( handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv424sdspi_host_io_int_enable18sdspi_dev_handle_t "Permalink to this definition")  

Enable SDIO interrupt.

Parameters:

**handle** -- Handle of the sdspi device

Returns:

- ESP\_OK on success

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdspi\_host\_io\_int\_wait( handle, uint32\_t timeout\_ticks) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv422sdspi_host_io_int_wait18sdspi_dev_handle_t8uint32_t "Permalink to this definition")  

Wait for SDIO interrupt until timeout.

Parameters:

- **handle** -- Handle of the sdspi device
- **timeout\_ticks** -- Ticks to wait before timeout.

Returns:

- ESP\_OK on success

bool sdspi\_host\_check\_buffer\_alignment(int slot, const void \*buf, size\_t size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv433sdspi_host_check_buffer_alignmentiPKv6size_t "Permalink to this definition")  

Check if the buffer meets the alignment requirements.

Parameters:

- **slot** -- **\[in\]** slot number (SDMMC\_HOST\_SLOT\_0 or SDMMC\_HOST\_SLOT\_1)
- **buf** -- **\[in\]** buffer pointer
- **size** -- **\[in\]** buffer size

Returns:

True for aligned buffer, false for not aligned buffer

### Structures

struct sdspi\_device\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv421sdspi_device_config_t "Permalink to this definition")  

Extra configuration for SD SPI device.

Public Members

[spi\_host\_device\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/spi_master.html#_CPPv417spi_host_device_t "spi_host_device_t") host\_id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t7host_idE "Permalink to this definition")  

SPI host to use, SPIx\_HOST (see spi\_types.h).

gpio\_num\_t gpio\_cs [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t7gpio_csE "Permalink to this definition")  

GPIO number of CS signal.

gpio\_num\_t gpio\_cd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t7gpio_cdE "Permalink to this definition")  

GPIO number of card detect signal.

gpio\_num\_t gpio\_wp [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t7gpio_wpE "Permalink to this definition")  

GPIO number of write protect signal.

gpio\_num\_t gpio\_int [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t8gpio_intE "Permalink to this definition")  

GPIO number of interrupt line (input) for SDIO card.

bool gpio\_wp\_polarity [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t16gpio_wp_polarityE "Permalink to this definition")  

GPIO write protect polarity 0 means "active low", i.e. card is protected when the GPIO is low; 1 means "active high", i.e. card is protected when GPIO is high.

uint16\_t duty\_cycle\_pos [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t14duty_cycle_posE "Permalink to this definition")  

Duty cycle of positive clock, in 1/256th increments (128 = 50%/50% duty). Setting this to 0 (=not setting it) is equivalent to setting this to 128.

int8\_t wait\_for\_miso [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv4N21sdspi_device_config_t13wait_for_misoE "Permalink to this definition")  

Timeout value in the driver will be waiting for MISO to be high before sending commands. Possible values are the following: 0: default value (40ms); -1: no waiting (0ms); 1-127: timeout in ms; else: invalid value, default will be used. This can be used to speed up transactions in certain scenarios but should not be needed if correct pull-up resistors are used. Use with care on devices where multiple SPI slaves use the same SPI bus.

### Macros

SDSPI\_DEFAULT\_HOST [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_DEFAULT_HOST "Permalink to this definition")  

SDSPI\_DEFAULT\_DMA [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_DEFAULT_DMA "Permalink to this definition")  

SDSPI\_HOST\_DEFAULT() [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_HOST_DEFAULT "Permalink to this definition")  

Default [sdmmc\_host\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#structsdmmc__host__t) structure initializer for SD over SPI driver.

Uses SPI mode and max frequency set to 20MHz

'slot' should be set to an sdspi device initialized by `sdspi_host_init_device()`.

SDSPI\_SLOT\_NO\_CS [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_SLOT_NO_CS "Permalink to this definition")  

indicates that card select line is not used

SDSPI\_SLOT\_NO\_CD [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_SLOT_NO_CD "Permalink to this definition")  

indicates that card detect line is not used

SDSPI\_SLOT\_NO\_WP [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_SLOT_NO_WP "Permalink to this definition")  

indicates that write protect line is not used

SDSPI\_SLOT\_NO\_INT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_SLOT_NO_INT "Permalink to this definition")  

indicates that interrupt line is not used

SDSPI\_IO\_ACTIVE\_LOW [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_IO_ACTIVE_LOW "Permalink to this definition")  

SDSPI\_DEVICE\_CONFIG\_DEFAULT() [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#c.SDSPI_DEVICE_CONFIG_DEFAULT "Permalink to this definition")  

Macro defining default configuration of SD SPI device.

### Type Definitions

typedef int sdspi\_dev\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv418sdspi_dev_handle_t "Permalink to this definition")  

Handle representing an SD SPI device.