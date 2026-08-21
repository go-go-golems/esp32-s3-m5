#!/usr/bin/env python3
"""Instrument the exact official Arduino comparison workspace without serial observer effects."""

from pathlib import Path
import argparse
import shutil

TRACE_IMPL = r'''
#include "esp60_m5_i2c_trace.h"
#include <esp_timer.h>

namespace {
constexpr size_t ESP60_TRACE_CAPACITY = 6000;
constexpr uint8_t ESP60_ST25R3916_ADDRESS = 0x50;

struct Esp60TraceContext {
    bool active{};
    uint8_t address{};
    uint8_t key{};
    uint8_t failure_stage{};
    uint16_t write_len{};
    uint16_t read_len{};
    int64_t started_us{};
};

Esp60TraceContext esp60_contexts[2]{};
esp60_m5_i2c_event_t esp60_events[ESP60_TRACE_CAPACITY]{};
esp60_m5_i2c_stats_t esp60_stats{};
size_t esp60_head{};
size_t esp60_count{};

Esp60TraceContext& esp60_context(int port)
{
    static Esp60TraceContext invalid{};
    return (port >= 0 && port < 2) ? esp60_contexts[port] : invalid;
}

void esp60_complete(int port)
{
    auto& context = esp60_context(port);
    if (!context.active) return;
    context.active = false;
    if (context.address != ESP60_ST25R3916_ADDRESS) return;

    esp60_m5_i2c_event_t event{};
    event.sequence = ++esp60_stats.total;
    event.timestamp_us = static_cast<uint32_t>(context.started_us);
    event.elapsed_us = static_cast<uint32_t>(esp_timer_get_time() - context.started_us);
    event.write_len = context.write_len;
    event.read_len = context.read_len;
    event.kind = context.write_len && context.read_len ? ESP60_M5_I2C_WRITE_READ
               : context.read_len ? ESP60_M5_I2C_READ : ESP60_M5_I2C_WRITE;
    event.key = context.key;
    event.failure_stage = context.failure_stage;
    if (context.failure_stage == ESP60_M5_I2C_FAIL_NONE) ++esp60_stats.succeeded;
    else ++esp60_stats.failed;

    if (esp60_count == ESP60_TRACE_CAPACITY) {
        esp60_events[esp60_head] = event;
        esp60_head = (esp60_head + 1) % ESP60_TRACE_CAPACITY;
        ++esp60_stats.dropped;
    } else {
        esp60_events[(esp60_head + esp60_count) % ESP60_TRACE_CAPACITY] = event;
        ++esp60_count;
    }
    esp60_stats.buffered = esp60_count;
}

void esp60_note_write(int port, const uint8_t* data, size_t length, bool ok)
{
    auto& context = esp60_context(port);
    if (!context.active) return;
    if (context.write_len == 0 && data && length) context.key = data[0];
    context.write_len = static_cast<uint16_t>(context.write_len + length);
    if (!ok) context.failure_stage |= ESP60_M5_I2C_FAIL_WRITE;
}
}  // namespace

extern "C" void esp60_m5_i2c_trace_reset(void)
{
    esp60_stats = {};
    esp60_head = 0;
    esp60_count = 0;
}

extern "C" void esp60_m5_i2c_trace_get_stats(esp60_m5_i2c_stats_t* out)
{
    if (out) {
        esp60_stats.buffered = esp60_count;
        *out = esp60_stats;
    }
}

extern "C" size_t esp60_m5_i2c_trace_drain(esp60_m5_i2c_event_t* out, size_t capacity)
{
    if (!out || !capacity) return 0;
    const size_t amount = capacity < esp60_count ? capacity : esp60_count;
    for (size_t i = 0; i < amount; ++i) {
        out[i] = esp60_events[(esp60_head + i) % ESP60_TRACE_CAPACITY];
    }
    esp60_head = (esp60_head + amount) % ESP60_TRACE_CAPACITY;
    esp60_count -= amount;
    esp60_stats.buffered = esp60_count;
    return amount;
}
'''


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if text.count(old) != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {text.count(old)}")
    return text.replace(old, new, 1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("workspace", type=Path)
    args = parser.parse_args()
    workspace = args.workspace.resolve()
    here = Path(__file__).resolve().parent.parent
    artifacts = here / "sources/code/arduino-trace"
    source = workspace / ".pio/libdeps/cores3/M5Unified/src/utility/I2C_Class.cpp"
    include_dir = workspace / "include"
    sketch = workspace / "src/main.cpp"
    if not source.exists():
        raise SystemExit(f"missing {source}; run `pio pkg install -e cores3` first")

    include_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(artifacts / "esp60_m5_i2c_trace.h", include_dir)
    shutil.copy2(artifacts / "Detect-traced.cpp", sketch)

    # PlatformIO does not expose a project's include/ directory while compiling
    # dependency sources, so add it explicitly for patched M5Unified.
    ini_path = workspace / "platformio.ini"
    ini = ini_path.read_text()
    include_flag = f"  -I{include_dir}"
    if include_flag not in ini:
        ini = replace_once(ini, "build_flags =\n", f"build_flags =\n{include_flag}\n", "build_flags")
        ini_path.write_text(ini)

    text = source.read_text()
    marker = "ESP60_TRACE_CAPACITY"
    if marker in text:
        print(f"already instrumented: {source}")
        return
    text = replace_once(text, '#include <M5GFX.h>\n', '#include <M5GFX.h>\n' + TRACE_IMPL + '\n', "trace implementation")

    text = replace_once(text,
'''  bool I2C_Class::start(std::uint8_t address, bool read, std::uint32_t freq) const
  {
    return m5gfx::i2c::beginTransaction(_port_num, address, freq, read).has_value();
  }
''',
'''  bool I2C_Class::start(std::uint8_t address, bool read, std::uint32_t freq) const
  {
    auto& trace = esp60_context(_port_num);
    trace = {};
    trace.active = true;
    trace.address = address;
    trace.started_us = esp_timer_get_time();
    if (read) trace.read_len = 0;
    const bool ok = m5gfx::i2c::beginTransaction(_port_num, address, freq, read).has_value();
    if (!ok) {
      trace.failure_stage |= ESP60_M5_I2C_FAIL_START;
      esp60_complete(_port_num);
    }
    return ok;
  }
''', "start")

    text = replace_once(text,
'''  bool I2C_Class::restart(std::uint8_t address, bool read, std::uint32_t freq) const
  {
    return m5gfx::i2c::restart(_port_num, address, freq, read).has_value();
  }
''',
'''  bool I2C_Class::restart(std::uint8_t address, bool read, std::uint32_t freq) const
  {
    const bool ok = m5gfx::i2c::restart(_port_num, address, freq, read).has_value();
    auto& trace = esp60_context(_port_num);
    if (!ok && trace.active) {
      trace.failure_stage |= ESP60_M5_I2C_FAIL_RESTART;
      esp60_complete(_port_num);
    }
    return ok;
  }
''', "restart")

    text = replace_once(text,
'''  bool I2C_Class::stop(void) const
  {
    return m5gfx::i2c::endTransaction(_port_num).has_value();
  }
''',
'''  bool I2C_Class::stop(void) const
  {
    const bool ok = m5gfx::i2c::endTransaction(_port_num).has_value();
    auto& trace = esp60_context(_port_num);
    if (!ok && trace.active) trace.failure_stage |= ESP60_M5_I2C_FAIL_STOP;
    esp60_complete(_port_num);
    return ok;
  }
''', "stop")

    text = replace_once(text,
'''  bool I2C_Class::write(std::uint8_t data) const
  {
    return m5gfx::i2c::writeBytes(_port_num, &data, 1).has_value();
  }
''',
'''  bool I2C_Class::write(std::uint8_t data) const
  {
    const bool ok = m5gfx::i2c::writeBytes(_port_num, &data, 1).has_value();
    esp60_note_write(_port_num, &data, 1, ok);
    return ok;
  }
''', "write byte")

    text = replace_once(text,
'''  bool I2C_Class::write(const std::uint8_t* __restrict__ data, std::size_t length) const
  {
    return m5gfx::i2c::writeBytes(_port_num, data, length).has_value();
  }
''',
'''  bool I2C_Class::write(const std::uint8_t* __restrict__ data, std::size_t length) const
  {
    const bool ok = m5gfx::i2c::writeBytes(_port_num, data, length).has_value();
    esp60_note_write(_port_num, data, length, ok);
    return ok;
  }
''', "write buffer")

    text = replace_once(text,
'''  bool I2C_Class::read(std::uint8_t* __restrict__ result, std::size_t length, bool last_nack) const
  {
    return m5gfx::i2c::readBytes(_port_num, result, length, last_nack).has_value();
  }
''',
'''  bool I2C_Class::read(std::uint8_t* __restrict__ result, std::size_t length, bool last_nack) const
  {
    const bool ok = m5gfx::i2c::readBytes(_port_num, result, length, last_nack).has_value();
    auto& trace = esp60_context(_port_num);
    if (trace.active) {
      trace.read_len = static_cast<uint16_t>(trace.read_len + length);
      if (!ok) trace.failure_stage |= ESP60_M5_I2C_FAIL_READ;
    }
    return ok;
  }
''', "read")

    source.write_text(text)
    print(f"instrumented: {source}")
    print(f"installed traced sketch: {sketch}")


if __name__ == "__main__":
    main()
