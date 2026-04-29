#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* Printer UART configuration for AtomS3R Lite + K118 */
#define PRINTER_UART_NUM   UART_NUM_1
#define PRINTER_TX_GPIO    5
#define PRINTER_RX_GPIO    6
#define PRINTER_BAUD       9600

/* Barcode types (ESC/POS) */
typedef enum {
    PRINTER_BC_UPC_A    = 0x41,
    PRINTER_BC_UPC_E    = 0x42,
    PRINTER_BC_EAN13    = 0x43,
    PRINTER_BC_EAN8     = 0x44,
    PRINTER_BC_CODE39   = 0x45,
    PRINTER_BC_ITF      = 0x46,
    PRINTER_BC_CODABAR  = 0x47,
    PRINTER_BC_CODE93   = 0x48,
    PRINTER_BC_CODE128  = 0x49,
} printer_barcode_type_t;

/* QR error correction levels */
typedef enum {
    PRINTER_QR_EC_L = 0x48,  /* Low    (~7%)  */
    PRINTER_QR_EC_M = 0x49,  /* Medium (~15%) */
    PRINTER_QR_EC_Q = 0x4A,  /* Quartile (~25%) */
    PRINTER_QR_EC_H = 0x4B,  /* High   (~30%) */
} printer_qr_ec_level_t;

/**
 * Initialize the printer UART (9600 8N1 on GPIO5/GPIO6) and send ESC @.
 */
esp_err_t printer_drv_init(void);

/**
 * Send ESC @ to reset the printer to power-on defaults.
 */
esp_err_t printer_drv_reset(void);

/**
 * Print a plain-text string (ASCII) followed by a line feed.
 */
esp_err_t printer_drv_print_text(const char *text);

/**
 * Feed paper by `lines` lines.
 */
esp_err_t printer_drv_feed(uint8_t lines);

/**
 * Set font size. Both width and height are set to `size` (0–7).
 * 0 = normal, 7 = 8x magnification in each dimension.
 */
esp_err_t printer_drv_set_font_size(uint8_t size);

/**
 * Enable or disable bold (emphasized) printing.
 */
esp_err_t printer_drv_set_bold(bool on);

/**
 * Set text alignment: 0=left, 1=center, 2=right.
 */
esp_err_t printer_drv_set_align(uint8_t align);

/**
 * Print a barcode of the given type containing `data`.
 */
esp_err_t printer_drv_print_barcode(printer_barcode_type_t type,
                                     const char *data);

/**
 * Print a QR code containing `data` with the given error correction level.
 */
esp_err_t printer_drv_print_qr(printer_qr_ec_level_t ec_level,
                                 const char *data);

/**
 * Print a 1-bit monochrome raster bitmap (MSB first).
 * `width` must be a multiple of 8. `pixels` points to (width/8)*height bytes.
 */
esp_err_t printer_drv_print_bitmap(uint16_t width, uint16_t height,
                                    const uint8_t *pixels);
