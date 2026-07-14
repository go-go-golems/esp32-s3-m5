#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <ESP32Encoder.h>

#include "input_events.h"

namespace ppa_dial {

struct TouchState {
  bool pressed = false;
  int16_t x = -1;
  int16_t y = -1;
};

class LGFX_M5Dial : public lgfx::LGFX_Device {
 public:
  LGFX_M5Dial();

 private:
  lgfx::Panel_GC9A01 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
};

class M5DialBoard {
 public:
  bool init();
  void set_button_irq_task(TaskHandle_t task_handle);
  void poll(QueueHandle_t event_queue);

  LGFX_M5Dial &display() { return display_; }
  TouchState read_touch();
  bool touch_ready() const { return touch_ready_; }

 private:
  void init_power_hold();
  void init_encoder_button_gpio();
  bool init_touch_controller();
  void poll_touch(QueueHandle_t event_queue);
  TouchState sample_touch();
  static void IRAM_ATTR button_isr_handler(void *arg);

  LGFX_M5Dial display_{};
  ESP32Encoder encoder_{};
  int encoder_last_count_ = 0;

  bool button_raw_state_ = true;
  bool button_stable_state_ = true;
  int64_t button_last_change_ms_ = 0;
  int64_t button_pressed_ms_ = 0;
  bool button_long_press_reported_ = false;
  TaskHandle_t button_irq_task_ = nullptr;

  bool touch_ready_ = false;
  bool touch_was_pressed_ = false;
  bool touch_swipe_reported_ = false;
  TouchState touch_state_{};
  TouchState touch_start_{};
  TouchState touch_last_{};
};

}  // namespace ppa_dial
