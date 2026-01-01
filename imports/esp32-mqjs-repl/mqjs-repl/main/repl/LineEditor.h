#pragma once

#include <optional>
#include <string>

#include "console/IConsole.h"

class LineEditor {
 public:
  explicit LineEditor(std::string prompt);

  void SetPrompt(std::string prompt);
  void PrintPrompt(IConsole& console);

  std::optional<std::string> FeedByte(uint8_t byte, IConsole& console);

 private:
  std::string prompt_;
  std::string line_;
  bool in_escape_ = false;
  int escape_len_ = 0;
};
