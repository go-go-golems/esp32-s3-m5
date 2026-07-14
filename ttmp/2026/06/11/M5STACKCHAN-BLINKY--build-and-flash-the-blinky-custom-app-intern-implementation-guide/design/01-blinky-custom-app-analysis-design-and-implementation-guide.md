---
title: Blinky Custom App Analysis Design and Implementation Guide
doc_type: design
ticket: M5STACKCHAN-BLINKY
topics:
  - m5stackchan
  - firmware
  - custom-app
  - esp32-s3
  - blinky
  - intern-guide
  - mooncake
status: active
---

# Blinky Custom App: Analysis, Design, and Implementation Guide

**Ticket:** M5STACKCHAN-BLINKY
**Audience:** New intern — this document assumes you can read C++ and use a terminal, but you've never touched this codebase before.
**Goal:** Build a custom "Blinky" app that toggles the StackChan's LED ring and displays "LED: ON"/"LED: OFF" on screen, then flash it to a physical StackChan device.

---

## 1. Executive Summary

This guide takes you from zero to a working custom app running on real hardware. You'll learn how the M5StackChan firmware is structured, how the Mooncake app framework works, how LVGL rendering and hardware peripherals are accessed through the HAL, and how to create, register, build, and flash a new app. The deliverable is a "Blinky" app: press the home indicator to toggle the RGB LED ring on and off, with a label showing the current state.

The guide is organized in four layers:

1. **System architecture** — the big picture of what runs where and why
2. **Framework internals** — the Mooncake app lifecycle, the HAL, and the LVGL threading model
3. **Design and implementation** — the Blinky app design with pseudocode and API references
4. **Build, flash, and verify** — exact commands and expected output

---

## 2. System Architecture

### 2.1 Hardware overview

The M5StackChan (SKU K151) is a desktop robot built around the M5Stack CoreS3, which uses an ESP32-S3 dual-core Xtensa processor running at 240 MHz with 16 MB SPI flash and 8 MB PSRAM. The CoreS3 module provides the display (2" ILI9342C IPS LCD, 320×240, with FT6336U capacitive touch), camera (GC0308), audio (ES7210 codec + AW88298 amp + dual microphones), IMU (BMI270 + BMM150), and a USB-C port that doubles as both power and a built-in JTAG debug interface.

The robot body module connects to the CoreS3 via an I2C bus (GPIO 11/12) and a UART bus (GPIO 6/7 for servos). The body contains:

- Two SCS0009 feedback servos (yaw = continuous rotation, pitch = 90° range) on UART at 1 Mbps
- 12× WS2812C RGB LEDs arranged in a ring (the "neon light")
- ST25R3916 NFC controller
- Si12T capacitive touch sensor (head pet detection)
- INA226 battery monitor
- PY32L020 I/O expander (controls servo power and RGB enable)
- IRM56384 infrared receiver and IR transmitter

```
┌────────────────────────────────────────────────────────┐
│                    CoreS3 (Host)                       │
│  ESP32-S3 @ 240 MHz  │  16 MB Flash  │  8 MB PSRAM   │
│  Wi-Fi + BLE 5        │  USB-C CDC/JTAG               │
│                                                        │
│  Display: ILI9342C 320×240 IPS (SPI)                  │
│  Touch: FT6336U (I2C at 0x48)                         │
│  Camera: GC0308 (8-bit parallel)                      │
│  Audio: ES7210 codec + AW88298 amp (I2S)              │
│  IMU: BMI270+BMM150 (I2C)                             │
│  PMIC: AXP2101 (I2C at 0x34)                          │
│  RTC: BM8563/PCF8563 (I2C at 0x51)                    │
│  IO Expander: AW9523B (I2C at 0x2E)                   │
└──────────────┬─────────────────────────────────────────┘
               │ I2C (G11/G12) + UART (G6/G7) + GPIO
┌──────────────▼─────────────────────────────────────────┐
│                   Robot Body                           │
│  Servos: 2× SCS0009 on UART                           │
│  LEDs: 12× WS2812C ring (left + right neon)           │
│  NFC: ST25R3916 (I2C at 0x50)                        │
│  Touch: Si12T (I2C at 0x68)                          │
│  Battery: INA226 (I2C at 0x41) + 550mAh cell         │
│  IO Expander: PY32L020 (I2C at 0x6F/0x71)            │
│  IR: IRM56384 receiver (G10) + transmitter (G5)       │
└────────────────────────────────────────────────────────┘
```

**Key reference for this project:** The WS2812C LED ring is what Blinky controls. It is driven by the `espressif/led_strip` component and exposed through the StackChan subsystem's `leftNeonLight()` and `rightNeonLight()` methods.

### 2.2 Software stack

The firmware is built on ESP-IDF v5.5.4 and uses three major frameworks layered on top of each other:

```
┌───────────────────────────────────────────────────┐
│  Your Custom App (AppBlinky)                      │  ← You write this
├───────────────────────────────────────────────────┤
│  Mooncake App Framework (v2.3.3)                  │  ← App lifecycle, launcher
├──────────────────┬────────────────────────────────┤
│  StackChan Subsys│  XiaoZhi AI Framework (v2.2.4) │  ← Robot personality + AI
├──────────────────┴────────────────────────────────┤
│  HAL (Hardware Abstraction Layer)                 │  ← Bridges hardware to apps
├───────────────────────────────────────────────────┤
│  LVGL 9.4 + smooth_ui_toolkit (v2.12.0)           │  ← UI rendering
├───────────────────────────────────────────────────┤
│  ESP-IDF v5.5.4 + FreeRTOS                       │  ← RTOS, drivers, WiFi
├───────────────────────────────────────────────────┤
│  ESP32-S3 Hardware                                │
└───────────────────────────────────────────────────┘
```

The boot sequence in `main/main.cpp` is:

1. `app_main()` initializes the HAL (`GetHAL().init()`)
2. Installs all Mooncake apps into the framework
3. Runs the Mooncake main loop (`GetMooncake().update()`) which calls each app's `onRunning()`
4. If the AI Agent is requested, uninstalls Mooncake apps and starts XiaoZhi (which never returns)

**Important:** XiaoZhi and Mooncake are mutually exclusive at runtime. When XiaoZhi is running, Mooncake apps are destroyed. This is a design choice — XiaoZhi is a separate application mode, not just another app.

### 2.3 Threading model

The ESP32-S3 has two cores. The firmware uses FreeRTOS tasks:

- **Main task** (Core 1): Runs `app_main()`, the Mooncake loop, and all app logic
- **LVGL task** (Core 0): Dedicated task for display rendering at ~30 FPS
- **Audio pipeline task**: XiaoZhi's audio codec and wake word processing
- **WiFi/BLE tasks**: Network stack (ESP-IDF managed)
- **Servo task**: Periodic servo feedback polling (when using feedback servos)

Because LVGL runs on a separate task, **you must lock before any LVGL operation** and unlock after. The `LvglLockGuard` RAII class does this automatically:

```cpp
{
    LvglLockGuard lock;  // Acquires mutex
    lv_label_set_text(label, "Hello");  // Safe
}  // Mutex released when lock goes out of scope
```

If you skip the lock, you'll get corrupted display output or hard crashes.

---

## 3. Mooncake App Framework Internals

### 3.1 Ability hierarchy

Mooncake provides a class hierarchy for different use cases:

```
AbilityBase            (base: baseCreate/baseUpdate/baseDestroy)
  └── BasicAbility     (onCreate/onRunning/onDestroy — like Arduino setup/loop)
        └── UIAbility    (onCreate/onShow/onForeground/onBackground/onHide/onDestroy)
        └── WorkerAbility(onCreate/onResume/onRunning/onPause/onDestroy)
        └── AppAbility   (onCreate/onOpen/onRunning/onSleeping/onClose/onDestroy) ← Your app
```

For custom apps, you always inherit from `AppAbility`. It provides:

- **Open/close lifecycle** — the launcher opens and closes your app
- **App metadata** — name, icon, and a `userData` void pointer
- **State machine** — `StateGoOpen → StateRunning → StateGoClose → StateSleeping`

### 3.2 AppAbility API reference

Source: `components/mooncake/src/ability/ability.h`

```cpp
class AppAbility : public AbilityBase {
public:
    // --- App metadata ---
    struct AppInfo_t {
        std::string name;       // Displayed in the launcher's icon label
        void* icon = nullptr;   // Pointer to lv_image_dsc_t from assets
        void* userData = nullptr; // Free-form: StackChan uses this for theme color
    };

    // Get/set app info
    const AppInfo_t& getAppInfo();
    AppInfo_t& setAppInfo();     // Returns mutable reference — set fields in constructor

    // --- Lifecycle control ---
    void open();                 // Triggers StateGoOpen → calls onOpen()
    void close();                // Triggers StateGoClose → calls onClose()

    // --- State query ---
    enum State_t { StateNull, StateGoOpen, StateRunning, StateGoClose, StateSleeping };
    State_t currentState();

    // --- Lifecycle callbacks (override these) ---
    virtual void onCreate();     // App is installed into Mooncake (one-time setup)
    virtual void onOpen();       // App is opened — create UI, start tasks
    virtual void onRunning();    // Called every frame while app is active
    virtual void onSleeping();   // App is in background (unused in StackChan)
    virtual void onClose();      // App is closed — destroy UI, free resources
    virtual void onDestroy();    // App is uninstalled from Mooncake
};
```

**Key pattern:** You set `setAppInfo().name` and `setAppInfo().icon` in the constructor. The launcher reads these to build the home screen.

### 3.3 State machine

```
                 open()
  StateSleeping ──────▶ StateGoOpen
                           │
                     onOpen() called
                           │
                           ▼
                      StateRunning
                           │
                    onRunning() every frame
                           │
                 close()   │
                      │    │
                      ▼    │
                   StateGoClose
                      │
                onClose() called
                      │
                      ▼
                   StateSleeping
```

The framework calls `onRunning()` on every iteration of the Mooncake main loop. This runs at the same rate as `GetMooncake().update()`, which is called in a tight `while(1)` loop in `app_main()`. There is no fixed frame rate — `onRunning()` executes as fast as the main loop can iterate, which is typically hundreds of times per second. For timed behavior (like toggling every 500ms), you must check `GetHAL().millis()` yourself.

### 3.4 How apps are registered

In `main/main.cpp`:

```cpp
GetMooncake().installApp(std::make_unique<AppLauncher>());
GetMooncake().installApp(std::make_unique<AppAiAgent>());
GetMooncake().installApp(std::make_unique<AppEspnowControl>());
// ... etc
```

`installApp()` calls `onCreate()` immediately. The launcher then calls `open()` on whichever app the user selects.

In `main/apps/apps.h`, all app headers are included:

```cpp
#include "app_launcher/app_launcher.h"
#include "app_ai_agent/app_ai_agent.h"
#include "app_espnow_ctrl/app_espnow_ctrl.h"
// ... etc
```

**You must add your app's header here** and add an `installApp()` line in `main.cpp`.

### 3.5 Build system — why you don't need to edit CMakeLists.txt

The `main/CMakeLists.txt` uses a glob to collect all source files:

```cmake
file(GLOB_RECURSE STACK_CHAN_SOURCES
    "apps/*.c"
    "apps/*.cc"
    "apps/*.cpp"
    ...
)
```

This means any `.cpp` file you place under `main/apps/` is automatically compiled. Just create the directory and files, and the build system picks them up. No CMake changes needed.

**Caveat:** If you add a new file to an already-configured build, CMake won't notice until you re-run `idf.py reconfigure` or touch `CMakeLists.txt`. In practice, `idf.py build` handles this automatically on the first build after adding files.

---

## 4. The HAL and StackChan Subsystem

### 4.1 HAL singleton

The HAL is accessed through a global singleton:

```cpp
GetHAL()  // Returns Hal& — the one and only instance
```

Key HAL methods you'll use:

| Method | Signature | What it does |
|--------|-----------|--------------|
| `init()` | `void init()` | Initializes all hardware subsystems |
| `millis()` | `uint32_t millis()` | Returns milliseconds since boot (wraps at ~49 days) |
| `delay()` | `void delay(uint32_t ms)` | Blocking delay (feeds watchdog) |
| `lvglLock()` | `void lvglLock()` | Acquires the LVGL mutex |
| `lvglUnlock()` | `void lvglUnlock()` | Releases the LVGL mutex |
| `feedTheDog()` | `void feedTheDog()` | Feeds the task watchdog |
| `getStackChan()` | `StackChan& getStackChan()` | Returns the robot subsystem |

Source: `main/hal/hal.h`

### 4.2 StackChan subsystem

`GetHAL().getStackChan()` (conveniently available as `GetStackChan()`) provides access to the robot's personality:

```cpp
// Motion (servo control)
auto& motion = GetStackChan().motion();
motion.moveWithSpeed(yaw, pitch, speed);   // yaw: -1280..1280, pitch: 0..900, speed: 0..1000
int16_t yaw = motion.getCurrentYawAngle();

// LED ring (neon lights)
GetStackChan().leftNeonLight().setColor(r, g, b);   // r,g,b: 0-255 (safe: 0-168)
GetStackChan().rightNeonLight().setColor(r, g, b);

// Avatar (face rendering)
auto& avatar = GetStackChan().avatar();
avatar.setExpression(Expression::Happy);
```

**For Blinky, we only need the neon light API.** The `setColor(r, g, b)` method takes three 8-bit values (0–255). The safe range recommended by the MCP tool definition is 0–168 to avoid overdriving the LEDs, but 0–255 works fine for short durations.

Source: `main/hal/hal_mcp.cpp` lines 72–88 (the MCP `set_led_color` tool shows the exact API usage)

### 4.3 LVGL threading contract

This is the most important rule in the entire firmware:

> **Every LVGL call must be protected by the LVGL lock.**

The LVGL task runs on Core 0 and continuously renders the display. If you modify LVGL objects from the main task (Core 1) without locking, you'll get data races that manifest as:
- Garbled display output
- Random hard faults
- Objects appearing at wrong positions or with wrong content

The `LvglLockGuard` class (from `smooth_ui_toolkit`) provides RAII-based locking:

```cpp
void MyApp::onOpen() {
    LvglLockGuard lock;  // Locks automatically
    _label = lv_label_create(lv_screen_active());
    lv_label_set_text(_label, "Hello");
    // Lock released when function returns
}
```

You must also lock in `onRunning()` if you modify LVGL objects:

```cpp
void MyApp::onRunning() {
    LvglLockGuard lock;
    lv_label_set_text(_label, "Updated");
}
```

**Performance tip:** Keep the lock held for the shortest possible time. Don't do I/O, network, or heavy computation while holding the lock — it blocks the LVGL render task and causes visible stuttering.

---

## 5. Common UI Components

The firmware provides reusable UI components in `main/apps/common/`. For Blinky, we only need the home indicator.

### 5.1 Home indicator

Source: `main/apps/common/home_indicator/home_indicator.h`

The home indicator is a swipe-up-from-bottom gesture that shows a circular button. When pressed, it calls your callback (typically `close()`).

```cpp
// Create — call once in onOpen()
view::create_home_indicator(
    [this]() { close(); },    // Callback when home is pressed
    0xFFAA00,                 // Button color (hex RGB)
    0x664400                  // Border color (hex RGB)
);

// Update — call every frame in onRunning()
view::update_home_indicator();

// Destroy — call once in onClose()
view::destroy_home_indicator();
```

**Why must you call `update_home_indicator()` every frame?** The home indicator uses a spring physics animation (`AnimateValue` from `smooth_ui_toolkit`) to smoothly move the button into view when the swipe gesture is detected. The spring needs to be ticked every frame to update its position.

### 5.2 Toast notifications

Source: `main/apps/common/toast/toast.h`

```cpp
view::create_toast("Saved!", 2000);  // message, duration_ms
```

A toast is a temporary overlay that appears on screen and fades out after the specified duration. It's fire-and-forget — no cleanup needed.

### 5.3 Other components

| Component | Header | Purpose |
|-----------|--------|---------|
| Status bar | `common/status_bar/status_bar.h` | Battery, WiFi, time display |
| Loading page | `common/loading_page/loading_page.h` | Full-screen spinner |
| Reminder | `common/reminder/reminder.h` | Scheduled reminder with view |

---

## 6. App Icon and Theme Color

### 6.1 How the launcher reads app metadata

The launcher (`app_launcher/`) creates a scrollable panel of app icons. For each installed app, it reads:

1. **`getAppInfo().name`** — displayed as a text label below the icon
2. **`getAppInfo().icon`** — cast to `lv_image_dsc_t*` and used as the icon image
3. **`getAppInfo().userData`** — cast to `uint32_t*` and used as a theme color for the step indicator dots

### 6.2 Setting the icon

Icons are stored as `.bin` files in `main/assets/assets_bin/`. These are LVGL-compatible image descriptors packed into the assets SPIFFS partition at build time. At runtime, you load them with:

```cpp
static auto icon = assets::get_image("icon_myapp.bin");
setAppInfo().icon = (void*)&icon;
```

The `static` keyword is important — the `lv_image_dsc_t` returned by `get_image()` is a stack-allocated value that must persist for the lifetime of the app.

**To add a custom icon:**
1. Prepare a PNG image (ideally 64×64 or 96×96 pixels)
2. Convert to LVGL `.bin` format using [LVGL Image Converter](https://lvgl.io/tools/imageconverter) — select "True color with alpha" color format and "Binary RGB565 Swap" output format
3. Place the `.bin` file in `main/assets/assets_bin/`
4. Reference it in your app constructor

For Blinky, we'll skip the icon (the launcher will show an empty panel) and just set the theme color.

### 6.3 Setting the theme color

```cpp
static uint32_t theme_color = 0xFFAA00;  // Amber
setAppInfo().userData = (void*)&theme_color;
```

Again, `static` is required — the `uint32_t` must live for the app's lifetime. The `userData` field is a `void*` so the framework doesn't prescribe what it contains. StackChan's launcher uses it as an RGB color.

---

## 7. Design: Blinky App

### 7.1 Requirements

1. The app appears in the launcher with an amber theme color
2. On open, displays "LED: OFF" centered on screen with a home indicator
3. Every 500ms, toggles the LED ring between on (amber) and off
4. Updates the label to reflect the current LED state
5. On close, turns off the LED and cleans up all resources

### 7.2 Class diagram

```
AppAbility (mooncake)
  │
  │  overrides: onOpen, onRunning, onClose
  │
  └── AppBlinky
        ├── _led_on: bool             (current LED state)
        ├── _last_toggle: uint32_t    (timestamp of last toggle)
        └── _label: lv_obj_t*        (LVGL label widget)
```

### 7.3 State machine

```
                    open() from launcher
                         │
                         ▼
                   ┌──────────────┐
                   │  onOpen()    │
                   │  - Create label
                   │  - Create home indicator
                   │  - LED OFF
                   └──────┬───────┘
                          │
                          ▼
                   ┌──────────────┐
              ┌───▶│  onRunning() │◀───┐
              │    │              │    │
              │  Every 500ms:    │    │
              │  Toggle LED      │    │
              │  Update label    │    │
              │  Update home ind │    │
              │    └──────┬───────┘    │
              │           │            │
              │           └────────────┘
              │
              │  close() from home indicator
              │           │
              │           ▼
              │    ┌──────────────┐
              │    │  onClose()   │
              │    │  - LED OFF
              │    │  - Delete label
              │    │  - Destroy home indicator
              │    └──────────────┘
              │
              └── (loops until close)
```

### 7.4 Pseudocode

```
CLASS AppBlinky INHERITS AppAbility:

  CONSTRUCTOR:
    setAppInfo().name = "Blinky"
    static theme_color = 0xFFAA00  // Amber
    setAppInfo().userData = &theme_color

  PRIVATE:
    _led_on: bool = false
    _last_toggle: uint32_t = 0
    _label: lv_obj_t* = null

  METHOD onOpen():
    LOG "Blinky: on open"
    LOCK LVGL:
      _label = lv_label_create(lv_screen_active())
      lv_label_set_text(_label, "LED: OFF")
      lv_obj_set_align(_label, LV_ALIGN_CENTER)
      lv_obj_set_style_text_font(_label, &lv_font_montserrat_24, 0)
    UNLOCK LVGL

    create_home_indicator(callback={this.close()})
    _led_on = false
    _last_toggle = GetHAL().millis()

  METHOD onRunning():
    now = GetHAL().millis()
    IF now - _last_toggle > 500:
      _led_on = NOT _led_on
      IF _led_on:
        GetStackChan().leftNeonLight().setColor(168, 133, 0)   // Amber
        GetStackChan().rightNeonLight().setColor(168, 133, 0)
        LOCK LVGL:
          lv_label_set_text(_label, "LED: ON")
      ELSE:
        GetStackChan().leftNeonLight().setColor(0, 0, 0)
        GetStackChan().rightNeonLight().setColor(0, 0, 0)
        LOCK LVGL:
          lv_label_set_text(_label, "LED: OFF")
      _last_toggle = now

    update_home_indicator()

  METHOD onClose():
    LOG "Blinky: on close"
    GetStackChan().leftNeonLight().setColor(0, 0, 0)
    GetStackChan().rightNeonLight().setColor(0, 0, 0)

    LOCK LVGL:
      lv_obj_delete(_label)
      _label = null
    UNLOCK LVGL

    destroy_home_indicator()
```

### 7.5 Key design decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Use raw LVGL C API for the label | Not the smooth_ui_toolkit C++ wrapper | Fewer dependencies, simpler to understand for an intern; the template app uses the C++ `Button` wrapper but a label doesn't need click handling |
| Toggle at 500ms | Not faster, not slower | Fast enough to see the effect clearly; slow enough to not spam the LED driver |
| Use `setColor(168, 133, 0)` for amber | Not `setColor(255, 200, 0)` | The MCP tool recommends 0–168 as the safe range to avoid overdriving the WS2812C LEDs |
| Set both left and right neon lights | Not just one | The LED ring has two halves — setting only one side would look broken |
| Use `lv_obj_delete()` in onClose | Not `lv_obj_clean()` | We only have one label, so `delete` is sufficient. `clean` would delete all children of the screen, which could destroy the home indicator too |
| Don't use `LvglLockGuard` for LED calls | LED calls don't touch LVGL | The LED ring is driven by the RMT peripheral, not LVGL. No lock needed. |

---

## 8. Implementation

### 8.1 File structure

Create these files:

```
main/apps/app_blinky/
├── app_blinky.h
└── app_blinky.cpp
```

### 8.2 Header file

```cpp
// main/apps/app_blinky/app_blinky.h
/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>

/**
 * @brief Blinky App — toggles the LED ring on and off with a label display.
 *
 * Demonstrates the Mooncake app lifecycle, LVGL label creation, the home indicator,
 * and the StackChan neon light API.
 */
class AppBlinky : public mooncake::AppAbility {
public:
    AppBlinky();

    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    bool _led_on = false;
    uint32_t _last_toggle = 0;
    lv_obj_t* _label = nullptr;
};
```

### 8.3 Implementation file

```cpp
// main/apps/app_blinky/app_blinky.cpp
/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT
 */
#include "app_blinky.h"
#include <hal/hal.h>
#include <mooncake_log.h>
#include <stackchan/stackchan.h>
#include <apps/common/common.h>

using namespace mooncake;

static const std::string_view _tag = "Blinky";

AppBlinky::AppBlinky()
{
    // Configure app name (shown in the launcher's icon label)
    setAppInfo().name = "Blinky";

    // Configure theme color (shown in the launcher's step indicator)
    // Amber color to match the LED color we'll display
    static uint32_t theme_color = 0xFFAA00;
    setAppInfo().userData = (void*)&theme_color;
}

void AppBlinky::onOpen()
{
    mclog::tagInfo(_tag, "on open");

    // Create label — MUST hold LVGL lock
    {
        LvglLockGuard lock;
        _label = lv_label_create(lv_screen_active());
        lv_label_set_text(_label, "LED: OFF");
        lv_obj_set_align(_label, LV_ALIGN_CENTER);
        lv_obj_set_style_text_font(_label, &lv_font_montserrat_24, 0);
    }

    // Create home indicator (swipe-up-to-close button)
    view::create_home_indicator([this]() { close(); });

    // Initialize state
    _led_on = false;
    _last_toggle = GetHAL().millis();
}

void AppBlinky::onRunning()
{
    // Toggle LED every 500ms
    uint32_t now = GetHAL().millis();
    if (now - _last_toggle > 500) {
        _led_on = !_led_on;

        if (_led_on) {
            // Set LED ring to amber (safe range 0-168)
            GetStackChan().leftNeonLight().setColor(168, 133, 0);
            GetStackChan().rightNeonLight().setColor(168, 133, 0);

            LvglLockGuard lock;
            lv_label_set_text(_label, "LED: ON");
        } else {
            // Turn off LED ring
            GetStackChan().leftNeonLight().setColor(0, 0, 0);
            GetStackChan().rightNeonLight().setColor(0, 0, 0);

            LvglLockGuard lock;
            lv_label_set_text(_label, "LED: OFF");
        }

        _last_toggle = now;
    }

    // Must call every frame for home indicator spring animation
    view::update_home_indicator();
}

void AppBlinky::onClose()
{
    mclog::tagInfo(_tag, "on close");

    // Turn off LED ring
    GetStackChan().leftNeonLight().setColor(0, 0, 0);
    GetStackChan().rightNeonLight().setColor(0, 0, 0);

    // Destroy LVGL objects — MUST hold lock
    {
        LvglLockGuard lock;
        if (_label) {
            lv_obj_delete(_label);
            _label = nullptr;
        }
    }

    // Destroy home indicator
    view::destroy_home_indicator();
}
```

### 8.4 Register the app

**Edit `main/apps/apps.h`** — add one line:

```cpp
#pragma once
#include "app_launcher/app_launcher.h"
#include "app_ai_agent/app_ai_agent.h"
#include "app_avatar/app_avatar.h"
#include "app_setup/app_setup.h"
#include "app_espnow_ctrl/app_espnow_ctrl.h"
#include "app_app_center/app_app_center.h"
#include "app_ezdata/app_ezdata.h"
#include "app_dance/app_dance.h"
#include "app_blinky/app_blinky.h"     // ← ADD THIS LINE
```

**Edit `main/main.cpp`** — add one line after the other `installApp` calls:

```cpp
GetMooncake().installApp(std::make_unique<AppBlinky>());   // ← ADD THIS LINE
```

You'll also need to add `#include <lvgl.h>` in the header if it's not already pulled in by `mooncake.h`. In practice, the `lv_obj_t*` type and LVGL functions are available because `mooncake.h` includes the LVGL headers transitively through the framework's UI support. If you get a compile error about `lv_obj_t` being undefined, add `#include <lvgl.h>` to `app_blinky.h`.

---

## 9. Build, Flash, and Verify

### 9.1 Prerequisites

You need the firmware build environment set up. If you haven't done this before, follow the [M5StackChan Firmware Build Developer Guide](../../docs/guides/firmware-build-developer-guide.md) sections 1–5 first. In summary:

```bash
# One-time setup
git clone --depth 1 --branch v5.5.4 --recursive \
  https://github.com/espressif/esp-idf.git ~/esp/esp-idf-5.5.4
cd ~/esp/esp-idf-5.5.4 && ./install.sh esp32s3

# Clone firmware and fetch dependencies
git clone --depth 1 https://github.com/m5stack/StackChan.git ~/stackchan
cd ~/stackchan/firmware
python3 ./fetch_repos.py
```

### 9.2 Add the Blinky files

```bash
cd ~/stackchan/firmware

# Create the app directory
mkdir -p main/apps/app_blinky
```

Then create `app_blinky.h` and `app_blinky.cpp` as shown in section 8.2 and 8.3.

Then edit `main/apps/apps.h` and `main/main.cpp` as shown in section 8.4.

### 9.3 Build

```bash
source ~/esp/esp-idf-5.5.4/export.sh
cd ~/stackchan/firmware

# First build after adding new files — reconfigure to pick up the new .cpp
idf.py reconfigure

# Build
idf.py build
```

**Expected result:** The build should complete with 0 errors. You may see a few new warnings (unused variable, etc.) but no errors. The total build step count will be slightly higher than 2491 (the base count) because of your new files.

**If you get a compile error:**

| Error | Cause | Fix |
|-------|-------|-----|
| `'lv_obj_t' does not name a type` | Missing LVGL include | Add `#include <lvgl.h>` to `app_blinky.h` |
| `'GetStackChan' was not declared` | Missing StackChan include | Add `#include <stackchan/stackchan.h>` to `app_blinky.cpp` |
| `'view' has not been declared` | Missing common header | Add `#include <apps/common/common.h>` to `app_blinky.cpp` |
| `'LvglLockGuard' was not declared` | Missing lock header | Add `#include <smooth_lvgl.hpp>` to `app_blinky.cpp` |
| `undefined reference to AppBlinky` | App not found by glob | Make sure the file is under `main/apps/app_blinky/` and has a `.cpp` extension |
| `AppBlinky` not in launcher | Missing `installApp()` | Add the line in `main.cpp` |

### 9.4 Flash

```bash
# Make sure the StackChan is connected via USB-C
ls /dev/ttyACM*  # Should show /dev/ttyACM0

# Flash everything (bootloader, partition table, app, assets)
idf.py -p /dev/ttyACM0 flash
```

This takes about 25 seconds. The flash process writes to the `ota_0` partition (at offset `0x20000`), replacing the previous firmware.

### 9.5 Monitor and verify

```bash
idf.py -p /dev/ttyACM0 monitor
```

**What you should see:**

1. Normal boot sequence (6–7 seconds)
2. Mooncake launcher starts
3. In the launcher, you should see **9 apps** instead of the usual 8 — "Blinky" appears as a new icon panel
4. Swipe to the Blinky panel and tap to open it
5. The screen shows "LED: OFF" centered in Montserrat 24pt font
6. Every 500ms, the LED ring toggles between amber and off
7. The label updates between "LED: ON" and "LED: OFF"
8. Swipe up from the bottom to reveal the home button, then tap it to close
9. LED turns off, label disappears, you return to the launcher

**Serial log output you should see:**

```
I (xxxxx) Blinky: on open
I (xxxxx) Blinky: on close
```

### 9.6 Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| App doesn't appear in launcher | `installApp()` line missing or typo in `main.cpp` | Check the include and install lines |
| App appears but tapping does nothing | `onOpen()` crashes before creating UI | Check serial log for panic/crash; likely a null pointer or LVGL lock issue |
| LED doesn't toggle but label changes | `GetStackChan()` returns wrong object | Verify the StackChan subsystem is initialized before your app opens (it should be — `GetHAL().init()` runs before Mooncake) |
| Label shows but no home indicator | `create_home_indicator()` not called or `update_home_indicator()` not called every frame | Check both calls are present |
| Screen goes blank after opening | LVGL lock not held during object creation | Wrap all LVGL calls in `LvglLockGuard` |
| Device reboots after opening app | Hard fault in `onRunning()` | Check for null pointer dereferences; make sure `_label` is valid before calling `lv_label_set_text` |

---

## 10. Beyond Blinky: Extension Ideas

Once Blinky is working, here are ideas for extending it:

### 10.1 Button-controlled toggle

Instead of auto-toggling every 500ms, add a touch button that toggles the LED on tap:

```cpp
// In onOpen():
LvglLockGuard lock;
_btn = lv_button_create(lv_screen_active());
lv_obj_set_align(_btn, LV_ALIGN_CENTER);
lv_obj_set_size(_btn, 120, 50);
lv_obj_t* btn_label = lv_label_create(_btn);
lv_label_set_text(btn_label, "Toggle");
lv_obj_add_event_cb(_btn, [](lv_event_t* e) {
    auto* app = static_cast<AppBlinky*>(lv_event_get_user_data(e));
    app->_led_on = !app->_led_on;
    // ... update LED and label
}, LV_EVENT_CLICKED, this);
```

### 10.2 Color picker

Use the IMU tilt to control the LED hue:

```cpp
// In onRunning():
auto& imu = GetHAL().getImu();  // Or read from BMI270 directly
float roll = imu.getRoll();
float pitch = imu.getPitch();
// Map roll/pitch to hue/saturation
uint8_t r = ...; uint8_t g = ...; uint8_t b = ...;
GetStackChan().leftNeonLight().setColor(r, g, b);
GetStackChan().rightNeonLight().setColor(r, g, b);
```

### 10.3 Servo movement

Make the head turn when the LED is on:

```cpp
auto& motion = GetStackChan().motion();
if (_led_on) {
    motion.moveWithSpeed(0, 450, 600);  // Center, mid-pitch
} else {
    motion.moveWithSpeed(0, 0, 600);    // Center, bottom
}
```

---

## 11. Key File Reference

Every file you need to touch or understand for this project:

| File | Path | Purpose |
|------|------|---------|
| App header (create) | `main/apps/app_blinky/app_blinky.h` | Blinky class declaration |
| App implementation (create) | `main/apps/app_blinky/app_blinky.cpp` | Blinky lifecycle methods |
| App includes (edit) | `main/apps/apps.h` | Add `#include "app_blinky/app_blinky.h"` |
| Entry point (edit) | `main/main.cpp` | Add `installApp(std::make_unique<AppBlinky>())` |
| Template reference | `main/apps/app_template/app_template.h` | Official template — copy this structure |
| Template reference | `main/apps/app_template/app_template.cpp` | Official template — see lifecycle pattern |
| HAL API | `main/hal/hal.h` | All hardware access methods |
| LED API usage | `main/hal/hal_mcp.cpp:72-88` | MCP `set_led_color` tool — shows `setColor()` pattern |
| Home indicator | `main/apps/common/home_indicator/home_indicator.h` | Swipe-up home button API |
| Common components | `main/apps/common/common.h` | Aggregated includes for common UI |
| Mooncake framework | `components/mooncake/src/ability/ability.h` | `AppAbility` class, `AppInfo_t`, state machine |
| StackChan subsystem | `main/stackchan/stackchan.h` | Robot subsystem: avatar, motion, neon lights |
| Build system | `main/CMakeLists.txt` | Glob-based source collection (no edits needed) |
| Partition table | `partitions.csv` | Dual OTA + 4MB assets layout |
| SDK defaults | `sdkconfig.defaults` | ESP-IDF build configuration |
| Developer guide | `docs/guides/firmware-build-developer-guide.md` | Full build instructions |

---

## 12. API Quick Reference

### 12.1 Mooncake AppAbility

```cpp
// Constructor — set app info
setAppInfo().name = "MyApp";
setAppInfo().icon = (void*)&my_icon;
setAppInfo().userData = (void*)&my_color;

// Lifecycle
void onCreate() override;    // One-time: installed
void onOpen() override;      // UI creation
void onRunning() override;   // Every frame while active
void onClose() override;     // Cleanup
void close();                // Trigger close from inside

// State
State_t currentState();      // StateGoOpen, StateRunning, etc.
```

### 12.2 HAL

```cpp
GetHAL().millis();                        // uint32_t — ms since boot
GetHAL().delay(ms);                       // Blocking delay
GetHAL().lvglLock();                      // Acquire LVGL mutex
GetHAL().lvglUnlock();                    // Release LVGL mutex
GetHAL().feedTheDog();                    // Feed task watchdog
GetHAL().requestWarmReboot(appId);        // Warm reboot into specific app
```

### 12.3 StackChan subsystem

```cpp
GetStackChan().leftNeonLight().setColor(r, g, b);   // Left LED ring
GetStackChan().rightNeonLight().setColor(r, g, b);  // Right LED ring
GetStackChan().motion().moveWithSpeed(yaw, pitch, speed);  // Servo control
GetStackChan().avatar().setExpression(expr);                // Face expression
```

### 12.4 Common UI

```cpp
view::create_home_indicator(on_close_callback, btn_color, border_color);
view::update_home_indicator();    // Must call every frame!
view::destroy_home_indicator();
view::create_toast(message, duration_ms);
```

### 12.5 LVGL essentials

```cpp
// Object creation
lv_obj_t* label = lv_label_create(parent);
lv_label_set_text(label, "Hello");
lv_obj_set_align(label, LV_ALIGN_CENTER);
lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

// Screen
lv_screen_active();  // Returns lv_obj_t* for the current screen

// Deletion
lv_obj_delete(obj);  // Delete a single object and its children

// Lock (preferred method)
LvglLockGuard lock;  // RAII — acquires on construction, releases on destruction
```

---

## 13. Appendix: The Existing App Template (Reference Implementation)

For comparison, here is the official `app_template` that ships with the firmware. It's simpler than Blinky — it just has a quit button and prints "hi" every second. Study this to see the same lifecycle pattern in its most minimal form.

**`app_template.h`** — (`main/apps/app_template/app_template.h`)

```cpp
#pragma once
#include <mooncake.h>

class AppTemplate : public mooncake::AppAbility {
public:
    AppTemplate();
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;
};
```

**`app_template.cpp`** — (`main/apps/app_template/app_template.cpp`)

```cpp
#include "app_template.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <assets/assets.h>
#include <smooth_lvgl.hpp>

using namespace mooncake;
using namespace smooth_ui_toolkit::lvgl_cpp;

AppTemplate::AppTemplate()
{
    setAppInfo().name = "AppTemplate";
    // setAppInfo().icon = (void*)&icon_app_dummy;  // Commented out — no icon
}

void AppTemplate::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");
}

static std::unique_ptr<Button> _button_quit;
static uint32_t _time_count = 0;

void AppTemplate::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    LvglLockGuard lock;

    _button_quit = std::make_unique<Button>(lv_screen_active());
    _button_quit->setAlign(LV_ALIGN_CENTER);
    _button_quit->label().setText("QUIT");
    _button_quit->onClick().connect([this]() { close(); });
}

void AppTemplate::onRunning()
{
    if (GetHAL().millis() - _time_count > 1000) {
        mclog::tagInfo(getAppInfo().name, "hi");
        _time_count = GetHAL().millis();
    }
}

void AppTemplate::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    LvglLockGuard lock;
    _button_quit.reset();
}
```

**Key differences from Blinky:**

| Aspect | Template | Blinky |
|--------|----------|--------|
| UI widget | `Button` (C++ wrapper) | `lv_label_t` (C API) |
| Home indicator | None | Yes — needed for usability |
| Hardware access | None | LED ring via `GetStackChan()` |
| Theme color | Not set | Amber (0xFFAA00) |
| Static variables | File-scope `static` | Class member variables |
| Timing | Print "hi" every 1s | Toggle LED every 500ms |

---

## 14. Appendix: Partition Layout

Understanding the partition layout helps you know what gets flashed where:

```
Address    Size       Name       Type     Description
───────── ────────── ────────── ──────── ──────────────────
0x000000   32 KB     bootloader boot     Second-stage bootloader
0x008000    4 KB     partition  table    Partition table itself
0x009000   16 KB     nvs        data     WiFi credentials, settings
0x00D000    8 KB     otadata    data     OTA partition selection
0x00F000    4 KB     phy_init   data     RF calibration data
0x020000  4.9 MB     ota_0      app      Application partition A ← idf.py flash writes here
0x510000  4.9 MB     ota_1      app      Application partition B ← OTA updates write here
0xA00000  4.0 MB     assets     data     Images, fonts, emoji (SPIFFS)
0xE00000   64 KB     coredump   data     Crash dump storage
───────────────────────────────────────────────────────────
Total: 16 MB
```

When you run `idf.py flash`, the firmware binary goes to `ota_0`. The assets binary (containing the icons) goes to `assets`. The bootloader and partition table go to their fixed offsets. Your Blinky app is compiled into the `ota_0` binary along with all other apps — it's a single monolithic firmware image.

---

## 15. Appendix: Neon Light Internals

If you want to understand what happens when you call `setColor(r, g, b)`:

1. `GetStackChan().leftNeonLight()` returns a `CircularStrip` object (from `xiaozhi-esp32/main/led/circular_strip.cc`)
2. `CircularStrip` wraps the `espressif/led_strip` component, which drives WS2812C LEDs via the ESP-IDF RMT (Remote Control) peripheral
3. `setColor(r, g, b)` converts the RGB values to GRB format (WS2812C expects Green-Red-Blue byte order), writes them to the LED strip buffer, and calls `led_strip_refresh()` to push the data out on the RMT channel
4. The RMT peripheral bit-bangs the WS2812C protocol at 800 kHz — each LED receives 24 bits of color data

The LED ring has 12 LEDs total, split into left and right halves. Each `CircularStrip` instance controls one half (6 LEDs). Both strips share the same GPIO pin (the WS2812C data line is daisy-chained through all 12 LEDs).

**Safe color range:** The MCP tool definition limits RGB values to 0–168. This is because the WS2812C LEDs at full brightness (255) draw significant current and can cause voltage droop on the power rail, potentially causing brownout resets. For the Blinky app, 168 is perfectly bright and safe.
