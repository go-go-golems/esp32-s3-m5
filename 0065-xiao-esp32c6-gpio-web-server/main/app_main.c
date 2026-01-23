#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"

#include "gpio_ctl.h"
#include "http_server.h"
#include "oled_status.h"
#include "wifi_console.h"
#include "wifi_mgr.h"

static const char *TAG = "mo065_0065";

static void on_wifi_got_ip(uint32_t ip4_host_order, void *ctx)
{
    (void)ip4_host_order;
    (void)ctx;
    (void)http_server_start();
}

void app_main(void)
{
    ESP_LOGI(TAG, "boot: ESP-IDF %s", esp_get_idf_version());
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());

    ESP_ERROR_CHECK(gpio_ctl_init());

    wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, NULL);
    ESP_ERROR_CHECK(wifi_mgr_start());

    const esp_err_t oled_err = oled_status_start();
    if (oled_err == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "OLED disabled (enable in menuconfig: MO-065: OLED status display)");
    } else if (oled_err != ESP_OK) {
        ESP_LOGW(TAG, "OLED init failed: %s", esp_err_to_name(oled_err));
    }

    const wifi_console_config_t wifi_console_cfg = {
        .prompt = "c6> ",
        .register_extra = NULL,
    };
    wifi_console_start(&wifi_console_cfg);
}
