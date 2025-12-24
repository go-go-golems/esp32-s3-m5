#pragma once

#include <stdint.h>

namespace m5gfx {
class M5Canvas;
} // namespace m5gfx

typedef struct echo_gif_render_ctx_tag {
    m5gfx::M5Canvas *canvas;
    int canvas_w;
    int canvas_h;
    int gif_canvas_w;
    int gif_canvas_h;
    int scale_x;
    int scale_y;
    int off_x;
    int off_y;
} EchoGifRenderCtx;

// Open GIF by FATFS path (e.g. "/storage/gifs/foo.gif").
bool echo_gif_player_open_path(const char *path, EchoGifRenderCtx *ctx);

// Open GIF by registry index (see echo_gif_registry).
bool echo_gif_player_open_index(int idx, EchoGifRenderCtx *ctx);

// Render one frame (AnimatedGIF::playFrame semantics).
// Returns the same value as AnimatedGIF::playFrame(). The computed frame delay (ms) is written to out_frame_delay_ms.
int echo_gif_player_play_frame(int *out_frame_delay_ms, EchoGifRenderCtx *ctx);

int echo_gif_player_last_error(void);
void echo_gif_player_reset(void);

int echo_gif_player_canvas_width(void);
int echo_gif_player_canvas_height(void);
int echo_gif_player_frame_width(void);
int echo_gif_player_frame_height(void);
int echo_gif_player_frame_x_off(void);
int echo_gif_player_frame_y_off(void);

const char *echo_gif_player_current_path(void);
int32_t echo_gif_player_current_file_size(void);


