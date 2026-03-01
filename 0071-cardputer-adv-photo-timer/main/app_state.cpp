#include "app_state.h"

#include <string.h>

#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "timer_engine.h"

namespace {

SemaphoreHandle_t s_mu = nullptr;
TimerConfig s_cfg;
uint32_t s_revision = 0;
TimerEngine s_engine;

const TimerPreset* active_preset_locked() {
  return find_preset_by_id(s_cfg, s_cfg.active_preset_id);
}

void bind_active_locked() {
  const TimerPreset* preset = active_preset_locked();
  if (!preset && !s_cfg.presets.empty()) {
    s_cfg.active_preset_id = s_cfg.presets[0].id;
    preset = &s_cfg.presets[0];
  }
  if (preset) {
    (void)s_engine.bind_preset(preset);
  }
}

}  // namespace

esp_err_t app_state_init(const TimerConfig* cfg) {
  if (!cfg) return ESP_ERR_INVALID_ARG;
  if (!s_mu) {
    s_mu = xSemaphoreCreateMutex();
  }
  if (!s_mu) return ESP_ERR_NO_MEM;

  xSemaphoreTake(s_mu, portMAX_DELAY);
  s_cfg = *cfg;
  bind_active_locked();
  s_revision++;
  xSemaphoreGive(s_mu);
  return ESP_OK;
}

void app_state_tick(void) {
  s_engine.update((uint64_t)esp_timer_get_time());
}

TimerSnapshot app_state_snapshot(void) {
  return s_engine.snapshot();
}

TimerConfig app_state_config_copy(void) {
  TimerConfig out;
  if (!s_mu) return out;
  xSemaphoreTake(s_mu, portMAX_DELAY);
  out = s_cfg;
  xSemaphoreGive(s_mu);
  return out;
}

uint32_t app_state_config_revision(void) {
  if (!s_mu) return 0;
  xSemaphoreTake(s_mu, portMAX_DELAY);
  const uint32_t out = s_revision;
  xSemaphoreGive(s_mu);
  return out;
}

esp_err_t app_state_replace_config(const TimerConfig* cfg) {
  if (!cfg || cfg->presets.empty()) return ESP_ERR_INVALID_ARG;
  if (!s_mu) return ESP_ERR_INVALID_STATE;

  xSemaphoreTake(s_mu, portMAX_DELAY);
  s_cfg = *cfg;
  bind_active_locked();
  s_revision++;
  xSemaphoreGive(s_mu);
  return ESP_OK;
}

esp_err_t app_state_timer_action(const char* action, const char* preset_id) {
  if (!action) return ESP_ERR_INVALID_ARG;

  if (strcmp(action, "select") == 0) {
    if (!preset_id || !s_mu) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s_mu, portMAX_DELAY);
    const TimerPreset* preset = find_preset_by_id(s_cfg, preset_id);
    if (!preset) {
      xSemaphoreGive(s_mu);
      return ESP_ERR_NOT_FOUND;
    }

    s_cfg.active_preset_id = preset_id;
    (void)s_engine.bind_preset(preset);
    s_revision++;
    xSemaphoreGive(s_mu);
    return ESP_OK;
  }

  if (strcmp(action, "start") == 0) {
    s_engine.start();
    return ESP_OK;
  }
  if (strcmp(action, "pause") == 0) {
    s_engine.pause();
    return ESP_OK;
  }
  if (strcmp(action, "resume") == 0) {
    s_engine.resume();
    return ESP_OK;
  }
  if (strcmp(action, "toggle") == 0) {
    s_engine.toggle_run_pause();
    return ESP_OK;
  }
  if (strcmp(action, "next") == 0) {
    s_engine.next_step();
    return ESP_OK;
  }
  if (strcmp(action, "reset") == 0) {
    s_engine.reset();
    return ESP_OK;
  }

  return ESP_ERR_INVALID_ARG;
}
