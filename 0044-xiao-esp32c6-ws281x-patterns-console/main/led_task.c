#include "led_task.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "led_task";

typedef struct {
    QueueHandle_t q;
    TaskHandle_t task;

    led_ws281x_t strip;
    led_ws281x_cfg_t ws_cfg_applied;
    led_ws281x_cfg_t ws_cfg_staged;

    led_patterns_t patterns;
    led_pattern_cfg_t pat_cfg;

    uint32_t frame_ms;
    bool paused;
    bool running;
} led_task_ctx_t;

static led_task_ctx_t s_ctx = {};
static led_status_t s_status = {};
static StaticSemaphore_t s_status_mux_buf;
static SemaphoreHandle_t s_status_mux;

static void status_mux_init_once(void)
{
    if (!s_status_mux) {
        s_status_mux = xSemaphoreCreateMutexStatic(&s_status_mux_buf);
    }
}

static void snapshot_status(led_status_t *out)
{
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));
    if (!s_status_mux) {
        return;
    }
    if (xSemaphoreTake(s_status_mux, 0) != pdTRUE) {
        return;
    }
    *out = s_status;
    xSemaphoreGive(s_status_mux);
}

void led_task_get_status(led_status_t *out)
{
    snapshot_status(out);
}

static void apply_msg(led_task_ctx_t *ctx, const led_msg_t *m)
{
    switch (m->type) {
    case LED_MSG_SET_PATTERN_CFG:
        ctx->pat_cfg = m->u.pattern;
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);
        break;
    case LED_MSG_SET_PATTERN_TYPE:
        ctx->pat_cfg.type = m->u.pattern_type;
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);
        break;
    case LED_MSG_SET_GLOBAL_BRIGHTNESS_PCT:
        ctx->pat_cfg.global_brightness_pct = m->u.brightness_pct ? m->u.brightness_pct : 1;
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);
        break;
    case LED_MSG_SET_FRAME_MS:
        ctx->frame_ms = (m->u.frame_ms == 0) ? 1 : m->u.frame_ms;
        break;

    case LED_MSG_SET_RAINBOW:
        ctx->pat_cfg.type = LED_PATTERN_RAINBOW;
        ctx->pat_cfg.u.rainbow = m->u.rainbow;
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);
        break;
    case LED_MSG_SET_CHASE:
        ctx->pat_cfg.type = LED_PATTERN_CHASE;
        ctx->pat_cfg.u.chase = m->u.chase;
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);
        break;
    case LED_MSG_SET_BREATHING:
        ctx->pat_cfg.type = LED_PATTERN_BREATHING;
        ctx->pat_cfg.u.breathing = m->u.breathing;
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);
        break;
    case LED_MSG_SET_SPARKLE:
        ctx->pat_cfg.type = LED_PATTERN_SPARKLE;
        ctx->pat_cfg.u.sparkle = m->u.sparkle;
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);
        break;

    case LED_MSG_WS_SET_CFG:
        ctx->ws_cfg_staged = m->u.ws_cfg;
        break;
    case LED_MSG_WS_APPLY_CFG: {
        const led_ws281x_cfg_t next = ctx->ws_cfg_staged;
        ESP_LOGI(TAG, "ws apply: gpio=%d leds=%u res=%uHz", next.gpio_num, (unsigned)next.led_count, (unsigned)next.resolution_hz);
        led_ws281x_deinit(&ctx->strip);
        ESP_ERROR_CHECK(led_ws281x_init(&ctx->strip, &next));

        led_patterns_deinit(&ctx->patterns);
        ESP_ERROR_CHECK(led_patterns_init(&ctx->patterns, next.led_count));
        led_patterns_set_cfg(&ctx->patterns, &ctx->pat_cfg);

        ctx->ws_cfg_applied = next;
        break;
    }

    case LED_MSG_PAUSE:
        ctx->paused = true;
        break;
    case LED_MSG_RESUME:
        ctx->paused = false;
        break;
    case LED_MSG_CLEAR:
        led_ws281x_clear(&ctx->strip);
        (void)led_ws281x_show(&ctx->strip);
        break;
    default:
        break;
    }
}

static void led_task_main(void *arg)
{
    led_task_ctx_t *ctx = (led_task_ctx_t *)arg;
    ctx->running = true;

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        led_msg_t msg;
        while (xQueueReceive(ctx->q, &msg, 0) == pdTRUE) {
            apply_msg(ctx, &msg);
        }

        if (!ctx->paused) {
            const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
            led_patterns_render_to_ws281x(&ctx->patterns, now_ms, &ctx->strip);
            (void)led_ws281x_show(&ctx->strip);
        }

        if (s_status_mux && xSemaphoreTake(s_status_mux, 0) == pdTRUE) {
            s_status.running = ctx->running;
            s_status.paused = ctx->paused;
            s_status.frame_ms = ctx->frame_ms;
            s_status.ws_cfg = ctx->ws_cfg_applied;
            s_status.pat_cfg = ctx->pat_cfg;
            xSemaphoreGive(s_status_mux);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ctx->frame_ms));
    }
}

esp_err_t led_task_start(const led_ws281x_cfg_t *ws_cfg, const led_pattern_cfg_t *pat_cfg, uint32_t frame_ms)
{
    ESP_RETURN_ON_FALSE(ws_cfg && pat_cfg, ESP_ERR_INVALID_ARG, TAG, "invalid args");
    ESP_RETURN_ON_FALSE(!s_ctx.task, ESP_ERR_INVALID_STATE, TAG, "already started");

    status_mux_init_once();

    s_ctx = (led_task_ctx_t){
        .q = NULL,
        .task = NULL,
        .strip = {},
        .ws_cfg_applied = *ws_cfg,
        .ws_cfg_staged = *ws_cfg,
        .patterns = {},
        .pat_cfg = *pat_cfg,
        .frame_ms = (frame_ms == 0) ? 1 : frame_ms,
        .paused = false,
        .running = false,
    };

    s_ctx.q = xQueueCreate(8, sizeof(led_msg_t));
    ESP_RETURN_ON_FALSE(s_ctx.q, ESP_ERR_NO_MEM, TAG, "xQueueCreate failed");

    ESP_RETURN_ON_ERROR(led_ws281x_init(&s_ctx.strip, &s_ctx.ws_cfg_applied), TAG, "ws init failed");
    ESP_RETURN_ON_ERROR(led_patterns_init(&s_ctx.patterns, s_ctx.ws_cfg_applied.led_count), TAG, "patterns init failed");
    led_patterns_set_cfg(&s_ctx.patterns, &s_ctx.pat_cfg);

    if (s_status_mux && xSemaphoreTake(s_status_mux, 0) == pdTRUE) {
        s_status = (led_status_t){
            .running = false,
            .paused = s_ctx.paused,
            .frame_ms = s_ctx.frame_ms,
            .ws_cfg = s_ctx.ws_cfg_applied,
            .pat_cfg = s_ctx.pat_cfg,
        };
        xSemaphoreGive(s_status_mux);
    }

    BaseType_t ok = xTaskCreate(&led_task_main, "led", 4096, &s_ctx, 5, &s_ctx.task);
    ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_FAIL, TAG, "xTaskCreate failed");

    ESP_LOGI(TAG, "started: frame_ms=%u", (unsigned)s_ctx.frame_ms);
    return ESP_OK;
}

esp_err_t led_task_send(const led_msg_t *msg, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(msg, ESP_ERR_INVALID_ARG, TAG, "msg null");
    ESP_RETURN_ON_FALSE(s_ctx.q, ESP_ERR_INVALID_STATE, TAG, "not started");

    const TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    if (xQueueSend(s_ctx.q, msg, ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}
