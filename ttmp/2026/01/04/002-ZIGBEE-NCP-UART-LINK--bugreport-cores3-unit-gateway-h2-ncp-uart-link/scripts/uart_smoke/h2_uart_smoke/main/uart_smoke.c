#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "H2_UART_SMOKE";

static const uart_port_t BUS_UART = UART_NUM_1;
static const int BUS_TX_PIN = 24;
static const int BUS_RX_PIN = 23;

static void rx_echo_task(void *arg)
{
    uint8_t buf[256];
    size_t used = 0;
    uint32_t counter = 0;

    while (true) {
        int n = uart_read_bytes(BUS_UART, buf + used, sizeof(buf) - 1 - used, pdMS_TO_TICKS(250));
        if (n <= 0) {
            continue;
        }
        used += (size_t)n;
        buf[used] = 0;

        uint8_t *newline = (uint8_t *)memchr(buf, '\n', used);
        if (!newline) {
            if (used >= sizeof(buf) - 1) {
                ESP_LOGW(TAG, "RX overflow, dropping");
                used = 0;
            }
            continue;
        }

        size_t line_len = (size_t)(newline - buf) + 1;
        ESP_LOGI(TAG, "RX <- %.*s", (int)line_len, (const char *)buf);

        char reply[96];
        int r = snprintf(reply, sizeof(reply), "PONG %u\n", (unsigned)counter++);
        if (r > 0) {
            ESP_LOGI(TAG, "TX -> %s", reply);
            uart_write_bytes(BUS_UART, reply, r);
        }

        memmove(buf, buf + line_len, used - line_len);
        used -= line_len;
    }
}

void app_main(void)
{
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(BUS_UART, 2048, 2048, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(BUS_UART, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(BUS_UART, BUS_TX_PIN, BUS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_flush_input(BUS_UART));

    ESP_LOGI(TAG, "Configured BUS_UART=%d TX=%d RX=%d @115200", (int)BUS_UART, BUS_TX_PIN, BUS_RX_PIN);

    xTaskCreate(rx_echo_task, "rx_echo_task", 4096, NULL, 5, NULL);
}

