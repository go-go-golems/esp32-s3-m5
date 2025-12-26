/*
 * Tutorial 0017: UART terminal bridge (UART <-> WebSocket).
 */

#include "uart_terminal.h"

#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_log.h"

#include "http_server.h"

static const char *TAG = "atoms3r_web_ui_0017";

static bool s_started = false;
static uart_port_t s_uart_num = UART_NUM_1;

static esp_err_t ws_binary_to_uart(const uint8_t *data, size_t len) {
    if (!data || len == 0) return ESP_OK;
    const int n = uart_write_bytes(s_uart_num, (const char *)data, len);
    return (n < 0) ? ESP_FAIL : ESP_OK;
}

static void uart_rx_task(void *arg) {
    (void)arg;
    uint8_t buf[256];
    while (true) {
        const int n = uart_read_bytes(s_uart_num, buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (n > 0) {
            (void)http_server_ws_broadcast_binary(buf, (size_t)n);
        }
    }
}

esp_err_t uart_terminal_start(void) {
    if (s_started) {
        return ESP_OK;
    }

    s_uart_num = (uart_port_t)CONFIG_TUTORIAL_0017_UART_NUM;

    uart_config_t cfg = {};
    cfg.baud_rate = CONFIG_TUTORIAL_0017_UART_BAUDRATE;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    const int rx_buf = 4096;
    const int tx_buf = 2048;

    ESP_ERROR_CHECK(uart_driver_install(s_uart_num, rx_buf, tx_buf, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(s_uart_num, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(s_uart_num,
                                 CONFIG_TUTORIAL_0017_UART_TX_GPIO,
                                 CONFIG_TUTORIAL_0017_UART_RX_GPIO,
                                 UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "uart terminal: UART%u baud=%d tx=%d rx=%d",
             (unsigned)s_uart_num,
             CONFIG_TUTORIAL_0017_UART_BAUDRATE,
             CONFIG_TUTORIAL_0017_UART_TX_GPIO,
             CONFIG_TUTORIAL_0017_UART_RX_GPIO);

    // Browser -> device: bind websocket binary RX to uart writes.
    http_server_ws_set_binary_rx_cb(ws_binary_to_uart);

    xTaskCreate(uart_rx_task, "uart_rx", 4096, nullptr, 10, nullptr);

    s_started = true;
    return ESP_OK;
}


