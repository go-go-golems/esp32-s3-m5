---
Title: Tab5 boot logo display failure resolution report and residual display stability notes
Ticket: ESP-49-TAB5-BOOTLOGO
Status: active
Topics:
    - firmware
    - display
    - lvgl
    - mipidsi
    - boot
    - esp-idf
    - m5stack
    - tab5
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp
      Note: |-
        Factory reference initialization order that guided the repair.
        Factory initialization order that guided the repair
    - Path: M5Tab5-UserDemo/platforms/tab5/sdkconfig.defaults
      Note: |-
        Factory configuration showing the need for CONFIG_IDF_EXPERIMENTAL_FEATURES before 200 MHz PSRAM is allowed.
        Factory PSRAM and performance baseline
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c
      Note: |-
        Application entrypoint showing display-first startup followed by Wi-Fi and HTTP services.
        Display-first application startup sequence
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c
      Note: |-
        Final repaired display bring-up path that now follows the BSP initialization order.
        Repaired BSP-driven display bring-up path
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/display_app.h
      Note: |-
        Simplified display app public contract after removing the custom low-level ST7123 path.
        Simplified display API after dropping the manual ST7123 path
    - Path: esp32-s3-m5/0051-tab5-boot-logo/sdkconfig
      Note: |-
        Active build configuration after enabling ESP32-P4 experimental features and 200 MHz PSRAM.
        Active runtime configuration showing 200 MHz PSRAM
    - Path: esp32-s3-m5/0051-tab5-boot-logo/sdkconfig.defaults
      Note: |-
        Persisted defaults reflecting the runtime-stable PSRAM setting.
        Persisted defaults for the tuned build
    - Path: esp32-s3-m5/ttmp/2026/04/21/ESP-49-TAB5-BOOTLOGO--tab5-boot-logo-display-firmware-guide/scripts/01-flash-and-capture-monitor.sh
      Note: |-
        Ticket-local script used to flash the firmware and capture the serial monitor log.
        Traceable flash and monitor capture helper
    - Path: esp32-s3-m5/ttmp/2026/04/21/ESP-49-TAB5-BOOTLOGO--tab5-boot-logo-display-firmware-guide/scripts/02-compare-display-config.sh
      Note: |-
        Ticket-local config comparison helper against the factory firmware.
        Factory/tutorial display config comparison helper
    - Path: esp32-s3-m5/ttmp/2026/04/21/ESP-49-TAB5-BOOTLOGO--tab5-boot-logo-display-firmware-guide/scripts/03-enable-psram-200m.sh
      Note: |-
        Ticket-local script used to enable 200 MHz PSRAM in a reproducible way.
        Ticket-local PSRAM tuning helper
ExternalSources:
    - https://components.espressif.com/components/espressif/esp_lvgl_port
Summary: Resolution report for the Tab5 boot logo firmware failure. The original display-init hang was fixed by switching from the hand-rolled low-level ST7123 path to the BSP-driven board-preparation and display startup sequence. A follow-up display stability issue was traced to PSRAM throughput, and the system was aligned with the factory firmware's 200 MHz PSRAM configuration.
LastUpdated: 2026-04-22T02:40:00Z
WhatFor: Use this document to understand what was broken, what was changed, why the repair worked, and what remains to be visually confirmed on hardware.
WhenToUse: Use after reading the initial failure analysis, when you want the repaired state and its evidence in one place.
---


# Tab5 boot logo display failure resolution report and residual display stability notes

## Executive Summary

The Tab5 boot logo firmware is now in a significantly better state than the original failing build.

At the beginning of this debugging pass, the firmware:

- built successfully,
- flashed successfully,
- reached `app_main()`,
- started display initialization,
- powered the MIPI DSI PHY,
- and then hung in the ST7123 panel initialization path until the watchdog fired.

That hard failure has now been repaired.

The key fix was **architectural**, not cosmetic: the application stopped trying to manually reconstruct the entire low-level ST7123 bring-up sequence in `main/display_app.c`, and instead began following the board-preparation order used by the original M5Stack firmware. In practice, that meant:

1. initialize the shared I2C bus,
2. initialize the PI4IOE I/O expanders,
3. reset the touch/LCD-related lines through the expander-controlled path,
4. start the display through `bsp_display_start_with_config()`,
5. rotate the display,
6. enable the backlight,
7. and only then create the LVGL logo object.

After that fix, the serial log showed successful display startup all the way through:

- ST7123 controller detected,
- ST7123 touch initialized,
- display initialized,
- backlight enabled,
- logo rendered,
- Wi-Fi stack started,
- HTTP server started,
- app reached its `ready` log.

A second issue then became visible: the user reported that the screen had turned blue and a narrow fluttering grey bar appeared on one edge. That symptom matched repeated serial errors of the form:

```text
lcd.dsi.dpi: can't fetch data from external memory fast enough, underrun happens
```

That is not the original bring-up bug. It is a display feed / memory bandwidth problem. The most effective next correction came from comparing the project against the original factory firmware configuration and noticing that the factory build uses **200 MHz PSRAM**, while the tutorial firmware was still effectively running PSRAM at **20 MHz**.

A crucial ESP-IDF detail turned out to matter here:

- on ESP32-P4, `CONFIG_SPIRAM_SPEED_200M` is gated behind `CONFIG_IDF_EXPERIMENTAL_FEATURES=y`.

Once that gating flag was enabled and the firmware was rebuilt, the new boot logs showed:

```text
I esp_psram: Speed: 200MHz
```

and the captured serial log no longer showed the previous DSI underrun errors during the capture window.

So the current state is:

- **hard display-init hang:** fixed
- **display + logo + Wi-Fi + HTTP startup:** working in serial evidence
- **PSRAM speed issue causing display underrun:** addressed
- **remaining work:** confirm on-device visual stability after the 200 MHz PSRAM build, and document any remaining artifacts if they still exist

---

## Problem Statement and Context

This document closes the loop on two closely related display problems in the `0051-tab5-boot-logo` tutorial firmware:

1. the original **display initialization hang**, and
2. the later **visible display instability / edge flutter** caused by DSI display underrun.

The audience for this report is a new engineer who was not present during the debugging session and needs both the technical facts and the reasoning behind the final changes.

### Scope

This document covers:

- what the tutorial firmware was trying to do,
- why the first implementation failed,
- what was changed to fix that failure,
- why the second visual artifact appeared,
- what was learned from the original factory firmware,
- and what remains to validate.

It does not attempt to cover every future refinement of the project, such as:

- touch UI development,
- animation work,
- LVGL screen composition beyond the static logo,
- long-term BSP cleanup,
- or final polishing of the build configuration.

---

## Baseline: What the Firmware is Supposed to Do

The tutorial firmware lives at:

- `esp32-s3-m5/0051-tab5-boot-logo`

Its intended runtime behavior is:

```text
boot
  -> initialize display
  -> show M5 logo on the Tab5 screen
  -> continue running Wi-Fi (ESP-Hosted via ESP32-C6)
  -> continue running the HTTP echo server
  -> expose esp_console over USB Serial/JTAG
```

The application entrypoint remains deliberately simple:

- `esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c`

Current startup sequence:

```c
ESP_ERROR_CHECK(display_app_init());
ESP_ERROR_CHECK(echo_state_init());
ESP_ERROR_CHECK(wifi_app_start());
wifi_console_start();
ESP_ERROR_CHECK(http_server_start());
```

This means the display path is a hard prerequisite for the rest of the demo.

---

## Part I — Original Failure: Why the First Display Bring-up Hung

## What the first version did

The original custom `display_app.c` tried to perform display bring-up almost entirely by hand:

- backlight PWM init,
- DSI PHY power enable,
- DSI bus creation,
- DBI panel IO creation,
- ST7123 panel object creation,
- panel reset,
- panel init,
- display-on command,
- LVGL port init,
- DSI display registration with LVGL,
- logo rendering.

That looked reasonable from a pure display-driver perspective, but it missed a critical board-level truth:

> On the Tab5, the display is not ready just because the DSI PHY exists.

The panel also depends on board preparation through shared I2C and IO expanders.

## What the original firmware does instead

The factory firmware under `M5Tab5-UserDemo/platforms/tab5` initializes the board in this order:

1. camera oscillator,
2. shared I2C,
3. IO expanders,
4. codec / IMU / RTC / power-monitor pieces,
5. touch reset,
6. display startup through the BSP wrapper,
7. software rotation,
8. backlight on.

Evidence:

- `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp:47-105`

This sequence matters because the PI4IOE expanders drive signals such as:

- `LCD_RST`
- `TP_RST`
- `EXT5V_EN`
- other board-level enable lines

Evidence for that is in the copied BSP implementation:

- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:245-323`

The critical comment in that file explicitly says PI4IOE1 sets outputs including `EXT5V_EN`, `LCD_RST`, and `TP_RST` high.

## What the observed failure looked like

The serial logs from the failing build showed:

```text
I (...) tab5_boot_logo: boot
I (...) display: Initializing MIPI DSI display (ST7123, 720x1280 portrait)
I (...) M5STACK_TAB5: MIPI DSI PHY Powered on
E (...) task_wdt: Task watchdog got triggered
```

And the backtrace repeatedly pointed into:

- `mipi_dsi_hal_host_gen_read_short_packet`

That lines up with the ST7123 driver implementation in:

- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/esp_lcd_st7123.c`

where the panel initialization performs an early panel ID read:

```c
uint8_t ID[3];
ESP_RETURN_ON_ERROR(esp_lcd_panel_io_rx_param(io, 0x04, ID, 3), TAG, "read ID failed");
```

So the panel was being spoken to over DSI before the board-level prerequisites had been properly satisfied.

### Root cause of the first bug

The first bug was therefore:

> **The tutorial firmware attempted low-level ST7123 communication before reproducing the Tab5’s board-level display preparation sequence.**

That sequence depends on:

- shared I2C initialization,
- IO-expander initialization,
- and expander-controlled reset/enable lines.

---

## Part II — The Repair: What Was Changed

## Design decision

Instead of continuing to debug the hand-rolled low-level panel path, the firmware was moved closer to the factory architecture.

That was the right choice for three reasons:

1. the factory sequence is already known-good,
2. this tutorial is supposed to teach the board, not reimplement the BSP,
3. and using the BSP wrapper reduces the number of board-specific failure points.

## New display initialization path

The repaired `display_app.c` now does this:

```text
display_app_init()
  -> bsp_i2c_init()
  -> bsp_i2c_get_handle()
  -> bsp_io_expander_pi4ioe_init(i2c)
  -> bsp_reset_tp()
  -> bsp_display_start_with_config(&cfg)
  -> lv_display_set_rotation(..., 90)
  -> bsp_display_backlight_on()
  -> create centered logo image
```

### Why each step exists

#### 1. `bsp_i2c_init()`
The shared board I2C bus must exist before the expanders can be configured.

#### 2. `bsp_io_expander_pi4ioe_init(i2c)`
This initializes the PI4IOE chips that drive board-level display/touch/power/reset lines.

#### 3. `bsp_reset_tp()`
This matches the factory sequence and toggles expander-controlled reset lines used by touch and LCD-related logic.

#### 4. `bsp_display_start_with_config(&cfg)`
This lets the BSP own:

- LVGL port init,
- backlight PWM init,
- display handle creation,
- ST7123 wrapper selection,
- touch attachment,
- and LVGL display registration.

#### 5. `lv_display_set_rotation(..., LV_DISPLAY_ROTATION_90)`
The Tab5 uses a portrait physical panel but the application draws in landscape logical coordinates.

#### 6. `bsp_display_backlight_on()`
The backlight is explicitly enabled after display startup.

#### 7. `lv_img_create()` with `logo_tab`
Only once the LVGL display is truly alive does the app create the logo widget.

## Key file changed

- `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c`

## Result after this repair

The hard hang disappeared.

The new serial log showed:

```text
I (...) display: Initializing Tab5 display via BSP bring-up path
I (...) M5STACK_TAB5: reset tp
I (...) LVGL: Starting LVGL task
I (...) M5STACK_TAB5: Install LCD driver of ST7123
I (...) st7123: LCD ID: 80 A0 FB
I (...) M5STACK_TAB5: ST7123 Display initialized with resolution 720x1280
I (...) M5STACK_TAB5: ST7123 touch panel initialized successfully
I (...) M5STACK_TAB5: Setting LCD backlight: 100%
I (...) display: Display initialized -- M5 logo rendered on screen
```

This is the clearest confirmation that the original architectural diagnosis was correct.

---

## Part III — Second Issue: Display Instability and the Fluttering Edge Bar

Once the display started working, a second symptom became visible on hardware.

### User-reported symptom

The user reported:

- the screen turned blue,
- there was visible jitter,
- and one side showed a narrow fluttering grey bar roughly a few pixels wide.

This is a classic example of a second-order bug that was hidden behind the first one. The original hard failure prevented any visual debugging, so the display stability issue only became visible once the panel actually came up.

## Serial evidence matched the visual artifact

The serial log during that phase contained repeated messages like:

```text
E lcd.dsi.dpi: can't fetch data from external memory fast enough, underrun happens
```

That error means the display engine is not being fed pixel data quickly enough from memory. In other words:

```text
display refresh demand > memory / bus delivery rate
```

When that happens, visible artifacts such as edge tearing, flashing vertical bars, or fluttering regions are expected.

### Important distinction

This second bug was **not** another ST7123 bring-up bug.

It was a **throughput / memory-configuration issue** after the display was already functional.

---

## Part IV — Using the Original Firmware as a Performance Baseline

This is where the original firmware was especially useful.

Comparing the tutorial firmware against the original `M5Tab5-UserDemo/platforms/tab5/sdkconfig` showed an important difference:

### Original firmware

- `CONFIG_SPIRAM_SPEED_200M=y`

### Tutorial firmware at the time of the fluttering artifact

- `CONFIG_SPIRAM_SPEED_20M=y`

That was a huge clue. A 720x1280 DSI display with large LVGL buffers in PSRAM is much more likely to underrun when PSRAM is still running at 20 MHz.

## Important ESP-IDF detail discovered during debugging

The ESP-IDF Kconfig for ESP32-P4 PSRAM says:

- `CONFIG_SPIRAM_SPEED_200M`
- depends on
- `CONFIG_IDF_EXPERIMENTAL_FEATURES`

This matters because simply editing `sdkconfig` to say `SPIRAM_SPEED_200M=y` is not enough unless the experimental-features gate is also enabled.

This exact dependency was found in:

- `/home/manuel/esp/esp-idf-5.4.2/components/esp_psram/esp32p4/Kconfig.spiram`

Relevant logic:

```text
config SPIRAM_SPEED_200M
    depends on IDF_EXPERIMENTAL_FEATURES
```

That explains why the early attempt to force 200 MHz PSRAM did not “stick”: Kconfig silently resolved back to 20 MHz because the dependency was not satisfied.

---

## Part V — Second Repair: Enable 200 MHz PSRAM Correctly

## What was changed

The project was updated so that both:

- `CONFIG_IDF_EXPERIMENTAL_FEATURES=y`
- `CONFIG_SPIRAM_SPEED_200M=y`

were enabled.

This was done in a traceable way using the ticket-local script:

- `scripts/03-enable-psram-200m.sh`

## Verification from serial log

After rebuilding and reflashing, the monitor log showed:

```text
I esp_psram: Speed: 200MHz
```

It also showed DQS tuning succeeding:

```text
I (196) MSPI DQS: tuning success, best phase id is 0
I (369) MSPI DQS: tuning success, best delayline id is 17
```

That is strong evidence that the new PSRAM configuration was actually applied.

## Result after the PSRAM change

The captured serial log after the 200 MHz build showed:

- successful display init,
- successful touch init,
- successful Wi-Fi / HTTP startup,
- and **no DSI underrun messages during the capture window**.

That is a strong sign that the fluttering artifact should be reduced or eliminated, though the final truth still depends on human eyes on the physical display.

---

## Final Observed Runtime Sequence After the Fixes

The repaired firmware now reaches the following milestones in one boot:

```text
boot
  -> display init via BSP
  -> ST7123 LCD ID read succeeds
  -> display initialized
  -> touch initialized
  -> backlight enabled
  -> logo rendered
  -> ESP-Hosted SDIO link starts
  -> SoftAP comes up
  -> esp_console starts
  -> HTTP server starts
  -> app logs ready
```

The key final runtime evidence includes:

```text
I (...) st7123: LCD ID: 80 A0 FB
I (...) M5STACK_TAB5: ST7123 Display initialized with resolution 720x1280
I (...) display: Display initialized -- M5 logo rendered on screen
I (...) tab5_text_echo_wifi: SoftAP browse: http://192.168.4.1/
I (...) tab5_text_echo_console: esp_console started over USB Serial/JTAG
I (...) tab5_text_echo_http: starting server on port 80
I (...) tab5_boot_logo: ready — logo displayed, Wi-Fi running, HTTP echo server up
```

That is the first fully successful end-to-end startup sequence for this tutorial firmware.

---

## Remaining Residual Risk

## What is fixed with high confidence

1. **Display initialization hang**
   - fixed
2. **Panel ID read stall**
   - fixed
3. **App blocked before Wi-Fi / HTTP startup**
   - fixed
4. **PSRAM speed stuck at 20 MHz**
   - fixed
5. **Serial DSI underrun spam seen in the earlier display-active build**
   - not observed in the new capture window

## What still needs explicit visual confirmation

Although the new serial evidence is very good, there is still one residual item that should be treated honestly:

- the user-reported visible fluttering edge bar should be confirmed on the physical display after the 200 MHz PSRAM build.

In other words, current status is:

- **serial evidence says much better**,
- **human visual confirmation still desirable**.

That is the correct engineering posture: do not overstate what has been proven.

---

## Implementation Notes for a New Intern

If you inherit this work, the two lessons to remember are:

### Lesson 1 — preserve initialization order before preserving API shape

When porting from the original vendor HAL to a smaller tutorial firmware, the initialization order is often more important than the exact low-level code.

The first failure happened because the app copied the low-level panel calls but not the board preparation order.

### Lesson 2 — always compare runtime symptoms to the original firmware configuration

The second failure looked like a visual artifact, but the serial errors and config diff pointed to PSRAM bandwidth. The original firmware already contained the answer:

- it ran PSRAM at 200 MHz,
- and the tutorial build did not.

That comparison turned a vague “jitter on one side” symptom into a concrete, testable hypothesis.

---

## Scripts Written During This Repair

To make the debugging sequence traceable, the following scripts were stored in the ticket’s `scripts/` directory:

1. `scripts/01-flash-and-capture-monitor.sh`
   - flashes the current firmware,
   - starts a detached monitor session,
   - captures the serial output,
   - and prints the tail of the log.

2. `scripts/02-compare-display-config.sh`
   - compares the tutorial firmware config against the original Tab5 firmware config.

3. `scripts/03-enable-psram-200m.sh`
   - enables `CONFIG_IDF_EXPERIMENTAL_FEATURES=y`
   - and requests `CONFIG_SPIRAM_SPEED_200M=y`
   - so that the runtime PSRAM throughput matches the factory design more closely.

These scripts are not product code. They are ticket-local debugging artifacts meant to preserve the investigation flow.

---

## Practical Next Steps

## Immediate next step

Ask for a fresh visual check on the hardware after the 200 MHz PSRAM image.

Suggested question:

> Does the logo now appear stable, without the narrow fluttering grey bar at the edge?

## If the user still sees a residual artifact

Then the next debugging pass should compare more of the factory runtime-relevant config, in this order:

1. DSI / LVGL tearing-avoidance settings
2. backlight GPIO warning (`GPIO 22 is not usable, maybe conflict with others`)
3. full-refresh / direct-mode display flags
4. additional cache or optimization settings, but only in small controlled steps

The important thing is not to regress into another broad config sweep. The successful pattern here was:

- change one root-cause hypothesis at a time,
- rebuild,
- reflash,
- observe the board,
- preserve the evidence.

---

## References

### Code and configuration

- `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c`
- `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.h`
- `esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c`
- `esp32-s3-m5/0051-tab5-boot-logo/sdkconfig`
- `esp32-s3-m5/0051-tab5-boot-logo/sdkconfig.defaults`
- `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp`
- `M5Tab5-UserDemo/platforms/tab5/sdkconfig`
- `M5Tab5-UserDemo/platforms/tab5/sdkconfig.defaults`
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c`
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/esp_lcd_st7123.c`
- `/home/manuel/esp/esp-idf-5.4.2/components/esp_psram/esp32p4/Kconfig.spiram`

### Ticket-local debugging scripts

- `scripts/01-flash-and-capture-monitor.sh`
- `scripts/02-compare-display-config.sh`
- `scripts/03-enable-psram-200m.sh`

### Captured runtime logs

- `/tmp/tab5_boot_logo_monitor.log`
- earlier failing capture: `/tmp/tab5_boot_screen.log`
