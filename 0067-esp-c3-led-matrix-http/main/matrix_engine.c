#include "matrix_engine.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "max7219.h"

static const char *TAG = "0067_matrix";

static max7219_t s_dev = {0};
static SemaphoreHandle_t s_mu;
static bool s_ready;
static bool s_reverse_modules;
static bool s_flip_vertical = true;
static bool s_rotate_180;
static int s_chain_len_cfg = 12;
static int s_default_fps = 15;
static uint8_t s_fb[8][MAX7219_MAX_CHAIN_LEN] = {0};

static TaskHandle_t s_anim_task;
static matrix_mode_t s_mode = MATRIX_MODE_IDLE;
static matrix_scroll_loop_t s_scroll_loop = MATRIX_SCROLL_LOOP_GAP;
static uint32_t s_fps = 15;
static uint32_t s_pause_ms = 250;
static uint32_t s_repeat_count;
static uint32_t s_cycles_done;
static bool s_wave;
static bool s_restart;
static uint8_t *s_text_cols;
static int s_text_w;
static char s_text[65];
static int s_intensity = 4;
static bool s_test_mode;
static bool s_first_animation_started;

static const int8_t s_wave16[16] = {0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1, -2, -1};

static const uint8_t s_font5x7[95][5] = {
    [' ' - 32] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['!' - 32] = {0x00, 0x00, 0x5F, 0x00, 0x00},
    ['"' - 32] = {0x00, 0x07, 0x00, 0x07, 0x00},
    ['#' - 32] = {0x14, 0x7F, 0x14, 0x7F, 0x14},
    ['$' - 32] = {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    ['%' - 32] = {0x23, 0x13, 0x08, 0x64, 0x62},
    ['&' - 32] = {0x36, 0x49, 0x55, 0x22, 0x50},
    ['\'' - 32] = {0x00, 0x05, 0x03, 0x00, 0x00},
    ['(' - 32] = {0x00, 0x1C, 0x22, 0x41, 0x00},
    [')' - 32] = {0x00, 0x41, 0x22, 0x1C, 0x00},
    ['*' - 32] = {0x14, 0x08, 0x3E, 0x08, 0x14},
    ['+' - 32] = {0x08, 0x08, 0x3E, 0x08, 0x08},
    [',' - 32] = {0x00, 0x50, 0x30, 0x00, 0x00},
    ['-' - 32] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['.' - 32] = {0x00, 0x60, 0x60, 0x00, 0x00},
    ['/' - 32] = {0x20, 0x10, 0x08, 0x04, 0x02},
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
    [':' - 32] = {0x00, 0x36, 0x36, 0x00, 0x00},
    [';' - 32] = {0x00, 0x56, 0x36, 0x00, 0x00},
    ['<' - 32] = {0x08, 0x14, 0x22, 0x41, 0x00},
    ['=' - 32] = {0x14, 0x14, 0x14, 0x14, 0x14},
    ['>' - 32] = {0x00, 0x41, 0x22, 0x14, 0x08},
    ['?' - 32] = {0x02, 0x01, 0x51, 0x09, 0x06},
    ['@' - 32] = {0x32, 0x49, 0x79, 0x41, 0x3E},
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
    ['[' - 32] = {0x00, 0x7F, 0x41, 0x41, 0x00},
    ['\\' - 32] = {0x02, 0x04, 0x08, 0x10, 0x20},
    [']' - 32] = {0x00, 0x41, 0x41, 0x7F, 0x00},
    ['^' - 32] = {0x04, 0x02, 0x01, 0x02, 0x04},
    ['_' - 32] = {0x40, 0x40, 0x40, 0x40, 0x40},
    ['`' - 32] = {0x00, 0x01, 0x02, 0x04, 0x00},
    ['{' - 32] = {0x00, 0x08, 0x36, 0x41, 0x00},
    ['|' - 32] = {0x00, 0x00, 0x7F, 0x00, 0x00},
    ['}' - 32] = {0x00, 0x41, 0x36, 0x08, 0x00},
    ['~' - 32] = {0x08, 0x04, 0x08, 0x10, 0x08},
};

static char up(char c)
{
    if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
    return c;
}

static const uint8_t *glyph(char c)
{
    c = up(c);
    if (c < 32 || c > 126) c = ' ';
    return s_font5x7[c - 32];
}

static int chain_len_active(void)
{
    if (s_ready && s_dev.chain_len > 0) return s_dev.chain_len;
    return s_chain_len_cfg;
}

static int width_active(void)
{
    return chain_len_active() * 8;
}

static void lock(void)
{
    if (s_mu) xSemaphoreTake(s_mu, portMAX_DELAY);
}

static void unlock(void)
{
    if (s_mu) xSemaphoreGive(s_mu);
}

static int x_to_module(int x)
{
    return x / 8;
}

static uint8_t x_to_bit(int x)
{
    return (uint8_t)(1u << (x % 8));
}

static uint8_t reverse_bits8(uint8_t v)
{
    v = (uint8_t)(((v & 0xF0u) >> 4) | ((v & 0x0Fu) << 4));
    v = (uint8_t)(((v & 0xCCu) >> 2) | ((v & 0x33u) << 2));
    v = (uint8_t)(((v & 0xAAu) >> 1) | ((v & 0x55u) << 1));
    return v;
}

static void fb_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

static esp_err_t fb_flush_row(int y)
{
    const bool eff_flipv = (s_flip_vertical != s_rotate_180);
    const bool eff_fliph = (s_reverse_modules != s_rotate_180);
    const int row = eff_flipv ? (7 - y) : y;
    uint8_t phy[MAX7219_MAX_CHAIN_LEN] = {0};
    const int n = chain_len_active();
    for (int m = 0; m < n; m++) {
        const int src = eff_fliph ? ((n - 1) - m) : m;
        uint8_t bits = s_fb[y][src];
        if (eff_fliph) bits = reverse_bits8(bits);
        phy[m] = bits;
    }
    return max7219_set_row_chain(&s_dev, (uint8_t)row, phy);
}

static esp_err_t fb_flush_all(void)
{
    for (int y = 0; y < 8; y++) {
        esp_err_t err = fb_flush_row(y);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

static void render_char(char c, uint8_t out[8])
{
    memset(out, 0, 8);
    const uint8_t *g = glyph(c);
    const int x0 = 1;
    for (int col = 0; col < 5; col++) {
        uint8_t bits = g[col];
        for (int y = 0; y < 7; y++) {
            if (bits & (uint8_t)(1u << y)) out[y] |= (uint8_t)(1u << (x0 + col));
        }
    }
}

static void fb_from_cols(const uint8_t *cols, int width)
{
    fb_clear();
    if (!cols || width <= 0) return;
    if (width > 8 * MAX7219_MAX_CHAIN_LEN) width = 8 * MAX7219_MAX_CHAIN_LEN;
    for (int x = 0; x < width; x++) {
        int m = x_to_module(x);
        uint8_t bit = x_to_bit(x);
        uint8_t col = cols[x];
        for (int y = 0; y < 8; y++) {
            if (col & (uint8_t)(1u << y)) s_fb[y][m] |= bit;
        }
    }
}

static void set_text_cols_locked(const char *text)
{
    if (s_text_cols) {
        free(s_text_cols);
        s_text_cols = NULL;
    }
    s_text_w = 0;

    if (!text) return;
    size_t len = strlen(text);
    if (len > 64) len = 64;
    if (len == 0) return;

    int w = (int)(len * 6);
    uint8_t *cols = (uint8_t *)calloc((size_t)w, 1);
    if (!cols) return;

    int x = 0;
    memset(s_text, 0, sizeof(s_text));
    for (size_t i = 0; i < len; i++) {
        char c = up(text[i]);
        s_text[i] = c;
        const uint8_t *g = glyph(c);
        for (int col = 0; col < 5; col++) cols[x++] = g[col];
        cols[x++] = 0;
    }

    s_text_cols = cols;
    s_text_w = w;
    s_restart = true;
}

static int8_t drop_sample(int frame, int char_idx)
{
    // simple bounce profile, deterministic and cheap
    const float t = (float)(frame - char_idx);
    if (t < 0) return -7;
    float y = 0.0f;
    float v = -3.8f;
    float g = 0.28f;
    for (int i = 0; i < (int)t; i++) {
        v += g;
        y += v;
        if (y >= 0.0f) {
            y = 0.0f;
            v = -v * 0.72f;
            if (fabsf(v) < 0.2f) return 0;
        }
    }
    int off = (int)lroundf(y);
    if (off < -7) off = -7;
    if (off > 7) off = 7;
    return (int8_t)off;
}

static void mark_first_animation_started_locked(void)
{
    if (!s_first_animation_started) {
        s_first_animation_started = true;
        ESP_LOGI(TAG, "first animation started, releasing boot preview");
    }
}

static esp_err_t start_scroll_locked(const char *text,
                                     uint32_t fps,
                                     uint32_t pause_ms,
                                     uint32_t repeat_count,
                                     bool wave,
                                     matrix_scroll_loop_t loop_mode,
                                     bool boot_ip_preview)
{
    if (boot_ip_preview && s_first_animation_started) return ESP_OK;
    if (loop_mode < MATRIX_SCROLL_LOOP_GAP || loop_mode > MATRIX_SCROLL_LOOP_RIGHT_EXIT) return ESP_ERR_INVALID_ARG;

    set_text_cols_locked(text);
    if (!s_text_cols || s_text_w <= 0) return ESP_ERR_INVALID_ARG;

    if (!boot_ip_preview) {
        mark_first_animation_started_locked();
    }

    s_mode = MATRIX_MODE_SCROLL;
    s_scroll_loop = loop_mode;
    s_wave = wave;
    s_fps = fps ? fps : (uint32_t)s_default_fps;
    s_pause_ms = pause_ms;
    s_repeat_count = repeat_count;
    s_cycles_done = 0;
    if (s_anim_task) xTaskNotifyGive(s_anim_task);
    return ESP_OK;
}

static bool mark_cycle_complete_and_check_stop(matrix_mode_t expected_mode)
{
    bool should_stop = false;
    lock();
    if (s_mode == expected_mode && s_repeat_count > 0) {
        s_cycles_done++;
        if (s_cycles_done >= s_repeat_count) {
            s_mode = MATRIX_MODE_IDLE;
            should_stop = true;
        }
    }
    unlock();
    return should_stop;
}

static void anim_task(void *arg)
{
    (void)arg;
    uint32_t frame = 0;

    for (;;) {
        lock();
        matrix_mode_t mode = s_mode;
        uint32_t fps = s_fps;
        uint32_t pause_ms = s_pause_ms;
        matrix_scroll_loop_t loop_mode = s_scroll_loop;
        bool wave = s_wave;
        bool restart = s_restart;
        s_restart = false;
        const uint8_t *text_cols = s_text_cols;
        int text_w = s_text_w;
        char text[65];
        memcpy(text, s_text, sizeof(text));
        unlock();

        if (mode == MATRIX_MODE_IDLE || mode == MATRIX_MODE_TEXT || mode == MATRIX_MODE_SCRIPT || !s_ready || text_w <= 0 || !text_cols) {
            frame = 0;
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
            continue;
        }

        if (fps < 1) fps = 1;
        if (fps > 60) fps = 60;
        const uint32_t frame_ms = 1000u / fps;
        const int width = width_active();
        static int pos = 0;

        uint8_t cols[8 * MAX7219_MAX_CHAIN_LEN] = {0};
        if (mode == MATRIX_MODE_SCROLL) {
            if (restart) {
                if (loop_mode == MATRIX_SCROLL_LOOP_WRAP) pos = 0;
                else if (loop_mode == MATRIX_SCROLL_LOOP_RIGHT_EXIT) pos = -text_w;
                else pos = width;
            }

            if (loop_mode == MATRIX_SCROLL_LOOP_WRAP) {
                for (int x = 0; x < width; x++) {
                    int t = x - pos;
                    if (text_w > 0) {
                        t %= text_w;
                        if (t < 0) t += text_w;
                    }
                    uint8_t bits = text_cols[t];
                    if (wave) {
                        int char_idx = t / 6;
                        int8_t yoff = s_wave16[(int)((frame + (uint32_t)(char_idx * 2)) & 0x0F)];
                        if (yoff > 0) bits <<= yoff;
                        if (yoff < 0) bits >>= -yoff;
                    }
                    cols[x] = bits;
                }
                pos--;
                if (pos <= -text_w) {
                    pos += text_w;
                    if (mark_cycle_complete_and_check_stop(MATRIX_MODE_SCROLL)) {
                        frame = 0;
                        continue;
                    }
                    if (pause_ms) vTaskDelay(pdMS_TO_TICKS(pause_ms));
                }
            } else if (loop_mode == MATRIX_SCROLL_LOOP_RIGHT_EXIT) {
                for (int x = 0; x < width; x++) {
                    int t = x - pos;
                    if (t >= 0 && t < text_w) {
                        uint8_t bits = text_cols[t];
                        if (wave) {
                            int char_idx = t / 6;
                            int8_t yoff = s_wave16[(int)((frame + (uint32_t)(char_idx * 2)) & 0x0F)];
                            if (yoff > 0) bits <<= yoff;
                            if (yoff < 0) bits >>= -yoff;
                        }
                        cols[x] = bits;
                    }
                }
                pos++;
                if (pos > width) {
                    pos = -text_w;
                    if (mark_cycle_complete_and_check_stop(MATRIX_MODE_SCROLL)) {
                        frame = 0;
                        continue;
                    }
                    if (pause_ms) vTaskDelay(pdMS_TO_TICKS(pause_ms));
                }
            } else {
                for (int x = 0; x < width; x++) {
                    int t = x - pos;
                    if (t >= 0 && t < text_w) {
                        uint8_t bits = text_cols[t];
                        if (wave) {
                            int char_idx = t / 6;
                            int8_t yoff = s_wave16[(int)((frame + (uint32_t)(char_idx * 2)) & 0x0F)];
                            if (yoff > 0) bits <<= yoff;
                            if (yoff < 0) bits >>= -yoff;
                        }
                        cols[x] = bits;
                    }
                }
                pos--;
                if (pos < -text_w) {
                    pos = width;
                    if (mark_cycle_complete_and_check_stop(MATRIX_MODE_SCROLL)) {
                        frame = 0;
                        continue;
                    }
                    if (pause_ms) vTaskDelay(pdMS_TO_TICKS(pause_ms));
                }
            }
        } else if (mode == MATRIX_MODE_DROP) {
            int len = (int)strlen(text);
            int cell = 6;
            int start_x = (width - len * cell) / 2;
            for (int i = 0; i < len; i++) {
                const uint8_t *g = glyph(text[i]);
                int8_t yoff = drop_sample((int)frame, i);
                for (int col = 0; col < 5; col++) {
                    int x = start_x + i * cell + col;
                    if (x < 0 || x >= width) continue;
                    uint8_t bits = g[col];
                    if (yoff > 0) bits <<= yoff;
                    if (yoff < 0) bits >>= -yoff;
                    cols[x] = bits;
                }
            }
            frame++;
            if (frame > 200) {
                frame = 0;
                if (mark_cycle_complete_and_check_stop(MATRIX_MODE_DROP)) continue;
                if (pause_ms) vTaskDelay(pdMS_TO_TICKS(pause_ms));
            }
        }

        lock();
        fb_from_cols(cols, width);
        (void)fb_flush_all();
        unlock();

        vTaskDelay(pdMS_TO_TICKS(frame_ms));
        if (mode == MATRIX_MODE_SCROLL) frame++;
    }
}

esp_err_t matrix_engine_init(const matrix_engine_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;
    if (!s_mu) s_mu = xSemaphoreCreateMutex();
    if (!s_mu) return ESP_ERR_NO_MEM;

    lock();
    s_chain_len_cfg = cfg->chain_len;
    s_default_fps = cfg->default_fps > 0 ? cfg->default_fps : 15;
    s_fps = s_default_fps;
    s_reverse_modules = cfg->default_fliph;
    s_flip_vertical = cfg->default_flipv;
    s_rotate_180 = cfg->default_rotate_180;
    s_scroll_loop = MATRIX_SCROLL_LOOP_GAP;
    s_first_animation_started = false;
    esp_err_t err = max7219_open(&s_dev, SPI2_HOST, cfg->pin_sck, cfg->pin_mosi, cfg->pin_cs, cfg->chain_len, cfg->spi_hz);
    if (err == ESP_OK) err = max7219_init(&s_dev);
    if (err == ESP_OK) {
        s_ready = true;
        fb_clear();
        (void)fb_flush_all();
        if (!s_anim_task) xTaskCreate(anim_task, "matrix_anim", 4096, NULL, 2, &s_anim_task);
        ESP_LOGI(TAG,
                 "matrix ready: chain=%d spi_hz=%d pins(mosi=%d sck=%d cs=%d) orient(fliph=%s flipv=%s rot180=%s)",
                 cfg->chain_len,
                 s_dev.clock_hz,
                 cfg->pin_mosi,
                 cfg->pin_sck,
                 cfg->pin_cs,
                 s_reverse_modules ? "on" : "off",
                 s_flip_vertical ? "on" : "off",
                 s_rotate_180 ? "on" : "off");
    }
    unlock();
    return err;
}

esp_err_t matrix_engine_play_boot_animation(void)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    const int w = width_active();

    for (int x = 0; x < w; x += 3) {
        lock();
        if (!s_ready) {
            unlock();
            return ESP_ERR_INVALID_STATE;
        }
        fb_clear();
        for (int y = 0; y < 8; y++) {
            const int x0 = x + ((y & 1) ? 1 : 0);
            if (x0 >= 0 && x0 < w) s_fb[y][x_to_module(x0)] |= x_to_bit(x0);
            const int x1 = x0 - 1;
            if (x1 >= 0 && x1 < w) s_fb[y][x_to_module(x1)] |= x_to_bit(x1);
        }
        (void)fb_flush_all();
        unlock();
        vTaskDelay(pdMS_TO_TICKS(14));
    }

    for (int i = 0; i < 2; i++) {
        lock();
        for (int y = 0; y < 8; y++) {
            for (int m = 0; m < chain_len_active(); m++) s_fb[y][m] = 0xFF;
        }
        (void)fb_flush_all();
        unlock();
        vTaskDelay(pdMS_TO_TICKS(55));

        lock();
        fb_clear();
        (void)fb_flush_all();
        unlock();
        vTaskDelay(pdMS_TO_TICKS(55));
    }

    lock();
    s_mode = MATRIX_MODE_IDLE;
    s_repeat_count = 0;
    s_cycles_done = 0;
    fb_clear();
    esp_err_t err = fb_flush_all();
    unlock();
    return err;
}

esp_err_t matrix_engine_show_boot_ip(const char *ip_text, uint32_t fps, uint32_t pause_ms, matrix_scroll_loop_t loop_mode)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = start_scroll_locked(ip_text, fps, pause_ms, 0, false, loop_mode, true);
    unlock();
    return err;
}

esp_err_t matrix_engine_set_text(const char *text)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    s_mode = MATRIX_MODE_TEXT;
    memset(s_text, 0, sizeof(s_text));
    size_t len = text ? strlen(text) : 0;
    if (len > (size_t)chain_len_active()) len = (size_t)chain_len_active();
    for (int m = 0; m < chain_len_active(); m++) {
        char c = (m < (int)len) ? up(text[m]) : ' ';
        uint8_t rows[8];
        render_char(c, rows);
        for (int y = 0; y < 8; y++) s_fb[y][m] = rows[y];
        if (m < (int)sizeof(s_text) - 1 && m < (int)len) s_text[m] = c;
    }
    esp_err_t err = fb_flush_all();
    unlock();
    return err;
}

esp_err_t matrix_engine_start_scroll(const char *text,
                                     uint32_t fps,
                                     uint32_t pause_ms,
                                     uint32_t repeat_count,
                                     bool wave,
                                     matrix_scroll_loop_t loop_mode)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = start_scroll_locked(text, fps, pause_ms, repeat_count, wave, loop_mode, false);
    unlock();
    return err;
}

esp_err_t matrix_engine_start_drop(const char *text, uint32_t fps, uint32_t pause_ms, uint32_t repeat_count)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    set_text_cols_locked(text);
    if (!s_text_cols || s_text_w <= 0) {
        unlock();
        return ESP_ERR_INVALID_ARG;
    }
    mark_first_animation_started_locked();
    s_mode = MATRIX_MODE_DROP;
    s_wave = false;
    s_fps = fps ? fps : (uint32_t)s_default_fps;
    s_pause_ms = pause_ms;
    s_repeat_count = repeat_count;
    s_cycles_done = 0;
    if (s_anim_task) xTaskNotifyGive(s_anim_task);
    unlock();
    return ESP_OK;
}

esp_err_t matrix_engine_stop(void)
{
    lock();
    s_mode = MATRIX_MODE_IDLE;
    s_repeat_count = 0;
    s_cycles_done = 0;
    unlock();
    return ESP_OK;
}

esp_err_t matrix_engine_set_intensity(int value)
{
    if (value < 0 || value > 15) return ESP_ERR_INVALID_ARG;
    lock();
    esp_err_t err = max7219_set_intensity(&s_dev, (uint8_t)value);
    if (err == ESP_OK) s_intensity = value;
    unlock();
    return err;
}

esp_err_t matrix_engine_set_test(bool on)
{
    lock();
    esp_err_t err = max7219_set_test(&s_dev, on);
    if (err == ESP_OK) s_test_mode = on;
    unlock();
    return err;
}

esp_err_t matrix_engine_set_spi_hz(int hz)
{
    lock();
    esp_err_t err = max7219_set_spi_clock_hz(&s_dev, hz);
    unlock();
    return err;
}

esp_err_t matrix_engine_set_chain_len(int n)
{
    if (n < 1 || n > MAX7219_MAX_CHAIN_LEN) return ESP_ERR_INVALID_ARG;
    lock();
    s_chain_len_cfg = n;
    s_dev.chain_len = n;
    fb_clear();
    esp_err_t err = fb_flush_all();
    unlock();
    return err;
}

esp_err_t matrix_engine_set_orientation(bool reverse_modules, bool flip_vertical)
{
    lock();
    s_reverse_modules = reverse_modules;
    s_flip_vertical = flip_vertical;
    esp_err_t err = fb_flush_all();
    unlock();
    return err;
}

esp_err_t matrix_engine_set_rotate_180(bool on)
{
    lock();
    s_rotate_180 = on;
    esp_err_t err = fb_flush_all();
    unlock();
    return err;
}

esp_err_t matrix_engine_get_status(matrix_status_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    lock();
    out->ready = s_ready;
    out->mode = s_mode;
    out->scroll_loop = s_scroll_loop;
    out->chain_len = chain_len_active();
    out->width = width_active();
    out->spi_hz = s_dev.clock_hz;
    out->intensity = s_intensity;
    out->test_mode = s_test_mode;
    out->reverse_modules = s_reverse_modules;
    out->flip_vertical = s_flip_vertical;
    out->rotate_180 = s_rotate_180;
    out->fps = s_fps;
    out->pause_ms = s_pause_ms;
    out->repeat_count = s_repeat_count;
    memset(out->text, 0, sizeof(out->text));
    strlcpy(out->text, s_text, sizeof(out->text));
    unlock();
    return ESP_OK;
}

int matrix_engine_width(void)
{
    lock();
    const int w = width_active();
    unlock();
    return w;
}

int matrix_engine_height(void)
{
    return 8;
}

esp_err_t matrix_engine_frame_clear(void)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    mark_first_animation_started_locked();
    s_mode = MATRIX_MODE_SCRIPT;
    fb_clear();
    unlock();
    return ESP_OK;
}

esp_err_t matrix_engine_frame_fill(bool on)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    mark_first_animation_started_locked();
    s_mode = MATRIX_MODE_SCRIPT;
    for (int y = 0; y < 8; y++) {
        for (int m = 0; m < chain_len_active(); m++) s_fb[y][m] = on ? 0xFF : 0x00;
    }
    unlock();
    return ESP_OK;
}

esp_err_t matrix_engine_frame_set_pixel(int x, int y, bool on)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    const int w = width_active();
    if (x < 0 || y < 0 || y >= 8 || x >= w) {
        unlock();
        return ESP_ERR_INVALID_ARG;
    }

    mark_first_animation_started_locked();
    s_mode = MATRIX_MODE_SCRIPT;
    const int m = x_to_module(x);
    const uint8_t bit = x_to_bit(x);
    if (on) s_fb[y][m] |= bit;
    else s_fb[y][m] &= (uint8_t)~bit;
    unlock();
    return ESP_OK;
}

esp_err_t matrix_engine_frame_get_pixel(int x, int y, bool *out_on)
{
    if (!out_on) return ESP_ERR_INVALID_ARG;
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    const int w = width_active();
    if (x < 0 || y < 0 || y >= 8 || x >= w) {
        unlock();
        return ESP_ERR_INVALID_ARG;
    }

    const int m = x_to_module(x);
    const uint8_t bit = x_to_bit(x);
    *out_on = (s_fb[y][m] & bit) != 0;
    unlock();
    return ESP_OK;
}

esp_err_t matrix_engine_frame_present(void)
{
    lock();
    if (!s_ready) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    mark_first_animation_started_locked();
    s_mode = MATRIX_MODE_SCRIPT;
    esp_err_t err = fb_flush_all();
    unlock();
    return err;
}
