#include <inttypes.h>
#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"

#include "sim_console.h"
#include "sim_engine.h"
#include "sim_ui.h"

#include "http_server.h"
#include "wifi_console.h"
#include "wifi_mgr.h"

static const char *TAG = "0066_ledsim";

static sim_engine_t *s_console_engine = nullptr;

static void register_console_commands(void)
{
    sim_console_register_commands(s_console_engine);
}

static void on_wifi_got_ip(uint32_t ip4_host_order, void *ctx)
{
    (void)ip4_host_order;
    sim_engine_t *engine = (sim_engine_t *)ctx;
    if (!engine) return;

    ESP_LOGI(TAG, "wifi got IP; starting web server...");
    (void)http_server_start(engine);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "boot: reset_reason=%d", (int)esp_reset_reason());

    static sim_engine_t engine;
    ESP_ERROR_CHECK(sim_engine_init(
        &engine,
        (uint16_t)CONFIG_TUTORIAL_0066_LED_COUNT,
        (uint32_t)CONFIG_TUTORIAL_0066_FRAME_MS));

    sim_ui_start(&engine);

    wifi_mgr_set_on_got_ip_cb(&on_wifi_got_ip, &engine);
    ESP_ERROR_CHECK(wifi_mgr_start());

    s_console_engine = &engine;
    wifi_console_config_t console_cfg = {};
    console_cfg.prompt = "sim> ";
    console_cfg.register_extra = &register_console_commands;
    wifi_console_start(&console_cfg);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
