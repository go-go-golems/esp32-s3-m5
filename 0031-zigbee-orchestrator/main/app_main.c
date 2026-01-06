/*
 * Ticket 0031: Zigbee orchestrator (Cardputer host)
 *
 * Phase 1/2 bring-up: USB-Serial/JTAG esp_console + application esp_event bus.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"

#include "gw_bus.h"
#include "gw_console.h"
#include "gw_zb.h"

void app_main(void) {
    ESP_ERROR_CHECK(gw_bus_start());
    ESP_ERROR_CHECK(gw_console_start());
    ESP_ERROR_CHECK(gw_zb_start());

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
