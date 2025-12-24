/*
 * ESP32-S3 tutorial 0013:
 * AtomS3R “GIF console” (real GIF playback + button + esp_console).
 *
 * This chapter started as a control-plane MVP (mock animations). It now plays
 * real GIFs using bitbank2/AnimatedGIF, loaded from a flash-bundled FATFS
 * partition ("storage") mounted at /storage (see partitions.csv).
 *
 * Hardware (AtomS3R) wiring (from schematic / prior investigation):
 * - DISP_CS    = GPIO14
 * - SPI_SCK    = GPIO15
 * - SPI_MOSI   = GPIO21
 * - DISP_RS/DC = GPIO42
 * - DISP_RST   = GPIO48
 *
 * Backlight:
 * - Some revisions appear to gate LED_BL via GPIO7 (active-low).
 * - Some revisions expose an I2C brightness device (addr 0x30, reg 0x0e) on SCL=GPIO0/SDA=GPIO45.
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

#include "esp_console.h"
#include "driver/usb_serial_jtag_vfs.h"

#include "M5GFX.h"
// Panel_GC9107 lives next to Panel_GC9A01 in LovyanGFX/M5GFX sources.
#include "lgfx/v1/panel/Panel_GC9A01.hpp"
#include "lgfx/v1/platforms/esp32/Bus_SPI.hpp"

#include "AnimatedGIF.h"
#include "gif_storage.h"

static const char *TAG = "atoms3r_gif_console";

// Fixed AtomS3R wiring.
static constexpr int PIN_LCD_CS = 14;
static constexpr int PIN_LCD_SCK = 15;
static constexpr int PIN_LCD_MOSI = 21;
static constexpr int PIN_LCD_DC = 42;
static constexpr int PIN_LCD_RST = 48;

// -------------------------
// Backlight helpers (optional)
// -------------------------

static bool s_bl_i2c_inited = false;
static constexpr i2c_port_t BL_I2C_PORT = I2C_NUM_0;

static esp_err_t backlight_i2c_write_reg_u8(uint8_t reg, uint8_t value) {
#if CONFIG_TUTORIAL_0013_BACKLIGHT_I2C_ENABLE
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(
        BL_I2C_PORT,
        (uint8_t)CONFIG_TUTORIAL_0013_BL_I2C_ADDR,
        buf,
        sizeof(buf),
        pdMS_TO_TICKS(1000));
#else
    (void)reg;
    (void)value;
    return ESP_OK;
#endif
}

static esp_err_t backlight_i2c_init(void) {
#if CONFIG_TUTORIAL_0013_BACKLIGHT_I2C_ENABLE
    if (s_bl_i2c_inited) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "backlight i2c init: port=%d scl=%d sda=%d addr=0x%02x reg=0x%02x",
             (int)BL_I2C_PORT,
             CONFIG_TUTORIAL_0013_BL_I2C_SCL_GPIO,
             CONFIG_TUTORIAL_0013_BL_I2C_SDA_GPIO,
             (unsigned)CONFIG_TUTORIAL_0013_BL_I2C_ADDR,
             (unsigned)CONFIG_TUTORIAL_0013_BL_I2C_REG);

    // NOTE: We intentionally use the legacy I2C driver here because M5GFX/LovyanGFX
    // links against the legacy driver (`driver/i2c.h`). Mixing legacy + new driver_ng
    // aborts during startup via a constructor in ESP-IDF 5.x.
    //
    // If you migrate M5GFX to `driver/i2c_master.h`, you can switch this back too.
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.scl_io_num = (gpio_num_t)CONFIG_TUTORIAL_0013_BL_I2C_SCL_GPIO;
    conf.sda_io_num = (gpio_num_t)CONFIG_TUTORIAL_0013_BL_I2C_SDA_GPIO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;

    esp_err_t err = i2c_param_config(BL_I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(BL_I2C_PORT, conf.mode, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) {
        // Already installed by some other component (e.g. M5GFX board init).
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    // AtomS3R backlight init sequence (mirrors M5GFX's Light_M5StackAtomS3R):
    // - I2C addr 0x30 (decimal 48)
    // - reg 0x00 := 0b0100_0000
    // - reg 0x08 := 0b0000_0001
    // - reg 0x70 := 0b0000_0000
    // Then brightness is written to reg 0x0e.
    ESP_LOGI(TAG, "backlight i2c chip init: addr=0x%02x (reg 0x00/0x08/0x70)",
             (unsigned)CONFIG_TUTORIAL_0013_BL_I2C_ADDR);

    err = backlight_i2c_write_reg_u8(0x00, 0x40);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight i2c init write reg 0x00 failed: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(1));

    err = backlight_i2c_write_reg_u8(0x08, 0x01);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight i2c init write reg 0x08 failed: %s", esp_err_to_name(err));
        return err;
    }

    err = backlight_i2c_write_reg_u8(0x70, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight i2c init write reg 0x70 failed: %s", esp_err_to_name(err));
        return err;
    }

    s_bl_i2c_inited = true;
    return ESP_OK;
#else
    return ESP_OK;
#endif
}

static esp_err_t backlight_i2c_set(uint8_t brightness) {
#if CONFIG_TUTORIAL_0013_BACKLIGHT_I2C_ENABLE
    esp_err_t err = backlight_i2c_init();
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "backlight i2c set: reg=0x%02x value=%u",
             (unsigned)CONFIG_TUTORIAL_0013_BL_I2C_REG,
             (unsigned)brightness);

    err = backlight_i2c_write_reg_u8((uint8_t)CONFIG_TUTORIAL_0013_BL_I2C_REG, brightness);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_write_to_device failed: %s", esp_err_to_name(err));
    }
    return err;
#else
    (void)brightness;
    return ESP_OK;
#endif
}

static void backlight_gate_gpio_set(bool on) {
#if CONFIG_TUTORIAL_0013_BACKLIGHT_GATE_ENABLE
    gpio_reset_pin((gpio_num_t)CONFIG_TUTORIAL_0013_BACKLIGHT_GATE_GPIO);
    gpio_set_direction((gpio_num_t)CONFIG_TUTORIAL_0013_BACKLIGHT_GATE_GPIO, GPIO_MODE_OUTPUT);

    // Kconfig bool may be undefined when disabled; use preprocessor guards.
#if CONFIG_TUTORIAL_0013_BACKLIGHT_GATE_ACTIVE_LOW
    const int level = on ? 0 : 1;
#else
    const int level = on ? 1 : 0;
#endif
    ESP_LOGI(TAG, "backlight gate gpio: pin=%d active_low=%d -> %s (level=%d)",
             CONFIG_TUTORIAL_0013_BACKLIGHT_GATE_GPIO,
             (int)(0
#if CONFIG_TUTORIAL_0013_BACKLIGHT_GATE_ACTIVE_LOW
                   + 1
#endif
                   ),
             on ? "ON" : "OFF",
             level);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0013_BACKLIGHT_GATE_GPIO, level);
#else
    (void)on;
#endif
}

static void backlight_prepare_for_init(void) {
    // Keep backlight off during panel init to avoid flashing garbage.
    backlight_gate_gpio_set(false);
    (void)backlight_i2c_set(0);
}

static void backlight_enable_after_init(void) {
    backlight_gate_gpio_set(true);
#if CONFIG_TUTORIAL_0013_BACKLIGHT_I2C_ENABLE
    (void)backlight_i2c_set((uint8_t)CONFIG_TUTORIAL_0013_BL_BRIGHTNESS_ON);
#endif
}

// -------------------------
// Display init via M5GFX/LovyanGFX
// -------------------------

static lgfx::Bus_SPI s_bus;
static lgfx::Panel_GC9107 s_panel;
static m5gfx::M5GFX s_display;

static bool display_init_m5gfx(void) {
    // Configure SPI bus
    {
        auto cfg = s_bus.config();
        cfg.spi_host = SPI3_HOST; // AtomS3(R) uses SPI3 in M5GFX
        cfg.spi_mode = 0;
        cfg.spi_3wire = true;
        cfg.freq_write = CONFIG_TUTORIAL_0013_LCD_SPI_PCLK_HZ;
        cfg.freq_read = 16000000;
        cfg.pin_sclk = PIN_LCD_SCK;
        cfg.pin_mosi = PIN_LCD_MOSI;
        cfg.pin_miso = -1;
        cfg.pin_dc = PIN_LCD_DC;
        s_bus.config(cfg);
    }

    // Configure panel
    {
        s_panel.bus(&s_bus);
        auto pcfg = s_panel.config();
        pcfg.pin_cs = PIN_LCD_CS;
        pcfg.pin_rst = PIN_LCD_RST;
        pcfg.panel_width = CONFIG_TUTORIAL_0013_LCD_HRES;
        pcfg.panel_height = CONFIG_TUTORIAL_0013_LCD_VRES;
        pcfg.offset_x = CONFIG_TUTORIAL_0013_LCD_X_OFFSET;
        pcfg.offset_y = CONFIG_TUTORIAL_0013_LCD_Y_OFFSET;
        pcfg.readable = false;
#if CONFIG_TUTORIAL_0013_LCD_INVERT
        pcfg.invert = true;
#else
        pcfg.invert = false;
#endif
#if CONFIG_TUTORIAL_0013_LCD_RGB_ORDER_RGB
        pcfg.rgb_order = true; // RGB
#else
        pcfg.rgb_order = false; // BGR
#endif
        pcfg.bus_shared = false;
        s_panel.config(pcfg);
    }

    // Kconfig bools are often undefined when disabled; keep all logging compile-safe.
#if CONFIG_TUTORIAL_0013_LCD_INVERT
    const int invert = 1;
#else
    const int invert = 0;
#endif
#if CONFIG_TUTORIAL_0013_LCD_RGB_ORDER_RGB
    const char *rgb_order = "RGB";
#else
    const char *rgb_order = "BGR";
#endif

    ESP_LOGI(TAG, "m5gfx init: pclk=%dHz gap=(%d,%d) invert=%d rgb_order=%s",
             CONFIG_TUTORIAL_0013_LCD_SPI_PCLK_HZ,
             CONFIG_TUTORIAL_0013_LCD_X_OFFSET,
             CONFIG_TUTORIAL_0013_LCD_Y_OFFSET,
             invert,
             rgb_order);

    bool ok = s_display.init(&s_panel);
    if (!ok) {
        ESP_LOGE(TAG, "M5GFX init failed");
        return false;
    }

    s_display.setColorDepth(16);
    s_display.setRotation(0);
    return true;
}

static void present_canvas(M5Canvas &canvas) {
#if CONFIG_TUTORIAL_0013_PRESENT_USE_DMA
    canvas.pushSprite(0, 0);
    s_display.waitDMA();
#else
    canvas.pushSprite(0, 0);
#endif
}

// -------------------------
// GIF playback (AnimatedGIF + FATFS assets)
// -------------------------

typedef struct gif_render_ctx_tag {
    M5Canvas *canvas;
    int canvas_w;
    int canvas_h;
    int gif_canvas_w;
    int gif_canvas_h;
    int scale_x;
    int scale_y;
    int off_x;
    int off_y;
} GifRenderCtx;

static AnimatedGIF s_gif;

static void GIFDraw(GIFDRAW *pDraw) {
    auto *ctx = (GifRenderCtx *)pDraw->pUser;
    if (!ctx || !ctx->canvas) {
        return;
    }

    auto *dst = (uint16_t *)ctx->canvas->getBuffer();
    const uint8_t *s = pDraw->pPixels;
    const uint16_t *pal = pDraw->pPalette;

    // Map this scanline into the output canvas. We treat AnimatedGIF coordinates as being
    // relative to the GIF canvas, then scale/offset into our 128x128 screen canvas.
    const int src_y = pDraw->iY + pDraw->y;
    const int src_x0 = pDraw->iX;
    const int src_w = pDraw->iWidth;

    // Nearest-neighbor scaling (integer scale factors).
    const int sx = (ctx->scale_x > 0) ? ctx->scale_x : 1;
    const int sy = (ctx->scale_y > 0) ? ctx->scale_y : 1;

    const int dst_y0 = ctx->off_y + src_y * sy;
    for (int dy = 0; dy < sy; dy++) {
        const int y = dst_y0 + dy;
        if ((unsigned)y >= (unsigned)ctx->canvas_h) {
            continue;
        }
        uint16_t *row = &dst[y * ctx->canvas_w];

        if (pDraw->ucHasTransparency) {
            const uint8_t t = pDraw->ucTransparent;
            for (int i = 0; i < src_w; i++) {
                const uint8_t idx = s[i];
                if (idx == t) {
                    continue;
                }
                uint16_t c = pal[idx];
#if CONFIG_TUTORIAL_0013_GIF_SWAP_BYTES
                c = __builtin_bswap16(c);
#endif
                const int dst_x0 = ctx->off_x + (src_x0 + i) * sx;
                for (int dx = 0; dx < sx; dx++) {
                    const int x = dst_x0 + dx;
                    if ((unsigned)x < (unsigned)ctx->canvas_w) {
                        row[x] = c;
                    }
                }
            }
        } else {
            for (int i = 0; i < src_w; i++) {
                uint16_t c = pal[s[i]];
#if CONFIG_TUTORIAL_0013_GIF_SWAP_BYTES
                c = __builtin_bswap16(c);
#endif
                const int dst_x0 = ctx->off_x + (src_x0 + i) * sx;
                for (int dx = 0; dx < sx; dx++) {
                    const int x = dst_x0 + dx;
                    if ((unsigned)x < (unsigned)ctx->canvas_w) {
                        row[x] = c;
                    }
                }
            }
        }
    }
}

static char s_gif_paths[CONFIG_TUTORIAL_0013_GIF_MAX_COUNT][CONFIG_TUTORIAL_0013_GIF_MAX_PATH_LEN];
static int s_gif_count = 0;
static char s_current_gif[CONFIG_TUTORIAL_0013_GIF_MAX_PATH_LEN] = {0};
static int32_t s_current_gif_file_size = 0;

static int refresh_gif_list(void) {
    s_gif_count = gif_storage_list_gifs((char *)s_gif_paths, CONFIG_TUTORIAL_0013_GIF_MAX_COUNT, CONFIG_TUTORIAL_0013_GIF_MAX_PATH_LEN);
    return s_gif_count;
}

static const char *path_basename(const char *path) {
    if (!path) return "";
    const char *slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

static int find_gif_index_by_name(const char *name) {
    if (!name || !*name) return -1;
    for (int i = 0; i < s_gif_count; i++) {
        const char *base = path_basename(s_gif_paths[i]);
        if (strcasecmp(name, base) == 0) return i;
        // Allow omitting ".gif"
        const size_t base_len = strlen(base);
        if (base_len > 4 && strcasecmp(base + (base_len - 4), ".gif") == 0) {
            const size_t stem_len = base_len - 4;
            if (strlen(name) == stem_len && strncasecmp(name, base, stem_len) == 0) {
                return i;
            }
        }
    }
    return -1;
}

// AnimatedGIF file callbacks (stream from FATFS; avoid loading whole file into RAM)
static void *gif_open_cb(const char *szFilename, int32_t *pFileSize) {
    if (pFileSize) *pFileSize = 0;
    FILE *f = fopen(szFilename, "rb");
    if (!f) {
        ESP_LOGE(TAG, "gif_open_cb: fopen failed: %s errno=%d", szFilename, errno);
        return nullptr;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return nullptr;
    }
    const long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return nullptr;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return nullptr;
    }
    if (pFileSize) *pFileSize = (int32_t)size;
    s_current_gif_file_size = (int32_t)size;
    return (void *)f;
}

static void gif_close_cb(void *pHandle) {
    if (!pHandle) return;
    fclose((FILE *)pHandle);
}

static int32_t gif_read_cb(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    if (!pFile || !pBuf || iLen <= 0) return 0;
    FILE *f = (FILE *)pFile->fHandle;
    if (!f) return 0;
    const size_t n = fread(pBuf, 1, (size_t)iLen, f);
    pFile->iPos += (int32_t)n;
    return (int32_t)n;
}

static int32_t gif_seek_cb(GIFFILE *pFile, int32_t iPosition) {
    if (!pFile) return -1;
    FILE *f = (FILE *)pFile->fHandle;
    if (!f) return -1;
    if (fseek(f, (long)iPosition, SEEK_SET) != 0) {
        return -1;
    }
    pFile->iPos = iPosition;
    return iPosition;
}

static bool open_gif_index(int idx, GifRenderCtx *ctx) {
    if (idx < 0 || idx >= s_gif_count || !ctx) {
        return false;
    }

    const char *path = s_gif_paths[idx];

    // Close previous and attempt open. Re-begin resets internal state, including file position.
    s_gif.close();
    s_gif.begin(GIF_PALETTE_RGB565_LE);
    s_current_gif_file_size = 0;
    const int open_ok = s_gif.open(path, gif_open_cb, gif_close_cb, gif_read_cb, gif_seek_cb, GIFDraw);
    if (!open_ok) {
        const int last_err = s_gif.getLastError();
        ESP_LOGE(TAG, "gif open failed: %s open_ok=%d last_error=%d", path, open_ok, last_err);
        return false;
    }

    snprintf(s_current_gif, sizeof(s_current_gif), "%s", path);

    // Update render ctx with GIF canvas geometry.
    ctx->gif_canvas_w = s_gif.getCanvasWidth();
    ctx->gif_canvas_h = s_gif.getCanvasHeight();
#if CONFIG_TUTORIAL_0013_GIF_SCALE_TO_FULL_SCREEN
    ctx->scale_x = (ctx->gif_canvas_w > 0) ? (ctx->canvas_w / ctx->gif_canvas_w) : 1;
    ctx->scale_y = (ctx->gif_canvas_h > 0) ? (ctx->canvas_h / ctx->gif_canvas_h) : 1;
    if (ctx->scale_x < 1) ctx->scale_x = 1;
    if (ctx->scale_y < 1) ctx->scale_y = 1;
#else
    ctx->scale_x = 1;
    ctx->scale_y = 1;
#endif
    const int scaled_w = ctx->gif_canvas_w * ctx->scale_x;
    const int scaled_h = ctx->gif_canvas_h * ctx->scale_y;
    ctx->off_x = (scaled_w < ctx->canvas_w) ? ((ctx->canvas_w - scaled_w) / 2) : 0;
    ctx->off_y = (scaled_h < ctx->canvas_h) ? ((ctx->canvas_h - scaled_h) / 2) : 0;

    ESP_LOGI(TAG, "gif open ok: %s bytes=%u canvas=%dx%d frame=%dx%d off=(%d,%d)",
             path,
             (unsigned)s_current_gif_file_size,
             s_gif.getCanvasWidth(),
             s_gif.getCanvasHeight(),
             s_gif.getFrameWidth(),
             s_gif.getFrameHeight(),
             s_gif.getFrameXOff(),
             s_gif.getFrameYOff());

    ESP_LOGI(TAG, "gif render: swap_bytes=%d scale=%dx%d gif_canvas=%dx%d -> scaled=%dx%d off=(%d,%d)",
             (int)(0
#if CONFIG_TUTORIAL_0013_GIF_SWAP_BYTES
                   + 1
#endif
                   ),
             ctx->scale_x, ctx->scale_y,
             ctx->gif_canvas_w, ctx->gif_canvas_h,
             scaled_w, scaled_h,
             ctx->off_x, ctx->off_y);

    return true;
}

// -------------------------
// Control plane: queue + button + console commands
// -------------------------

enum class CtrlType : uint8_t {
    PlayIndex,
    Stop,
    Next,
    Info,
    SetBrightness,
};

struct CtrlEvent {
    CtrlType type;
    int32_t arg; // index or brightness
};

static QueueHandle_t s_ctrl_q = nullptr;

static bool try_parse_int(const char *s, int *out) {
    if (!s || !*s) return false;
    char *end = nullptr;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (int)v;
    return true;
}

static void ctrl_send(const CtrlEvent &ev) {
    if (!s_ctrl_q) return;
    // Drop on overflow: control plane should remain non-blocking.
    (void)xQueueSend(s_ctrl_q, &ev, 0);
}

static void IRAM_ATTR button_isr(void *arg) {
    (void)arg;
    if (!s_ctrl_q) return;
    CtrlEvent ev = {.type = CtrlType::Next, .arg = 0};
    BaseType_t hp_task_woken = pdFALSE;
    xQueueSendFromISR(s_ctrl_q, &ev, &hp_task_woken);
    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}

static void button_init(void) {
    const gpio_num_t pin = (gpio_num_t)CONFIG_TUTORIAL_0013_BUTTON_GPIO;
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << (int)pin;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
#if CONFIG_TUTORIAL_0013_BUTTON_ACTIVE_LOW
    io.intr_type = GPIO_INTR_NEGEDGE;
#else
    io.intr_type = GPIO_INTR_POSEDGE;
#endif

    ESP_ERROR_CHECK(gpio_config(&io));

    // NOTE: If another module already installed the ISR service, INVALID_STATE is OK.
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(pin, button_isr, nullptr));

    ESP_LOGI(TAG, "button init: gpio=%d active_low=%d debounce_ms=%d",
             (int)pin,
             (int)(0
#if CONFIG_TUTORIAL_0013_BUTTON_ACTIVE_LOW
                   + 1
#endif
                   ),
             CONFIG_TUTORIAL_0013_BUTTON_DEBOUNCE_MS);
}

static void button_poll_debug_log(void) {
    const gpio_num_t pin = (gpio_num_t)CONFIG_TUTORIAL_0013_BUTTON_GPIO;
    int level = gpio_get_level(pin);
#if CONFIG_TUTORIAL_0013_BUTTON_ACTIVE_LOW
    const int pressed = (level == 0) ? 1 : 0;
#else
    const int pressed = (level != 0) ? 1 : 0;
#endif
    ESP_LOGI(TAG, "button poll: gpio=%d level=%d pressed=%d", (int)pin, level, pressed);
}

static int cmd_list(int, char **) {
    const int n = refresh_gif_list();
    printf("gif assets (%d):\n", n);
    for (int i = 0; i < n; i++) {
        printf("  %d: %s\n", i, path_basename(s_gif_paths[i]));
    }
    return 0;
}

static int cmd_play(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: play <id|name>\n");
        return 1;
    }
    // Refresh on demand so list/play reflects whatever is currently flashed into storage.
    (void)refresh_gif_list();

    int idx = -1;
    if (try_parse_int(argv[1], &idx)) {
        // ok
    } else {
        idx = find_gif_index_by_name(argv[1]);
    }
    if (idx < 0 || idx >= s_gif_count) {
        printf("invalid gif: %s\n", argv[1]);
        return 1;
    }
    ctrl_send({.type = CtrlType::PlayIndex, .arg = idx});
    return 0;
}

static int cmd_stop(int, char **) {
    ctrl_send({.type = CtrlType::Stop, .arg = 0});
    return 0;
}

static int cmd_next(int, char **) {
    ctrl_send({.type = CtrlType::Next, .arg = 0});
    return 0;
}

static int cmd_info(int, char **) {
    ctrl_send({.type = CtrlType::Info, .arg = 0});
    return 0;
}

static int cmd_brightness(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: brightness <0..255>\n");
        return 1;
    }
    int v = 0;
    if (!try_parse_int(argv[1], &v) || v < 0 || v > 255) {
        printf("invalid brightness: %s\n", argv[1]);
        return 1;
    }
    ctrl_send({.type = CtrlType::SetBrightness, .arg = v});
    return 0;
}

static void console_register_commands(void) {
    esp_console_cmd_t cmd = {};

    cmd = {};
    cmd.command = "list";
    cmd.help = "List available GIFs (from /storage/gifs)";
    cmd.func = &cmd_list;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "play";
    cmd.help = "Play GIF by id or name: play <id|name>";
    cmd.func = &cmd_play;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "stop";
    cmd.help = "Stop playback";
    cmd.func = &cmd_stop;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "next";
    cmd.help = "Select next GIF and play";
    cmd.func = &cmd_next;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "info";
    cmd.help = "Print current playback state";
    cmd.func = &cmd_info;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "brightness";
    cmd.help = "Set backlight brightness (0..255) when I2C brightness device is enabled";
    cmd.func = &cmd_brightness;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void console_start_uart_grove(void) {
#if CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_repl_t *repl = nullptr;

    // IMPORTANT:
    // Like the USB Serial/JTAG helper, `esp_console_new_repl_uart()` also calls
    // `esp_console_init()` internally (via esp_console_common_init). Don’t call
    // esp_console_init() yourself beforehand.
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = CONFIG_TUTORIAL_0013_CONSOLE_PROMPT;

    esp_console_dev_uart_config_t hw_config = {
        .channel = CONFIG_TUTORIAL_0013_CONSOLE_UART_NUM,
        .baud_rate = CONFIG_TUTORIAL_0013_CONSOLE_UART_BAUDRATE,
        .tx_gpio_num = CONFIG_TUTORIAL_0013_CONSOLE_UART_TX_GPIO,
        .rx_gpio_num = CONFIG_TUTORIAL_0013_CONSOLE_UART_RX_GPIO,
    };

    esp_err_t err = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_uart failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    console_register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over UART (channel=%d tx=%d rx=%d baud=%d)",
             CONFIG_TUTORIAL_0013_CONSOLE_UART_NUM,
             CONFIG_TUTORIAL_0013_CONSOLE_UART_TX_GPIO,
             CONFIG_TUTORIAL_0013_CONSOLE_UART_RX_GPIO,
             CONFIG_TUTORIAL_0013_CONSOLE_UART_BAUDRATE);
#else
    ESP_LOGW(TAG, "UART console support is disabled in sdkconfig (CONFIG_ESP_CONSOLE_UART_*); no console started");
#endif
}

static void console_start_usb_serial_jtag(void) {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_repl_t *repl = nullptr;

    // IMPORTANT (ESP-IDF 5.4.1):
    // `esp_console_new_repl_usb_serial_jtag()` internally installs the USB Serial/JTAG driver
    // and also initializes `esp_console`. If we call `esp_console_init()` ourselves first,
    // `esp_console_new_repl_usb_serial_jtag()` will fail with ESP_ERR_INVALID_STATE.
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "gif> ";

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    console_register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over USB Serial/JTAG");
#else
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no console started");
#endif
}

static void console_start(void) {
#if CONFIG_TUTORIAL_0013_CONSOLE_BINDING_GROVE_UART
    console_start_uart_grove();
#elif CONFIG_TUTORIAL_0013_CONSOLE_BINDING_USB_SERIAL_JTAG
    console_start_usb_serial_jtag();
#else
    // Should not happen (choice), but keep it safe.
    console_start_uart_grove();
#endif
}

extern "C" void app_main(void) {
    const int w = CONFIG_TUTORIAL_0013_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0013_LCD_VRES;

    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    backlight_prepare_for_init();

    ESP_LOGI(TAG, "bringing up M5GFX display...");
    if (!display_init_m5gfx()) {
        ESP_LOGE(TAG, "display init failed, aborting");
        return;
    }

    backlight_enable_after_init();

    // Visual sanity test: show solid colors before starting the animation.
    // This helps distinguish "backlight/power flicker" from "frame update tearing".
    ESP_LOGI(TAG, "visual test: solid colors (red/green/blue/white/black)");
    s_display.fillScreen(TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(120));
    s_display.fillScreen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(120));
    s_display.fillScreen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(120));
    s_display.fillScreen(TFT_WHITE);
    vTaskDelay(pdMS_TO_TICKS(120));
    s_display.fillScreen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Offscreen canvas (LovyanGFX sprite). Keep it out of PSRAM by default:
    // - sprite buffers allocate using AllocationSource::Dma when PSRAM is disabled
    // - that is safest for SPI DMA full-frame presents.
    M5Canvas canvas(&s_display);
#if CONFIG_TUTORIAL_0013_CANVAS_USE_PSRAM
    canvas.setPsram(true);
#else
    canvas.setPsram(false);
#endif
    canvas.setColorDepth(16);
    void *buf = canvas.createSprite(w, h);
    if (!buf) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", w, h);
        return;
    }

    ESP_LOGI(TAG, "canvas ok: %u bytes; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             (unsigned)canvas.bufferLength(),
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    // Control plane wiring.
    s_ctrl_q = xQueueCreate(16, sizeof(CtrlEvent));
    if (!s_ctrl_q) {
        ESP_LOGE(TAG, "failed to create control queue");
        return;
    }

    button_init();
    console_start();
    button_poll_debug_log();

    // Storage + asset registry.
    esp_err_t storage_err = gif_storage_mount();
    if (storage_err != ESP_OK) {
        ESP_LOGW(TAG, "storage mount failed: %s (no GIFs will be available)", esp_err_to_name(storage_err));
    }
    const int n_gifs = refresh_gif_list();
    ESP_LOGI(TAG, "gif registry: %d asset(s) found under /storage/gifs", n_gifs);

    GifRenderCtx gif_ctx = {};
    gif_ctx.canvas = &canvas;
    gif_ctx.canvas_w = w;
    gif_ctx.canvas_h = h;

    int gif_idx = 0;
    bool playing = false;
    int delay_ms = CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS;
    uint32_t frame = 0;
    uint32_t last_next_ms = 0;

    if (n_gifs > 0) {
        canvas.fillScreen(TFT_BLACK);
        present_canvas(canvas);
        if (open_gif_index(gif_idx, &gif_ctx)) {
            playing = true;
            printf("playing: %d (%s)\n", gif_idx, path_basename(s_gif_paths[gif_idx]));
        }
    } else {
        printf("no gifs found under /storage/gifs (flash a storage partition image)\n");
    }

    ESP_LOGI(TAG, "starting GIF playback loop (min_delay_ms=%d dma_present=%d psram=%d)",
             CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS,
             (int)(0
#if CONFIG_TUTORIAL_0013_PRESENT_USE_DMA
                   + 1
#endif
                   ),
             (int)(0
#if CONFIG_TUTORIAL_0013_CANVAS_USE_PSRAM
                   + 1
#endif
                   ));

    while (true) {
        TickType_t wait_ticks = portMAX_DELAY;
        if (playing) {
            const int wms = (delay_ms > 0) ? delay_ms : CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS;
            wait_ticks = pdMS_TO_TICKS((uint32_t)wms);
        }

        CtrlEvent ev = {};
        if (xQueueReceive(s_ctrl_q, &ev, wait_ticks) == pdTRUE) {
            const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
            switch (ev.type) {
                case CtrlType::PlayIndex: {
                    int idx = (int)ev.arg;
                    if (idx >= 0 && idx < s_gif_count) {
                        gif_idx = idx;
                        frame = 0;
                        if (open_gif_index(gif_idx, &gif_ctx)) {
                            playing = true;
                            delay_ms = CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS;
                            printf("playing: %d (%s)\n", gif_idx, path_basename(s_gif_paths[gif_idx]));
                        } else {
                            playing = false;
                            printf("failed to open: %d\n", gif_idx);
                        }
                    }
                    break;
                }
                case CtrlType::Stop:
                    playing = false;
                    printf("stopped\n");
                    break;
                case CtrlType::Next: {
                    const uint32_t dt = now_ms - last_next_ms;
                    if (dt < (uint32_t)CONFIG_TUTORIAL_0013_BUTTON_DEBOUNCE_MS) {
                        break;
                    }
                    last_next_ms = now_ms;
                    if (s_gif_count <= 0) {
                        break;
                    }
                    gif_idx = (gif_idx + 1) % s_gif_count;
                    frame = 0;
                    if (open_gif_index(gif_idx, &gif_ctx)) {
                        playing = true;
                        delay_ms = CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS;
                        printf("next: %d (%s)\n", gif_idx, path_basename(s_gif_paths[gif_idx]));
                    } else {
                        playing = false;
                        printf("failed to open: %d\n", gif_idx);
                    }
                    break;
                }
                case CtrlType::Info:
                    printf("state: playing=%d gif=%d/%d name=%s bytes=%u frame=%" PRIu32 " min_delay_ms=%d last_delay_ms=%d\n",
                           playing ? 1 : 0,
                           gif_idx,
                           s_gif_count,
                           s_current_gif[0] ? path_basename(s_current_gif) : "(none)",
                           (unsigned)s_current_gif_file_size,
                           frame,
                           CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS,
                           delay_ms);
                    printf("gif geom: canvas=%dx%d frame=%dx%d off=(%d,%d) render_scale=%dx%d render_off=(%d,%d)\n",
                           s_gif.getCanvasWidth(),
                           s_gif.getCanvasHeight(),
                           s_gif.getFrameWidth(),
                           s_gif.getFrameHeight(),
                           s_gif.getFrameXOff(),
                           s_gif.getFrameYOff(),
                           gif_ctx.scale_x,
                           gif_ctx.scale_y,
                           gif_ctx.off_x,
                           gif_ctx.off_y);
                    break;
                case CtrlType::SetBrightness: {
                    int v = (int)ev.arg;
                    (void)backlight_i2c_set((uint8_t)v);
                    printf("brightness: %d\n", v);
                    break;
                }
            }
            continue;
        }

        if (!playing) {
            continue;
        }
        int frame_delay_ms = 0;
        const int prc = s_gif.playFrame(false, &frame_delay_ms, &gif_ctx);
        const int last_err = s_gif.getLastError();

        // IMPORTANT: per AnimatedGIF docs:
        // - playFrame() can return 0 even if it successfully rendered a frame (EOF reached right after).
        //   In that case last_err will be GIF_SUCCESS and we still must present the canvas at least once.
        if (prc > 0 || last_err == GIF_SUCCESS) {
            present_canvas(canvas);
            frame++;

            if (frame_delay_ms < CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS) {
                frame_delay_ms = CONFIG_TUTORIAL_0013_GIF_MIN_FRAME_DELAY_MS;
            }
            delay_ms = frame_delay_ms;
        }

        if (prc == 0) {
            canvas.fillScreen(TFT_BLACK);
            s_gif.reset();
            vTaskDelay(pdMS_TO_TICKS(1));
        } else if (prc < 0) {
            ESP_LOGE(TAG, "gif playFrame failed: last_error=%d", last_err);
            canvas.fillScreen(TFT_BLACK);
            s_gif.reset();
            vTaskDelay(pdMS_TO_TICKS(100));
        }

#if CONFIG_TUTORIAL_0013_LOG_EVERY_N_FRAMES > 0
        if ((frame % (uint32_t)CONFIG_TUTORIAL_0013_LOG_EVERY_N_FRAMES) == 0) {
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " free_heap=%" PRIu32, frame, esp_get_free_heap_size());
        }
#endif
    }
}



