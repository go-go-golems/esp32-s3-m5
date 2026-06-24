#include "visual_repl.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "picocalc_lcd.h"

namespace {
constexpr const char *kTag = "visual_repl";
constexpr size_t kLineLen = VISUAL_REPL_COLS + 1;

struct Row {
    visual_repl_style_t style;
    char text[kLineLen];
};

static bool s_initialized = false;
static Row s_history[VISUAL_REPL_HISTORY_ROWS];
static uint32_t s_history_count = 0;
static char s_input[VISUAL_REPL_INPUT_MAX + 1];
static size_t s_cursor = 0;
static uint16_t s_row_pixels[PICOCALC_LCD_WIDTH * VISUAL_REPL_CELL_H];
static uint32_t s_render_count = 0;
static uint32_t s_last_render_ms = 0;

struct Palette {
    uint16_t fg;
    uint16_t bg;
};

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint16_t>(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

Palette palette_for(visual_repl_style_t style)
{
    switch (style) {
        case VISUAL_REPL_STYLE_PROMPT: return {rgb565(120, 220, 255), PICOCALC_LCD_RGB565_BLACK};
        case VISUAL_REPL_STYLE_INPUT:  return {PICOCALC_LCD_RGB565_WHITE, rgb565(10, 16, 24)};
        case VISUAL_REPL_STYLE_OUTPUT: return {rgb565(210, 210, 210), PICOCALC_LCD_RGB565_BLACK};
        case VISUAL_REPL_STYLE_ERROR:  return {rgb565(255, 95, 95), PICOCALC_LCD_RGB565_BLACK};
        case VISUAL_REPL_STYLE_STATUS: return {rgb565(255, 210, 90), rgb565(20, 20, 20)};
        case VISUAL_REPL_STYLE_SYSTEM:
        default:                       return {rgb565(140, 180, 150), PICOCALC_LCD_RGB565_BLACK};
    }
}

void copy_truncated(char *dst, size_t dst_len, const char *src)
{
    if (!dst || dst_len == 0) return;
    if (!src) src = "";
    size_t n = std::min(std::strlen(src), dst_len - 1);
    std::memcpy(dst, src, n);
    dst[n] = 0;
}

void clear_row(Row &row)
{
    row.style = VISUAL_REPL_STYLE_SYSTEM;
    row.text[0] = 0;
}

const uint8_t *glyph5x7(char c)
{
    static const uint8_t SPACE[7] = {0,0,0,0,0,0,0};
    static const uint8_t EXCL[7]  = {0x04,0x04,0x04,0x04,0x04,0x00,0x04};
    static const uint8_t QUOTE[7] = {0x0a,0x0a,0x00,0x00,0x00,0x00,0x00};
    static const uint8_t HASH[7]  = {0x0a,0x1f,0x0a,0x0a,0x1f,0x0a,0x00};
    static const uint8_t DOLL[7]  = {0x04,0x0f,0x14,0x0e,0x05,0x1e,0x04};
    static const uint8_t PCT[7]   = {0x18,0x19,0x02,0x04,0x08,0x13,0x03};
    static const uint8_t AMP[7]   = {0x0c,0x12,0x14,0x08,0x15,0x12,0x0d};
    static const uint8_t APOS[7]  = {0x04,0x04,0x08,0x00,0x00,0x00,0x00};
    static const uint8_t LP[7]    = {0x02,0x04,0x08,0x08,0x08,0x04,0x02};
    static const uint8_t RP[7]    = {0x08,0x04,0x02,0x02,0x02,0x04,0x08};
    static const uint8_t STAR[7]  = {0x00,0x15,0x0e,0x1f,0x0e,0x15,0x00};
    static const uint8_t PLUS[7]  = {0x00,0x04,0x04,0x1f,0x04,0x04,0x00};
    static const uint8_t COMMA[7] = {0x00,0x00,0x00,0x00,0x04,0x04,0x08};
    static const uint8_t MINUS[7] = {0x00,0x00,0x00,0x1f,0x00,0x00,0x00};
    static const uint8_t DOT[7]   = {0x00,0x00,0x00,0x00,0x00,0x0c,0x0c};
    static const uint8_t SLASH[7] = {0x01,0x02,0x04,0x08,0x10,0x00,0x00};
    static const uint8_t COLON[7] = {0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x00};
    static const uint8_t SEMI[7]  = {0x00,0x0c,0x0c,0x00,0x04,0x04,0x08};
    static const uint8_t LT[7]    = {0x02,0x04,0x08,0x10,0x08,0x04,0x02};
    static const uint8_t EQ[7]    = {0x00,0x00,0x1f,0x00,0x1f,0x00,0x00};
    static const uint8_t GT[7]    = {0x08,0x04,0x02,0x01,0x02,0x04,0x08};
    static const uint8_t QMARK[7] = {0x0e,0x11,0x01,0x02,0x04,0x00,0x04};
    static const uint8_t AT[7]    = {0x0e,0x11,0x17,0x15,0x17,0x10,0x0e};
    static const uint8_t LBR[7]   = {0x0e,0x08,0x08,0x08,0x08,0x08,0x0e};
    static const uint8_t BSL[7]   = {0x10,0x08,0x04,0x02,0x01,0x00,0x00};
    static const uint8_t RBR[7]   = {0x0e,0x02,0x02,0x02,0x02,0x02,0x0e};
    static const uint8_t CARET[7] = {0x04,0x0a,0x11,0x00,0x00,0x00,0x00};
    static const uint8_t UNDER[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x1f};
    static const uint8_t GRAVE[7] = {0x08,0x04,0x02,0x00,0x00,0x00,0x00};
    static const uint8_t LCUR[7]  = {0x02,0x04,0x04,0x18,0x04,0x04,0x02};
    static const uint8_t BAR[7]   = {0x04,0x04,0x04,0x00,0x04,0x04,0x04};
    static const uint8_t RCUR[7]  = {0x08,0x04,0x04,0x03,0x04,0x04,0x08};
    static const uint8_t TILDE[7] = {0x00,0x00,0x08,0x15,0x02,0x00,0x00};

    static const uint8_t DIGITS[10][7] = {
        {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e}, {0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
        {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f}, {0x1f,0x02,0x04,0x02,0x01,0x11,0x0e},
        {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02}, {0x1f,0x10,0x1e,0x01,0x01,0x11,0x0e},
        {0x06,0x08,0x10,0x1e,0x11,0x11,0x0e}, {0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
        {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e}, {0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c},
    };
    static const uint8_t LETTERS[26][7] = {
        {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11}, {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
        {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e}, {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
        {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f}, {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
        {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f}, {0x11,0x11,0x11,0x1f,0x11,0x11,0x11},
        {0x0e,0x04,0x04,0x04,0x04,0x04,0x0e}, {0x07,0x02,0x02,0x02,0x12,0x12,0x0c},
        {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, {0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
        {0x11,0x1b,0x15,0x15,0x11,0x11,0x11}, {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
        {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e}, {0x1e,0x11,0x11,0x1e,0x10,0x10,0x10},
        {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d}, {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11},
        {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e}, {0x1f,0x04,0x04,0x04,0x04,0x04,0x04},
        {0x11,0x11,0x11,0x11,0x11,0x11,0x0e}, {0x11,0x11,0x11,0x11,0x11,0x0a,0x04},
        {0x11,0x11,0x11,0x15,0x15,0x15,0x0a}, {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11},
        {0x11,0x11,0x0a,0x04,0x04,0x04,0x04}, {0x1f,0x01,0x02,0x04,0x08,0x10,0x1f},
    };

    unsigned char uc = static_cast<unsigned char>(c);
    if (uc >= 'a' && uc <= 'z') uc = static_cast<unsigned char>(std::toupper(uc));
    if (uc >= '0' && uc <= '9') return DIGITS[uc - '0'];
    if (uc >= 'A' && uc <= 'Z') return LETTERS[uc - 'A'];
    switch (uc) {
        case ' ': return SPACE; case '!': return EXCL; case '"': return QUOTE; case '#': return HASH;
        case '$': return DOLL; case '%': return PCT; case '&': return AMP; case '\'': return APOS;
        case '(': return LP; case ')': return RP; case '*': return STAR; case '+': return PLUS;
        case ',': return COMMA; case '-': return MINUS; case '.': return DOT; case '/': return SLASH;
        case ':': return COLON; case ';': return SEMI; case '<': return LT; case '=': return EQ;
        case '>': return GT; case '?': return QMARK; case '@': return AT; case '[': return LBR;
        case '\\': return BSL; case ']': return RBR; case '^': return CARET; case '_': return UNDER;
        case '`': return GRAVE; case '{': return LCUR; case '|': return BAR; case '}': return RCUR;
        case '~': return TILDE; default: return QMARK;
    }
}

void draw_cell(uint16_t *row_pixels, int col, char ch, Palette p, bool cursor)
{
    const int x0 = col * VISUAL_REPL_CELL_W;
    const uint8_t *glyph = glyph5x7(ch);
    for (int y = 0; y < VISUAL_REPL_CELL_H; ++y) {
        for (int x = 0; x < VISUAL_REPL_CELL_W; ++x) {
            row_pixels[y * PICOCALC_LCD_WIDTH + x0 + x] = cursor ? p.fg : p.bg;
        }
    }
    const int scale = 2;
    const int glyph_x = x0 + 1;
    const int glyph_y = 1;
    for (int gy = 0; gy < 7; ++gy) {
        for (int gx = 0; gx < 5; ++gx) {
            if ((glyph[gy] & (1 << (4 - gx))) == 0) continue;
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    const int px = glyph_x + gx * scale + sx;
                    const int py = glyph_y + gy * scale + sy;
                    if (px >= x0 && px < x0 + VISUAL_REPL_CELL_W && py >= 0 && py < VISUAL_REPL_CELL_H) {
                        row_pixels[py * PICOCALC_LCD_WIDTH + px] = cursor ? p.bg : p.fg;
                    }
                }
            }
        }
    }
}

esp_err_t render_text_row(int screen_row, visual_repl_style_t style, const char *text, size_t cursor_col, bool show_cursor)
{
    Palette p = palette_for(style);
    for (int col = 0; col < VISUAL_REPL_COLS; ++col) {
        char ch = (text && text[col]) ? text[col] : ' ';
        const bool cursor = show_cursor && static_cast<size_t>(col) == cursor_col;
        draw_cell(s_row_pixels, col, ch, p, cursor);
    }
    return picocalc_lcd_blit_row(screen_row * VISUAL_REPL_CELL_H, VISUAL_REPL_CELL_H,
                                 s_row_pixels, PICOCALC_LCD_WIDTH * VISUAL_REPL_CELL_H);
}

} // namespace

esp_err_t visual_repl_init(void)
{
    visual_repl_clear();
    s_initialized = true;
    ESP_LOGI(kTag, "visual REPL model initialized: %ux%u cells (%ux%u pixels)",
             VISUAL_REPL_COLS, VISUAL_REPL_ROWS, VISUAL_REPL_CELL_W, VISUAL_REPL_CELL_H);
    return ESP_OK;
}

void visual_repl_clear(void)
{
    for (auto &row : s_history) clear_row(row);
    s_history_count = 0;
    s_input[0] = 0;
    s_cursor = 0;
}

esp_err_t visual_repl_append_line(visual_repl_style_t style, const char *text)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    const uint32_t slot = s_history_count % VISUAL_REPL_HISTORY_ROWS;
    s_history[slot].style = style;
    copy_truncated(s_history[slot].text, sizeof(s_history[slot].text), text);
    ++s_history_count;
    return ESP_OK;
}

esp_err_t visual_repl_set_input(const char *text, size_t cursor)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    copy_truncated(s_input, sizeof(s_input), text);
    const size_t len = std::strlen(s_input);
    s_cursor = std::min(cursor, len);
    return ESP_OK;
}

esp_err_t visual_repl_render(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    const int64_t start = esp_timer_get_time();
    const uint32_t visible_history_rows = VISUAL_REPL_ROWS - 1;
    const uint32_t available = std::min<uint32_t>(s_history_count, visible_history_rows);
    const uint32_t first_sequence = s_history_count > visible_history_rows ? s_history_count - visible_history_rows : 0;

    for (uint32_t screen_row = 0; screen_row < visible_history_rows; ++screen_row) {
        if (screen_row < visible_history_rows - available) {
            ESP_RETURN_ON_ERROR(render_text_row(screen_row, VISUAL_REPL_STYLE_SYSTEM, "", 0, false), kTag, "blank row");
            continue;
        }
        const uint32_t seq = first_sequence + (screen_row - (visible_history_rows - available));
        const Row &row = s_history[seq % VISUAL_REPL_HISTORY_ROWS];
        ESP_RETURN_ON_ERROR(render_text_row(screen_row, row.style, row.text, 0, false), kTag, "history row");
    }

    char prompt_line[VISUAL_REPL_COLS + 1];
    prompt_line[0] = '>';
    prompt_line[1] = ' ';
    const size_t input_room = VISUAL_REPL_COLS - 2;
    const size_t input_len = std::min(std::strlen(s_input), input_room);
    std::memcpy(prompt_line + 2, s_input, input_len);
    prompt_line[2 + input_len] = 0;
    const size_t cursor_col = std::min<size_t>(s_cursor + 2, VISUAL_REPL_COLS - 1);
    ESP_RETURN_ON_ERROR(render_text_row(VISUAL_REPL_ROWS - 1, VISUAL_REPL_STYLE_INPUT,
                                        prompt_line, cursor_col, true), kTag, "input row");
    s_last_render_ms = static_cast<uint32_t>((esp_timer_get_time() - start) / 1000);
    ++s_render_count;
    return ESP_OK;
}

void visual_repl_get_status(visual_repl_status_t *out)
{
    if (!out) return;
    out->initialized = s_initialized;
    out->cols = VISUAL_REPL_COLS;
    out->rows = VISUAL_REPL_ROWS;
    out->cell_w = VISUAL_REPL_CELL_W;
    out->cell_h = VISUAL_REPL_CELL_H;
    out->history_count = s_history_count;
    out->render_count = s_render_count;
    out->last_render_ms = s_last_render_ms;
}

esp_err_t visual_repl_demo_screen(void)
{
    ESP_RETURN_ON_ERROR(visual_repl_init(), kTag, "init");
    ESP_RETURN_ON_ERROR(visual_repl_append_line(VISUAL_REPL_STYLE_SYSTEM, "ESP32-P4 VISUAL QUICKJS REPL"), kTag, "line");
    ESP_RETURN_ON_ERROR(visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "LCD 320X320  40X20 CELLS  RGB565"), kTag, "line");
    ESP_RETURN_ON_ERROR(visual_repl_append_line(VISUAL_REPL_STYLE_PROMPT, "> PRINT(1+2)"), kTag, "line");
    ESP_RETURN_ON_ERROR(visual_repl_append_line(VISUAL_REPL_STYLE_OUTPUT, "3"), kTag, "line");
    ESP_RETURN_ON_ERROR(visual_repl_append_line(VISUAL_REPL_STYLE_PROMPT, "> THROW NEW ERROR('BOOM')"), kTag, "line");
    ESP_RETURN_ON_ERROR(visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, "ERROR: BOOM"), kTag, "line");
    ESP_RETURN_ON_ERROR(visual_repl_append_line(VISUAL_REPL_STYLE_SYSTEM, "KEYBOARD + QUICKJS BRIDGE COMING NEXT"), kTag, "line");
    ESP_RETURN_ON_ERROR(visual_repl_set_input("PRINT('HELLO')", 14), kTag, "input");
    return visual_repl_render();
}
