#include "repl/LineEditor.h"

LineEditor::LineEditor(std::string prompt) : prompt_(std::move(prompt)) {
  line_.reserve(512);
}

void LineEditor::SetPrompt(std::string prompt) {
  prompt_ = std::move(prompt);
}

void LineEditor::PrintPrompt(IConsole& console) {
  console.WriteString(prompt_.c_str());
}

std::optional<std::string> LineEditor::FeedByte(uint8_t byte, IConsole& console) {
  if (byte == '\n' || byte == '\r') {
    console.WriteString("\n");
    std::string completed;
    completed.swap(line_);
    line_.clear();
    return completed;
  }

  if (byte == '\b' || byte == 127) {
    if (!line_.empty()) {
      line_.pop_back();
      console.WriteString("\b \b");
    }
    return std::nullopt;
  }

  if (byte >= 32 && byte < 127) {
    if (line_.size() < 511) {
      line_.push_back(static_cast<char>(byte));
      console.Write(&byte, 1);
    }
    return std::nullopt;
  }

  return std::nullopt;
}

