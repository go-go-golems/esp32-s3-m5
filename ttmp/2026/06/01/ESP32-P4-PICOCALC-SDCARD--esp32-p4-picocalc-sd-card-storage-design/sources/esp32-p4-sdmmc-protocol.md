---
Title: Source - esp32-p4-sdmmc-protocol
Ticket: ESP32-P4-PICOCALC-SDCARD
Status: active
Topics:
    - esp32-p4
    - picocalc
DocType: source
Intent: reference
Summary: "Downloaded reference material for ESP32-P4-PICOCALC-SDCARD"
---

## SD/SDIO/MMC Driver

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-reference/storage/sdmmc.html)

## Overview

The SD/SDIO/MMC driver supports SD memory, SDIO cards, and eMMC chips. This is a protocol layer driver ([sdmmc/include/sdmmc\_cmd.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/sdmmc/include/sdmmc_cmd.h)) which can work together with:

- SDMMC host driver ([esp\_driver\_sdmmc/legacy/include/driver/sdmmc\_host.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_sdmmc/legacy/include/driver/sdmmc_host.h)), see [SDMMC Host API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html) for more details.
- SDSPI host driver ([esp\_driver\_sdspi/include/driver/sdspi\_host.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_sdspi/include/driver/sdspi_host.h)), see [SD SPI Host API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html) for more details.

### Protocol Layer vs Host Layer

The SDMMC protocol layer described in this document handles the specifics of the SD protocol, such as the card initialization flow and various data transfer command flows. The protocol layer works with the host via the structure. This structure contains pointers to various functions of the host.

Host layer driver(s) implement the protocol layer driver by supporting these functions:

- Sending commands to slave devices
- Sending and receiving data
- Handling error conditions within the bus
![](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/_images/blockdiag-71e55707da6ab61c6c99f49fb3b0e441cb91c5e8.png)

SD Host Side Component Architecture 

## Application Examples

- [storage/sd\_card/sdmmc](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/storage/sd_card/sdmmc) demonstrates how to operate an SD card formatted with the FatFS file system via the SDMMC interface.
- [storage/emmc](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/storage/emmc) demonstrates how to operate an eMMC chip formatted with the FatFS file system via the SDMMC interface.
- [storage/sd\_card/sdspi](https://github.com/espressif/esp-idf/tree/v6.0.1/examples/storage/sd_card/sdspi) demonstrates how to operate an SD card formatted with the FatFS file system via the SPI interface.

## Protocol Layer API

The protocol layer is given the structure. This structure describes the SD/MMC host driver, lists its capabilities, and provides pointers to functions for the implementation driver. The protocol layer stores card-specific information in the structure. When sending commands to the SD/MMC host driver, the protocol layer uses the structure to describe the command, arguments, expected return values, and data to transfer if there is any.

### Using API with SD Memory Cards

- To initialize the SDMMC host, call the host driver functions, e.g., [`sdmmc_host_init()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv415sdmmc_host_initv "sdmmc_host_init"), [`sdmmc_host_init_slot()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv420sdmmc_host_init_slotiPK19sdmmc_slot_config_t "sdmmc_host_init_slot").¸
- To initialize the SDSPI host, call the host driver functions, e.g., [`sdspi_host_init()`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdspi_host.html#_CPPv415sdspi_host_initv "sdspi_host_init"), `sdspi_host_init_slot()`.
- To initialize the card, call and pass to it the parameters `host` - the host driver information, and `card` - a pointer to the structure which will be filled with information about the card when the function completes.
- To read and write sectors of the card, use and respectively and pass to it the parameter `card` - a pointer to the card information structure.
- If the card is not used anymore, call the host driver function to disable the host peripheral and free the resources allocated by the driver (`sdmmc_host_deinit` for SDMMC or `sdspi_host_deinit` for SDSPI).

### Unaligned Buffer Performance

When buffers passed to or are not DMA-capable (e.g., allocated in PSRAM), the driver copies data through a temporary DMA-capable buffer. By default, this is done one block at a time using single-block transfer commands.

To improve throughput in this scenario, set the field to a value greater than 1. This enables multi-block transfer commands (CMD18/CMD25), which can significantly reduce transfer overhead. The trade-off is higher heap usage (buffer size = N × block size, where N is the configured value and the block size is typically 512 bytes). When this field is 0 (default), the driver falls back to single-block transfers (equivalent to 1). Values greater than 1 are recommended to be a multiple of the block size (e.g., 2, 4, 8, 16 or 32) for best performance.

Note

Keep this value at 0 or 1 if your card or configuration does not support multi-block read/write commands (CMD18 and CMD25).

Alternatively, a pre-allocated DMA-capable buffer can be provided via the field. This avoids per-transfer heap allocations and allows the driver to reuse the same buffer across transfers. The buffer must be at least one sector in size (typically 512 bytes) and should ideally be a multiple of the sector size.

### Using API with eMMC Chips

From the protocol layer's perspective, eMMC memory chips behave exactly like SD memory cards. Even though eMMCs are chips and do not have a card form factor, the terminology for SD cards can still be applied to eMMC due to the similarity of the protocol (sdmmc\_card\_t, sdmmc\_card\_init). Note that eMMC chips cannot be used over SPI, which makes them incompatible with the SD SPI host driver.

To initialize eMMC memory and perform read/write operations, follow the steps listed for SD cards in the previous section.

### Using API with SDIO Cards

Initialization and the probing process are the same as with SD memory cards. The only difference is in data transfer commands in SDIO mode.

During the card initialization and probing, performed with, the driver only configures the following registers of the IO card:

1. The IO portion of the card is reset by setting RES bit in the I/O Abort (0x06) register.
2. If 4-line mode is enabled in host and slot configuration, the driver attempts to set the Bus width field in the Bus Interface Control (0x07) register. If setting the filed is successful, which means that the slave supports 4-line mode, the host is also switched to 4-line mode.
3. If high-speed mode is enabled in the host configuration, the SHS bit is set in the High Speed (0x13) register.

In particular, the driver does not set any bits in (1) I/O Enable and Int Enable registers, (2) I/O block sizes, etc. Applications can set them by calling.

For card configuration and data transfer, choose the pair of functions relevant to your case from the table below.

| Action | Read Function | Write Function |
| --- | --- | --- |
| Read and write a single byte using IO\_RW\_DIRECT (CMD52) |  |  |
| Read and write multiple bytes using IO\_RW\_EXTENDED (CMD53) in byte mode |  |  |
| Read and write blocks of data using IO\_RW\_EXTENDED (CMD53) in block mode |  |  |

SDIO interrupts can be enabled by the application using the function. When using SDIO in 1-line mode, the D1 line also needs to be connected to use SDIO interrupts.

If you want the application to wait until the SDIO interrupt occurs, use.

### Combo (Memory + IO) Cards

The driver does not support SD combo cards. Combo cards are treated as IO cards.

### Thread Safety

Most applications need to use the protocol layer only in one task. For this reason, the protocol layer does not implement any kind of locking on the structure, or when accessing SDMMC or SD SPI host drivers. Such locking is usually implemented on a higher layer, e.g., in the filesystem driver.

## API Reference

### Header File

- [components/sdmmc/include/sdmmc\_cmd.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/sdmmc/include/sdmmc_cmd.h)
- This header file can be included with:
	> ```c
	> #include "sdmmc_cmd.h"
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

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_card\_init(const \*host, \*out\_card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv415sdmmc_card_initPK12sdmmc_host_tP12sdmmc_card_t "Permalink to this definition")  

Probe and initialize SD/MMC card using given host

Parameters:

- **host** -- pointer to structure defining host controller
- **out\_card** -- pointer to structure which will receive information about the card when the function completes

Returns:

- ESP\_OK on success
- One of the error codes from SDMMC host controller

void sdmmc\_card\_print\_info(FILE \*stream, const \*card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv421sdmmc_card_print_infoP4FILEPK12sdmmc_card_t "Permalink to this definition")  

Print information about the card to a stream.

Parameters:

- **stream** -- stream obtained using fopen or fdopen
- **card** -- card information structure initialized using sdmmc\_card\_init

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_get\_status( \*card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv416sdmmc_get_statusP12sdmmc_card_t "Permalink to this definition")  

Get status of SD/MMC card

Parameters:

**card** -- pointer to card information structure previously initialized using sdmmc\_card\_init

Returns:

- ESP\_OK on success
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_write\_sectors( \*card, const void \*src, size\_t start\_sector, size\_t sector\_count) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv419sdmmc_write_sectorsP12sdmmc_card_tPKv6size_t6size_t "Permalink to this definition")  

Write given number of sectors to SD/MMC card

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **src** -- pointer to data buffer to read data from; data size must be equal to sector\_count \* card->csd.sector\_size
- **start\_sector** -- sector where to start writing
- **sector\_count** -- number of sectors to write

Returns:

- ESP\_OK on success or sector\_count equal to 0
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_read\_sectors( \*card, void \*dst, size\_t start\_sector, size\_t sector\_count) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv418sdmmc_read_sectorsP12sdmmc_card_tPv6size_t6size_t "Permalink to this definition")  

Read given number of sectors from the SD/MMC card

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **dst** -- pointer to data buffer to write into; buffer size must be at least sector\_count \* card->csd.sector\_size
- **start\_sector** -- sector where to start reading
- **sector\_count** -- number of sectors to read

Returns:

- ESP\_OK on success or sector\_count equal to 0
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_erase\_sectors( \*card, size\_t start\_sector, size\_t sector\_count, arg) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv419sdmmc_erase_sectorsP12sdmmc_card_t6size_t6size_t17sdmmc_erase_arg_t "Permalink to this definition")  

Erase given number of sectors from the SD/MMC card

Note

When sdmmc\_erase\_sectors used with cards in SDSPI mode, it was observed that card requires re-init after erase operation.

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **start\_sector** -- sector where to start erase
- **sector\_count** -- number of sectors to erase
- **arg** -- erase command (CMD38) argument

Returns:

- ESP\_OK on success or sector\_count equal to 0
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_can\_discard( \*card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv417sdmmc_can_discardP12sdmmc_card_t "Permalink to this definition")  

Check if SD/MMC card supports discard

Parameters:

**card** -- pointer to card information structure previously initialized using sdmmc\_card\_init

Returns:

- ESP\_OK if supported by the card/device
- ESP\_FAIL if not supported by the card/device

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_can\_trim( \*card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv414sdmmc_can_trimP12sdmmc_card_t "Permalink to this definition")  

Check if SD/MMC card supports trim

Parameters:

**card** -- pointer to card information structure previously initialized using sdmmc\_card\_init

Returns:

- ESP\_OK if supported by the card/device
- ESP\_FAIL if not supported by the card/device

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_mmc\_can\_sanitize( \*card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv422sdmmc_mmc_can_sanitizeP12sdmmc_card_t "Permalink to this definition")  

Check if SD/MMC card supports sanitize

Parameters:

**card** -- pointer to card information structure previously initialized using sdmmc\_card\_init

Returns:

- ESP\_OK if supported by the card/device
- ESP\_FAIL if not supported by the card/device

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_mmc\_sanitize( \*card, uint32\_t timeout\_ms) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv418sdmmc_mmc_sanitizeP12sdmmc_card_t8uint32_t "Permalink to this definition")  

Sanitize the data that was unmapped by a Discard command

Note

Discard command has to precede sanitize operation. To discard, use MMC\_DICARD\_ARG with sdmmc\_erase\_sectors argument

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **timeout\_ms** -- timeout value in milliseconds required to sanitize the selected range of sectors.

Returns:

- ESP\_OK on success
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_full\_erase( \*card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv416sdmmc_full_eraseP12sdmmc_card_t "Permalink to this definition")  

Erase complete SD/MMC card

Parameters:

**card** -- pointer to card information structure previously initialized using sdmmc\_card\_init

Returns:

- ESP\_OK on success
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_read\_byte( \*card, uint32\_t function, uint32\_t reg, uint8\_t \*out\_byte) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv418sdmmc_io_read_byteP12sdmmc_card_t8uint32_t8uint32_tP7uint8_t "Permalink to this definition")  

Read one byte from an SDIO card using IO\_RW\_DIRECT (CMD52)

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **function** -- IO function number
- **reg** -- byte address within IO function
- **out\_byte** -- **\[out\]** output, receives the value read from the card

Returns:

- ESP\_OK on success
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_write\_byte( \*card, uint32\_t function, uint32\_t reg, uint8\_t in\_byte, uint8\_t \*out\_byte) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv419sdmmc_io_write_byteP12sdmmc_card_t8uint32_t8uint32_t7uint8_tP7uint8_t "Permalink to this definition")  

Write one byte to an SDIO card using IO\_RW\_DIRECT (CMD52)

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **function** -- IO function number
- **reg** -- byte address within IO function
- **in\_byte** -- value to be written
- **out\_byte** -- **\[out\]** if not NULL, receives new byte value read from the card (read-after-write).

Returns:

- ESP\_OK on success
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_read\_bytes( \*card, uint32\_t function, uint32\_t addr, void \*dst, size\_t size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv419sdmmc_io_read_bytesP12sdmmc_card_t8uint32_t8uint32_tPv6size_t "Permalink to this definition")  

Read multiple bytes from an SDIO card using IO\_RW\_EXTENDED (CMD53)

This function performs read operation using CMD53 in byte mode. For block mode, see sdmmc\_io\_read\_blocks.

By default OP Code is set (incrementing address). To send CMD53 without this bit, OR the argument `addr` with `SDMMC_IO_FIXED_ADDR`.

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **function** -- IO function number
- **addr** -- byte address within IO function where reading starts
- **dst** -- buffer which receives the data read from card. Aligned to 4 byte boundary unless `SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF` flag is set when calling `sdmmc_card_init`. The flag is mandatory when the buffer is behind the cache.
- **size** -- number of bytes to read, 1 to 512.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_SIZE if size exceeds 512 bytes
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_write\_bytes( \*card, uint32\_t function, uint32\_t addr, const void \*src, size\_t size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv420sdmmc_io_write_bytesP12sdmmc_card_t8uint32_t8uint32_tPKv6size_t "Permalink to this definition")  

Write multiple bytes to an SDIO card using IO\_RW\_EXTENDED (CMD53)

This function performs write operation using CMD53 in byte mode. For block mode, see sdmmc\_io\_write\_blocks.

By default OP Code is set (incrementing address). To send CMD53 without this bit, OR the argument `addr` with `SDMMC_IO_FIXED_ADDR`.

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **function** -- IO function number
- **addr** -- byte address within IO function where writing starts
- **src** -- data to be written. Aligned to 4 byte boundary unless `SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF` flag is set when calling `sdmmc_card_init`. The flag is mandatory when the buffer is behind the cache.
- **size** -- number of bytes to write, 1 to 512.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_SIZE if size exceeds 512 bytes
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_read\_blocks( \*card, uint32\_t function, uint32\_t addr, void \*dst, size\_t size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv420sdmmc_io_read_blocksP12sdmmc_card_t8uint32_t8uint32_tPv6size_t "Permalink to this definition")  

Read blocks of data from an SDIO card using IO\_RW\_EXTENDED (CMD53)

This function performs read operation using CMD53 in block mode. For byte mode, see sdmmc\_io\_read\_bytes.

By default OP Code is set (incrementing address). To send CMD53 without this bit, OR the argument `addr` with `SDMMC_IO_FIXED_ADDR`.

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **function** -- IO function number
- **addr** -- byte address within IO function where writing starts
- **dst** -- buffer which receives the data read from card. Aligned to 4 byte boundary, and also cache line size if the buffer is behind the cache.
- **size** -- number of bytes to read, must be divisible by the card block size.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_SIZE if size is not divisible by 512 bytes
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_write\_blocks( \*card, uint32\_t function, uint32\_t addr, const void \*src, size\_t size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv421sdmmc_io_write_blocksP12sdmmc_card_t8uint32_t8uint32_tPKv6size_t "Permalink to this definition")  

Write blocks of data to an SDIO card using IO\_RW\_EXTENDED (CMD53)

This function performs write operation using CMD53 in block mode. For byte mode, see sdmmc\_io\_write\_bytes.

By default OP Code is set (incrementing address). To send CMD53 without this bit, OR the argument `addr` with `SDMMC_IO_FIXED_ADDR`.

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **function** -- IO function number
- **addr** -- byte address within IO function where writing starts
- **src** -- data to be written. Aligned to 4 byte boundary, and also cache line size if the buffer is behind the cache.
- **size** -- number of bytes to write, must be divisible by the card block size.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_SIZE if size is not divisible by 512 bytes
- One of the error codes from SDMMC host controller

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_enable\_int( \*card) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv419sdmmc_io_enable_intP12sdmmc_card_t "Permalink to this definition")  

Enable SDIO interrupt in the SDMMC host

Parameters:

**card** -- pointer to card information structure previously initialized using sdmmc\_card\_init

Returns:

- ESP\_OK on success
- ESP\_ERR\_NOT\_SUPPORTED if the host controller does not support IO interrupts

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_wait\_int( \*card, uint32\_t timeout\_ticks) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv417sdmmc_io_wait_intP12sdmmc_card_t8uint32_t "Permalink to this definition")  

Block until an SDIO interrupt is received

Slave uses D1 line to signal interrupt condition to the host. This function can be used to wait for the interrupt.

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **timeout\_ticks** -- time to wait for the interrupt, in RTOS ticks

Returns:

- ESP\_OK if the interrupt is received
- ESP\_ERR\_NOT\_SUPPORTED if the host controller does not support IO interrupts
- ESP\_ERR\_TIMEOUT if the interrupt does not happen in timeout\_ticks

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_get\_cis\_data( \*card, uint8\_t \*out\_buffer, size\_t buffer\_size, size\_t \*inout\_cis\_size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv421sdmmc_io_get_cis_dataP12sdmmc_card_tP7uint8_t6size_tP6size_t "Permalink to this definition")  

Get the data of CIS region of an SDIO card.

You may provide a buffer not sufficient to store all the CIS data. In this case, this function stores as much data into your buffer as possible. Also, this function will try to get and return the size required for you.

Parameters:

- **card** -- pointer to card information structure previously initialized using sdmmc\_card\_init
- **out\_buffer** -- Output buffer of the CIS data
- **buffer\_size** -- Size of the buffer.
- **inout\_cis\_size** -- Mandatory, pointer to a size, input and output.
	- input: Limitation of maximum searching range, should be 0 or larger than buffer\_size. The function searches for CIS\_CODE\_END until this range. Set to 0 to search infinitely.
		- output: The size required to store all the CIS data, if CIS\_CODE\_END is found.

Returns:

- ESP\_OK: on success
- ESP\_ERR\_INVALID\_RESPONSE: if the card does not (correctly) support CIS.
- ESP\_ERR\_INVALID\_SIZE: CIS\_CODE\_END found, but buffer\_size is less than required size, which is stored in the inout\_cis\_size then.
- ESP\_ERR\_NOT\_FOUND: if the CIS\_CODE\_END not found. Increase input value of inout\_cis\_size or set it to 0, if you still want to search for the end; output value of inout\_cis\_size is invalid in this case.
- and other error code return from sdmmc\_io\_read\_bytes

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_io\_print\_cis\_info(uint8\_t \*buffer, size\_t buffer\_size, FILE \*fp) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv423sdmmc_io_print_cis_infoP7uint8_t6size_tP4FILE "Permalink to this definition")  

Parse and print the CIS information of an SDIO card.

Note

Not all the CIS codes and all kinds of tuples are supported. If you see some unresolved code, you can add the parsing of these code in sdmmc\_io.c and contribute to the IDF through the Github repository.

```cpp
using sdmmc_card_init
```

Parameters:

- **buffer** -- Buffer to parse
- **buffer\_size** -- Size of the buffer.
- **fp** -- File pointer to print to, set to NULL to print to stdout.

Returns:

- ESP\_OK: on success
- ESP\_ERR\_NOT\_SUPPORTED: if the value from the card is not supported to be parsed.
- ESP\_ERR\_INVALID\_SIZE: if the CIS size fields are not correct.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") sdmmc\_get\_blockdev( \*card, esp\_blockdev\_handle\_t \*out\_handle) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv418sdmmc_get_blockdevP12sdmmc_card_tP21esp_blockdev_handle_t "Permalink to this definition")  

Get a block device handle for the SD/MMC card

This function allocates a block device handle and initializes it with the card information.

Parameters:

- **card** -- **\[in\]** Pointer to card information structure previously initialized using sdmmc\_card\_init.
- **out\_handle** -- **\[inout\]** Pointer to variable which will receive the block device handle.

Returns:

- ESP\_OK on success
- ESP\_ERR\_INVALID\_ARG if card or out\_handle is NULL
- ESP\_ERR\_NO\_MEM if memory allocation fails

### Macros

SDMMC\_IO\_FIXED\_ADDR [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_IO_FIXED_ADDR "Permalink to this definition")  

Call `sdmmc_io_read_bytes`, `sdmmc_io_write_bytes`, `sdmmc_io_read_blocks` or `sdmmc_io_write_bocks` APIs with address ORed by this flag to send CMD53 with OP Code clear (fixed address)

### Header File

- [components/sdmmc/include/sd\_protocol\_types.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/sdmmc/include/sd_protocol_types.h)
- This header file can be included with:
	> ```c
	> #include "sd_protocol_types.h"
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

### Structures

struct sdmmc\_csd\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv411sdmmc_csd_t "Permalink to this definition")  

Decoded values from SD card Card Specific Data register

Public Members

int csd\_ver [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_csd_t7csd_verE "Permalink to this definition")  

CSD structure format

int mmc\_ver [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_csd_t7mmc_verE "Permalink to this definition")  

MMC version (for CID format)

int capacity [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_csd_t8capacityE "Permalink to this definition")  

total number of sectors

int sector\_size [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_csd_t11sector_sizeE "Permalink to this definition")  

sector size in bytes

int read\_block\_len [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_csd_t14read_block_lenE "Permalink to this definition")  

block length for reads

int card\_command\_class [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_csd_t18card_command_classE "Permalink to this definition")  

Card Command Class for SD

int tr\_speed [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_csd_t8tr_speedE "Permalink to this definition")  

Max transfer speed

struct sdmmc\_cid\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv411sdmmc_cid_t "Permalink to this definition")  

Decoded values from SD card Card IDentification register

Public Members

int mfg\_id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_cid_t6mfg_idE "Permalink to this definition")  

manufacturer identification number

int oem\_id [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_cid_t6oem_idE "Permalink to this definition")  

OEM/product identification number

char name\[8\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_cid_t4nameE "Permalink to this definition")  

product name (MMC v1 has the longest)

int revision [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_cid_t8revisionE "Permalink to this definition")  

product revision

int serial [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_cid_t6serialE "Permalink to this definition")  

product serial number

int date [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_cid_t4dateE "Permalink to this definition")  

manufacturing date

struct sdmmc\_scr\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv411sdmmc_scr_t "Permalink to this definition")  

Decoded values from SD Configuration Register Note: When new member is added, update reserved bits accordingly

Public Members

uint32\_t sd\_spec [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_scr_t7sd_specE "Permalink to this definition")  

SD Physical layer specification version, reported by card

uint32\_t erase\_mem\_state [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_scr_t15erase_mem_stateE "Permalink to this definition")  

data state on card after erase whether 0 or 1 (card vendor dependent)

uint32\_t bus\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_scr_t9bus_widthE "Permalink to this definition")  

bus widths supported by card: BIT(0) — 1-bit bus, BIT(2) — 4-bit bus

uint32\_t reserved [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_scr_t8reservedE "Permalink to this definition")  

reserved for future expansion

uint32\_t rsvd\_mnf [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_scr_t8rsvd_mnfE "Permalink to this definition")  

reserved for manufacturer usage

struct sdmmc\_ssr\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv411sdmmc_ssr_t "Permalink to this definition")  

Decoded values from SD Status Register Note: When new member is added, update reserved bits accordingly

Public Members

uint32\_t alloc\_unit\_kb [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t13alloc_unit_kbE "Permalink to this definition")  

Allocation unit of the card, in multiples of kB (1024 bytes)

uint32\_t erase\_size\_au [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t13erase_size_auE "Permalink to this definition")  

Erase size for the purpose of timeout calculation, in multiples of allocation unit

uint32\_t cur\_bus\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t13cur_bus_widthE "Permalink to this definition")  

SD current bus width

uint32\_t discard\_support [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t15discard_supportE "Permalink to this definition")  

SD discard feature support

uint32\_t fule\_support [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t12fule_supportE "Permalink to this definition")  

SD FILE (Full User Area Logical Erase) feature support

uint32\_t erase\_timeout [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t13erase_timeoutE "Permalink to this definition")  

Timeout (in seconds) for erase of a single allocation unit

uint32\_t erase\_offset [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t12erase_offsetE "Permalink to this definition")  

Constant timeout offset (in seconds) for any erase operation

uint32\_t reserved [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N11sdmmc_ssr_t8reservedE "Permalink to this definition")  

reserved for future expansion

struct sdmmc\_ext\_csd\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv415sdmmc_ext_csd_t "Permalink to this definition")  

Decoded values of Extended Card Specific Data

Public Members

uint8\_t rev [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_ext_csd_t3revE "Permalink to this definition")  

Extended CSD Revision

uint8\_t power\_class [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_ext_csd_t11power_classE "Permalink to this definition")  

Power class used by the card

uint8\_t erase\_mem\_state [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_ext_csd_t15erase_mem_stateE "Permalink to this definition")  

data state on card after erase whether 0 or 1 (card vendor dependent)

uint8\_t sec\_feature [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_ext_csd_t11sec_featureE "Permalink to this definition")  

secure data management features supported by the card

struct sdmmc\_switch\_func\_rsp\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv423sdmmc_switch_func_rsp_t "Permalink to this definition")  

SD SWITCH\_FUNC response buffer

Public Members

uint32\_t data\[512 / 8 / sizeof(uint32\_t)\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N23sdmmc_switch_func_rsp_t4dataE "Permalink to this definition")  

response data

struct sdmmc\_command\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv415sdmmc_command_t "Permalink to this definition")  

SD/MMC command information

Public Members

uint32\_t opcode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t6opcodeE "Permalink to this definition")  

SD or MMC command index

uint32\_t arg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t3argE "Permalink to this definition")  

SD/MMC command argument

response [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t8responseE "Permalink to this definition")  

response buffer

void \*data [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t4dataE "Permalink to this definition")  

buffer to send or read into

size\_t datalen [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t7datalenE "Permalink to this definition")  

length of data in the buffer

size\_t buflen [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t6buflenE "Permalink to this definition")  

length of the buffer

size\_t blklen [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t6blklenE "Permalink to this definition")  

block length

int flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t5flagsE "Permalink to this definition")  

see below

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") error [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t5errorE "Permalink to this definition")  

error returned from transfer

uint32\_t timeout\_ms [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t10timeout_msE "Permalink to this definition")  

response timeout, in milliseconds

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*volt\_switch\_cb)(void\*, int) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t14volt_switch_cbE "Permalink to this definition")  

callback to be called during CMD11 to switch voltage

void \*volt\_switch\_cb\_arg [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N15sdmmc_command_t18volt_switch_cb_argE "Permalink to this definition")  

argument to be passed to the CMD11 callback

struct sdmmc\_host\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_host_t "Permalink to this definition")  

SD/MMC Host description

This structure defines properties of SD/MMC host and functions of SD/MMC host which can be used by upper layers.

Public Members

uint32\_t flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t5flagsE "Permalink to this definition")  

flags defining host properties

int slot [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t4slotE "Permalink to this definition")  

slot number, to be passed to host functions

int max\_freq\_khz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t12max_freq_khzE "Permalink to this definition")  

max frequency supported by the host

float io\_voltage [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t10io_voltageE "Permalink to this definition")  

I/O voltage used by the controller (voltage switching is not supported)

driver\_strength [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t15driver_strengthE "Permalink to this definition")  

Driver Strength

current\_limit [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t13current_limitE "Permalink to this definition")  

Current Limit

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*init)(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t4initE "Permalink to this definition")  

Host function to initialize the driver

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*set\_bus\_width)(int slot, size\_t width) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t13set_bus_widthE "Permalink to this definition")  

host function to set bus width

size\_t (\*get\_bus\_width)(int slot) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t13get_bus_widthE "Permalink to this definition")  

host function to get bus width

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*set\_bus\_ddr\_mode)(int slot, bool ddr\_enable) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t16set_bus_ddr_modeE "Permalink to this definition")  

host function to set DDR mode

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*set\_card\_clk)(int slot, uint32\_t freq\_khz) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t12set_card_clkE "Permalink to this definition")  

host function to set card clock frequency

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*set\_cclk\_always\_on)(int slot, bool cclk\_always\_on) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t18set_cclk_always_onE "Permalink to this definition")  

host function to set whether the clock is always enabled

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*do\_transaction)(int slot, \*cmdinfo) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t14do_transactionE "Permalink to this definition")  

host function to do a transaction

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*deinit)(void) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t6deinitE "Permalink to this definition")  

host function to deinitialize the driver

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*deinit\_p)(int slot) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t8deinit_pE "Permalink to this definition")  

host function to deinitialize the driver, called with the `slot`

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*io\_int\_enable)(int slot) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t13io_int_enableE "Permalink to this definition")  

Host function to enable SDIO interrupt line

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*io\_int\_wait)(int slot, uint32\_t timeout\_ticks) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t11io_int_waitE "Permalink to this definition")  

Host function to wait for SDIO interrupt line to be active

int command\_timeout\_ms [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t18command_timeout_msE "Permalink to this definition")  

timeout, in milliseconds, of a single command. Set to 0 to use the default value.

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*get\_real\_freq)(int slot, int \*real\_freq) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t13get_real_freqE "Permalink to this definition")  

Host function to provide real working freq, based on SDMMC controller setup

sdmmc\_delay\_phase\_t input\_delay\_phase [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t17input_delay_phaseE "Permalink to this definition")  

input delay phase, this will only take into effect when the host works in SDMMC\_FREQ\_HIGHSPEED or SDMMC\_FREQ\_52M. Driver will print out how long the delay is

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*set\_input\_delay)(int slot, sdmmc\_delay\_phase\_t delay\_phase) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t15set_input_delayE "Permalink to this definition")  

set input delay phase

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*set\_input\_delayline)(int slot, sdmmc\_delay\_line\_t delay\_line) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t19set_input_delaylineE "Permalink to this definition")  

set input delay line

size\_t unaligned\_multi\_block\_rw\_max\_chunk\_size [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t39unaligned_multi_block_rw_max_chunk_sizeE "Permalink to this definition")  

Maximum number of blocks to read/write at once when using an unaligned buffer.

When a multi-block read/write is requested with an unaligned buffer, the driver splits the transfer into chunks of this many blocks. Set to 0 to use the default value of 1 (single-block transfers). Higher values improve throughput but require a larger DMA-capable temporary buffer.

void \*dma\_aligned\_buffer [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t18dma_aligned_bufferE "Permalink to this definition")  

Cache aligned buffer for multi-block RW and IO commands.

Use cases:

- Temporary buffer for multi-block read/write transactions to/from unaligned buffers. Allocate with DMA capable memory, size should be an integer multiple of your card's sector size. The number of blocks transferred per chunk is controlled by `unaligned_multi_block_rw_max_chunk_size`.
- Cache aligned buffer for IO commands in SDIO mode. If you allocate manually, make sure it is at least SDMMC\_IO\_BLOCK\_SIZE bytes large.

[sd\_pwr\_ctrl\_handle\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/sdmmc_host.html#_CPPv420sd_pwr_ctrl_handle_t "sd_pwr_ctrl_handle_t") pwr\_ctrl\_handle [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t15pwr_ctrl_handleE "Permalink to this definition")  

Power control handle

bool (\*check\_buffer\_alignment)(int slot, const void \*buf, size\_t size) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t22check_buffer_alignmentE "Permalink to this definition")  

Check if buffer meets alignment requirements

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*is\_slot\_set\_to\_uhs1)(int slot, bool \*is\_uhs1) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_host_t19is_slot_set_to_uhs1E "Permalink to this definition")  

host slot is set to uhs1 or not

struct sdmmc\_card\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv412sdmmc_card_t "Permalink to this definition")  

SD/MMC card information structure

Public Members

host [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t4hostE "Permalink to this definition")  

Host with which the card is associated

uint32\_t ocr [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t3ocrE "Permalink to this definition")  

OCR (Operation Conditions Register) value

cid [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t3cidE "Permalink to this definition")  

decoded CID (Card IDentification) register value

raw\_cid [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t7raw_cidE "Permalink to this definition")  

raw CID of MMC card to be decoded after the CSD is fetched in the data transfer mode

csd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t3csdE "Permalink to this definition")  

decoded CSD (Card-Specific Data) register value

scr [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t3scrE "Permalink to this definition")  

decoded SCR (SD card Configuration Register) value

ssr [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t3ssrE "Permalink to this definition")  

decoded SSR (SD Status Register) value

ext\_csd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t7ext_csdE "Permalink to this definition")  

decoded EXT\_CSD (Extended Card Specific Data) register value

uint16\_t rca [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t3rcaE "Permalink to this definition")  

RCA (Relative Card Address)

uint32\_t max\_freq\_khz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t12max_freq_khzE "Permalink to this definition")  

Maximum frequency, in kHz, supported by the card

int real\_freq\_khz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t13real_freq_khzE "Permalink to this definition")  

Real working frequency, in kHz, configured on the host controller

uint32\_t is\_mem [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t6is_memE "Permalink to this definition")  

Bit indicates if the card is a memory card

uint32\_t is\_sdio [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t7is_sdioE "Permalink to this definition")  

Bit indicates if the card is an IO card

uint32\_t is\_mmc [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t6is_mmcE "Permalink to this definition")  

Bit indicates if the card is MMC

uint32\_t num\_io\_functions [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t16num_io_functionsE "Permalink to this definition")  

If is\_sdio is 1, contains the number of IO functions on the card

uint32\_t log\_bus\_width [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t13log_bus_widthE "Permalink to this definition")  

log2(bus width supported by card)

uint32\_t is\_ddr [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t6is_ddrE "Permalink to this definition")  

Card supports DDR mode

uint32\_t is\_uhs1 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t7is_uhs1E "Permalink to this definition")  

Card supports UHS-1 mode

uint32\_t reserved [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N12sdmmc_card_t8reservedE "Permalink to this definition")  

Reserved for future expansion

### Macros

SDMMC\_HOST\_FLAG\_1BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_1BIT "Permalink to this definition")  

host supports 1-line SD and MMC protocol

SDMMC\_HOST\_FLAG\_4BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_4BIT "Permalink to this definition")  

host supports 4-line SD and MMC protocol

SDMMC\_HOST\_FLAG\_8BIT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_8BIT "Permalink to this definition")  

host supports 8-line MMC protocol

SDMMC\_HOST\_FLAG\_SPI [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_SPI "Permalink to this definition")  

host supports SPI protocol

SDMMC\_HOST\_FLAG\_DDR [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_DDR "Permalink to this definition")  

host supports DDR mode for SD/MMC

SDMMC\_HOST\_FLAG\_DEINIT\_ARG [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_DEINIT_ARG "Permalink to this definition")  

host `deinit` function called with the slot argument

SDMMC\_HOST\_FLAG\_ALLOC\_ALIGNED\_BUF [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF "Permalink to this definition")  

Allocate internal buffer of 512 bytes that meets DMA's requirements. Currently this is only used by the SDIO driver. Set this flag when using SDIO CMD53 byte mode, with user buffer that is behind the cache or not aligned to 4 byte boundary.

SDMMC\_HOST\_FLAG\_SPI\_IGNORE\_DATA\_CRC [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_HOST_FLAG_SPI_IGNORE_DATA_CRC "Permalink to this definition")  

SPI mode only: Do not enable CRC verification (skip CMD59). Not recommended as it disables data integrity checking.

SDMMC\_FREQ\_DEFAULT [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_DEFAULT "Permalink to this definition")  

SD/MMC Default speed (limited by clock divider)

SDMMC\_FREQ\_HIGHSPEED [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_HIGHSPEED "Permalink to this definition")  

SD High speed (limited by clock divider)

SDMMC\_FREQ\_PROBING [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_PROBING "Permalink to this definition")  

SD/MMC probing speed

SDMMC\_FREQ\_52M [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_52M "Permalink to this definition")  

MMC 52MHz speed

SDMMC\_FREQ\_26M [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_26M "Permalink to this definition")  

MMC 26MHz speed

SDMMC\_FREQ\_DDR50 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_DDR50 "Permalink to this definition")  

MMC 50MHz speed

SDMMC\_FREQ\_SDR50 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_SDR50 "Permalink to this definition")  

MMC 100MHz speed

SDMMC\_FREQ\_SDR104 [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#c.SDMMC_FREQ_SDR104 "Permalink to this definition")  

MMC 200MHz speed

### Type Definitions

typedef uint32\_t sdmmc\_response\_t\[4\] [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv416sdmmc_response_t "Permalink to this definition")  

SD/MMC command response buffer

### Enumerations

enum sdmmc\_driver\_strength\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv423sdmmc_driver_strength_t "Permalink to this definition")  

SD/MMC Driver Strength.

*Values:*

enumerator SDMMC\_DRIVER\_STRENGTH\_B [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N23sdmmc_driver_strength_t23SDMMC_DRIVER_STRENGTH_BE "Permalink to this definition")  

Type B

enumerator SDMMC\_DRIVER\_STRENGTH\_A [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N23sdmmc_driver_strength_t23SDMMC_DRIVER_STRENGTH_AE "Permalink to this definition")  

Type A

enumerator SDMMC\_DRIVER\_STRENGTH\_C [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N23sdmmc_driver_strength_t23SDMMC_DRIVER_STRENGTH_CE "Permalink to this definition")  

Type C

enumerator SDMMC\_DRIVER\_STRENGTH\_D [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N23sdmmc_driver_strength_t23SDMMC_DRIVER_STRENGTH_DE "Permalink to this definition")  

Type D

enum sdmmc\_current\_limit\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv421sdmmc_current_limit_t "Permalink to this definition")  

SD/MMC Current Limit.

*Values:*

enumerator SDMMC\_CURRENT\_LIMIT\_200MA [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N21sdmmc_current_limit_t25SDMMC_CURRENT_LIMIT_200MAE "Permalink to this definition")  

200 mA

enumerator SDMMC\_CURRENT\_LIMIT\_400MA [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N21sdmmc_current_limit_t25SDMMC_CURRENT_LIMIT_400MAE "Permalink to this definition")  

400 mA

enumerator SDMMC\_CURRENT\_LIMIT\_600MA [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N21sdmmc_current_limit_t25SDMMC_CURRENT_LIMIT_600MAE "Permalink to this definition")  

600 mA

enumerator SDMMC\_CURRENT\_LIMIT\_800MA [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N21sdmmc_current_limit_t25SDMMC_CURRENT_LIMIT_800MAE "Permalink to this definition")  

800 mA

enum sdmmc\_erase\_arg\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv417sdmmc_erase_arg_t "Permalink to this definition")  

SD/MMC erase command(38) arguments SD: ERASE: Erase the write blocks, physical/hard erase.

DISCARD: Card may deallocate the discarded blocks partially or completely. After discard operation the previously written data may be partially or fully read by the host depending on card implementation.

MMC: ERASE: Does TRIM, applies erase operation to write blocks instead of Erase Group.

DISCARD: The Discard function allows the host to identify data that is no longer required so that the device can erase the data if necessary during background erase events. Applies to write blocks instead of Erase Group After discard operation, the original data may be remained partially or fully accessible to the host dependent on device.

*Values:*

enumerator SDMMC\_ERASE\_ARG [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N17sdmmc_erase_arg_t15SDMMC_ERASE_ARGE "Permalink to this definition")  

Erase operation on SD, Trim operation on MMC

enumerator SDMMC\_DISCARD\_ARG [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/storage/sdmmc.html#_CPPv4N17sdmmc_erase_arg_t17SDMMC_DISCARD_ARGE "Permalink to this definition")  

Discard operation for SD/MMC

### Header File

- [components/esp\_driver\_sdmmc/legacy/include/driver/sdmmc\_types.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_driver_sdmmc/legacy/include/driver/sdmmc_types.h)
- This header file can be included with:
	> ```c
	> #include "driver/sdmmc_types.h"
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