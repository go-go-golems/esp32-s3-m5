#include "sdkconfig.h"

#include "oled_status.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"

#include "wifi_mgr.h"

#if CONFIG_MO065_OLED_ENABLE

#include "driver/i2c_master.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_vendor.h"
#include "lwip/inet.h"

#include "oled_font_5x7.h"

static const char *TAG = "mo065_oled";

static i2c_master_bus_handle_t s_bus = NULL;
static esp_lcd_panel_io_handle_t s_io = NULL;
static esp_lcd_panel_handle_t s_panel = NULL;

static uint8_t s_fb[128 * 64 / 8];

static inline void fb_clear(void)
{
    memset(s_fb, 0x00, sizeof(s_fb));
}

static inline void fb_set_pixel(int x, int y, bool on)
{
    if ((unsigned)x >= 128u || (unsigned)y >= 64u) return;
    const int page = y / 8;
    const int idx = page * 128 + x;
    const uint8_t bit = (uint8_t)(1u << (y & 7));
    if (on) {
        s_fb[idx] |= bit;
    } else {
        s_fb[idx] &= (uint8_t)~bit;
    }
}

static void fb_draw_char_5x7(int x, int y, char c)
{
    const uint8_t *cols = oled_font5x7_cols(c);
    for (int col = 0; col < 5; col++) {
        const uint8_t bits = cols[col];
        for (int row = 0; row < 7; row++) {
            const bool on = (bits & (uint8_t)(1u << row)) != 0;
            fb_set_pixel(x + col, y + row, on);
        }
    }
}

static void fb_draw_text(int x, int y, const char *s, int max_chars)
{
    if (!s) return;
    int cx = x;
    int n = 0;
    while (*s && (max_chars < 0 || n < max_chars)) {
        fb_draw_char_5x7(cx, y, *s++);
        cx += 6;  // 5px glyph + 1px spacing
        n++;
    }
}

static const char *wifi_state_to_str(wifi_mgr_state_t st)
{
    switch (st) {
        case WIFI_MGR_STATE_UNINIT:
            return "UNINIT";
        case WIFI_MGR_STATE_IDLE:
            return "IDLE";
        case WIFI_MGR_STATE_CONNECTING:
            return "CONNECTING";
        case WIFI_MGR_STATE_CONNECTED:
            return "CONNECTED";
        default:
            return "?";
    }
}

static void render_status(const wifi_mgr_status_t *st)
{
    fb_clear();

    char line1[32];
    snprintf(line1, sizeof(line1), "WIFI:%s", wifi_state_to_str(st->state));

    char line2[32];
    if (st->ssid[0]) {
        snprintf(line2, sizeof(line2), "SSID:%s", st->ssid);
    } else {
        snprintf(line2, sizeof(line2), "SSID:-");
    }

    char ipbuf[20];
    if (st->ip4) {
        ip4_addr_t ip = {.addr = htonl(st->ip4)};
        ip4addr_ntoa_r(&ip, ipbuf, sizeof(ipbuf));
    } else {
        strcpy(ipbuf, "-");
    }

    char line3[32];
    snprintf(line3, sizeof(line3), "IP:%s", ipbuf);

    // 128px wide, 6px per char (5+1) => ~21 chars.
    fb_draw_text(0, 0, line1, 21);
    fb_draw_text(0, 16, line2, 21);
    fb_draw_text(0, 32, line3, 21);
}

static void probe_i2c_addrs(void)
{
#if CONFIG_MO065_OLED_SCAN_ON_BOOT
    for (int addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_master_probe(s_bus, (uint16_t)addr, 50) == ESP_OK) {
            ESP_LOGI(TAG, "I2C ACK at 0x%02X", addr);
        }
    }
#endif
}

static esp_err_t oled_init(void)
{
    if (s_panel) return ESP_OK;

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .sda_io_num = (gpio_num_t)CONFIG_MO065_OLED_I2C_SDA_GPIO_NUM,
        .scl_io_num = (gpio_num_t)CONFIG_MO065_OLED_I2C_SCL_GPIO_NUM,
        .flags.enable_internal_pullup = CONFIG_MO065_OLED_I2C_INTERNAL_PULLUPS ? 1 : 0,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "i2c_new_master_bus");

    probe_i2c_addrs();

    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr = (uint8_t)CONFIG_MO065_OLED_I2C_ADDR,
        .scl_speed_hz = CONFIG_MO065_OLED_I2C_SPEED_HZ,
        .control_phase_bytes = 1,  // per SSD1306 datasheet
        .dc_bit_offset = 6,        // per SSD1306 datasheet
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_bus, &io_cfg, &s_io), TAG, "new_panel_io_i2c");

    esp_lcd_panel_ssd1306_config_t ssd1306_cfg = {
        .height = 64,
    };
    esp_lcd_panel_dev_config_t panel_cfg = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
        .vendor_config = &ssd1306_cfg,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ssd1306(s_io, &panel_cfg, &s_panel), TAG, "new_panel_ssd1306");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "panel_reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel_init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "disp_on");

    fb_clear();
    return ESP_OK;
}

static void oled_task(void *arg)
{
    (void)arg;

    wifi_mgr_status_t prev = {};
    bool have_prev = false;

    while (true) {
        wifi_mgr_status_t st = {};
        if (wifi_mgr_get_status(&st) == ESP_OK) {
            const bool changed =
                !have_prev || (st.state != prev.state) || (st.ip4 != prev.ip4) || (strcmp(st.ssid, prev.ssid) != 0);

            if (changed) {
                render_status(&st);
                (void)esp_lcd_panel_draw_bitmap(s_panel, 0, 0, 128, 64, s_fb);
                prev = st;
                have_prev = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(CONFIG_MO065_OLED_REFRESH_MS));
    }
}

esp_err_t oled_status_start(void)
{
    ESP_LOGI(TAG,
             "OLED config: sda=%d scl=%d addr=0x%02X speed=%dHz internal_pullups=%s scan_on_boot=%s refresh=%dms",
             CONFIG_MO065_OLED_I2C_SDA_GPIO_NUM,
             CONFIG_MO065_OLED_I2C_SCL_GPIO_NUM,
             CONFIG_MO065_OLED_I2C_ADDR,
             CONFIG_MO065_OLED_I2C_SPEED_HZ,
             CONFIG_MO065_OLED_I2C_INTERNAL_PULLUPS ? "yes" : "no",
             CONFIG_MO065_OLED_SCAN_ON_BOOT ? "yes" : "no",
             CONFIG_MO065_OLED_REFRESH_MS);

    esp_err_t err = oled_init();
    if (err != ESP_OK) return err;

    static bool started = false;
    if (!started) {
        started = true;
        if (xTaskCreate(oled_task, "oled", 4096, NULL, 5, NULL) != pdPASS) {
            ESP_LOGE(TAG, "xTaskCreate(oled) failed");
            return ESP_ERR_NO_MEM;
        }
        ESP_LOGI(TAG, "OLED task started");
    }
    return ESP_OK;
}

#else  // CONFIG_MO065_OLED_ENABLE

esp_err_t oled_status_start(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

#endif
