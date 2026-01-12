#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_timer.h"

#include "exercizer/ControlPlaneTypes.h"

class GpioEngine {
 public:
  void HandleEvent(const exercizer_ctrl_event_t &ev);
  void StopAll();
  void StopPin(uint8_t pin);

 private:
  struct Channel {
    gpio_num_t pin = GPIO_NUM_NC;
    exercizer_gpio_config_t cfg = {};
    volatile int level = 0;
    esp_timer_handle_t square_timer = nullptr;
    esp_timer_handle_t pulse_periodic = nullptr;
    esp_timer_handle_t pulse_low_oneshot = nullptr;
  };

  void ApplyConfig(Channel *ch, const exercizer_gpio_config_t &cfg);
  void StopChannel(Channel *ch, bool release);
  void EnsureTimer(esp_timer_handle_t *h, const char *name, esp_timer_cb_t cb, void *arg);
  Channel *FindChannel(uint8_t pin);
  Channel *AllocChannel(uint8_t pin);

  static void SquareCb(void *arg);
  static void PulsePeriodicCb(void *arg);
  static void PulseLowCb(void *arg);

  static constexpr size_t kMaxChannels = 8;
  Channel channels_[kMaxChannels] = {};
};
