# `cardputer_kb` — Tutorial (ESP-IDF)

## Goal

Integrate `cardputer_kb` into an ESP-IDF firmware to:

- read physical keys as `keyNum` values (1..56) and `KeyPos{x,y}`
- optionally decode “navigation actions” from chords (Fn+key)

## 1) Add the component to your project

### Option A: Project is inside this repo

Most projects here use `EXTRA_COMPONENT_DIRS` in the top-level `CMakeLists.txt`.

Example:

```cmake
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/cardputer_kb"
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo/components/M5GFX"
)
```

### Option B: Copy component into your project

Copy `components/cardputer_kb/` into your project’s `components/` folder.

## 2) Require the component from your `main` component

In `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES cardputer_kb
)
```

If you also use M5GFX:

```cmake
idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES cardputer_kb M5GFX
)
```

## 3) Scan the matrix

Minimal scan loop:

```cpp
#include "cardputer_kb/scanner.h"

cardputer_kb::UnifiedScanner kb;
ESP_ERROR_CHECK(kb.init());

while (true) {
  cardputer_kb::ScanSnapshot s = kb.scan();
  // s.pressed_keynums is a stable physical key identity set.
  vTaskDelay(1);
}
```

Common debugging output:

```cpp
if (!s.pressed_keynums.empty()) {
  // print keyNum list; translate to labels with layout helpers if desired
}
```

## 4) Translate keyNums to labels (optional)

```cpp
#include "cardputer_kb/layout.h"

for (auto kn : s.pressed_keynums) {
  int x=-1, y=-1;
  cardputer_kb::xy_from_keynum(kn, &x, &y);
  const char* label = cardputer_kb::legend_for_xy(x, y);
}
```

## 5) Decode semantic actions (optional)

If your firmware uses “navigation actions”, include:

```cpp
#include "cardputer_kb/bindings.h"
```

Define a binding table (or include a captured one):

```cpp
#include "cardputer_kb/bindings_m5cardputer_captured.h"

auto* b = cardputer_kb::decode_best(
  s.pressed_keynums,
  cardputer_kb::kCapturedBindingsM5Cardputer,
  sizeof(cardputer_kb::kCapturedBindingsM5Cardputer) / sizeof(cardputer_kb::kCapturedBindingsM5Cardputer[0])
);
if (b) {
  // b->action is the semantic action
}
```

Notes:

- A binding is a set of required `keyNum`s; chords are naturally supported.
- “Most specific wins” means `[fn, x]` can override `[x]` if both match.
- The included Cardputer bindings intentionally use **Fn-chords** for navigation (and for Esc/back, Del) so these actions don't collide with printable punctuation keys.

## 6) Regenerate bindings (recommended)

Use the 0023 calibrator firmware to capture semantic bindings on real hardware:

- Firmware: `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator`
- Host helper: `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/tools/cfg_to_header.py`

Workflow:

1. Run wizard mode on device, press required keys/chords, and print `CFG_BEGIN...CFG_END`.
2. Save the JSON blob to `cfg.json`.
3. Convert:
   - `python3 tools/cfg_to_header.py cfg.json > bindings.h`

## Troubleshooting

- If flashing fails with “port busy”: stop `idf.py monitor` (it locks the device node).
- If keys never register:
  - confirm the scanner autodetect switched to alt IN0/IN1 if needed
  - verify the Cardputer is on GPIO matrix pins (not a different keyboard accessory)
