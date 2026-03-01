#pragma once

#include <stdint.h>

#include <string>
#include <vector>

enum class TimerRunState {
  kIdle = 0,
  kRunning = 1,
  kPaused = 2,
  kComplete = 3,
};

struct TimerStep {
  std::string name;
  uint32_t seconds = 0;
};

struct TimerPreset {
  std::string id;
  std::string name;
  std::vector<TimerStep> steps;
};

struct TimerConfig {
  uint32_t version = 1;
  std::string active_preset_id;
  std::vector<TimerPreset> presets;
};

struct TimerSnapshot {
  TimerRunState state = TimerRunState::kIdle;
  bool has_preset = false;

  std::string preset_id;
  std::string preset_name;

  int step_index = -1;
  int step_count = 0;
  std::string step_name;
  uint32_t step_total_sec = 0;
  uint32_t step_remaining_sec = 0;
};

inline const TimerPreset* find_preset_by_id(const TimerConfig& cfg, const std::string& id) {
  for (const auto& preset : cfg.presets) {
    if (preset.id == id) {
      return &preset;
    }
  }
  return nullptr;
}
