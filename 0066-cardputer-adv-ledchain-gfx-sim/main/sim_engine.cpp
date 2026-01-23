#include "sim_engine.h"

#include <string.h>

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "sim_engine";

namespace {

enum sim_cmd_type_t : uint8_t {
  SIM_CMD_SET_CFG = 1,
  SIM_CMD_SET_FRAME_MS = 2,
};

struct sim_cmd_t {
  sim_cmd_type_t type = SIM_CMD_SET_CFG;
  led_pattern_cfg_t cfg = {};
  uint32_t frame_ms = 16;
};

static constexpr uint32_t kEngineTickMs = 16;  // ~60 Hz (approx; acceptable jitter per requirements)

static void engine_task_main(void *arg) {
  auto *e = static_cast<sim_engine_t *>(arg);
  if (!e) {
    vTaskDelete(nullptr);
    return;
  }

  led_patterns_t patterns = {};
  led_ws281x_t strip = {};

  // Allocate an internal virtual strip pixel buffer (owned by the engine task).
  const size_t nbytes = (size_t)e->led_count * 3;
  uint8_t *work = (uint8_t *)heap_caps_malloc(nbytes, MALLOC_CAP_DEFAULT);
  if (!work) {
    ESP_LOGE(TAG, "engine: malloc work pixels failed (%u bytes)", (unsigned)nbytes);
    vTaskDelete(nullptr);
    return;
  }
  memset(work, 0, nbytes);

  strip.cfg = (led_ws281x_cfg_t){
      .gpio_num = -1,
      .led_count = e->led_count,
      .order = e->order,
      .resolution_hz = 0,
      .t0h_ns = 0,
      .t0l_ns = 0,
      .t1h_ns = 0,
      .t1l_ns = 0,
      .reset_us = 0,
  };
  strip.chan = nullptr;
  strip.encoder = nullptr;
  strip.pixels = work;

  if (led_patterns_init(&patterns, e->led_count) != ESP_OK) {
    ESP_LOGE(TAG, "engine: led_patterns_init failed");
    free(work);
    vTaskDelete(nullptr);
    return;
  }

  // Seed state from shared snapshot (initial config set by sim_engine_init).
  led_pattern_cfg_t cfg = {};
  uint32_t frame_ms = 16;
  if (e->mu && xSemaphoreTake(e->mu, portMAX_DELAY) == pdTRUE) {
    cfg = e->cfg;
    frame_ms = e->frame_ms;
    xSemaphoreGive(e->mu);
  } else {
    cfg.type = LED_PATTERN_RAINBOW;
    cfg.global_brightness_pct = 25;
  }
  led_patterns_set_cfg(&patterns, &cfg);

  TickType_t last = xTaskGetTickCount();
  for (;;) {
    // Drain control queue (non-blocking).
    sim_cmd_t cmd = {};
    while (e->q && xQueueReceive(e->q, &cmd, 0) == pdTRUE) {
      if (cmd.type == SIM_CMD_SET_CFG) {
        cfg = cmd.cfg;
        led_patterns_set_cfg(&patterns, &cfg);
      } else if (cmd.type == SIM_CMD_SET_FRAME_MS) {
        frame_ms = cmd.frame_ms;
        if (frame_ms == 0) frame_ms = 1;
      }
    }

    const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    led_patterns_render_to_ws281x(&patterns, now_ms, &strip);

    // Publish snapshot.
    if (e->mu && xSemaphoreTake(e->mu, portMAX_DELAY) == pdTRUE) {
      const int next = (e->active_frame == 0) ? 1 : 0;
      if (e->frames[next]) {
        memcpy(e->frames[next], work, nbytes);
        e->active_frame = next;
        e->frame_seq++;
        e->last_render_ms = now_ms;
      }
      e->cfg = cfg;
      e->frame_ms = frame_ms;
      xSemaphoreGive(e->mu);
    }

    vTaskDelayUntil(&last, pdMS_TO_TICKS(kEngineTickMs));
  }
}

}  // namespace

esp_err_t sim_engine_init(sim_engine_t *e, uint16_t led_count, uint32_t frame_ms) {
  ESP_RETURN_ON_FALSE(e && led_count > 0, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
  memset(e, 0, sizeof(*e));

  e->mu = xSemaphoreCreateMutexStatic(&e->mu_buf);
  ESP_RETURN_ON_FALSE(e->mu, ESP_ERR_NO_MEM, TAG, "mutex init failed");

  e->led_count = led_count;
  e->order = LED_WS281X_ORDER_GRB;

  if (frame_ms == 0) frame_ms = 1;
  e->frame_ms = frame_ms;

  const size_t nbytes = (size_t)led_count * 3;
  e->frames[0] = (uint8_t *)heap_caps_malloc(nbytes, MALLOC_CAP_DEFAULT);
  e->frames[1] = (uint8_t *)heap_caps_malloc(nbytes, MALLOC_CAP_DEFAULT);
  ESP_RETURN_ON_FALSE(e->frames[0] && e->frames[1], ESP_ERR_NO_MEM, TAG, "malloc frames failed");
  memset(e->frames[0], 0, nbytes);
  memset(e->frames[1], 0, nbytes);
  e->active_frame = 0;
  e->frame_seq = 0;

  e->cfg = (led_pattern_cfg_t){
      .type = LED_PATTERN_RAINBOW,
      .global_brightness_pct = 25,
      .u =
          {
              .rainbow =
                  {
                      .speed = 5,
                      .saturation = 100,
                      .spread_x10 = 10,
                  },
          },
  };

  e->q = xQueueCreate(16, sizeof(sim_cmd_t));
  ESP_RETURN_ON_FALSE(e->q, ESP_ERR_NO_MEM, TAG, "queue init failed");

  BaseType_t ok = xTaskCreate(&engine_task_main, "0066_engine", 6144, e, 8, &e->task);
  ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "task create failed");

  return ESP_OK;
}

void sim_engine_deinit(sim_engine_t *e) {
  if (!e) return;

  if (e->task) {
    vTaskDelete(e->task);
    e->task = nullptr;
  }
  if (e->q) {
    vQueueDelete(e->q);
    e->q = nullptr;
  }

  if (e->frames[0]) {
    free(e->frames[0]);
    e->frames[0] = nullptr;
  }
  if (e->frames[1]) {
    free(e->frames[1]);
    e->frames[1] = nullptr;
  }

  *e = sim_engine_t{};
}

void sim_engine_get_cfg(sim_engine_t *e, led_pattern_cfg_t *out) {
  if (!e || !out || !e->mu) return;
  if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) return;
  *out = e->cfg;
  xSemaphoreGive(e->mu);
}

void sim_engine_set_cfg(sim_engine_t *e, const led_pattern_cfg_t *cfg) {
  if (!e || !cfg || !e->q) return;
  sim_cmd_t cmd = {};
  cmd.type = SIM_CMD_SET_CFG;
  cmd.cfg = *cfg;
  (void)xQueueSend(e->q, &cmd, pdMS_TO_TICKS(50));
}

uint32_t sim_engine_get_frame_ms(sim_engine_t *e) {
  if (!e || !e->mu) return 16;
  if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) return 16;
  const uint32_t v = e->frame_ms;
  xSemaphoreGive(e->mu);
  return v;
}

void sim_engine_set_frame_ms(sim_engine_t *e, uint32_t frame_ms) {
  if (!e || !e->q) return;
  if (frame_ms == 0) frame_ms = 1;
  sim_cmd_t cmd = {};
  cmd.type = SIM_CMD_SET_FRAME_MS;
  cmd.frame_ms = frame_ms;
  (void)xQueueSend(e->q, &cmd, pdMS_TO_TICKS(50));
}

uint16_t sim_engine_get_led_count(sim_engine_t *e) {
  if (!e) return 0;
  return e->led_count;
}

led_ws281x_color_order_t sim_engine_get_order(sim_engine_t *e) {
  if (!e) return LED_WS281X_ORDER_GRB;
  return e->order;
}

esp_err_t sim_engine_copy_latest_pixels(sim_engine_t *e, uint8_t *out_pixels, size_t out_len) {
  ESP_RETURN_ON_FALSE(e && e->mu && out_pixels, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
  const size_t need = (size_t)e->led_count * 3;
  ESP_RETURN_ON_FALSE(out_len >= need, ESP_ERR_INVALID_SIZE, TAG, "short out");

  if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) return ESP_FAIL;
  const int idx = (e->active_frame == 0) ? 0 : 1;
  if (!e->frames[idx]) {
    xSemaphoreGive(e->mu);
    return ESP_ERR_INVALID_STATE;
  }
  memcpy(out_pixels, e->frames[idx], need);
  xSemaphoreGive(e->mu);
  return ESP_OK;
}

