/*
 * ESP32-S3 tutorial 0010:
 * AtomS3R SPI LCD animation using M5GFX (LovyanGFX) with the M5Canvas (sprite) API.
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
#include <math.h>

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

static const char *TAG = "atoms3r_m5gfx_canvas";

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
#if CONFIG_TUTORIAL_0010_BACKLIGHT_I2C_ENABLE
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(
        BL_I2C_PORT,
        (uint8_t)CONFIG_TUTORIAL_0010_BL_I2C_ADDR,
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
#if CONFIG_TUTORIAL_0010_BACKLIGHT_I2C_ENABLE
    if (s_bl_i2c_inited) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "backlight i2c init: port=%d scl=%d sda=%d addr=0x%02x reg=0x%02x",
             (int)BL_I2C_PORT,
             CONFIG_TUTORIAL_0010_BL_I2C_SCL_GPIO,
             CONFIG_TUTORIAL_0010_BL_I2C_SDA_GPIO,
             (unsigned)CONFIG_TUTORIAL_0010_BL_I2C_ADDR,
             (unsigned)CONFIG_TUTORIAL_0010_BL_I2C_REG);

    // NOTE: We intentionally use the legacy I2C driver here because M5GFX/LovyanGFX
    // links against the legacy driver (`driver/i2c.h`). Mixing legacy + new driver_ng
    // aborts during startup via a constructor in ESP-IDF 5.x.
    //
    // If you migrate M5GFX to `driver/i2c_master.h`, you can switch this back too.
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.scl_io_num = (gpio_num_t)CONFIG_TUTORIAL_0010_BL_I2C_SCL_GPIO;
    conf.sda_io_num = (gpio_num_t)CONFIG_TUTORIAL_0010_BL_I2C_SDA_GPIO;
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
             (unsigned)CONFIG_TUTORIAL_0010_BL_I2C_ADDR);

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
#if CONFIG_TUTORIAL_0010_BACKLIGHT_I2C_ENABLE
    esp_err_t err = backlight_i2c_init();
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "backlight i2c set: reg=0x%02x value=%u",
             (unsigned)CONFIG_TUTORIAL_0010_BL_I2C_REG,
             (unsigned)brightness);

    err = backlight_i2c_write_reg_u8((uint8_t)CONFIG_TUTORIAL_0010_BL_I2C_REG, brightness);
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
#if CONFIG_TUTORIAL_0010_BACKLIGHT_GATE_ENABLE
    gpio_reset_pin((gpio_num_t)CONFIG_TUTORIAL_0010_BACKLIGHT_GATE_GPIO);
    gpio_set_direction((gpio_num_t)CONFIG_TUTORIAL_0010_BACKLIGHT_GATE_GPIO, GPIO_MODE_OUTPUT);

    // Kconfig bool may be undefined when disabled; use preprocessor guards.
#if CONFIG_TUTORIAL_0010_BACKLIGHT_GATE_ACTIVE_LOW
    const int level = on ? 0 : 1;
#else
    const int level = on ? 1 : 0;
#endif
    ESP_LOGI(TAG, "backlight gate gpio: pin=%d active_low=%d -> %s (level=%d)",
             CONFIG_TUTORIAL_0010_BACKLIGHT_GATE_GPIO,
             (int)(0
#if CONFIG_TUTORIAL_0010_BACKLIGHT_GATE_ACTIVE_LOW
                   + 1
#endif
                   ),
             on ? "ON" : "OFF",
             level);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0010_BACKLIGHT_GATE_GPIO, level);
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
#if CONFIG_TUTORIAL_0010_BACKLIGHT_I2C_ENABLE
    (void)backlight_i2c_set((uint8_t)CONFIG_TUTORIAL_0010_BL_BRIGHTNESS_ON);
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
        cfg.freq_write = CONFIG_TUTORIAL_0010_LCD_SPI_PCLK_HZ;
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
        pcfg.panel_width = CONFIG_TUTORIAL_0010_LCD_HRES;
        pcfg.panel_height = CONFIG_TUTORIAL_0010_LCD_VRES;
        pcfg.offset_x = CONFIG_TUTORIAL_0010_LCD_X_OFFSET;
        pcfg.offset_y = CONFIG_TUTORIAL_0010_LCD_Y_OFFSET;
        pcfg.readable = false;
#if CONFIG_TUTORIAL_0010_LCD_INVERT
        pcfg.invert = true;
#else
        pcfg.invert = false;
#endif
#if CONFIG_TUTORIAL_0010_LCD_RGB_ORDER_RGB
        pcfg.rgb_order = true; // RGB
#else
        pcfg.rgb_order = false; // BGR
#endif
        pcfg.bus_shared = false;
        s_panel.config(pcfg);
    }

    // Kconfig bools are often undefined when disabled; keep all logging compile-safe.
#if CONFIG_TUTORIAL_0010_LCD_INVERT
    const int invert = 1;
#else
    const int invert = 0;
#endif
#if CONFIG_TUTORIAL_0010_LCD_RGB_ORDER_RGB
    const char *rgb_order = "RGB";
#else
    const char *rgb_order = "BGR";
#endif

    ESP_LOGI(TAG, "m5gfx init: pclk=%dHz gap=(%d,%d) invert=%d rgb_order=%s",
             CONFIG_TUTORIAL_0010_LCD_SPI_PCLK_HZ,
             CONFIG_TUTORIAL_0010_LCD_X_OFFSET,
             CONFIG_TUTORIAL_0010_LCD_Y_OFFSET,
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

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline uint16_t hsv_to_rgb565(uint8_t h, uint8_t s, uint8_t v) {
    // Integer HSV->RGB adapted from common embedded implementations.
    // h: 0..255, s/v: 0..255
    if (s == 0) {
        return rgb565(v, v, v);
    }

    const uint8_t region = (uint8_t)(h / 43); // 0..5
    const uint8_t remainder = (uint8_t)((h - (region * 43)) * 6);

    const uint8_t p = (uint8_t)((v * (uint16_t)(255 - s)) >> 8);
    const uint8_t q = (uint8_t)((v * (uint16_t)(255 - ((s * (uint16_t)remainder) >> 8))) >> 8);
    const uint8_t t = (uint8_t)((v * (uint16_t)(255 - ((s * (uint16_t)(255 - remainder)) >> 8))) >> 8);

    uint8_t r = 0, g = 0, b = 0;
    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return rgb565(r, g, b);
}

static void build_plasma_tables(uint8_t sin8[256], uint16_t palette[256]) {
    constexpr float kTwoPi = 6.2831853071795864769f;
    for (int i = 0; i < 256; i++) {
        const float a = (float)i * kTwoPi / 256.0f;
        const float s = sinf(a);
        const int v = (int)((s + 1.0f) * 127.5f);
        sin8[i] = (uint8_t)((v < 0) ? 0 : (v > 255) ? 255 : v);

        // Hue wheel palette (full saturation/value).
        palette[i] = hsv_to_rgb565((uint8_t)i, 255, 255);
    }
}

static inline void draw_plasma(uint16_t *dst565, int w, int h, uint32_t t,
                               const uint8_t sin8[256], const uint16_t palette[256]) {
    // Classic plasma: mix multiple sine waves with different frequencies, speeds, and directions.
    // This creates organic "blobs" that flow independently rather than a static pattern scrolling.
    const uint8_t t1 = (uint8_t)(t & 0xFF);
    const uint8_t t2 = (uint8_t)((t >> 1) & 0xFF);
    const uint8_t t3 = (uint8_t)((t >> 2) & 0xFF);
    const uint8_t t4 = (uint8_t)((t * 3) & 0xFF);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Mix 4 sine waves with different spatial frequencies and time offsets
            const uint8_t v1 = sin8[(uint8_t)(x * 3 + t1)];
            const uint8_t v2 = sin8[(uint8_t)(y * 4 + t2)];
            const uint8_t v3 = sin8[(uint8_t)((x * 2 + y * 2) + t3)];
            const uint8_t v4 = sin8[(uint8_t)((x - y) * 2 + t4)];

            const uint16_t sum = (uint16_t)v1 + (uint16_t)v2 + (uint16_t)v3 + (uint16_t)v4;
            const uint8_t idx = (uint8_t)(sum >> 2); // average /4 -> 0..255
            dst565[y * w + x] = palette[idx];
        }
    }
}

extern "C" void app_main(void) {
    const int w = CONFIG_TUTORIAL_0010_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0010_LCD_VRES;

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
    vTaskDelay(pdMS_TO_TICKS(500));
    s_display.fillScreen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(500));
    s_display.fillScreen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(500));
    s_display.fillScreen(TFT_WHITE);
    vTaskDelay(pdMS_TO_TICKS(500));
    s_display.fillScreen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Offscreen canvas (LovyanGFX sprite). Keep it out of PSRAM by default:
    // - sprite buffers allocate using AllocationSource::Dma when PSRAM is disabled
    // - that is safest for SPI DMA full-frame presents.
    M5Canvas canvas(&s_display);
#if CONFIG_TUTORIAL_0010_CANVAS_USE_PSRAM
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

    uint8_t sin8[256] = {};
    uint16_t palette[256] = {};
    build_plasma_tables(sin8, palette);

    uint32_t t = 0;

#if CONFIG_TUTORIAL_0010_LOG_EVERY_N_FRAMES > 0
    uint32_t frame = 0;
    uint32_t last_tick = xTaskGetTickCount();
#endif

    ESP_LOGI(TAG, "starting canvas animation loop (frame_delay_ms=%d dma_present=%d psram=%d)",
             CONFIG_TUTORIAL_0010_FRAME_DELAY_MS,
             (int)(0
#if CONFIG_TUTORIAL_0010_PRESENT_USE_DMA
                   + 1
#endif
                   ),
             (int)(0
#if CONFIG_TUTORIAL_0010_CANVAS_USE_PSRAM
                   + 1
#endif
                   ));
    while (true) {
        auto *dst = (uint16_t *)canvas.getBuffer();
        draw_plasma(dst, w, h, t, sin8, palette);

#if CONFIG_TUTORIAL_0010_PRESENT_USE_DMA
        // Match the UserDemo idiom (pushSprite), but keep an explicit DMA flush for full-rate animation.
        // pushSprite internally selects DMA based on the sprite allocation source (PSRAM disables DMA).
        canvas.pushSprite(0, 0);
        s_display.waitDMA();
#else
        // UserDemo style: push the sprite to the parent display.
        canvas.pushSprite(0, 0);
#endif

        t += 2;
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0010_FRAME_DELAY_MS));

#if CONFIG_TUTORIAL_0010_LOG_EVERY_N_FRAMES > 0
        frame++;
        if ((frame % (uint32_t)CONFIG_TUTORIAL_0010_LOG_EVERY_N_FRAMES) == 0) {
            uint32_t now = xTaskGetTickCount();
            uint32_t dt_ms = (now - last_tick) * portTICK_PERIOD_MS;
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " dt=%" PRIu32 "ms free_heap=%" PRIu32,
                     frame, dt_ms, esp_get_free_heap_size());
        }
#endif
    }
}


