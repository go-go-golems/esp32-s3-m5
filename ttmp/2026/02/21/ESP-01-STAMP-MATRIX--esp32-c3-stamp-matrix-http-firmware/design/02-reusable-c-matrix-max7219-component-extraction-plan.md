---
Title: Reusable C++ Matrix MAX7219 Component Extraction Plan
Ticket: ESP-01-STAMP-MATRIX
Status: active
Topics:
    - esp32
    - esp-idf
    - m5stack
    - led-matrix
    - wifi
    - rest
    - console
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: Legacy monolithic matrix + animation source mapped into C++ classes
    - Path: 0036-cardputer-adv-led-matrix-console/main/max7219.c
      Note: MAX7219 transport functions mapped into driver class
    - Path: 0036-cardputer-adv-led-matrix-console/main/max7219.h
      Note: Legacy constants and register contract mapped into types/config
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/app_main.c
      Note: Reference app_main startup wiring where parser registration will integrate
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp
      Note: Reference pattern for registering extra console command groups
    - Path: components/wifi_console/wifi_console.c
      Note: Console registration patterns and command style baseline
ExternalSources: []
Summary: Detailed extraction plan to convert the current C-based matrix and animation logic into a reusable C++ component, including an optional built-in command parser for easy esp_console registration.
LastUpdated: 2026-02-21T17:08:00-05:00
WhatFor: Define a reusable, testable C++ architecture for MAX7219 chain control, full animations, and drop-in console command registration.
WhenToUse: Use when implementing or reviewing componentization of matrix logic for firmware 0067 and future projects.
---


# Reusable C++ Matrix MAX7219 Component Extraction Plan

## 1) Goal and scope

The goal is to extract the existing matrix + MAX7219 + animation stack (currently centered in `0036-cardputer-adv-led-matrix-console/main/matrix_console.c`) into a reusable C++ component.

This extraction includes animation behavior and command parsing support, not only low-level drawing.

In scope:

- MAX7219 SPI transport and chain writes
- framebuffer model and flush logic
- text rendering (font + mapping)
- animation engine (scroll, wave, drop-bounce, spin, flipboard)
- runtime parameter mutation (fps, pause, intensity, chain, orientation)
- optional built-in command parser with easy `esp_console` registration

Out of scope:

- Cardputer-specific keyboard input logic (`kbd_*` path)
- Wi-Fi and HTTP stack implementation details

## 2) Why class-based extraction is needed

Current `0036` behavior is strong, but the implementation is monolithic:

- parser + engine + hardware write paths are tightly coupled
- many globals make multi-control-surface use risky
- unit testing is difficult because transport and state are hardwired

For `0067`, both REST and console must control one shared state machine. A class facade and explicit API eliminate drift and reduce race risk.

## 3) Target component layout

```text
components/led_matrix_max7219_cpp/
  CMakeLists.txt
  Kconfig
  include/
    led_matrix/matrix_types.hpp
    led_matrix/matrix_status.hpp
    led_matrix/max7219_driver.hpp
    led_matrix/matrix_framebuffer.hpp
    led_matrix/matrix_animator.hpp
    led_matrix/matrix_controller.hpp
    led_matrix/matrix_console_parser.hpp
  src/
    max7219_driver.cpp
    matrix_font_5x7.cpp
    matrix_framebuffer.cpp
    matrix_animator.cpp
    matrix_controller.cpp
    matrix_console_parser.cpp
  test/
    test_matrix_mapping.cpp
    test_matrix_animator.cpp
    test_matrix_console_parser.cpp
```

Integration model:

- REST adapter calls `MatrixController`
- built-in parser (or custom adapter) calls `MatrixController`
- all front doors share one process-level instance

## 4) Core class architecture

## 4.1 Facade class: `MatrixController`

`MatrixController` owns hardware object, animation runtime, synchronization, and worker task.

```cpp
class MatrixController {
public:
  esp_err_t init(const MatrixConfig& cfg);
  esp_err_t reconfigure(const MatrixConfig& cfg);

  esp_err_t clear();
  esp_err_t setText(const std::string& text);

  esp_err_t startScroll(const ScrollConfig& cfg);
  esp_err_t startWave(const WaveConfig& cfg);
  esp_err_t startDropBounce(const DropConfig& cfg);
  esp_err_t startSpin(const SpinConfig& cfg);
  esp_err_t startFlipboard(const FlipConfig& cfg);
  esp_err_t stopAnimation();

  esp_err_t setIntensity(uint8_t intensity);
  esp_err_t setSpiHz(uint32_t hz);
  esp_err_t setChainLen(uint8_t chain_len);
  esp_err_t setOrientation(bool reverse_modules, bool flip_vertical);

  MatrixStatus getStatus() const;
};
```

Rule:

- parser and REST handlers never write internal fields directly.

## 4.2 Transport class: `Max7219Driver`

Thin hardware wrapper around SPI + MAX7219 register protocol.

Responsibilities:

- open bus/device
- init register sequence
- write row chain (with daisy-chain reverse mapping)
- set intensity/test mode
- runtime SPI clock update

## 4.3 State/render classes

- `MatrixFramebuffer`: deterministic bit-level model (`8 x chain_len`)
- `MatrixFont5x7`: glyph lookup and text->columns helpers

## 4.4 Animation runtime: `MatrixAnimator`

Mode engine preserving existing `0036` behavior:

- scroll
- wave
- drop-bounce physics
- spin letters
- flipboard transitions

The drop physics equations should remain behavior-identical on first extraction pass.

## 4.5 Built-in parser class: `MatrixConsoleParser`

The component includes an optional parser so it can be registered in one line from firmware code.

```cpp
struct MatrixConsoleParserConfig {
  const char* command_name = "matrix";
  const char* help = "MAX7219 matrix control";
};

class MatrixConsoleParser {
public:
  explicit MatrixConsoleParser(MatrixController& controller);
  esp_err_t registerWithEspConsole(const MatrixConsoleParserConfig& cfg = {});

private:
  static int entryPoint(int argc, char** argv);
  int handle(int argc, char** argv);
  MatrixController& controller_;
};
```

Design intent:

- parser is packaged with the component for convenience
- parser remains a thin dispatch layer over facade API

## 5) Data model

```cpp
enum class MatrixMode {
  Idle,
  StaticText,
  Scroll,
  Wave,
  DropBounce,
  Spin,
  Flipboard,
};

struct MatrixConfig {
  spi_host_device_t spi_host;
  int pin_mosi;
  int pin_sck;
  int pin_cs;
  uint8_t chain_len;
  uint32_t spi_hz;
  bool reverse_modules;
  bool flip_vertical;
};

struct MatrixStatus {
  bool ready;
  MatrixMode mode;
  uint8_t chain_len;
  uint16_t width;
  uint8_t intensity;
  uint32_t spi_hz;
  uint16_t fps;
  uint32_t pause_ms;
  bool reverse_modules;
  bool flip_vertical;
  std::string text;
};
```

## 6) Threading and synchronization model

Single-owner mutable state:

- one process-level `MatrixController`
- one internal mutex guarding mutable state
- one worker task rendering frames

Worker loop (conceptual):

```cpp
void MatrixController::workerLoop() {
  for (;;) {
    waitForNotifyOrTimeout();

    StateSnapshot s;
    {
      LockGuard g(mu_);
      if (shutdown_) break;
      s = snapshotLocked();
    }

    Frame next = animator_.renderNext(s, tick_ms_);

    {
      LockGuard g(mu_);
      fb_ = next;
      flushFramebufferLocked();
    }
  }
}
```

Lifecycle:

```text
init -> create worker
start* -> set mode/config + notify
stop -> mode idle
deinit -> shutdown + worker teardown
```

## 7) Legacy-to-class mapping

## 7.1 Hardware mapping

| Legacy | New |
|---|---|
| `max7219_open` | `Max7219Driver::open` |
| `max7219_init` | `Max7219Driver::initRegs` |
| `max7219_set_row_chain` | `Max7219Driver::writeRowChain` |
| `max7219_set_spi_clock_hz` | `Max7219Driver::setClockHz` |

## 7.2 Rendering and flush mapping

| Legacy | New |
|---|---|
| `fb_from_cols` | `MatrixFramebuffer::loadFromCols` |
| `fb_flush_row` | `MatrixController::flushRowLocked` |
| `fb_flush_all` | `MatrixController::flushAllLocked` |
| `render_text_centered_cols` | `MatrixFont5x7::renderCentered` |

## 7.3 Animation mapping

| Legacy block | New |
|---|---|
| `scroll_task` | `MatrixAnimator::tickScroll` |
| `text_anim_task (wave)` | `MatrixAnimator::tickWave` |
| `text_anim_task (drop)` | `MatrixAnimator::tickDropBounce` |
| `text_anim_task (spin)` | `MatrixAnimator::tickSpin` |
| `text_anim_task (flipboard)` | `MatrixAnimator::tickFlipboard` |
| `drop_rebuild_seq` | `DropBounceModel::rebuildSequence` |

## 7.4 Parser mapping

| Command | Dispatch target |
|---|---|
| `matrix status` | `controller.getStatus()` |
| `matrix text <TEXT>` | `controller.setText(...)` |
| `matrix scroll on ...` | `controller.startScroll(...)` |
| `matrix scroll off` | `controller.stopAnimation()` |
| `matrix anim drop ...` | `controller.startDropBounce(...)` |
| `matrix anim wave ...` | `controller.startWave(...)` |
| `matrix anim spin ...` | `controller.startSpin(...)` |
| `matrix anim flip ...` | `controller.startFlipboard(...)` |
| `matrix anim off` | `controller.stopAnimation()` |
| `matrix intensity ...` | `controller.setIntensity(...)` |
| `matrix spi ...` | `controller.setSpiHz(...)` |
| `matrix chain ...` | `controller.setChainLen(...)` |
| `matrix reverse ...` | `controller.setOrientation(...)` |
| `matrix flipv ...` | `controller.setOrientation(...)` |

## 8) Easy console registration path

Firmware integration example:

```cpp
static MatrixController g_matrix;
static MatrixConsoleParser g_matrix_parser(g_matrix);

void app_main(void) {
  MatrixConfig cfg = {/* pins and defaults */};
  ESP_ERROR_CHECK(g_matrix.init(cfg));

  MatrixConsoleParserConfig pcfg = {};
  pcfg.command_name = "matrix";
  ESP_ERROR_CHECK(g_matrix_parser.registerWithEspConsole(pcfg));
}
```

For projects that already have custom parser style, this can be skipped and the controller API called directly.

## 9) REST + parser consistency contract

Both control planes must call the same facade methods. No duplicate state is allowed in adapters.

Examples:

- REST `POST /api/matrix/anim {"mode":"drop"...}` -> `startDropBounce`
- parser `matrix anim drop ...` -> `startDropBounce`

This keeps behavior parity and simplifies debugging.

## 10) Configuration and build toggles

Component-level Kconfig should include:

```text
menu "LED Matrix MAX7219 C++"
  config LED_MATRIX_MAX_CHAIN_LEN
  config LED_MATRIX_DEFAULT_CHAIN_LEN
  config LED_MATRIX_DEFAULT_SPI_HZ
  config LED_MATRIX_WORKER_STACK
  config LED_MATRIX_WORKER_PRIO
  config LED_MATRIX_ENABLE_CONSOLE_PARSER
endmenu
```

Project-level `Kconfig.projbuild` still defines board pin defaults (`MOSI=4`, `CS=5`, `SCK=6` for STAMP C3).

## 11) Testing strategy

Logic tests:

- glyph mapping
- column/framebuffer mapping
- drop sequence generation
- flipboard transition progression

Integration tests:

- SPI mock to assert row ordering and chain byte reversal
- parser tests for representative argv command sets

On-device smoke:

- boot + ids pattern
- each animation mode start/stop
- rapid mode changes from REST while parser commands run
- chain length changes under idle and active modes

## 12) Migration phases

Phase A:

- mechanical extraction into classes
- compile and basic draw

Phase B:

- mode-by-mode parity vs `0036`
- lock in drop/wave/flip timing tests

Phase C:

- integrate parser + REST adapters onto shared facade
- remove duplicated command-side state

Phase D:

- hardening (memory ownership, status snapshots, perf tuning)

## 13) Risks and mitigations

Risk: animation feel regression.

- Mitigation: preserve formulas/constants first, optimize later.

Risk: deadlock in worker/API paths.

- Mitigation: strict lock scopes, no external callbacks under lock.

Risk: control-plane race (REST vs parser).

- Mitigation: single facade API and single mutex-protected state.

Risk: parser coupling too tightly to core.

- Mitigation: keep parser optional behind `LED_MATRIX_ENABLE_CONSOLE_PARSER` and isolated in its own files.

## 14) Recommended starting skeleton

```cpp
class MatrixController {
public:
  esp_err_t init(const MatrixConfig& cfg);
  esp_err_t startDropBounce(const DropConfig& cfg);
  esp_err_t startScroll(const ScrollConfig& cfg);
  esp_err_t stopAnimation();
  MatrixStatus getStatus() const;
};

class MatrixConsoleParser {
public:
  explicit MatrixConsoleParser(MatrixController& controller);
  esp_err_t registerWithEspConsole(const MatrixConsoleParserConfig& cfg = {});
};
```

This gives immediate reuse value and enables straightforward registration in firmware entrypoints.

## 15) Bottom line

The reusable C++ module should ship with:

- stable `MatrixController` facade
- full animation engine parity
- optional built-in parser for direct console registration

That combination gives 0067 fast integration while preserving long-term reuse across future matrix projects.
