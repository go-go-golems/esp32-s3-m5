#include "matrix_console.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "max7219.h"

static const char *TAG = "matrix_console";

static max7219_t s_matrix;
static bool s_matrix_ready = false;
static bool s_reverse_modules = false;
static bool s_flip_vertical = false;

static uint8_t s_fb[8][MAX7219_DEFAULT_CHAIN_LEN] = {0};

static esp_err_t fb_flush_all(void);
static void try_autoinit(void);

static SemaphoreHandle_t s_matrix_mu;
static TaskHandle_t s_blink_task;
static bool s_blink_enabled = false;
static uint32_t s_blink_on_ms = 250;
static uint32_t s_blink_off_ms = 250;
static uint8_t s_blink_saved_fb[8][MAX7219_DEFAULT_CHAIN_LEN] = {0};
static bool s_blink_have_saved_fb = false;

static void matrix_lock(void) {
    if (s_matrix_mu) xSemaphoreTake(s_matrix_mu, portMAX_DELAY);
}

static void matrix_unlock(void) {
    if (s_matrix_mu) xSemaphoreGive(s_matrix_mu);
}

static void print_matrix_help(void) {
    printf("matrix commands:\n");
    printf("  matrix init\n");
    printf("  matrix status\n");
    printf("  matrix clear\n");
    printf("  matrix test on|off\n");
    printf("  matrix safe on|off                            (RAM-based full-on/full-off; good for wiring bring-up)\n");
    printf("  matrix text <ABCD>                            (renders 4 chars: one per 8x8 module)\n");
    printf("  matrix text <A> <B> <C> <D>                   (same, but split args)\n");
    printf("  matrix intensity <0..15>\n");
    printf("  matrix spi [hz]                               (get/set SPI clock; default is compile-time)\n");
    printf("  matrix reverse on|off\n");
    printf("  matrix flipv on|off                           (flip vertically; fixes upside-down glyphs)\n");
    printf("  matrix row <0..7> <0x00..0xff>                 (sets this row on all modules)\n");
    printf("  matrix row4 <0..7> <b0> <b1> <b2> <b3>          (one byte per module)\n");
    printf("  matrix px <0..%d> <0..7> <0|1>                   (pixel on the 32x8 strip)\n",
           8 * MAX7219_DEFAULT_CHAIN_LEN - 1);
    printf("  matrix pattern rows|cols|diag|checker|off|ids\n");
    printf("  matrix pattern onehot <0..3>                     (turn on exactly one module)\n");
    printf("  matrix blink on [on_ms] [off_ms]                 (loop full-on/full-off)\n");
    printf("  matrix blink off\n");
    printf("  matrix blink status\n");
}

static void fb_clear(void) {
    memset(s_fb, 0, sizeof(s_fb));
}

static void fb_set_ids_pattern(void) {
    const uint8_t fills[MAX7219_DEFAULT_CHAIN_LEN] = {0x81, 0x42, 0x24, 0x18};
    for (int y = 0; y < 8; y++) {
        for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
            s_fb[y][m] = fills[m];
        }
    }
}

static char ascii_upper(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
    return c;
}

// 5x7 font, column-major: 5 bytes per glyph, LSB = top row.
static const uint8_t s_font5x7[59][5] = {
    [' ' - 32] = {0x00, 0x00, 0x00, 0x00, 0x00},

    ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4' - 32] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5' - 32] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6' - 32] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7' - 32] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8' - 32] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9' - 32] = {0x06, 0x49, 0x49, 0x29, 0x1E},

    ['A' - 32] = {0x7C, 0x12, 0x11, 0x12, 0x7C},
    ['B' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D' - 32] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G' - 32] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H' - 32] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I' - 32] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['J' - 32] = {0x20, 0x40, 0x41, 0x3F, 0x01},
    ['K' - 32] = {0x7F, 0x08, 0x14, 0x22, 0x41},
    ['L' - 32] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M' - 32] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['N' - 32] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['Q' - 32] = {0x3E, 0x41, 0x51, 0x21, 0x5E},
    ['R' - 32] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S' - 32] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T' - 32] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U' - 32] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V' - 32] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W' - 32] = {0x3F, 0x40, 0x38, 0x40, 0x3F},
    ['X' - 32] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y' - 32] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z' - 32] = {0x61, 0x51, 0x49, 0x45, 0x43},
};

static const uint8_t *font5x7_get_cols(char c) {
    c = ascii_upper(c);
    if (c < 32 || c > 90) c = ' ';
    return s_font5x7[c - 32];
}

static void render_char_8x8_rows(char c, uint8_t out_rows[8]) {
    memset(out_rows, 0, 8);
    const uint8_t *cols = font5x7_get_cols(c);
    const int x0 = 1;

    for (int col = 0; col < 5; col++) {
        const uint8_t column_bits = cols[col];
        for (int y = 0; y < 7; y++) {
            if (column_bits & (uint8_t)(1u << y)) {
                out_rows[y] |= (uint8_t)(1u << (x0 + col));
            }
        }
    }
}

static int fb_width(void) {
    return 8 * MAX7219_DEFAULT_CHAIN_LEN;
}

static int x_to_module(int x) {
    return x / 8;
}

static uint8_t x_to_bit(int x) {
    return (uint8_t)(1u << (x % 8));
}

static esp_err_t fb_flush_row(int y) {
    if (y < 0 || y >= 8) return ESP_ERR_INVALID_ARG;

    const int phy_row = s_flip_vertical ? (7 - y) : y;

    matrix_lock();
    const int n = (s_matrix.chain_len > 0) ? s_matrix.chain_len : MAX7219_DEFAULT_CHAIN_LEN;
    uint8_t phy[MAX7219_DEFAULT_CHAIN_LEN] = {0};
    for (int m = 0; m < n && m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
        const int src = s_reverse_modules ? ((n - 1) - m) : m;
        phy[m] = s_fb[y][src];
    }

    esp_err_t err = max7219_set_row_chain(&s_matrix, (uint8_t)phy_row, phy);
    matrix_unlock();
    return err;
}

static esp_err_t fb_flush_all(void) {
    for (int y = 0; y < 8; y++) {
        esp_err_t err = fb_flush_row(y);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

static void blink_task(void *arg) {
    (void)arg;
    for (;;) {
        if (!s_blink_enabled) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        uint8_t ones[8][MAX7219_DEFAULT_CHAIN_LEN];
        uint8_t zeros[8][MAX7219_DEFAULT_CHAIN_LEN];
        for (int y = 0; y < 8; y++) {
            for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
                ones[y][m] = 0xFF;
                zeros[y][m] = 0x00;
            }
        }

        matrix_lock();
        memcpy(s_fb, ones, sizeof(s_fb));
        matrix_unlock();
        (void)fb_flush_all();
        vTaskDelay(pdMS_TO_TICKS(s_blink_on_ms));

        if (!s_blink_enabled) continue;

        matrix_lock();
        memcpy(s_fb, zeros, sizeof(s_fb));
        matrix_unlock();
        (void)fb_flush_all();
        vTaskDelay(pdMS_TO_TICKS(s_blink_off_ms));
    }
}

static void blink_ensure_task(void) {
    if (s_blink_task) return;
    xTaskCreate(&blink_task, "matrix_blink", 3072, NULL, 2, &s_blink_task);
}

static void blink_on(uint32_t on_ms, uint32_t off_ms) {
    if (!s_matrix_ready) {
        printf("matrix not initialized (run: matrix init)\n");
        return;
    }
    if (!s_blink_have_saved_fb) {
        matrix_lock();
        memcpy(s_blink_saved_fb, s_fb, sizeof(s_fb));
        matrix_unlock();
        s_blink_have_saved_fb = true;
    }
    s_blink_on_ms = on_ms;
    s_blink_off_ms = off_ms;
    s_blink_enabled = true;
    blink_ensure_task();
    xTaskNotifyGive(s_blink_task);
}

static void blink_off(void) {
    s_blink_enabled = false;
    if (s_blink_have_saved_fb) {
        matrix_lock();
        memcpy(s_fb, s_blink_saved_fb, sizeof(s_fb));
        matrix_unlock();
        (void)fb_flush_all();
        s_blink_have_saved_fb = false;
    }
    if (s_blink_task) {
        xTaskNotifyGive(s_blink_task);
    }
}

static void try_autoinit(void) {
    if (s_matrix_ready) return;

    esp_err_t err = max7219_open(&s_matrix,
                                 MAX7219_DEFAULT_SPI_HOST,
                                 MAX7219_DEFAULT_PIN_SCK,
                                 MAX7219_DEFAULT_PIN_MOSI,
                                 MAX7219_DEFAULT_PIN_CS,
                                 MAX7219_DEFAULT_CHAIN_LEN);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "MAX7219 auto-open failed: %s (use `matrix init` to retry)", esp_err_to_name(err));
        return;
    }

    err = max7219_init(&s_matrix);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "MAX7219 auto-init failed: %s (use `matrix init` to retry)", esp_err_to_name(err));
        return;
    }

    s_matrix_ready = true;
    if (!s_matrix_mu) s_matrix_mu = xSemaphoreCreateMutex();
    fb_clear();
    fb_set_ids_pattern();
    err = fb_flush_all();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "MAX7219 auto pattern failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "MAX7219 ready (auto-init); showing 'ids' pattern (try: matrix help, matrix test on)");
}

static int cmd_matrix(int argc, char **argv) {
    if (argc < 2) {
        // Treat bare `matrix` as `matrix help` to avoid confusing "non-zero error" messages in idf.py monitor.
        print_matrix_help();
        return 0;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_matrix_help();
        return 0;
    }

    if (strcmp(argv[1], "init") == 0) {
        if (!s_matrix_mu) s_matrix_mu = xSemaphoreCreateMutex();
        esp_err_t err = max7219_open(&s_matrix, MAX7219_DEFAULT_SPI_HOST, MAX7219_DEFAULT_PIN_SCK,
                                     MAX7219_DEFAULT_PIN_MOSI, MAX7219_DEFAULT_PIN_CS, MAX7219_DEFAULT_CHAIN_LEN);
        if (err != ESP_OK) {
            printf("init failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        err = max7219_init(&s_matrix);
        if (err != ESP_OK) {
            printf("init failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        s_matrix_ready = true;
        fb_clear();
        fb_set_ids_pattern();
        (void)fb_flush_all();
        printf("ok\n");
        return 0;
    }

    if (!s_matrix_ready) {
        printf("matrix not initialized (run: matrix init)\n");
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        printf("ok: chain_len=%d spi_hz=%d reverse_modules=%s flipv=%s blink=%s on_ms=%u off_ms=%u\n",
               s_matrix.chain_len,
               s_matrix.clock_hz,
               s_reverse_modules ? "on" : "off",
               s_flip_vertical ? "on" : "off",
               s_blink_enabled ? "on" : "off",
               (unsigned)s_blink_on_ms,
               (unsigned)s_blink_off_ms);
        return 0;
    }

    if (strcmp(argv[1], "spi") == 0) {
        blink_off();
        if (argc < 3) {
            printf("ok: spi_hz=%d\n", s_matrix.clock_hz);
            return 0;
        }
        char *end = NULL;
        long v = strtol(argv[2], &end, 0);
        if (!end || *end != '\0' || v < 1000 || v > 10000000) {
            printf("invalid hz: %s (expected 1000..10000000)\n", argv[2]);
            return 1;
        }
        matrix_lock();
        esp_err_t err = max7219_set_spi_clock_hz(&s_matrix, (int)v);
        matrix_unlock();
        if (err != ESP_OK) {
            printf("spi failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok: spi_hz=%d\n", s_matrix.clock_hz);
        return 0;
    }

    if (strcmp(argv[1], "blink") == 0) {
        if (argc < 3) {
            printf("usage: matrix blink on [on_ms] [off_ms] | matrix blink off | matrix blink status\n");
            return 1;
        }
        if (strcmp(argv[2], "status") == 0) {
            printf("ok: blink=%s on_ms=%u off_ms=%u\n",
                   s_blink_enabled ? "on" : "off",
                   (unsigned)s_blink_on_ms,
                   (unsigned)s_blink_off_ms);
            return 0;
        }
        if (strcmp(argv[2], "off") == 0) {
            blink_off();
            printf("ok\n");
            return 0;
        }
        if (strcmp(argv[2], "on") == 0) {
            uint32_t on_ms = s_blink_on_ms;
            uint32_t off_ms = s_blink_off_ms;
            if (argc >= 4) {
                char *end = NULL;
                long v = strtol(argv[3], &end, 0);
                if (!end || *end != '\0' || v < 1 || v > 60000) {
                    printf("invalid on_ms: %s\n", argv[3]);
                    return 1;
                }
                on_ms = (uint32_t)v;
            }
            if (argc >= 5) {
                char *end = NULL;
                long v = strtol(argv[4], &end, 0);
                if (!end || *end != '\0' || v < 1 || v > 60000) {
                    printf("invalid off_ms: %s\n", argv[4]);
                    return 1;
                }
                off_ms = (uint32_t)v;
            }
            blink_on(on_ms, off_ms);
            if (s_blink_enabled) printf("ok\n");
            return s_blink_enabled ? 0 : 1;
        }
        printf("usage: matrix blink on [on_ms] [off_ms] | matrix blink off | matrix blink status\n");
        return 1;
    }

    if (strcmp(argv[1], "clear") == 0) {
        blink_off();
        fb_clear();
        esp_err_t err = max7219_clear(&s_matrix);
        if (err != ESP_OK) {
            printf("clear failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "test") == 0) {
        blink_off();
        if (argc < 3) {
            printf("usage: matrix test on|off\n");
            return 1;
        }
        bool on = false;
        if (strcmp(argv[2], "on") == 0) {
            on = true;
        } else if (strcmp(argv[2], "off") == 0) {
            on = false;
        } else {
            printf("usage: matrix test on|off\n");
            return 1;
        }
        esp_err_t err = max7219_set_test(&s_matrix, on);
        if (err != ESP_OK) {
            printf("test failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "safe") == 0) {
        blink_off();
        if (argc < 3) {
            printf("usage: matrix safe on|off\n");
            return 1;
        }

        const bool on = (strcmp(argv[2], "on") == 0);
        const bool off = (strcmp(argv[2], "off") == 0);
        if (!on && !off) {
            printf("usage: matrix safe on|off\n");
            return 1;
        }

        for (int y = 0; y < 8; y++) {
            for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
                s_fb[y][m] = on ? 0xFF : 0x00;
            }
        }
        esp_err_t err = fb_flush_all();
        if (err != ESP_OK) {
            printf("safe failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "text") == 0) {
        blink_off();

        char chars[4] = {' ', ' ', ' ', ' '};
        if (argc == 3) {
            const char *s = argv[2];
            const size_t n = strlen(s);
            for (size_t i = 0; i < 4 && i < n; i++) chars[i] = s[i];
        } else if (argc == 6) {
            for (int i = 0; i < 4; i++) chars[i] = argv[2 + i][0];
        } else {
            printf("usage: matrix text <ABCD>\n");
            printf("       matrix text <A> <B> <C> <D>\n");
            return 1;
        }

        for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
            uint8_t rows[8];
            render_char_8x8_rows(chars[m], rows);
            for (int y = 0; y < 8; y++) {
                s_fb[y][m] = rows[y];
            }
        }

        esp_err_t err = fb_flush_all();
        if (err != ESP_OK) {
            printf("text failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "intensity") == 0) {
        blink_off();
        if (argc < 3) {
            printf("usage: matrix intensity <0..15>\n");
            return 1;
        }
        char *end = NULL;
        long v = strtol(argv[2], &end, 0);
        if (!end || *end != '\0' || v < 0 || v > 15) {
            printf("invalid intensity: %s\n", argv[2]);
            return 1;
        }
        esp_err_t err = max7219_set_intensity(&s_matrix, (uint8_t)v);
        if (err != ESP_OK) {
            printf("intensity failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "reverse") == 0) {
        if (argc < 3) {
            printf("usage: matrix reverse on|off\n");
            return 1;
        }
        if (strcmp(argv[2], "on") == 0) {
            s_reverse_modules = true;
        } else if (strcmp(argv[2], "off") == 0) {
            s_reverse_modules = false;
        } else {
            printf("usage: matrix reverse on|off\n");
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "flipv") == 0) {
        if (argc < 3) {
            printf("usage: matrix flipv on|off\n");
            return 1;
        }
        if (strcmp(argv[2], "on") == 0) {
            s_flip_vertical = true;
        } else if (strcmp(argv[2], "off") == 0) {
            s_flip_vertical = false;
        } else {
            printf("usage: matrix flipv on|off\n");
            return 1;
        }
        (void)fb_flush_all();
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "row") == 0) {
        blink_off();
        if (argc < 4) {
            printf("usage: matrix row <0..7> <0x00..0xff>\n");
            return 1;
        }
        char *end1 = NULL;
        char *end2 = NULL;
        long row = strtol(argv[2], &end1, 0);
        long val = strtol(argv[3], &end2, 0);
        if (!end1 || *end1 != '\0' || row < 0 || row > 7) {
            printf("invalid row: %s\n", argv[2]);
            return 1;
        }
        if (!end2 || *end2 != '\0' || val < 0 || val > 255) {
            printf("invalid value: %s\n", argv[3]);
            return 1;
        }

        for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
            s_fb[row][m] = (uint8_t)val;
        }
        esp_err_t err = fb_flush_row((int)row);
        if (err != ESP_OK) {
            printf("row failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "row4") == 0) {
        blink_off();
        if (argc < 7) {
            printf("usage: matrix row4 <0..7> <b0> <b1> <b2> <b3>\n");
            return 1;
        }
        char *end0 = NULL;
        long row = strtol(argv[2], &end0, 0);
        if (!end0 || *end0 != '\0' || row < 0 || row > 7) {
            printf("invalid row: %s\n", argv[2]);
            return 1;
        }

        for (int i = 0; i < 4; i++) {
            char *end = NULL;
            long v = strtol(argv[3 + i], &end, 0);
            if (!end || *end != '\0' || v < 0 || v > 255) {
                printf("invalid byte: %s\n", argv[3 + i]);
                return 1;
            }
            s_fb[row][i] = (uint8_t)v;
        }

        esp_err_t err = fb_flush_row((int)row);
        if (err != ESP_OK) {
            printf("row4 failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "px") == 0) {
        blink_off();
        if (argc < 5) {
            printf("usage: matrix px <0..%d> <0..7> <0|1>\n", fb_width() - 1);
            return 1;
        }
        char *endx = NULL;
        char *endy = NULL;
        char *endv = NULL;
        long x = strtol(argv[2], &endx, 0);
        long y = strtol(argv[3], &endy, 0);
        long v = strtol(argv[4], &endv, 0);
        if (!endx || *endx != '\0' || x < 0 || x >= fb_width()) {
            printf("invalid x: %s\n", argv[2]);
            return 1;
        }
        if (!endy || *endy != '\0' || y < 0 || y > 7) {
            printf("invalid y: %s\n", argv[3]);
            return 1;
        }
        if (!endv || *endv != '\0' || (v != 0 && v != 1)) {
            printf("invalid value: %s\n", argv[4]);
            return 1;
        }

        const int module = x_to_module((int)x);
        const uint8_t mask = x_to_bit((int)x);
        if (v) {
            s_fb[y][module] |= mask;
        } else {
            s_fb[y][module] &= (uint8_t)~mask;
        }

        esp_err_t err = fb_flush_row((int)y);
        if (err != ESP_OK) {
            printf("px failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "pattern") == 0) {
        blink_off();
        if (argc < 3) {
            printf("usage: matrix pattern rows|cols|diag|checker|off|ids\n");
            printf("       matrix pattern onehot <0..3>\n");
            return 1;
        }
        fb_clear();

        if (strcmp(argv[2], "off") == 0) {
            // already cleared
        } else if (strcmp(argv[2], "rows") == 0) {
            for (int y = 0; y < 8; y++) {
                for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
                    s_fb[y][m] = (y & 1) ? 0xFF : 0x00;
                }
            }
        } else if (strcmp(argv[2], "cols") == 0) {
            for (int y = 0; y < 8; y++) {
                for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
                    s_fb[y][m] = 0xAA;
                }
            }
        } else if (strcmp(argv[2], "diag") == 0) {
            for (int i = 0; i < 8; i++) {
                s_fb[i][0] = (uint8_t)(1u << i);
            }
        } else if (strcmp(argv[2], "checker") == 0) {
            for (int y = 0; y < 8; y++) {
                for (int m = 0; m < MAX7219_DEFAULT_CHAIN_LEN; m++) {
                    s_fb[y][m] = (y & 1) ? 0xAA : 0x55;
                }
            }
        } else if (strcmp(argv[2], "ids") == 0) {
            fb_set_ids_pattern();
        } else if (strcmp(argv[2], "onehot") == 0) {
            if (argc < 4) {
                printf("usage: matrix pattern onehot <0..3>\n");
                return 1;
            }
            char *end = NULL;
            long m = strtol(argv[3], &end, 0);
            if (!end || *end != '\0' || m < 0 || m >= MAX7219_DEFAULT_CHAIN_LEN) {
                printf("invalid module: %s\n", argv[3]);
                return 1;
            }
            for (int y = 0; y < 8; y++) {
                s_fb[y][(int)m] = 0xFF;
            }
        } else {
            printf("usage: matrix pattern rows|cols|diag|checker|off|ids\n");
            return 1;
        }

        esp_err_t err = fb_flush_all();
        if (err != ESP_OK) {
            printf("pattern failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    printf("unknown subcommand: %s (try: matrix help)\n", argv[1]);
    return 1;
}

static void register_commands(void) {
    esp_console_cmd_t cmd = {0};
    cmd.command = "matrix";
    cmd.help = "MAX7219 LED matrix control: matrix help";
    cmd.func = &cmd_matrix;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void matrix_console_start(void) {
#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no REPL started");
    return;
#else
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "matrix> ";
    repl_config.task_stack_size = 4096;

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over USB Serial/JTAG (try: help, matrix help)");
    try_autoinit();
#endif
}
