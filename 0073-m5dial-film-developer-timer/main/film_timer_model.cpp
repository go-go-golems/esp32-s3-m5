#include "film_timer_model.h"

namespace tutorial_0073 {

void FilmTimerModel::load_recipe(const RecipeSelection &selection, const FilmCatalogEntry *recipe) {
  selection_ = selection;
  recipe_ = recipe;
  if (!recipe_) {
    clear();
    return;
  }

  duration_ms_ = recipe_->time_seconds * 1000U;
  remaining_ms_ = duration_ms_;
  started_us_ = 0;
  state_ = FilmTimerState::kReady;
}

void FilmTimerModel::tick(uint64_t now_us) {
  if (state_ != FilmTimerState::kRunning || !recipe_) {
    return;
  }

  const uint64_t elapsed_ms = now_us > started_us_ ? (now_us - started_us_) / 1000ULL : 0ULL;
  if (elapsed_ms >= duration_ms_) {
    remaining_ms_ = 0;
    state_ = FilmTimerState::kComplete;
    return;
  }

  remaining_ms_ = static_cast<uint32_t>(duration_ms_ - elapsed_ms);
}

void FilmTimerModel::toggle_run_pause(uint64_t now_us) {
  if (!recipe_) {
    return;
  }

  if (state_ == FilmTimerState::kRunning) {
    tick(now_us);
    state_ = FilmTimerState::kPaused;
    return;
  }

  if (state_ == FilmTimerState::kPaused) {
    const uint64_t elapsed_before_pause_ms = duration_ms_ - remaining_ms_;
    started_us_ = now_us - (elapsed_before_pause_ms * 1000ULL);
    state_ = FilmTimerState::kRunning;
    return;
  }

  if (state_ == FilmTimerState::kComplete) {
    remaining_ms_ = duration_ms_;
  }

  started_us_ = now_us;
  state_ = FilmTimerState::kRunning;
}

void FilmTimerModel::reset() {
  if (!recipe_) {
    return;
  }

  remaining_ms_ = duration_ms_;
  started_us_ = 0;
  state_ = FilmTimerState::kReady;
}

void FilmTimerModel::clear() {
  selection_ = {};
  recipe_ = nullptr;
  state_ = FilmTimerState::kEmpty;
  duration_ms_ = 0;
  remaining_ms_ = 0;
  started_us_ = 0;
}

FilmTimerSnapshot FilmTimerModel::snapshot() const {
  FilmTimerSnapshot out;
  out.state = state_;
  out.selection = selection_;
  out.recipe = recipe_;
  out.duration_ms = duration_ms_;
  out.remaining_ms = remaining_ms_;
  return out;
}

}  // namespace tutorial_0073
