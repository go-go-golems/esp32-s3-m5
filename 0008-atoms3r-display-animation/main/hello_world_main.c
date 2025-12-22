/*
 * ESP32-S3 tutorial 0008:
 * AtomS3R SPI LCD bring-up (esp_lcd) + simple looping animation.
 *
 * AtomS3R display wiring (M5GFX deep dive / schematic):
 * - DISP_CS   = GPIO14
 * - SPI_SCK   = GPIO15
 * - SPI_MOSI  = GPIO21
 * - DISP_RS/DC= GPIO42
 * - DISP_RST  = GPIO48
 *
 * Panel/controller:
 * - AtomS3R uses GC9107 (128x160 RAM with visible 128x128 window at y-offset 32).
 *
 * Backlight:
 * - Default is I2C-controlled brightness device (addr 0x30, reg 0x0e) per M5GFX.
 * - A GPIO fallback is provided via menuconfig for hardware revisions that gate LED_BL via GPIO.
 */

#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_lcd_gc9107.h"

static const char *TAG = "atoms3r_anim";

// Fixed ATOMS3R wiring from schematic.
#define PIN_LCD_CS        14
#define PIN_LCD_SCK       15
#define PIN_LCD_MOSI      21
#define PIN_LCD_DC        42
#define PIN_LCD_RST       48

#define LCD_SPI_HOST      SPI3_HOST

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    // 5-6-5
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// M5GFX GC9107 init sequence (from LovyanGFX / M5GFX `Panel_GC9107`).
// Note: esp_lcd_gc9107 sends SLPOUT + MADCTL + COLMOD before running vendor init cmds,
// so we keep this list vendor-focused (no 0x36/0x3A here).
static const gc9107_lcd_init_cmd_t s_gc9107_init_cmds_m5gfx[] = {
    {0xFE, NULL, 0, 5},
    {0xEF, NULL, 0, 5},
    {0xB0, (uint8_t[]){0xC0}, 1, 0},
    {0xB2, (uint8_t[]){0x2F}, 1, 0},
    {0xB3, (uint8_t[]){0x03}, 1, 0},
    {0xB6, (uint8_t[]){0x19}, 1, 0},
    {0xB7, (uint8_t[]){0x01}, 1, 0},
    {0xAC, (uint8_t[]){0xCB}, 1, 0},
    {0xAB, (uint8_t[]){0x0E}, 1, 0},
    {0xB4, (uint8_t[]){0x04}, 1, 0},
    {0xA8, (uint8_t[]){0x19}, 1, 0},
    {0xB8, (uint8_t[]){0x08}, 1, 0},
    {0xE8, (uint8_t[]){0x24}, 1, 0},
    {0xE9, (uint8_t[]){0x48}, 1, 0},
    {0xEA, (uint8_t[]){0x22}, 1, 0},
    {0xC6, (uint8_t[]){0x30}, 1, 0},
    {0xC7, (uint8_t[]){0x18}, 1, 0},
    {0xF0, (uint8_t[]){0x01, 0x2b, 0x23, 0x3c, 0xb7, 0x12, 0x17, 0x60, 0x00, 0x06, 0x0c, 0x17, 0x12, 0x1f}, 14, 0},
    {0xF1, (uint8_t[]){0x05, 0x2e, 0x2d, 0x44, 0xd6, 0x15, 0x17, 0xa0, 0x02, 0x0d, 0x0d, 0x1a, 0x18, 0x1f}, 14, 0},
};

// Backlight (AtomS3R): default is an I2C-controlled brightness device.
static i2c_master_bus_handle_t s_bl_i2c_bus = NULL;
static i2c_master_dev_handle_t s_bl_i2c_dev = NULL;

#if CONFIG_TUTORIAL_0008_BACKLIGHT_I2C && CONFIG_TUTORIAL_0008_BL_I2C_USE_GATE_GPIO
static void backlight_gate_gpio_init(void) {
    gpio_reset_pin((gpio_num_t)CONFIG_TUTORIAL_0008_BL_I2C_GATE_GPIO);
    gpio_set_direction((gpio_num_t)CONFIG_TUTORIAL_0008_BL_I2C_GATE_GPIO, GPIO_MODE_OUTPUT);
}

static void backlight_gate_gpio_set(bool on) {
    backlight_gate_gpio_init();
    int level = on ? (CONFIG_TUTORIAL_0008_BL_I2C_GATE_ACTIVE_LOW ? 0 : 1)
                   : (CONFIG_TUTORIAL_0008_BL_I2C_GATE_ACTIVE_LOW ? 1 : 0);
    ESP_LOGI(TAG, "backlight gate gpio: pin=%d active_low=%d -> %s (level=%d)",
             CONFIG_TUTORIAL_0008_BL_I2C_GATE_GPIO,
             (int)CONFIG_TUTORIAL_0008_BL_I2C_GATE_ACTIVE_LOW,
             on ? "ON" : "OFF",
             level);
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0008_BL_I2C_GATE_GPIO, level);
}
#else
static __attribute__((unused)) void backlight_gate_gpio_set(bool on) { (void)on; }
#endif

static esp_err_t backlight_i2c_init(void) {
    // If both handles are valid, we're already initialized.
    if (s_bl_i2c_bus != NULL && s_bl_i2c_dev != NULL) {
        return ESP_OK;
    }

    ESP_LOGI(TAG,
             "backlight i2c init: port=%d scl=%d sda=%d addr=0x%02x reg=0x%02x",
             (int)I2C_NUM_0,
             CONFIG_TUTORIAL_0008_BL_I2C_SCL_GPIO,
             CONFIG_TUTORIAL_0008_BL_I2C_SDA_GPIO,
             (unsigned)CONFIG_TUTORIAL_0008_BL_I2C_ADDR,
             (unsigned)CONFIG_TUTORIAL_0008_BL_I2C_REG);

    // Create I2C bus if it doesn't exist yet.
    if (s_bl_i2c_bus == NULL) {
        i2c_master_bus_config_t bus_cfg = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = I2C_NUM_0,
            .scl_io_num = CONFIG_TUTORIAL_0008_BL_I2C_SCL_GPIO,
            .sda_io_num = CONFIG_TUTORIAL_0008_BL_I2C_SDA_GPIO,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };

        esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bl_i2c_bus);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    // Add device to bus if it doesn't exist yet.
    if (s_bl_i2c_dev == NULL) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = CONFIG_TUTORIAL_0008_BL_I2C_ADDR,
            .scl_speed_hz = 400000,
        };
        esp_err_t err = i2c_master_bus_add_device(s_bl_i2c_bus, &dev_cfg, &s_bl_i2c_dev);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_master_bus_add_device failed (addr=0x%02x): %s",
                     (unsigned)CONFIG_TUTORIAL_0008_BL_I2C_ADDR,
                     esp_err_to_name(err));
            return err;
        }
    }

    return ESP_OK;
}

static esp_err_t backlight_i2c_set(uint8_t brightness) {
    esp_err_t err = backlight_i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight_i2c_init failed: %s", esp_err_to_name(err));
        return err;
    }

    if (s_bl_i2c_dev == NULL) {
        ESP_LOGE(TAG, "backlight I2C device handle is NULL");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "backlight i2c set: reg=0x%02x value=%u",
             (unsigned)CONFIG_TUTORIAL_0008_BL_I2C_REG,
             (unsigned)brightness);

    uint8_t buf[2] = {
        (uint8_t)CONFIG_TUTORIAL_0008_BL_I2C_REG,
        brightness,
    };
    err = i2c_master_transmit(s_bl_i2c_dev, buf, sizeof(buf), 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_transmit failed: %s", esp_err_to_name(err));
    }
    return err;
}

// GPIO backlight fallback is only compiled when enabled (CONFIG symbol may not exist otherwise).
#if CONFIG_TUTORIAL_0008_BACKLIGHT_GPIO
static void backlight_gpio_init(void) {
    gpio_reset_pin((gpio_num_t)CONFIG_TUTORIAL_0008_BL_GPIO);
    gpio_set_direction((gpio_num_t)CONFIG_TUTORIAL_0008_BL_GPIO, GPIO_MODE_OUTPUT);
}

static void backlight_gpio_set(bool on) {
    backlight_gpio_init();
#if CONFIG_TUTORIAL_0008_BL_GPIO_ACTIVE_LOW
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0008_BL_GPIO, on ? 0 : 1);
#else
    gpio_set_level((gpio_num_t)CONFIG_TUTORIAL_0008_BL_GPIO, on ? 1 : 0);
#endif
}
#else
static __attribute__((unused)) void backlight_gpio_set(bool on) { (void)on; }
#endif

static void backlight_prepare_for_init(void) {
#if CONFIG_TUTORIAL_0008_BACKLIGHT_I2C
    // 0 brightness during init
    ESP_LOGI(TAG, "backlight mode: I2C (init->brightness=0)");
    backlight_gate_gpio_set(false);
    esp_err_t err = backlight_i2c_set(0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "backlight i2c init/off failed: %s", esp_err_to_name(err));
    }
#elif CONFIG_TUTORIAL_0008_BACKLIGHT_GPIO
    ESP_LOGI(TAG, "backlight mode: GPIO (init->off) gpio=%d active_low=%d",
             CONFIG_TUTORIAL_0008_BL_GPIO,
             (int)CONFIG_TUTORIAL_0008_BL_GPIO_ACTIVE_LOW);
    backlight_gpio_set(false);
#else
    ESP_LOGI(TAG, "backlight mode: none");
    // none
#endif
}

static void backlight_enable_after_init(void) {
#if CONFIG_TUTORIAL_0008_BACKLIGHT_I2C
    ESP_LOGI(TAG, "backlight enable: I2C brightness=%d", CONFIG_TUTORIAL_0008_BL_BRIGHTNESS_ON);
    backlight_gate_gpio_set(true);
    esp_err_t err = backlight_i2c_set((uint8_t)CONFIG_TUTORIAL_0008_BL_BRIGHTNESS_ON);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "backlight i2c set failed: %s", esp_err_to_name(err));
    }
#elif CONFIG_TUTORIAL_0008_BACKLIGHT_GPIO
    ESP_LOGI(TAG, "backlight enable: GPIO");
    backlight_gpio_set(true);
#else
    // none
#endif
}

static esp_lcd_panel_handle_t lcd_init(void) {
    const int hres = CONFIG_TUTORIAL_0008_LCD_HRES;
    const int vres = CONFIG_TUTORIAL_0008_LCD_VRES;

    // Keep backlight off during init to avoid flashing garbage.
    backlight_prepare_for_init();

    ESP_LOGI(TAG, "spi bus init: host=%d sclk=%d mosi=%d cs=%d dc=%d rst=%d pclk=%dHz",
             (int)LCD_SPI_HOST,
             PIN_LCD_SCK, PIN_LCD_MOSI, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST,
             CONFIG_TUTORIAL_0008_LCD_SPI_PCLK_HZ);

    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = hres * vres * 2 + 16,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = CONFIG_TUTORIAL_0008_LCD_SPI_PCLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle));

#if CONFIG_TUTORIAL_0008_LCD_COLOR_SPACE_BGR
    const lcd_rgb_element_order_t rgb_order = LCD_RGB_ELEMENT_ORDER_BGR;
#else
    const lcd_rgb_element_order_t rgb_order = LCD_RGB_ELEMENT_ORDER_RGB;
#endif

#if CONFIG_TUTORIAL_0008_LCD_INVERT_COLOR
    const bool invert = true;
#else
    const bool invert = false;
#endif

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = rgb_order,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
    };

    // Optionally use a vendor init sequence matching M5GFX's GC9107 init.
#if CONFIG_TUTORIAL_0008_USE_M5GFX_INIT_CMDS
    gc9107_vendor_config_t vendor_config = {
        .init_cmds = s_gc9107_init_cmds_m5gfx,
        .init_cmds_size = sizeof(s_gc9107_init_cmds_m5gfx) / sizeof(s_gc9107_init_cmds_m5gfx[0]),
    };
    panel_config.vendor_config = &vendor_config;
    ESP_LOGI(TAG, "gc9107 init profile: M5GFX (vendor init cmds)");
#else
    ESP_LOGI(TAG, "gc9107 init profile: esp_lcd default (vendor init cmds)");
#endif

    // AtomS3R panel controller: GC9107 (via espressif/esp_lcd_gc9107 component).
    ESP_LOGI(TAG, "panel create: gc9107 bits_per_pixel=%u rgb_order=%s",
             (unsigned)panel_config.bits_per_pixel,
             (rgb_order == LCD_RGB_ELEMENT_ORDER_BGR) ? "BGR" : "RGB");
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9107(io_handle, &panel_config, &panel_handle));

    ESP_LOGI(TAG, "panel reset/init...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_LOGI(TAG, "panel gap: x=%d y=%d", CONFIG_TUTORIAL_0008_LCD_X_OFFSET, CONFIG_TUTORIAL_0008_LCD_Y_OFFSET);
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, CONFIG_TUTORIAL_0008_LCD_X_OFFSET, CONFIG_TUTORIAL_0008_LCD_Y_OFFSET));
    ESP_LOGI(TAG, "panel invert: %d", (int)invert);
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, invert));
    ESP_LOGI(TAG, "panel display on");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    backlight_enable_after_init();

    ESP_LOGI(TAG, "lcd init ok (%dx%d), pclk=%dHz, gap=(%d,%d), colorspace=%s",
             hres, vres,
             CONFIG_TUTORIAL_0008_LCD_SPI_PCLK_HZ,
             CONFIG_TUTORIAL_0008_LCD_X_OFFSET,
             CONFIG_TUTORIAL_0008_LCD_Y_OFFSET,
             rgb_order == LCD_RGB_ELEMENT_ORDER_BGR ? "BGR" : "RGB");
    ESP_LOGI(TAG, "rgb565 byteswap: %d (SPI LCDs are typically big-endian)", (int)CONFIG_TUTORIAL_0008_SWAP_RGB565_BYTES);
#if CONFIG_TUTORIAL_0008_BACKLIGHT_I2C && CONFIG_TUTORIAL_0008_BL_I2C_USE_GATE_GPIO
    ESP_LOGI(TAG, "backlight i2c gate gpio: enabled=1 pin=%d active_low=%d",
             (int)CONFIG_TUTORIAL_0008_BL_I2C_GATE_GPIO,
             (int)CONFIG_TUTORIAL_0008_BL_I2C_GATE_ACTIVE_LOW);
#elif CONFIG_TUTORIAL_0008_BACKLIGHT_I2C
    ESP_LOGI(TAG, "backlight i2c gate gpio: enabled=0");
#endif

    return panel_handle;
}

static void draw_frame(uint16_t *fb, int w, int h, int t) {
    // Simple animated plasma-ish gradient (cheap integer math).
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t r = (uint8_t)((x * 2 + t) & 0xFF);
            uint8_t g = (uint8_t)((y * 2 + (t >> 1)) & 0xFF);
            uint8_t b = (uint8_t)(((x + y) * 2 + (t >> 2)) & 0xFF);
            uint16_t px = rgb565(r, g, b);
#if CONFIG_TUTORIAL_0008_SWAP_RGB565_BYTES
            // SPI LCD expects MSB-first (big-endian) byte order for RGB565.
            // Our fb is uint16_t on a little-endian CPU, so swap bytes.
            px = __builtin_bswap16(px);
#endif
            fb[y * w + x] = px;
        }
    }
}

void app_main(void) {
    const int w = CONFIG_TUTORIAL_0008_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0008_LCD_VRES;

    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "bringing up lcd...");
    esp_lcd_panel_handle_t panel = lcd_init();

    // DMA-capable framebuffer (SPI DMA).
    uint32_t fb_bytes = (uint32_t)w * (uint32_t)h * (uint32_t)sizeof(uint16_t);
    uint16_t *fb = (uint16_t *)heap_caps_malloc(fb_bytes, MALLOC_CAP_DMA);
    if (fb == NULL) {
        ESP_LOGE(TAG, "failed to allocate framebuffer (%" PRIu32 " bytes)", fb_bytes);
        return;
    }
    ESP_LOGI(TAG, "framebuffer ok: %" PRIu32 " bytes; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             fb_bytes,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    int t = 0;

#if CONFIG_TUTORIAL_0008_LOG_EVERY_N_FRAMES > 0
    uint32_t frame = 0;
    uint32_t last_tick = xTaskGetTickCount();
#endif
    while (1) {
        draw_frame(fb, w, h, t);
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, w, h, fb));
        t += 3;

#if CONFIG_TUTORIAL_0008_LOG_EVERY_N_FRAMES > 0
        frame++;
        if ((frame % (uint32_t)CONFIG_TUTORIAL_0008_LOG_EVERY_N_FRAMES) == 0) {
            uint32_t now = xTaskGetTickCount();
            uint32_t dt_ms = (now - last_tick) * portTICK_PERIOD_MS;
            last_tick = now;
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " dt=%" PRIu32 "ms free_heap=%" PRIu32 " dma_free=%" PRIu32,
                     frame, dt_ms,
                     esp_get_free_heap_size(),
                     (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0008_FRAME_DELAY_MS));
    }
}
