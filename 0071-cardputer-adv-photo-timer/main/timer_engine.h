#pragma once

#include <stdint.h>

#include "photo_timer_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class TimerEngine {
 public:
  TimerEngine();
  ~TimerEngine();

  bool bind_preset(const TimerPreset* preset);

  void start();
  void pause();
  void resume();
  void toggle_run_pause();
  void next_step();
  void reset();

  void update(uint64_t now_us);

  TimerSnapshot snapshot() const;

 private:
  void reset_locked();
  void start_locked(uint64_t now_us);
  void update_locked(uint64_t now_us);
  void advance_step_locked(uint64_t now_us);

  mutable SemaphoreHandle_t mu_ = nullptr;

  TimerPreset preset_;
  bool has_preset_ = false;

  TimerRunState state_ = TimerRunState::kIdle;
  int step_index_ = -1;

  uint64_t step_started_us_ = 0;
  uint32_t step_duration_ms_ = 0;
  uint32_t step_remaining_ms_ = 0;
};
