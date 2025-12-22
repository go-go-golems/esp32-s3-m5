/*
 * ESP32-S3 tutorial 0008:
 * ATOMS3R SPI LCD bring-up (esp_lcd) + simple looping animation.
 *
 * Pin mapping (ATOMS3R schematic + summary doc):
 * - DISP_CS   = GPIO14
 * - SPI_SCK   = GPIO15
 * - SPI_MOSI  = GPIO21
 * - DISP_RS/DC= GPIO42
 * - DISP_RST  = GPIO48
 * - LED_BL    = GPIO7 (active-low, via PMOS)
 *
 * Panel/controller:
 * - Likely ST7789 128x128 (controller not explicitly named in the schematic).
 * - If colors are wrong, toggle BGR/RGB in menuconfig.
 * - If pixels are shifted, set x/y offsets in menuconfig.
 */

#include <stdint.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

static const char *TAG = "atoms3r_anim";

// Fixed ATOMS3R wiring from schematic.
#define PIN_LCD_CS        14
#define PIN_LCD_SCK       15
#define PIN_LCD_MOSI      21
#define PIN_LCD_DC        42
#define PIN_LCD_RST       48
#define PIN_LCD_BL        7   // active-low

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    // 5-6-5
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void backlight_on(void) {
    gpio_reset_pin((gpio_num_t)PIN_LCD_BL);
    gpio_set_direction((gpio_num_t)PIN_LCD_BL, GPIO_MODE_OUTPUT);
    // Active-low (PMOS): drive low to enable.
    gpio_set_level((gpio_num_t)PIN_LCD_BL, 0);
}

static void backlight_off(void) {
    gpio_reset_pin((gpio_num_t)PIN_LCD_BL);
    gpio_set_direction((gpio_num_t)PIN_LCD_BL, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PIN_LCD_BL, 1);
}

static esp_lcd_panel_handle_t lcd_init(void) {
    const int hres = CONFIG_TUTORIAL_0008_LCD_HRES;
    const int vres = CONFIG_TUTORIAL_0008_LCD_VRES;

    // Keep backlight off during init to avoid flashing garbage.
    backlight_off();

    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = hres * vres * 2 + 16,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

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
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

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

    // NOTE: If the actual controller differs, this call will need to change.
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, CONFIG_TUTORIAL_0008_LCD_X_OFFSET, CONFIG_TUTORIAL_0008_LCD_Y_OFFSET));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, invert));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    backlight_on();

    ESP_LOGI(TAG, "lcd init ok (%dx%d), pclk=%dHz, gap=(%d,%d), colorspace=%s",
             hres, vres,
             CONFIG_TUTORIAL_0008_LCD_SPI_PCLK_HZ,
             CONFIG_TUTORIAL_0008_LCD_X_OFFSET,
             CONFIG_TUTORIAL_0008_LCD_Y_OFFSET,
             rgb_order == LCD_RGB_ELEMENT_ORDER_BGR ? "BGR" : "RGB");

    return panel_handle;
}

static void draw_frame(uint16_t *fb, int w, int h, int t) {
    // Simple animated plasma-ish gradient (cheap integer math).
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t r = (uint8_t)((x * 2 + t) & 0xFF);
            uint8_t g = (uint8_t)((y * 2 + (t >> 1)) & 0xFF);
            uint8_t b = (uint8_t)(((x + y) * 2 + (t >> 2)) & 0xFF);
            fb[y * w + x] = rgb565(r, g, b);
        }
    }
}

void app_main(void) {
    const int w = CONFIG_TUTORIAL_0008_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0008_LCD_VRES;

    ESP_LOGI(TAG, "boot; bringing up lcd");
    esp_lcd_panel_handle_t panel = lcd_init();

    // DMA-capable framebuffer (SPI DMA).
    size_t fb_bytes = (size_t)w * (size_t)h * sizeof(uint16_t);
    uint16_t *fb = (uint16_t *)heap_caps_malloc(fb_bytes, MALLOC_CAP_DMA);
    if (fb == NULL) {
        ESP_LOGE(TAG, "failed to allocate framebuffer (%u bytes)", (unsigned)fb_bytes);
        return;
    }

    int t = 0;
    while (1) {
        draw_frame(fb, w, h, t);
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, w, h, fb));
        t += 3;
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0008_FRAME_DELAY_MS));
    }
}
