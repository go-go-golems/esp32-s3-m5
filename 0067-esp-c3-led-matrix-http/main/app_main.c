#include "sdkconfig.h"

#include <stdio.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"

#include "http_server.h"
#include "js_console.h"
#include "matrix_engine.h"
#include "mqjs/js_runtime_bridge.h"
#include "wifi_console.h"
#include "wifi_mgr.h"

static const char *TAG = "0067_main";

#if CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FLIPH
#define MATRIX_CFG_DEFAULT_FLIPH true
#else
#define MATRIX_CFG_DEFAULT_FLIPH false
#endif

#if CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FLIPV
#define MATRIX_CFG_DEFAULT_FLIPV true
#else
#define MATRIX_CFG_DEFAULT_FLIPV false
#endif

static void register_extra_commands(void)
{
    extern void matrix_console_register_commands(void);
    matrix_console_register_commands();
    js_console_register_commands();
}

static void on_wifi_got_ip(uint32_t ip4_host_order, void *ctx)
{
    (void)ctx;
    const uint8_t a = (uint8_t)((ip4_host_order >> 24) & 0xFFu);
    const uint8_t b = (uint8_t)((ip4_host_order >> 16) & 0xFFu);
    const uint8_t c = (uint8_t)((ip4_host_order >> 8) & 0xFFu);
    const uint8_t d = (uint8_t)(ip4_host_order & 0xFFu);
    char ip_text[40];
    snprintf(ip_text, sizeof(ip_text), "IP %u.%u.%u.%u", a, b, c, d);

    (void)matrix_engine_show_boot_ip(ip_text, 14, 300, MATRIX_SCROLL_LOOP_WRAP);
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
        .default_fliph = MATRIX_CFG_DEFAULT_FLIPH,
        .default_flipv = MATRIX_CFG_DEFAULT_FLIPV,
    };
    ESP_ERROR_CHECK(matrix_engine_init(&cfg));
    (void)matrix_engine_play_boot_animation();
    ESP_ERROR_CHECK(js_service_start());

    wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, NULL);
    ESP_ERROR_CHECK(wifi_mgr_start());

    const wifi_console_config_t console_cfg = {
        .prompt = "c3m> ",
        .register_extra = register_extra_commands,
    };
    wifi_console_start(&console_cfg);
}
