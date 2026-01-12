---
Title: 'ESP-IDF Monitor (idf_monitor): Deep Analysis'
Ticket: 0033-ANALYZE-IDF-PY
Status: active
Topics:
    - esp-idf
    - build-tool
    - python
    - go
    - cmake
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/console_reader.py
      Note: ConsoleReader thread and TTY key handling
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/reset.py
      Note: Reset strategies and custom_reset_sequence support
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/serial_handler.py
      Note: 'Serial processing hot path: split/filter/color/decode'
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/serial_reader.py
      Note: SerialReader thread
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/idf_monitor.py
      Note: Main monitor entry
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/reset.py
      Note: Reference implementation for custom reset sequences (exec-based)
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/tools/idf_monitor.py
      Note: ESP-IDF wrapper that launches python -m esp_idf_monitor
ExternalSources: []
Summary: 'Deep, source-based analysis of ESP-IDF''s `idf_monitor`/`esp-idf-monitor` serial monitor: architecture, main loop, threads, decoding pipelines (PC addr/panic/coredump/binary logs), reset strategies, and IDE WebSocket integration.'
LastUpdated: 2026-01-10T21:27:52.46429883-05:00
WhatFor: Onboarding a new developer into `esp-idf-monitor` internals (and the way `idf.py monitor` works), and as a reference when debugging monitor behavior or re-implementing the monitor in another language.
WhenToUse: When you need to understand or modify monitor behavior (keybindings, filters, reconnection, decoding, reset, IDE integration), or when designing a replacement / wrapper around `idf_monitor`.
---


# ESP-IDF Monitor (`idf_monitor` / `esp-idf-monitor`) — Deep Analysis

## Terminology (names vs reality)

In ESP-IDF v5.4.x the “monitor” tool is split into:

- A **thin wrapper script** in the ESP-IDF repo: `~/esp/esp-idf-5.4.1/tools/idf_monitor.py`
  - This file is only a launcher: it runs `python -m esp_idf_monitor ...`.
- A **real Python package** installed into the ESP-IDF Python environment: `esp_idf_monitor`
  - Example location on this machine:
    - `~/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/`

This document analyzes the **installed package** (the real implementation) and explains how it’s invoked by `idf.py`.

## Source Map (key files + responsibilities)

**Entry points**

- `~/esp/esp-idf-5.4.1/tools/idf_monitor.py`
  - Wrapper: `python -m esp_idf_monitor`.
- `.../site-packages/esp_idf_monitor/__main__.py`
  - Entrypoint for `python -m esp_idf_monitor` → calls `esp_idf_monitor.idf_monitor.main()`.
- `.../site-packages/esp_idf_monitor/idf_monitor.py`
  - Main program: argument parsing, port detection, monitor construction, main loop.

**Core architecture**

- `.../site-packages/esp_idf_monitor/base/console_reader.py`
  - Reads keypresses from the terminal in a thread, emits events into queues.
- `.../site-packages/esp_idf_monitor/base/console_parser.py`
  - Keybinding/menu state machine: CTRL-T style commands, help, exit.
- `.../site-packages/esp_idf_monitor/base/serial_reader.py`
  - Reads serial bytes in a thread, reconnect logic, initial reset/open sequencing.
- `.../site-packages/esp_idf_monitor/base/serial_handler.py`
  - Converts raw serial bytes to lines, filtering, coloring, decoding hooks, command handling.

**Decoding / tooling integrations**

- `.../site-packages/esp_idf_monitor/base/logger.py`
  - Printing, optional logging to file, timestamps, PC address decoding (via dependency).
- `.../site-packages/esp_idf_monitor/base/coredump.py`
  - Detects and decodes UART core dumps (or forwards to IDE over WebSocket).
- `.../site-packages/esp_idf_monitor/base/gdbhelper.py`
  - Detects GDB stub stop packets, spawns GDB or forwards to IDE over WebSocket.
- `.../site-packages/esp_idf_monitor/base/binlog.py`
  - “Binary log” framing detection + decoding back to text logs using ELF strings.

**Reset + configuration**

- `.../site-packages/esp_idf_monitor/base/reset.py`
  - Reset strategies (hard reset, bootloader entry), optional custom reset sequences.
- `.../site-packages/esp_idf_monitor/base/chip_specific_config.py`
  - Per-chip timing constants for reset sequences.
- `.../site-packages/esp_idf_monitor/config.py`
  - Config file discovery + validation; feeds keybinding and reset configuration.
- `.../site-packages/esp_idf_monitor/base/key_config.py`
  - Computes control-key codes from config at import time.
- `.../site-packages/esp_idf_monitor/base/constants.py`
  - Event tags, command IDs, environment variable names, timing constants.

**IDE integration**

- `.../site-packages/esp_idf_monitor/base/web_socket_client.py`
  - WebSocket JSON messages: `gdb_stub`, `coredump`, waits for `debug_finished`.

## How `idf.py monitor` invokes this tool (high level)

At a high level, `idf.py monitor` does not implement monitoring itself. It launches the monitor tool (wrapper script) with arguments such as:

- serial `--port` / `--baud`
- one or more ELF files (for addr2line / symbol resolution)
- `--target` and other decode-related options

The critical point: the ESP-IDF tree’s `tools/idf_monitor.py` is just a shim; the real logic is the installed module `esp_idf_monitor`.

## CLI (arguments + environment defaults)

Parsing is done in `.../base/argument_parser.py:get_parser()`. It uses `argparse` and defaults to a mixture of:

- explicit defaults (e.g. `115200`)
- environment variables (e.g. `ESPTOOL_PORT`, `IDF_MONITOR_BAUD`)
- config file values (for keybindings + some timings)

### Primary arguments

- `--port/-p`: serial port device
  - Default: `ESPTOOL_PORT` (or autodetect if unset).
- `--baud/-b`: baud rate
  - Default: `IDF_MONITOR_BAUD` or `MONITORBAUD` or `115200`.
- `--no-reset`: don’t reset on startup
  - Default is derived from `ESP_IDF_MONITOR_NO_RESET` and `DEFAULT_TARGET_RESET`.
- `--toolchain-prefix`: prefix for toolchain binaries (GDB, addr2line)
  - Default: `xtensa-esp32-elf-` (in `base/constants.py`).
- `--decode-coredumps`: `info` or `disable`
  - Default: `info` (decode by default).
- `--decode-panic`: `backtrace` or `disable`
  - Default: `disable` (panic/backtrace decoding is opt-in).
- `--ws`: WebSocket server URL (IDE integration)
  - Default: `ESP_IDF_MONITOR_WS` if set.
- `--timestamps`, `--timestamp-format`
  - Adds timestamps at the beginning of each printed line.
- `elf_files` (positional): 0..N ELF files (order matters for address decoding)

### Less obvious / important arguments

- `--open-port-attempts`: retry policy if port isn’t present at startup
  - `0` means “infinite attempts”.
  - `reconnect_delay` is taken from config file (`esp-idf-monitor.cfg`, etc.) and used by `SerialReader`.
- `--rom-elf-file`: ROM ELF for decoding ROM addresses
  - If omitted, monitor attempts autodetection using `IDF_PATH` and `ESP_ROM_ELF_DIR` via `base/rom_elf_getter.py`.
- `--disable-address-decoding/-d`: disables PC address decoding.
- `--print_filter`: log filtering string parsed by `base/line_matcher.py`.
- `--force-color` / `--disable-auto-color`: color-related toggles.

## Architecture Overview (threads + queues + main thread event loop)

The design is explicitly “**all event processing happens in the main thread**” (see `esp_idf_monitor/idf_monitor.py:Monitor` docstring). Worker threads only enqueue events.

### Actors

- **Main thread**
  - Owns the “business logic” of decoding and command handling.
  - Runs `Monitor.main_loop()` which repeatedly calls `Monitor._main_loop()`.
- **ConsoleReader thread**
  - Reads key presses from the local terminal.
  - Produces:
    - `TAG_KEY` events for data to write to the serial port
    - `TAG_CMD` events for monitor commands (reset, toggle logging, etc.)
- **SerialReader thread**
  - Opens the serial port (with optional reset), then reads serial bytes.
  - Produces `TAG_SERIAL` events containing raw bytes.

### Event queues

Monitor uses **two queues**:

- `event_queue`: mixed event stream (`TAG_KEY`, `TAG_SERIAL`, `TAG_CMD`, flush events)
- `cmd_queue`: command events (`TAG_CMD`) that should run “ASAP”

The main loop gives **priority** to `cmd_queue` (non-blocking `get_nowait()`), then falls back to waiting on `event_queue` with a small timeout (`EVENT_QUEUE_TIMEOUT`).

### Core dataflow diagram

```text
-------------------+                     +---------------------------+
| ConsoleReader     |  TAG_KEY / TAG_CMD  | cmd_queue (priority)      |
| (thread)          +-------------------->+---------------------------+
| - console.getkey  |                     +---------------------------+
| - ConsoleParser   |  TAG_KEY / TAG_CMD  | event_queue (fallback)    |
+-------------------+-------------------->+---------------------------+
                                              |
                                              v
                                      +------------------+
                                      | Monitor._main_loop|
                                      | (main thread)     |
                                      +------------------+
                                       | TAG_SERIAL → SerialHandler
                                       | TAG_KEY    → serial_write()
                                       | TAG_CMD    → handle_commands()
                                       v
                                 +--------------+
                                 | SerialHandler|
                                 +--------------+
                                        |
                                        v
                                     Logger

+-------------------+  TAG_SERIAL       ^
| SerialReader       +------------------+
| (thread)           |
| - open/reconnect   |
| - serial.read(...) |
+-------------------+
```

## `esp_idf_monitor.idf_monitor` (main program)

### `main()` flow (pseudocode)

Based on `esp_idf_monitor/idf_monitor.py:main()`:

```pseudo
args = parse_args()

require stdin is a TTY (unless ESP_IDF_MONITOR_TEST)

args.eol = args.eol or (LF if target == linux else CR)

rom_elf_file = (
  args.rom_elf_file if provided
  else autodetect via get_rom_elf_path(target, revision)
)

strip MAKEFLAGS jobserver args (so nested make isn't constrained)

ws = WebSocketClient(args.ws) if args.ws else None

if target == linux:
  cls = LinuxMonitor
  serial_instance = None
else:
  port = args.port or detect_port()
  normalize Windows COM names and macOS /dev/tty.* → /dev/cu.*
  serial_instance = serial.serial_for_url(port, baud, do_not_open=True, exclusive=True)
  set serial_instance.write_timeout (except RFC2217)

  export environment:
    ESPPORT = str(args.port)   # see "Gotchas" below
    ESPTOOL_OPEN_PORT_ATTEMPTS = str(args.open_port_attempts)

  cls = SerialMonitor

monitor = cls(serial_instance, args.elf_files, ..., ws, ..., rom_elf_file)

print help banner and filter banner
Config().load_configuration(verbose=True)  # prints loaded config file path if any

monitor.main_loop()

finally: ws.close()
```

### Gotcha: `ESPPORT` value when autodetecting

`main()` intentionally avoids mutating `args.port` while it normalizes `port` for Windows/macOS, but then exports `ESPPORT` using `args.port` (not the local `port` variable). If the user didn’t pass `--port` and autodetection ran, `args.port` can be `None`, so `ESPPORT` may be set to `"None"`.

This matters because other tools/scripts may read `ESPPORT`.

## Monitor classes and lifecycle

### `Monitor` base class (`esp_idf_monitor/idf_monitor.py:Monitor`)

`Monitor` wires together:

- `event_queue`, `cmd_queue`
- `miniterm.Console` (terminal IO)
- `Logger`
- optional `CoreDump` (only if at least one ELF exists)
- a `Reader` implementation:
  - `SerialReader` (device target)
  - `LinuxReader` (linux target)
- optional `GDBHelper` (only if ELF exists, and non-linux target)
- `SerialHandler` or `SerialHandlerNoElf`
- `ConsoleParser` and `ConsoleReader`
- `LineMatcher` for `--print_filter`

#### Context manager behavior: `with monitor:`

`Monitor.__enter__()` stops `serial_reader` and `console_reader`. This is used to temporarily disable monitor IO interception while executing “foreground” actions like:

- running a build/flash target via `run_make()`
- running `gdb` (so the terminal can be used by GDB)

Each subclass implements `__exit__()` to restart readers appropriately.

### `SerialMonitor` subclass

Key responsibilities:

- Implements `serial_write()` using `serial.Serial.write()`, with:
  - `SerialTimeoutException` warning aimed at “wrong console selected” problems
  - ignores `SerialException` and `UnicodeEncodeError`
- Implements `check_gdb_stub_and_run()`:
  - calls `GDBHelper.check_gdb_stub_trigger(line)`
  - if triggered, enters `with self:` and runs `GDBHelper.run_gdb()`
- Adds a post-GDB continuation behavior in `_main_loop()`:
  - if `gdb_exit` flag is set:
    - waits `GDB_EXIT_TIMEOUT`
    - writes `GDB_UART_CONTINUE_COMMAND` (`'+$c#63'`) to continue the program
    - sets `serial_handler.start_cmd_sent = True` so `SerialHandler` strips the leading `+` ACK from the UART stream

### `LinuxMonitor` subclass

Linux target mode treats the ELF file as an executable:

- `serial` is actually a `subprocess.Popen(...)`
- `LinuxReader` reads 1 byte at a time from process stdout and emits `TAG_SERIAL`
- `serial_write()` writes to `proc.stdin`
- no GDB integration

This is a “simulator style” mode, and it has additional ELF existence requirements.

## Console input pipeline (keyboard → commands or serial write)

### `ConsoleReader` (`base/console_reader.py`)

Core behavior:

- `console.setup()` and `console.cleanup()` are used around the loop.
- On non-Windows, it configures the tty for a non-blocking-ish read:
  - `VMIN = 0`, `VTIME = 2` (0.2s)
  - Motivation: avoid `TIOCSTI` based cancellation (insecure, removed in newer kernels).
- In “test mode” (`ESP_IDF_MONITOR_TEST=1`) it avoids calling `console.getkey()` entirely to avoid PTY cancellation hangs.

On each keypress:

- `ConsoleParser.parse(c)` returns either:
  - `(TAG_KEY, str)` for bytes to write (after EOL translation), or
  - `(TAG_CMD, CMD_*)` for a command, or
  - `None` (when key was consumed as help/menu UI)
- Commands go to `cmd_queue` except for `CMD_STOP` which is queued in `event_queue` so it is handled last.

### `ConsoleParser` (`base/console_parser.py`)

This is the keybinding/menu state machine.

#### Important symbols (from `base/key_config.py`)

These are computed **at import time** from config:

- `MENU_KEY` (default `Ctrl-T`)
- `EXIT_KEY` (default `Ctrl-]`)
- command keys (reset/build/flash/toggles/etc.)
- `SKIP_MENU_KEY` boolean (special mode)

#### Parse rules (simplified)

```pseudo
if pressed MENU_KEY previously:
  handle menu command (Ctrl-T <x>)
elif SKIP_MENU_KEY and key is a command key:
  handle menu command (without needing menu prefix)
elif key == MENU_KEY:
  set _pressed_menu_key = true  # next key decides command
elif key == EXIT_KEY:
  return CMD_STOP
else:
  return TAG_KEY with EOL normalization
```

#### Command mapping

Menu command keys map to `CMD_*` values (from `base/constants.py`), notably:

- reset: `CMD_RESET`
- build & flash: `CMD_MAKE`
- app-flash: `CMD_APP_FLASH`
- toggle output: `CMD_OUTPUT_TOGGLE`
- toggle logging: `CMD_TOGGLE_LOGGING`
- toggle timestamps: `CMD_TOGGLE_TIMESTAMPS`
- enter bootloader/pause app: `CMD_ENTER_BOOT`
- stop: `CMD_STOP`

Help text is printed when you press `Ctrl-T Ctrl-H` (or `h`, `H`, `?`).

## Serial reading pipeline (device → bytes → reconnection)

### `SerialReader` (`base/serial_reader.py`)

#### Open behavior

If `serial_instance` is created with `do_not_open=True`, the reader opens it on its own thread:

1. Sets DTR/RTS to LOW prior to opening (`Reset._setRTS(LOW)`, `_setDTR(LOW)`).
2. `serial.open()`
3. Sets RTS HIGH then DTR HIGH (ordering chosen “to avoid reset”)
4. Optionally performs a hard reset (`Reset.hard()`) depending on `--no-reset` and GDB-related state.

#### Read loop

`SerialReader` reads:

- `serial.read(serial.in_waiting or 1)` when open
  - pushes `(TAG_SERIAL, data)` to `event_queue`
- On exceptions (device disappears, etc.)
  - prints error
  - closes the port
  - loops attempting to reopen with `RECONNECT_DELAY` from config (`esp-idf-monitor.cfg`) until success or monitor stops

#### Cancellation / stopping

`StoppableThread.stop()` sets `_thread = None`, calls `_cancel()`, then joins. `SerialReader._cancel()` calls `serial.cancel_read()` when supported.

#### “Open port attempts”

If opening the port fails at startup and `--open-port-attempts` wasn’t used (default `1`), it prints available ports and returns (exits reader).

## Main serial processing pipeline (bytes → lines → decoding hooks)

### `SerialHandler` (`base/serial_handler.py`)

This class does the heavy lifting of:

- splitting serial bytes into lines
- filtering lines (`--print_filter`)
- coloring (auto-color based on log level)
- detecting and decoding:
  - panic stack dumps
  - coredump blocks
  - binary log frames
  - possible PC addresses
  - GDB stub stop packets
- command handling (reset, build/flash, toggles)

### `SerialHandler.handle_serial_input()` (pseudocode)

This is the “hot path” for almost everything printed.

```pseudo
if start_cmd_sent:
  strip '+' ACK from incoming data

lines = splitdata(data)           # yields complete lines, keeps partial in _last_line_part

if binary_log_detected:
  try:
    (text_lines, _last_line_part) = binlog.convert_to_text(frame_bytes)
    for each text_line:
      print_colored(text_line)
      logger.handle_possible_pc_address_in_line(text_line)
    return
  except ValueError:
    exit binary mode, fall back to plain splitting of accumulated bytes

for each line in lines:
  if serial_check_exit and line.strip() == EXIT_KEY:
    raise SerialStopException   # only in test mode

  if target != linux:
    check_panic_decode_trigger(line.strip())

  with coredump.check(line.strip()):
    decoded_line = try decode utf-8 else decode ignore
    if line matches filter or forced:
      print_colored(line)
      compare_elf_sha256(decoded_line)
      logger.handle_possible_pc_address_in_line(line.strip())

  check_gdb_stub_and_run(line.strip())
  _force_line_print = false

if _last_line_part starts with CONSOLE_STATUS_QUERY:
  print it verbatim
  drop it from buffer

if finalize_line timer fired and filter matches partial:
  print partial, decode possible PC address, update buffers, clear _last_line_part
else:
  keep _last_line_part for later
```

### Handling “data without EOL” (flush timer)

Monitor schedules a timer (`LAST_LINE_THREAD_INTERVAL`, default `0.1s`) after each `TAG_SERIAL` event. If no more serial data arrives soon, the timer enqueues a `TAG_SERIAL_FLUSH` event.

`TAG_SERIAL_FLUSH` causes `SerialHandler.handle_serial_input(... finalize_line=...)` to run and optionally treat `_last_line_part` as a printable “completed line”.

This is a workaround for devices/prints that don’t include a newline. It intentionally avoids “finalizing” lines while a coredump is in progress, because coredump decoding uses empty lines as a terminator.

### Filtering (print_filter)

`base/line_matcher.py:LineMatcher` parses a filter string like:

- `*:I` (print INFO and above for all tags)
- `wifi:W` (print WARN and above for `wifi`)
- `main:V` (print everything for `main`)

It recognizes ESP-IDF log line prefix patterns and compares levels.

### Auto-coloring

`SerialHandler.print_colored()` wraps lines with ANSI colors unless `--disable-auto-color` is set.

It detects log level prefix via `AUTO_COLOR_REGEX` from `base/output_helpers.py`:

- `I (...)` → green
- `W (...)` → yellow
- `E (...)` → red

It handles partial lines by tracking `_trailing_color` and only resetting colors when a newline arrives.

## Address decoding (`addr2line` style) and logging

### `Logger` (`base/logger.py`)

`Logger` is the single printing abstraction used across the monitor:

- prints to terminal using `miniterm.Console.write_bytes`
- optionally writes to a log file (`toggle_logging()`)
- optionally injects timestamps at the start of each output line (`--timestamps`)

#### PC address decoding

If enabled, `Logger` uses `esp_idf_panic_decoder.PcAddressDecoder` (a dependency) to translate addresses found in the line:

- `logger.handle_possible_pc_address_in_line(line)`
  - Calls `pc_address_decoder.translate_addresses(...)`
  - Prints `--- <addr>: <func> at <file>:<line>` entries, including “(inlined by)” chains.

This is why ELF files are so important: without an ELF, the monitor degrades to a simpler display mode.

## Panic decoding (`--decode-panic backtrace`)

Panic decoding is implemented in `SerialHandler.check_panic_decode_trigger()` and uses `esp_idf_panic_decoder.PanicOutputDecoder`.

It is a small state machine driven by line contents:

- Trigger start: regex `PANIC_START` (e.g. “Core 0 register dump:”)
- Detect stack dump start: `PANIC_STACK_DUMP`
  - disables normal logging during stack memory dump
- Detect end: on an empty line after stack dump, it processes the buffered panic output

On success it prints a “Backtrace” section. On failure it prints warnings and prints the raw buffered portion to avoid losing data.

## Core dump decoding (`--decode-coredumps info`)

Core dump detection/processing is handled by `base/coredump.py:CoreDump`.

### State machine

Symbols:

- Start marker: `COREDUMP_UART_START`
- End marker: `COREDUMP_UART_END`
- Prompt: `COREDUMP_UART_PROMPT` (“Press Enter to print core dump to UART...”)

States:

- `COREDUMP_IDLE`
- `COREDUMP_READING`
- `COREDUMP_DONE`

### How it’s hooked into the hot path

`SerialHandler.handle_serial_input()` wraps processing of each stripped line in:

```python
with coredump.check(line_strip):
    ... print/decode ...
```

The context manager:

- checks for start/prompt/end markers **before** printing
- re-enables output **after** processing a full coredump block

### Processing behavior

When a coredump ends:

- If `--ws` is used, the coredump is written to a temp file and a WebSocket message is sent:
  - `{'event': 'coredump', 'file': <tmp>, 'prog': <elf>}`
  - Then it waits for `{'event': 'debug_finished'}`
- Otherwise it tries to import `esp_coredump` and runs:
  - `esp_coredump.CoreDump(...).info_corefile()` with stdout captured and printed
- If decoding fails, it prints the raw coredump block instead of losing it

## GDB stub detection and GDB launch

### Trigger detection (`GDBHelper.check_gdb_stub_trigger()`)

GDB stub output is detected via regex against serial bytes:

- pattern: `b'\\$(T..)#(..)'`
  - `T..` is the payload
  - `(..)` is hex checksum

The checksum is recomputed and compared. If valid:

- With `--ws`: a WebSocket `gdb_stub` event is emitted and monitor waits for `debug_finished`
- Without `--ws`: returns `True` to the monitor so it can launch GDB

The helper keeps a `gdb_buffer` to handle the case where the stop packet is split across serial reads.

### Launching GDB (`GDBHelper.run_gdb()`)

Builds a command like:

```text
<toolchain-prefix>gdb
  -ex "set serial baud <baud>"
  -ex "target remote <port>"
  <elf0>
  -ex "add-symbol-file <elf1>"
  -ex "add-symbol-file <elf2>"
  ...
```

Then it starts `subprocess.Popen(cmd)` and blocks until GDB exits, ignoring Ctrl-C in the monitor while GDB is running.

After exit:

- sets `gdb_exit = True`
- attempts `process.terminate()` and `stty sane` for tty recovery

### Post-GDB “continue” sequence

`SerialMonitor._main_loop()` checks `gdb_exit` and sends the monitor’s continue packet (`GDB_UART_CONTINUE_COMMAND`), then marks `serial_handler.start_cmd_sent = True` so the ACK `+` from the target is stripped from subsequent serial data.

## Reset behavior and custom reset sequences

### `Reset` (`base/reset.py`)

Monitor owns its own reset helper class, separate from esptool, but intentionally compatible with esptool’s configuration style.

#### `hard()`

Toggles RTS to reset the chip:

- RTS LOW (EN low) → wait → RTS HIGH (EN high)

Timing parameters come from `base/chip_specific_config.py` and vary by chip+revision.

#### `to_bootloader()`

Order of behavior:

1. If a config file defines `custom_reset_sequence`, it is executed.
2. Else if the port PID matches the ESP USB Serial/JTAG peripheral (`USB_JTAG_SERIAL_PID = 0x1001`), uses a JTAG-specific toggle sequence.
3. Else uses the “traditional” bootloader entry sequence (DTR/RTS toggles with chip-specific delays).

#### Custom reset sequences (format)

Monitor’s `Reset` supports a string format compatible with esptool’s `CustomReset`:

- commands separated by `|`
- each command begins with a letter:
  - `D` → set DTR (`0`/`1`)
  - `R` → set RTS (`0`/`1`)
  - `U` → set both DTR and RTS at once (Unix-only)
  - `W` → wait in seconds (float)

Example (classic bootloader entry):

```text
D1|R0|W0.1|D0|R1|W0.05|D1
```

Both esptool and esp-idf-monitor implement this by converting the string to Python statements and calling `exec(...)`. This means config files effectively contain “code”, which is powerful but has security implications if config files are untrusted.

### Config file discovery

Monitor’s `Config` implementation (`esp_idf_monitor/config.py`) searches:

1. current working directory
2. OS-specific config dir (`~/.config/esp-idf-monitor` on POSIX)
3. home directory

for one of:

- `esp-idf-monitor.cfg`
- `config.cfg`
- `tox.ini`

Or it can be specified explicitly with `ESP_IDF_MONITOR_CFGFILE`.

The `Reset` class also falls back to searching for an `esptool` config section if the monitor config file isn’t found or doesn’t contain `custom_reset_sequence`.

## WebSocket / IDE integration

When `--ws <url>` is used:

- `WebSocketClient` connects and sends JSON dictionaries
- monitor emits “debug events”:
  - `{'event': 'gdb_stub', 'port': ..., 'prog': ...}`
  - `{'event': 'coredump', 'file': ..., 'prog': ...}`
- monitor waits for:
  - `{'event': 'debug_finished'}`

This provides a way for an IDE to:

- take over GDB interaction
- decode coredumps out-of-process
- then signal monitor to resume

## Platform-specific behaviors and sharp edges

### Windows COM port name normalization

`idf_monitor.main()` rewrites `COMx` to `\\\\.\\COMx` for COM ports > 10 so GDB can open them.

### macOS `/dev/tty.*` normalization

`/dev/tty.*` is rewritten to `/dev/cu.*` to avoid GDB hangs on macOS.

### ANSI color conversion on Windows

`base/ansi_color_converter.py` wraps `sys.stderr` and console outputs to translate ANSI color sequences into Windows Console calls.

### TTY requirement

Monitor exits with an error if `stdin` is not a TTY (unless test mode). This is fundamental because the tool needs to capture interactive key input.

## “API Reference” (symbols and key methods)

This section is a developer-oriented symbol map. It is not a stable public API, but it’s how the code is structured today.

### `esp_idf_monitor.idf_monitor`

- `class Monitor`
  - `__init__(serial_instance, elf_files, print_filter, make, encrypted, reset, open_port_attempts, toolchain_prefix, eol, decode_coredumps, decode_panic, target, websocket_client, enable_address_decoding, timestamps, timestamp_format, force_color, disable_auto_color, rom_elf_file)`
  - `main_loop()`: run until stopped; owns shutdown sequencing
  - `_main_loop()`: consume one event and dispatch to handler
  - `run_make(target)`: run build target with monitor temporarily disabled
- `class SerialMonitor(Monitor)`
  - `serial_write(bytes)`, `check_gdb_stub_and_run(line)`, `_main_loop()`
- `class LinuxMonitor(Monitor)`
  - `serial_write(bytes)`
- `detect_port()`: select last connected port (Linux has special-case `/dev/ttyUSB0`)
- `main()`: CLI + orchestration

### `esp_idf_monitor.base.console_reader`

- `class ConsoleReader(StoppableThread)`
  - `run()`: reads keys, calls `ConsoleParser.parse`, enqueues events

### `esp_idf_monitor.base.console_parser`

- `class ConsoleParser`
  - `parse(key)`: produces `(TAG_KEY, str)` or `(TAG_CMD, CMD_*)`
  - `get_help_text()`, `get_next_action_text()`
  - `parse_next_action_key(key)`
- `prompt_next_action(reason, console, console_parser, event_queue, cmd_queue)`
  - used when build/flash command fails

### `esp_idf_monitor.base.serial_reader`

- `class SerialReader(Reader)`
  - `open_serial(reset: bool)`
  - `run()`: open/read/reconnect loop
  - `close_serial()`, `_disable_closing_wait_or_discard_data()`

### `esp_idf_monitor.base.serial_handler`

- `class SerialHandler`
  - `handle_serial_input(data, console_parser, coredump, gdb_helper, line_matcher, check_gdb_stub_and_run, finalize_line=False)`
  - `handle_commands(cmd, chip, run_make_func, console_reader, serial_reader)`
  - `splitdata(data)`, `print_colored(line)`
  - `check_panic_decode_trigger(line)`
- `class SerialHandlerNoElf(SerialHandler)`
  - reduced serial input handler when ELF files are missing

### `esp_idf_monitor.base.coredump`

- `class CoreDump`
  - `check(line)` context manager: detects coredump framing and manages output muting
  - `_process_coredump()`: uses `esp_coredump` or WebSocket

### `esp_idf_monitor.base.gdbhelper`

- `class GDBHelper`
  - `check_gdb_stub_trigger(line) -> bool`
  - `run_gdb()`

### `esp_idf_monitor.base.reset`

- `class Reset`
  - `hard()`
  - `to_bootloader()`
  - `_parse_string_to_seq(seq_str) -> str` (then executed via `exec`)

## Relationship to `esptool` (why we looked at it)

Monitor intentionally reuses “esptool-like” reset configuration:

- It can read `custom_reset_sequence` from an esptool config section.
- It implements the same string grammar (`D|R|U|W` commands with `|` separators).
- `esptool` itself implements the same via `exec(...)` in `.../site-packages/esptool/reset.py:CustomReset`.

Also, in ESP-IDF the “esptool” shipped under `components/esptool_py/esptool/esptool.py` is a wrapper that runs the installed `esptool` module (`python -m esptool`), just like `tools/idf_monitor.py` does for `esp_idf_monitor`.

## Practical “how to debug” checklist for new developers

- Confirm which monitor you’re running:
  - `python -c "import esp_idf_monitor,inspect; print(esp_idf_monitor.__file__)"`
- Run help to see arguments and defaults:
  - `python -m esp_idf_monitor --help`
- If keybindings behave oddly, check config discovery:
  - create `esp-idf-monitor.cfg` with `[esp-idf-monitor]` section in cwd or `~/.config/esp-idf-monitor/`
- If the device disappears/reconnects:
  - monitor will reconnect; adjust `reconnect_delay` in config if needed
- If address decoding is missing:
  - ensure ELF paths are correct and exist; otherwise monitor falls back to `SerialHandlerNoElf`
- If coredump decoding fails:
  - verify `esp_coredump` is installed in the IDF python env, or use `--decode-coredumps disable`
- If GDB doesn’t start:
  - verify `<toolchain-prefix>gdb` exists and `--toolchain-prefix` matches your toolchain
