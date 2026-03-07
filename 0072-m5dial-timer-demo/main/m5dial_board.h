#pragma once

#include <stdint.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <ESP32Encoder.h>

namespace tutorial_0072 {

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
  void poll();

  LGFX_M5Dial &display() { return display_; }
  int take_encoder_steps();
  bool take_button_press();
  bool take_button_long_press();
  TouchState read_touch();

 private:
  void init_power_hold();
  void init_encoder_button_gpio();

  LGFX_M5Dial display_{};
  ESP32Encoder encoder_{};
  int encoder_last_count_ = 0;

  int encoder_steps_pending_ = 0;

  bool button_raw_state_ = true;
  bool button_stable_state_ = true;
  int64_t button_last_change_ms_ = 0;
  int64_t button_pressed_ms_ = 0;
  bool button_press_pending_ = false;
  bool button_long_press_pending_ = false;
  bool button_long_press_reported_ = false;
};

}  // namespace tutorial_0072
