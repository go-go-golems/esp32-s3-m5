#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VISUAL_REPL_COLS 40
#define VISUAL_REPL_ROWS 20
#define VISUAL_REPL_CELL_W 8
#define VISUAL_REPL_CELL_H 16
#define VISUAL_REPL_HISTORY_ROWS 80
#define VISUAL_REPL_INPUT_MAX 160

typedef enum {
    VISUAL_REPL_STYLE_SYSTEM = 0,
    VISUAL_REPL_STYLE_PROMPT,
    VISUAL_REPL_STYLE_INPUT,
    VISUAL_REPL_STYLE_OUTPUT,
    VISUAL_REPL_STYLE_ERROR,
    VISUAL_REPL_STYLE_STATUS,
} visual_repl_style_t;

typedef struct {
    bool initialized;
    uint16_t cols;
    uint16_t rows;
    uint16_t cell_w;
    uint16_t cell_h;
    uint32_t history_count;
    uint32_t render_count;
    uint32_t last_render_ms;
} visual_repl_status_t;

esp_err_t visual_repl_init(void);
void visual_repl_clear(void);
esp_err_t visual_repl_append_line(visual_repl_style_t style, const char *text);
esp_err_t visual_repl_set_input(const char *text, size_t cursor);
esp_err_t visual_repl_render(void);
esp_err_t visual_repl_render_input(void);
esp_err_t visual_repl_dump_text(char *dst, size_t dst_len);
void visual_repl_get_status(visual_repl_status_t *out);
esp_err_t visual_repl_demo_screen(void);

#ifdef __cplusplus
}
#endif
