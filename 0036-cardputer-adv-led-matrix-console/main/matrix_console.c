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
static bool s_flip_vertical = true;

static int s_chain_len_cfg = MAX7219_DEFAULT_CHAIN_LEN;

static uint8_t s_fb[8][MAX7219_MAX_CHAIN_LEN] = {0};

static esp_err_t fb_flush_all(void);
static void try_autoinit(void);
static int x_to_module(int x);
static uint8_t x_to_bit(int x);

static SemaphoreHandle_t s_matrix_mu;
static TaskHandle_t s_blink_task;
static bool s_blink_enabled = false;
static uint32_t s_blink_on_ms = 250;
static uint32_t s_blink_off_ms = 250;
static uint8_t s_blink_saved_fb[8][MAX7219_MAX_CHAIN_LEN] = {0};
static bool s_blink_have_saved_fb = false;

static TaskHandle_t s_scroll_task;
static bool s_scroll_enabled = false;
static bool s_scroll_wave = false;
static uint32_t s_scroll_fps = 15;
static uint32_t s_scroll_pause_ms = 250;
static uint8_t *s_scroll_text_cols;
static int s_scroll_text_w = 0;
static uint8_t s_scroll_saved_fb[8][MAX7219_MAX_CHAIN_LEN] = {0};
static bool s_scroll_have_saved_fb = false;

typedef enum {
    TEXT_ANIM_NONE = 0,
    TEXT_ANIM_DROP_BOUNCE,
    TEXT_ANIM_WAVE,
    TEXT_ANIM_FLIPBOARD,
} text_anim_mode_t;

static TaskHandle_t s_text_anim_task;
static bool s_text_anim_enabled = false;
static text_anim_mode_t s_text_anim_mode = TEXT_ANIM_NONE;
static uint32_t s_text_anim_fps = 15;
static uint32_t s_text_anim_pause_ms = 250;
static uint32_t s_text_anim_hold_ms = 750;
static char s_text_anim_text[65];
static int s_text_anim_len = 0;
static uint8_t s_text_anim_saved_fb[8][MAX7219_MAX_CHAIN_LEN] = {0};
static bool s_text_anim_have_saved_fb = false;

#define FLIPBOARD_MAX_TEXTS 8
static char s_flipboard_buf[129];
static const char *s_flipboard_texts[FLIPBOARD_MAX_TEXTS];
static int s_flipboard_count = 0;

static void matrix_lock(void) {
    if (s_matrix_mu) xSemaphoreTake(s_matrix_mu, portMAX_DELAY);
}

static void matrix_unlock(void) {
    if (s_matrix_mu) xSemaphoreGive(s_matrix_mu);
}

static int chain_len_active(void) {
    if (s_matrix_ready && s_matrix.chain_len > 0) return s_matrix.chain_len;
    if (s_chain_len_cfg > 0) return s_chain_len_cfg;
    return MAX7219_DEFAULT_CHAIN_LEN;
}

static int width_active(void) {
    return 8 * chain_len_active();
}

static void print_matrix_help(void) {
    printf("matrix commands:\n");
    printf("  matrix init\n");
    printf("  matrix status\n");
    printf("  matrix clear\n");
    printf("  matrix test on|off\n");
    printf("  matrix safe on|off                            (RAM-based full-on/full-off; good for wiring bring-up)\n");
    printf("  matrix chain [n]                              (get/set chained modules, max %d)\n", MAX7219_MAX_CHAIN_LEN);
    printf("  matrix text <TEXT>                            (renders 1 char per module)\n");
    printf("  matrix scroll on <TEXT> [fps] [pause_ms]      (smooth 1px scroll; A-Z 0-9 space)\n");
    printf("  matrix scroll wave <TEXT> [fps] [pause_ms]    (scroll + wave)\n");
    printf("  matrix scroll off\n");
    printf("  matrix scroll status\n");
    printf("  matrix anim drop <TEXT> [fps] [pause_ms]      (drop + bounce into place)\n");
    printf("  matrix anim wave <TEXT> [fps]                (wave text in place)\n");
    printf("  matrix anim flip <A|B|C...> [fps] [hold_ms]   (flipboard between texts)\n");
    printf("  matrix anim off\n");
    printf("  matrix anim status\n");
    printf("  matrix intensity <0..15>\n");
    printf("  matrix spi [hz]                               (get/set SPI clock; default is compile-time)\n");
    printf("  matrix reverse on|off\n");
    printf("  matrix flipv on|off                           (flip vertically; fixes upside-down glyphs)\n");
    printf("  matrix row <0..7> <0x00..0xff>                 (sets this row on all modules)\n");
    printf("  matrix rowm <0..7> <b0..bN-1>                  (one byte per module, N=chain_len)\n");
    printf("  matrix row4 <0..7> <b0> <b1> <b2> <b3>          (compat: set first 4 modules)\n");
    printf("  matrix px <0..%d> <0..7> <0|1>                   (pixel on the %dx8 strip)\n",
           width_active() - 1,
           width_active());
    printf("  matrix pattern rows|cols|diag|checker|off|ids\n");
    printf("  matrix pattern onehot <0..N-1>                   (turn on exactly one module)\n");
    printf("  matrix blink on [on_ms] [off_ms]                 (loop full-on/full-off)\n");
    printf("  matrix blink off\n");
    printf("  matrix blink status\n");
}

static void fb_clear(void) {
    memset(s_fb, 0, sizeof(s_fb));
}

static void fb_from_cols(const uint8_t *cols, int width) {
    if (!cols) return;
    if (width <= 0) return;
    if (width > (8 * MAX7219_MAX_CHAIN_LEN)) width = 8 * MAX7219_MAX_CHAIN_LEN;
    fb_clear();
    for (int x = 0; x < width; x++) {
        const int module = x_to_module(x);
        const uint8_t mask = x_to_bit(x);
        const uint8_t col = cols[x];
        for (int y = 0; y < 8; y++) {
            if (col & (uint8_t)(1u << y)) s_fb[y][module] |= mask;
        }
    }
}

static void fb_set_ids_pattern(void) {
    const int n = chain_len_active();
    for (int y = 0; y < 8; y++) {
        for (int m = 0; m < n; m++) {
            s_fb[y][m] = (uint8_t)(1u << ((m + y) & 7));
        }
    }
}

static char ascii_upper(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
    return c;
}

static const int8_t s_wave16[16] = {0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1, -2, -1};

static uint8_t col_shift_y(uint8_t bits, int8_t y_offset) {
    if (y_offset > 0) {
        if (y_offset > 7) y_offset = 7;
        return (uint8_t)(bits << y_offset);
    }
    if (y_offset < 0) {
        int s = -y_offset;
        if (s > 7) s = 7;
        return (uint8_t)(bits >> s);
    }
    return bits;
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
    return width_active();
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
    const int n = chain_len_active();
    uint8_t phy[MAX7219_MAX_CHAIN_LEN] = {0};
    for (int m = 0; m < n && m < MAX7219_MAX_CHAIN_LEN; m++) {
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

        const int n = chain_len_active();
        uint8_t ones[8][MAX7219_MAX_CHAIN_LEN];
        uint8_t zeros[8][MAX7219_MAX_CHAIN_LEN];
        memset(ones, 0, sizeof(ones));
        memset(zeros, 0, sizeof(zeros));
        for (int y = 0; y < 8; y++) {
            for (int m = 0; m < n; m++) {
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

static void scroll_task(void *arg) {
    (void)arg;
    uint32_t frame = 0;
    for (;;) {
        if (!s_scroll_enabled) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            frame = 0;
            continue;
        }

        if (s_scroll_pause_ms) vTaskDelay(pdMS_TO_TICKS(s_scroll_pause_ms));

        const uint32_t frame_ms = (s_scroll_fps > 0) ? (1000u / s_scroll_fps) : 66u;
        const int width = fb_width();
        int pos = width;
        for (;;) {
            if (!s_scroll_enabled) break;

            uint8_t cols[8 * MAX7219_MAX_CHAIN_LEN] = {0};
            for (int x = 0; x < width; x++) {
                const int t = x - pos;
                if (t >= 0 && t < s_scroll_text_w && s_scroll_text_cols) {
                    uint8_t bits = s_scroll_text_cols[t];
                    if (s_scroll_wave) {
                        const int char_idx = t / 6;
                        const int8_t yoff = s_wave16[(int)((frame + (uint32_t)(char_idx * 2)) & 0x0F)];
                        bits = col_shift_y(bits, yoff);
                    }
                    cols[x] = bits;
                }
            }

            matrix_lock();
            fb_from_cols(cols, width);
            matrix_unlock();
            (void)fb_flush_all();

            vTaskDelay(pdMS_TO_TICKS(frame_ms));
            pos--;
            frame++;
            if (pos < -s_scroll_text_w) {
                pos = width;
                if (s_scroll_pause_ms) vTaskDelay(pdMS_TO_TICKS(s_scroll_pause_ms));
            }
        }
    }
}

static void scroll_ensure_task(void) {
    if (s_scroll_task) return;
    xTaskCreate(&scroll_task, "matrix_scroll", 4096, NULL, 2, &s_scroll_task);
}

static void scroll_free_text(void) {
    if (s_scroll_text_cols) free(s_scroll_text_cols);
    s_scroll_text_cols = NULL;
    s_scroll_text_w = 0;
}

static bool char_supported_for_scroll(char c) {
    c = ascii_upper(c);
    if (c == ' ') return true;
    if (c >= '0' && c <= '9') return true;
    if (c >= 'A' && c <= 'Z') return true;
    return false;
}

static void render_text_centered_cols(uint8_t *out_cols,
                                     int width,
                                     const char *text,
                                     int text_len,
                                     const int8_t *y_offsets) {
    if (!out_cols) return;
    if (width <= 0) return;
    if (!text || text_len <= 0) {
        memset(out_cols, 0, (size_t)width);
        return;
    }

    const int cell = 6; // 5 cols + 1 spacing
    const int total_w = text_len * cell;
    const int start_x = (width - total_w) / 2;
    memset(out_cols, 0, (size_t)width);

    for (int i = 0; i < text_len; i++) {
        char c = ascii_upper(text[i]);
        if (!char_supported_for_scroll(c)) c = ' ';
        const uint8_t *g = font5x7_get_cols(c);
        const int8_t yoff = y_offsets ? y_offsets[i] : 0;
        const int base_x = start_x + i * cell;
        for (int col = 0; col < 5; col++) {
            const int x = base_x + col;
            if (x < 0 || x >= width) continue;
            uint8_t bits = g[col];
            if (yoff > 0) {
                bits = (uint8_t)(bits << (yoff > 7 ? 7 : yoff));
            } else if (yoff < 0) {
                const int s = (-yoff > 7) ? 7 : -yoff;
                bits = (uint8_t)(bits >> s);
            }
            out_cols[x] |= bits;
        }
    }
}

static uint16_t q8_ease_in_quad(uint16_t t_q8) {
    uint32_t t = t_q8;
    return (uint16_t)((t * t) >> 8);
}

static uint16_t q8_ease_out_quad(uint16_t t_q8) {
    uint32_t inv = 256u - (uint32_t)t_q8;
    uint32_t inv2 = (inv * inv) >> 8;
    return (uint16_t)(256u - inv2);
}

static uint8_t col_scale_y(uint8_t bits, uint16_t scale_y_q8) {
    if (scale_y_q8 < 16) return 0;
    const int center = 3;
    uint8_t out = 0;
    for (int y = 0; y < 8; y++) {
        const int dy = y - center;
        const int src_y = center + (int)((dy * 256) / (int)scale_y_q8);
        if (src_y < 0 || src_y > 7) continue;
        if ((bits >> src_y) & 1u) out |= (uint8_t)(1u << y);
    }
    return out;
}

static int flipboard_set_texts(const char *spec) {
    memset(s_flipboard_buf, 0, sizeof(s_flipboard_buf));
    s_flipboard_count = 0;
    if (!spec || *spec == '\0') return 0;

    size_t n = strlen(spec);
    if (n >= sizeof(s_flipboard_buf)) n = sizeof(s_flipboard_buf) - 1;
    memcpy(s_flipboard_buf, spec, n);

    // Sanitize in-place: uppercase and unsupported -> space.
    for (size_t i = 0; i < n; i++) {
        if (s_flipboard_buf[i] == '|') continue;
        char c = ascii_upper(s_flipboard_buf[i]);
        if (!char_supported_for_scroll(c)) c = ' ';
        s_flipboard_buf[i] = c;
    }

    // Split on '|'
    char *p = s_flipboard_buf;
    while (p && *p && s_flipboard_count < FLIPBOARD_MAX_TEXTS) {
        char *sep = strchr(p, '|');
        if (sep) *sep = '\0';
        // Skip empty segments
        while (*p == ' ') p++;
        if (*p != '\0') {
            s_flipboard_texts[s_flipboard_count++] = p;
        }
        p = sep ? (sep + 1) : NULL;
    }

    if (s_flipboard_count == 1 && s_flipboard_count < FLIPBOARD_MAX_TEXTS) {
        s_flipboard_texts[s_flipboard_count++] = " ";
    }

    return s_flipboard_count;
}

static void scroll_set_text(const char *text) {
    scroll_free_text();
    if (!text) return;

    size_t len = strlen(text);
    if (len == 0) return;
    if (len > 128) len = 128;

    const int cell = 6; // 5 cols + 1 spacing
    const int w = (int)(len * (size_t)cell);
    uint8_t *cols = (uint8_t *)calloc((size_t)w, 1);
    if (!cols) return;

    int x = 0;
    for (size_t i = 0; i < len; i++) {
        char c = ascii_upper(text[i]);
        if (!char_supported_for_scroll(c)) c = ' ';
        const uint8_t *g = font5x7_get_cols(c);
        for (int col = 0; col < 5; col++) cols[x++] = g[col];
        cols[x++] = 0x00;
    }

    s_scroll_text_cols = cols;
    s_scroll_text_w = w;
}

static void scroll_on(const char *text, uint32_t fps, uint32_t pause_ms, bool wave) {
    if (!s_matrix_ready) {
        printf("matrix not initialized (run: matrix init)\n");
        return;
    }
    scroll_set_text(text);
    if (!s_scroll_text_cols || s_scroll_text_w <= 0) {
        printf("scroll: empty/unsupported text\n");
        scroll_free_text();
        return;
    }

    if (!s_scroll_have_saved_fb) {
        matrix_lock();
        memcpy(s_scroll_saved_fb, s_fb, sizeof(s_fb));
        matrix_unlock();
        s_scroll_have_saved_fb = true;
    }

    if (fps < 1) fps = 1;
    if (fps > 60) fps = 60;
    if (pause_ms > 60000) pause_ms = 60000;
    s_scroll_fps = fps;
    s_scroll_pause_ms = pause_ms;
    s_scroll_wave = wave;
    s_scroll_enabled = true;
    scroll_ensure_task();
    xTaskNotifyGive(s_scroll_task);
}

static void scroll_off(void) {
    s_scroll_enabled = false;
    s_scroll_wave = false;
    if (s_scroll_have_saved_fb) {
        matrix_lock();
        memcpy(s_fb, s_scroll_saved_fb, sizeof(s_fb));
        matrix_unlock();
        (void)fb_flush_all();
        s_scroll_have_saved_fb = false;
    }
    scroll_free_text();
    if (s_scroll_task) xTaskNotifyGive(s_scroll_task);
}

static void text_anim_task(void *arg) {
    (void)arg;

    static const int8_t wave16[16] = {0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1, -2, -1};
    static const int8_t drop_seq[] = {-7, -6, -5, -4, -3, -2, -1, 0, 1, 0, 1, 0, 0};
    const int drop_len = (int)(sizeof(drop_seq) / sizeof(drop_seq[0]));
    const int drop_stagger = 2;
    const int drop_hold_frames = 20;
    const int drop_gap_frames = 10;

    uint32_t frame = 0;

    for (;;) {
        if (!s_text_anim_enabled || s_text_anim_mode == TEXT_ANIM_NONE) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            frame = 0;
            continue;
        }

        const uint32_t fps = (s_text_anim_fps > 0) ? s_text_anim_fps : 15;
        const uint32_t frame_ms = 1000u / (fps > 0 ? fps : 15);
        const uint32_t pause_ms = s_text_anim_pause_ms;

        const int width = width_active();
        const int max_cols = 8 * MAX7219_MAX_CHAIN_LEN;
        uint8_t cols[max_cols];
        int8_t yoffs[64];

        if (s_text_anim_mode == TEXT_ANIM_FLIPBOARD) {
            if (s_flipboard_count < 2) {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
        } else {
            if (s_text_anim_len <= 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
        }

        if (s_text_anim_mode == TEXT_ANIM_WAVE) {
            for (;;) {
                if (!s_text_anim_enabled || s_text_anim_mode != TEXT_ANIM_WAVE) break;
                for (int i = 0; i < s_text_anim_len && i < (int)(sizeof(yoffs) / sizeof(yoffs[0])); i++) {
                    const int idx = (int)((frame + (uint32_t)(i * 2)) & 0x0F);
                    yoffs[i] = wave16[idx];
                }
                render_text_centered_cols(cols, width, s_text_anim_text, s_text_anim_len, yoffs);
                matrix_lock();
                fb_from_cols(cols, width);
                matrix_unlock();
                (void)fb_flush_all();
                frame++;
                vTaskDelay(pdMS_TO_TICKS(frame_ms));
            }
            continue;
        }

        if (s_text_anim_mode == TEXT_ANIM_DROP_BOUNCE) {
            for (;;) {
                if (!s_text_anim_enabled || s_text_anim_mode != TEXT_ANIM_DROP_BOUNCE) break;
                const int text_frames = drop_len + drop_stagger * (s_text_anim_len - 1);
                const int cycle = text_frames + drop_hold_frames + drop_gap_frames;
                const int f = (int)(frame % (uint32_t)cycle);

                if (f >= (text_frames + drop_hold_frames)) {
                    memset(cols, 0, (size_t)width);
                } else {
                    for (int i = 0; i < s_text_anim_len && i < (int)(sizeof(yoffs) / sizeof(yoffs[0])); i++) {
                        const int t = f - i * drop_stagger;
                        int8_t off = -7;
                        if (t < 0) {
                            off = -7;
                        } else if (t >= drop_len) {
                            off = 0;
                        } else {
                            off = drop_seq[t];
                        }
                        yoffs[i] = off;
                    }
                    render_text_centered_cols(cols, width, s_text_anim_text, s_text_anim_len, yoffs);
                }

                matrix_lock();
                fb_from_cols(cols, width);
                matrix_unlock();
                (void)fb_flush_all();

                frame++;
                vTaskDelay(pdMS_TO_TICKS(frame_ms));
                if (frame % (uint32_t)cycle == 0 && pause_ms) vTaskDelay(pdMS_TO_TICKS(pause_ms));
            }
            continue;
        }

        if (s_text_anim_mode == TEXT_ANIM_FLIPBOARD) {
            const int flip_frames = 12;
            const int half = flip_frames / 2;
            int hold_frames = (int)((s_text_anim_hold_ms * fps) / 1000u);
            if (hold_frames < 1) hold_frames = 1;
            const int segment = hold_frames + flip_frames;
            const int cycle = s_flipboard_count * segment;

            for (;;) {
                if (!s_text_anim_enabled || s_text_anim_mode != TEXT_ANIM_FLIPBOARD) break;

                const int f = (int)(frame % (uint32_t)cycle);
                const int idx = f / segment;
                const int segf = f % segment;
                const char *cur = s_flipboard_texts[idx % s_flipboard_count];
                const char *nxt = s_flipboard_texts[(idx + 1) % s_flipboard_count];

                const char *display = cur;
                uint16_t scale_q8 = 256;
                if (segf >= hold_frames) {
                    const int flipf = segf - hold_frames;
                    if (flipf < half) {
                        display = cur;
                        const uint16_t t = (uint16_t)((flipf * 256) / half);
                        const uint16_t eased = q8_ease_in_quad(t);
                        scale_q8 = (uint16_t)(256u - eased);
                    } else {
                        display = nxt;
                        const uint16_t t = (uint16_t)(((flipf - half) * 256) / half);
                        scale_q8 = q8_ease_out_quad(t);
                    }
                }

                int len = (int)strlen(display);
                if (len > 64) len = 64;
                render_text_centered_cols(cols, width, display, len, NULL);
                for (int x = 0; x < width; x++) cols[x] = col_scale_y(cols[x], scale_q8);

                matrix_lock();
                fb_from_cols(cols, width);
                matrix_unlock();
                (void)fb_flush_all();

                frame++;
                vTaskDelay(pdMS_TO_TICKS(frame_ms));
            }
            continue;
        }
    }
}

static void text_anim_ensure_task(void) {
    if (s_text_anim_task) return;
    xTaskCreate(&text_anim_task, "matrix_anim", 4096, NULL, 2, &s_text_anim_task);
}

static void text_anim_set_text(const char *text) {
    memset(s_text_anim_text, 0, sizeof(s_text_anim_text));
    s_text_anim_len = 0;
    if (!text) return;
    size_t len = strlen(text);
    if (len > 64) len = 64;
    for (size_t i = 0; i < len; i++) {
        char c = ascii_upper(text[i]);
        if (!char_supported_for_scroll(c)) c = ' ';
        s_text_anim_text[i] = c;
    }
    s_text_anim_len = (int)len;
}

static void text_anim_on(text_anim_mode_t mode, const char *text, uint32_t fps, uint32_t pause_ms) {
    if (!s_matrix_ready) {
        printf("matrix not initialized (run: matrix init)\n");
        return;
    }
    if (mode == TEXT_ANIM_NONE) return;
    if (mode == TEXT_ANIM_FLIPBOARD) return;
    text_anim_set_text(text);
    if (s_text_anim_len <= 0) {
        printf("anim: empty text\n");
        return;
    }

    if (!s_text_anim_have_saved_fb) {
        matrix_lock();
        memcpy(s_text_anim_saved_fb, s_fb, sizeof(s_fb));
        matrix_unlock();
        s_text_anim_have_saved_fb = true;
    }

    if (fps < 1) fps = 1;
    if (fps > 60) fps = 60;
    if (pause_ms > 60000) pause_ms = 60000;
    s_text_anim_fps = fps;
    s_text_anim_pause_ms = pause_ms;
    s_text_anim_mode = mode;
    s_text_anim_enabled = true;
    text_anim_ensure_task();
    xTaskNotifyGive(s_text_anim_task);
}

static void text_anim_on_flipboard(const char *spec, uint32_t fps, uint32_t hold_ms) {
    if (!s_matrix_ready) {
        printf("matrix not initialized (run: matrix init)\n");
        return;
    }
    if (flipboard_set_texts(spec) < 2) {
        printf("anim flip: need at least 1 text (use A|B|C...)\n");
        return;
    }

    if (!s_text_anim_have_saved_fb) {
        matrix_lock();
        memcpy(s_text_anim_saved_fb, s_fb, sizeof(s_fb));
        matrix_unlock();
        s_text_anim_have_saved_fb = true;
    }

    if (fps < 1) fps = 1;
    if (fps > 60) fps = 60;
    if (hold_ms > 60000) hold_ms = 60000;
    s_text_anim_fps = fps;
    s_text_anim_hold_ms = hold_ms;
    s_text_anim_pause_ms = 0;
    s_text_anim_mode = TEXT_ANIM_FLIPBOARD;
    s_text_anim_enabled = true;
    text_anim_ensure_task();
    xTaskNotifyGive(s_text_anim_task);
}

static void text_anim_off(void) {
    s_text_anim_enabled = false;
    s_text_anim_mode = TEXT_ANIM_NONE;
    s_flipboard_count = 0;
    if (s_text_anim_have_saved_fb) {
        matrix_lock();
        memcpy(s_fb, s_text_anim_saved_fb, sizeof(s_fb));
        matrix_unlock();
        (void)fb_flush_all();
        s_text_anim_have_saved_fb = false;
    }
    if (s_text_anim_task) xTaskNotifyGive(s_text_anim_task);
}

static void stop_animations(void) {
    blink_off();
    scroll_off();
    text_anim_off();
}

static void try_autoinit(void) {
    if (s_matrix_ready) return;

    esp_err_t err = max7219_open(&s_matrix,
                                 MAX7219_DEFAULT_SPI_HOST,
                                 MAX7219_DEFAULT_PIN_SCK,
                                 MAX7219_DEFAULT_PIN_MOSI,
                                 MAX7219_DEFAULT_PIN_CS,
                                 s_chain_len_cfg);
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
                                     MAX7219_DEFAULT_PIN_MOSI, MAX7219_DEFAULT_PIN_CS, s_chain_len_cfg);
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

    if (strcmp(argv[1], "status") == 0) {
        printf("ok: chain_cfg=%d chain_len=%d width=%d spi_hz=%d reverse_modules=%s flipv=%s blink=%s on_ms=%u off_ms=%u scroll=%s fps=%u pause_ms=%u\n",
               s_chain_len_cfg,
               s_matrix_ready ? s_matrix.chain_len : 0,
               width_active(),
               s_matrix_ready ? s_matrix.clock_hz : 0,
               s_reverse_modules ? "on" : "off",
               s_flip_vertical ? "on" : "off",
               s_blink_enabled ? "on" : "off",
               (unsigned)s_blink_on_ms,
               (unsigned)s_blink_off_ms,
               s_scroll_enabled ? "on" : "off",
               (unsigned)s_scroll_fps,
               (unsigned)s_scroll_pause_ms);
        return 0;
    }

    if (strcmp(argv[1], "chain") == 0) {
        if (argc < 3) {
            printf("ok: chain_cfg=%d\n", s_chain_len_cfg);
            return 0;
        }
        char *end = NULL;
        long v = strtol(argv[2], &end, 0);
        if (!end || *end != '\0' || v < 1 || v > MAX7219_MAX_CHAIN_LEN) {
            printf("invalid chain len: %s (expected 1..%d)\n", argv[2], MAX7219_MAX_CHAIN_LEN);
            return 1;
        }
        stop_animations();
        s_chain_len_cfg = (int)v;
        if (s_matrix_ready) {
            esp_err_t err = max7219_open(&s_matrix, MAX7219_DEFAULT_SPI_HOST, MAX7219_DEFAULT_PIN_SCK,
                                         MAX7219_DEFAULT_PIN_MOSI, MAX7219_DEFAULT_PIN_CS, s_chain_len_cfg);
            if (err != ESP_OK) {
                printf("chain failed: %s\n", esp_err_to_name(err));
                return 1;
            }
            err = max7219_init(&s_matrix);
            if (err != ESP_OK) {
                printf("chain failed: %s\n", esp_err_to_name(err));
                return 1;
            }
            fb_clear();
            fb_set_ids_pattern();
            (void)fb_flush_all();
        }
        printf("ok: chain_cfg=%d\n", s_chain_len_cfg);
        return 0;
    }

    if (!s_matrix_ready) {
        printf("matrix not initialized (run: matrix init)\n");
        return 1;
    }

    if (strcmp(argv[1], "spi") == 0) {
        stop_animations();
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
        scroll_off();
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

    if (strcmp(argv[1], "scroll") == 0) {
        if (argc < 3) {
            printf("usage: matrix scroll on <TEXT> [fps] [pause_ms]\n");
            printf("       matrix scroll wave <TEXT> [fps] [pause_ms]\n");
            printf("       matrix scroll off\n");
            printf("       matrix scroll status\n");
            return 1;
        }
        if (strcmp(argv[2], "status") == 0) {
            printf("ok: scroll=%s mode=%s fps=%u pause_ms=%u\n",
                   s_scroll_enabled ? "on" : "off",
                   s_scroll_wave ? "wave" : "plain",
                   (unsigned)s_scroll_fps,
                   (unsigned)s_scroll_pause_ms);
            return 0;
        }
        if (strcmp(argv[2], "off") == 0) {
            scroll_off();
            printf("ok\n");
            return 0;
        }
        if (strcmp(argv[2], "on") == 0 || strcmp(argv[2], "wave") == 0) {
            const bool wave = (strcmp(argv[2], "wave") == 0);
            if (argc < 4) {
                printf("usage: matrix scroll %s <TEXT> [fps] [pause_ms]\n", wave ? "wave" : "on");
                return 1;
            }
            uint32_t fps = s_scroll_fps;
            uint32_t pause_ms = s_scroll_pause_ms;
            if (argc >= 5) {
                char *end = NULL;
                long v = strtol(argv[4], &end, 0);
                if (!end || *end != '\0' || v < 1 || v > 60) {
                    printf("invalid fps: %s\n", argv[4]);
                    return 1;
                }
                fps = (uint32_t)v;
            }
            if (argc >= 6) {
                char *end = NULL;
                long v = strtol(argv[5], &end, 0);
                if (!end || *end != '\0' || v < 0 || v > 60000) {
                    printf("invalid pause_ms: %s\n", argv[5]);
                    return 1;
                }
                pause_ms = (uint32_t)v;
            }
            blink_off();
            scroll_on(argv[3], fps, pause_ms, wave);
            if (s_scroll_enabled) printf("ok\n");
            return s_scroll_enabled ? 0 : 1;
        }
        printf("usage: matrix scroll on <TEXT> [fps] [pause_ms]\n");
        printf("       matrix scroll wave <TEXT> [fps] [pause_ms]\n");
        printf("       matrix scroll off\n");
        printf("       matrix scroll status\n");
        return 1;
    }

    if (strcmp(argv[1], "anim") == 0) {
        if (argc < 3) {
            printf("usage: matrix anim drop <TEXT> [fps] [pause_ms]\n");
            printf("       matrix anim wave <TEXT> [fps]\n");
            printf("       matrix anim flip <A|B|C...> [fps] [hold_ms]\n");
            printf("       matrix anim off\n");
            printf("       matrix anim status\n");
            return 1;
        }
        if (strcmp(argv[2], "status") == 0) {
            const char *mode = "off";
            if (s_text_anim_mode == TEXT_ANIM_DROP_BOUNCE) mode = "drop";
            if (s_text_anim_mode == TEXT_ANIM_WAVE) mode = "wave";
            if (s_text_anim_mode == TEXT_ANIM_FLIPBOARD) mode = "flip";
            printf("ok: anim=%s fps=%u pause_ms=%u hold_ms=%u text_len=%d flip_count=%d\n",
                   s_text_anim_enabled ? mode : "off",
                   (unsigned)s_text_anim_fps,
                   (unsigned)s_text_anim_pause_ms,
                   (unsigned)s_text_anim_hold_ms,
                   s_text_anim_len,
                   s_flipboard_count);
            return 0;
        }
        if (strcmp(argv[2], "off") == 0) {
            text_anim_off();
            printf("ok\n");
            return 0;
        }
        if (strcmp(argv[2], "flip") == 0) {
            if (argc < 4) {
                printf("usage: matrix anim flip <A|B|C...> [fps] [hold_ms]\n");
                return 1;
            }
            uint32_t fps = s_text_anim_fps;
            uint32_t hold_ms = s_text_anim_hold_ms;
            if (argc >= 5) {
                char *end = NULL;
                long v = strtol(argv[4], &end, 0);
                if (!end || *end != '\0' || v < 1 || v > 60) {
                    printf("invalid fps: %s\n", argv[4]);
                    return 1;
                }
                fps = (uint32_t)v;
            }
            if (argc >= 6) {
                char *end = NULL;
                long v = strtol(argv[5], &end, 0);
                if (!end || *end != '\0' || v < 0 || v > 60000) {
                    printf("invalid hold_ms: %s\n", argv[5]);
                    return 1;
                }
                hold_ms = (uint32_t)v;
            }
            stop_animations();
            text_anim_on_flipboard(argv[3], fps, hold_ms);
            if (s_text_anim_enabled) printf("ok\n");
            return s_text_anim_enabled ? 0 : 1;
        }
        if (strcmp(argv[2], "drop") == 0 || strcmp(argv[2], "wave") == 0) {
            if (argc < 4) {
                printf("usage: matrix anim %s <TEXT> [fps] [pause_ms]\n", argv[2]);
                return 1;
            }
            uint32_t fps = s_text_anim_fps;
            uint32_t pause_ms = s_text_anim_pause_ms;
            if (argc >= 5) {
                char *end = NULL;
                long v = strtol(argv[4], &end, 0);
                if (!end || *end != '\0' || v < 1 || v > 60) {
                    printf("invalid fps: %s\n", argv[4]);
                    return 1;
                }
                fps = (uint32_t)v;
            }
            if (argc >= 6) {
                char *end = NULL;
                long v = strtol(argv[5], &end, 0);
                if (!end || *end != '\0' || v < 0 || v > 60000) {
                    printf("invalid pause_ms: %s\n", argv[5]);
                    return 1;
                }
                pause_ms = (uint32_t)v;
            }
            stop_animations();
            const text_anim_mode_t mode = (strcmp(argv[2], "drop") == 0) ? TEXT_ANIM_DROP_BOUNCE : TEXT_ANIM_WAVE;
            if (mode == TEXT_ANIM_WAVE) pause_ms = 0;
            text_anim_on(mode, argv[3], fps, pause_ms);
            if (s_text_anim_enabled) printf("ok\n");
            return s_text_anim_enabled ? 0 : 1;
        }
        printf("usage: matrix anim drop <TEXT> [fps] [pause_ms]\n");
        printf("       matrix anim wave <TEXT> [fps]\n");
        printf("       matrix anim flip <A|B|C...> [fps] [hold_ms]\n");
        printf("       matrix anim off\n");
        printf("       matrix anim status\n");
        return 1;
    }

    if (strcmp(argv[1], "clear") == 0) {
        stop_animations();
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
        stop_animations();
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
        stop_animations();
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

        const int n = chain_len_active();
        for (int y = 0; y < 8; y++) {
            for (int m = 0; m < n; m++) {
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
        stop_animations();

        if (argc < 3) {
            printf("usage: matrix text <TEXT>\n");
            return 1;
        }

        const char *s = argv[2];
        const size_t slen = strlen(s);
        const int n = chain_len_active();
        for (int m = 0; m < n; m++) {
            const char c = ((size_t)m < slen) ? s[m] : ' ';
            uint8_t rows[8];
            render_char_8x8_rows(c, rows);
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
        stop_animations();
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
        stop_animations();
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
        stop_animations();
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
        stop_animations();
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

        const int n = chain_len_active();
        for (int m = 0; m < n; m++) {
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

    if (strcmp(argv[1], "rowm") == 0) {
        stop_animations();
        const int n = chain_len_active();
        if (argc < 3 + n) {
            printf("usage: matrix rowm <0..7> <b0..b%d>\n", n - 1);
            return 1;
        }
        char *end0 = NULL;
        long row = strtol(argv[2], &end0, 0);
        if (!end0 || *end0 != '\0' || row < 0 || row > 7) {
            printf("invalid row: %s\n", argv[2]);
            return 1;
        }
        for (int m = 0; m < n; m++) {
            char *end = NULL;
            long v = strtol(argv[3 + m], &end, 0);
            if (!end || *end != '\0' || v < 0 || v > 255) {
                printf("invalid byte[%d]: %s\n", m, argv[3 + m]);
                return 1;
            }
            s_fb[row][m] = (uint8_t)v;
        }
        esp_err_t err = fb_flush_row((int)row);
        if (err != ESP_OK) {
            printf("rowm failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }

    if (strcmp(argv[1], "row4") == 0) {
        stop_animations();
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
        stop_animations();
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
        stop_animations();
        if (argc < 3) {
            printf("usage: matrix pattern rows|cols|diag|checker|off|ids\n");
            printf("       matrix pattern onehot <0..N-1>\n");
            return 1;
        }
        fb_clear();
        const int n = chain_len_active();

        if (strcmp(argv[2], "off") == 0) {
            // already cleared
        } else if (strcmp(argv[2], "rows") == 0) {
            for (int y = 0; y < 8; y++) {
                for (int m = 0; m < n; m++) {
                    s_fb[y][m] = (y & 1) ? 0xFF : 0x00;
                }
            }
        } else if (strcmp(argv[2], "cols") == 0) {
            for (int y = 0; y < 8; y++) {
                for (int m = 0; m < n; m++) {
                    s_fb[y][m] = 0xAA;
                }
            }
        } else if (strcmp(argv[2], "diag") == 0) {
            for (int i = 0; i < 8; i++) {
                s_fb[i][0] = (uint8_t)(1u << i);
            }
        } else if (strcmp(argv[2], "checker") == 0) {
            for (int y = 0; y < 8; y++) {
                for (int m = 0; m < n; m++) {
                    s_fb[y][m] = (y & 1) ? 0xAA : 0x55;
                }
            }
        } else if (strcmp(argv[2], "ids") == 0) {
            fb_set_ids_pattern();
        } else if (strcmp(argv[2], "onehot") == 0) {
            if (argc < 4) {
                printf("usage: matrix pattern onehot <0..N-1>\n");
                return 1;
            }
            char *end = NULL;
            long m = strtol(argv[3], &end, 0);
            if (!end || *end != '\0' || m < 0 || m >= n) {
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
