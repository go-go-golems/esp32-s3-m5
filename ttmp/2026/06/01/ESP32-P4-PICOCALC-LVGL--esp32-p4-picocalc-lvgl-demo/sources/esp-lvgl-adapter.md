---
Title: Source - esp-lvgl-adapter
Ticket: ESP32-P4-PICOCALC-LVGL
Status: active
Topics:
    - esp32-p4
    - picocalc
    - lvgl
DocType: source
Intent: reference
Summary: "Downloaded reference material for LVGL investigation"
---

## ESP LVGL ADAPTER

[\[中文\]](https://docs.espressif.com/projects/esp-iot-solution/zh_CN/latest/display/tools/esp_lvgl_adapter.html)

`esp_lvgl_adapter` provides a runtime-managed LVGL integration for ESP-IDF projects. It unifies display registration, tearing control, thread-safe LVGL access, and optional integration with filesystem, image decoding, and FreeType fonts. It supports LVGL v8 and v9.

## Features

- **Unified display registration**: Supports MIPI DSI / RGB / QSPI / SPI / I2C / I80 interfaces
- **Tearing control**: Multiple tearing modes
- **Thread safety**: Lock/unlock APIs for safe multi-threaded LVGL access
- **Optional modules** (via Kconfig):
	- Filesystem bridge (`esp_lv_fs`)
		- Image decoder (`esp_lv_decoder`)
		- FreeType vector font loader
		- FPS statistics, Dummy Draw (headless rendering)
- **Multi-display support**: Manage multiple displays simultaneously

## Add to Project

### Via Component Service

This component is available from Espressif’s component service. Add it to your project with:

```bash
idf.py add-dependency "espressif/esp_lvgl_adapter"
```

### In idf\_component.yml

Alternatively, declare it in your component manifest:

```yaml
dependencies:
  espressif/esp_lvgl_adapter: "*"
```

### Requirements

- **ESP-IDF**: >= 5.5
- **LVGL**: 8.x or 9.x (auto-detected)
- **Optional dependencies**: Required only if corresponding Kconfig options are enabled

## Quick Start

Here’s a minimal example showing how to initialize the adapter and register a display:

```c
#include "esp_lv_adapter.h"  // Includes display & input adapters

void app_main(void)
{
    // Step 0: Create your esp_lcd panel and (optionally) panel_io with esp_lcd APIs
    esp_lcd_panel_handle_t panel = /* ... */;
    esp_lcd_panel_io_handle_t panel_io = /* ... or NULL */;

    // Step 1: Initialize the adapter
    esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(esp_lv_adapter_init(&cfg));

    // Step 2: Register a display (choose macro by interface)
    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
        panel,           // LCD panel handle
        panel_io,        // LCD panel IO handle (can be NULL for some interfaces)
        800,             // Horizontal resolution
        480,             // Vertical resolution
        ESP_LV_ADAPTER_ROTATE_0 // Rotation
    );
    lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    assert(disp != NULL);

    // Step 3: (Optional) Register input device(s)
    // Create touch handle using esp_lcd_touch API (implementation omitted here)
    // esp_lcd_touch_handle_t touch_handle = /* ... */;
    // esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handle);
    // lv_indev_t *touch = esp_lv_adapter_register_touch(&touch_cfg);
    // assert(touch != NULL);

    // Step 4: Start the adapter task
    ESP_ERROR_CHECK(esp_lv_adapter_start());

    // Step 5: Draw with LVGL (guarded by adapter lock for thread safety)
    if (esp_lv_adapter_lock(-1) == ESP_OK) {
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Hello LVGL!");
        lv_obj_center(label);
        esp_lv_adapter_unlock();
    }
}
```

## Usage

This section guides you through integrating the LVGL adapter with ESP-IDF projects.

### Recommended SDKCONFIG Settings

Add these configurations to your project’s `sdkconfig.defaults` for optimal LVGL performance:

#### Core LVGL Configurations

```
# FreeRTOS tick rate (improves timer precision)
CONFIG_FREERTOS_HZ=1000

# LVGL OS integration
CONFIG_LV_OS_FREERTOS=y

# Use standard C library functions
CONFIG_LV_USE_CLIB_MALLOC=y
CONFIG_LV_USE_CLIB_STRING=y
CONFIG_LV_USE_CLIB_SPRINTF=y

# Display refresh rate (in milliseconds, 15ms ≈ 66 FPS)
CONFIG_LV_DEF_REFR_PERIOD=15

# Enable style caching for better performance
CONFIG_LV_OBJ_STYLE_CACHE=y
```

#### Multi-core Optimization (ESP32-S3/P4)

For chips with multiple cores, enable parallel rendering:

```
# Use 2 drawing threads for better performance
CONFIG_LV_DRAW_SW_DRAW_UNIT_CNT=2
```

#### PSRAM Optimization (When Available)

For devices with PSRAM, optimize cache and XIP settings:

**ESP32-S3:**

```
CONFIG_SPIRAM_XIP_FROM_PSRAM=y
CONFIG_ESP32S3_INSTRUCTION_CACHE_32KB=y
CONFIG_ESP32S3_DATA_CACHE_64KB=y
CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y
```

**ESP32-P4:**

```
CONFIG_SPIRAM_XIP_FROM_PSRAM=y
CONFIG_CACHE_L2_CACHE_256KB=y
CONFIG_CACHE_L2_CACHE_LINE_128B=y
```

### Decision Flow

Before starting, make these decisions:

1. **Pick interface type**: Determine your display interface (MIPI DSI / RGB / QSPI / SPI / I2C / I80)
2. **Pick default macro**: Select corresponding `ESP_LV_ADAPTER_DISPLAY_XXX_DEFAULT_CONFIG` macro
3. **Pick appropriate tearing mode** (RGB/MIPI DSI only):
	- Compute required frame buffer count (`num_fbs`)
		- Pass `num_fbs` to esp\_lcd panel config
4. **Tune buffer size**: Adjust `buffer_height` based on memory vs throughput needs

### Display Registration and Tearing

#### Interface Types and Default Macros

Choose the configuration macro based on your display interface:

| Interface Type | Default Config Macro | Default Tearing Mode |
| --- | --- | --- |
| MIPI DSI | `ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(...)` | `ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_MIPI_DSI` |
| RGB | `ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(...)` | `ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_RGB` |
| SPI/I2C/I80/QSPI (with PSRAM) | `ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(...)` | `ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT` (i.e., `NONE`) |
| SPI/I2C/I80/QSPI (without PSRAM) | `ESP_LV_ADAPTER_DISPLAY_SPI_WITHOUT_PSRAM_DEFAULT_CONFIG(...)` | `ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT` (i.e., `NONE`) |

Note

- Only MIPI DSI and RGB support tearing modes
- SPI/I2C/I80/QSPI are collectively called “OTHER” interfaces in the adapter and support only `NONE` mode

#### Computing Frame Buffer Count

For RGB/MIPI DSI, compute required frame buffers early before hardware initialization:

```c
uint8_t num_fbs = esp_lv_adapter_get_required_frame_buffer_count(
    ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_RGB,  // Tearing mode
    ESP_LV_ADAPTER_ROTATE_0                // Rotation
);
// Pass num_fbs to your esp_lcd panel config
```

**Frame Buffer Count Rules**:

- 90°/270° rotation or triple-buffer modes: **3** frame buffers
- Double-buffer modes: **2** frame buffers
- Single-buffer mode: **1** frame buffer

#### Tearing Modes: Selector (RGB/MIPI DSI)

Choose the appropriate tearing mode based on your use case:

| Tearing Mode | Use Case | Frame Buffers | Memory |
| --- | --- | --- | --- |
| `TRIPLE_PARTIAL` | 90°/270° rotation, High-res smooth UI | 3 | High |
| `TRIPLE_FULL` | Full-screen/large-area updates, Plenty of RAM | 3 | High |
| `DOUBLE_FULL` | Large-area updates, Tighter RAM | 2 | Medium |
| `DOUBLE_DIRECT` | Small-area updates, Widget/UI deltas | 2 | Medium |
| `NONE` | Static UI, Ultra-low RAM | 1 | Low |

Warning

- RGB/MIPI DSI with `TEAR_AVOID_MODE_NONE` **forbids rotation** (any non-zero rotation is rejected)
- OTHER (SPI/I2C/I80/QSPI) interfaces support `NONE` only; for rotation, configure panel orientation (swap XY/mirror) during LCD initialization and adjust touch mapping accordingly

#### Memory Estimation

Full-frame buffer size ≈ `horizontal_res × vertical_res × bytes_per_pixel`

**Example** (RGB565 format, 2 bytes/pixel):

- 800×480 resolution: ≈ 750 KB
- Triple buffering: ≈ 2.2 MB
- **Conclusion**: PSRAM is typically required

### Buffer Tuning

#### Default buffer\_height Values

Default `buffer_height` for each interface (see `esp_lv_adapter_display.h`):

| Interface Type | Default `buffer_height` | Description |
| --- | --- | --- |
| MIPI DSI | 50 | Medium stripe height |
| RGB | 50 | Medium stripe height |
| OTHER with PSRAM | `ver_res` (full height) | Full-height partial refresh, better throughput |
| OTHER without PSRAM | 10 | Small stripe to save RAM |

#### Tuning Tips

- **Increase** `buffer_height`:
	- ✅ Improves throughput (fewer flushes, larger stripes)
		- ❌ Increases RAM usage
- **Decrease** `buffer_height`:
	- ✅ Saves RAM
		- ❌ More flushes, may reduce performance

### Thread-Safe LVGL Access

LVGL APIs are **not thread-safe**. Guard all LVGL calls with the adapter’s lock:

```c
if (esp_lv_adapter_lock(-1) == ESP_OK) {
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 100, 50);
    esp_lv_adapter_unlock();
}
```

### Input Devices

#### Touch

The adapter provides a default configuration macro `ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG` to simplify touch device registration:

```c
// Register touch device using default configuration macro
esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handle);
lv_indev_t *touch = esp_lv_adapter_register_touch(&touch_cfg);
assert(touch != NULL);
```

**Parameters**:

- `disp`: Associated LVGL display device
- `touch_handle`: Touch handle created via `esp_lcd_touch` API
- Default scale factors: x = 1.0, y = 1.0

#### Encoder/Knob

Requires Kconfig option `ESP_LV_ADAPTER_ENABLE_KNOB`. See `esp_lv_adapter_input.h` for `esp_lv_adapter_encoder_config_t`.

### Filesystem Bridge (Optional)

If `ESP_LV_ADAPTER_ENABLE_FS` is enabled, mount a memory-mapped asset partition as an LVGL drive. Indexing comes from `esp_mmap_assets`.

```c
#include "esp_lv_fs.h"
#include "esp_mmap_assets.h"

// 1. Create mmap assets handle
mmap_assets_handle_t assets;
const mmap_assets_config_t mmap_cfg = {
    .partition_label = "assets_A",
    .max_files = MMAP_DRIVE_A_FILES,
    .checksum  = MMAP_DRIVE_A_CHECKSUM,
    .flags = { .mmap_enable = true },
};
ESP_ERROR_CHECK(mmap_assets_new(&mmap_cfg, &assets));

// 2. Mount as LVGL filesystem
esp_lv_fs_handle_t fs_handle;
const fs_cfg_t fs_cfg = {
    .fs_letter = 'A',           // Drive letter (A to Z)
    .fs_assets = assets,
    .fs_nums = MMAP_DRIVE_A_FILES,
};
ESP_ERROR_CHECK(esp_lv_adapter_fs_mount(&fs_cfg, &fs_handle));

// 3. Use file paths in LVGL, e.g., "A:my_image.png"
lv_obj_t *img = lv_img_create(lv_scr_act());
lv_img_set_src(img, "A:my_image.png");
```

Depends on [esp\_lv\_fs](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lv_fs.html) and [esp\_mmap\_assets](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_mmap_assets.html) components.

### Image Decoding (Optional)

Enable `ESP_LV_ADAPTER_ENABLE_DECODER` to integrate `esp_lv_decoder` with LVGL. Use LVGL image widgets as usual; load from the mounted drive:

```c
// Use LVGL image widget to load images
lv_obj_t *img = lv_img_create(lv_scr_act());
lv_img_set_src(img, "I:red_png.png");  // I: is the decoder drive letter
```

**Supported Formats**:

- Standard: JPG, PNG, QOI
- Split formats: SJPG, SPNG, SQOI (optimized for embedded systems)
- Hardware acceleration: JPEG, PJPG (if chip supports)

For details, refer to [esp\_lv\_decoder](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lv_decoder.html).

### FreeType Fonts (Optional)

Enable `ESP_LV_ADAPTER_ENABLE_FREETYPE` and ensure LVGL FreeType is enabled. Then initialize fonts from file or memory:

```c
#include "esp_lv_adapter.h" // FreeType APIs under this header

// Load font from file
esp_lv_adapter_ft_font_handle_t fh;
esp_lv_adapter_ft_font_config_t cfg = {
    .name = "F:my_font.ttf",         // Font file path (Note: no slash after colon for mmap assets)
    .size = 30,                      // Font size
    .style = ESP_LV_ADAPTER_FT_FONT_STYLE_NORMAL,
};
ESP_ERROR_CHECK(esp_lv_adapter_ft_font_init(&cfg, &fh));
const lv_font_t *font30 = esp_lv_adapter_ft_font_get(fh);

// Use the font
lv_obj_set_style_text_font(label, font30, 0);
```

**Stack Configuration**:

| LVGL Version | Required Setting | Explanation |
| --- | --- | --- |
| v8 | `CONFIG_ESP_MAIN_TASK_STACK_SIZE=32768` | Font initialization runs on calling thread |
| v9 | `CONFIG_LV_DRAW_THREAD_STACK_SIZE=32768` | Font rendering runs on draw threads |

Note

- LVGL v8: Does not support LVGL virtual filesystem (`lv_fs`), use direct file paths or memory buffers
- LVGL v9: Supports virtual filesystem paths (e.g., `"F:font.ttf"`)

### FPS Statistics and Dummy Draw

#### FPS Statistics

Requires `ESP_LV_ADAPTER_ENABLE_FPS_STATS`.

```c
// Enable FPS statistics
ESP_ERROR_CHECK(esp_lv_adapter_fps_stats_enable(disp, true));

// Get current FPS
uint32_t fps;
if (esp_lv_adapter_get_fps(disp, &fps) == ESP_OK) {
    printf("Current FPS: %lu\n", fps);
}
```

#### Dummy Draw

Dummy Draw mode bypasses LVGL rendering for direct display control:

```c
ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(disp, true));
ESP_ERROR_CHECK(esp_lv_adapter_dummy_draw_blit(disp, 0, 0, 800, 480, framebuffer, true));
ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(disp, false));
```

### Immediate Refresh

Force a synchronous refresh:

```c
esp_lv_adapter_refresh_now(disp);
```

## Configuration (Kconfig)

Configure via `idf.py menuconfig`:

| Option | Description |
| --- | --- |
| `ESP_LV_ADAPTER_ENABLE_FS` | Enable filesystem bridge (`esp_lv_fs`) |
| `ESP_LV_ADAPTER_ENABLE_DECODER` | Enable image decoder (`esp_lv_decoder`) |
| `ESP_LV_ADAPTER_ENABLE_FREETYPE` | Enable FreeType vector font support |
| `ESP_LV_ADAPTER_ENABLE_FPS_STATS` | Enable FPS monitoring APIs |
| `ESP_LV_ADAPTER_ENABLE_BUTTON` | Enable navigation button input support |
| `ESP_LV_ADAPTER_ENABLE_KNOB` | Enable rotary encoder input support |

## Limitations & Notes

### Interface and Tearing Mode Support

| Interface Type | Supported Tearing Modes |
| --- | --- |
| RGB / MIPI DSI | `NONE`, `DOUBLE_FULL`, `TRIPLE_FULL`, `DOUBLE_DIRECT`, `TRIPLE_PARTIAL` |
| OTHER (SPI/I2C/I80/QSPI) | `NONE` only |

### Rotation Limitations

- **RGB/MIPI DSI**:
	- `TEAR_AVOID_MODE_NONE` **forbids rotation** (any non-zero rotation is rejected)
- **OTHER (SPI/I2C/I80/QSPI)**:
	- Adapter does not apply 90°/270° rotation
		- For rotation, configure panel orientation (swap XY/mirror) during LCD initialization
		- Must also adjust touch coordinate mapping

### Buffering and Render Modes

- **Mode Mapping**:
	- `DOUBLE_FULL` / `TRIPLE_FULL` → FULL render mode
		- `DOUBLE_DIRECT` → DIRECT render mode
		- Others (incl. `TRIPLE_PARTIAL`) → PARTIAL render mode
- **LVGL Draw Buffer Count**:
	- `TRIPLE_PARTIAL`: 1 buffer
		- `TRIPLE_FULL` / `DOUBLE_*`: 2 buffers
		- FULL/DIRECT default: 2 buffers
		- Otherwise: 1 buffer
- **OTHER + NONE**:
	- No panel frame buffers requested
		- Adapter sends partial stripes from LVGL draw buffer

### PPA/DMA2D Acceleration

- With PPA acceleration, set `LV_DRAW_SW_DRAW_UNIT_CNT == 1` for best behavior
- Rotation uses PPA when possible; otherwise CPU copy with cache-friendly blocks
- **Effect**:
	- PPA reduces CPU load
		- Usually does not deliver noticeable FPS gains

## Deinitialization

Recommended cleanup order when shutting down UI:

```c
// 1. Unregister input devices
esp_lv_adapter_unregister_touch(touch);
// esp_lv_adapter_unregister_encoder(encoder);
// esp_lv_adapter_unregister_navigation_buttons(buttons);

// 2. Unregister display(s)
esp_lv_adapter_unregister_display(disp);

// 3. Unmount filesystem(s) (if used)
esp_lv_adapter_fs_unmount(fs_handle);
// Release mmap assets
mmap_assets_del(assets);

// 4. Deinitialize adapter
// Note: FreeType fonts are auto-cleaned if enabled
esp_lv_adapter_deinit();
```

## Compatibility

- **ESP-IDF**: >= 5.5
- **LVGL**: >= 8, < 10 (v8/v9 supported)

### ESP-IDF Patches

#### PPA Freeze Fix (ESP32-P4)

If you encounter display freeze when using `ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL` mode with screen rotation enabled on ESP32-P4, you need to apply the following patch:

**Target Version**: ESP-IDF release/v5.5 (commit `62beeae461bd3692c2028f96a93c84f11291e155`)

**Patch File**: `0001-bugfix-lcd-Fixed-PPA-freeze.patch`

**How to Apply**:

In your ESP-IDF repository root directory:

```bash
cd $IDF_PATH
git apply /path/to/esp_lvgl_adapter/0001-bugfix-lcd-Fixed-PPA-freeze.patch
```

**Issue Description**:

- This patch fixes a display freeze issue on ESP32-P4 when using `ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL` mode with rotation enabled, caused by PPA hardware acceleration
- Only affects scenarios that meet ALL of the following conditions:
	- Using ESP32-P4 chip
		- Using `TRIPLE_PARTIAL` tear avoidance mode
		- Screen rotation enabled (90°/270°)
- Not required for other configuration combinations or chips

**Verify Patch Applied**:

Check if `components/esp_driver_ppa/src/ppa_srm.c` contains the following code:

```c
PPA.sr_byte_order.sr_macro_bk_ro_bypass = 1;
```

## Glossary

| Term | Description |
| --- | --- |
| **Panel frame buffers** | Hardware frame buffers owned by the panel driver |
| **LVGL draw buffer** | RAM used by LVGL to render stripes/frames before flush |
| **Partial refresh** | Updating a sub-region (dirty area) of the screen instead of full screen |
| **Direct mode** | LVGL renders directly to the frame buffer (reduced copies; suited to small-area updates) |
| **PPA** | Pixel Processing Accelerator used for hardware-accelerated rotate/scale/memcpy-like operations |
| **Tearing** | Visual artifact when screen updates while being scanned; mitigated by buffering strategies |

## Examples

| Example | Description |
| --- | --- |
| `lvgl_common_demo` | Unified LVGL demo supporting multiple LCD interfaces |
| `lvgl_multi_screen` | Multi-display management |
| `lvgl_decode_image` | Image decoder showcase |
| `lvgl_freetype_font` | FreeType font loading and usage |
| `lvgl_dummy_draw` | Dummy Draw switching |

## Troubleshooting

### FreeType Crashes or Renders Incorrectly

**Solutions**:

- Ensure LVGL FreeType is enabled (`CONFIG_LV_USE_FREETYPE=y`)
- LVGL v8: Set `CONFIG_ESP_MAIN_TASK_STACK_SIZE=32768`
- LVGL v9: Set `CONFIG_LV_DRAW_THREAD_STACK_SIZE=32768`

### Screen Tearing or Flicker

**Possible Causes**:

- Inappropriate tearing mode
- Incorrect frame buffer count
- Improper rotation config

**Solutions**:

1. Choose appropriate tearing mode based on use case
2. Use `esp_lv_adapter_get_required_frame_buffer_count()` to compute correct frame buffer count

### Decoder or FS Not Working

**Possible Causes**:

- Kconfig options not enabled
- Component dependencies missing
- Asset partition not flashed

**Solutions**:

1. Verify corresponding Kconfig options
2. Ensure dependencies like `esp_mmap_assets` are added
3. Verify asset partition is properly packed and flashed

### LVGL Object API Crashes

**Possible Cause**:

- Accessing LVGL APIs without lock in multi-threaded environment

**Solution**:

```c
// All LVGL API calls must be protected by lock
if (esp_lv_adapter_lock(-1) == ESP_OK) {
    // LVGL API calls
    esp_lv_adapter_unlock();
}
```

## API Reference

### Functions

esp\_err\_t esp\_lv\_adapter\_init(const \*config) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv419esp_lv_adapter_initPK23esp_lv_adapter_config_t "Permalink to this definition")  

Initialize the LVGL adapter.

This function initializes the LVGL library and creates necessary resources. Must be called before any other LVGL adapter functions.

Parameters

**config** – **\[in\]** Pointer to LVGL adapter configuration

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter already initialized
- ESP\_ERR\_INVALID\_ARG: Invalid configuration parameter
- ESP\_ERR\_NO\_MEM: Memory allocation failed

esp\_err\_t esp\_lv\_adapter\_start(void) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv420esp_lv_adapter_startv "Permalink to this definition")  

Start the LVGL adapter task.

Creates and starts the LVGL task for handling timer events.

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized
- ESP\_ERR\_NO\_MEM: Task creation failed

esp\_err\_t esp\_lv\_adapter\_deinit(void) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv421esp_lv_adapter_deinitv "Permalink to this definition")  

Deinitialize the LVGL adapter.

Stops the LVGL task and releases all resources.

Returns

- ESP\_OK: Success

bool esp\_lv\_adapter\_is\_initialized(void) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv429esp_lv_adapter_is_initializedv "Permalink to this definition")  

Check if the LVGL adapter is initialized.

Returns

- true: Adapter is initialized
- false: Adapter is not initialized

esp\_err\_t esp\_lv\_adapter\_lock(int32\_t timeout\_ms) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv419esp_lv_adapter_lock7int32_t "Permalink to this definition")  

Acquire the LVGL adapter lock.

Must be called before accessing LVGL objects from user code.

Parameters

**timeout\_ms** – **\[in\]** Timeout in milliseconds, use -1 for infinite wait

Returns

- ESP\_OK: Lock acquired successfully
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized
- ESP\_ERR\_TIMEOUT: Failed to acquire lock within timeout

void esp\_lv\_adapter\_unlock(void) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv421esp_lv_adapter_unlockv "Permalink to this definition")  

Release the LVGL adapter lock.

esp\_err\_t esp\_lv\_adapter\_refresh\_now(lv\_display\_t \*disp) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv426esp_lv_adapter_refresh_nowP12lv_display_t "Permalink to this definition")  

Force LVGL to refresh the display immediately.

This function triggers an immediate refresh of the LVGL display, forcing all dirty areas to be redrawn without waiting for the next timer period. Useful when you need to update the display synchronously.

Parameters

**disp** – **\[in\]** Pointer to LVGL display object (NULL for default display)

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized

esp\_err\_t esp\_lv\_adapter\_pause(int32\_t timeout\_ms) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv420esp_lv_adapter_pause7int32_t "Permalink to this definition")  

Pause LVGL worker.

Stops the LVGL worker task and waits for acknowledgement. This API is intended for advanced use cases requiring custom control over the LVGL rendering loop. The internal LVGL tick timer is also stopped while paused so that tickless light sleep can enter naturally.

Note

You MUST pair this call with esp\_lv\_adapter\_resume()

Note

Do NOT call esp\_lv\_adapter\_sleep\_prepare() while manually paused

Note

Handle timeout errors appropriately

Parameters

**timeout\_ms** – **\[in\]** Timeout in ms, -1 for infinite

Returns

- ESP\_OK: Success
- ESP\_ERR\_TIMEOUT: Timeout waiting for pause acknowledgement
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized

esp\_err\_t esp\_lv\_adapter\_resume(void) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv421esp_lv_adapter_resumev "Permalink to this definition")  

Resume LVGL worker.

Resumes the LVGL worker task that was previously paused and restarts the internal LVGL tick timer. Do not call this if the adapter was paused by esp\_lv\_adapter\_sleep\_prepare().

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized

esp\_err\_t esp\_lv\_adapter\_sleep\_prepare(void) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv428esp_lv_adapter_sleep_preparev "Permalink to this definition")  

Prepare all displays for sleep.

Automatically detaches all LCD panels while preserving LVGL display objects and UI state. This allows you to safely delete LCD hardware handles to release DMA and framebuffer resources.

Note

This function automatically pauses the LVGL worker and waits for all pending flush operations to complete.

Note

After calling this function, you can safely call esp\_lcd\_panel\_del() for each display’s panel handle.

Note

Touch inputs remain registered. If you need to power down touch hardware, suspend it manually before calling this API.

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized or already sleeping

esp\_err\_t esp\_lv\_adapter\_sleep\_recover(lv\_display\_t \*disp, esp\_lcd\_panel\_handle\_t panel, esp\_lcd\_panel\_io\_handle\_t panel\_io) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv428esp_lv_adapter_sleep_recoverP12lv_display_t22esp_lcd_panel_handle_t25esp_lcd_panel_io_handle_t "Permalink to this definition")  

Recover a display from sleep.

Re-binds a new LCD panel to an existing LVGL display, automatically updating all framebuffer references. The UI elements are preserved.

Note

LCD hardware must be initialized before calling this function.

Note

This function can be called multiple times to recover multiple displays. When all displays are recovered, the adapter automatically resumes the LVGL worker.

Note

This function automatically triggers a full screen refresh for the display.

Parameters

- **disp** – **\[in\]** LVGL display handle to rebind
- **panel** – **\[in\]** New LCD panel handle
- **panel\_io** – **\[in\]** New LCD panel IO handle (can be NULL for RGB/MIPI DSI)

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized or not sleeping
- ESP\_ERR\_INVALID\_ARG: Invalid handles
- ESP\_FAIL: Failed to rebind panel

esp\_err\_t esp\_lv\_adapter\_set\_dummy\_draw(lv\_display\_t \*disp, bool enable) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv429esp_lv_adapter_set_dummy_drawP12lv_display_tb "Permalink to this definition")  

Enable or disable dummy draw mode for a display.

In dummy draw mode, LVGL rendering is bypassed and the application takes direct control of the display. Use esp\_lv\_adapter\_dummy\_draw\_blit() to update the screen.

Note

Manages locking internally. Disabling triggers a full screen refresh.

Parameters

- **disp** – **\[in\]** Pointer to LVGL display object
- **enable** – **\[in\]** true to enable dummy draw, false to disable

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized
- ESP\_ERR\_INVALID\_ARG: Invalid display pointer

bool esp\_lv\_adapter\_get\_dummy\_draw\_enabled(lv\_display\_t \*disp) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv437esp_lv_adapter_get_dummy_draw_enabledP12lv_display_t "Permalink to this definition")  

Get the current dummy draw mode state for a display.

This function retrieves whether dummy draw mode is currently enabled. In dummy draw mode, LVGL rendering is bypassed and the application has direct control over the framebuffer.

Parameters

**disp** – **\[in\]** Pointer to LVGL display object

Returns

- true: Dummy draw mode is enabled
- false: Dummy draw mode is disabled (or adapter not initialized/invalid display)

esp\_err\_t esp\_lv\_adapter\_set\_dummy\_draw\_callbacks(lv\_display\_t \*disp, const \*cbs, void \*user\_ctx) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv439esp_lv_adapter_set_dummy_draw_callbacksP12lv_display_tPK37esp_lv_adapter_dummy_draw_callbacks_tPv "Permalink to this definition")  

Register dummy draw callbacks for a display.

Callbacks allow applications to hook into dummy draw transitions and VSYNC notifications (useful for self-refresh flows). Registering new callbacks replaces any previously registered ones. Pass NULL to remove callbacks.

Parameters

- **disp** – Display handle
- **cbs** – Pointer to callback collection (NULL to clear)
- **user\_ctx** – User context pointer passed back to callbacks

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized
- ESP\_ERR\_INVALID\_ARG: Invalid display handle

esp\_err\_t esp\_lv\_adapter\_dummy\_draw\_blit(lv\_display\_t \*disp, int x\_start, int y\_start, int x\_end, int y\_end, const void \*frame\_buffer, bool wait) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv430esp_lv_adapter_dummy_draw_blitP12lv_display_tiiiiPKvb "Permalink to this definition")  

Blit a framebuffer region to display in dummy draw mode.

Directly sends framebuffer to display hardware, bypassing LVGL.

Parameters

- **disp** – **\[in\]** Pointer to LVGL display object
- **x\_start** – **\[in\]** Starting X coordinate (inclusive)
- **y\_start** – **\[in\]** Starting Y coordinate (inclusive)
- **x\_end** – **\[in\]** Ending X coordinate (exclusive)
- **y\_end** – **\[in\]** Ending Y coordinate (exclusive)
- **frame\_buffer** – **\[in\]** Pointer to framebuffer data
- **wait** – **\[in\]** true to wait for completion, false to return immediately

Returns

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_STATE: Adapter not initialized or dummy draw not enabled
- ESP\_ERR\_INVALID\_ARG: Invalid parameters

### Structures

struct esp\_lv\_adapter\_config\_t [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv423esp_lv_adapter_config_t "Permalink to this definition")  

LVGL adapter configuration structure.

Public Members

uint32\_t task\_stack\_size [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N23esp_lv_adapter_config_t15task_stack_sizeE "Permalink to this definition")  

LVGL task stack size in bytes

uint32\_t task\_priority [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N23esp_lv_adapter_config_t13task_priorityE "Permalink to this definition")  

LVGL task priority

int task\_core\_id [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N23esp_lv_adapter_config_t12task_core_idE "Permalink to this definition")  

LVGL task core ID (-1 for no affinity)

uint32\_t tick\_period\_ms [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N23esp_lv_adapter_config_t14tick_period_msE "Permalink to this definition")  

LVGL tick period in milliseconds

uint32\_t task\_min\_delay\_ms [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N23esp_lv_adapter_config_t17task_min_delay_msE "Permalink to this definition")  

Minimum LVGL task delay in milliseconds

uint32\_t task\_max\_delay\_ms [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N23esp_lv_adapter_config_t17task_max_delay_msE "Permalink to this definition")  

Maximum LVGL task delay in milliseconds

bool stack\_in\_psram [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N23esp_lv_adapter_config_t14stack_in_psramE "Permalink to this definition")  

Allocate LVGL task stack in PSRAM when available

struct esp\_lv\_adapter\_dummy\_draw\_callbacks\_t [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv437esp_lv_adapter_dummy_draw_callbacks_t "Permalink to this definition")  

Dummy draw callback collection.

All callbacks are optional; set any member to NULL to omit it.

Note

The on\_vsync callback is executed in interrupt context on most panels, so implementations must be ISR-safe.

Public Members

void (\*on\_enable)(lv\_display\_t \*disp, void \*user\_ctx) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N37esp_lv_adapter_dummy_draw_callbacks_t9on_enableE "Permalink to this definition")  

Called when dummy draw mode is enabled

void (\*on\_disable)(lv\_display\_t \*disp, void \*user\_ctx) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N37esp_lv_adapter_dummy_draw_callbacks_t10on_disableE "Permalink to this definition")  

Called when dummy draw mode is disabled

void (\*on\_color\_trans\_done)(lv\_display\_t \*disp, bool in\_isr, void \*user\_ctx) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N37esp_lv_adapter_dummy_draw_callbacks_t19on_color_trans_doneE "Permalink to this definition")  

Called when color transfer is done

void (\*on\_vsync)(lv\_display\_t \*disp, bool in\_isr, void \*user\_ctx) [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#_CPPv4N37esp_lv_adapter_dummy_draw_callbacks_t8on_vsyncE "Permalink to this definition")  

Called on VSYNC signal (ISR context)

### Macros

ESP\_LV\_ADAPTER\_DEFAULT\_STACK\_SIZE [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#c.ESP_LV_ADAPTER_DEFAULT_STACK_SIZE "Permalink to this definition")  

Default configuration values for LVGL adapter.

Default task stack size (8KB)

ESP\_LV\_ADAPTER\_DEFAULT\_TASK\_PRIORITY [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#c.ESP_LV_ADAPTER_DEFAULT_TASK_PRIORITY "Permalink to this definition")  

Default task priority

ESP\_LV\_ADAPTER\_DEFAULT\_TASK\_CORE\_ID [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#c.ESP_LV_ADAPTER_DEFAULT_TASK_CORE_ID "Permalink to this definition")  

Default task core ID (-1 for no affinity)

ESP\_LV\_ADAPTER\_DEFAULT\_TICK\_PERIOD\_MS [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#c.ESP_LV_ADAPTER_DEFAULT_TICK_PERIOD_MS "Permalink to this definition")  

Default tick period in milliseconds

ESP\_LV\_ADAPTER\_DEFAULT\_TASK\_MIN\_DELAY\_MS [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#c.ESP_LV_ADAPTER_DEFAULT_TASK_MIN_DELAY_MS "Permalink to this definition")  

Default minimum task delay in milliseconds

ESP\_LV\_ADAPTER\_DEFAULT\_TASK\_MAX\_DELAY\_MS [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#c.ESP_LV_ADAPTER_DEFAULT_TASK_MAX_DELAY_MS "Permalink to this definition")  

Default maximum task delay in milliseconds

ESP\_LV\_ADAPTER\_DEFAULT\_CONFIG() [](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lvgl_adapter.html#c.ESP_LV_ADAPTER_DEFAULT_CONFIG "Permalink to this definition")  

Default configuration for LVGL adapter.