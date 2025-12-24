#pragma once

#include <stdint.h>

#include "M5GFX.h"

typedef struct gif_render_ctx_tag {
    M5Canvas *canvas;
    int canvas_w;
    int canvas_h;
    int gif_canvas_w;
    int gif_canvas_h;
    int scale_x;
    int scale_y;
    int off_x;
    int off_y;
} GifRenderCtx;

// Open GIF by path (FATFS path under /storage/gifs/...).
bool gif_player_open_path(const char *path, GifRenderCtx *ctx);

// Open GIF by registry index (see gif_registry).
bool gif_player_open_index(int idx, GifRenderCtx *ctx);

// Render one frame (AnimatedGIF::playFrame semantics).
// Returns the same value as AnimatedGIF::playFrame(). The computed frame delay (ms) is written to out_frame_delay_ms.
int gif_player_play_frame(int *out_frame_delay_ms, GifRenderCtx *ctx);

int gif_player_last_error(void);
void gif_player_reset(void);

int gif_player_canvas_width(void);
int gif_player_canvas_height(void);
int gif_player_frame_width(void);
int gif_player_frame_height(void);
int gif_player_frame_x_off(void);
int gif_player_frame_y_off(void);

const char *gif_player_current_path(void);
int32_t gif_player_current_file_size(void);


