---
Title: Serial Monitor as Stream Processor (Parsing, Multiplexing, Screenshots)
Ticket: ESP-01-SERIAL-MONITOR
Status: active
Topics:
    - serial
    - console
    - tooling
    - esp-idf
    - usb-serial-jtag
    - debugging
    - display
    - esp32s3
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/output_helpers.py
      Note: ANSI constants and auto-color regex
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/serial_handler.py
      Note: Line splitting
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/tools/idf_monitor.py
      Note: ESP-IDF wrapper that runs python -m esp_idf_monitor
    - Path: 0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: Device-side fixed-length PNG framing over USB Serial/JTAG (PNG_BEGIN <len>)
    - Path: 0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py
      Note: Host-side fixed-length PNG capture script
    - Path: 0025-cardputer-lvgl-demo/main/screenshot_png.cpp
      Note: Device-side streamed PNG framing (PNG_BEGIN 0) and SD save path
    - Path: 0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py
      Note: Host-side script triggering console command then capturing PNG (len>0 or len==0)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T14:05:53.363851032-05:00
WhatFor: ""
WhenToUse: ""
---


# Serial Monitor as Stream Processor (Parsing, Multiplexing, Screenshots)

## Executive Summary

We can treat the serial connection not as “a terminal with a wire attached” but as a *byte stream that happens to carry several logical kinds of information*: human-readable logs, operator input, structured events, and occasionally large binary payloads (screenshots, traces, dumps).

ESP-IDF’s monitor (`idf.py monitor` → `esp_idf_monitor`) already implements this idea in a limited way: it reads bytes, segments into lines, colors/filters logs, detects and decodes panics/core dumps, and triggers side-effect actions (reset, flash, run GDB). Separately, several past projects in this repo have implemented “binary-in-serial” protocols (notably screenshot PNG framing) using small host parsers.

This document proposes a unified design: a **Serial Monitor as Stream Processor**. The monitor remains a usable serial console, but it *parses* the incoming stream into a sequence of typed records and can route each record to the right sink (terminal UI, file on disk, image viewer, address decoder, etc.). The centerpiece is a small, explicit **framing layer** for structured payloads that:

- coexists with normal logs (including ANSI colors),
- is robust to partial reads and mixed content,
- can be extended without “regex spaghetti,” and
- allows the host to offer “serial capabilities” (reset/flash, capture, command injection, automation) without contaminating the device’s UART protocol traffic.

The design is intentionally conservative: it can support the existing `PNG_BEGIN … PNG_END` screenshot protocol immediately, and it can evolve toward a more general frame format with capability negotiation.

## Problem Statement

### The serial port is being asked to be too many things

In practice, we use one wire (UART or USB Serial/JTAG) for:

1. **Logs**: newline-delimited text, often ESP-IDF style, sometimes colored.
2. **Interactive console**: `esp_console` REPL prompts and user keystrokes.
3. **Diagnostics**: panics, stack traces, core dumps, gdbstub traffic.
4. **Binary payloads**: screenshots, file transfers, protocol traces.

When we “just print bytes,” these layers interfere with each other:

- Binary payloads can look like garbage in a terminal and can break UTF-8 decoding.
- Terminal color codes (ANSI escape sequences) can confuse naïve “parse logs with regex” scripts.
- Incomplete lines are common (serial reads are chunked), which breaks line-based assumptions.
- Host-side tooling often forks into one-off scripts that know exactly one framing format, then get lost.

### UART is often not “just UART” on ESP32-S3 boards

On Cardputer/ESP32-S3 projects, UART pins are frequently repurposed for peripherals. A UART console can corrupt protocol traffic (keyboard, Grove, host↔NCP links). The default and recommended console for these projects is **USB Serial/JTAG** (see `sdkconfig.defaults` guidance in this repo’s `AGENTS.md`).

So the “monitor” must be able to work well with USB Serial/JTAG, and it must be careful about mixing console traffic with other on-device UART protocols.

### We want “serial capabilities” without sacrificing “serial simplicity”

ESP-IDF monitor gives valuable capabilities:

- auto-coloring and filtering logs,
- timestamping and logging to file,
- decoding panics and core dumps,
- resetting and reflashing,
- detecting gdbstub and launching GDB.

But we now also want higher-level capabilities:

- capture screenshots over the same serial stream,
- parse structured events (JSON/CBOR/etc.),
- pipe certain frames to a file, a web UI, or a testing harness,
- automate workflows (e.g., “send console command, await frame, validate output”).

We need a design that scales: adding one new payload type should feel like adding a small parser + handler, not rewriting the monitor.

## Proposed Solution

### 1) Model: the monitor is a pipeline from bytes → records → sinks

We adopt a simple model:

- **Input** is a byte stream `B`.
- The monitor incrementally parses `B` into a stream of **records** `R = r1, r2, ...`.
- Each record has a *type* and *payload* (and optionally metadata like timestamps).
- Records are routed to **sinks** (terminal output, file, image viewer, decoder, websocket, etc.).

This is the same conceptual move that compilers make: from characters to tokens to syntax trees to code generation. We don’t need a full compiler; we need a disciplined tokenizer and a routing table.

### 2) Use-case anchor: screenshot PNG framing over serial

We already have a working “binary payload over serial” pattern:

**Project A: `0022-cardputer-m5gfx-demo-suite` (fixed-length PNG)**

- Device emits:

```text
PNG_BEGIN <len>
<raw PNG bytes length=<len>>
PNG_END
```

- Host script (`tools/capture_screenshot_png.py`) waits for the header, reads exactly `<len>` bytes, then consumes lines until it sees `PNG_END`.

**Project B: `0025-cardputer-lvgl-demo` (streamed PNG, unknown length up front)**

- Device emits:

```text
PNG_BEGIN 0
<raw PNG bytes streamed>
PNG_END
```

- Host script (`tools/capture_screenshot_png_from_console.py`) triggers the console command, then:
  - if `<len> > 0`: reads exact length;
  - if `<len> == 0`: parses the PNG format itself (signature + chunks until `IEND`) to know when the image ends.

**Observed failure mode and fix**

In the demo-suite screenshot work, PNG encoding (deflate/miniz) could overflow the default ESP-IDF `main` task stack. The fix was to run encode+send in a dedicated FreeRTOS task with a larger stack and block until completion (see the prior screenshot stack-overflow ticket notes).

These patterns are successful because they treat “binary over serial” as a *temporary mode switch* in the stream parser. The monitor can generalize this approach: detect a header, switch to a frame-reading state, then switch back to line-based log parsing.

### 3) Generalize: define a “structured frame” envelope, but keep text-first ergonomics

We can build on the screenshot protocol and make it systematic.

#### 3.1 Principles for in-band framing on serial

1. **Incremental**: must work with partial reads and arbitrarily chunked input.
2. **Recoverable**: if parsing fails, we must resynchronize without restarting the monitor.
3. **Coexistent**: logs remain logs; we don’t require “everything becomes frames.”
4. **Low surprise**: a plain serial terminal should still be usable (even if it prints raw frame bytes).

#### 3.2 A minimal generalized frame format (text header + binary payload)

We propose a generalized envelope similar to `PNG_BEGIN`:

```text
@ESPFRAME <type> <len> <encoding> <crc32>
<payload bytes length=<len> (or len=0 for streamed formats)>
@ESPFRAME_END
```

Where:

- `<type>` is a small identifier (e.g. `png`, `json`, `cbor`, `binlog`, `file`).
- `<encoding>` describes how to interpret payload (`raw`, `base64`, `zlib`, etc.).
- `<crc32>` is optional but strongly recommended for large binary frames.
- `<len>` may be `0` for “streamed, self-delimiting” formats (like parsing PNG chunks until `IEND`).

The important thing is not the literal syntax; it is the *separation of concerns*: a monitor can parse headers with a line scanner, then read payload bytes with an exact reader or a type-specific streaming parser.

#### 3.3 Capability negotiation (optional, but makes tooling pleasant)

The monitor should not guess what frames mean. The device can advertise:

```text
@ESPCAPS {"frames":["png","json"],"console":"esp_console","transport":"usb_serial_jtag","version":1}
```

This can be emitted at boot or on-demand (host sends a query, device responds). With capabilities, the monitor can:

- enable specialized handlers only when supported,
- choose safer defaults (e.g., prefer USB Serial/JTAG console),
- present a consistent “what can I do?” UX.

### 4) Borrow and extend ESP-IDF monitor’s architecture

ESP-IDF monitor is already a stream processor; we should learn from it and reuse its invariants.

At a high level:

- A **SerialReader** thread reads from pySerial and pushes `(TAG_SERIAL, bytes)` to an event queue.
- A **ConsoleReader** thread reads keystrokes and pushes either:
  - `(TAG_KEY, key)` to send to device, or
  - `(TAG_CMD, cmd)` to request a host-side action (reset, flash, toggle logging, etc.).
- The main thread consumes events and dispatches serial bytes to a **SerialHandler**.

Key details worth retaining:

1. **Line buffering with incomplete-line finalization**
   - Bytes are split into lines (`splitlines(keepends=True)`), but the handler retains the last incomplete line (`_last_line_part`) and can “finalize” it after a short timer if no newline arrives.
   - This is a practical fix for devices that print without EOL.

2. **Decoding and resilience**
   - The monitor attempts to decode bytes as text; repeated failures trigger a warning about baud/XTAL mismatches.
   - It continues operating even under decode errors; it does not crash.

3. **Auto-coloring**
   - If a line matches ESP-IDF log format (roughly `I/W/E (timestamp) TAG:`), the monitor prefixes an ANSI color and resets at newline boundaries.
   - It carefully handles partial lines (set color now, reset later).

4. **Print filtering**
   - A `LineMatcher` parses `--print_filter` into tag/level rules and filters lines without requiring firmware-side log level changes.

5. **Special decoders (panic/core dump/gdbstub)**
   - Panic decode can mute output while a stack dump is being collected, then print a decoded backtrace.
   - Core dump decode detects the “CORE DUMP START/END” markers, buffers base64 payload, and runs a decoder to produce a report (or prints raw if unavailable).
   - GDB stub detection watches for the `$T..#..` pattern and can launch GDB or trigger a websocket workflow.

6. **Binary log decoding**
   - A special “binary log mode” detects frames by first byte (`0x01`/`0x02`), validates CRC, and uses ELF lookups to convert binary log frames into text lines.
   - If binary framing fails, it falls back to normal text parsing.

Our “stream processor monitor” should preserve this “always recover, never wedge” behavior.

#### 4.1 Anatomy of `idf_monitor.py`: the wrapper versus the monitor

In ESP-IDF 5.4.1, `IDF_PATH/tools/idf_monitor.py` is intentionally tiny: it simply runs `python -m esp_idf_monitor ...`. The actual implementation lives in the Python package `esp_idf_monitor` inside the ESP-IDF Python environment. This distinction matters because:

- “Grepping `idf_monitor.py`” often finds only the wrapper.
- Customizing behavior usually means importing/extending classes from `esp_idf_monitor.*`.

#### 4.2 How bytes become colored, filtered lines

The core serial parsing logic lives in `esp_idf_monitor.base.serial_handler.SerialHandler`.

**(a) Chunked reads, not line reads**

`SerialReader` reads whatever is available (or 1 byte) using:

```text
data = serial.read(serial.in_waiting or 1)
```

So the handler must expect:

- partial lines,
- multiple lines in one chunk,
- no newline at all for long periods,
- non-text bytes (noise or binary payloads).

`SerialReader` also handles a lot of “real device” lifecycle:

- It can assert DTR/RTS in a specific order to avoid unintended resets.
- It can optionally reset the device on connect.
- If the port disappears, it prints status, waits, and reconnects (without wedging the main loop).

**(b) Incremental line splitting with an “incomplete tail”**

`splitdata()` does line splitting in the only safe way for a chunked stream:

- split on newlines *but keep line endings* (`splitlines(keepends=True)`),
- prepend any previously saved tail (`_last_line_part`) to the first piece,
- if the last piece does not end with `\n`, save it back into `_last_line_part`.

This is exactly the pattern we want for our own demux state machine: always keep the “unconsumed suffix” explicitly.

**(c) Auto-coloring**

Auto-coloring is driven by a simple heuristic:

- If the line matches ESP-IDF log format prefix `I/W/E (timestamp ...)`, color it:
  - `I` → green
  - `W` → yellow
  - `E` → red

It uses an internal `AUTO_COLOR_REGEX` and prepends the appropriate ANSI sequence. Two details are subtle and important:

1. **Reset behavior depends on whether the newline is present**
   - If the handler sees a full line (with newline), it prints `COLOR + line + RESET`.
   - If the handler sees a partial line (no newline), it prints `COLOR + partial` and remembers “a trailing color is active,” so it can insert a reset later when the newline finally arrives.

2. **Coloring is a presentation decision, not a parsing decision**
   - The line content is still the device’s line; color is host-side decoration. This is helpful: we can keep parsing based on the raw bytes or decoded text, and only decorate at the sink.

**(d) Print filtering**

Filtering (`--print_filter`) is implemented by `LineMatcher`:

- It parses filter terms like `TAG:W` into a dictionary mapping tags to numeric levels.
- It matches lines with a regex that tolerates optional ANSI prefixes.
- It decides “print or drop” per line; it doesn’t mutate the stream.

For our design, the key lesson is: filtering belongs after segmentation into lines/records, not at the raw byte level.

**(e) Decode failures are expected**

The handler tries `line.decode()` for filtering decisions and warnings. If it sees repeated decode failures, it prints a targeted warning (“check baud rate and XTAL frequency”). It does not crash. This is the correct posture for a stream processor on a noisy channel.

#### 4.3 How “decoding logs and all that” works: special recognizers with side effects

ESP-IDF monitor adds a set of recognizers that are conceptually the same as our proposed “frame handlers”: they watch the stream for certain patterns and then temporarily change how the stream is handled.

**(a) Panic decoding**

When enabled, the monitor watches for panic markers and stack dump phases, buffers the panic output, and then runs a decoder to print a friendlier backtrace. During certain phases it mutes normal output to avoid interleaving decoded output with raw stack dump lines.

**(b) Core dump decoding**

Core dump handling is implemented as a context manager `CoreDump.check(line)` around the “print this line” logic:

- It detects prompts that request pressing Enter, and can inject a newline automatically.
- It detects `CORE DUMP START/END` markers.
- While in-progress, it buffers the base64 payload and prints progress updates (“Received N kB...”).
- At end, it decodes to a human-readable report (or prints raw if the decoder is unavailable).

This is a good example of a *transactional mode*: the monitor temporarily changes behavior while a multi-line payload is being collected.

**(c) GDB stub detection**

GDB stub traffic is recognized by looking for a `$T..#..` “reason for break” message and verifying the checksum. If detected, the monitor can launch GDB (or notify a websocket client) and then resume monitoring afterwards.

**(d) Binary log decoding**

Binary logging introduces a second “payload mode” that is not line-based:

- The handler detects the mode by inspecting the first byte (`0x01`/`0x02`).
- It then treats the incoming bytes as frames, validates CRC, and decodes strings using ELF lookups.
- If framing fails, it exits binary mode and reprocesses the accumulated bytes as normal text.

This logic is directly relevant to screenshot framing: screenshots are another “payload mode” that wants to be decoded to something meaningful (an image file) and should not be forced through the text line pipeline.

#### 4.4 Console input: host commands versus device input

The console side of `esp_idf_monitor` is a neat, minimal separation of concerns:

- `ConsoleReader` reads raw keystrokes and feeds them to a `ConsoleParser`.
- `ConsoleParser` implements a “menu key” escape (by default, Ctrl-T):
  - normal keys become `(TAG_KEY, key)` and are sent to the device,
  - menu sequences become `(TAG_CMD, cmd)` and are handled by the monitor (reset, flash, toggle logging, etc.).

This pattern is exactly what we want for “offering serial capabilities” without interfering with a device console or protocol stream: keep a small, explicit control namespace on the host side.

Implementation note: on Unix-like systems, `ConsoleReader` uses a non-blocking read configuration (via `termios` VMIN/VTIME) specifically to avoid relying on insecure injection mechanisms to unblock reads. This is the kind of operational detail that matters if we build our own monitor: correctness includes not wedging the TTY.

### 5) Where the new parser fits

We insert one new concept: a **Frame Demultiplexer** that runs before (or alongside) line-based log handling.

Conceptually:

```
Serial bytes
  -> demux state machine
       -> TextLine records  -> log coloring/filtering/address decoding -> terminal/logfile
       -> Frame records     -> frame handlers (PNG saver, JSON event bus, etc.)
       -> Raw bytes (fallback) -> terminal (escaped) or hexdump
```

We can support existing screenshot framing by implementing a `PNG_BEGIN` handler as a first-class frame detector.

### 6) “Offering serial capabilities” without contaminating protocols

We distinguish:

- **Data plane**: the serial bytes coming from the device (logs + frames).
- **Control plane**: host-side commands and workflows (reset, flash, capture, tests).

The user’s mental model is:

- I am in a terminal; normal typing goes to the device.
- Certain key chords (like ESP-IDF monitor’s menu key) invoke host actions.
- The monitor may also react to frames the device emits (save an image, decode a dump).

This aligns with ESP-IDF’s `Ctrl-T` menu concept, but extends it: “Ctrl-T Ctrl-P” might trigger “request screenshot, then capture and open it,” for example.

## Design Decisions

### D1: Prefer USB Serial/JTAG for interactive consoles on ESP32-S3 boards

Rationale: UART pins are frequently repurposed; a UART console can corrupt protocol traffic. USB Serial/JTAG provides a dedicated debug channel on these boards.

Baseline config recommendation for firmwares that use a console:

```text
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

### D2: Use “text header + binary payload” framing as the first extension mechanism

Rationale: it matches existing successful screenshot protocols and can be parsed incrementally with minimal ambiguity. It also degrades gracefully: a plain terminal still shows the header and then “garbage bytes,” which is acceptable for debugging.

### D3: Keep parsing state explicit and small

Rationale: the failure mode of ad-hoc parsing is “it works until it doesn’t, then nobody knows why.” A small state machine (`TEXT`, `FRAME_HEADER`, `FRAME_PAYLOAD`) is easier to reason about and to test.

### D4: “Be liberal in what you accept,” but “be strict in what you emit”

Rationale: the host parser should tolerate slightly malformed frames and resynchronize, but our device-side emitters should follow a strict format with optional CRC to keep debugging sane.

### D5: Do not depend on UTF-8 always succeeding

Rationale: serial noise, binary payloads, and misconfigured baud/XTAL can produce invalid sequences. The monitor must treat bytes as bytes first, text second.

## Alternatives Considered

### A1: Base64 everything into log lines

Pros:
- Fully text-only; safe for terminals and loggers.
- Easy to multiplex with line-based tools.

Cons:
- 33% size overhead (or more), plus CPU overhead on both sides.
- Harder to stream large payloads efficiently.
- Still requires a “begin/end” discipline and careful resynchronization.

### A2: Pure binary framing (SLIP/COBS/CBOR) on the entire serial stream

Pros:
- Robust binary transport; clean delimiting and CRC.
- Easy to define an RPC/event protocol.

Cons:
- Breaks the “I can just open a terminal and read logs” ergonomics.
- Requires *all* firmware output to be encoded into frames, or a hard demux boundary that is difficult to guarantee without a second channel.

### A3: Split channels physically (second UART, USB CDC composite, Wi-Fi/WebSocket)

Pros:
- Strong separation: logs on one channel, binary frames on another.
- Avoids ambiguity and reduces parser complexity.

Cons:
- Not always available; UART pins may be in use, and not all environments can rely on Wi-Fi.
- More bring-up complexity.

Pragmatic conclusion: keep the “single stream” design, but structure it so we can move to multi-channel later if needed.

## Implementation Plan

### Phase 0: Document and unify existing screenshot framing

- Treat `PNG_BEGIN … PNG_END` as the first supported “structured frame type.”
- Provide a single ticket-local host script that can:
  - wait for a screenshot frame,
  - support `<len> > 0` and `<len> == 0` modes,
  - optionally trigger the console command (`screenshot`) before capturing.

### Phase 1: Build a demux library and a “tee monitor”

- Implement an incremental parser that can emit:
  - text chunks/lines,
  - screenshot frames,
  - unknown frames (captured as raw bytes with metadata).
- Wire it into a minimal monitor that:
  - prints logs to stdout/stderr (preserving existing ANSI),
  - saves screenshots to files,
  - still supports writing to the serial port (at minimum: send a command line).

### Phase 2: Integrate ESP-IDF monitor capabilities

Two options:

1. **Wrap/extend `esp_idf_monitor`**:
   - Intercept `(TAG_SERIAL, bytes)` events before `SerialHandler.handle_serial_input`.
   - Demux frames and pass only text/log bytes into the original handler.
   - Keep panic/core dump/gdb/binary log logic intact.

2. **Reimplement monitor with the same architecture**:
   - Reuse the proven threading/event-queue structure and re-implement only what we need.
   - This is more work, but offers cleaner extensibility.

### Phase 3: Add capability negotiation and richer frames

- Add optional `@ESPCAPS` emission on device side.
- Add `@ESPFRAME json …` frames for structured telemetry/events.
- Add additional payload types as needed (file transfer, metrics snapshot, etc.).

### Phase 4: Tests and resynchronization torture suite

- Feed the parser synthetic streams:
  - mixed logs + frames,
  - frames split across arbitrary chunk boundaries,
  - invalid UTF-8 sequences,
  - truncated frames, bad CRC, missing end markers.
- Ensure:
  - the parser always makes progress,
  - it resynchronizes predictably,
  - it never wedges the terminal output.

## Open Questions

1. **How strict should resynchronization be?**  
   For example, if we miss the header line, should we ever try to “find” a payload by scanning raw bytes? (For PNG, scanning for the signature is possible but can produce false positives.)

2. **Do we standardize on “len=0 means self-delimiting”?**  
   This is convenient for PNG and some chunked formats, but not all payload types are self-delimiting.

3. **Should we include CRC by default for all frames?**  
   CRC improves confidence and resync (you can reject corrupt frames), but it costs firmware CPU and code complexity.

4. **Where do we want the UI to live?**  
   Pure terminal (like `idf_monitor`), terminal with a side panel, or terminal + web UI (websocket sinks)?

5. **How do we keep “host commands” from colliding with “device console commands”?**  
   ESP-IDF uses a menu key to separate host commands. If we add more features, do we keep the same approach or introduce a structured host command mode?

## References

### Existing screenshot-over-serial projects

- `0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`
- `0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py`
- `0025-cardputer-lvgl-demo/main/screenshot_png.cpp`
- `0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py`

### ESP-IDF monitor (wrapper + implementation)

- `/home/manuel/esp/esp-idf-5.4.1/tools/idf_monitor.py` (thin wrapper that runs `python -m esp_idf_monitor`)
- `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/idf_monitor.py`
- `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/serial_handler.py` (line splitting, coloring, decoding hooks)
- `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/output_helpers.py` (ANSI constants + auto-color regex)
- `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/line_matcher.py` (print filtering)
- `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/coredump.py` (core dump detection and decoding)
- `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/gdbhelper.py` (gdbstub detection and launching)

### Related repo analyses

- `ttmp/2026/01/01/0026-B3-SCREENSHOT-STACKOVERFLOW--cardputer-demo-screenshot-png-send-triggers-stack-overflow-in-main/reference/01-diary.md`
- `ttmp/2026/01/02/0027-ESP-HELPER-TOOLING--esp-helper-tooling-go-debugging-flashing-helper/analysis/01-inventory-esp-idf-helper-scripts-tmux-serial-idf-py.md`
