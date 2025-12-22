/*
 * ESP32-S3 tutorial 0009:
 * AtomS3R SPI LCD animation, but using M5GFX (LovyanGFX) instead of esp_lcd.
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
#include "driver/i2c_master.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "M5GFX.h"
// Panel_GC9107 lives next to Panel_GC9A01 in LovyanGFX/M5GFX sources.
#include "lgfx/v1/panel/Panel_GC9A01.hpp"
#include "lgfx/v1/platforms/esp32/Bus_SPI.hpp"

static const char *TAG = "atoms3r_m5gfx";

// Fixed AtomS3R wiring.
static constexpr int PIN_LCD_CS = 14;
static constexpr int PIN_LCD_SCK = 15;
static constexpr int PIN_LCD_MOSI = 21;
static constexpr int PIN_LCD_DC = 42;
static constexpr int PIN_LCD_RST = 48;

// -------------------------
// Backlight helpers (optional)
// -------------------------

static i2c_master_bus_handle_t s_bl_bus = nullptr;
static i2c_master_dev_handle_t s_bl_dev = nullptr;

static esp_err_t backlight_i2c_init(void) {
#if CONFIG_TUTORIAL_0009_BACKLIGHT_I2C_ENABLE
    if (s_bl_bus && s_bl_dev) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "backlight i2c init: port=%d scl=%d sda=%d addr=0x%02x reg=0x%02x",
             (int)I2C_NUM_0,
             CONFIG_TUTORIAL_0009_BL_I2C_SCL_GPIO,
             CONFIG_TUTORIAL_0009_BL_I2C_SDA_GPIO,
             (unsigned)CONFIG_TUTORIAL_0009_BL_I2C_ADDR,
             (unsigned)CONFIG_TUTORIAL_0009_BL_I2C_REG);

    if (!s_bl_bus) {
        i2c_master_bus_config_t bus_cfg = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = I2C_NUM_0,
            .scl_io_num = CONFIG_TUTORIAL_0009_BL_I2C_SCL_GPIO,
            .sda_io_num = CONFIG_TUTORIAL_0009_BL_I2C_SDA_GPIO,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bl_bus);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    if (!s_bl_dev) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = CONFIG_TUTORIAL_0009_BL_I2C_ADDR,
            .scl_speed_hz = 400000,
        };
        esp_err_t err = i2c_master_bus_add_device(s_bl_bus, &dev_cfg, &s_bl_dev);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_master_bus_add_device failed (addr=0x%02x): %s",
                     (unsigned)CONFIG_TUTORIAL_0009_BL_I2C_ADDR,
                     esp_err_to_name(err));
            return err;
        }
    }

    return ESP_OK;
#else
    return ESP_OK;
#endif
}

static esp_err_t backlight_i2c_set(uint8_t brightness) {
#if CONFIG_TUTORIAL_0009_BACKLIGHT_I2C_ENABLE
    esp_err_t err = backlight_i2c_init();
    if (err != ESP_OK) {
        return err;
    }
    if (!s_bl_dev) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "backlight i2c set: reg=0x%02x value=%u",
             (unsigned)CONFIG_TUTORIAL_0009_BL_I2C_REG,
             (unsigned)brightness);

    uint8_t buf[2] = {(uint8_t)CONFIG_TUTORIAL_0009_BL_I2C_REG, brightness};
    err = i2c_master_transmit(s_bl_dev, buf, sizeof(buf), 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_transmit failed: %s", esp_err_to_name(err));
    }
    return err;
#else
    (void)brightness;
    return ESP_OK;
#endif
}

static void backlight_gate_gpio_set(bool on) {
#if CONFIG_TUTORIAL_0009_BACKLIGHT_GATE_ENABLE
    gpio_reset_pin((gpio_num_t)CONFIG_TUTORIAL_0009_BACKLIGHT_GATE_GPIO);
    gpio_set_direction((gpio_num_t)CONFIG_TUTORIAL_0009_BACKLIGHT_GATE_GPIO, GPIO_MODE_OUTPUT);

    // Kconfig bool may be undefined when disabled; use preprocessor guards.
#if CONFIG_TUTORIAL_0009_BACKLIGHT_GATE_ACTIVE_LOW
    const int level = on ? 0 : 1;
#else
    const int level = on ? 1 : 0;
#endif
    ESP_LOGI(TAG, "backlight gate gpio: pin=%d active_low=%d -> %s (level=%d)",
             CONFIG_TUTORIAL_0009_BACKLIGHT_GATE_GPIO,
             (int)(0
#if CONFIG_TUTORIAL_0009_BACKLIGHT_GATE_ACTIVE_LOW
                   + 1
#endif
                   ),
             on ? "ON" : "OFF",
             level);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0009_BACKLIGHT_GATE_GPIO, level);
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
#if CONFIG_TUTORIAL_0009_BACKLIGHT_I2C_ENABLE
    (void)backlight_i2c_set((uint8_t)CONFIG_TUTORIAL_0009_BL_BRIGHTNESS_ON);
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
        cfg.freq_write = CONFIG_TUTORIAL_0009_LCD_SPI_PCLK_HZ;
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
        pcfg.panel_width = CONFIG_TUTORIAL_0009_LCD_HRES;
        pcfg.panel_height = CONFIG_TUTORIAL_0009_LCD_VRES;
        pcfg.offset_x = CONFIG_TUTORIAL_0009_LCD_X_OFFSET;
        pcfg.offset_y = CONFIG_TUTORIAL_0009_LCD_Y_OFFSET;
        pcfg.readable = false;
#if CONFIG_TUTORIAL_0009_LCD_INVERT
        pcfg.invert = true;
#else
        pcfg.invert = false;
#endif
#if CONFIG_TUTORIAL_0009_LCD_RGB_ORDER_RGB
        pcfg.rgb_order = true; // RGB
#else
        pcfg.rgb_order = false; // BGR
#endif
        pcfg.bus_shared = false;
        s_panel.config(pcfg);
    }

    // Kconfig bools are often undefined when disabled; keep all logging compile-safe.
#if CONFIG_TUTORIAL_0009_LCD_INVERT
    const int invert = 1;
#else
    const int invert = 0;
#endif
#if CONFIG_TUTORIAL_0009_LCD_RGB_ORDER_RGB
    const char *rgb_order = "RGB";
#else
    const char *rgb_order = "BGR";
#endif

    ESP_LOGI(TAG, "m5gfx init: pclk=%dHz gap=(%d,%d) invert=%d rgb_order=%s",
             CONFIG_TUTORIAL_0009_LCD_SPI_PCLK_HZ,
             CONFIG_TUTORIAL_0009_LCD_X_OFFSET,
             CONFIG_TUTORIAL_0009_LCD_Y_OFFSET,
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

static inline void draw_frame(lgfx::rgb565_t *fb, int w, int h, int t) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t r = (uint8_t)((x * 2 + t) & 0xFF);
            uint8_t g = (uint8_t)((y * 2 + (t >> 1)) & 0xFF);
            uint8_t b = (uint8_t)(((x + y) * 2 + (t >> 2)) & 0xFF);
            fb[y * w + x].set(r, g, b);
        }
    }
}

extern "C" void app_main(void) {
    const int w = CONFIG_TUTORIAL_0009_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0009_LCD_VRES;

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

    const size_t fb_bytes = (size_t)w * (size_t)h * sizeof(lgfx::rgb565_t);
    auto *fb = (lgfx::rgb565_t *)heap_caps_malloc(fb_bytes, MALLOC_CAP_DEFAULT);
    if (!fb) {
        ESP_LOGE(TAG, "framebuffer alloc failed: %" PRIu32 " bytes", (uint32_t)fb_bytes);
        return;
    }
    ESP_LOGI(TAG, "framebuffer ok: %" PRIu32 " bytes; free_heap=%" PRIu32,
             (uint32_t)fb_bytes, esp_get_free_heap_size());

    int t = 0;

#if CONFIG_TUTORIAL_0009_LOG_EVERY_N_FRAMES > 0
    uint32_t frame = 0;
    uint32_t last_tick = xTaskGetTickCount();
#endif

    while (true) {
        draw_frame(fb, w, h, t);
        // Full-frame blit
        s_display.pushImage(0, 0, w, h, fb);
        t += 3;
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0009_FRAME_DELAY_MS));

#if CONFIG_TUTORIAL_0009_LOG_EVERY_N_FRAMES > 0
        frame++;
        if ((frame % (uint32_t)CONFIG_TUTORIAL_0009_LOG_EVERY_N_FRAMES) == 0) {
            uint32_t now = xTaskGetTickCount();
            uint32_t dt_ms = (now - last_tick) * portTICK_PERIOD_MS;
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " dt=%" PRIu32 "ms free_heap=%" PRIu32,
                     frame, dt_ms, esp_get_free_heap_size());
        }
#endif
    }
}


