---
Title: Diary — Tab5 boot logo display firmware
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Tab5-UserDemo/app/apps/app_startup_anim/app_startup_anim.cpp
    - Path: M5Tab5-UserDemo/app/assets/images/logo_tab.c
    - Path: M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp
      Note: Reference initialization sequence used for comparison
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c
      Note: Boot order for the reproduced failure
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c
      Note: Runtime failure investigation target
    - Path: esp32-s3-m5/ttmp/2026/04/21/ESP-49-TAB5-BOOTLOGO--tab5-boot-logo-display-firmware-guide/design-doc/01-tab5-boot-logo-display-firmware-design-and-implementation-guide.md
ExternalSources: []
Summary: ""
LastUpdated: 2026-04-21T19:00:00Z
WhatFor: ""
WhenToUse: ""
---


# Diary — Tab5 boot logo display firmware

## Entry 1 — 2026-04-21T19:00 — Research and design

**Goal:** understand the Tab5 display stack well enough to write a minimal boot logo firmware.

**Step 1 — read block diagram first**

Before reading any code, looked at `Tab5_Overall_Design_Block_Diagram.webp`. The block diagram gives the mental map: the ESP32-P4 is the main application processor, and it connects to all peripherals over GPIO, I2C, MIPI DSI, and other buses. The display is driven by the P4's MIPI DSI interface. The ESP32-C6 (U2) handles radio over SDIO and does not touch the display directly.

**Step 2 — read schematics for the display path**

Scanned the five schematic pages (`sch_tab5_b08_page_01.webp` through `_05.webp`) and the pinmap (`C145_Pinmap_Overview.png`). Key facts confirmed:

- The display is a 5-inch IPS TFT 1280×720 portrait with a ST7123 driver
- It connects to the P4 via MIPI DSI 2-lane over GPIO 14–19 (DSI_TX lane 0) and GPIO 20–25 (DSI_TX lane 1)
- Backlight is driven by a ME2212 boost converter enable signal controlled via GPIO expander PI4IOE
- The shared I2C bus uses GPIO 31/32 for display reset, touch, IMU, and audio control
- The pinmap confirms the display signals on the M5-Bus connector

**Step 3 — read the factory HAL init sequence**

`hal_esp32.cpp` shows the complete initialization sequence in order:

```text
camera_oscillator_init()
i2c_master_init(I2C_PORT_NUM)       ← shared I2C on GPIO 31/32
bsp_io_expander_init()                ← PI4IOE GPIO expanders
bsp_audio_init()                      ← ES7210 + ES8388 + NS4150B
bsp_imu_init()                        ← BMI270
bsp_power_monitor_init()              ← INA226
bsp_display_start_with_config(cfg)    ← display + lvgl_port + backlight
bsp_display_rotate(lv_disp, 90)       ← software 90° rotation
bsp_display_backlight_on()            ← enable backlight LED
```

The display initialization happens after all I/O expanders and audio are set up because those peripherals share the I2C bus. The display start function is the critical call; it creates the panel handle, initializes LVGL, and attaches the display.

**Step 4 — read the startup animation app**

`app_startup_anim.cpp` shows how images are created and animated. The C++ `Image` class wraps `lv_img_create()`. The `AppStartupAnim::onOpen()` callback:

1. Removes screen scrollability
2. Sets the background to white
3. Creates an image widget with `std::make_unique<Image>(lv_screen_active())`
4. Sets the source to `&logo_tab` (the `LV_IMG_DECLARE`d image)
5. Positions it off-screen with negative Y (above the top)
6. Animates it into position with spring interpolation

The logo is 180×80 pixels, RGB565, placed in a landscape canvas and rotated 90° by LVGL so it appears upright on the physical portrait screen.

**Step 5 — verify the logo image descriptor format**

Read `logo_tab.c`. The image is 180×80 pixels at RGB565 (28800 bytes). The descriptor is:

```c
const lv_image_dsc_t logo_tab = {
    .header.cf       = LV_COLOR_FORMAT_RGB565,
    .header.magic    = LV_IMAGE_HEADER_MAGIC,
    .header.w        = 180,
    .header.h        = 80,
    .data_size       = 14400 * 2,
    .data            = logo_tab_map,
};
```

The 28800-byte size checks out: 180 × 80 × 2 = 28800. The logo can be copied verbatim.

**Step 6 — review esp_lvgl_port display API**

`esp_lvgl_port.h` and `esp_lvgl_port_disp.h` were read to confirm the DSI-specific add function and the double-buffered PSRAM config. The key calls are:

- `esp_lvgl_port_init()` — starts the background task and timer
- `lvgl_port_add_disp_dsi()` — registers a DSI display with LVGL
- `lvgl_port_lock()` / `lvgl_port_unlock()` — mutex guards for cross-task LVGL access

The DSI config accepts flags including `.avoid_tearing` to prevent tearing artifacts on the 60 Hz panel.

**Step 7 — note the P4 software rotation**

The factory firmware sets `.sw_rotate = true` in the display flags. The P4 has no hardware display rotation engine, so the 90° rotation is done in software by LVGL. This means the application draws in landscape coordinates and LVGL rotates the output before sending it over DSI. The logo is 180×80 in landscape, so centering it with `lv_obj_center()` places it correctly after rotation.

**Step 8 — wrote the design document**

Wrote the design guide covering: the layered display stack (DSI → panel IO → ST7123 → lvgl_port → LVGL → app), the `LV_IMG_DECLARE` pattern, the double-buffered PSRAM buffer configuration, software rotation semantics, backlight control, and the boot sequence ordering constraint.

**Next steps:** implement the firmware in `0051-tab5-boot-logo`, copy the logo assets, write `display_app.c`, extend the SDK defaults, build, flash, and verify the logo appears on screen.

## Step 2: Build, flash, reproduce the failure, and write the bug report

This step turned the work from a design exercise into a concrete debugging exercise. The main outcome was that the new `0051-tab5-boot-logo` firmware now builds and flashes, but the display still does not come up. Instead of guessing from source alone, I pushed the firmware onto the actual Tab5, captured the runtime logs, and compared the custom display path against the original factory HAL bring-up order.

The important conclusion from this step is that the project is now in a very useful intermediate state: the build system, assets, partition sizing, and flashing flow are all real and working, but the display path is still architecturally incomplete. The runtime evidence strongly suggests that the firmware is attempting ST7123 panel communication before the board-level display prerequisites have been established.

### Prompt Context

**User prompt (verbatim):** "alright, write a detailed bug report and analysis with all the details you can in the ticket.

reate a detailed analysis and bug report that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable.

While doing so, you can also look in the original firmware and even run and compile it to see if you find anything."

**Assistant interpretation:** Create a long-form, intern-friendly failure analysis in the existing docmgr ticket, back it with file evidence and real runtime observations, and upload the finished bundle to reMarkable.

**Inferred user intent:** Preserve the debugging state in a way that helps a fresh engineer immediately understand the system, the failure, and the next steps instead of repeating the same exploration.

**Commit (code):** N/A

### What I did
- Built `esp32-s3-m5/0051-tab5-boot-logo` successfully after fixing asset linkage and partition sizing.
- Flashed the firmware to the Tab5 on `/dev/ttyACM0`.
- Captured the boot log and verified that the app reaches `display_app_init()` and powers the DSI PHY.
- Confirmed the app then stalls and triggers the watchdog in the MIPI DSI short-packet read path.
- Built the original reference firmware under `M5Tab5-UserDemo/platforms/tab5` to confirm the local factory checkout and toolchain are healthy.
- Compared the current custom `display_app.c` path against the original `HalEsp32::init()` sequence and the copied `m5stack_tab5` BSP implementation.
- Wrote a new detailed design doc / bug report in the ticket documenting the current state, evidence, probable root cause, and proposed repair direction.

### Why
- A build-only success is not enough for this board; the Tab5 is a board-integration problem, not just a compile problem.
- The only credible way to write a useful bug report was to observe the failing firmware on real hardware and compare it against the original known-good init order.
- The new document needed to be detailed enough that an intern could continue from it without re-reading the whole codebase from scratch.

### What worked
- The tutorial firmware now builds reproducibly.
- The app partition was expanded to 2 MB, so the large display-enabled image fits cleanly.
- The firmware flashes successfully to the board.
- The serial log captures a stable, repeatable failure signature.
- The original M5Stack Tab5 firmware also builds successfully in this environment, which increases confidence that the problem is in the tutorial adaptation rather than the local SDK/toolchain.

### What didn't work
- The display still does not initialize successfully.
- The firmware hangs during display bring-up and never reaches Wi-Fi / HTTP startup.
- The key runtime symptom is a watchdog-triggered stall in the DSI read path.

Exact relevant log evidence captured from `/tmp/tab5_boot_screen.log`:

```text
I (4031) tab5_boot_logo: boot
I (4031) display: Initializing MIPI DSI display (ST7123, 720x1280 portrait)
I (4031) M5STACK_TAB5: MIPI DSI PHY Powered on
E (9171) task_wdt: Task watchdog got triggered.
...
mipi_dsi_hal_host_gen_read_short_packet
```

Commands run:

```bash
cd esp32-s3-m5/0051-tab5-boot-logo
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/ttyACM0 flash

cd M5Tab5-UserDemo/platforms/tab5
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build
```

### What I learned
- The Tab5 display stack is not just `esp_lcd_new_dsi_bus()` plus a panel driver. Board-level setup matters.
- The original firmware’s ordering is extremely informative: `bsp_i2c_init()` and `bsp_io_expander_pi4ioe_init()` happen before display init.
- The copied BSP already contains those functions, but the tutorial app is not using them before the ST7123 bring-up.
- The stall location matches that hypothesis: the ST7123 driver tries to read panel ID via `esp_lcd_panel_io_rx_param(io, 0x04, ID, 3)` and appears to block waiting for a response.
- One earlier research note was too simplistic: the panel uses the P4’s dedicated MIPI DSI interface, not ordinary general-purpose GPIO numbers in the way a SPI/I2C peripheral would.

### What was tricky to build
- The binary outgrew the default 1 MB single-app partition once LVGL, the copied BSP, and display assets were linked in. I had to move the project onto a 2 MB custom factory partition before the image would pass the size check.
- The copied `m5stack_tab5` component also pulled in dependencies and assumptions that had to be made locally buildable before runtime debugging could even begin.
- The display failure is tricky because it is not a clean immediate `ESP_FAIL` return. Instead, the app gets far enough to power the DSI PHY and then stalls inside lower-level DSI read logic, which looks at first glance like a bus issue but is more likely an incomplete board-prep issue.

### What warrants a second pair of eyes
- Whether I2C + IO expander initialization alone is sufficient before the current custom display path, or whether the tutorial firmware should stop hand-rolling low-level display bring-up and switch directly to `bsp_display_start_with_config()`.
- Whether any additional delays or reset sequencing are required after IO-expander setup and before panel ID read.
- Whether the copied local BSP should remain in the tutorial project or be minimized after the display path is stabilized.

### What should be done in the future
- Patch `display_app.c` to perform the missing board prep (`bsp_i2c_init()`, `bsp_i2c_get_handle()`, `bsp_io_expander_pi4ioe_init(...)`) before DSI panel access.
- Prefer the higher-level BSP display wrapper path after proving the root cause.
- Re-test on hardware and update the bug report with the next observed failure mode or success result.
- Correct earlier documentation wherever it overstates the role of ordinary GPIO numbering in the DSI interface.

### Code review instructions
- Start with the new bug report document in `design-doc/02-...bring-up-failure-analysis.md`.
- Then compare these files in order:
  1. `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp`
  2. `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c`
  3. `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c`
  4. `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/esp_lcd_st7123.c`
- Validate by rebuilding, reflashing, and confirming whether the watchdog still fires in `mipi_dsi_hal_host_gen_read_short_packet`.

### Technical details
- Original HAL display init sequence evidence: `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp:51-105`
- Missing tutorial prerequisites: `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c:41-116`
- I2C and IO expander helpers already available locally: `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:100-127` and `:245-323`
- ST7123 ID read location: `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/esp_lcd_st7123.c:209-210`

## Step 3: Repair the display bring-up path and address the display underrun artifact

This step turned the ticket from a failure analysis into an actual repair. The first half of the fix addressed the architectural bug: the tutorial firmware was trying to talk to the ST7123 panel before reproducing the board-level preparation sequence from the original Tab5 HAL. The second half addressed the new symptom that appeared once the screen came alive: a visible fluttering edge artifact consistent with MIPI DSI underrun.

The key result is that the firmware now reaches a full ready state. The serial log shows successful ST7123 detection, touch initialization, backlight enable, logo rendering, ESP-Hosted SDIO startup, SoftAP startup, console startup, HTTP server startup, and the final `ready` log. After that, I compared the build against the original firmware again and found that the tutorial firmware was still running PSRAM at 20 MHz. Enabling the original firmware’s 200 MHz PSRAM setting—together with the required ESP-IDF experimental-features gate—produced a new boot where the log reported `I esp_psram: Speed: 200MHz`, and the earlier DSI underrun spam disappeared from the captured monitor log.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Finish the fix, keep using the original firmware as a reference, and write down the repaired state clearly when done.

**Inferred user intent:** Turn the investigation into a working or nearly-working firmware state, while preserving the full reasoning trail for later review.

**Commit (code):** N/A

### What I did
- Rewrote `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c` to stop hand-rolling low-level ST7123 bring-up in the app.
- Switched the display path to:
  - `bsp_i2c_init()`
  - `bsp_i2c_get_handle()`
  - `bsp_io_expander_pi4ioe_init(...)`
  - `bsp_reset_tp()`
  - `bsp_display_start_with_config(&cfg)`
  - `lv_display_set_rotation(...)`
  - `bsp_display_backlight_on()`
  - LVGL logo creation
- Simplified `display_app.h` accordingly.
- Built and flashed the repaired firmware.
- Captured the new serial log and confirmed the original ST7123 hang was gone.
- Wrote ticket-local scripts in `scripts/` with numeric prefixes, per instruction:
  - `01-flash-and-capture-monitor.sh`
  - `02-compare-display-config.sh`
  - `03-enable-psram-200m.sh`
- Compared the tutorial firmware config against the original firmware config and found the PSRAM speed mismatch.
- Enabled `CONFIG_IDF_EXPERIMENTAL_FEATURES=y` and `CONFIG_SPIRAM_SPEED_200M=y` so the P4 build could actually use 200 MHz PSRAM.
- Rebuilt, reflashed, and captured a new log that showed `I esp_psram: Speed: 200MHz`.
- Wrote a new detailed resolution report describing both the architectural repair and the later underrun fix.

### Why
- The first failure was caused by incorrect initialization order, so the fix had to follow the original firmware’s board-prep sequence.
- Once that was fixed, the visible display instability had to be treated as a second, independent problem rather than as a regression of the first bug.
- The original firmware was the right performance baseline; it already encoded the correct PSRAM speed for this board.

### What worked
- The BSP-driven display path works.
- The hard hang in ST7123 initialization is gone.
- The panel ID now reads successfully.
- The firmware now reaches the final application `ready` log.
- The ESP-Hosted Wi-Fi stack and HTTP server still come up after the display fix.
- After enabling 200 MHz PSRAM properly, the boot log shows the new speed and the previous DSI underrun errors no longer appeared in the capture window.

Concrete successful log evidence from `/tmp/tab5_boot_logo_monitor.log`:

```text
I (...) st7123: LCD ID: 80 A0 FB
I (...) M5STACK_TAB5: ST7123 Display initialized with resolution 720x1280
I (...) display: Display initialized -- M5 logo rendered on screen
I (...) tab5_text_echo_wifi: SoftAP browse: http://192.168.4.1/
I (...) tab5_text_echo_console: esp_console started over USB Serial/JTAG
I (...) tab5_boot_logo: ready — logo displayed, Wi-Fi running, HTTP echo server up
I esp_psram: Speed: 200MHz
```

### What didn't work
- My first attempt to improve the display stability by copying several factory performance-related config knobs at once was too aggressive and caused linker/layout failures in the tutorial app.
- Simply forcing `CONFIG_SPIRAM_SPEED_200M=y` was not enough at first, because ESP-IDF silently kept the effective speed at 20 MHz until `CONFIG_IDF_EXPERIMENTAL_FEATURES=y` was also enabled.
- Before the PSRAM fix, the user still observed a blue screen with a fluttering edge bar and the log emitted repeated underrun errors.

Observed pre-fix underrun symptom:

```text
E lcd.dsi.dpi: can't fetch data from external memory fast enough, underrun happens
```

### What I learned
- The first display bug was indeed an initialization-order bug, not a raw driver bug.
- The copied BSP was already good enough to fix the problem once I let it own the display startup instead of duplicating the low-level logic.
- On ESP32-P4, 200 MHz PSRAM is gated by `CONFIG_IDF_EXPERIMENTAL_FEATURES`, which is easy to miss if you only compare final config values and not the Kconfig dependencies.
- Once the display path becomes functional, second-order artifacts like bandwidth underrun become visible and need to be debugged separately.

### What was tricky to build
- The BSP-driven repair was conceptually simple but required resisting the temptation to keep patching the old custom ST7123 path. The cleaner fix was to delete complexity, not add more of it.
- The PSRAM speed change was tricky because the config looked edited but the runtime still reported 20 MHz until the hidden Kconfig dependency was satisfied.
- The original firmware was essential not just for code reference but for configuration reference. Without comparing against its `sdkconfig`, it would have been much harder to explain the display underrun symptom.

### What warrants a second pair of eyes
- The screen should still be visually confirmed on hardware after the 200 MHz PSRAM image. The serial evidence is very good, but the user’s eyes are the source of truth for the remaining display artifact question.
- The warning `ledc: GPIO 22 is not usable, maybe conflict with others` should be reviewed later, even though the backlight clearly comes on and the display works.
- The project now depends on a copied `m5stack_tab5` component with a few local edits; that is acceptable for ticket progress but should be reviewed for long-term maintainability.

### What should be done in the future
- Ask for a fresh hardware check on whether the fluttering edge bar is now gone or significantly improved after the 200 MHz PSRAM build.
- If any display artifact remains, compare more of the factory DSI/LVGL runtime config in small increments instead of broad config sweeps.
- If the display is visually stable, update the ticket to mark the display bug as resolved and move on to polish / cleanup work.

### Code review instructions
- Start with the repaired `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c` and compare it to `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp`.
- Then read the new resolution report in `design-doc/03-...resolution-report-and-residual-display-stability-notes.md`.
- Review the ticket-local scripts in `scripts/` to replay the flash / capture / config steps.
- Validate by:
  1. building the project,
  2. flashing it,
  3. confirming the monitor log shows `LCD ID: 80 A0 FB`, `Display initialized`, and `ready`,
  4. confirming `I esp_psram: Speed: 200MHz`,
  5. checking the screen with your eyes for any residual flutter.

### Technical details
- Repaired display path: `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c`
- Factory ordering baseline: `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp:51-105`
- BSP display wrapper path: `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:1497+`
- Kconfig gate for 200 MHz PSRAM: `/home/manuel/esp/esp-idf-5.4.2/components/esp_psram/esp32p4/Kconfig.spiram`
- Monitor helper: `scripts/01-flash-and-capture-monitor.sh`
- Config comparison helper: `scripts/02-compare-display-config.sh`
- PSRAM tuning helper: `scripts/03-enable-psram-200m.sh`
