/*
 * ESP32-S3 tutorial 0017:
 * AtomS3R Web UI (graphics upload + WebSocket terminal).
 */

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_app.h"
#include "http_server.h"
#include "storage_fatfs.h"
#include "uart_terminal.h"
#include "wifi_softap.h"
#include "button_input.h"

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(display_app_init());
    display_app_show_boot_screen("AtomS3R Web UI", "Tutorial 0017");

    // Next milestone: WiFi SoftAP + logs for connection instructions.
    ESP_ERROR_CHECK(wifi_softap_start());

    // Storage baseline for future uploads.
    ESP_ERROR_CHECK(storage_fatfs_ensure_graphics_dir());

    // HTTP server baseline.
    ESP_ERROR_CHECK(http_server_start());

    // UART + button -> websocket streams.
    ESP_ERROR_CHECK(uart_terminal_start());
    ESP_ERROR_CHECK(button_input_start());

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


