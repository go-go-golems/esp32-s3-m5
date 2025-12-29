#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "uart-echo";

// Track C firmware: keep it minimal and explicit.
// - Use UART0 (default console UART in ESP-IDF/QEMU setups).
// - Read with finite timeout so we can log a heartbeat.
// - Echo bytes back using uart_write_bytes() (avoid stdio buffering).

static void uart_echo_task(void *arg)
{
    (void)arg;

    uint8_t buf[128];
    uint32_t no_rx_ticks = 0;

    ESP_LOGI(TAG, "UART echo task started. Type bytes; they will be echoed back.");

    while (1) {
        int n = uart_read_bytes(UART_NUM_0, buf, sizeof(buf), pdMS_TO_TICKS(200));
        if (n > 0) {
            no_rx_ticks = 0;

            // Echo raw bytes back to UART0.
            int written = uart_write_bytes(UART_NUM_0, (const char *)buf, n);
            if (written < 0) {
                ESP_LOGW(TAG, "uart_write_bytes returned %d", written);
            }

            // Also log a short summary (bytes + printable preview).
            char preview[64];
            size_t preview_n = (size_t)n;
            if (preview_n >= sizeof(preview)) {
                preview_n = sizeof(preview) - 1;
            }
            memcpy(preview, buf, preview_n);
            for (size_t i = 0; i < preview_n; i++) {
                if (preview[i] < 32 || preview[i] > 126) {
                    preview[i] = '.';
                }
            }
            preview[preview_n] = '\0';
            ESP_LOGI(TAG, "RX %d bytes (preview): %s", n, preview);
        } else {
            // No RX. Periodically print a heartbeat so we know the task loop is alive.
            no_rx_ticks++;
            if (no_rx_ticks % 25 == 0) { // ~5 seconds at 200ms per loop
                ESP_LOGI(TAG, "No RX for ~5s (still alive).");
            }
        }
    }
}

void app_main(void)
{
    // UART0 config.
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_LOGI(TAG, "Configuring UART0 @ 115200 8N1");
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &cfg));

    // RX buffer enabled (2k). TX buffer left as 0 (uses driver’s minimal path).
    // If we discover TX buffering matters, we can revisit, but keep it minimal first.
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0));

    xTaskCreate(uart_echo_task, "uart_echo", 4096, NULL, 5, NULL);

    // app_main returns; the task should keep running.
    ESP_LOGI(TAG, "app_main done; uart_echo task should now own the loop.");
}


