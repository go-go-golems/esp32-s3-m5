/*
 * ESP32-S3 tutorial 0014:
 * AtomS3R SPI LCD animation using M5GFX (LovyanGFX) with a minimal playback harness
 * for GIF decoding (ticket 0010: port AnimatedGIF as an ESP-IDF component).
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

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "M5GFX.h"
// Panel_GC9107 lives next to Panel_GC9A01 in LovyanGFX/M5GFX sources.
#include "lgfx/v1/panel/Panel_GC9A01.hpp"
#include "lgfx/v1/platforms/esp32/Bus_SPI.hpp"

#include "AnimatedGIF.h"

#include "assets/green.h"

static const char *TAG = "atoms3r_animatedgif_single";

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
#if CONFIG_TUTORIAL_0014_BACKLIGHT_I2C_ENABLE
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(
        BL_I2C_PORT,
        (uint8_t)CONFIG_TUTORIAL_0014_BL_I2C_ADDR,
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
#if CONFIG_TUTORIAL_0014_BACKLIGHT_I2C_ENABLE
    if (s_bl_i2c_inited) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "backlight i2c init: port=%d scl=%d sda=%d addr=0x%02x reg=0x%02x",
             (int)BL_I2C_PORT,
             CONFIG_TUTORIAL_0014_BL_I2C_SCL_GPIO,
             CONFIG_TUTORIAL_0014_BL_I2C_SDA_GPIO,
             (unsigned)CONFIG_TUTORIAL_0014_BL_I2C_ADDR,
             (unsigned)CONFIG_TUTORIAL_0014_BL_I2C_REG);

    // NOTE: We intentionally use the legacy I2C driver here because M5GFX/LovyanGFX
    // links against the legacy driver (`driver/i2c.h`). Mixing legacy + new driver_ng
    // aborts during startup via a constructor in ESP-IDF 5.x.
    //
    // If you migrate M5GFX to `driver/i2c_master.h`, you can switch this back too.
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.scl_io_num = (gpio_num_t)CONFIG_TUTORIAL_0014_BL_I2C_SCL_GPIO;
    conf.sda_io_num = (gpio_num_t)CONFIG_TUTORIAL_0014_BL_I2C_SDA_GPIO;
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
             (unsigned)CONFIG_TUTORIAL_0014_BL_I2C_ADDR);

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
#if CONFIG_TUTORIAL_0014_BACKLIGHT_I2C_ENABLE
    esp_err_t err = backlight_i2c_init();
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "backlight i2c set: reg=0x%02x value=%u",
             (unsigned)CONFIG_TUTORIAL_0014_BL_I2C_REG,
             (unsigned)brightness);

    err = backlight_i2c_write_reg_u8((uint8_t)CONFIG_TUTORIAL_0014_BL_I2C_REG, brightness);
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
#if CONFIG_TUTORIAL_0014_BACKLIGHT_GATE_ENABLE
    gpio_reset_pin((gpio_num_t)CONFIG_TUTORIAL_0014_BACKLIGHT_GATE_GPIO);
    gpio_set_direction((gpio_num_t)CONFIG_TUTORIAL_0014_BACKLIGHT_GATE_GPIO, GPIO_MODE_OUTPUT);

    // Kconfig bool may be undefined when disabled; use preprocessor guards.
#if CONFIG_TUTORIAL_0014_BACKLIGHT_GATE_ACTIVE_LOW
    const int level = on ? 0 : 1;
#else
    const int level = on ? 1 : 0;
#endif
    ESP_LOGI(TAG, "backlight gate gpio: pin=%d active_low=%d -> %s (level=%d)",
             CONFIG_TUTORIAL_0014_BACKLIGHT_GATE_GPIO,
             (int)(0
#if CONFIG_TUTORIAL_0014_BACKLIGHT_GATE_ACTIVE_LOW
                   + 1
#endif
                   ),
             on ? "ON" : "OFF",
             level);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0014_BACKLIGHT_GATE_GPIO, level);
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
#if CONFIG_TUTORIAL_0014_BACKLIGHT_I2C_ENABLE
    (void)backlight_i2c_set((uint8_t)CONFIG_TUTORIAL_0014_BL_BRIGHTNESS_ON);
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
        cfg.freq_write = CONFIG_TUTORIAL_0014_LCD_SPI_PCLK_HZ;
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
        pcfg.panel_width = CONFIG_TUTORIAL_0014_LCD_HRES;
        pcfg.panel_height = CONFIG_TUTORIAL_0014_LCD_VRES;
        pcfg.offset_x = CONFIG_TUTORIAL_0014_LCD_X_OFFSET;
        pcfg.offset_y = CONFIG_TUTORIAL_0014_LCD_Y_OFFSET;
        pcfg.readable = false;
#if CONFIG_TUTORIAL_0014_LCD_INVERT
        pcfg.invert = true;
#else
        pcfg.invert = false;
#endif
#if CONFIG_TUTORIAL_0014_LCD_RGB_ORDER_RGB
        pcfg.rgb_order = true; // RGB
#else
        pcfg.rgb_order = false; // BGR
#endif
        pcfg.bus_shared = false;
        s_panel.config(pcfg);
    }

    // Kconfig bools are often undefined when disabled; keep all logging compile-safe.
#if CONFIG_TUTORIAL_0014_LCD_INVERT
    const int invert = 1;
#else
    const int invert = 0;
#endif
#if CONFIG_TUTORIAL_0014_LCD_RGB_ORDER_RGB
    const char *rgb_order = "RGB";
#else
    const char *rgb_order = "BGR";
#endif

    ESP_LOGI(TAG, "m5gfx init: pclk=%dHz gap=(%d,%d) invert=%d rgb_order=%s",
             CONFIG_TUTORIAL_0014_LCD_SPI_PCLK_HZ,
             CONFIG_TUTORIAL_0014_LCD_X_OFFSET,
             CONFIG_TUTORIAL_0014_LCD_Y_OFFSET,
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
#if CONFIG_TUTORIAL_0014_PRESENT_USE_DMA
    canvas.pushSprite(0, 0);
    s_display.waitDMA();
#else
    canvas.pushSprite(0, 0);
#endif
}

typedef struct gif_render_ctx_tag {
    M5Canvas *canvas;
    int canvas_w;
    int canvas_h;
} GifRenderCtx;

static AnimatedGIF s_gif;

static void GIFDraw(GIFDRAW *pDraw) {
    auto *ctx = (GifRenderCtx *)pDraw->pUser;
    if (!ctx || !ctx->canvas) {
        return;
    }

    const int y = pDraw->iY + pDraw->y;
    if (y < 0 || y >= ctx->canvas_h) {
        return;
    }

    int x = pDraw->iX;
    int w = pDraw->iWidth;
    int src_off = 0;
    if (x < 0) {
        src_off = -x;
        x = 0;
        w -= src_off;
    }
    if (x + w > ctx->canvas_w) {
        w = ctx->canvas_w - x;
    }
    if (w <= 0) {
        return;
    }

    auto *dst = (uint16_t *)ctx->canvas->getBuffer();
    uint16_t *d = &dst[y * ctx->canvas_w + x];
    const uint8_t *s = &pDraw->pPixels[src_off];
    const uint16_t *pal = pDraw->pPalette;

    if (pDraw->ucHasTransparency) {
        const uint8_t t = pDraw->ucTransparent;
        for (int i = 0; i < w; i++) {
            const uint8_t idx = s[i];
            if (idx != t) {
                d[i] = pal[idx];
            }
        }
    } else {
        for (int i = 0; i < w; i++) {
            d[i] = pal[s[i]];
        }
    }
}

extern "C" void app_main(void) {
    const int w = CONFIG_TUTORIAL_0014_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0014_LCD_VRES;

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

    // Visual sanity test: show solid colors before starting playback.
    ESP_LOGI(TAG, "visual test: solid colors (red/green/blue/white/black)");
    s_display.fillScreen(TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(250));
    s_display.fillScreen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(250));
    s_display.fillScreen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(250));
    s_display.fillScreen(TFT_WHITE);
    vTaskDelay(pdMS_TO_TICKS(250));
    s_display.fillScreen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(250));

    // Offscreen canvas (LovyanGFX sprite). Keep it out of PSRAM by default:
    // - sprite buffers allocate using AllocationSource::Dma when PSRAM is disabled
    // - that is safest for SPI DMA full-frame presents.
    M5Canvas canvas(&s_display);
#if CONFIG_TUTORIAL_0014_CANVAS_USE_PSRAM
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

    canvas.fillScreen(TFT_BLACK);
    present_canvas(canvas);

    GifRenderCtx ctx = {};
    ctx.canvas = &canvas;
    ctx.canvas_w = w;
    ctx.canvas_h = h;

    // Decode output for RAW mode is palettized indices; we translate through the RGB565 palette.
    s_gif.begin(GIF_PALETTE_RGB565_LE);
    int open_rc = s_gif.open((uint8_t *)green, (int)sizeof(green), GIFDraw);
    if (open_rc != GIF_SUCCESS) {
        ESP_LOGE(TAG, "gif open failed: rc=%d", open_rc);
        return;
    }

    ESP_LOGI(TAG, "gif open ok: canvas=%dx%d frame=%dx%d off=(%d,%d)",
             s_gif.getCanvasWidth(),
             s_gif.getCanvasHeight(),
             s_gif.getFrameWidth(),
             s_gif.getFrameHeight(),
             s_gif.getFrameXOff(),
             s_gif.getFrameYOff());

#if CONFIG_TUTORIAL_0014_LOG_EVERY_N_FRAMES > 0
    uint32_t frame = 0;
#endif

    while (true) {
#if CONFIG_TUTORIAL_0014_LOG_EVERY_N_FRAMES > 0
        frame++;
        if ((frame % CONFIG_TUTORIAL_0014_LOG_EVERY_N_FRAMES) == 0) {
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " free_heap=%" PRIu32 " dma_free=%" PRIu32,
                     frame,
                     esp_get_free_heap_size(),
                     (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));
        }
#endif

        int delay_ms = 0;
        if (s_gif.playFrame(false, &delay_ms, &ctx)) {
            present_canvas(canvas);

            // Some GIFs use 0 delay; keep the task yielding and prevent a tight loop.
            if (delay_ms < 10) {
                delay_ms = 10;
            }
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        } else {
            // End of file: restart.
            canvas.fillScreen(TFT_BLACK);
            s_gif.reset();
        }
    }
}


