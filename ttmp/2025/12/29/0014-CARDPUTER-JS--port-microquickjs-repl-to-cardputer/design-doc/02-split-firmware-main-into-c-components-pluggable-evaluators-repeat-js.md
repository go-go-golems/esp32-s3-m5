---
Title: Split firmware main into C++ components + pluggable evaluators (Repeat/JS)
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Reference for Cardputer USB Serial JTAG console behavior and patterns
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: MicroQuickJS public embedding API used by a future JsEvaluator wrapper
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/CMakeLists.txt
      Note: Build wiring for the split C++ firmware (console/repl/eval/storage)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: Current firmware entrypoint that wires the split components together
    - Path: imports/esp32-mqjs-repl/mqjs-repl/sdkconfig
      Note: Console transport defaults (UART primary
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-31T20:17:28.144951933-05:00
WhatFor: ""
WhenToUse: ""
---


# Split firmware main into C++ components + pluggable evaluators (Repeat/JS)

## Executive Summary

The original firmware entrypoint (pre-split monolith) mixed several concerns in one file: storage (SPIFFS), library autoloading, UART transport, a line editor, JS engine initialization, and REPL evaluation. This makes it hard to:

- Debug I/O problems (e.g., “does QEMU deliver bytes to UART RX?”) without also bringing up storage + JS.
- Swap transports (UART → USB Serial JTAG for Cardputer) without touching core REPL/eval logic.
- Add higher-level REPL features (meta-commands, modes, help, structured output) cleanly.

This design proposes splitting “main” into small C++ components with explicit interfaces and a “pluggable evaluator” model. The key feature is a REPL that can run with a trivial evaluator (e.g. `RepeatEvaluator`) to validate the transport + line discipline before involving MicroQuickJS. Once I/O is proven, the same REPL loop can switch to `JsEvaluator`.

The end state is a small orchestrator (`app_main`) that wires up:

1) a `Console` (UART0 / USB Serial JTAG),
2) a `LineEditor` (byte → line),
3) a `CommandDispatcher` (meta-commands + mode switching),
4) an `Evaluator` implementation (Repeat / JS / other),
5) optional `Storage` (SPIFFS) + `Autoload` (script execution).

## Problem Statement

### Symptoms in the current system

- The firmware blocks in `uart_read_bytes()` inside `repl_task()` and can print `js>` but cannot necessarily receive interactive input under QEMU (tracked in ticket `0015-QEMU-REPL-INPUT`).
- Autoload scripts currently fail to parse under MicroQuickJS in QEMU (tracked in ticket `0016-SPIFFS-AUTOLOAD`), and this failure happens *before* the REPL is useful.

### Root problems we want to solve with architecture (not hacks)

1) **Tight coupling of concerns**
   - It’s not possible to bring up “just the REPL I/O” without also initializing SPIFFS and the JS engine.
   - Debugging becomes “all or nothing”, slowing iteration.

2) **Transport is baked into REPL logic**
   - Input comes from UART driver calls directly inside the REPL loop.
   - Output uses `printf()`/`putchar()` directly, making it harder to move toward USB Serial JTAG, screen output, or custom framing.

3) **No evaluator abstraction**
   - REPL always assumes “evaluate JavaScript”.
   - For debugging I/O, we want evaluators like:
     - **Repeat**: return the exact bytes or line so we can test newline/backspace handling.
     - **HexDump**: print input bytes as hex to confirm end-of-line and encoding.
     - **Noop**: accept lines but don’t do anything, testing only prompt redraw and flow.

4) **Portability constraints**
   - Cardputer uses USB Serial JTAG for interactive console; QEMU uses a TCP serial socket. We need a “console abstraction” so the rest of the system doesn’t care.

### Non-goals (for this design)

- This doc does not specify the final Cardputer hardware integration (keyboard scanning, display rendering, speaker, etc.). It focuses on a robust REPL core that we can trust as we iterate.
- This doc does not attempt to solve MicroQuickJS stdlib/atom-table issues; that’s covered separately (see analysis doc).

## Proposed Solution

### Proposed module boundaries

Split `imports/esp32-mqjs-repl/mqjs-repl/main/` into a small set of C++ units with clear responsibility:

- `app_main.cpp`: wiring + lifecycle
- `console/`:
  - `IConsole.h`: abstract “byte stream” interface
  - `UartConsole.{h,cpp}`: UART0 implementation (QEMU-friendly)
  - `UsbSerialJtagConsole.{h,cpp}`: Cardputer implementation (future)
- `repl/`:
  - `LineEditor.{h,cpp}`: prompt + basic line editing
  - `ReplLoop.{h,cpp}`: orchestrates read → edit → dispatch → evaluate → print → prompt
  - `CommandDispatcher.{h,cpp}`: meta-commands and evaluator mode switching
- `eval/`:
  - `IEvaluator.h`: evaluator interface
  - `RepeatEvaluator.{h,cpp}`: echoes input (for transport validation)
  - `JsEvaluator.{h,cpp}`: MicroQuickJS-backed evaluator
- `storage/` (optional at first):
  - `SpiffsStorage.{h,cpp}`: init/mount/format logic
  - `Autoload.{h,cpp}`: scan directory, load scripts, report errors

### Data flow (runtime)

At runtime, bytes flow from the console into the line editor; full lines are then handled either as meta-commands (starting with `:`) or as code passed to the current evaluator.

```text
                +--------------------+
                |      IConsole      |
                |  read()/write()    |
                +---------+----------+
                          |
                          v
                 +--------+--------+
                 |    LineEditor   |
                 | byte -> line    |
                 +--------+--------+
                          |
                 (line complete)
                          |
                          v
                +---------+----------+
                | CommandDispatcher  |
                | ":" commands       |
                +----+----------+----+
                     |          |
             command |          | code
                     v          v
            +--------+--+    +--+----------------+
            | repl control|  |   IEvaluator      |
            | mode/prompt |  | eval(line)->Result|
            +-------------+  +---------+----------+
                                         |
                                         v
                                 +-------+-------+
                                 | IConsole write|
                                 +---------------+
```

### Why C++?

We’re not chasing “C++ for C++’s sake”. The main benefits here are:

- Interface boundaries (`IConsole`, `IEvaluator`) that can be swapped without `#ifdef` soup.
- RAII for resource management (UART driver install/uninstall; SPIFFS mount/unmount) where appropriate.
- Structured composition: `ReplLoop` “has-a” `LineEditor`, “has-a” evaluator, “has-a” console.

ESP-IDF supports C++ builds; we keep the external entrypoint as:

```cpp
extern "C" void app_main(void) {
  App app;
  app.Run();
}
```

### Core interfaces (API signatures)

#### Console: byte stream abstraction

```cpp
// console/IConsole.h
class IConsole {
public:
  virtual ~IConsole() = default;

  // Blocking read of up to `max_len` bytes. Returns actual bytes read.
  virtual int Read(uint8_t* buffer, int max_len, TickType_t timeout) = 0;

  // Best-effort write of `len` bytes.
  virtual void Write(const uint8_t* buffer, int len) = 0;

  // Convenience helpers.
  void WriteString(const char* s);
};
```

Implementation notes:

- `UartConsole` wraps `uart_param_config`, `uart_driver_install`, `uart_read_bytes`, `uart_write_bytes`.
- `UsbSerialJtagConsole` will wrap ESP-IDF USB Serial JTAG driver APIs; for Cardputer we expect `printf()` to already route there, but *input* still needs an implementation.

#### Evaluator: “what happens when a line is submitted?”

```cpp
// eval/IEvaluator.h
struct EvalResult {
  bool ok;
  // Text to write back to console. Could be empty.
  std::string output;
  // Optional: change prompt (e.g. "repeat> " vs "js> ").
  std::optional<std::string> new_prompt;
};

class IEvaluator {
public:
  virtual ~IEvaluator() = default;
  virtual const char* Name() const = 0;
  virtual EvalResult EvalLine(std::string_view line) = 0;
};
```

#### LineEditor: keep the “terminal-ish” behavior in one place

```cpp
// repl/LineEditor.h
class LineEditor {
public:
  explicit LineEditor(std::string prompt);
  void SetPrompt(std::string prompt);

  // Feed one byte; returns a completed line when user hits Enter.
  // The line returned does NOT include \r or \n.
  std::optional<std::string> FeedByte(uint8_t byte, IConsole& console);

  // Print prompt (e.g. at startup and after each evaluation).
  void PrintPrompt(IConsole& console);
};
```

### REPL meta-commands (to enable evaluator testing)

We add a minimal “command layer” on top of line evaluation. The intent is:

- Make transport debugging possible without touching the JS engine.
- Provide discoverability (help output) for new users.
- Give us a safe place to add future REPL features (e.g., `:load`, `:reset`, `:heap`, `:spiffs ls`).

#### Command syntax

- Lines beginning with `:` are REPL commands.
- Everything else is passed to the active evaluator.

Examples:

- `:help` — print help
- `:mode` — show current mode/evaluator
- `:mode repeat` — switch to Repeat evaluator
- `:mode js` — switch to JS evaluator (if available/initialized)
- `:prompt js> ` — set prompt explicitly

#### Dispatcher pseudo-code

```text
handleLine(line):
  if line startsWith ":":
    cmd = parseCommand(line)
    return handleCommand(cmd)
  else:
    return evaluator.EvalLine(line)
```

### Repeat evaluator (for verifying the REPL itself)

This is the first evaluator we should get working under QEMU because it removes all other variables.

Behavior:

- Prints back exactly what was entered (and optionally the byte count).
- Optionally can print escaped sequences (`\r`, `\n`, non-printable bytes) if we later add a raw-byte mode.

Example transcript:

```text
repeat> hello
hello
repeat> var x = 1
var x = 1
```

Implementation sketch:

```cpp
class RepeatEvaluator final : public IEvaluator {
public:
  const char* Name() const override { return "repeat"; }
  EvalResult EvalLine(std::string_view line) override {
    EvalResult r;
    r.ok = true;
    r.output = std::string(line) + "\n";
    r.new_prompt = std::string("repeat> ");
    return r;
  }
};
```

### Js evaluator (MicroQuickJS-backed)

Once transport and line editing are validated, we can enable the real JS evaluator.

Key responsibilities:

- Own the `JSContext*` and its memory arena.
- Provide `EvalLine(...)` that runs `JS_Eval(...)`, prints results, prints exceptions.
- Decide REPL evaluation flags (recommended starting point: `JS_EVAL_REPL | JS_EVAL_RETVAL` once stdlib supports it).

Sketch:

```cpp
class JsEvaluator final : public IEvaluator {
public:
  explicit JsEvaluator(MicroQuickJsEngine& engine);
  const char* Name() const override { return "js"; }
  EvalResult EvalLine(std::string_view line) override;
};
```

### Tasking model (FreeRTOS)

We keep the “one task owns JS context” rule:

- `app_main` initializes and creates `ReplTask`.
- `ReplTask` owns the console, line editor, and active evaluator.
- If other tasks need to interact with JS later, they do so via message passing (queue) to the REPL/JS task.

Diagram:

```text
    +-------------------+           +-------------------+
    |  Other tasks      |  queue    |   ReplTask        |
    | (wifi, ui, etc)   +----------->   owns JSContext  |
    +-------------------+           +-------------------+
```

This keeps us out of “cross-task JSContext access” bugs early.

## Design Decisions

### D1: Pluggable evaluators are a first-class concept

Rationale:

- It lets us debug and develop in layers.
- It gives us a “known-good” REPL harness even if JS is temporarily broken.
- It provides a clean place to add future non-JS commands (e.g. device control) without abusing JS parsing.

### D2: Meta-commands use a prefix (`:`) and are parsed before evaluation

Rationale:

- Minimal friction: a single character is easy to type on embedded keyboards and serial consoles.
- Unambiguous: avoids conflicting with “normal” JS unless the user intentionally uses a command.
- Future-proof: we can add `:load`, `:reset`, `:mem`, etc.

### D3: Keep `IConsole` as raw bytes (not lines)

Rationale:

- The line discipline is a REPL concern, not a transport concern.
- Some transports (USB Serial JTAG, UART) may deliver bytes differently; REPL should normalize behavior.
- Later we might support binary protocols, bracketed paste, or history; byte-level control is helpful.

### D4: Keep initial “transport bring-up” independent of storage + JS

Rationale:

- This directly supports the project’s original strategy: “test without storage first”.
- Storage and autoload are a second layer that should be enabled only after REPL I/O is confirmed.

## Alternatives Considered

### A1: Keep C, just split into multiple `.c` files

Pros:

- Minimal toolchain surprises.

Cons:

- Interface boundaries become conventions rather than enforced contracts.
- Harder to express “replace evaluator/console” cleanly without global state.

Decision: rejected for now; C++ provides cleaner structure for multiple swappable components.

### A2: Use ESP-IDF `esp_console` REPL framework

Pros:

- Built-in line editing and command registration.

Cons:

- Our long-term goal is a JS REPL, not a command shell.
- `esp_console` is oriented around C-registered commands, not evaluating a language.
- It may complicate QEMU debugging if it uses different UART/stdio pathways than our firmware.

Decision: not used initially. We can revisit later once we want shell-like commands.

### A3: Make “repeat” mode a JS function instead of a separate evaluator

Pros:

- One evaluator only.

Cons:

- If JS parsing or stdlib is broken (which it currently is for some scripts), we lose our debugging tool.
- We still can’t isolate transport problems.

Decision: rejected; repeat mode must not depend on JS.

## Implementation Plan

### Step 1: Mechanical split without behavior change

- Create directory structure and move code out of `main.c` into:
  - `UartConsole`
  - `LineEditor`
  - `ReplLoop`
- Keep UART transport and `printf` output as-is.
- Keep JS eval as-is (even if limited).

Success criteria:

- Builds and boots exactly like today.

### Step 2: Add evaluator abstraction and Repeat evaluator

- Introduce `IEvaluator`.
- Implement `RepeatEvaluator`.
- Wire `:mode repeat` command.
- Default boot mode should be configurable (compile-time or runtime), ideally defaulting to repeat in QEMU/dev builds.

Success criteria:

- Under QEMU we can type and see echoed lines; we can validate newline/backspace behavior.

### Step 3: Add JS evaluator wrapper (still optional)

- Wrap MicroQuickJS context creation and eval into `MicroQuickJsEngine` + `JsEvaluator`.
- Add `:mode js`.
- Add `:reset` (optional) to recreate the context.

Success criteria:

- We can evaluate simple expressions and see output/exceptions routed through the same REPL harness.

### Step 4: Storage and autoload become an opt-in layer

- Implement `SpiffsStorage` and `Autoload`.
- Add commands:
  - `:spiffs status`
  - `:spiffs ls`
  - `:load /spiffs/foo.js`
  - `:autoload on|off`

Success criteria:

- Autoload failures do not prevent the REPL from being usable; they show as errors but REPL continues.

### Step 5: Cardputer transport swap

- Implement `UsbSerialJtagConsole`.
- Choose transport at build-time via `sdkconfig.defaults` (Cardputer vs QEMU dev).
- Keep the rest unchanged.

## Open Questions

1) Should the REPL output path go through `IConsole::Write()` exclusively (no `printf`), or do we allow `printf` for logs and use `IConsole` only for interactive output?
2) Do we want multiline input (continuation prompt) for JS, or keep single-line for now?
3) Should `:mode js` be available if SPIFFS/autoload failed, or should evaluator availability be independent?
4) Should we support “raw byte mode” for QEMU debugging (dump every received byte before line editing), or keep that as a temporary build flag?

## References

- Current split implementation entrypoint: `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
- MicroQuickJS embedding API: `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
- Existing ticket context: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/index.md`
- QEMU REPL input bug: `ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/index.md`
- SPIFFS/autoload parse errors: `ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/index.md`
