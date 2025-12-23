/*
 * ESP32-S3 tutorial 0013:
 * AtomS3R “GIF console” MVP (mock animations + button + esp_console).
 *
 * This chapter focuses on the control plane first. Real GIF decoding and flash
 * asset bundling come later.
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

static const char *TAG = "atoms3r_gif_console_mock";

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

// -------------------------
// Mock animations (Phase A)
// -------------------------

struct MockAnim {
    const char *name;
    void (*render)(M5Canvas &canvas, uint32_t frame, uint32_t t_ms);
};

static void anim_solid(M5Canvas &canvas, uint16_t color565) {
    canvas.fillScreen(color565);
}

static void render_red(M5Canvas &canvas, uint32_t, uint32_t) { anim_solid(canvas, TFT_RED); }
static void render_green(M5Canvas &canvas, uint32_t, uint32_t) { anim_solid(canvas, TFT_GREEN); }
static void render_blue(M5Canvas &canvas, uint32_t, uint32_t) { anim_solid(canvas, TFT_BLUE); }

static void render_bounce_square(M5Canvas &canvas, uint32_t frame, uint32_t) {
    const int w = canvas.width();
    const int h = canvas.height();
    canvas.fillScreen(TFT_BLACK);

    const int size = (w < h ? w : h) / 4;
    const int period_x = (w - size) > 0 ? (w - size) : 1;
    const int period_y = (h - size) > 0 ? (h - size) : 1;
    const int x = (int)((frame * 4) % (uint32_t)period_x);
    const int y = (int)((frame * 3) % (uint32_t)period_y);
    canvas.fillRect(x, y, size, size, TFT_YELLOW);
}

static void render_checker(M5Canvas &canvas, uint32_t frame, uint32_t) {
    const int w = canvas.width();
    const int h = canvas.height();
    const int cell = 16;
    const bool phase = ((frame / 5) % 2) != 0;
    for (int y = 0; y < h; y += cell) {
        for (int x = 0; x < w; x += cell) {
            const bool on = ((((x / cell) + (y / cell)) % 2) != 0) ^ phase;
            canvas.fillRect(x, y, cell, cell, on ? TFT_WHITE : TFT_DARKGREY);
        }
    }
}

static const MockAnim kAnims[] = {
    {"red", render_red},
    {"green", render_green},
    {"blue", render_blue},
    {"bounce", render_bounce_square},
    {"checker", render_checker},
};

static constexpr int kAnimCount = (int)(sizeof(kAnims) / sizeof(kAnims[0]));

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

static int find_anim_index_by_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < kAnimCount; i++) {
        if (strcasecmp(name, kAnims[i].name) == 0) return i;
    }
    return -1;
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

static int cmd_list(int, char **) {
    printf("mock animations (%d):\n", kAnimCount);
    for (int i = 0; i < kAnimCount; i++) {
        printf("  %d: %s\n", i, kAnims[i].name);
    }
    return 0;
}

static int cmd_play(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: play <id|name>\n");
        return 1;
    }
    int idx = -1;
    if (try_parse_int(argv[1], &idx)) {
        // ok
    } else {
        idx = find_anim_index_by_name(argv[1]);
    }
    if (idx < 0 || idx >= kAnimCount) {
        printf("invalid animation: %s\n", argv[1]);
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
    cmd.help = "List available mock animations";
    cmd.func = &cmd_list;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "play";
    cmd.help = "Play animation by id or name: play <id|name>";
    cmd.func = &cmd_play;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "stop";
    cmd.help = "Stop playback";
    cmd.func = &cmd_stop;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "next";
    cmd.help = "Select next animation and play";
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

static void console_start_usb_serial_jtag(void) {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_repl_t *repl = nullptr;

    // Route stdin/stdout to USB Serial/JTAG.
    usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CRLF);
    usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
    usb_serial_jtag_vfs_use_driver();

    esp_console_config_t console_config = ESP_CONSOLE_CONFIG_DEFAULT();
    console_config.max_cmdline_length = 256;
    console_config.max_cmdline_args = 8;
    ESP_ERROR_CHECK(esp_console_init(&console_config));
    esp_console_register_help_command();

    console_register_commands();

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "gif> ";

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    ESP_LOGI(TAG, "esp_console started over USB Serial/JTAG");
#else
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no console started");
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
    console_start_usb_serial_jtag();

    bool playing = true;
    int anim_idx = 0;
    uint32_t frame = 0;
    uint32_t last_next_ms = 0;

    ESP_LOGI(TAG, "starting mock playback loop (frame_delay_ms=%d dma_present=%d psram=%d)",
             CONFIG_TUTORIAL_0013_FRAME_DELAY_MS,
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

    int64_t next_frame_us = esp_timer_get_time();
    while (true) {
        const int64_t now_us = esp_timer_get_time();
        const uint32_t now_ms = (uint32_t)(now_us / 1000);

        TickType_t wait_ticks = portMAX_DELAY;
        if (playing) {
            int64_t dt_us = next_frame_us - now_us;
            if (dt_us < 0) dt_us = 0;
            wait_ticks = pdMS_TO_TICKS((uint32_t)(dt_us / 1000));
        }

        CtrlEvent ev = {};
        if (xQueueReceive(s_ctrl_q, &ev, wait_ticks) == pdTRUE) {
            switch (ev.type) {
                case CtrlType::PlayIndex: {
                    int idx = (int)ev.arg;
                    if (idx >= 0 && idx < kAnimCount) {
                        anim_idx = idx;
                        playing = true;
                        frame = 0;
                        next_frame_us = esp_timer_get_time();
                        printf("playing: %d (%s)\n", anim_idx, kAnims[anim_idx].name);
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
                    anim_idx = (anim_idx + 1) % kAnimCount;
                    playing = true;
                    frame = 0;
                    next_frame_us = esp_timer_get_time();
                    printf("next: %d (%s)\n", anim_idx, kAnims[anim_idx].name);
                    break;
                }
                case CtrlType::Info:
                    printf("state: playing=%d anim=%d (%s) frame=%" PRIu32 " frame_delay_ms=%d\n",
                           playing ? 1 : 0,
                           anim_idx,
                           kAnims[anim_idx].name,
                           frame,
                           CONFIG_TUTORIAL_0013_FRAME_DELAY_MS);
                    break;
                case CtrlType::SetBrightness: {
                    int v = (int)ev.arg;
                    (void)backlight_i2c_set((uint8_t)v);
                    printf("brightness: %d\n", v);
                    break;
                }
            }
        }

        if (!playing) {
            continue;
        }

        const int64_t now2_us = esp_timer_get_time();
        if (now2_us < next_frame_us) {
            continue;
        }

        const uint32_t t_ms = (uint32_t)(now2_us / 1000);
        kAnims[anim_idx].render(canvas, frame, t_ms);

#if CONFIG_TUTORIAL_0013_PRESENT_USE_DMA
        canvas.pushSprite(0, 0);
        s_display.waitDMA();
#else
        canvas.pushSprite(0, 0);
#endif

        frame++;
        next_frame_us = now2_us + ((int64_t)CONFIG_TUTORIAL_0013_FRAME_DELAY_MS * 1000);

#if CONFIG_TUTORIAL_0013_LOG_EVERY_N_FRAMES > 0
        if ((frame % (uint32_t)CONFIG_TUTORIAL_0013_LOG_EVERY_N_FRAMES) == 0) {
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " free_heap=%" PRIu32, frame, esp_get_free_heap_size());
        }
#endif
    }
}


