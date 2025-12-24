/*
 * Tutorial 0013: GIF playback (AnimatedGIF + FATFS assets).
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_log.h"

// IMPORTANT: include M5GFX (via gif_player.h) before AnimatedGIF.h to avoid
// a macro collision between AnimatedGIF's memcpy_P macro and LovyanGFX pgmspace helpers.
#include "gif_player.h"
#include "AnimatedGIF.h"

#include "gif_registry.h"

static const char *TAG = "atoms3r_gif_console";

static AnimatedGIF s_gif;

static char s_current_gif[CONFIG_TUTORIAL_0013_GIF_MAX_PATH_LEN] = {0};
static int32_t s_current_gif_file_size = 0;

static void GIFDraw(GIFDRAW *pDraw) {
    auto *ctx = (GifRenderCtx *)pDraw->pUser;
    if (!ctx || !ctx->canvas) {
        return;
    }

    auto *dst = (uint16_t *)ctx->canvas->getBuffer();
    const uint8_t *s = pDraw->pPixels;
    const uint16_t *pal = pDraw->pPalette;

    const int src_y = pDraw->iY + pDraw->y;
    const int src_x0 = pDraw->iX;
    const int src_w = pDraw->iWidth;

    const int sx = (ctx->scale_x > 0) ? ctx->scale_x : 1;
    const int sy = (ctx->scale_y > 0) ? ctx->scale_y : 1;

    const int dst_y0 = ctx->off_y + src_y * sy;
    for (int dy = 0; dy < sy; dy++) {
        const int y = dst_y0 + dy;
        if ((unsigned)y >= (unsigned)ctx->canvas_h) {
            continue;
        }
        uint16_t *row = &dst[y * ctx->canvas_w];

        if (pDraw->ucHasTransparency) {
            const uint8_t t = pDraw->ucTransparent;
            for (int i = 0; i < src_w; i++) {
                const uint8_t idx = s[i];
                if (idx == t) {
                    continue;
                }
                uint16_t c = pal[idx];
#if CONFIG_TUTORIAL_0013_GIF_SWAP_BYTES
                c = __builtin_bswap16(c);
#endif
                const int dst_x0 = ctx->off_x + (src_x0 + i) * sx;
                for (int dx = 0; dx < sx; dx++) {
                    const int x = dst_x0 + dx;
                    if ((unsigned)x < (unsigned)ctx->canvas_w) {
                        row[x] = c;
                    }
                }
            }
        } else {
            for (int i = 0; i < src_w; i++) {
                uint16_t c = pal[s[i]];
#if CONFIG_TUTORIAL_0013_GIF_SWAP_BYTES
                c = __builtin_bswap16(c);
#endif
                const int dst_x0 = ctx->off_x + (src_x0 + i) * sx;
                for (int dx = 0; dx < sx; dx++) {
                    const int x = dst_x0 + dx;
                    if ((unsigned)x < (unsigned)ctx->canvas_w) {
                        row[x] = c;
                    }
                }
            }
        }
    }
}

// AnimatedGIF file callbacks (stream from FATFS; avoid loading whole file into RAM)
static void *gif_open_cb(const char *szFilename, int32_t *pFileSize) {
    if (pFileSize) *pFileSize = 0;
    FILE *f = fopen(szFilename, "rb");
    if (!f) {
        ESP_LOGE(TAG, "gif_open_cb: fopen failed: %s errno=%d", szFilename, errno);
        return nullptr;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return nullptr;
    }
    const long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return nullptr;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return nullptr;
    }
    if (pFileSize) *pFileSize = (int32_t)size;
    s_current_gif_file_size = (int32_t)size;
    return (void *)f;
}

static void gif_close_cb(void *pHandle) {
    if (!pHandle) return;
    fclose((FILE *)pHandle);
}

static int32_t gif_read_cb(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    if (!pFile || !pBuf || iLen <= 0) return 0;
    FILE *f = (FILE *)pFile->fHandle;
    if (!f) return 0;
    const size_t n = fread(pBuf, 1, (size_t)iLen, f);
    pFile->iPos += (int32_t)n;
    return (int32_t)n;
}

static int32_t gif_seek_cb(GIFFILE *pFile, int32_t iPosition) {
    if (!pFile) return -1;
    FILE *f = (FILE *)pFile->fHandle;
    if (!f) return -1;
    if (fseek(f, (long)iPosition, SEEK_SET) != 0) {
        return -1;
    }
    pFile->iPos = iPosition;
    return iPosition;
}

bool gif_player_open_path(const char *path, GifRenderCtx *ctx) {
    if (!path || !*path || !ctx) {
        return false;
    }

    // Close previous and attempt open. Re-begin resets internal state, including file position.
    s_gif.close();
    s_gif.begin(GIF_PALETTE_RGB565_LE);
    s_current_gif_file_size = 0;
    const int open_ok = s_gif.open(path, gif_open_cb, gif_close_cb, gif_read_cb, gif_seek_cb, GIFDraw);
    if (!open_ok) {
        const int last_err = s_gif.getLastError();
        ESP_LOGE(TAG, "gif open failed: %s open_ok=%d last_error=%d", path, open_ok, last_err);
        return false;
    }

    snprintf(s_current_gif, sizeof(s_current_gif), "%s", path);

    // Update render ctx with GIF canvas geometry.
    ctx->gif_canvas_w = s_gif.getCanvasWidth();
    ctx->gif_canvas_h = s_gif.getCanvasHeight();
#if CONFIG_TUTORIAL_0013_GIF_SCALE_TO_FULL_SCREEN
    ctx->scale_x = (ctx->gif_canvas_w > 0) ? (ctx->canvas_w / ctx->gif_canvas_w) : 1;
    ctx->scale_y = (ctx->gif_canvas_h > 0) ? (ctx->canvas_h / ctx->gif_canvas_h) : 1;
    if (ctx->scale_x < 1) ctx->scale_x = 1;
    if (ctx->scale_y < 1) ctx->scale_y = 1;
#else
    ctx->scale_x = 1;
    ctx->scale_y = 1;
#endif
    const int scaled_w = ctx->gif_canvas_w * ctx->scale_x;
    const int scaled_h = ctx->gif_canvas_h * ctx->scale_y;
    ctx->off_x = (scaled_w < ctx->canvas_w) ? ((ctx->canvas_w - scaled_w) / 2) : 0;
    ctx->off_y = (scaled_h < ctx->canvas_h) ? ((ctx->canvas_h - scaled_h) / 2) : 0;

    ESP_LOGI(TAG, "gif open ok: %s bytes=%u canvas=%dx%d frame=%dx%d off=(%d,%d)",
             path,
             (unsigned)s_current_gif_file_size,
             s_gif.getCanvasWidth(),
             s_gif.getCanvasHeight(),
             s_gif.getFrameWidth(),
             s_gif.getFrameHeight(),
             s_gif.getFrameXOff(),
             s_gif.getFrameYOff());

    ESP_LOGI(TAG, "gif render: swap_bytes=%d scale=%dx%d gif_canvas=%dx%d -> scaled=%dx%d off=(%d,%d)",
             (int)(0
#if CONFIG_TUTORIAL_0013_GIF_SWAP_BYTES
                   + 1
#endif
                   ),
             ctx->scale_x, ctx->scale_y,
             ctx->gif_canvas_w, ctx->gif_canvas_h,
             scaled_w, scaled_h,
             ctx->off_x, ctx->off_y);

    return true;
}

bool gif_player_open_index(int idx, GifRenderCtx *ctx) {
    const char *path = gif_registry_path(idx);
    if (!path) {
        return false;
    }
    return gif_player_open_path(path, ctx);
}

int gif_player_play_frame(int *out_frame_delay_ms, GifRenderCtx *ctx) {
    if (out_frame_delay_ms) {
        *out_frame_delay_ms = 0;
    }
    // Keep the same semantics as the pre-refactor code:
    // - pass bSync=false
    // - pass an explicit delay output pointer
    // - pass ctx as the user pointer (GIFDraw reads it via pDraw->pUser)
    int frame_delay_ms = 0;
    const int prc = s_gif.playFrame(false, &frame_delay_ms, ctx);
    if (out_frame_delay_ms) {
        *out_frame_delay_ms = frame_delay_ms;
    }
    return prc;
}

int gif_player_last_error(void) {
    return s_gif.getLastError();
}

void gif_player_reset(void) {
    s_gif.reset();
}

int gif_player_canvas_width(void) {
    return s_gif.getCanvasWidth();
}

int gif_player_canvas_height(void) {
    return s_gif.getCanvasHeight();
}

int gif_player_frame_width(void) {
    return s_gif.getFrameWidth();
}

int gif_player_frame_height(void) {
    return s_gif.getFrameHeight();
}

int gif_player_frame_x_off(void) {
    return s_gif.getFrameXOff();
}

int gif_player_frame_y_off(void) {
    return s_gif.getFrameYOff();
}

const char *gif_player_current_path(void) {
    return s_current_gif;
}

int32_t gif_player_current_file_size(void) {
    return s_current_gif_file_size;
}


