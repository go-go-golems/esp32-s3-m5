---
Title: 0067 ESP-C3 LED Matrix HTTP Firmware Architecture and Intern Guide
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
      Note: Source of scroll
    - Path: 0036-cardputer-adv-led-matrix-console/main/max7219.c
      Note: MAX7219 chain driver behavior and SPI runtime tuning reused in 0067 plan
    - Path: 0036-cardputer-adv-led-matrix-console/main/max7219.h
      Note: MAX7219 defaults and register contract analyzed for C3 pin migration
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/Kconfig.projbuild
      Note: Reference for project-level configurable GPIO/menuconfig patterns
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/app_main.c
      Note: on_got_ip startup pattern for deferred HTTP start
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/http_server.c
      Note: Minimal REST JSON parsing/response pattern for matrix API
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild
      Note: Reference Kconfig structure for tutorial-scoped runtime config
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp
      Note: Pattern for wifi_console register_extra command registration
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp
      Note: Route scaling and max_uri_handlers hardening patterns
    - Path: 0067-esp-c3-led-matrix-http/main/app_main.c
      Note: Actual startup wiring used in validated implementation
    - Path: 0067-esp-c3-led-matrix-http/main/http_server.c
      Note: Actual REST endpoint implementation
    - Path: 0067-esp-c3-led-matrix-http/main/matrix_engine.c
      Note: Actual animation and framebuffer implementation
    - Path: 0067-esp-c3-led-matrix-http/sdkconfig.defaults
      Note: Console backend configuration validated on hardware
    - Path: components/wifi_console/wifi_console.c
      Note: Shared esp_console Wi-Fi commands and backend selection strategy
    - Path: components/wifi_mgr/wifi_mgr.c
      Note: Shared Wi-Fi STA manager architecture reused directly
ExternalSources:
    - https://docs.m5stack.com/en/core/stamp_c3
    - https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html
Summary: Architecture and implementation guide for firmware 0067 that ports Cardputer MAX7219 matrix animation to M5Stamp C3 and adds Wi-Fi REST plus esp_console control.
LastUpdated: 2026-02-21T16:20:00-05:00
WhatFor: Deep technical onboarding and implementation planning document for combining existing matrix animation, Wi-Fi manager, and HTTP patterns into a new ESP32-C3 firmware.
WhenToUse: Use when implementing, reviewing, testing, or extending 0067-esp-c3-led-matrix-http.
---



# 0067 ESP-C3 LED Matrix HTTP Firmware Architecture and Intern Guide

## 1) Why this firmware exists

`0067-esp-c3-led-matrix-http` is the next integration firmware in this repository. Its purpose is to combine three things we already built separately:

1. A working 12x chained MAX7219 8x8 matrix text/animation engine with bounce and wave behaviors.
2. A production-proven Wi-Fi runtime configuration flow over `esp_console`.
3. A REST control plane over `esp_http_server`.

The target hardware is M5Stack STAMP C3, driving a 12-module MAX7219 chain over SPI with this user-provided mapping:

- `DIN` -> `GPIO4`
- `CS` -> `GPIO5`
- `CLK` -> `GPIO6`

The user goal is specifically "bouncing text" previously built on Cardputer, now controlled over Wi-Fi REST with Wi-Fi console support.

## 2) Executive architecture in one page

### System shape

```text
                        +------------------------------+
                        | USB Serial/JTAG esp_console  |
                        | prompt: c3m>                 |
                        +---------------+--------------+
                                        |
                                        | matrix / wifi commands
                                        v
+-------------------+       +-----------+------------+       +------------------+
| Wi-Fi STA Manager |<----->| Matrix Control Service |<----->| MAX7219 HAL      |
| (wifi_mgr)        |       | (state + animations)   |       | (SPI + registers)|
+---------+---------+       +-----------+------------+       +---------+--------+
          |                             ^                              |
          | got IP callback             | frame buffer flush           |
          v                             |                              v
+---------+-----------------------------+---------------+     12 x 8x8 modules
| HTTP Server (esp_http_server)                         |     daisy chained
| /api/matrix/*                                         |
| /api/status                                           |
+-------------------------------------------------------+
```

### Design principle

One shared matrix engine state, two front doors:

- console commands (`matrix ...`, `wifi ...`)
- REST endpoints (`/api/matrix/...`)

Both mutate the same engine through one mutex-protected API. This avoids drift where console behavior and HTTP behavior diverge.

## 3) What exists today: deep code analysis

This section maps exactly what to reuse and what to refactor.

### 3.1 Reuse map

| Source | Reuse level | Why it matters |
|---|---|---|
| `0036-cardputer-adv-led-matrix-console/main/max7219.c` | Reuse almost as-is | Already stable MAX7219 chain driver and row mapping |
| `0036-cardputer-adv-led-matrix-console/main/matrix_console.c` | Partial extraction | Contains full text + bounce + wave + spin + flip animation logic |
| `components/wifi_mgr/wifi_mgr.c` | Reuse as-is | Shared STA state machine, NVS credentials, retry policy |
| `components/wifi_console/wifi_console.c` | Reuse as-is | Shared runtime Wi-Fi REPL with backend auto-selection |
| `0065-xiao-esp32c6-gpio-web-server/main/http_server.c` | Reuse pattern | Clean small REST JSON parse/respond scaffolding |
| `0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp` | Reuse startup pattern | Demonstrates `wifi_console.register_extra` integration |
| `0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp` | Reuse hardening ideas | `max_uri_handlers`, larger route sets, robust body parsing |

### 3.2 Matrix HAL findings (`0036`)

Driver contract lives in:

- `0036-cardputer-adv-led-matrix-console/main/max7219.h`
- `0036-cardputer-adv-led-matrix-console/main/max7219.c`

Key facts:

- Chain length max is 16 (`MAX7219_MAX_CHAIN_LEN`) and default 12 (`MAX7219_DEFAULT_CHAIN_LEN`), see `max7219.h:14-16`.
- Current hardcoded pins are Cardputer-oriented (`SCK=40`, `MOSI=14`, `CS=5`), see `max7219.h:11-13`.
- Daisy-chain byte order is already handled correctly by reversing source order in `max7219_set_row_chain`, see `max7219.c:180-186`.
- SPI rate is runtime adjustable and clamped to `1k..10M`, see `max7219.c:5-8` and `max7219.c:126-151`.

Conclusion:

- Keep the HAL mostly unchanged.
- Replace default pin constants with Kconfig-driven values for STAMP C3 (`DIN=4`, `CS=5`, `CLK=6`).
- Keep chain-byte reversal logic exactly as-is.

### 3.3 Animation engine findings (`0036`)

All animation and command logic sits in one large file:

- `0036-cardputer-adv-led-matrix-console/main/matrix_console.c` (2375 lines)

High-value reusable pieces:

- Framebuffer model `s_fb[8][MAX7219_MAX_CHAIN_LEN]`, see `matrix_console.c:34`.
- Scroll task and text-column generation, see `matrix_console.c:748-823` and `matrix_console.c:1069-1137`.
- Drop-bounce physics integrator, see `matrix_console.c:906-970`.
- Wave, spin, flipboard animation loops, see `matrix_console.c:1153-1463`.
- Runtime controls for fps/pause/bounce coefficients in command parser, see `matrix_console.c:1819-1963`.

What is Cardputer-specific and should be removed for 0067:

- TCA8418 keyboard stack (`kbd_*` path), see `matrix_console.c:48-75`, `matrix_console.c:210-301`, `matrix_console.c:1519-1593`.
- Any key-to-text feed assumptions.

Architectural concern:

- In `0036`, console parsing, matrix state, and animation tasks are tightly coupled in one translation unit with many file-scope globals. This made bring-up fast, but it is not ideal for dual-frontdoor control (REST + console).

Refactor direction:

- Keep the math and rendering primitives.
- Move command parsing and HTTP handlers into thin adapters that call one shared service API.

### 3.4 Wi-Fi manager findings (shared component)

Files:

- `components/wifi_mgr/wifi_mgr.c`
- `components/wifi_mgr/include/wifi_mgr.h`
- `components/wifi_mgr/Kconfig`

Strengths:

- Robust startup + NVS init with repair path, see `wifi_mgr.c:77-98` and `wifi_mgr.c:306-334`.
- Event-driven state transitions and retry policy, see `wifi_mgr.c:197-283` and `wifi_mgr.c:233-257`.
- Callback on got IP for deferred server startup, see `wifi_mgr.c:286-292` and `wifi_mgr.c:260-275`.
- Runtime and persisted credentials separation, see `wifi_mgr.c:106-151` and `wifi_mgr.c:352-404`.

Implication for 0067:

- Reuse fully; do not fork unless C3-specific issue appears.

### 3.5 Wi-Fi console findings (shared component)

Files:

- `components/wifi_console/wifi_console.c`
- `components/wifi_console/include/wifi_console.h`
- `components/wifi_console/Kconfig`

Key behavior:

- Provides `wifi status|scan|join|set|connect|disconnect|clear`, see `wifi_console.c:62-73` and parser at `wifi_console.c:88-287`.
- Supports registering extra command groups, see `wifi_console.c:289-298`.
- Selects backend based on sdkconfig: USB Serial/JTAG, USB CDC, or UART, see `wifi_console.c:309-323`.

This component is exactly what 0067 needs for Wi-Fi console support without reimplementing REPL plumbing.

### 3.6 HTTP pattern findings (`0065` and `0066`)

`0065` gives minimal clean API patterns:

- JSON body read helper, see `0065.../http_server.c:42-56`.
- cJSON parse/validate/respond flow, see `0065.../http_server.c:108-157`.
- Simple `on_wifi_got_ip -> http_server_start`, see `0065.../app_main.c:16-21`.

`0066` adds scale-up lessons:

- `cfg.max_uri_handlers = 24` to avoid hidden route registration failures, see `0066.../http_server.cpp:560-563`.
- startup with `wifi_console.register_extra`, see `0066.../app_main.cpp:27-31` and `:61-65`.

Recommendation:

- Base 0067 HTTP implementation on 0065 simplicity.
- Apply 0066 hardening where route count grows beyond defaults.

## 4) Hardware fundamentals an intern must know

## 4.1 M5Stack STAMP C3 baseline

From the vendor page:

- MCU is ESP32-C3 (single-core RISC-V, Wi-Fi, BLE), with flash and GPIO breakout.
- Board includes USB serial converter and 5V->3.3V regulation on the module ecosystem.
- Exposes GPIO0..GPIO10 on castellated/module pads.

Source:

- `https://docs.m5stack.com/en/core/stamp_c3`

Practical consequence:

- GPIO4, GPIO5, GPIO6 are valid for SPI output signals in this project.
- Always check for boot or board wiring side effects when repurposing pins.

## 4.2 MAX7219 chain fundamentals

Each MAX7219 module is an 8x8 matrix driver. In a daisy chain:

- You clock register/data pairs for all modules in one transaction.
- First shifted pair lands at the farthest module.
- Last shifted pair lands at the closest module.

That is why `max7219_set_row_chain` reverses array order before transmit.

Registers used by this project:

- `DECODE_MODE (0x09)` -> `0x00` (matrix mode)
- `SCAN_LIMIT (0x0B)` -> `0x07` (8 rows)
- `INTENSITY (0x0A)` -> `0..15`
- `SHUTDOWN (0x0C)` -> `0x01` (normal operation)
- `DISPLAY_TEST (0x0F)` -> full-on hardware test

See initialization sequence in `max7219.c:85-104`.

## 4.3 USB Serial/JTAG console fundamentals for ESP32-C3

Relevant ESP-IDF doc:

- `https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html`

Key points:

- USB Serial/JTAG is a native peripheral path for console/JTAG on supported chips.
- If eFuse `DIS_USB_JTAG` is burnt, USB Serial/JTAG cannot be used.
- External JTAG usage can involve GPIO4..GPIO7; do not mix that with matrix SPI on those pins in the same hardware profile.

Project policy for this repo (AGENTS guidance):

- Prefer USB Serial/JTAG console to avoid colliding with application UART usage.

For 0067 we keep that as default in `sdkconfig.defaults`.

## 5) Proposed firmware layout: `0067-esp-c3-led-matrix-http`

## 5.1 Directory structure

```text
0067-esp-c3-led-matrix-http/
  CMakeLists.txt
  sdkconfig.defaults
  partitions.csv
  main/
    CMakeLists.txt
    Kconfig.projbuild
    app_main.c
    matrix_cfg.h
    matrix_hal_max7219.c
    matrix_hal_max7219.h
    matrix_engine.c
    matrix_engine.h
    matrix_console_cmds.c
    matrix_console_cmds.h
    matrix_http.c
    matrix_http.h
    assets/
      index.html         # optional tiny control page
```

Reuse components via `EXTRA_COMPONENT_DIRS`:

- `../components/wifi_mgr`
- `../components/wifi_console`
- `../components/httpd_assets_embed`

## 5.2 Module responsibilities

### `matrix_hal_max7219.*`

- Open/init/config SPI device.
- Push row-chain bytes.
- Own nothing about text/animation.

### `matrix_engine.*`

- Own framebuffer and animation state.
- Provide thread-safe API for:
  - set text static
  - start/stop scroll
  - start/stop drop bounce
  - start/stop wave/spin/flip
  - set intensity
  - set chain length
  - set reverse/flip orientation
  - status snapshot
- Single internal worker task for animation stepping.

### `matrix_console_cmds.*`

- Register `matrix` command family into `esp_console`.
- Parse argv and call engine API.
- Keep command names largely compatible with 0036 for muscle memory.

### `matrix_http.*`

- Register `/api/matrix/*` endpoints.
- Parse JSON -> call engine API -> return normalized status JSON.

### `app_main.c`

- Initialize engine and hardware.
- Wire Wi-Fi got-IP callback to HTTP server start.
- Start Wi-Fi manager.
- Start shared wifi console with `register_extra = matrix_console_register_commands`.

## 5.3 Startup lifecycle

```text
boot
  -> matrix_engine_init(cfg)
  -> wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip)
  -> wifi_mgr_start()
  -> wifi_console_start(prompt="c3m>", register_extra=matrix_console_register_commands)

on_wifi_got_ip
  -> matrix_http_start()
```

This mirrors proven flow in `0065` and `0066`.

## 6) API design for dual control (console + REST)

## 6.1 Console command compatibility plan

Keep these from `0036` because user already knows them:

- `matrix status`
- `matrix init`
- `matrix clear`
- `matrix text <TEXT>`
- `matrix scroll on <TEXT> [fps] [pause_ms]`
- `matrix scroll wave <TEXT> [fps] [pause_ms]`
- `matrix scroll off`
- `matrix anim drop <TEXT> [fps] [pause_ms]`
- `matrix anim dropcfg [gravity_px_s2] [bounce]`
- `matrix anim wave|spin|flip ...`
- `matrix anim off`
- `matrix intensity <0..15>`
- `matrix chain [n]`
- `matrix reverse on|off`
- `matrix flipv on|off`
- `matrix spi [hz]`

Drop for 0067 initial release:

- `kbd ...` command group (Cardputer-only).

## 6.2 REST endpoint proposal

All under `/api/matrix`.

### Read APIs

- `GET /api/matrix/status`
- `GET /api/matrix/frame` (optional; raw bytes for future visualizer)

### Write APIs

- `POST /api/matrix/text`
- `POST /api/matrix/scroll`
- `POST /api/matrix/anim`
- `POST /api/matrix/stop`
- `POST /api/matrix/config`

Example `POST /api/matrix/anim` body:

```json
{
  "mode": "drop",
  "text": "HELLO INTERN",
  "fps": 20,
  "pause_ms": 200,
  "drop": {
    "gravity_px_s2": 220,
    "bounce": 0.82
  }
}
```

Normalized status response (all write APIs return this):

```json
{
  "ok": true,
  "ready": true,
  "mode": "drop",
  "text": "HELLO INTERN",
  "chain_len": 12,
  "width": 96,
  "fps": 20,
  "pause_ms": 200,
  "intensity": 4,
  "spi_hz": 100000,
  "reverse_modules": false,
  "flip_vertical": true
}
```

## 6.3 Concurrency rule

Single writer invariant:

- Any API path that mutates animation or framebuffer takes `engine->mu`.
- HTTP and console command handlers never write global state directly.
- Worker task reads immutable snapshots or protected state.

This avoids race patterns that would otherwise happen when REST and REPL send overlapping commands.

## 7) Kconfig and sdkconfig plan

## 7.1 New Kconfig namespace

Use `menu "0067: ESP32-C3 LED matrix HTTP"` with at least:

- matrix SPI pins:
  - `CONFIG_TUTORIAL_0067_MAX7219_PIN_MOSI` default `4`
  - `CONFIG_TUTORIAL_0067_MAX7219_PIN_CS` default `5`
  - `CONFIG_TUTORIAL_0067_MAX7219_PIN_SCK` default `6`
- chain and timing:
  - `CONFIG_TUTORIAL_0067_CHAIN_LEN` default `12`
  - `CONFIG_TUTORIAL_0067_SPI_HZ` default `100000`
  - `CONFIG_TUTORIAL_0067_DEFAULT_FPS` default `15`
- HTTP limits:
  - `CONFIG_TUTORIAL_0067_HTTP_MAX_BODY` default `512`
- behavior toggles:
  - `CONFIG_TUTORIAL_0067_AUTOINIT_ON_BOOT` default `y`

## 7.2 `sdkconfig.defaults` baseline

```ini
# Console backend preference
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED=y
CONFIG_ESP_CONSOLE_UART_NUM=-1

# Logging
CONFIG_LOG_DEFAULT_LEVEL_INFO=y

# Wi-Fi manager defaults
CONFIG_WIFI_MGR_AUTOCONNECT_ON_BOOT=y
CONFIG_WIFI_MGR_MAX_RETRY=10
```

Notes:

- This aligns with shared `wifi_console` backend selection logic.
- If your physical STAMP carrier does not expose native USB Serial/JTAG reliably, switch to UART in menuconfig and document reserved pins.

## 8) Exact extraction strategy from existing code

## 8.1 What to copy first

1. Copy `0036/main/max7219.[ch]` -> `0067/main/matrix_hal_max7219.[ch]`.
2. Copy selected rendering and animation functions from `0036/main/matrix_console.c` into `matrix_engine.c`:
   - framebuffer helpers
   - font table and text-to-column helpers
   - drop/wave/spin/flip runtime loops
3. Copy `0065/main/http_server.c` structure to seed `matrix_http.c`.
4. Reuse shared components from `components/` instead of copying.

## 8.2 What not to copy

- `0036/main/tca8418.*`
- all `kbd_*` paths and key mapping arrays
- Cardputer-specific pin macros and board assumptions

## 8.3 Refactor target API (pseudocode)

```c
// matrix_engine.h
typedef enum {
  MATRIX_MODE_IDLE,
  MATRIX_MODE_TEXT,
  MATRIX_MODE_SCROLL,
  MATRIX_MODE_WAVE,
  MATRIX_MODE_DROP,
  MATRIX_MODE_SPIN,
  MATRIX_MODE_FLIP
} matrix_mode_t;

typedef struct {
  bool ready;
  matrix_mode_t mode;
  char text[65];
  uint8_t intensity;
  uint8_t chain_len;
  uint32_t spi_hz;
  uint32_t fps;
  uint32_t pause_ms;
  bool reverse_modules;
  bool flip_vertical;
} matrix_status_t;

esp_err_t matrix_engine_init(const matrix_cfg_t *cfg);
esp_err_t matrix_engine_start_text(const char *text);
esp_err_t matrix_engine_start_scroll(const char *text, uint32_t fps, uint32_t pause_ms, bool wave);
esp_err_t matrix_engine_start_anim_drop(const char *text, uint32_t fps, uint32_t pause_ms);
esp_err_t matrix_engine_start_anim_spin(const char *text, uint32_t fps, uint32_t pause_ms);
esp_err_t matrix_engine_start_anim_flip(const char *spec, uint32_t fps, uint32_t hold_ms);
esp_err_t matrix_engine_stop(void);
esp_err_t matrix_engine_get_status(matrix_status_t *out);
```

This is intentionally command-agnostic so both REST and REPL are just adapters.

## 9) Intern fundamentals: how animations actually work

## 9.1 Frame model

The engine represents the full 12-module strip as an 8-row matrix with module-major bytes:

- `fb[y][module]` is one row-byte for one module.
- bit position in byte is x-position inside module.
- overall width is `8 * chain_len`.

Mapping helpers in existing code:

- `x_to_module(x)` and `x_to_bit(x)`.

## 9.2 Scroll model

Scroll text is converted once into a linear column buffer:

- each character is 5 glyph columns + 1 spacer
- text buffer width = `len * 6`

Each frame:

- compute `t = x - pos`
- sample `text_cols[t]` if in range
- optional wave y-shift per character phase
- write framebuffer
- flush rows to MAX7219

## 9.3 Drop-bounce model

The "bouncing text" is a discrete physics sequence in pixel space:

- acceleration `g` converted from px/s^2 to px/frame^2
- vertical offset integrated in fixed-point Q8
- collision at baseline inverts velocity with restitution coefficient
- sequence trimmed and tailed for smooth settle

This is implemented in `drop_rebuild_seq` and sampled with stagger per character.

Formula (conceptual):

- `v[t+1] = v[t] + g`
- `y[t+1] = y[t] + v[t+1]`
- if `y >= 0`: `y=0`, `v = -v * restitution`

This is why the effect feels physical rather than scripted.

## 10) Setup guide: building 0067 from scratch

## 10.1 Create project folder

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
mkdir -p 0067-esp-c3-led-matrix-http/main/assets
```

## 10.2 Start from known CMake pattern

Use the same top-level style as 0065/0066 and include shared components:

- `wifi_mgr`
- `wifi_console`
- `httpd_assets_embed`

## 10.3 Set target and baseline config

```bash
cd 0067-esp-c3-led-matrix-http
idf.py set-target esp32c3
idf.py reconfigure
```

Then verify console backend in config:

- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
- UART console disabled unless intentionally needed.

## 10.4 Implement in this order

1. HAL + basic init + `matrix status`
2. static text
3. scroll and wave
4. drop bounce
5. REST status + text + stop
6. remaining endpoints and config
7. polish and validation

Reason:

- keeps a working visible milestone at each stage
- minimizes simultaneous unknowns (hardware + Wi-Fi + HTTP + animation)

## 11) Validation and test plan

## 11.1 Manual bring-up sequence

1. Flash and open monitor.
2. Confirm prompt appears (`c3m>` expected).
3. Run `wifi scan` and `wifi join <idx> <pass> --save`.
4. Verify IP log appears.
5. Run `matrix status`.
6. Run `matrix text HELLO`.
7. Run `matrix anim drop HELLO 20 250`.
8. Use REST `curl` to change text and mode while animation is active.

## 11.2 REST smoke commands

```bash
curl -s http://<ip>/api/matrix/status

curl -s -X POST http://<ip>/api/matrix/text \
  -H 'content-type: application/json' \
  -d '{"text":"HELLO C3"}'

curl -s -X POST http://<ip>/api/matrix/anim \
  -H 'content-type: application/json' \
  -d '{"mode":"drop","text":"BOUNCE","fps":20,"pause_ms":200}'

curl -s -X POST http://<ip>/api/matrix/stop
```

## 11.3 Regression checks

- Console and REST both must be able to start and stop the same animation modes.
- Chain length changes must not crash active tasks.
- Switching Wi-Fi networks at runtime must keep matrix service alive.
- Invalid JSON and invalid parameter ranges return clear 400 errors.

## 12) Risks and mitigations

## 12.1 Monolithic legacy logic risk

Risk:

- Directly pasting all of `matrix_console.c` keeps technical debt and tight coupling.

Mitigation:

- Extract engine first, then thin adapters.

## 12.2 Pin conflict / signal integrity risk

Risk:

- Wrong pin assumptions or poor wiring causes random display behavior.

Mitigation:

- Keep conservative SPI default (`100 kHz`) during bring-up.
- Expose runtime `matrix spi` command for tuning.
- Keep short ground returns and shared reference between STAMP and matrix chain.

## 12.3 HTTP + console race risk

Risk:

- Two controllers writing engine state simultaneously.

Mitigation:

- One API boundary with mutex inside engine.
- No direct global writes from handlers.

## 12.4 Route registration limit risk

Risk:

- More endpoints than default `esp_http_server` handler limit.

Mitigation:

- Increase `cfg.max_uri_handlers` when needed (copy 0066 approach).

## 13) Suggested first implementation backlog

## Milestone A: skeleton and visibility

- [ ] project scaffold with target esp32c3
- [ ] HAL init and ids pattern
- [ ] console prompt and `matrix status`
- [ ] wifi console available

## Milestone B: user-visible behaviors

- [ ] `matrix text`
- [ ] `matrix scroll on|off`
- [ ] `matrix anim drop`
- [ ] runtime intensity and spi commands

## Milestone C: REST control

- [ ] `GET /api/matrix/status`
- [ ] `POST /api/matrix/text`
- [ ] `POST /api/matrix/anim`
- [ ] `POST /api/matrix/stop`

## Milestone D: hardening

- [ ] input validation and bounds checks
- [ ] route count and body size guardrails
- [ ] basic test script for curl + console flow

## 14) File-by-file reading guide for interns

Read in this order:

1. `components/wifi_mgr/wifi_mgr.c`
2. `components/wifi_console/wifi_console.c`
3. `0036-cardputer-adv-led-matrix-console/main/max7219.c`
4. `0036-cardputer-adv-led-matrix-console/main/matrix_console.c` (animation sections first)
5. `0065-xiao-esp32c6-gpio-web-server/main/http_server.c`
6. `0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp`
7. `0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp` (startup and registration parts)

Why this order:

- It teaches platform plumbing first (Wi-Fi + console), then device control, then HTTP adapter patterns.

## 15) Bottom line

The codebase already contains almost everything needed for `0067-esp-c3-led-matrix-http`:

- proven matrix animation engine (`0036`)
- reusable Wi-Fi manager and REPL (`components/wifi_mgr`, `components/wifi_console`)
- minimal and scalable HTTP patterns (`0065`, `0066`)

The core engineering task is not invention, it is clean extraction and integration:

- remove Cardputer keyboard coupling
- make pins/config board-specific (STAMP C3)
- expose one unified engine to both console and REST

If done this way, intern onboarding is straightforward, the firmware remains debuggable, and future modes/endpoints can be added without rewriting the core.

## 16) Implementation Addendum (2026-02-21)

This section records what was actually implemented and verified in firmware `0067-esp-c3-led-matrix-http`.

### 16.1 Implemented files

- `0067-esp-c3-led-matrix-http/CMakeLists.txt`
- `0067-esp-c3-led-matrix-http/sdkconfig.defaults`
- `0067-esp-c3-led-matrix-http/partitions.csv`
- `0067-esp-c3-led-matrix-http/main/Kconfig.projbuild`
- `0067-esp-c3-led-matrix-http/main/app_main.c`
- `0067-esp-c3-led-matrix-http/main/max7219.[ch]`
- `0067-esp-c3-led-matrix-http/main/matrix_engine.[ch]`
- `0067-esp-c3-led-matrix-http/main/matrix_console.[ch]`
- `0067-esp-c3-led-matrix-http/main/http_server.[ch]`
- `0067-esp-c3-led-matrix-http/main/assets/index.html`

### 16.2 Runtime integration outcome

- Matrix bus pins are configured per request:
  - `DIN/MOSI = GPIO4`
  - `CS = GPIO5`
  - `CLK/SCK = GPIO6`
- Matrix chain length default: `12`.
- HTTP service starts on Wi-Fi got-IP callback.
- Console command parser is registered through `wifi_console` `register_extra` callback.

### 16.3 Console backend correction for STAMP C3 bring-up

Initial boot showed ROM output but no usable REPL when configured for USB Serial/JTAG on this hardware path. The connected host port was `/dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00` -> `/dev/ttyACM0`, which is a USB-UART bridge path in this setup.

Final working configuration uses UART console defaults in `sdkconfig.defaults`:

```ini
CONFIG_ESP_CONSOLE_UART_DEFAULT=y
CONFIG_ESP_CONSOLE_UART=y
CONFIG_ESP_CONSOLE_UART_NUM=0
CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=n
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED=n
```

### 16.4 Verified live workflow

Using `idf.py` in `tmux` with `/dev/serial/by-id`:

```bash
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00 flash monitor
```

Console sequence verified:

```text
wifi set --ssid "CLUB:LINK" --pass "AllTogether0" --save
wifi connect
wifi status
matrix text HELLO
matrix anim drop HELLO 18 400
matrix scroll wave HI 12 200
matrix status
help matrix
```

Observed result highlights:

- Wi-Fi connected to `CLUB:LINK`.
- DHCP IP acquired: `192.168.3.119`.
- HTTP server started on port `80`.
- Matrix status reflected mode transitions (`text`, `drop`, `scroll`, `idle`).

REST verification from host:

```bash
curl -sS http://192.168.3.119/api/matrix/status
curl -sS -X POST http://192.168.3.119/api/matrix/text -H 'content-type: application/json' -d '{"text":"RESTOK"}'
curl -sS -X POST http://192.168.3.119/api/matrix/anim -H 'content-type: application/json' -d '{"mode":"wave","text":"WIFI","fps":20,"pause_ms":250}'
curl -sS -X POST http://192.168.3.119/api/matrix/stop -H 'content-type: application/json' -d '{}'
```

All endpoints returned `{"ok":true,...}` responses with updated matrix state.
