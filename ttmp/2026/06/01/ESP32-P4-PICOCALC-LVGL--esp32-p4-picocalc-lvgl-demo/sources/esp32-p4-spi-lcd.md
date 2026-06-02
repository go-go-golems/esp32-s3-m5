---
Title: Source - esp32-p4-spi-lcd
Ticket: ESP32-P4-PICOCALC-LVGL
Status: active
Topics:
    - esp32-p4
    - picocalc
    - lvgl
DocType: source
Intent: reference
Summary: "Downloaded reference material for LVGL investigation"
---

## SPI Interfaced LCD

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0.1/esp32p4/api-reference/peripherals/lcd/spi_lcd.html)

1. Create an SPI bus. Please refer to [SPI Master API doc](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/spi_master.html) for more details.
	> Currently the driver supports SPI, Quad SPI and Octal SPI (simulate Intel 8080 timing) modes.
	> 
	> ```c
	> spi_bus_config_t buscfg = {
	>     .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
	>     .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
	>     .miso_io_num = EXAMPLE_PIN_NUM_MISO,
	>     .quadwp_io_num = -1,
	>     .quadhd_io_num = -1,
	>     .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t), // transfer 80 lines of pixels (assume pixel is RGB565) at most in one SPI transaction
	> };
	> ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO)); // Enable the DMA feature
	> ```
2. Allocate an LCD IO device handle from the SPI bus. In this step, you need to provide the following information:
	> - sets the GPIO number for the DC signal line (some LCD calls this `RS` line). The LCD driver uses this GPIO to switch between sending command and sending data.
	> - sets the GPIO number for the CS signal line. The LCD driver uses this GPIO to select the LCD chip. If the SPI bus only has one device attached (i.e., this LCD), you can set the GPIO number to `-1` to occupy the bus exclusively.
	> - sets the frequency of the pixel clock, in Hz. The value should not exceed the range recommended in the LCD spec.
	> - sets the SPI mode. The LCD driver uses this mode to communicate with the LCD. For the meaning of the SPI mode, please refer to the [SPI Master API doc](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/spi_master.html).
	> - and set the bit width of the command and parameter that recognized by the LCD controller chip. This is chip specific, you should refer to your LCD spec in advance.
	> - sets the depth of the SPI transaction queue. A bigger value means more transactions can be queued up, but it also consumes more memory.
	> - sets the amount of SPI bit-cycles which the cs should be activated before the transmission (0-16).
	> - sets the amount of SPI bit-cycles which the cs should stay active after the transmission (0-16).
	> 
	> ```c
	> esp_lcd_panel_io_handle_t io_handle = NULL;
	> esp_lcd_panel_io_spi_config_t io_config = {
	>     .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
	>     .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
	>     .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
	>     .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
	>     .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
	>     .spi_mode = 0,
	>     .trans_queue_depth = 10,
	> };
	> // Attach the LCD to the SPI bus
	> ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
	> ```
3. Install the LCD controller driver. The LCD controller driver is responsible for sending the commands and parameters to the LCD controller chip. In this step, you need to specify the SPI IO device handle that allocated in the last step, and some panel specific configurations:
	> - `esp_lcd_panel_dev_config_t::reset_gpio_num` sets the LCD's hardware reset GPIO number. If the LCD does not have a hardware reset pin, set this to `-1`.
	> - `esp_lcd_panel_dev_config_t::rgb_ele_order` sets the RGB element order of each color data.
	> - `esp_lcd_panel_dev_config_t::bits_per_pixel` sets the bit width of the pixel color data. The LCD driver uses this value to calculate the number of bytes to send to the LCD controller chip.
	> - `esp_lcd_panel_dev_config_t::data_endian` specifies the data endian to be transmitted to the screen. No need to specify for color data within one byte, like RGB232. For drivers that do not support specifying data endian, this field would be ignored.
	> 
	> ```c
	> esp_lcd_panel_handle_t panel_handle = NULL;
	> esp_lcd_panel_dev_config_t panel_config = {
	>     .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
	>     .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
	>     .bits_per_pixel = 16,
	> };
	> // Create LCD panel handle for ST7789, with the SPI IO device handle
	> ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
	> ```

## API Reference

### Header File

- [components/esp\_lcd/include/esp\_lcd\_io\_spi.h](https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_lcd/include/esp_lcd_io_spi.h)
- This header file can be included with:
	> ```c
	> #include "esp_lcd_io_spi.h"
	> ```
- This header file is a part of the API provided by the `esp_lcd` component. To declare that your component depends on `esp_lcd`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES esp_lcd
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES esp_lcd
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_lcd\_new\_panel\_io\_spi( bus, const \*io\_config, [esp\_lcd\_panel\_io\_handle\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/index.html#_CPPv425esp_lcd_panel_io_handle_t "esp_lcd_panel_io_handle_t") \*ret\_io) [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv424esp_lcd_new_panel_io_spi24esp_lcd_spi_bus_handle_tPK29esp_lcd_panel_io_spi_config_tP25esp_lcd_panel_io_handle_t "Permalink to this definition")  

Create LCD panel IO handle, for SPI interface.

Parameters:

- **bus** -- **\[in\]** SPI bus handle
- **io\_config** -- **\[in\]** IO configuration, for SPI interface
- **ret\_io** -- **\[out\]** Returned IO handle

Returns:

- ESP\_ERR\_INVALID\_ARG if parameter is invalid
- ESP\_ERR\_NO\_MEM if out of memory
- ESP\_OK on success

### Structures

struct esp\_lcd\_panel\_io\_spi\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv429esp_lcd_panel_io_spi_config_t "Permalink to this definition")  

Panel IO configuration structure, for SPI interface.

Public Members

gpio\_num\_t cs\_gpio\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t11cs_gpio_numE "Permalink to this definition")  

GPIO used for CS line

gpio\_num\_t dc\_gpio\_num [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t11dc_gpio_numE "Permalink to this definition")  

GPIO used to select the D/C line, set this to -1 if the D/C line is not used

int spi\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t8spi_modeE "Permalink to this definition")  

Traditional SPI mode (0~3)

unsigned int pclk\_hz [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t7pclk_hzE "Permalink to this definition")  

Frequency of pixel clock

size\_t trans\_queue\_depth [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t17trans_queue_depthE "Permalink to this definition")  

Size of internal transaction queue

[esp\_lcd\_panel\_io\_color\_trans\_done\_cb\_t](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/index.html#_CPPv438esp_lcd_panel_io_color_trans_done_cb_t "esp_lcd_panel_io_color_trans_done_cb_t") on\_color\_trans\_done [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19on_color_trans_doneE "Permalink to this definition")  

Callback invoked when color data transfer has finished

void \*user\_ctx [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t8user_ctxE "Permalink to this definition")  

User private data, passed directly to on\_color\_trans\_done's user\_ctx

int lcd\_cmd\_bits [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t12lcd_cmd_bitsE "Permalink to this definition")  

Bit-width of LCD command

int lcd\_param\_bits [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t14lcd_param_bitsE "Permalink to this definition")  

Bit-width of LCD parameter

uint8\_t cs\_ena\_pretrans [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t15cs_ena_pretransE "Permalink to this definition")  

Amount of SPI bit-cycles the cs should be activated before the transmission (0-16)

uint8\_t cs\_ena\_posttrans [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t16cs_ena_posttransE "Permalink to this definition")  

Amount of SPI bit-cycles the cs should stay active after the transmission (0-16)

struct:: flags [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t5flagsE "Permalink to this definition")  

Extra flags to fine-tune the SPI device

struct esp\_lcd\_spi\_flags\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_tE "Permalink to this definition")  

Extra flags to fine-tune the SPI device.

Public Members

unsigned int dc\_high\_on\_cmd [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t14dc_high_on_cmdE "Permalink to this definition")  

If enabled, DC level = 1 indicates command transfer

unsigned int dc\_low\_on\_data [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t14dc_low_on_dataE "Permalink to this definition")  

If enabled, DC level = 0 indicates color data transfer

unsigned int dc\_low\_on\_param [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t15dc_low_on_paramE "Permalink to this definition")  

If enabled, DC level = 0 indicates parameter transfer

unsigned int octal\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t10octal_modeE "Permalink to this definition")  

transmit data and parameters with 8 lines

unsigned int quad\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t9quad_modeE "Permalink to this definition")  

transmit data and parameters with 4 lines

unsigned int sio\_mode [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t8sio_modeE "Permalink to this definition")  

Read and write through a single data line (MOSI)

unsigned int lsb\_first [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t9lsb_firstE "Permalink to this definition")  

Transmit LSB bit first

unsigned int cs\_high\_active [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv4N29esp_lcd_panel_io_spi_config_t19esp_lcd_spi_flags_t14cs_high_activeE "Permalink to this definition")  

CS line is high active

### Type Definitions

typedef int esp\_lcd\_spi\_bus\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/lcd/spi_lcd.html#_CPPv424esp_lcd_spi_bus_handle_t "Permalink to this definition")  

Type of LCD SPI bus handle