# picocalc_lcd

Reusable PicoCalc 320×320 RGB565 LCD component extracted from `0099-esp32-p4-picocalc-display-keyboard`.

## Hardware mapping

Same-position RPico socket adapter:

| PicoCalc / RPico net | ESP32-P4 GPIO |
|---|---:|
| GP10 / LCD SCK | GPIO3 |
| GP11 / LCD MOSI | GPIO2 |
| GP12 / LCD MISO | not used |
| GP13 / LCD CS | GPIO7 |
| GP14 / LCD DC | GPIO24 |
| GP15 / LCD RST | GPIO25 |

## Display configuration

- Resolution: 320×320
- Pixel format: RGB565
- SPI host: `SPI2_HOST`
- SPI clock source: `SPI_CLK_SRC_SPLL`
- Default requested SCLK: 80 MHz
- Maximum SPI transfer size: 32 KiB
- Internal DMA fill buffer: allocated lazily for fill operations

The SPLL clock source and 32 KiB transfer size come from the validated 0099 display work. ESP32-P4's default GPSPI source is XTAL, which rejects SCLK requests above 20 MHz. SPLL is required for the tested high-speed path.

## Public API

- `picocalc_lcd_init()` initializes the bus, panel, and RGB565 mode.
- `picocalc_lcd_fill()` fills the full display.
- `picocalc_lcd_fill_rect()` fills a clipped rectangle with one RGB565 color.
- `picocalc_lcd_blit_rect()` transfers caller-provided RGB565 pixels to a rectangle.
- `picocalc_lcd_blit_row()` transfers one full-width row band.
- `picocalc_lcd_actual_khz()` reports the ESP-IDF actual SPI frequency when available.

The visual REPL renderer should build row-sized RGB565 buffers and call `picocalc_lcd_blit_row()` for dirty rows.
