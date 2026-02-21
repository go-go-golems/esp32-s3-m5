#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_system.h"

#include "http_server.h"
#include "js_console.h"
#include "matrix_engine.h"
#include "mqjs/js_runtime_bridge.h"
#include "wifi_console.h"
#include "wifi_mgr.h"

static const char *TAG = "0067_main";

static void register_extra_commands(void)
{
    extern void matrix_console_register_commands(void);
    matrix_console_register_commands();
    js_console_register_commands();
}

static void on_wifi_got_ip(uint32_t ip4_host_order, void *ctx)
{
    (void)ip4_host_order;
    (void)ctx;
    (void)http_server_start();
}

void app_main(void)
{
    ESP_LOGI(TAG, "boot: reset_reason=%d", (int)esp_reset_reason());

    matrix_engine_config_t cfg = {
        .pin_mosi = CONFIG_TUTORIAL_0067_MATRIX_PIN_MOSI,
        .pin_sck = CONFIG_TUTORIAL_0067_MATRIX_PIN_SCK,
        .pin_cs = CONFIG_TUTORIAL_0067_MATRIX_PIN_CS,
        .chain_len = CONFIG_TUTORIAL_0067_MATRIX_CHAIN_LEN,
        .spi_hz = CONFIG_TUTORIAL_0067_MATRIX_SPI_HZ,
        .default_fps = CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FPS,
    };
    ESP_ERROR_CHECK(matrix_engine_init(&cfg));
    ESP_ERROR_CHECK(js_service_start());

    wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, NULL);
    ESP_ERROR_CHECK(wifi_mgr_start());

    const wifi_console_config_t console_cfg = {
        .prompt = "c3m> ",
        .register_extra = register_extra_commands,
    };
    wifi_console_start(&console_cfg);
}
