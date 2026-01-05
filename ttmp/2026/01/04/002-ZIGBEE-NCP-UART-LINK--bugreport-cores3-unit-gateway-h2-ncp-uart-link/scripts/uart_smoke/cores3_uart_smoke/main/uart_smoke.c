#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CORES3_UART_SMOKE";

static const uart_port_t BUS_UART = UART_NUM_1;
static const int BUS_TX_PIN = 17;
static const int BUS_RX_PIN = 18;

static void tx_task(void *arg)
{
    uint32_t counter = 0;
    while (true) {
        char line[64];
        int n = snprintf(line, sizeof(line), "PING %u\n", (unsigned)counter++);
        if (n > 0) {
            ESP_LOGI(TAG, "TX -> %s", line);
            uart_write_bytes(BUS_UART, line, n);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void rx_task(void *arg)
{
    uint8_t buf[256];
    while (true) {
        int n = uart_read_bytes(BUS_UART, buf, sizeof(buf) - 1, pdMS_TO_TICKS(250));
        if (n > 0) {
            buf[n] = 0;
            ESP_LOGI(TAG, "RX <- %.*s", n, (const char *)buf);
        }
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

    xTaskCreate(tx_task, "tx_task", 4096, NULL, 5, NULL);
    xTaskCreate(rx_task, "rx_task", 4096, NULL, 5, NULL);
}

