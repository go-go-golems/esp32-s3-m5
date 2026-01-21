#include "mled_effect_led.h"

#include <string.h>

#include "sdkconfig.h"

#include "esp_log.h"

#include "led_task.h"
#include "led_ws281x.h"
#include "mled_node.h"

static const char *TAG = "mled_effect_led";

static uint8_t clamp_u8(uint8_t v, uint8_t lo, uint8_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static led_rgb8_t rgb3(const uint8_t *d)
{
    return (led_rgb8_t){
        .r = d[0],
        .g = d[1],
        .b = d[2],
    };
}

static void map_pattern_cfg(led_pattern_cfg_t *out, const mled_pattern_config_t *p)
{
    memset(out, 0, sizeof(*out));
    out->global_brightness_pct = clamp_u8(p->brightness_pct ? p->brightness_pct : 1, 1, 100);

    switch ((mled_pattern_type_t)p->pattern_type) {
    case MLED_PATTERN_OFF:
        out->type = LED_PATTERN_OFF;
        break;
    case MLED_PATTERN_RAINBOW:
        out->type = LED_PATTERN_RAINBOW;
        out->u.rainbow.speed = clamp_u8(p->data[0], 0, 20);
        out->u.rainbow.saturation = clamp_u8(p->data[1], 0, 100);
        out->u.rainbow.spread_x10 = clamp_u8(p->data[2] ? p->data[2] : 1, 1, 50);
        break;
    case MLED_PATTERN_CHASE:
        out->type = LED_PATTERN_CHASE;
        out->u.chase.speed = p->data[0];
        out->u.chase.tail_len = p->data[1] ? p->data[1] : 1;
        out->u.chase.gap_len = p->data[2];
        out->u.chase.trains = p->data[3] ? p->data[3] : 1;
        out->u.chase.fg = rgb3(&p->data[4]);
        out->u.chase.bg = rgb3(&p->data[7]);
        out->u.chase.dir = (led_direction_t)clamp_u8(p->data[10], 0, 2);
        out->u.chase.fade_tail = p->data[11] ? true : false;
        break;
    case MLED_PATTERN_BREATHING: {
        out->type = LED_PATTERN_BREATHING;
        out->u.breathing.speed = clamp_u8(p->data[0], 0, 20);
        out->u.breathing.color = rgb3(&p->data[1]);
        const uint8_t min_bri = p->data[4];
        const uint8_t max_bri = p->data[5];
        out->u.breathing.min_bri = (min_bri <= max_bri) ? min_bri : max_bri;
        out->u.breathing.max_bri = (min_bri <= max_bri) ? max_bri : min_bri;
        out->u.breathing.curve = (led_curve_t)clamp_u8(p->data[6], 0, 2);
        break;
    }
    case MLED_PATTERN_SPARKLE:
        out->type = LED_PATTERN_SPARKLE;
        out->u.sparkle.speed = clamp_u8(p->data[0], 0, 20);
        out->u.sparkle.color = rgb3(&p->data[1]);
        out->u.sparkle.density_pct = clamp_u8(p->data[4], 0, 100);
        out->u.sparkle.fade_speed = p->data[5] ? p->data[5] : 1;
        out->u.sparkle.color_mode = (led_sparkle_color_mode_t)clamp_u8(p->data[6], 0, 2);
        out->u.sparkle.background = rgb3(&p->data[7]);
        break;
    default:
        out->type = LED_PATTERN_OFF;
        break;
    }
}

void mled_effect_led_start(void)
{
    led_ws281x_cfg_t ws_cfg = {
        .gpio_num = CONFIG_MLEDNODE_WS281X_GPIO_NUM,
        .led_count = (uint16_t)CONFIG_MLEDNODE_WS281X_LED_COUNT,
        .order = LED_WS281X_ORDER_GRB,
        .resolution_hz = 10000000u,
        .t0h_ns = (uint32_t)CONFIG_MLEDNODE_WS281X_T0H_NS,
        .t0l_ns = (uint32_t)CONFIG_MLEDNODE_WS281X_T0L_NS,
        .t1h_ns = (uint32_t)CONFIG_MLEDNODE_WS281X_T1H_NS,
        .t1l_ns = (uint32_t)CONFIG_MLEDNODE_WS281X_T1L_NS,
        .reset_us = (uint32_t)CONFIG_MLEDNODE_WS281X_RESET_US,
    };

    led_pattern_cfg_t pat_cfg = {
        .type = LED_PATTERN_OFF,
        .global_brightness_pct = 25,
    };

    const uint32_t frame_ms = (uint32_t)CONFIG_MLEDNODE_WS281X_FRAME_MS;
    ESP_LOGI(TAG, "ws: gpio=%d leds=%u frame_ms=%u", ws_cfg.gpio_num, (unsigned)ws_cfg.led_count, (unsigned)frame_ms);
    ESP_ERROR_CHECK(led_task_start(&ws_cfg, &pat_cfg, frame_ms));

    const mled_node_effect_status_t st = {
        .pattern_type = MLED_PATTERN_OFF,
        .brightness_pct = pat_cfg.global_brightness_pct,
        .frame_ms = (uint16_t)frame_ms,
    };
    mled_node_set_effect_status(&st);
}

void mled_effect_led_apply_pattern(const mled_pattern_config_t *p)
{
    if (!p) {
        return;
    }

    led_pattern_cfg_t cfg;
    map_pattern_cfg(&cfg, p);

    // Intentionally INFO-level: this is our end-to-end "did the LED adapter run?" breadcrumb.
    ESP_LOGI(TAG, "apply: type=%u bri=%u%%", (unsigned)p->pattern_type, (unsigned)cfg.global_brightness_pct);

    led_msg_t msg = {
        .type = LED_MSG_SET_PATTERN_CFG,
        .u.pattern = cfg,
    };

    esp_err_t err = led_task_send(&msg, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "led_task_send failed: %s", esp_err_to_name(err));
        return;
    }

    const mled_node_effect_status_t st = {
        .pattern_type = p->pattern_type,
        .brightness_pct = cfg.global_brightness_pct,
        .frame_ms = (uint16_t)CONFIG_MLEDNODE_WS281X_FRAME_MS,
    };
    mled_node_set_effect_status(&st);
}
