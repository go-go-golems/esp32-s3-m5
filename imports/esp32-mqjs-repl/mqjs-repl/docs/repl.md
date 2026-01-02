---
Title: Cardputer JS REPL: REPL Component Reference
Slug: mqjs-repl-component-reference
Short: Developer reference for the REPL loop, line editor, and console abstraction
Topics:
  - console
  - serial
  - uart
  - usb-serial-jtag
  - cardputer
  - esp-idf
  - esp32s3
  - debugging
IsTemplate: false
IsTopLevel: false
ShowPerDefault: false
SectionType: GeneralTopic
---

# Cardputer JS REPL: REPL Component Reference

## Overview

The “REPL component” is the interactive loop that:

1) reads bytes from a console transport,
2) turns bytes into completed lines (with minimal line editing),
3) handles REPL meta-commands (lines starting with `:`), and
4) forwards non-command lines to an evaluator (Repeat or JS).

This doc is for developers who want to change REPL behavior without breaking:

- prompt + input correctness (QEMU + device),
- meta-commands (`:help`, `:mode`, `:stats`, `:reset`, `:autoload`, `:prompt`),
- and transport independence (`UART0` vs `USB Serial/JTAG`).

## Narrative Walkthrough (What’s Actually Happening)

When you interact with the firmware, you’re really talking to a single FreeRTOS task that runs an infinite loop: read some bytes, feed them into a line editor, and act only when a full line is complete. That architecture is boring on purpose—it keeps the “interactive contract” predictable, and it makes it much easier to debug issues like “did the device receive bytes?” without involving unrelated subsystems.

The REPL is intentionally split into three responsibilities:

- **Transport (`IConsole`)**: “Where do bytes come from, and where does output go?” This can be UART0 (QEMU) or USB Serial/JTAG (Cardputer). Everything else treats it as a byte stream.
- **Line discipline (`LineEditor`)**: “How do raw bytes become a human-friendly line?” This is where backspace, Ctrl shortcuts, and prompt printing live.
- **Dispatch (`ReplLoop`)**: “What do we do with a line?” Command lines (starting with `:`) are handled by the REPL itself; all other lines are forwarded to the active evaluator.

One practical implication: if you want to improve the user experience (history, arrow keys, multiline input), those changes belong almost entirely in the line editor and should not require changing transports or the evaluator.

## Code Map (Start Here)

- Entry wiring: `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
  - `app_main()` creates:
    - `IConsole` implementation (`UartConsole` or `UsbSerialJtagConsole`)
    - `ModeSwitchingEvaluator` (default mode: repeat)
    - `LineEditor` with `evaluator.Prompt()`
    - `ReplLoop`, then runs them in a FreeRTOS task.
- REPL loop: `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`
  - `ReplLoop::Run(IConsole&, LineEditor&, IEvaluator&)`
  - `ReplLoop::HandleLine(...)` parses meta-commands and calls evaluator.
- Line editor: `imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.cpp`
  - `LineEditor::FeedByte(uint8_t, IConsole&) -> optional<string>` returns a line on Enter.
  - `LineEditor::PrintPrompt(IConsole&)`, `LineEditor::SetPrompt(std::string)`.
- Console abstraction: `imports/esp32-mqjs-repl/mqjs-repl/main/console/IConsole.h`
  - `IConsole::Read(...)`, `IConsole::Write(...)`, `IConsole::WriteString(...)`
  - Implementations:
    - UART: `imports/esp32-mqjs-repl/mqjs-repl/main/console/UartConsole.cpp`
    - USB Serial/JTAG: `imports/esp32-mqjs-repl/mqjs-repl/main/console/UsbSerialJtagConsole.cpp`

## Command Contract (Meta-Commands)

Meta-commands are parsed in `ReplLoop::HandleLine(...)` and must remain stable:

- `:help` — print command help
- `:mode` — show current evaluator
- `:mode repeat|js` — switch evaluator (delegates to `IEvaluator::SetMode`)
- `:reset` — reset the current evaluator (delegates to `IEvaluator::Reset`)
- `:stats` — print heap stats + evaluator stats (delegates to `IEvaluator::GetStats`)
- `:autoload [--format]` — run `/spiffs/autoload/*.js` (delegates to `IEvaluator::Autoload`)
- `:prompt TEXT` — set prompt string (LineEditor only)

Rules:
- Commands are matched after trimming whitespace.
- Unknown commands are treated as evaluator input (and may error in JS mode).

## Transport Independence

The REPL is intentionally transport-agnostic. Only `IConsole` knows how bytes are read/written.

- For QEMU testing, use `UartConsole` (UART0).
- For Cardputer, use `UsbSerialJtagConsole` (USB Serial/JTAG, `/dev/ttyACM*` on Linux).

Build-time selection is via Kconfig (`CONFIG_MQJS_REPL_CONSOLE_USB_SERIAL_JTAG`) and helper:
- `imports/esp32-mqjs-repl/mqjs-repl/tools/set_repl_console.sh`

## Line Editing Behavior (Current)

The editor is intentionally minimal and stable for embedded environments:

- Backspace handling
- Ctrl shortcuts:
  - `Ctrl+C` cancel line
  - `Ctrl+U` kill line
  - `Ctrl+L` clear screen
- ANSI escape sequences are ignored (to avoid arrow keys polluting the buffer)

If you want REPL history and arrow keys, the implementation belongs in `LineEditor::FeedByte`.
Be careful: adding escape parsing changes how terminals interact and can regress QEMU/device input.

## How to Validate Changes

These are scripts that can be used as inspiration on how to validate using the REPL component.

### QEMU (non-interactive, UART stdio)

Repeat-mode smoke:
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_repeat_repl_qemu_uart_stdio.sh --timeout 120
```

JS-mode smoke (also exercises `:stats` + `:autoload`):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_js_repl_qemu_uart_stdio.sh --timeout 120
```

### Device (Cardputer)

Stable flash (USB Serial/JTAG):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag
./build.sh build
./tools/flash_device_usb_jtag.sh --port auto
```

Device JS smoke (asserts seeded SPIFFS autoload works):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
python3 ./tools/test_js_repl_device_uart_raw.py --port auto --timeout 90
```

## Common Pitfalls

- **USB Serial/JTAG re-enumeration**: `/dev/ttyACM0` can become `/dev/ttyACM1` after resets.
  - Prefer `/dev/serial/by-id/...` or `--port auto` (scripts) to avoid port churn.
- **Blocking behavior**: `ReplLoop::Run` blocks forever on reads; keep any new work (parsing, file I/O) bounded and avoid hidden boot-time operations.
- **Output buffering**: `console_printf(...)` in `ReplLoop.cpp` uses a fixed 256-byte buffer; large dumps should stream instead of formatting huge lines.

## Usage Sketches (Snippets)

### Add a new meta-command

Add a new branch in `ReplLoop::HandleLine(...)` (in `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`):

```cpp
if (stripped == ":ping") {
  console.WriteString("pong\n");
  return;
}
```

Validate by typing `:ping` over device/QEMU and ensuring it prints `pong` and returns to the prompt.

### Add a new evaluator mode

1) Implement `IEvaluator` (new file under `imports/esp32-mqjs-repl/mqjs-repl/main/eval/`):

```cpp
class MyEvaluator final : public IEvaluator {
 public:
  const char* Name() const override { return "my"; }
  const char* Prompt() const override { return "my> "; }
  EvalResult EvalLine(std::string_view line) override { return {.ok=true, .output="ok\n"}; }
};
```

2) Wire it into `ModeSwitchingEvaluator::SetMode(...)`:

```cpp
if (mode == "my") {
  current_ = &my_;
  return true;
}
```

3) Update `ModeHelp()` string (so `:help` lists the mode).

### Prototype history/arrows in the line editor

The intended extension point is `LineEditor::FeedByte(...)`:

```cpp
// Pseudocode sketch: when you see ESC [ A, replace current line with previous history entry.
if (in_escape_sequence && escape_is_arrow_up) {
  line_ = history_.Prev();
  RedrawLine(console);  // clear + print prompt + line_
  return std::nullopt;
}
```

Keep the initial implementation conservative: maintain a small ring buffer and avoid dynamic allocation in hot input paths.
