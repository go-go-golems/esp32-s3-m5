/*
 * 0099 — ESP32-P4 PicoCalc display + keyboard smoke test.
 *
 * Purpose:
 *   Fast PicoCalc peripheral iteration without ESP-Hosted/Wi-Fi/HTTP.
 *
 * Same-position physical RPico adapter mapping:
 *   Keyboard: Pico GP6/SDA -> GPIO50, Pico GP7/SCL -> GPIO49.
 *   LCD:      Pico GP10/SCK -> GPIO3, GP11/MOSI -> GPIO2,
 *             GP13/CS -> GPIO7, GP14/DC -> GPIO24, GP15/RST -> GPIO25.
 *
 * Console:
 *   Waveshare CH343 USB-UART bridge on ESP32-P4 UART0 GPIO37/GPIO38.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_chip_info.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"

#include "picocalc_keyboard.h"

static const char *TAG = "p4_picocalc";

#define LCD_HOST               SPI2_HOST
#define LCD_PIN_SCK            3
#define LCD_PIN_MOSI           2
#define LCD_PIN_MISO           (-1)
#define LCD_PIN_CS             7
#define LCD_PIN_DC             24
#define LCD_PIN_RST            25
#define LCD_WIDTH              320
#define LCD_HEIGHT             320
#define LCD_SPI_HZ             (20 * 1000 * 1000)

#define LCD_CMD_SWRESET        0x01
#define LCD_CMD_SLPOUT         0x11
#define LCD_CMD_INVON          0x21
#define LCD_CMD_DISPON         0x29
#define LCD_CMD_CASET          0x2A
#define LCD_CMD_RASET          0x2B
#define LCD_CMD_RAMWR          0x2C
#define LCD_CMD_MADCTL         0x36
#define LCD_CMD_COLMOD         0x3A

static spi_device_handle_t s_lcd = NULL;
static bool s_lcd_initialized = false;
static TaskHandle_t s_kbd_raw_task = NULL;
static volatile bool s_kbd_raw_enabled = false;

static esp_err_t lcd_tx(const void *data, size_t len)
{
    if (!s_lcd || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        const size_t chunk = len > 4092 ? 4092 : len;
        spi_transaction_t t = {
            .length = chunk * 8,
            .tx_buffer = p,
        };
        esp_err_t err = spi_device_polling_transmit(s_lcd, &t);
        if (err != ESP_OK) {
            return err;
        }
        p += chunk;
        len -= chunk;
    }
    return ESP_OK;
}

static esp_err_t lcd_cmd(uint8_t cmd)
{
    gpio_set_level(LCD_PIN_DC, 0);
    return lcd_tx(&cmd, 1);
}

static esp_err_t lcd_data(const void *data, size_t len)
{
    gpio_set_level(LCD_PIN_DC, 1);
    return lcd_tx(data, len);
}

static esp_err_t lcd_cmd_data(uint8_t cmd, const void *data, size_t len)
{
    esp_err_t err = lcd_cmd(cmd);
    if (err != ESP_OK) {
        return err;
    }
    if (data && len > 0) {
        return lcd_data(data, len);
    }
    return ESP_OK;
}

static void lcd_reset(void)
{
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(30));
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));
}

static esp_err_t lcd_init_bus(void)
{
    if (s_lcd) {
        return ESP_OK;
    }

    gpio_config_t out = {
        .pin_bit_mask = (1ULL << LCD_PIN_DC) | (1ULL << LCD_PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&out));
    gpio_set_level(LCD_PIN_DC, 0);
    gpio_set_level(LCD_PIN_RST, 1);

    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = LCD_PIN_MISO,
        .sclk_io_num = LCD_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t err = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = LCD_SPI_HZ,
        .mode = 0,
        .spics_io_num = LCD_PIN_CS,
        .queue_size = 1,
    };

    err = spi_bus_add_device(LCD_HOST, &devcfg, &s_lcd);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "LCD SPI ready: sck=%d mosi=%d cs=%d dc=%d rst=%d hz=%d",
             LCD_PIN_SCK, LCD_PIN_MOSI, LCD_PIN_CS, LCD_PIN_DC, LCD_PIN_RST, LCD_SPI_HZ);
    return ESP_OK;
}

static esp_err_t lcd_init_panel(void)
{
    ESP_RETURN_ON_ERROR(lcd_init_bus(), TAG, "lcd_init_bus");

    lcd_reset();

    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_SWRESET), TAG, "swreset");
    vTaskDelay(pdMS_TO_TICKS(150));
    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_SLPOUT), TAG, "sleep out");
    vTaskDelay(pdMS_TO_TICKS(150));

    // RGB565, MADCTL MX|BGR. This is the minimal profile that worked in the
    // Pico SDK driver path and is enough for a color-bar smoke test.
    const uint8_t colmod = 0x55;
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_COLMOD, &colmod, 1), TAG, "colmod");
    vTaskDelay(pdMS_TO_TICKS(10));

    const uint8_t madctl = 0x48;
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_MADCTL, &madctl, 1), TAG, "madctl");
    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_INVON), TAG, "invon");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(lcd_cmd(LCD_CMD_DISPON), TAG, "display on");
    vTaskDelay(pdMS_TO_TICKS(50));

    s_lcd_initialized = true;
    ESP_LOGI(TAG, "LCD panel initialized (%dx%d RGB565)", LCD_WIDTH, LCD_HEIGHT);
    return ESP_OK;
}

static esp_err_t lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t col[] = {x0 >> 8, x0 & 0xff, x1 >> 8, x1 & 0xff};
    uint8_t row[] = {y0 >> 8, y0 & 0xff, y1 >> 8, y1 & 0xff};
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_CASET, col, sizeof(col)), TAG, "caset");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(LCD_CMD_RASET, row, sizeof(row)), TAG, "raset");
    return lcd_cmd(LCD_CMD_RAMWR);
}

static esp_err_t lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (!s_lcd_initialized) {
        ESP_RETURN_ON_ERROR(lcd_init_panel(), TAG, "lcd init");
    }
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return ESP_ERR_INVALID_ARG;
    }
    if (x + w > LCD_WIDTH) {
        w = LCD_WIDTH - x;
    }
    if (y + h > LCD_HEIGHT) {
        h = LCD_HEIGHT - y;
    }

    ESP_RETURN_ON_ERROR(lcd_set_window(x, y, x + w - 1, y + h - 1), TAG, "window");

    uint8_t buf[512];
    for (size_t i = 0; i < sizeof(buf); i += 2) {
        buf[i] = (uint8_t)(color >> 8);
        buf[i + 1] = (uint8_t)(color & 0xff);
    }

    uint32_t pixels = (uint32_t)w * (uint32_t)h;
    gpio_set_level(LCD_PIN_DC, 1);
    while (pixels > 0) {
        const uint32_t chunk_pixels = pixels > (sizeof(buf) / 2) ? (sizeof(buf) / 2) : pixels;
        ESP_RETURN_ON_ERROR(lcd_tx(buf, chunk_pixels * 2), TAG, "pixels");
        pixels -= chunk_pixels;
    }
    return ESP_OK;
}

static uint16_t color_from_name(const char *name, bool *ok)
{
    *ok = true;
    if (!name || strcmp(name, "white") == 0) return 0xffff;
    if (strcmp(name, "black") == 0) return 0x0000;
    if (strcmp(name, "red") == 0) return 0xf800;
    if (strcmp(name, "green") == 0) return 0x07e0;
    if (strcmp(name, "blue") == 0) return 0x001f;
    if (strcmp(name, "yellow") == 0) return 0xffe0;
    if (strcmp(name, "cyan") == 0) return 0x07ff;
    if (strcmp(name, "magenta") == 0) return 0xf81f;
    *ok = false;
    return 0;
}

static void print_keyboard_event(const picocalc_key_event_t *event)
{
    const bool printable = event->key >= 32 && event->key < 127;
    const char *name = picocalc_keyboard_key_name(event->key);
    if (printable) {
        printf("kbd event state=%u state_name=%s key=0x%02x ascii='%c' name=%s\n",
               event->state, picocalc_keyboard_state_name(event->state), event->key, (char)event->key, name);
    } else {
        printf("kbd event state=%u state_name=%s key=0x%02x ascii=. name=%s\n",
               event->state, picocalc_keyboard_state_name(event->state), event->key, name);
    }
}

static void keyboard_raw_task(void *arg)
{
    (void)arg;
    while (s_kbd_raw_enabled) {
        picocalc_key_event_t event = {0};
        esp_err_t err = picocalc_keyboard_poll_event(&event);
        if (err == ESP_OK && event.valid) {
            print_keyboard_event(&event);
            continue;
        }
        if (err != ESP_ERR_NOT_FOUND && err != ESP_OK) {
            printf("kbd raw err=%s\n", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    s_kbd_raw_task = NULL;
    vTaskDelete(NULL);
}

static void print_keyboard_status(void)
{
    picocalc_keyboard_diag_t diag = {0};
    uint8_t status = 0;
    esp_err_t err = picocalc_keyboard_read_status(&status);
    picocalc_keyboard_get_diag(&diag);
    if (err != ESP_OK) {
        printf("kbd status ok=0 err=%s initialized=%d errors=%" PRIu32 " last=0x%02x\n",
               esp_err_to_name(err), diag.initialized ? 1 : 0, diag.error_count, diag.last_status);
        return;
    }
    printf("kbd status ok=1 raw=0x%02x fifo=%u caps=%u num=%u initialized=%d errors=%" PRIu32 "\n",
           status,
           picocalc_keyboard_fifo_count(status),
           (status & PICOCALC_KBD_CAPS_LOCK_MASK) ? 1 : 0,
           (status & PICOCALC_KBD_NUM_LOCK_MASK) ? 1 : 0,
           diag.initialized ? 1 : 0,
           diag.error_count);
}

static int cmd_kbd(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "status") == 0) {
        print_keyboard_status();
        return 0;
    }
    if (strcmp(argv[1], "poll") == 0) {
        int limit = argc >= 3 ? atoi(argv[2]) : 10;
        if (limit <= 0) limit = 1;
        if (limit > 200) limit = 200;
        int events = 0;
        for (int i = 0; i < limit; i++) {
            picocalc_key_event_t event = {0};
            esp_err_t err = picocalc_keyboard_poll_event(&event);
            if (err == ESP_ERR_NOT_FOUND) break;
            if (err != ESP_OK) {
                printf("kbd poll err=%s after=%d\n", esp_err_to_name(err), events);
                return 1;
            }
            if (event.valid) {
                print_keyboard_event(&event);
                events++;
            }
        }
        printf("kbd poll done events=%d limit=%d\n", events, limit);
        return 0;
    }
    if (strcmp(argv[1], "raw") == 0) {
        if (argc < 3) {
            printf("usage: kbd raw on|off\n");
            return 1;
        }
        if (strcmp(argv[2], "on") == 0) {
            if (s_kbd_raw_task) {
                printf("kbd raw already on\n");
                return 0;
            }
            s_kbd_raw_enabled = true;
            if (xTaskCreate(keyboard_raw_task, "kbd_raw", 4096, NULL, 5, &s_kbd_raw_task) != pdPASS) {
                s_kbd_raw_enabled = false;
                s_kbd_raw_task = NULL;
                printf("kbd raw: failed to create task\n");
                return 1;
            }
            printf("kbd raw on\n");
            return 0;
        }
        if (strcmp(argv[2], "off") == 0) {
            s_kbd_raw_enabled = false;
            printf(s_kbd_raw_task ? "kbd raw off requested\n" : "kbd raw already off\n");
            return 0;
        }
    }
    printf("usage: kbd status | kbd poll [limit] | kbd raw on|off\n");
    return 1;
}

static int cmd_lcd(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "init") == 0) {
        esp_err_t err = lcd_init_panel();
        printf("lcd init: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "fill") == 0) {
        bool ok = false;
        uint16_t color = color_from_name(argc >= 3 ? argv[2] : "white", &ok);
        if (!ok) {
            printf("usage: lcd fill red|green|blue|white|black|yellow|cyan|magenta\n");
            return 1;
        }
        int64_t start = esp_timer_get_time();
        esp_err_t err = lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
        int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        printf("lcd fill color=0x%04x err=%s elapsed_ms=%" PRId64 "\n", color, esp_err_to_name(err), elapsed_ms);
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "bars") == 0) {
        const uint16_t colors[] = {0xf800, 0xffe0, 0x07e0, 0x07ff, 0x001f, 0xf81f, 0xffff, 0x0000};
        const uint16_t bar_w = LCD_WIDTH / (sizeof(colors) / sizeof(colors[0]));
        int64_t start = esp_timer_get_time();
        for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
            uint16_t x = i * bar_w;
            uint16_t w = (i == (sizeof(colors) / sizeof(colors[0])) - 1) ? (LCD_WIDTH - x) : bar_w;
            esp_err_t err = lcd_fill_rect(x, 0, w, LCD_HEIGHT, colors[i]);
            if (err != ESP_OK) {
                printf("lcd bars err=%s at bar=%u\n", esp_err_to_name(err), (unsigned)i);
                return 1;
            }
        }
        int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        printf("lcd bars ok elapsed_ms=%" PRId64 "\n", elapsed_ms);
        return 0;
    }
    printf("usage: lcd init | lcd fill <color> | lcd bars\n");
    return 1;
}

static int cmd_status(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    esp_chip_info_t chip = {0};
    esp_chip_info(&chip);
    uint32_t flash_size = 0;
    (void)esp_flash_get_size(NULL, &flash_size);
    printf("status project=0099 target=%s rev=%d cores=%d flash=%" PRIu32 " internal_free=%u psram_free=%u lcd=%s\n",
           CONFIG_IDF_TARGET,
           chip.revision,
           chip.cores,
           flash_size,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           s_lcd_initialized ? "initialized" : "not_initialized");
    return 0;
}

static void start_console(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "p4dk> ";
    repl_cfg.task_stack_size = 4096;

    esp_console_register_help_command();

    const esp_console_cmd_t status_cmd = {
        .command = "status",
        .help = "Print firmware/chip/display status",
        .func = cmd_status,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&status_cmd));

    const esp_console_cmd_t kbd_cmd = {
        .command = "kbd",
        .help = "PicoCalc keyboard diagnostics: status, poll [limit], raw on|off",
        .func = cmd_kbd,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&kbd_cmd));

    const esp_console_cmd_t lcd_cmd_def = {
        .command = "lcd",
        .help = "PicoCalc LCD diagnostics: init, fill <color>, bars",
        .func = cmd_lcd,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&lcd_cmd_def));

#if CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_dev_uart_config_t hw_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_cfg, &repl_cfg, &repl));
#else
#error This app expects UART console on the CH343 bridge.
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void app_main(void)
{
    ESP_LOGI(TAG, "boot: ESP32-P4 PicoCalc display+keyboard smoke test");
    ESP_LOGI(TAG, "console: CH343 UART0 bridge at 115200 baud");
    ESP_LOGI(TAG, "keyboard: SDA GPIO%d SCL GPIO%d addr=0x%02x hz=%d",
             PICOCALC_KBD_I2C_SDA_GPIO,
             PICOCALC_KBD_I2C_SCL_GPIO,
             PICOCALC_KBD_I2C_ADDR,
             PICOCALC_KBD_I2C_SPEED_HZ);
    ESP_LOGI(TAG, "lcd: sck=%d mosi=%d cs=%d dc=%d rst=%d hz=%d",
             LCD_PIN_SCK, LCD_PIN_MOSI, LCD_PIN_CS, LCD_PIN_DC, LCD_PIN_RST, LCD_SPI_HZ);

    esp_err_t kbd_err = picocalc_keyboard_init();
    if (kbd_err != ESP_OK) {
        ESP_LOGW(TAG, "keyboard init failed: %s; console diagnostics can retry", esp_err_to_name(kbd_err));
    }

    start_console();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "heartbeat: internal_free=%u psram_free=%u lcd=%s",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 s_lcd_initialized ? "initialized" : "not_initialized");
    }
}
