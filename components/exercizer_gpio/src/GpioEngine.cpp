#include "exercizer/GpioEngine.h"

#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "exercizer_gpio";

void GpioEngine::EnsureTimer(esp_timer_handle_t *h, const char *name, esp_timer_cb_t cb, void *arg) {
  if (!h || *h) {
    return;
  }
  esp_timer_create_args_t args = {};
  args.callback = cb;
  args.arg = arg;
  args.dispatch_method = ESP_TIMER_TASK;
  args.name = name;
  ESP_ERROR_CHECK(esp_timer_create(&args, h));
}

GpioEngine::Channel *GpioEngine::FindChannel(uint8_t pin) {
  for (auto &ch : channels_) {
    if (ch.pin != GPIO_NUM_NC && ch.pin == static_cast<gpio_num_t>(pin)) {
      return &ch;
    }
  }
  return nullptr;
}

GpioEngine::Channel *GpioEngine::AllocChannel(uint8_t pin) {
  if (pin == EXERCIZER_GPIO_STOP_ALL) {
    return nullptr;
  }
  if (auto *existing = FindChannel(pin)) {
    return existing;
  }
  for (auto &ch : channels_) {
    if (ch.pin == GPIO_NUM_NC) {
      ch.pin = static_cast<gpio_num_t>(pin);
      return &ch;
    }
  }
  return nullptr;
}

void GpioEngine::StopChannel(Channel *ch, bool release) {
  if (!ch) {
    return;
  }
  if (ch->square_timer) {
    (void)esp_timer_stop(ch->square_timer);
  }
  if (ch->pulse_periodic) {
    (void)esp_timer_stop(ch->pulse_periodic);
  }
  if (ch->pulse_low_oneshot) {
    (void)esp_timer_stop(ch->pulse_low_oneshot);
  }
  if (ch->pin != GPIO_NUM_NC) {
    gpio_set_level(ch->pin, 0);
    gpio_set_direction(ch->pin, GPIO_MODE_INPUT);
    (void)gpio_pullup_dis(ch->pin);
    (void)gpio_pulldown_dis(ch->pin);
    ESP_LOGI(TAG, "gpio stop: pin=%d", (int)ch->pin);
  }
  if (release) {
    ch->pin = GPIO_NUM_NC;
  }
  memset(&ch->cfg, 0, sizeof(ch->cfg));
  __atomic_store_n(&ch->level, 0, __ATOMIC_RELAXED);
}

void GpioEngine::StopAll() {
  for (auto &ch : channels_) {
    if (ch.pin != GPIO_NUM_NC) {
      StopChannel(&ch, true);
    }
  }
}

void GpioEngine::StopPin(uint8_t pin) {
  if (pin == EXERCIZER_GPIO_STOP_ALL) {
    StopAll();
    return;
  }
  auto *ch = FindChannel(pin);
  if (!ch) {
    ESP_LOGW(TAG, "gpio stop: pin=%d not active", (int)pin);
    return;
  }
  StopChannel(ch, true);
}

void GpioEngine::SquareCb(void *arg) {
  auto *ch = static_cast<Channel *>(arg);
  if (!ch || ch->pin == GPIO_NUM_NC) {
    return;
  }
  int lvl = __atomic_load_n(&ch->level, __ATOMIC_RELAXED);
  lvl = lvl ? 0 : 1;
  __atomic_store_n(&ch->level, lvl, __ATOMIC_RELAXED);
  gpio_set_level(ch->pin, lvl);
}

void GpioEngine::PulseLowCb(void *arg) {
  auto *ch = static_cast<Channel *>(arg);
  if (!ch || ch->pin == GPIO_NUM_NC) {
    return;
  }
  gpio_set_level(ch->pin, 0);
  __atomic_store_n(&ch->level, 0, __ATOMIC_RELAXED);
}

void GpioEngine::PulsePeriodicCb(void *arg) {
  auto *ch = static_cast<Channel *>(arg);
  if (!ch || ch->pin == GPIO_NUM_NC) {
    return;
  }
  gpio_set_level(ch->pin, 1);
  __atomic_store_n(&ch->level, 1, __ATOMIC_RELAXED);
  if (ch->pulse_low_oneshot && ch->cfg.width_us > 0) {
    (void)esp_timer_stop(ch->pulse_low_oneshot);
    (void)esp_timer_start_once(ch->pulse_low_oneshot, (uint64_t)ch->cfg.width_us);
  }
}

void GpioEngine::ApplyConfig(Channel *ch, const exercizer_gpio_config_t &cfg) {
  if (!ch || ch->pin == GPIO_NUM_NC) {
    return;
  }

  StopChannel(ch, false);
  ch->cfg = cfg;

  gpio_reset_pin(ch->pin);
  gpio_set_direction(ch->pin, GPIO_MODE_OUTPUT);
  (void)gpio_pullup_dis(ch->pin);
  (void)gpio_pulldown_dis(ch->pin);

  if (cfg.mode == EXERCIZER_GPIO_MODE_HIGH) {
    __atomic_store_n(&ch->level, 1, __ATOMIC_RELAXED);
    gpio_set_level(ch->pin, 1);
    ESP_LOGI(TAG, "gpio high: pin=%d", (int)ch->pin);
    return;
  }

  if (cfg.mode == EXERCIZER_GPIO_MODE_LOW) {
    __atomic_store_n(&ch->level, 0, __ATOMIC_RELAXED);
    gpio_set_level(ch->pin, 0);
    ESP_LOGI(TAG, "gpio low: pin=%d", (int)ch->pin);
    return;
  }

  if (cfg.mode == EXERCIZER_GPIO_MODE_SQUARE) {
    if (cfg.hz == 0) {
      ESP_LOGW(TAG, "gpio square invalid hz=0 (stopping)");
      StopChannel(ch, true);
      return;
    }
    EnsureTimer(&ch->square_timer, "gpio_sq", &GpioEngine::SquareCb, ch);
    const uint64_t half_period_us = 500000ULL / (uint64_t)cfg.hz;
    __atomic_store_n(&ch->level, 0, __ATOMIC_RELAXED);
    gpio_set_level(ch->pin, 0);
    ESP_ERROR_CHECK(esp_timer_start_periodic(ch->square_timer, half_period_us > 0 ? half_period_us : 1));
    ESP_LOGI(TAG, "gpio square: pin=%d hz=%" PRIu32, (int)ch->pin, cfg.hz);
    return;
  }

  if (cfg.mode == EXERCIZER_GPIO_MODE_PULSE) {
    if (cfg.width_us == 0 || cfg.period_ms == 0) {
      ESP_LOGW(TAG, "gpio pulse invalid width_us=%" PRIu32 " period_ms=%" PRIu32 " (stopping)",
               cfg.width_us,
               cfg.period_ms);
      StopChannel(ch, true);
      return;
    }
    EnsureTimer(&ch->pulse_periodic, "gpio_pulse", &GpioEngine::PulsePeriodicCb, ch);
    EnsureTimer(&ch->pulse_low_oneshot, "gpio_pulse_lo", &GpioEngine::PulseLowCb, ch);
    __atomic_store_n(&ch->level, 0, __ATOMIC_RELAXED);
    gpio_set_level(ch->pin, 0);
    ESP_ERROR_CHECK(esp_timer_start_periodic(ch->pulse_periodic, (uint64_t)cfg.period_ms * 1000ULL));
    ESP_LOGI(TAG, "gpio pulse: pin=%d width_us=%" PRIu32 " period_ms=%" PRIu32,
             (int)ch->pin,
             cfg.width_us,
             cfg.period_ms);
    return;
  }

  StopChannel(ch, true);
}

void GpioEngine::HandleEvent(const exercizer_ctrl_event_t &ev) {
  if (ev.type == EXERCIZER_CTRL_GPIO_SET) {
    exercizer_gpio_config_t cfg = {};
    if (ev.data_len < sizeof(cfg)) {
      ESP_LOGW(TAG, "gpio config too small (len=%u)", ev.data_len);
      return;
    }
    memcpy(&cfg, ev.data, sizeof(cfg));
    auto *ch = AllocChannel(cfg.pin);
    if (!ch) {
      ESP_LOGW(TAG, "gpio config dropped (no channel free) pin=%d", (int)cfg.pin);
      return;
    }
    ApplyConfig(ch, cfg);
    return;
  }

  if (ev.type == EXERCIZER_CTRL_GPIO_STOP) {
    if (ev.data_len >= sizeof(exercizer_gpio_stop_t)) {
      exercizer_gpio_stop_t stop = {};
      memcpy(&stop, ev.data, sizeof(stop));
      if (stop.pin == EXERCIZER_GPIO_STOP_ALL) {
        StopAll();
      } else {
        StopPin(stop.pin);
      }
      return;
    }
    StopAll();
  }
}
