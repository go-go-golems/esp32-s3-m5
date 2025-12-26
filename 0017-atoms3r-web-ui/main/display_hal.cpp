/*
 * Tutorial 0017: Display bring-up (AtomS3R GC9107) via M5GFX/LovyanGFX.
 */

#include "display_hal.h"

#include "sdkconfig.h"

#include "esp_log.h"

#include "M5GFX.h"
// Panel_GC9107 lives next to Panel_GC9A01 in LovyanGFX/M5GFX sources.
#include "lgfx/v1/panel/Panel_GC9A01.hpp"
#include "lgfx/v1/platforms/esp32/Bus_SPI.hpp"

static const char *TAG = "atoms3r_web_ui_0017";

// Fixed AtomS3R wiring.
static constexpr int PIN_LCD_CS = 14;
static constexpr int PIN_LCD_SCK = 15;
static constexpr int PIN_LCD_MOSI = 21;
static constexpr int PIN_LCD_DC = 42;
static constexpr int PIN_LCD_RST = 48;

static lgfx::Bus_SPI s_bus;
static lgfx::Panel_GC9107 s_panel;
static m5gfx::M5GFX s_display;

m5gfx::M5GFX &display_get(void) {
    return s_display;
}

bool display_init_m5gfx(void) {
    // Configure SPI bus
    {
        auto cfg = s_bus.config();
        cfg.spi_host = SPI3_HOST; // AtomS3(R) uses SPI3 in M5GFX
        cfg.spi_mode = 0;
        cfg.spi_3wire = true;
        cfg.freq_write = CONFIG_TUTORIAL_0017_LCD_SPI_PCLK_HZ;
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
        pcfg.panel_width = CONFIG_TUTORIAL_0017_LCD_HRES;
        pcfg.panel_height = CONFIG_TUTORIAL_0017_LCD_VRES;
        pcfg.offset_x = CONFIG_TUTORIAL_0017_LCD_X_OFFSET;
        pcfg.offset_y = CONFIG_TUTORIAL_0017_LCD_Y_OFFSET;
        pcfg.readable = false;
#if CONFIG_TUTORIAL_0017_LCD_INVERT
        pcfg.invert = true;
#else
        pcfg.invert = false;
#endif
#if CONFIG_TUTORIAL_0017_LCD_RGB_ORDER_RGB
        pcfg.rgb_order = true; // RGB
#else
        pcfg.rgb_order = false; // BGR
#endif
        pcfg.bus_shared = false;
        s_panel.config(pcfg);
    }

#if CONFIG_TUTORIAL_0017_LCD_INVERT
    const int invert = 1;
#else
    const int invert = 0;
#endif
#if CONFIG_TUTORIAL_0017_LCD_RGB_ORDER_RGB
    const char *rgb_order = "RGB";
#else
    const char *rgb_order = "BGR";
#endif

    ESP_LOGI(TAG, "m5gfx init: pclk=%dHz gap=(%d,%d) invert=%d rgb_order=%s",
             CONFIG_TUTORIAL_0017_LCD_SPI_PCLK_HZ,
             CONFIG_TUTORIAL_0017_LCD_X_OFFSET,
             CONFIG_TUTORIAL_0017_LCD_Y_OFFSET,
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

void display_present_canvas(M5Canvas &canvas) {
    canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0017_PRESENT_USE_DMA
    s_display.waitDMA();
#endif
}


