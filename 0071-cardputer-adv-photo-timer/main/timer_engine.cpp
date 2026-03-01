#include "timer_engine.h"

#include "esp_timer.h"

namespace {

uint64_t now_us() {
  return (uint64_t)esp_timer_get_time();
}

uint32_t clamp_seconds_from_ms(uint32_t ms) {
  return (ms + 999U) / 1000U;
}

}  // namespace

TimerEngine::TimerEngine() {
  mu_ = xSemaphoreCreateMutex();
}

TimerEngine::~TimerEngine() {
  if (mu_) {
    vSemaphoreDelete(mu_);
    mu_ = nullptr;
  }
}

bool TimerEngine::bind_preset(const TimerPreset* preset) {
  if (!preset || preset->steps.empty() || !mu_) {
    return false;
  }

  xSemaphoreTake(mu_, portMAX_DELAY);
  preset_ = *preset;
  has_preset_ = true;
  reset_locked();
  xSemaphoreGive(mu_);

  return true;
}

void TimerEngine::start() {
  if (!mu_) return;
  xSemaphoreTake(mu_, portMAX_DELAY);
  start_locked(now_us());
  xSemaphoreGive(mu_);
}

void TimerEngine::pause() {
  if (!mu_) return;
  xSemaphoreTake(mu_, portMAX_DELAY);
  if (state_ == TimerRunState::kRunning) {
    update_locked(now_us());
    state_ = TimerRunState::kPaused;
  }
  xSemaphoreGive(mu_);
}

void TimerEngine::resume() {
  if (!mu_) return;
  xSemaphoreTake(mu_, portMAX_DELAY);
  if (state_ == TimerRunState::kPaused && step_index_ >= 0 && step_index_ < (int)preset_.steps.size()) {
    const uint64_t elapsed_before_pause_ms = (uint64_t)step_duration_ms_ - (uint64_t)step_remaining_ms_;
    step_started_us_ = now_us() - (elapsed_before_pause_ms * 1000ULL);
    state_ = TimerRunState::kRunning;
  }
  xSemaphoreGive(mu_);
}

void TimerEngine::toggle_run_pause() {
  if (!mu_) return;
  xSemaphoreTake(mu_, portMAX_DELAY);
  if (state_ == TimerRunState::kRunning) {
    update_locked(now_us());
    state_ = TimerRunState::kPaused;
  } else if (state_ == TimerRunState::kPaused) {
    const uint64_t elapsed_before_pause_ms = (uint64_t)step_duration_ms_ - (uint64_t)step_remaining_ms_;
    step_started_us_ = now_us() - (elapsed_before_pause_ms * 1000ULL);
    state_ = TimerRunState::kRunning;
  } else {
    start_locked(now_us());
  }
  xSemaphoreGive(mu_);
}

void TimerEngine::next_step() {
  if (!mu_) return;
  xSemaphoreTake(mu_, portMAX_DELAY);
  if (!has_preset_) {
    xSemaphoreGive(mu_);
    return;
  }

  if (step_index_ < 0) {
    start_locked(now_us());
    xSemaphoreGive(mu_);
    return;
  }

  advance_step_locked(now_us());
  xSemaphoreGive(mu_);
}

void TimerEngine::reset() {
  if (!mu_) return;
  xSemaphoreTake(mu_, portMAX_DELAY);
  reset_locked();
  xSemaphoreGive(mu_);
}

void TimerEngine::update(uint64_t now) {
  if (!mu_) return;
  xSemaphoreTake(mu_, portMAX_DELAY);
  update_locked(now);
  xSemaphoreGive(mu_);
}

TimerSnapshot TimerEngine::snapshot() const {
  TimerSnapshot out;
  if (!mu_) return out;

  xSemaphoreTake(mu_, portMAX_DELAY);
  out.state = state_;
  out.has_preset = has_preset_;

  if (has_preset_) {
    out.preset_id = preset_.id;
    out.preset_name = preset_.name;
    out.step_count = (int)preset_.steps.size();
    out.step_index = step_index_;

    if (step_index_ >= 0 && step_index_ < (int)preset_.steps.size()) {
      out.step_name = preset_.steps[(size_t)step_index_].name;
      out.step_total_sec = preset_.steps[(size_t)step_index_].seconds;
      out.step_remaining_sec = clamp_seconds_from_ms(step_remaining_ms_);
    }
  }

  xSemaphoreGive(mu_);
  return out;
}

void TimerEngine::reset_locked() {
  state_ = TimerRunState::kIdle;
  step_index_ = has_preset_ ? 0 : -1;
  step_started_us_ = 0;
  step_duration_ms_ = 0;
  step_remaining_ms_ = 0;

  if (has_preset_ && !preset_.steps.empty()) {
    const uint32_t seconds = preset_.steps[0].seconds;
    step_duration_ms_ = seconds * 1000U;
    step_remaining_ms_ = step_duration_ms_;
  }
}

void TimerEngine::start_locked(uint64_t now) {
  if (!has_preset_ || preset_.steps.empty()) {
    return;
  }

  if (state_ == TimerRunState::kPaused && step_index_ >= 0) {
    const uint64_t elapsed_before_pause_ms = (uint64_t)step_duration_ms_ - (uint64_t)step_remaining_ms_;
    step_started_us_ = now - (elapsed_before_pause_ms * 1000ULL);
    state_ = TimerRunState::kRunning;
    return;
  }

  if (state_ == TimerRunState::kComplete || step_index_ < 0 || step_index_ >= (int)preset_.steps.size()) {
    step_index_ = 0;
  }

  const uint32_t seconds = preset_.steps[(size_t)step_index_].seconds;
  step_duration_ms_ = seconds * 1000U;
  step_remaining_ms_ = step_duration_ms_;
  step_started_us_ = now;
  state_ = TimerRunState::kRunning;
}

void TimerEngine::update_locked(uint64_t now) {
  if (state_ != TimerRunState::kRunning || !has_preset_ || step_index_ < 0 ||
      step_index_ >= (int)preset_.steps.size()) {
    return;
  }

  const uint64_t elapsed_ms = (now > step_started_us_) ? ((now - step_started_us_) / 1000ULL) : 0ULL;
  if (elapsed_ms >= (uint64_t)step_duration_ms_) {
    step_remaining_ms_ = 0;
    advance_step_locked(now);
    return;
  }

  step_remaining_ms_ = (uint32_t)((uint64_t)step_duration_ms_ - elapsed_ms);
}

void TimerEngine::advance_step_locked(uint64_t now) {
  if (!has_preset_ || preset_.steps.empty()) {
    state_ = TimerRunState::kIdle;
    return;
  }

  const int next = step_index_ + 1;
  if (next >= (int)preset_.steps.size()) {
    step_index_ = (int)preset_.steps.size() - 1;
    step_remaining_ms_ = 0;
    state_ = TimerRunState::kComplete;
    return;
  }

  step_index_ = next;
  const uint32_t seconds = preset_.steps[(size_t)step_index_].seconds;
  step_duration_ms_ = seconds * 1000U;
  step_remaining_ms_ = step_duration_ms_;
  step_started_us_ = now;
  state_ = TimerRunState::kRunning;
}
