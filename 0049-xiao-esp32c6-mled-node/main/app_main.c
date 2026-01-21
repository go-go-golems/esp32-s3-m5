#include <stdint.h>

#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"

#include "wifi_console.h"
#include "wifi_mgr.h"
#include "mled_effect_led.h"
#include "mled_node.h"

static const char *TAG = "mlednode_0049";

static void on_node_apply(uint32_t cue_id, const mled_pattern_config_t *pattern, void *ctx)
{
    (void)cue_id;
    (void)ctx;
    mled_effect_led_apply_pattern(pattern);
}

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

    mled_effect_led_start();
    mled_node_set_on_apply(on_node_apply, NULL);

    wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, NULL);
    ESP_ERROR_CHECK(wifi_mgr_start());

    // Console is intentionally started after Wi-Fi stack bring-up, so the first prompt
    // appears quickly and early Wi-Fi logs are visible.
    wifi_console_start();
}
