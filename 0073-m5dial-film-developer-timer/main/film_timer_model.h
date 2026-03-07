#pragma once

#include <stdint.h>

#include "film_catalog.h"

namespace tutorial_0073 {

enum class FilmTimerState {
  kEmpty = 0,
  kReady,
  kRunning,
  kPaused,
  kComplete,
};

struct FilmTimerSnapshot {
  FilmTimerState state = FilmTimerState::kEmpty;
  RecipeSelection selection{};
  const FilmCatalogEntry *recipe = nullptr;
  uint32_t duration_ms = 0;
  uint32_t remaining_ms = 0;
};

class FilmTimerModel {
 public:
  void load_recipe(const RecipeSelection &selection, const FilmCatalogEntry *recipe);
  void tick(uint64_t now_us);
  void toggle_run_pause(uint64_t now_us);
  void reset();
  void clear();

  bool has_recipe() const { return recipe_ != nullptr; }
  FilmTimerSnapshot snapshot() const;

 private:
  RecipeSelection selection_{};
  const FilmCatalogEntry *recipe_ = nullptr;
  FilmTimerState state_ = FilmTimerState::kEmpty;
  uint32_t duration_ms_ = 0;
  uint32_t remaining_ms_ = 0;
  uint64_t started_us_ = 0;
};

}  // namespace tutorial_0073
