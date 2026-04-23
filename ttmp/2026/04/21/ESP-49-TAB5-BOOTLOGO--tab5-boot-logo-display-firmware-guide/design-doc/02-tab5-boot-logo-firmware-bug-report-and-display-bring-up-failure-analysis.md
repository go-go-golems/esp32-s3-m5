---
Title: Tab5 boot logo firmware bug report and display bring-up failure analysis
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
    - Path: M5Tab5-UserDemo/platforms/tab5/components/m5stack_tab5/include/bsp/display.h
      Note: |-
        Public BSP display constants and API contracts, including panel resolution, DSI power LDO, and backlight APIs.
        Public display constants and BSP display API contracts
    - Path: M5Tab5-UserDemo/platforms/tab5/main/app_main.cpp
      Note: |-
        Factory firmware entrypoint that injects the hardware abstraction layer and starts the main application loop.
        Factory entrypoint showing HAL injection and app lifecycle
    - Path: M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp
      Note: |-
        Canonical initialization order for camera clock, I2C, IO expanders, codecs, IMU, RTC, and display.
        Known-good board initialization order and display startup flow
    - Path: esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/esp_lcd_st7123.c
      Note: |-
        ST7123 panel driver implementation; the runtime stall occurs during its ID read.
        Panel driver where the ST7123 ID read happens
    - Path: esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c
      Note: |-
        Copied BSP implementation with I2C, IO expander, backlight, DSI power, and ST7123 wrapper functions.
        Copied BSP with I2C
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/CMakeLists.txt
      Note: Current component wiring, embedded assets, and BSP dependency list.
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c
      Note: |-
        Tutorial firmware entrypoint showing current boot order.
        Current tutorial boot order
    - Path: esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c
      Note: |-
        Current custom display bring-up path that hangs at runtime.
        Current failing custom display bring-up path
ExternalSources:
    - https://components.espressif.com/components/espressif/esp_lvgl_port
Summary: Detailed bug report for the current Tab5 boot logo firmware failure. The firmware builds and flashes successfully, but display bring-up stalls in ST7123 MIPI DSI panel initialization. The most likely cause is missing board-level preparation before panel access, especially I2C and IO-expander-controlled display power/reset sequencing.
LastUpdated: 2026-04-22T02:15:00Z
WhatFor: Use this document to onboard a new engineer to the Tab5 display stack, reproduce the failure, understand the evidence, and implement the next repair step.
WhenToUse: Use when debugging the Tab5 boot logo firmware, reviewing the failed bring-up attempt, or planning the next iteration of the display initialization path.
---


# Tab5 boot logo firmware bug report and display bring-up failure analysis

## Executive Summary

We do have a docmgr ticket for this work: `ESP-49-TAB5-BOOTLOGO`, stored under `esp32-s3-m5/ttmp/2026/04/21/ESP-49-TAB5-BOOTLOGO--tab5-boot-logo-display-firmware-guide`.

The current `0051-tab5-boot-logo` firmware is **not yet functionally correct** even though it now **builds, fits in flash, and flashes successfully**. The firmware reaches `app_main()`, begins display initialization, powers the MIPI DSI PHY, and then stalls during ST7123 panel initialization. The watchdog later fires because the main task never returns from the DSI panel path. The most important runtime evidence is that the stall occurs in the DSI short-packet read path while the ST7123 driver is trying to read the panel ID.

The most likely explanation is that the firmware is trying to talk to the display controller **before the board-level display prerequisites are established**. The original factory firmware performs display bring-up only after I2C and the PI4IOE I/O expanders have been initialized. Those expanders drive important non-DSI control signals such as LCD reset and power-enables. The custom `display_app.c` currently skips that broader board-preparation sequence and attempts a low-level DSI bring-up too early.

In short:

- **What works now:** project scaffolding, asset wiring, BSP import, build, flash, serial logging, 2 MB app partition.
- **What fails now:** display bring-up; the logo never renders; the app does not progress to Wi-Fi/HTTP startup.
- **Most likely root cause:** missing I2C + IO-expander-controlled display reset/power preparation before ST7123 panel access.
- **Recommended direction:** stop hand-rebuilding the entire display stack in `main/display_app.c`; first reproduce the board-prep sequence from the factory HAL, then prefer the BSP wrapper `bsp_display_start_with_config()` path over the current manual ST7123 wiring.

---

## Problem Statement and Scope

The goal of the `0051-tab5-boot-logo` firmware is straightforward: boot on the M5Stack Tab5, initialize the onboard 5-inch display, show the M5 logo, then continue running the existing Wi-Fi and HTTP stack in the background.

The current failure prevents the very first visible milestone: the firmware does not reliably finish display initialization. This makes the board appear dead or stuck even though the serial console shows partial progress.

This document is limited to the current display bring-up failure and the engineering context needed to understand it. It does **not** attempt to solve every future part of the firmware (touch, animations, mooncake integration, runtime UI, or production hardening). Instead, it explains:

1. what the Tab5 hardware/software stack looks like,
2. how the original firmware initializes the board,
3. how the new tutorial firmware differs,
4. where the current failure occurs,
5. why the current architecture is likely incomplete,
6. and what the next debugging/fix steps should be.

---

## System Orientation for a New Intern

Before looking at the bug itself, a new engineer needs a basic mental model of the Tab5.

### The board is a two-chip system

The Tab5 is not a normal single-SoC ESP board.

- **ESP32-P4** is the main application processor.
  - It runs the user application.
  - It owns the display stack, LVGL, peripherals, filesystems, and most local device logic.
- **ESP32-C6** is the radio sidecar.
  - It provides Wi-Fi/BT/802.15.4.
  - The P4 talks to it through the ESP-Hosted / `esp_wifi_remote` stack over SDIO.

That means there are really two mostly independent subsystems:

```text
+-------------------+        SDIO / ESP-Hosted        +----------------------+
| ESP32-P4          | <-----------------------------> | ESP32-C6             |
| - app_main()      |                                 | - Wi-Fi / BT radios  |
| - LVGL            |                                 | - remote Wi-Fi stack |
| - MIPI DSI        |                                 +----------------------+
| - I2C peripherals |
+-------------------+
```

The display problem in this ticket lives entirely on the **P4 side**.

### The display is not only a DSI peripheral

It is tempting to think the display path is simply:

```text
P4 DSI controller -> panel driver -> screen
```

That is incomplete.

The real board-level path also includes:

- DSI PHY power through an on-chip LDO channel
- board I2C initialization
- PI4IOE I/O expanders
- display reset and enable signals controlled by those expanders
- backlight PWM through LEDC
- panel vendor initialization commands
- LVGL display registration

A more realistic mental model is:

```text
P4 app code
  -> board prep (I2C, IO expander, reset/enables)
  -> DSI PHY power
  -> DSI bus
  -> DBI command transport
  -> ST7123 panel driver
  -> LVGL port
  -> LVGL objects
  -> visible pixels
```

If the board-prep layer is missing, the lower-level DSI code can compile perfectly and still hang at runtime.

### There are two firmware worlds in this repo

There are two relevant firmware worlds in this workspace:

1. **Factory/reference firmware** under `M5Tab5-UserDemo/platforms/tab5`
   - Uses M5Stack’s app framework and HAL abstraction.
   - Contains the working board init order.
2. **Tutorial firmware** under `esp32-s3-m5/0051-tab5-boot-logo`
   - A simpler, tutorial-style ESP-IDF app.
   - Reuses some pieces from earlier Tab5 work and copies the Tab5 BSP locally.

The core debugging task is to compare those two worlds and identify what was lost in translation.

---

## Reproduction Summary

### Current tutorial firmware

Project:

- `esp32-s3-m5/0051-tab5-boot-logo`

Entry path:

- `esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c`

Current boot order:

- call `display_app_init()` first,
- then start echo state, Wi-Fi, console, and HTTP server.

Evidence:

- `esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c:22-34`

### Build status

The tutorial firmware now builds successfully after several integration fixes:

- local BSP component copied in,
- logo asset compiled into the app,
- JS asset embedded,
- partition table enlarged to 2 MB for the app partition.

The final binary size during the successful build was approximately `0x11c6a0` bytes, which fits inside the new `0x200000` factory app partition.

### Flash status

The firmware flashes successfully to the Tab5 using `/dev/ttyACM0`.

### Runtime behavior

Observed runtime sequence from serial log:

```text
I (...) tab5_boot_logo: boot
I (...) display: Initializing MIPI DSI display (ST7123, 720x1280 portrait)
I (...) M5STACK_TAB5: MIPI DSI PHY Powered on
E (...) task_wdt: Task watchdog got triggered
... backtrace in mipi_dsi_hal_host_gen_read_short_packet ...
```

Evidence:

- `/tmp/tab5_boot_screen.log` lines around 547-567
- extracted via `rg -n "tab5_boot_logo: boot|display: Initializing|MIPI DSI PHY Powered on|task_wdt|mipi_dsi_hal_host_gen_read_short_packet" /tmp/tab5_boot_screen.log -n -C 2`

This means the app reaches the beginning of `display_app_init()` and gets far enough to power the DSI PHY, but does not successfully complete panel init.

---

## Current-State Architecture

## A. Factory firmware initialization sequence

The original M5Stack HAL is the most important reference because it shows the **known-good init order**.

From `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp:47-105`:

1. initialize camera oscillator,
2. initialize shared I2C,
3. initialize PI4IOE I/O expanders,
4. enable charging-related controls,
5. scan I2C,
6. initialize codec,
7. initialize IMU,
8. initialize INA226,
9. initialize RTC,
10. only then initialize display,
11. rotate display 90°,
12. turn on backlight.

The most relevant lines are:

- `hal_esp32.cpp:51-59` — camera + I2C + IO expander
- `hal_esp32.cpp:89-105` — display init + rotation + backlight

This is the canonical board-preparation sequence.

### Evidence excerpt

```cpp
mclog::tagInfo(_tag, "i2c init");
bsp_i2c_init();

mclog::tagInfo(_tag, "io expander init");
i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();
bsp_io_expander_pi4ioe_init(i2c_bus_handle);
...
mclog::tagInfo(_tag, "display init");
bsp_reset_tp();
...
lvDisp = bsp_display_start_with_config(&cfg);
lv_display_set_rotation(lvDisp, LV_DISPLAY_ROTATION_90);
bsp_display_backlight_on();
```

This matters because the current tutorial firmware does **not** reproduce the same prerequisite sequence before touching the panel.

## B. Current tutorial firmware initialization sequence

From `esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c:22-34`:

```c
ESP_ERROR_CHECK(display_app_init());
ESP_ERROR_CHECK(echo_state_init());
ESP_ERROR_CHECK(wifi_app_start());
wifi_console_start();
ESP_ERROR_CHECK(http_server_start());
```

This ordering is reasonable for a finished demo: display first, network second. The problem is inside `display_app_init()`.

## C. Current `display_app_init()` behavior

The current display path is implemented in:

- `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c:36-185`

It currently does the following:

1. initialize display brightness PWM,
2. power the DSI PHY,
3. create DSI bus,
4. create DSI DBI panel IO,
5. create ST7123 panel handle,
6. reset/init/enable panel,
7. initialize LVGL port,
8. register display with LVGL,
9. rotate display,
10. create the logo image.

### Critical omission

It does **not** currently do any of the following before panel init:

- `bsp_i2c_init()`
- `bsp_i2c_get_handle()`
- `bsp_io_expander_pi4ioe_init(...)`

That is the central architectural gap.

## D. Board-preparation code that exists but is not used

The copied BSP already contains the missing building blocks.

### Shared I2C init

From `m5stack_tab5.c:100-127`:

- `bsp_i2c_init()` sets up the board I2C master bus.
- `bsp_i2c_get_handle()` returns the initialized handle.

### IO expander init

From `m5stack_tab5.c:245-323`:

- `bsp_io_expander_pi4ioe_init(i2c_master_bus_handle_t bus_handle)` configures two PI4IOE expanders.
- PI4IOE1 sets outputs including `SPK_EN`, `EXT5V_EN`, `LCD_RST`, `TP_RST`, and camera reset.
- PI4IOE2 sets outputs including `WLAN_PWR_EN`, `USB5V_EN`, and charge enable.

The explicit comment at `m5stack_tab5.c:279-282` is especially important:

```c
/* Output Port Register P1(SPK_EN), P2(EXT5V_EN), P4(LCD_RST), P5(TP_RST), P6(CAM)RST 输出高电平 */
```

That means the expander setup is directly responsible for asserting board-level lines required by the display subsystem.

### Backlight and DSI PHY are only part of the story

From `m5stack_tab5.c:1040-1117`:

- `bsp_display_brightness_init()` configures LEDC PWM for backlight control.
- `bsp_enable_dsi_phy_power()` powers the DSI PHY LDO.

Those are necessary, but they are not substitutes for the IO-expander board prep.

---

## Failure Analysis

## Observed failure point

The watchdog log shows the app stuck in the DSI read path:

```text
mipi_dsi_host_ll_gen_is_read_fifo_empty
mipi_dsi_hal_host_gen_read_short_packet
```

That strongly suggests the code is blocked waiting for a panel response that never arrives.

## Where that read happens in the ST7123 driver

The copied ST7123 driver performs an ID read very early during panel init.

From `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/esp_lcd_st7123.c:188-241`:

```c
uint8_t ID[3];
ESP_RETURN_ON_ERROR(esp_lcd_panel_io_rx_param(io, 0x04, ID, 3), TAG, "read ID failed");
ESP_LOGI(TAG, "LCD ID: %02X %02X %02X", ID[0], ID[1], ID[2]);
```

This happens before the vendor command sequence is sent.

### Why this is important

If the display is not powered/reset/enabled correctly yet, the ID read will not complete successfully. That matches the runtime symptom exactly: a stall in the low-level DSI read path before display init completes.

## Why the current custom path is likely incomplete

The tutorial firmware currently assumes this sequence is sufficient:

```text
backlight PWM -> DSI PHY power -> DSI bus -> ST7123 panel -> panel init
```

But the factory firmware uses this broader sequence:

```text
camera osc
-> I2C init
-> IO expander init
-> board enables / resets asserted
-> other shared-bus peripherals initialized
-> display start wrapper
-> LVGL rotation
-> backlight on
```

That difference matters because the Tab5 display is not just a self-contained panel on a raw DSI link. It depends on board-level controls managed by the IO expanders.

## Why the copied BSP itself is not enough

The presence of `m5stack_tab5.c` inside the tutorial project is not automatically enough. The current code bypasses the higher-level BSP wrapper and only calls a subset of the BSP functions. The current `display_app.c` uses:

- `bsp_display_brightness_init()`
- `bsp_enable_dsi_phy_power()`
- low-level `esp_lcd_*` setup

It does **not** use:

- `bsp_i2c_init()`
- `bsp_io_expander_pi4ioe_init()`
- `bsp_display_start_with_config()`

So the project contains the right ingredients, but the runtime path still omits critical preparation.

---

## Root-Cause Hypothesis

## Primary hypothesis

**Most likely root cause:** the custom firmware attempts ST7123 panel communication before the board-level display reset/power rails have been established through the Tab5 I2C + IO-expander sequence.

### Why this hypothesis fits the evidence

1. The original HAL initializes I2C and the IO expander before display startup.
2. The IO expander explicitly drives `LCD_RST` and `EXT5V_EN` high.
3. The custom firmware does not perform this step.
4. The stall occurs at panel ID read time, which is exactly when a not-yet-ready panel would fail to respond.
5. The firmware gets past DSI PHY power, so the problem is probably not “the app never entered display code”; it is more likely “display bus exists, panel not actually ready.”

## Secondary possibilities

These are lower-confidence possibilities, but worth tracking:

1. **The manual low-level path differs subtly from the BSP wrapper path.**
   - Example: exact config structure contents, timing, or expected rotation/color assumptions.
2. **The ST7123 driver’s early ID read may be too strict for this boot stage.**
   - If the panel requires a reset/power settle delay before `0x04` read, the early read could block.
3. **The copied BSP modifications may have changed behavior.**
   - Example: local edits to expose functions or adjust build wiring.
4. **Not all board-prep dependencies are known yet.**
   - The original HAL also initializes codec/IMU/INA226/RTC before display. Those may not be required, but we should not assume that without proving it.

## Things that are probably *not* the main root cause

Based on current evidence, these are less likely to be the primary problem:

- **Flash size / partition size** — already fixed; binary now fits in a 2 MB app partition.
- **Asset linkage** — logo symbol and embedded JS are now linked correctly.
- **Wi-Fi stack** — the app fails before it reaches Wi-Fi bring-up.
- **LVGL object creation** — the stall happens before LVGL display registration finishes.

---

## Architectural Comparison: Current vs Recommended Path

## Current path (problematic)

```text
app_main()
  -> display_app_init()
       -> bsp_display_brightness_init()
       -> bsp_enable_dsi_phy_power()
       -> esp_lcd_new_dsi_bus()
       -> esp_lcd_new_panel_io_dbi()
       -> esp_lcd_new_panel_st7123()
       -> esp_lcd_panel_init()
            -> esp_lcd_panel_io_rx_param(io, 0x04, ID, 3)   [hang likely here]
       -> lvgl_port_init()
       -> lvgl_port_add_disp_dsi()
       -> logo object creation
```

## Factory/reference path

```text
app_main()
  -> app::Init()
      -> hal::Inject(HalEsp32)
      -> HalEsp32::init()
           -> bsp_cam_osc_init()
           -> bsp_i2c_init()
           -> bsp_io_expander_pi4ioe_init(bsp_i2c_get_handle())
           -> codec / IMU / INA226 / RTC init
           -> bsp_display_start_with_config(&cfg)
                -> lvgl_port_init()
                -> bsp_display_brightness_init()
                -> bsp_display_lcd_init()
                     -> bsp_display_new_with_handles_to_st7123()
                     -> lvgl_port_add_disp_dsi()
           -> lv_display_set_rotation(...)
           -> bsp_display_backlight_on()
```

## Recommended next-step path

For the tutorial firmware, the next iteration should probably be:

```text
app_main()
  -> minimal_board_prep_for_display()
       -> bsp_i2c_init()
       -> bsp_io_expander_pi4ioe_init(bsp_i2c_get_handle())
  -> bsp_display_start_with_config(&cfg)
  -> lv_display_set_rotation(...)
  -> bsp_display_backlight_on()
  -> create logo LVGL object
  -> start Wi-Fi / HTTP after display works
```

That is much closer to the known-good path and removes unnecessary custom low-level wiring.

---

## Proposed Solution

## Short version

Do not continue debugging the current hand-rolled `display_app.c` in isolation until the board-prep gap is closed.

### Phase 1: prove board prep matters

Modify `display_app_init()` so that before any DSI bus or panel work, it does at least:

```c
ESP_ERROR_CHECK(bsp_i2c_init());
i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
bsp_io_expander_pi4ioe_init(bus);
```

Then retry the existing display path.

### Phase 2: simplify toward BSP usage

If Phase 1 confirms that board prep was missing, replace the manual low-level display path with the higher-level BSP wrapper:

```c
bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
    .buffer_size   = BSP_LCD_H_RES * BSP_LCD_V_RES,
    .double_buffer = true,
    .flags = {
        .buff_dma    = true or false,
        .buff_spiram = true,
        .sw_rotate   = true,
    },
};

lv_display_t *disp = bsp_display_start_with_config(&cfg);
lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
bsp_display_backlight_on();
```

Then create the logo LVGL object on the active screen.

### Why this is the better direction

- It uses the same board-aware path the factory firmware already relies on.
- It reduces duplicate logic.
- It avoids maintaining a second independent ST7123 bring-up path unless there is a strong reason to do so.
- It keeps the tutorial focused on the boot logo, not on rebuilding the BSP.

---

## Pseudocode for the Repair

## Option A — minimal patch to current custom display path

```c
esp_err_t display_app_init(void) {
    // 1. Board prep first
    ESP_ERROR_CHECK(bsp_i2c_init());
    i2c_master_bus_handle_t i2c = bsp_i2c_get_handle();
    bsp_io_expander_pi4ioe_init(i2c);

    // optional: small delay if hardware settle is needed
    vTaskDelay(pdMS_TO_TICKS(50));

    // 2. Existing custom display init path
    ESP_ERROR_CHECK(bsp_display_brightness_init());
    ESP_ERROR_CHECK(bsp_enable_dsi_phy_power());
    ... current DSI bus / ST7123 / LVGL path ...
}
```

## Option B — preferred patch using the BSP display wrapper

```c
esp_err_t display_app_init(void) {
    // 1. Board prep
    ESP_ERROR_CHECK(bsp_i2c_init());
    i2c_master_bus_handle_t i2c = bsp_i2c_get_handle();
    bsp_io_expander_pi4ioe_init(i2c);

    // 2. Display wrapper
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size   = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = true,
        .flags = {
            .buff_spiram = true,
            .sw_rotate   = true,
        },
    };

    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    assert(disp != NULL);

    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    ESP_ERROR_CHECK(bsp_display_backlight_on());

    // 3. Create logo widget
    lv_obj_t *img = lv_img_create(lv_screen_active());
    lv_img_set_src(img, &logo_tab);
    lv_obj_center(img);

    return ESP_OK;
}
```

---

## Why the Original Firmware Build Matters

As part of this investigation, the original reference firmware under `M5Tab5-UserDemo/platforms/tab5` was also built successfully.

### Why this matters

It tells us:

1. the local factory checkout is buildable,
2. its component graph is intact,
3. its display path is at least build-complete in this environment,
4. and the problem is therefore much more likely to be in our tutorial adaptation than in the local SDK/toolchain.

### Observed original build result

The build completed successfully and reported:

- `m5stack_tab5.bin binary size 0x57b700 bytes`
- smallest app partition `0xa00000 bytes`
- about 45% free in that partition

This was useful as a sanity check. The original firmware was **not flashed over the current repro firmware** during this pass because preserving the failing boot-logo image was more useful for continued debugging.

---

## Test and Validation Strategy

## Immediate validation targets

### Test 1 — prove board prep changes the failure mode

After adding `bsp_i2c_init()` + `bsp_io_expander_pi4ioe_init()`:

- flash firmware,
- monitor serial logs,
- confirm the watchdog no longer fires in `mipi_dsi_hal_host_gen_read_short_packet`.

Success criteria:

- either the panel ID is logged,
- or the code progresses beyond panel init and fails later in a different place.

A changed failure point is useful progress.

### Test 2 — BSP wrapper path

After switching to `bsp_display_start_with_config()`:

- flash firmware,
- verify that LVGL display registration completes,
- verify the screen lights up,
- verify the logo appears.

### Test 3 — network coexistence

Only after the display works:

- verify `app_main()` continues past `display_app_init()`,
- verify Wi-Fi SoftAP comes up,
- verify HTTP health route responds,
- verify the logo remains visible while network tasks run.

## Commands used in this investigation

```bash
# Build tutorial firmware
cd esp32-s3-m5/0051-tab5-boot-logo
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build

# Flash tutorial firmware
idf.py -p /dev/ttyACM0 flash

# Build original reference firmware
cd M5Tab5-UserDemo/platforms/tab5
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build
```

---

## Risks, Alternatives, and Open Questions

## Risks

1. **The display may need more than I2C + IO-expander prep.**
   - Codec or other shared-bus initialization could indirectly matter.
2. **The copied local BSP may diverge from the factory firmware over time.**
   - Keeping a copied component creates maintenance cost.
3. **The current 2 MB partition is a practical workaround, not yet a polished product choice.**
   - Fine for development, but should be reviewed intentionally later.

## Alternatives considered

### Alternative 1 — keep debugging the fully manual path

This is possible, but it is the least efficient path for a tutorial demo.

Why not preferred:

- duplicates BSP logic,
- increases surface area for board-specific mistakes,
- gives the intern more display-driver complexity than necessary.

### Alternative 2 — flash and compare the original firmware on hardware immediately

This would provide a known-good runtime baseline, but it would also overwrite the current failing tutorial image. That is useful later, but not strictly necessary to explain the present failure.

### Alternative 3 — stub out the ST7123 ID read in the driver

This might make the panel init progress, but it would be masking the likely underlying board-prep issue. It is not the right first fix.

## Open questions

1. Is I2C + IO-expander prep alone sufficient, or does the display path also require additional delays or reset sequencing?
2. Is `bsp_reset_tp()` needed before display startup in the tutorial path, as it is in the factory HAL?
3. Should the tutorial firmware keep a local copy of `m5stack_tab5`, or should it instead depend on a cleaner extracted subset/component in the future?

---

## Intern-Focused Summary

If you are new to this board, the key lesson is this:

> On the Tab5, the display is not “just a screen on a DSI bus.” It is a board subsystem with power, reset, and control signals managed outside the panel driver itself.

The current firmware already proves several useful things:

- the project structure is viable,
- the assets are wired correctly,
- the BSP copy compiles,
- the firmware flashes,
- and the failure is reproducible.

That is valuable progress.

But the missing piece is that the code currently jumps too quickly into low-level panel communication without first reproducing the original board initialization sequence that makes the display physically ready.

If you remember only one rule from this document, remember this one:

> When porting firmware from a board vendor’s working HAL to a smaller tutorial app, preserve the **initialization order** before you preserve the **API calls**.

That is the main engineering takeaway from this bug.

---

## References

### Primary evidence files

- `M5Tab5-UserDemo/platforms/tab5/main/app_main.cpp:13-29` — factory entrypoint
- `M5Tab5-UserDemo/platforms/tab5/main/hal/hal_esp32.cpp:47-105` — factory init order
- `M5Tab5-UserDemo/platforms/tab5/components/m5stack_tab5/include/bsp/display.h:44-60` — panel geometry and DSI power constants
- `esp32-s3-m5/0051-tab5-boot-logo/main/app_main.c:22-34` — tutorial boot order
- `esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c:36-185` — current failing display path
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:100-127` — `bsp_i2c_init()` and `bsp_i2c_get_handle()`
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:245-323` — IO expander initialization and asserted outputs
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:1040-1117` — backlight + DSI PHY power
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:1361-1448` — BSP ST7123 display wrapper
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/m5stack_tab5.c:1497-1535` — BSP LVGL display registration
- `esp32-s3-m5/0051-tab5-boot-logo/components/m5stack_tab5/esp_lcd_st7123.c:188-241` — ST7123 panel init and ID read
- `/tmp/tab5_boot_screen.log` — captured runtime serial evidence from the failing firmware image

### Useful APIs to understand

- `bsp_i2c_init()`
- `bsp_i2c_get_handle()`
- `bsp_io_expander_pi4ioe_init()`
- `bsp_display_start_with_config()`
- `bsp_display_backlight_on()`
- `esp_lcd_new_dsi_bus()`
- `esp_lcd_new_panel_io_dbi()`
- `esp_lcd_new_panel_st7123()`
- `esp_lcd_panel_io_rx_param()`
- `lvgl_port_init()`
- `lvgl_port_add_disp_dsi()`
