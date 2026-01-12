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
  if (in_escape_) {
    escape_len_++;
    if (byte >= 0x40 && byte <= 0x7e) {
      in_escape_ = false;
      escape_len_ = 0;
    } else if (escape_len_ > 16) {
      in_escape_ = false;
      escape_len_ = 0;
    }
    return std::nullopt;
  }

  if (byte == 0x1b) {  // ESC (start of an ANSI escape sequence)
    in_escape_ = true;
    escape_len_ = 0;
    return std::nullopt;
  }

  if (byte == 0x03) {  // Ctrl+C
    line_.clear();
    console.WriteString("^C\n");
    return std::string();
  }

  if (byte == 0x15) {  // Ctrl+U (kill line)
    while (!line_.empty()) {
      line_.pop_back();
      console.WriteString("\b \b");
    }
    return std::nullopt;
  }

  if (byte == 0x0c) {  // Ctrl+L (clear screen)
    console.WriteString("\033[2J\033[H");
    PrintPrompt(console);
    if (!line_.empty()) {
      console.Write(reinterpret_cast<const uint8_t*>(line_.data()),
                    static_cast<int>(line_.size()));
    }
    return std::nullopt;
  }

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
