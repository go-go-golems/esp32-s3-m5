#include "exercizer/GpioEngine.h"

#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "exercizer_gpio";

void GpioEngine::EnsureTimer(esp_timer_handle_t *h, const char *name, esp_timer_cb_t cb) {
  if (!h || *h) {
    return;
  }
  esp_timer_create_args_t args = {};
  args.callback = cb;
  args.arg = this;
  args.dispatch_method = ESP_TIMER_TASK;
  args.name = name;
  ESP_ERROR_CHECK(esp_timer_create(&args, h));
}

void GpioEngine::StopTimers() {
  if (square_timer_) {
    (void)esp_timer_stop(square_timer_);
  }
  if (pulse_periodic_) {
    (void)esp_timer_stop(pulse_periodic_);
  }
  if (pulse_low_oneshot_) {
    (void)esp_timer_stop(pulse_low_oneshot_);
  }
}

void GpioEngine::Stop() {
  StopTimers();
  if (pin_ != GPIO_NUM_NC) {
    gpio_set_level(pin_, 0);
    gpio_set_direction(pin_, GPIO_MODE_INPUT);
    (void)gpio_pullup_dis(pin_);
    (void)gpio_pulldown_dis(pin_);
    ESP_LOGI(TAG, "gpio stop: pin=%d", (int)pin_);
  }
  pin_ = GPIO_NUM_NC;
  memset(&cfg_, 0, sizeof(cfg_));
  __atomic_store_n(&level_, 0, __ATOMIC_RELAXED);
}

void GpioEngine::SquareCb(void *arg) {
  auto *self = static_cast<GpioEngine *>(arg);
  if (!self || self->pin_ == GPIO_NUM_NC) {
    return;
  }
  int lvl = __atomic_load_n(&self->level_, __ATOMIC_RELAXED);
  lvl = lvl ? 0 : 1;
  __atomic_store_n(&self->level_, lvl, __ATOMIC_RELAXED);
  gpio_set_level(self->pin_, lvl);
}

void GpioEngine::PulseLowCb(void *arg) {
  auto *self = static_cast<GpioEngine *>(arg);
  if (!self || self->pin_ == GPIO_NUM_NC) {
    return;
  }
  gpio_set_level(self->pin_, 0);
  __atomic_store_n(&self->level_, 0, __ATOMIC_RELAXED);
}

void GpioEngine::PulsePeriodicCb(void *arg) {
  auto *self = static_cast<GpioEngine *>(arg);
  if (!self || self->pin_ == GPIO_NUM_NC) {
    return;
  }
  gpio_set_level(self->pin_, 1);
  __atomic_store_n(&self->level_, 1, __ATOMIC_RELAXED);
  if (self->pulse_low_oneshot_ && self->cfg_.width_us > 0) {
    (void)esp_timer_stop(self->pulse_low_oneshot_);
    (void)esp_timer_start_once(self->pulse_low_oneshot_, (uint64_t)self->cfg_.width_us);
  }
}

void GpioEngine::ApplyConfig(const exercizer_gpio_config_t &cfg) {
  if (cfg.pin == 0 && cfg.mode == 0) {
    Stop();
    return;
  }

  gpio_num_t pin = static_cast<gpio_num_t>(cfg.pin);
  if (pin != pin_ && pin_ != GPIO_NUM_NC) {
    Stop();
  }

  pin_ = pin;
  cfg_ = cfg;

  gpio_reset_pin(pin_);
  gpio_set_direction(pin_, GPIO_MODE_OUTPUT);
  (void)gpio_pullup_dis(pin_);
  (void)gpio_pulldown_dis(pin_);

  StopTimers();

  if (cfg.mode == EXERCIZER_GPIO_MODE_HIGH) {
    __atomic_store_n(&level_, 1, __ATOMIC_RELAXED);
    gpio_set_level(pin_, 1);
    ESP_LOGI(TAG, "gpio high: pin=%d", (int)pin_);
    return;
  }

  if (cfg.mode == EXERCIZER_GPIO_MODE_LOW) {
    __atomic_store_n(&level_, 0, __ATOMIC_RELAXED);
    gpio_set_level(pin_, 0);
    ESP_LOGI(TAG, "gpio low: pin=%d", (int)pin_);
    return;
  }

  if (cfg.mode == EXERCIZER_GPIO_MODE_SQUARE) {
    if (cfg.hz == 0) {
      ESP_LOGW(TAG, "gpio square invalid hz=0 (stopping)");
      Stop();
      return;
    }
    EnsureTimer(&square_timer_, "gpio_sq", &GpioEngine::SquareCb);
    const uint64_t half_period_us = 500000ULL / (uint64_t)cfg.hz;
    __atomic_store_n(&level_, 0, __ATOMIC_RELAXED);
    gpio_set_level(pin_, 0);
    ESP_ERROR_CHECK(esp_timer_start_periodic(square_timer_, half_period_us > 0 ? half_period_us : 1));
    ESP_LOGI(TAG, "gpio square: pin=%d hz=%" PRIu32, (int)pin_, cfg.hz);
    return;
  }

  if (cfg.mode == EXERCIZER_GPIO_MODE_PULSE) {
    if (cfg.width_us == 0 || cfg.period_ms == 0) {
      ESP_LOGW(TAG, "gpio pulse invalid width_us=%" PRIu32 " period_ms=%" PRIu32 " (stopping)",
               cfg.width_us,
               cfg.period_ms);
      Stop();
      return;
    }
    EnsureTimer(&pulse_periodic_, "gpio_pulse", &GpioEngine::PulsePeriodicCb);
    EnsureTimer(&pulse_low_oneshot_, "gpio_pulse_lo", &GpioEngine::PulseLowCb);
    __atomic_store_n(&level_, 0, __ATOMIC_RELAXED);
    gpio_set_level(pin_, 0);
    ESP_ERROR_CHECK(esp_timer_start_periodic(pulse_periodic_, (uint64_t)cfg.period_ms * 1000ULL));
    ESP_LOGI(TAG, "gpio pulse: pin=%d width_us=%" PRIu32 " period_ms=%" PRIu32,
             (int)pin_,
             cfg.width_us,
             cfg.period_ms);
    return;
  }

  Stop();
}

void GpioEngine::HandleEvent(const exercizer_ctrl_event_t &ev) {
  if (ev.type == EXERCIZER_CTRL_GPIO_SET) {
    exercizer_gpio_config_t cfg = {};
    if (ev.data_len < sizeof(cfg)) {
      ESP_LOGW(TAG, "gpio config too small (len=%u)", ev.data_len);
      return;
    }
    memcpy(&cfg, ev.data, sizeof(cfg));
    ApplyConfig(cfg);
    return;
  }

  if (ev.type == EXERCIZER_CTRL_GPIO_STOP) {
    Stop();
  }
}
