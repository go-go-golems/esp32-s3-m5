#include <stdint.h>

#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"

#include "wifi_console.h"
#include "wifi_mgr.h"
#include "mled_node.h"

static const char *TAG = "mlednode_0049";

static void on_wifi_got_ip(uint32_t ip4_host_order, void *ctx)
{
    (void)ip4_host_order;
    (void)ctx;
    mled_node_start();
}

void app_main(void)
{
    ESP_LOGI(TAG, "boot: ESP-IDF %s", esp_get_idf_version());
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());

    wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, NULL);
    ESP_ERROR_CHECK(wifi_mgr_start());

    // Console is intentionally started after Wi-Fi stack bring-up, so the first prompt
    // appears quickly and early Wi-Fi logs are visible.
    wifi_console_start();
}
