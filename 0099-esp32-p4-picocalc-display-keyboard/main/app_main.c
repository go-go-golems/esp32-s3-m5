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
// The older RP2350 PicoCalc firmware defaulted to 75 MHz on RP2350 hardware.
// ESP32-P4's SPI_CLK_SRC_DEFAULT is XTAL (40 MHz), which makes ESP-IDF reject
// SCLK requests above 20 MHz because the GPSPI driver requires SCLK <= source/2.
// Use the high-speed SPLL source explicitly so 40/75/80 MHz can be tested.
#define LCD_DEFAULT_SPI_HZ        (80 * 1000 * 1000)
#define LCD_SPI_CLK_SRC           SPI_CLK_SRC_SPLL
#define LCD_SPI_MAX_TRANSFER_SZ   (32 * 1024)
#define LCD_FILL_DMA_CHUNK_BYTES  LCD_SPI_MAX_TRANSFER_SZ

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
static int s_lcd_spi_hz = LCD_DEFAULT_SPI_HZ;
static uint8_t *s_lcd_dma_buf = NULL;
static size_t s_lcd_dma_buf_len = 0;
static TaskHandle_t s_kbd_raw_task = NULL;
static volatile bool s_kbd_raw_enabled = false;

static esp_err_t lcd_tx(const void *data, size_t len)
{
    if (!s_lcd || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        const size_t chunk = len > LCD_SPI_MAX_TRANSFER_SZ ? LCD_SPI_MAX_TRANSFER_SZ : len;
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

static esp_err_t lcd_ensure_dma_buffer(size_t min_len)
{
    if (s_lcd_dma_buf && s_lcd_dma_buf_len >= min_len) {
        return ESP_OK;
    }

    if (s_lcd_dma_buf) {
        heap_caps_free(s_lcd_dma_buf);
        s_lcd_dma_buf = NULL;
        s_lcd_dma_buf_len = 0;
    }

    s_lcd_dma_buf = (uint8_t *)heap_caps_malloc(min_len, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_lcd_dma_buf) {
        return ESP_ERR_NO_MEM;
    }
    s_lcd_dma_buf_len = min_len;
    ESP_LOGI(TAG, "LCD DMA buffer allocated: %u bytes", (unsigned)s_lcd_dma_buf_len);
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

static esp_err_t lcd_add_device(void)
{
    if (s_lcd) {
        return ESP_OK;
    }

    spi_device_interface_config_t devcfg = {
        .clock_source = LCD_SPI_CLK_SRC,
        .clock_speed_hz = s_lcd_spi_hz,
        .mode = 0,
        .spics_io_num = LCD_PIN_CS,
        .queue_size = 4,
    };

    esp_err_t err = spi_bus_add_device(LCD_HOST, &devcfg, &s_lcd);
    if (err != ESP_OK) {
        return err;
    }

    int actual_khz = 0;
    err = spi_device_get_actual_freq(s_lcd, &actual_khz);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "LCD SPI device ready: clk_src=%d requested=%d Hz actual=%d kHz", LCD_SPI_CLK_SRC, s_lcd_spi_hz, actual_khz);
    } else {
        ESP_LOGW(TAG, "LCD SPI device ready but actual freq unavailable: %s", esp_err_to_name(err));
    }
    return ESP_OK;
}

static esp_err_t lcd_set_speed_hz(int hz)
{
    if (hz <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_lcd) {
        ESP_RETURN_ON_ERROR(spi_bus_remove_device(s_lcd), TAG, "remove lcd device");
        s_lcd = NULL;
    }

    const int old_hz = s_lcd_spi_hz;
    s_lcd_spi_hz = hz;
    esp_err_t err = lcd_add_device();
    if (err != ESP_OK) {
        s_lcd_spi_hz = old_hz;
        (void)lcd_add_device();
        return err;
    }
    return ESP_OK;
}

static esp_err_t lcd_init_bus_gpio_and_host(void)
{
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
        .max_transfer_sz = LCD_SPI_MAX_TRANSFER_SZ,
    };

    esp_err_t err = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    return ESP_OK;
}

static esp_err_t lcd_init_bus(void)
{
    ESP_RETURN_ON_ERROR(lcd_init_bus_gpio_and_host(), TAG, "lcd bus gpio/host");
    ESP_RETURN_ON_ERROR(lcd_add_device(), TAG, "add lcd device");

    ESP_LOGI(TAG, "LCD SPI ready: sck=%d mosi=%d cs=%d dc=%d rst=%d requested_hz=%d max_transfer=%d",
             LCD_PIN_SCK, LCD_PIN_MOSI, LCD_PIN_CS, LCD_PIN_DC, LCD_PIN_RST, s_lcd_spi_hz, LCD_SPI_MAX_TRANSFER_SZ);
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

    ESP_RETURN_ON_ERROR(lcd_ensure_dma_buffer(LCD_FILL_DMA_CHUNK_BYTES), TAG, "alloc fill dma buffer");
    for (size_t i = 0; i < s_lcd_dma_buf_len; i += 2) {
        s_lcd_dma_buf[i] = (uint8_t)(color >> 8);
        s_lcd_dma_buf[i + 1] = (uint8_t)(color & 0xff);
    }

    size_t bytes = (size_t)w * (size_t)h * 2;
    gpio_set_level(LCD_PIN_DC, 1);
    while (bytes > 0) {
        const size_t chunk = bytes > s_lcd_dma_buf_len ? s_lcd_dma_buf_len : bytes;
        ESP_RETURN_ON_ERROR(lcd_tx(s_lcd_dma_buf, chunk), TAG, "pixels");
        bytes -= chunk;
    }
    return ESP_OK;
}

static int lcd_actual_khz(void)
{
    int actual_khz = 0;
    if (s_lcd && spi_device_get_actual_freq(s_lcd, &actual_khz) == ESP_OK) {
        return actual_khz;
    }
    return 0;
}

static bool parse_speed_hz(const char *text, int *hz)
{
    if (!text || !hz) {
        return false;
    }

    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (value <= 0) {
        return false;
    }

    if (end && (*end == 'm' || *end == 'M')) {
        end++;
        if (*end == 'h' || *end == 'H') end++;
        if (*end == 'z' || *end == 'Z') end++;
        if (*end != '\0') return false;
        value *= 1000000L;
    } else if (end && (*end == 'k' || *end == 'K')) {
        end++;
        if (*end == 'h' || *end == 'H') end++;
        if (*end == 'z' || *end == 'Z') end++;
        if (*end != '\0') return false;
        value *= 1000L;
    } else if (end && *end != '\0') {
        return false;
    }

    if (value > 200000000L) {
        return false;
    }
    *hz = (int)value;
    return true;
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

typedef enum {
    LCD_PATTERN_CHECKER,
    LCD_PATTERN_STRIPES,
    LCD_PATTERN_DIAGONAL,
} lcd_pattern_t;

static bool pattern_from_name(const char *name, lcd_pattern_t *pattern)
{
    if (!name || !pattern) {
        return false;
    }
    if (strcmp(name, "checker") == 0) {
        *pattern = LCD_PATTERN_CHECKER;
        return true;
    }
    if (strcmp(name, "stripes") == 0) {
        *pattern = LCD_PATTERN_STRIPES;
        return true;
    }
    if (strcmp(name, "diagonal") == 0) {
        *pattern = LCD_PATTERN_DIAGONAL;
        return true;
    }
    return false;
}

static const char *pattern_name(lcd_pattern_t pattern)
{
    switch (pattern) {
    case LCD_PATTERN_CHECKER:
        return "checker";
    case LCD_PATTERN_STRIPES:
        return "stripes";
    case LCD_PATTERN_DIAGONAL:
        return "diagonal";
    default:
        return "unknown";
    }
}

static uint16_t pattern_pixel(lcd_pattern_t pattern, uint16_t x, uint16_t y)
{
    switch (pattern) {
    case LCD_PATTERN_CHECKER:
        return (((x / 8) ^ (y / 8)) & 1) ? 0xffff : 0x0000;
    case LCD_PATTERN_STRIPES:
        return (x & 1) ? 0xffff : 0x0000;
    case LCD_PATTERN_DIAGONAL:
        return (((x + y) / 4) & 1) ? 0xffe0 : 0x001f;
    default:
        return 0xf81f;
    }
}

static esp_err_t lcd_pattern_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, lcd_pattern_t pattern)
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
    ESP_RETURN_ON_ERROR(lcd_ensure_dma_buffer(LCD_FILL_DMA_CHUNK_BYTES), TAG, "alloc pattern dma buffer");

    const size_t total_pixels = (size_t)w * (size_t)h;
    const size_t max_chunk_pixels = s_lcd_dma_buf_len / 2;
    size_t pixel_offset = 0;
    gpio_set_level(LCD_PIN_DC, 1);
    while (pixel_offset < total_pixels) {
        const size_t remaining = total_pixels - pixel_offset;
        const size_t chunk_pixels = remaining > max_chunk_pixels ? max_chunk_pixels : remaining;
        for (size_t i = 0; i < chunk_pixels; i++) {
            const size_t index = pixel_offset + i;
            const uint16_t px = x + (uint16_t)(index % w);
            const uint16_t py = y + (uint16_t)(index / w);
            const uint16_t color = pattern_pixel(pattern, px, py);
            s_lcd_dma_buf[i * 2] = (uint8_t)(color >> 8);
            s_lcd_dma_buf[i * 2 + 1] = (uint8_t)(color & 0xff);
        }
        ESP_RETURN_ON_ERROR(lcd_tx(s_lcd_dma_buf, chunk_pixels * 2), TAG, "pattern pixels");
        pixel_offset += chunk_pixels;
    }
    return ESP_OK;
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
        const int64_t bytes = (int64_t)LCD_WIDTH * LCD_HEIGHT * 2;
        const int64_t kib_s = elapsed_ms > 0 ? ((bytes * 1000) / 1024) / elapsed_ms : 0;
        printf("lcd fill color=0x%04x err=%s elapsed_ms=%" PRId64 " throughput_kib_s=%" PRId64 " dma_chunk=%d\n",
               color, esp_err_to_name(err), elapsed_ms, kib_s, LCD_FILL_DMA_CHUNK_BYTES);
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "speed") == 0 || strcmp(argv[1], "baud") == 0) {
        if (argc < 3) {
            printf("lcd speed requested=%d actual_khz=%d\n", s_lcd_spi_hz, lcd_actual_khz());
            printf("usage: lcd speed <hz|MHz>; examples: lcd speed 40M, lcd speed 80000000\n");
            return 0;
        }
        int hz = 0;
        if (!parse_speed_hz(argv[2], &hz)) {
            printf("usage: lcd speed <hz|MHz>; examples: lcd speed 40M, lcd speed 80000000\n");
            return 1;
        }
        if (hz > 80000000) {
            printf("lcd speed warning: ESP-IDF GPSPI master usually caps normal SCLK at 80 MHz. Trying %d Hz anyway\n", hz);
        }
        esp_err_t err = lcd_init_bus_gpio_and_host();
        if (err == ESP_OK) {
            err = lcd_set_speed_hz(hz);
        }
        printf("lcd speed requested=%d err=%s actual_khz=%d\n", hz, esp_err_to_name(err), lcd_actual_khz());
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "bench") == 0) {
        int loops = argc >= 3 ? atoi(argv[2]) : 5;
        if (loops <= 0) loops = 1;
        if (loops > 100) loops = 100;
        const uint16_t colors[] = {0xf800, 0x07e0, 0x001f, 0xffff, 0x0000};
        int64_t start = esp_timer_get_time();
        for (int i = 0; i < loops; i++) {
            esp_err_t err = lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, colors[i % (sizeof(colors) / sizeof(colors[0]))]);
            if (err != ESP_OK) {
                printf("lcd bench err=%s at loop=%d\n", esp_err_to_name(err), i);
                return 1;
            }
        }
        int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        const int64_t bytes = (int64_t)LCD_WIDTH * LCD_HEIGHT * 2 * loops;
        const int64_t kib_s = elapsed_ms > 0 ? ((bytes * 1000) / 1024) / elapsed_ms : 0;
        printf("lcd bench loops=%d elapsed_ms=%" PRId64 " per_fill_ms=%" PRId64 " throughput_kib_s=%" PRId64 " requested=%d actual_khz=%d dma_chunk=%d\n",
               loops, elapsed_ms, elapsed_ms / loops, kib_s, s_lcd_spi_hz, lcd_actual_khz(), LCD_FILL_DMA_CHUNK_BYTES);
        return 0;
    }
    if (strcmp(argv[1], "pattern") == 0) {
        if (argc < 3) {
            printf("usage: lcd pattern checker|stripes|diagonal|all\n");
            return 1;
        }
        const lcd_pattern_t patterns[] = {LCD_PATTERN_CHECKER, LCD_PATTERN_STRIPES, LCD_PATTERN_DIAGONAL};
        if (strcmp(argv[2], "all") == 0) {
            for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
                int64_t start = esp_timer_get_time();
                esp_err_t err = lcd_pattern_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, patterns[i]);
                int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
                printf("lcd pattern name=%s err=%s elapsed_ms=%" PRId64 " requested=%d actual_khz=%d dma_chunk=%d\n",
                       pattern_name(patterns[i]), esp_err_to_name(err), elapsed_ms, s_lcd_spi_hz, lcd_actual_khz(), LCD_FILL_DMA_CHUNK_BYTES);
                if (err != ESP_OK) {
                    return 1;
                }
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            return 0;
        }
        lcd_pattern_t pattern = LCD_PATTERN_CHECKER;
        if (!pattern_from_name(argv[2], &pattern)) {
            printf("usage: lcd pattern checker|stripes|diagonal|all\n");
            return 1;
        }
        int64_t start = esp_timer_get_time();
        esp_err_t err = lcd_pattern_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, pattern);
        int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        printf("lcd pattern name=%s err=%s elapsed_ms=%" PRId64 " requested=%d actual_khz=%d dma_chunk=%d\n",
               pattern_name(pattern), esp_err_to_name(err), elapsed_ms, s_lcd_spi_hz, lcd_actual_khz(), LCD_FILL_DMA_CHUNK_BYTES);
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "rectbench") == 0 || strcmp(argv[1], "cellbench") == 0) {
        const bool is_cellbench = strcmp(argv[1], "cellbench") == 0;
        int w = argc >= 3 ? atoi(argv[2]) : (is_cellbench ? 8 : 16);
        int h = argc >= 4 ? atoi(argv[3]) : (is_cellbench ? 16 : 16);
        int loops = argc >= 5 ? atoi(argv[4]) : (is_cellbench ? 1000 : 500);
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;
        if (w > LCD_WIDTH) w = LCD_WIDTH;
        if (h > LCD_HEIGHT) h = LCD_HEIGHT;
        if (loops <= 0) loops = 1;
        if (loops > 10000) loops = 10000;
        const uint16_t colors[] = {0xf800, 0x07e0, 0x001f, 0xffff, 0x0000, 0xffe0, 0x07ff, 0xf81f};
        const int max_x = LCD_WIDTH - w;
        const int max_y = LCD_HEIGHT - h;
        int64_t start = esp_timer_get_time();
        for (int i = 0; i < loops; i++) {
            const uint16_t x = max_x > 0 ? (uint16_t)((i * 7) % (max_x + 1)) : 0;
            const uint16_t y = max_y > 0 ? (uint16_t)((i * 5) % (max_y + 1)) : 0;
            esp_err_t err = lcd_fill_rect(x, y, (uint16_t)w, (uint16_t)h, colors[i % (sizeof(colors) / sizeof(colors[0]))]);
            if (err != ESP_OK) {
                printf("lcd %s err=%s at loop=%d\n", argv[1], esp_err_to_name(err), i);
                return 1;
            }
        }
        int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        const int64_t bytes = (int64_t)w * h * 2 * loops;
        const int64_t kib_s = elapsed_ms > 0 ? ((bytes * 1000) / 1024) / elapsed_ms : 0;
        const int64_t rects_s = elapsed_ms > 0 ? ((int64_t)loops * 1000) / elapsed_ms : 0;
        printf("lcd %s w=%d h=%d loops=%d elapsed_ms=%" PRId64 " rects_s=%" PRId64 " payload_kib_s=%" PRId64 " requested=%d actual_khz=%d\n",
               argv[1], w, h, loops, elapsed_ms, rects_s, kib_s, s_lcd_spi_hz, lcd_actual_khz());
        return 0;
    }
    if (strcmp(argv[1], "rowbench") == 0) {
        int h = argc >= 3 ? atoi(argv[2]) : 16;
        int loops = argc >= 4 ? atoi(argv[3]) : 200;
        if (h <= 0) h = 1;
        if (h > LCD_HEIGHT) h = LCD_HEIGHT;
        if (loops <= 0) loops = 1;
        if (loops > 5000) loops = 5000;
        const uint16_t colors[] = {0x0000, 0xffff, 0x001f, 0xffe0, 0x07e0, 0xf81f};
        const int max_y = LCD_HEIGHT - h;
        int64_t start = esp_timer_get_time();
        for (int i = 0; i < loops; i++) {
            const uint16_t y = max_y > 0 ? (uint16_t)((i * h) % (max_y + 1)) : 0;
            esp_err_t err = lcd_fill_rect(0, y, LCD_WIDTH, (uint16_t)h, colors[i % (sizeof(colors) / sizeof(colors[0]))]);
            if (err != ESP_OK) {
                printf("lcd rowbench err=%s at loop=%d\n", esp_err_to_name(err), i);
                return 1;
            }
        }
        int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        const int64_t bytes = (int64_t)LCD_WIDTH * h * 2 * loops;
        const int64_t kib_s = elapsed_ms > 0 ? ((bytes * 1000) / 1024) / elapsed_ms : 0;
        const int64_t rows_s = elapsed_ms > 0 ? ((int64_t)loops * 1000) / elapsed_ms : 0;
        printf("lcd rowbench h=%d loops=%d elapsed_ms=%" PRId64 " rows_s=%" PRId64 " payload_kib_s=%" PRId64 " requested=%d actual_khz=%d\n",
               h, loops, elapsed_ms, rows_s, kib_s, s_lcd_spi_hz, lcd_actual_khz());
        return 0;
    }
    if (strcmp(argv[1], "scrollbench") == 0) {
        int row_h = argc >= 3 ? atoi(argv[2]) : 16;
        int loops = argc >= 4 ? atoi(argv[3]) : 20;
        if (row_h <= 0) row_h = 1;
        if (row_h > LCD_HEIGHT) row_h = LCD_HEIGHT;
        if (loops <= 0) loops = 1;
        if (loops > 500) loops = 500;
        const int rows = (LCD_HEIGHT + row_h - 1) / row_h;
        const uint16_t colors[] = {0x0000, 0x2104, 0x4208, 0x6318, 0x8410, 0xa514, 0xc618, 0xffff};
        int64_t start = esp_timer_get_time();
        for (int loop = 0; loop < loops; loop++) {
            for (int row = 0; row < rows; row++) {
                const uint16_t y = (uint16_t)(row * row_h);
                uint16_t h = (uint16_t)row_h;
                if (y + h > LCD_HEIGHT) {
                    h = LCD_HEIGHT - y;
                }
                const uint16_t color = colors[(row + loop) % (sizeof(colors) / sizeof(colors[0]))];
                esp_err_t err = lcd_fill_rect(0, y, LCD_WIDTH, h, color);
                if (err != ESP_OK) {
                    printf("lcd scrollbench err=%s at loop=%d row=%d\n", esp_err_to_name(err), loop, row);
                    return 1;
                }
            }
        }
        int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        const int64_t bytes = (int64_t)LCD_WIDTH * LCD_HEIGHT * 2 * loops;
        const int64_t kib_s = elapsed_ms > 0 ? ((bytes * 1000) / 1024) / elapsed_ms : 0;
        const int64_t scrolls_s = elapsed_ms > 0 ? ((int64_t)loops * 1000) / elapsed_ms : 0;
        const int64_t row_updates_s = elapsed_ms > 0 ? ((int64_t)loops * rows * 1000) / elapsed_ms : 0;
        printf("lcd scrollbench row_h=%d rows=%d loops=%d elapsed_ms=%" PRId64 " scrolls_s=%" PRId64 " row_updates_s=%" PRId64 " payload_kib_s=%" PRId64 " requested=%d actual_khz=%d\n",
               row_h, rows, loops, elapsed_ms, scrolls_s, row_updates_s, kib_s, s_lcd_spi_hz, lcd_actual_khz());
        return 0;
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
    printf("usage: lcd init | lcd speed <hz|MHz> | lcd bench [loops] | lcd rectbench [w h loops] | lcd cellbench [w h loops] | lcd rowbench [h loops] | lcd scrollbench [row_h loops] | lcd pattern checker|stripes|diagonal|all | lcd fill <color> | lcd bars\n");
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
    printf("status project=0099 target=%s rev=%d cores=%d flash=%" PRIu32 " internal_free=%u psram_free=%u lcd=%s lcd_requested_hz=%d lcd_actual_khz=%d lcd_dma_chunk=%d lcd_dma_buf=%u\n",
           CONFIG_IDF_TARGET,
           chip.revision,
           chip.cores,
           flash_size,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           s_lcd_initialized ? "initialized" : "not_initialized",
           s_lcd_spi_hz,
           lcd_actual_khz(),
           LCD_FILL_DMA_CHUNK_BYTES,
           (unsigned)s_lcd_dma_buf_len);
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
        .help = "PicoCalc LCD diagnostics: init, speed, bench, rect/cell/row/scrollbench, pattern, fill, bars",
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
    ESP_LOGI(TAG, "lcd: sck=%d mosi=%d cs=%d dc=%d rst=%d clk_src=%d default_hz=%d max_transfer=%d dma_chunk=%d",
             LCD_PIN_SCK, LCD_PIN_MOSI, LCD_PIN_CS, LCD_PIN_DC, LCD_PIN_RST, LCD_SPI_CLK_SRC, s_lcd_spi_hz,
             LCD_SPI_MAX_TRANSFER_SZ, LCD_FILL_DMA_CHUNK_BYTES);

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
