#include "timer_model.h"

namespace tutorial_0072 {

namespace {

constexpr uint32_t kMinDurationSec = 5;
constexpr uint32_t kMaxDurationSec = 90U * 60U;

}  // namespace

TimerModel::TimerModel() = default;

void TimerModel::tick(uint64_t now_us) {
  if (state_ != TimerState::kRunning) {
    return;
  }

  const uint64_t elapsed_ms = now_us > started_us_ ? (now_us - started_us_) / 1000ULL : 0ULL;
  if (elapsed_ms >= duration_ms_) {
    remaining_ms_ = 0;
    state_ = TimerState::kComplete;
    return;
  }

  remaining_ms_ = static_cast<uint32_t>(duration_ms_ - elapsed_ms);
}

void TimerModel::adjust_by_detents(int detents) {
  if (detents == 0 || state_ == TimerState::kRunning) {
    return;
  }

  const uint32_t current_sec = duration_ms_ / 1000U;
  const uint32_t step_sec = choose_step_sec(current_sec);
  const int32_t updated_sec = static_cast<int32_t>(current_sec) + (detents * static_cast<int32_t>(step_sec));
  set_duration_sec(clamp_duration_sec(updated_sec));
  state_ = TimerState::kIdle;
}

void TimerModel::toggle_run_pause(uint64_t now_us) {
  if (state_ == TimerState::kRunning) {
    tick(now_us);
    state_ = TimerState::kPaused;
    return;
  }

  if (state_ == TimerState::kPaused) {
    const uint64_t elapsed_before_pause_ms = duration_ms_ - remaining_ms_;
    started_us_ = now_us - (elapsed_before_pause_ms * 1000ULL);
    state_ = TimerState::kRunning;
    return;
  }

  if (state_ == TimerState::kComplete) {
    remaining_ms_ = duration_ms_;
  }

  started_us_ = now_us;
  state_ = TimerState::kRunning;
}

void TimerModel::reset() {
  remaining_ms_ = duration_ms_;
  state_ = TimerState::kIdle;
  started_us_ = 0;
}

TimerSnapshot TimerModel::snapshot() const {
  TimerSnapshot out;
  out.state = state_;
  out.duration_ms = duration_ms_;
  out.remaining_ms = remaining_ms_;
  out.adjustment_step_sec = choose_step_sec(duration_ms_ / 1000U);
  return out;
}

uint32_t TimerModel::clamp_duration_sec(int32_t seconds) {
  if (seconds < static_cast<int32_t>(kMinDurationSec)) {
    return kMinDurationSec;
  }
  if (seconds > static_cast<int32_t>(kMaxDurationSec)) {
    return kMaxDurationSec;
  }
  return static_cast<uint32_t>(seconds);
}

uint32_t TimerModel::choose_step_sec(uint32_t duration_sec) {
  if (duration_sec < 60U) {
    return 5;
  }
  if (duration_sec < 5U * 60U) {
    return 15;
  }
  if (duration_sec < 20U * 60U) {
    return 30;
  }
  return 60;
}

void TimerModel::set_duration_sec(uint32_t seconds) {
  duration_ms_ = seconds * 1000U;
  remaining_ms_ = duration_ms_;
  started_us_ = 0;
}

}  // namespace tutorial_0072
