---
Title: Diary
Ticket: 001-MAKE-GFX-EXAMPLE-WORK
Status: active
Topics:
    - esp32s3
    - esp-idf
    - display
    - gc9107
    - m5gfx
    - atoms3r
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Track a **detailed, step-by-step investigation diary** for why our ESP-IDF `esp_lcd` GC9107 example (`esp32-s3-m5/0008-atoms3r-display-animation`) can initialize successfully but still show a **black screen** on real AtomS3R hardware, while the PlatformIO UserDemo still works.

## Context

- **Known-good reference**: `M5AtomS3-UserDemo` (PlatformIO + Arduino + M5Unified/M5GFX). Manuel flashes it via: `pio run -e ATOMS3 -t upload`.
- **Our target**: AtomS3R, using pure ESP-IDF `esp_lcd` + managed component `espressif__esp_lcd_gc9107`.
- **Observed failures**:
  - Initially: “moving grid” artifacts while panel init/backlight logs looked OK.
  - After fixes (gap=32, RGB565 byteswap): panel remains **black** (no visible content).

## Quick Reference

### On-device log (provided by Manuel)

```text
I (198) app_init: ESP-IDF:          v5.4.1

I (270) main_task: Calling app_main()
I (270) atoms3r_anim: boot; free_heap=374308 dma_free=366804
I (270) atoms3r_anim: bringing up lcd...

I (280) atoms3r_anim: backlight mode: I2C (init->brightness=0)
I (280) atoms3r_anim: backlight i2c init: port=0 scl=0 sda=45 addr=0x30 reg=0x0e
I (290) atoms3r_anim: backlight i2c set: reg=0x0e value=0

I (290) atoms3r_anim: spi bus init: host=2 sclk=15 mosi=21 cs=14 dc=42 rst=48 pclk=40000000Hz
I (310) atoms3r_anim: panel create: gc9107 bits_per_pixel=16 rgb_order=BGR
I (320) gc9107: LCD panel create success, version: 2.0.0

I (330) atoms3r_anim: panel reset/init...
I (810) atoms3r_anim: panel gap: x=0 y=32
I (810) atoms3r_anim: panel invert: 0
I (810) atoms3r_anim: panel display on

I (810) atoms3r_anim: backlight enable: I2C brightness=255
I (810) atoms3r_anim: backlight i2c set: reg=0x0e value=255

I (820) atoms3r_anim: lcd init ok (128x128), pclk=40000000Hz, gap=(0,32), colorspace=BGR
I (820) atoms3r_anim: rgb565 byteswap: 1 (SPI LCDs are typically big-endian)
I (830) atoms3r_anim: framebuffer ok: 32768 bytes; free_heap=337908 dma_free=330404
```

## Usage Examples

### Fast check: confirm we’re running the intended config

- Ensure the on-device log contains:
  - `panel gap: x=0 y=32`
  - `rgb565 byteswap: 1`
  - `gc9107 init profile: ...` (added as a new log in later steps)

### Known-good “ground truth” to compare against

- Flash UserDemo (Manuel):
  - `pio run -e ATOMS3 -t upload`

## Related

- Analysis doc in this ticket: `analysis/01-atoms3r-gc9107-black-screen-retracing-steps-vs-m5gfx-and-esp-lcd.md`

## Step 1: Backlight gate GPIO + M5GFX init enabled; backlight returns; still no visible animation

This step captures the first “we’re getting closer” milestone: after adding a **backlight gate GPIO (GPIO7 active-low)** and switching the GC9107 init sequence to **M5GFX-style vendor init commands**, the **backlight turns on again**. The render loop is alive (heartbeat logs), but the user still reports a black screen (no visible animation).

### Observed log (provided by Manuel)

```text
I (270) atoms3r_anim: boot; free_heap=374228 dma_free=366724
I (270) atoms3r_anim: bringing up lcd...
I (280) atoms3r_anim: backlight mode: I2C (init->brightness=0)
I (290) atoms3r_anim: backlight gate gpio: pin=7 active_low=1 -> OFF (level=1)
I (300) atoms3r_anim: backlight i2c init: port=0 scl=0 sda=45 addr=0x30 reg=0x0e
I (300) atoms3r_anim: backlight i2c set: reg=0x0e value=0
I (310) atoms3r_anim: spi bus init: host=2 sclk=15 mosi=21 cs=14 dc=42 rst=48 pclk=40000000Hz
I (320) atoms3r_anim: gc9107 init profile: M5GFX (vendor init cmds)
I (330) atoms3r_anim: panel create: gc9107 bits_per_pixel=16 rgb_order=BGR
I (350) gc9107: LCD panel create success, version: 2.0.0
I (350) atoms3r_anim: panel reset/init...
I (690) atoms3r_anim: panel gap: x=0 y=32
I (690) atoms3r_anim: panel invert: 0
I (690) atoms3r_anim: panel display on
I (690) atoms3r_anim: backlight enable: I2C brightness=255
I (700) atoms3r_anim: backlight gate gpio: pin=7 active_low=1 -> ON (level=0)
I (710) atoms3r_anim: backlight i2c set: reg=0x0e value=255
I (710) atoms3r_anim: lcd init ok (128x128), pclk=40000000Hz, gap=(0,32), colorspace=BGR
I (720) atoms3r_anim: rgb565 byteswap: 1 (SPI LCDs are typically big-endian)
I (730) atoms3r_anim: framebuffer ok: 32768 bytes; free_heap=337828 dma_free=330324
I (1020) atoms3r_anim: heartbeat: frame=10 dt=280ms free_heap=337828 dma_free=330324
I (1320) atoms3r_anim: heartbeat: frame=20 dt=300ms free_heap=337828 dma_free=330324
```

### Expected behavior (what we *should* see)
- A **full-screen 128×128 animated color gradient** (a “plasma-ish” shifting gradient). It should be immediately visible and continuously changing (not static).

### What we learned
- Backlight likely needs **GPIO7 gating** on this hardware (or at least it doesn’t hurt and makes the backlight reliable again).
- The panel can still remain black even with correct `gap=(0,32)` and M5GFX vendor init, which suggests the remaining issue is likely on the **SPI transaction semantics** (DC/CS polarity, command/data separation, or SPI electrical/timing), or inversion/color-order requirements.

## Step 2: Create tutorial 0009 (M5GFX-based) to mirror tutorial 0008 animation on AtomS3R

This step creates a new ESP-IDF tutorial that reproduces the exact same animated gradient as `0008`, but draws it using **M5GFX / LovyanGFX** instead of `esp_lcd`. The goal is to have a “known-good display stack” baseline inside ESP-IDF, which makes it easier to isolate whether our remaining issues are in the `esp_lcd` stack or elsewhere.

**Commit (esp32-s3-m5):** 2ce5347234d8ef94d263c803a163cc6d3efaaf6e — "Tutorial 0009: AtomS3R display animation via M5GFX"

### What I did
- Added `esp32-s3-m5/0009-atoms3r-m5gfx-display-animation`
  - Uses `EXTRA_COMPONENT_DIRS` to reuse `M5Cardputer-UserDemo/components/M5GFX` as an ESP-IDF component
  - Uses `lgfx::Bus_SPI` + `lgfx::Panel_GC9107` with AtomS3R pinout and `offset_y=32`
  - Adds backlight support (GPIO7 gate + optional I2C 0x30/0x0e), heartbeat logs, and menuconfig knobs

### What worked
- After minor compatibility fixes, `idf.py build` succeeds for tutorial `0009` on ESP-IDF 5.4.1.

### What didn't work (and fix applied)
- M5GFX (vendored) needed an ESP-IDF 5.x compatibility fix for I2C peripheral mapping.

**Commit (esp32-s3-m5):** a9fa0166b2bb61717c116769aaf76b5f0cc67783 — "Tutorial 0009: fix IDF 5.4 C++ config"

### What I learned
- Using M5GFX directly inside ESP-IDF is feasible and gives us a solid baseline to compare against `esp_lcd`.

## Step 3: Boot crash in 0009 — ESP-IDF I2C driver_ng conflicts with M5GFX legacy I2C

This step captures a hard blocker that prevented us from using tutorial `0009` on real hardware: the app **aborts during early startup** (before `app_main`) with an ESP-IDF I2C driver conflict error. The key insight is that this is not “mysterious boot flakiness”: it is a deliberate ESP-IDF safeguard implemented as a **global constructor** inside the legacy I2C driver.

We addressed it by standardizing tutorial `0009`’s **backlight I2C helper** onto the **legacy** I2C APIs, matching what the vendored M5GFX/LovyanGFX codebase still links against. This should eliminate the driver_ng linkage and allow boot to proceed.

### Observed failure (provided by Manuel)

```text
E (257) i2c: CONFLICT! driver_ng is not allowed to be used with this old driver

abort() was called at PC 0x4201d533 on core 0

--- 0x4201d533: check_i2c_driver_conflict at .../components/driver/i2c/i2c.c:1723
--- 0x42002b6b: do_global_ctors at .../components/esp_system/startup.c:104
```

### What I did
- Wrote a full bug report + root-cause analysis: `analysis/02-bug-report--i2c-driver-ng-conflict-m5gfx-0009.md`
- Patched tutorial `0009` backlight I2C to use legacy ESP-IDF I2C (`driver/i2c.h`) instead of `driver/i2c_master.h`
  - Replace `i2c_new_master_bus()` / `i2c_master_bus_add_device()` / `i2c_master_transmit()` with:
    - `i2c_param_config()`
    - `i2c_driver_install()` (tolerate `ESP_ERR_INVALID_STATE` if already installed)
    - `i2c_master_write_to_device()`

### Why
- Vendored M5GFX/LovyanGFX still compiles against the legacy I2C driver and uses legacy symbols like `i2c_set_pin(...)`.
- ESP-IDF 5.x aborts if both legacy + driver_ng are linked, via a constructor (`check_i2c_driver_conflict`).
- Using legacy I2C in our own backlight helper should keep us on one I2C driver until we either migrate M5GFX or explicitly choose a different strategy.

### What worked
- Root cause was confirmed by reading ESP-IDF 5.4.1 source: `components/driver/i2c/i2c.c` contains `__attribute__((constructor)) check_i2c_driver_conflict()` which aborts if driver_ng is present.

### Build verification (local)

Command (note the required ESP-IDF env):

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation && \
set -o pipefail && idf.py build 2>&1 | tail -n 80
```

Result: **build succeeds** after the backlight I2C helper was switched back to legacy I2C APIs.

```text
Checking "python3" ...
Python 3.11.3
"python3" has been detected
Activating ESP-IDF 5.4
Setting IDF_PATH to '/home/manuel/esp/esp-idf-5.4.1'.
* Checking python version ... 3.11.3
* Checking python dependencies ... OK
* Deactivating the current ESP-IDF environment (if any) ... OK
* Establishing a new ESP-IDF environment ... OK
* Identifying shell ... zsh
* Detecting outdated tools in system ... Found tools that are not used by active ESP-IDF version.
For removing old versions of xtensa-esp32s3-elf, riscv32-esp-elf-gdb, 
openocd-esp32, xtensa-esp32s2-elf, xtensa-esp32-elf, riscv32-esp-elf, 
xtensa-esp-elf-gdb, esp32ulp-elf, esp-rom-elfs, esp-clang use command 'python 
/home/manuel/esp/esp-idf-5.4.1/tools/idf_tools.py uninstall'
To free up even more space, remove installation packages of those tools.
Use option python /home/manuel/esp/esp-idf-5.4.1/tools/idf_tools.py uninstall 
--remove-archives.
* Shell completion ... Autocompletion code generated

Done! You can now compile ESP-IDF projects.
Go to the project directory and run:

  idf.py build
Executing action: all (aliases: build)
Running ninja in directory /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build
Executing "ninja all"...
[0/1] Re-running CMake...
-- Building ESP-IDF components for target esp32s3
NOTICE: Processing 1 dependencies:
NOTICE: [1/1] idf (5.4.1)
-- M5GFX use components = nvs_flash;efuse;driver;esp_timer
-- Project sdkconfig file /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/sdkconfig
Loading defaults file /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/sdkconfig.defaults...
-- Compiler supported targets: xtensa-esp-elf
-- App "atoms3r_m5gfx_display_animation" version: a9fa016-dirty
-- Adding linker script /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/esp-idf/esp_system/ld/memory.ld
-- Adding linker script /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/esp-idf/esp_system/ld/sections.ld.in
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/esp_rom/esp32s3/ld/esp32s3.rom.ld
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/esp_rom/esp32s3/ld/esp32s3.rom.api.ld
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/esp_rom/esp32s3/ld/esp32s3.rom.bt_funcs.ld
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/esp_rom/esp32s3/ld/esp32s3.rom.libgcc.ld
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/esp_rom/esp32s3/ld/esp32s3.rom.wdt.ld
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/esp_rom/esp32s3/ld/esp32s3.rom.version.ld
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/esp_rom/esp32s3/ld/esp32s3.rom.newlib.ld
-- Adding linker script /home/manuel/esp/esp-idf-5.4.1/components/soc/esp32s3/ld/esp32s3.peripherals.ld
-- M5GFX use components = nvs_flash;efuse;driver;esp_timer
-- Components: M5GFX app_trace app_update bootloader bootloader_support bt cmock console cxx driver efuse esp-tls esp_adc esp_app_format esp_bootloader_format esp_coex esp_common esp_driver_ana_cmpr esp_driver_cam esp_driver_dac esp_driver_gpio esp_driver_gptimer esp_driver_i2c esp_driver_i2s esp_driver_isp esp_driver_jpeg esp_driver_ledc esp_driver_mcpwm esp_driver_parlio esp_driver_pcnt esp_driver_ppa esp_driver_rmt esp_driver_sdio esp_driver_sdm esp_driver_sdmmc esp_driver_sdspi esp_driver_spi esp_driver_touch_sens esp_driver_tsens esp_driver_uart esp_driver_usb_serial_jtag esp_eth esp_event esp_gdbstub esp_hid esp_http_client esp_http_server esp_https_ota esp_https_server esp_hw_support esp_lcd esp_local_ctrl esp_mm esp_netif esp_netif_stack esp_partition esp_phy esp_pm esp_psram esp_ringbuf esp_rom esp_security esp_system esp_timer esp_vfs_console esp_wifi espcoredump esptool_py fatfs freertos hal heap http_parser idf_test ieee802154 json log lwip main mbedtls mqtt newlib nvs_flash nvs_sec_provider openthread partition_table perfmon protobuf-c protocomm pthread rt sdmmc soc spi_flash spiffs tcp_transport touch_element ulp unity usb vfs wear_levelling wifi_provisioning wpa_supplicant xtensa
-- Configuring done (4.2s)
-- Generating done (0.6s)
-- Build files have been written to: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build
[1/14] Building C object esp-idf/esp_app_format/CMakeFiles/__idf_esp_app_format.dir/esp_app_desc.c.obj
[2/14] Linking C static library esp-idf/esp_app_format/libesp_app_format.a
[3/14] Generating ../../partition_table/partition-table.bin
Partition table binary generated. Contents:
*******************************************************************************
# ESP-IDF Partition Table
# Name, Type, SubType, Offset, Size, Flags
nvs,data,nvs,0x9000,20K,
phy_init,data,phy,0xf000,4K,
factory,app,factory,0x10000,4M,
storage,data,fat,0x410000,1M,
*******************************************************************************
[4/14] Performing build step for 'bootloader'
[1/1] cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/bootloader/esp-idf/esptool_py && /home/manuel/.espressif/python_env/idf5.4_py3.11_env/bin/python /home/manuel/esp/esp-idf-5.4.1/components/partition_table/check_sizes.py --offset 0x8000 bootloader 0x0 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/bootloader/bootloader.bin
Bootloader binary size 0x5220 bytes. 0x2de0 bytes (36%) free.
[5/14] No install step for 'bootloader'
[6/14] Completed 'bootloader'
[7/14] Building CXX object esp-idf/M5GFX/CMakeFiles/__idf_M5GFX.dir/src/lgfx/v1/platforms/esp32/common.cpp.obj
[8/14] Building CXX object esp-idf/main/CMakeFiles/__idf_main.dir/hello_world_main.cpp.obj
[9/14] Linking C static library esp-idf/M5GFX/libM5GFX.a
[10/14] Linking C static library esp-idf/main/libmain.a
[11/14] Generating ld/sections.ld
[12/14] Linking CXX executable atoms3r_m5gfx_display_animation.elf
[13/14] Generating binary image from built executable
esptool.py v4.10.0
Creating esp32s3 image...
Merged 2 ELF sections
Successfully created esp32s3 image.
Generated /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/atoms3r_m5gfx_display_animation.bin
[14/14] cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/esp-idf/esptool_py && /home/manuel/.espressif/python_env/idf5.4_py3.11_env/bin/python /home/manuel/esp/esp-idf-5.4.1/components/partition_table/check_sizes.py --offset 0x8000 partition --type app /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/partition_table/partition-table.bin /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build/atoms3r_m5gfx_display_animation.bin
atoms3r_m5gfx_display_animation.bin binary size 0x508c0 bytes. Smallest app partition is 0x400000 bytes. 0x3af740 bytes (92%) free.

Project build complete. To flash, run:
 idf.py flash
or
 idf.py -p PORT flash
or
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/atoms3r_m5gfx_display_animation.bin
or from the "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/build" directory
 python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash "@flash_args"
```

### What didn't work
- Using `driver/i2c_master.h` (new driver) in tutorial `0009` while linking M5GFX triggers an unavoidable abort during global constructors.

### What I learned
- In ESP-IDF 5.x, “driver conflicts” can manifest as **pre-`app_main`** aborts because the guard runs in global constructors.
- When embedding Arduino-ecosystem libraries inside ESP-IDF, you must verify which peripheral driver generation they link against; mixing old/new drivers is a common footgun.

### What was tricky to build
- The failure happens before `app_main()`, so normal application logging isn’t available; you must reason from boot logs/backtrace + IDF source.
- It’s easy to miss that even if your own app code only calls new APIs, a dependency can pull in legacy driver symbols and trigger the conflict check.

### What warrants a second pair of eyes
- Validate that the patched legacy-I2C backlight helper doesn’t accidentally reintroduce driver_ng linkage (e.g., via other code paths).
- Confirm no other component in tutorial `0009` (or linked dependencies) pulls in `driver/i2c_master.h`.

### What should be done in the future
- Decide and document a stable policy:
  - either keep `0009` on legacy I2C while M5GFX is vendored as-is,
  - or migrate vendored M5GFX I2C to driver_ng,
  - or (only for experiments) toggle `CONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK` with clear warnings about undefined behavior.

### Code review instructions
- Start at `esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/main/hello_world_main.cpp` (backlight I2C helper).
- Skim `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/common.cpp` for legacy I2C usage (`driver/i2c.h`, `i2c_set_pin`).
- Read `analysis/02-bug-report--i2c-driver-ng-conflict-m5gfx-0009.md` for the full RCA.

### Technical details
- ESP-IDF 5.4.1 legacy driver constructor:
  - `components/driver/i2c/i2c.c: check_i2c_driver_conflict()` aborts if `i2c_acquire_bus_handle` (driver_ng) is linked.

## Step 4: 0009 boots but screen is still blank — compare against M5AtomS3-UserDemo M5GFX backlight init

With the early-boot I2C conflict resolved, tutorial `0009` now **boots successfully** on hardware. After mirroring the backlight driver’s chip init sequence from the PlatformIO UserDemo, the user reports the LCD is now “working”, but the image **flickers/fluttering in a regular fast fashion**. This step focuses on distinguishing “backlight/power flicker” from “frame update tearing / SPI transfer artifacts”.

The key discovery is that the PlatformIO `M5AtomS3-UserDemo`’s version of M5GFX contains an AtomS3R-specific backlight driver (`Light_M5StackAtomS3R`) that performs a small **I2C chip init sequence** (register writes) before writing brightness. Our tutorial previously only wrote the brightness register, which may be insufficient to turn the backlight on.

### What I did
- Located AtomS3R GC9107 + backlight reference in PlatformIO-resolved M5GFX:
  - `M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5GFX/src/M5GFX.cpp`
- Compared its AtomS3R config against tutorial `0009`:
  - SPI pins match: MOSI=21, SCLK=15, DC=42, CS=14, RST=48
  - `spi_3wire=true`, `spi_mode=0`, `spi_host=SPI3_HOST`, `offset_y=32` match
  - **Backlight init differs**: UserDemo does extra I2C writes before brightness

### Why
- If the backlight is off, *any* display draw will look like “black screen”.
- The UserDemo is known-good, so we should copy its backlight enable sequence verbatim before pursuing deeper SPI hypotheses.

### What worked
- We identified the exact AtomS3R M5GFX backlight init sequence:
  - I2C bus pins: SDA=45, SCL=0
  - I2C device addr: `0x30` (decimal 48)
  - Init writes: `reg 0x00 := 0x40`, delay, `reg 0x08 := 0x01`, `reg 0x70 := 0x00`
  - Brightness: write `reg 0x0e := <brightness>`

### What didn't work
- Prior tutorial behavior: only writing `reg 0x0e` (brightness) may not enable the backlight driver chip.

### Follow-up: flicker observed after successful bring-up

User report (after backlight chip init was added): the display content is visible but **flickers**.

Next diagnostic added to `0009`:
- A **solid-color sequence** (red→green→blue→white→black) before starting the animation loop.
- This allows a quick A/B:
  - If flicker occurs during solid colors: likely backlight/power or panel init instability
  - If solid colors are stable but animation flickers: likely update/transfer pacing or tearing

### Update: flicker only happens during animation (solid colors stable)

User report: solid colors are stable; “flutter” happens at roughly the **animation frame rate**.

**Hypothesis**: our full-frame update loop may be starting a new SPI write while the previous one is still in flight (the M5GFX `endWrite()` path does not necessarily call `_bus->wait()`).

**Commit (esp32-s3-m5):** 9345c0860035928192315d1770a01de83b5e1358 — "0009: AtomS3R backlight init + DMA-synced animation"

Mitigation applied in `0009`:
- Allocate the framebuffer in **DMA-capable memory** (`MALLOC_CAP_DMA`)
- Use `pushImageDMA(...)` and explicitly call `waitDMA()` after each frame push

Expected result: animated frames should stop “fluttering” (or at least change character significantly) if the root cause was transfer overlap / lack of flush.

### What I learned
- On AtomS3R, the backlight path appears to be controlled by an I2C device that needs “bring-up” writes, not just a brightness value.

### What was tricky to build
- The “known-good” M5GFX code for AtomS3R lives in PlatformIO’s `.pio/libdeps/...` resolution, not necessarily the same vendored copy we reuse elsewhere.

### What warrants a second pair of eyes
- Confirm the I2C chip init sequence is correct for AtomS3R (and not specific to a subset of units).
- Validate that our optional GPIO7 “gate” does not inadvertently disable backlight on some revisions (M5GFX does not toggle it in the AtomS3R path).

### What should be done in the future
- If this I2C init sequence is required, codify it as an explicit “AtomS3R backlight init” helper (and document it as a hardware contract), rather than relying on implicit “brightness write only” behavior.

### Code review instructions
- Start at:
  - `M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5GFX/src/M5GFX.cpp` (search `board_M5AtomS3R` and `Light_M5StackAtomS3R`)
  - `esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/main/hello_world_main.cpp` (backlight I2C init)


