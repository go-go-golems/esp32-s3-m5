/*
 * printer_drv.c — Thermal printer UART driver for M5Stack K118.
 *
 * Sends ESC/POS commands over UART1 (9600 8N1) to the thermal printer
 * mechanism.  TX=GPIO5, RX=GPIO6 on the AtomS3R Lite.
 */

#include "printer_drv.h"

#include <string.h>
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "printer_drv";

/* ---- internal helpers ------------------------------------------------- */

static esp_err_t send_bytes(const uint8_t *data, size_t len)
{
    /* Log what we're sending */
    if (len <= 32) {
        char hex[128];
        for (size_t i = 0; i < len; i++) {
            sprintf(hex + i * 3, "%02X ", data[i]);
        }
        ESP_LOGI(TAG, "TX %u bytes: %s", (unsigned)len, hex);
    } else {
        ESP_LOGI(TAG, "TX %u bytes (head: %02X %02X %02X %02X ...)",
                 (unsigned)len, data[0], data[1], data[2], data[3]);
    }

    const int written = uart_write_bytes(PRINTER_UART_NUM, data, len);
    if (written < 0 || (size_t)written != len) {
        ESP_LOGE(TAG, "UART write failed: wrote %d of %u bytes",
                 written, (unsigned)len);
        return ESP_FAIL;
    }
    /* Wait for the UART TX FIFO to drain */
    esp_err_t wait_err = uart_wait_tx_done(PRINTER_UART_NUM, pdMS_TO_TICKS(500));
    if (wait_err != ESP_OK) {
        ESP_LOGW(TAG, "uart_wait_tx_done: %s", esp_err_to_name(wait_err));
    }
    return ESP_OK;
}

/* ---- public API ------------------------------------------------------- */

esp_err_t printer_drv_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = PRINTER_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(
        uart_driver_install(PRINTER_UART_NUM, 1024, 1024, 0, NULL, 0),
        TAG, "uart_driver_install");

    ESP_RETURN_ON_ERROR(
        uart_param_config(PRINTER_UART_NUM, &uart_config),
        TAG, "uart_param_config");

    ESP_RETURN_ON_ERROR(
        uart_set_pin(PRINTER_UART_NUM,
                     PRINTER_TX_GPIO, PRINTER_RX_GPIO,
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
        TAG, "uart_set_pin");

    ESP_LOGI(TAG, "Printer UART%d ready: TX=%d RX=%d baud=%d",
             PRINTER_UART_NUM, PRINTER_TX_GPIO, PRINTER_RX_GPIO, PRINTER_BAUD);

    return printer_drv_reset();
}

esp_err_t printer_drv_reset(void)
{
    static const uint8_t cmd[] = { 0x1B, 0x40 }; /* ESC @ */
    return send_bytes(cmd, sizeof(cmd));
}

esp_err_t printer_drv_print_text(const char *text)
{
    if (!text) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(send_bytes((const uint8_t *)text, strlen(text)),
                        TAG, "print_text data");
    uint8_t lf = 0x0A;
    return send_bytes(&lf, 1);
}

esp_err_t printer_drv_feed(uint8_t lines)
{
    /* ESC d n — Print and feed n lines */
    uint8_t cmd[] = { 0x1B, 0x64, lines };
    return send_bytes(cmd, sizeof(cmd));
}

esp_err_t printer_drv_set_font_size(uint8_t size)
{
    if (size > 7) size = 7;
    /* GS ! n — n = (height<<4) | width, each nibble 0–7 */
    uint8_t cmd[] = { 0x1D, 0x21, (uint8_t)((size << 4) | size) };
    return send_bytes(cmd, sizeof(cmd));
}

esp_err_t printer_drv_set_bold(bool on)
{
    /* ESC E n — n=1 bold on, n=0 bold off */
    uint8_t cmd[] = { 0x1B, 0x45, on ? 1 : 0 };
    return send_bytes(cmd, sizeof(cmd));
}

esp_err_t printer_drv_set_align(uint8_t align)
{
    if (align > 2) align = 0;
    /* ESC a n — 0=left 1=center 2=right */
    uint8_t cmd[] = { 0x1B, 0x61, align };
    return send_bytes(cmd, sizeof(cmd));
}

esp_err_t printer_drv_print_barcode(printer_barcode_type_t type,
                                     const char *data)
{
    if (!data) return ESP_ERR_INVALID_ARG;
    size_t len = strlen(data);
    /* GS k m n d1..dn */
    uint8_t header[] = { 0x1D, 0x6B, (uint8_t)type, (uint8_t)len };
    ESP_RETURN_ON_ERROR(send_bytes(header, sizeof(header)),
                        TAG, "barcode header");
    return send_bytes((const uint8_t *)data, len);
}

esp_err_t printer_drv_print_qr(printer_qr_ec_level_t ec_level,
                                 const char *data)
{
    if (!data) return ESP_ERR_INVALID_ARG;

    size_t len = strlen(data);

    /* Step 1: Set error correction level
     * GS ( k pL pH cn=49 fn=69 [n]   — but in the M5Stack dialect it's fn=69
     * Actually the M5Stack ATOM_PRINTER library uses:
     *   1D 28 6B 03 00 31 45 n           (cn=49 fn=69 = "set EC level")
     */
    uint8_t ecl_cmd[] = {
        0x1D, 0x28, 0x6B, 0x03, 0x00,
        0x31, 0x45, (uint8_t)ec_level
    };
    ESP_RETURN_ON_ERROR(send_bytes(ecl_cmd, sizeof(ecl_cmd)),
                        TAG, "qr ecl");

    /* Step 2: Store QR data
     * GS ( k pL pH cn=49 fn=80 m=48 d1..dk
     * pL = (len + 3) & 0xFF, pH = (len + 3) >> 8
     */
    uint16_t pL = (uint16_t)(len + 3);
    uint8_t store_header[] = {
        0x1D, 0x28, 0x6B,
        (uint8_t)(pL & 0xFF), (uint8_t)(pL >> 8),
        0x31, 0x50, 0x30
    };
    ESP_RETURN_ON_ERROR(send_bytes(store_header, sizeof(store_header)),
                        TAG, "qr store header");
    ESP_RETURN_ON_ERROR(send_bytes((const uint8_t *)data, len),
                        TAG, "qr store data");

    /* Step 3: Print QR code
     * GS ( k 03 00 cn=49 fn=81 m=48
     */
    uint8_t print_cmd[] = {
        0x1D, 0x28, 0x6B, 0x03, 0x00,
        0x31, 0x51, 0x30
    };
    return send_bytes(print_cmd, sizeof(print_cmd));
}

esp_err_t printer_drv_print_bitmap(uint16_t width, uint16_t height,
                                    const uint8_t *pixels)
{
    if (!pixels) return ESP_ERR_INVALID_ARG;

    uint16_t bytes_per_row = width / 8;

    /* GS v 0 m xL xH yL yH d1..dk */
    uint8_t header[] = {
        0x1D, 0x76, 0x30, 0x00,   /* mode=0 (normal) */
        (uint8_t)(bytes_per_row & 0xFF),
        (uint8_t)(bytes_per_row >> 8),
        (uint8_t)(height & 0xFF),
        (uint8_t)(height >> 8)
    };
    ESP_RETURN_ON_ERROR(send_bytes(header, sizeof(header)),
                        TAG, "bitmap header");
    return send_bytes(pixels, (size_t)bytes_per_row * height);
}

/* ---- probe / diagnostic functions ------------------------------------- */

int printer_drv_drain_rx(void)
{
    uint8_t buf[64];
    int total = 0;
    while (true) {
        int n = uart_read_bytes(PRINTER_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (n <= 0) break;
        total += n;
        /* Log received bytes */
        char hex[256];
        for (int i = 0; i < n && i < 64; i++) {
            sprintf(hex + i * 3, "%02X ", buf[i]);
        }
        ESP_LOGI(TAG, "RX %d bytes: %s", n, hex);
    }
    return total;
}

esp_err_t printer_drv_query_status(uint8_t n, uint8_t *out)
{
    if (n < 1 || n > 4 || !out) return ESP_ERR_INVALID_ARG;

    /* Drain any stale data first */
    printer_drv_drain_rx();

    /* DLE EOT n — Real-time status transmission */
    uint8_t cmd[] = { 0x10, 0x04, n };
    esp_err_t err = send_bytes(cmd, sizeof(cmd));
    if (err != ESP_OK) return err;

    /* Wait briefly for response */
    uint8_t resp = 0;
    int got = uart_read_bytes(PRINTER_UART_NUM, &resp, 1, pdMS_TO_TICKS(500));
    if (got == 1) {
        ESP_LOGI(TAG, "Status query n=%d response: 0x%02X", n, resp);
        *out = resp;
        return ESP_OK;
    }
    ESP_LOGW(TAG, "Status query n=%d: no response (got %d bytes)", n, got);
    return ESP_ERR_TIMEOUT;
}

esp_err_t printer_drv_send_raw(const uint8_t *data, size_t len)
{
    if (!data || len == 0) return ESP_ERR_INVALID_ARG;
    return send_bytes(data, len);
}
