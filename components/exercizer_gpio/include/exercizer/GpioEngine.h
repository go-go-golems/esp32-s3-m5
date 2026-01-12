#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_timer.h"

#include "exercizer/ControlPlaneTypes.h"

class GpioEngine {
 public:
  void HandleEvent(const exercizer_ctrl_event_t &ev);
  void Stop();

 private:
  void ApplyConfig(const exercizer_gpio_config_t &cfg);
  void StopTimers();
  void EnsureTimer(esp_timer_handle_t *h, const char *name, esp_timer_cb_t cb);

  static void SquareCb(void *arg);
  static void PulsePeriodicCb(void *arg);
  static void PulseLowCb(void *arg);

  gpio_num_t pin_ = GPIO_NUM_NC;
  exercizer_gpio_config_t cfg_ = {};
  volatile int level_ = 0;

  esp_timer_handle_t square_timer_ = nullptr;
  esp_timer_handle_t pulse_periodic_ = nullptr;
  esp_timer_handle_t pulse_low_oneshot_ = nullptr;
};
