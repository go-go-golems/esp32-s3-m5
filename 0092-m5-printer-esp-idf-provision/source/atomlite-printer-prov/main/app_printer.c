#include "app_printer.h"

#include <string.h>
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "printer";

static esp_err_t printer_write(const void *data, size_t len)
{
    const int written = uart_write_bytes(ATOM_PRINTER_UART_NUM, data, len);
    if (written < 0 || (size_t)written != len) {
        ESP_LOGE(TAG, "UART write failed: wrote %d of %u bytes", written, (unsigned)len);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t app_printer_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = ATOM_PRINTER_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(uart_driver_install(ATOM_PRINTER_UART_NUM, 1024, 0, 0, NULL, 0), TAG,
                        "install UART driver");
    ESP_RETURN_ON_ERROR(uart_param_config(ATOM_PRINTER_UART_NUM, &uart_config), TAG,
                        "configure UART");
    ESP_RETURN_ON_ERROR(uart_set_pin(ATOM_PRINTER_UART_NUM,
                                     ATOM_PRINTER_TX_GPIO,
                                     ATOM_PRINTER_RX_GPIO,
                                     UART_PIN_NO_CHANGE,
                                     UART_PIN_NO_CHANGE),
                        TAG, "set UART pins");

    ESP_LOGI(TAG, "ATOM printer UART%d ready: TX=%d RX=%d baud=%d",
             ATOM_PRINTER_UART_NUM, ATOM_PRINTER_TX_GPIO, ATOM_PRINTER_RX_GPIO, ATOM_PRINTER_BAUD);
    return app_printer_reset();
}

esp_err_t app_printer_reset(void)
{
    static const uint8_t init_cmd[] = {0x1b, 0x40};
    return printer_write(init_cmd, sizeof(init_cmd));
}

esp_err_t app_printer_print_text(const char *text)
{
    if (text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return printer_write(text, strlen(text));
}

esp_err_t app_printer_feed_lines(unsigned int lines)
{
    static const uint8_t lf = '\n';
    for (unsigned int i = 0; i < lines; ++i) {
        ESP_RETURN_ON_ERROR(printer_write(&lf, 1), TAG, "feed line");
    }
    return ESP_OK;
}

esp_err_t app_printer_print_wifi_status(const char *ip_address)
{
    if (ip_address == NULL) {
        ip_address = "unknown";
    }

    ESP_RETURN_ON_ERROR(app_printer_reset(), TAG, "reset printer");
    ESP_RETURN_ON_ERROR(app_printer_print_text("M5 Printer ESP-IDF\n"), TAG, "print title");
    ESP_RETURN_ON_ERROR(app_printer_print_text("ATOM Lite provisioned\n"), TAG, "print status");
    ESP_RETURN_ON_ERROR(app_printer_print_text("IP: "), TAG, "print ip label");
    ESP_RETURN_ON_ERROR(app_printer_print_text(ip_address), TAG, "print ip");
    ESP_RETURN_ON_ERROR(app_printer_feed_lines(3), TAG, "feed");
    return ESP_OK;
}
