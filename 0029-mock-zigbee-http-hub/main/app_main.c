/*
 * Tutorial 0029: Mock Zigbee HTTP hub
 *
 * Brings up WiFi STA, starts an application esp_event loop, starts the device simulator,
 * and serves an HTTP API + WebSocket event stream.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"

#include "hub_bus.h"
#include "hub_http.h"
#include "hub_pb.h"
#include "hub_registry.h"
#include "hub_sim.h"
#include "wifi_console.h"
#include "wifi_sta.h"

void app_main(void) {
    ESP_ERROR_CHECK(hub_wifi_start());
    wifi_console_start();
    ESP_ERROR_CHECK(hub_registry_init());
    ESP_ERROR_CHECK(hub_bus_start());
    ESP_ERROR_CHECK(hub_pb_register(hub_bus_get_loop()));
    ESP_ERROR_CHECK(hub_http_start());
    ESP_ERROR_CHECK(hub_sim_start());

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
