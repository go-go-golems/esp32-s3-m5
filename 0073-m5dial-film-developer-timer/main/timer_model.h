#pragma once

#include <stdint.h>

namespace tutorial_0072 {

enum class TimerState {
  kIdle,
  kRunning,
  kPaused,
  kComplete,
};

struct TimerSnapshot {
  TimerState state = TimerState::kIdle;
  uint32_t duration_ms = 5U * 60U * 1000U;
  uint32_t remaining_ms = 5U * 60U * 1000U;
  uint32_t adjustment_step_sec = 15;
};

class TimerModel {
 public:
  TimerModel();

  void tick(uint64_t now_us);
  void adjust_by_detents(int detents);
  void toggle_run_pause(uint64_t now_us);
  void reset();

  TimerSnapshot snapshot() const;

 private:
  static uint32_t clamp_duration_sec(int32_t seconds);
  static uint32_t choose_step_sec(uint32_t duration_sec);
  void set_duration_sec(uint32_t seconds);

  TimerState state_ = TimerState::kIdle;
  uint32_t duration_ms_ = 5U * 60U * 1000U;
  uint32_t remaining_ms_ = 5U * 60U * 1000U;
  uint64_t started_us_ = 0;
};

}  // namespace tutorial_0072
