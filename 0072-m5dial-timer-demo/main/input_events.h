#pragma once

#include <stdint.h>

namespace tutorial_0072 {

enum class SwipeDirection {
  kNone = 0,
  kLeft,
  kRight,
  kUp,
  kDown,
};

enum class InputEventType {
  kEncoderDelta = 0,
  kButtonShortPress,
  kButtonLongPress,
  kSwipe,
};

struct InputEvent {
  InputEventType type = InputEventType::kEncoderDelta;
  int32_t value = 0;
  SwipeDirection swipe = SwipeDirection::kNone;
  uint64_t timestamp_us = 0;
};

}  // namespace tutorial_0072
