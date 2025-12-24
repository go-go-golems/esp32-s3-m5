/*
 * ESP32-S3 tutorial 0013:
 * AtomS3R “GIF console” (real GIF playback + button + esp_console).
 *
 * This file is intentionally small: it is the top-level orchestrator which wires together:
 * - display bring-up + present
 * - backlight control
 * - GIF storage + registry + AnimatedGIF playback
 * - control plane (button ISR + console commands)
 * - optional UART RX heartbeat (debug instrumentation)
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

#include "M5GFX.h"
#include "AnimatedGIF.h"

#include "backlight.h"
#include "console_repl.h"
#include "control_plane.h"
#include "display_hal.h"
#include "uart_rx_heartbeat.h"

#include "echo_gif/gif_player.h"
#include "echo_gif/gif_registry.h"
#include "echo_gif/gif_storage.h"

static const char *TAG = "atoms3r_gif_console";

static void visual_test_solid_colors(void) {
    auto &display = display_get();
    display.fillScreen(TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_WHITE);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(120));
}

extern "C" void app_main(void) {
    const int w = CONFIG_TUTORIAL_0013_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0013_LCD_VRES;

    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    backlight_prepare_for_init();

    ESP_LOGI(TAG, "bringing up M5GFX display...");
    if (!display_init_m5gfx()) {
        ESP_LOGE(TAG, "display init failed, aborting");
        return;
    }

    backlight_enable_after_init();

    ESP_LOGI(TAG, "visual test: solid colors (red/green/blue/white/black)");
    visual_test_solid_colors();

    // Offscreen canvas (LovyanGFX sprite). Keep it out of PSRAM by default:
    // - sprite buffers allocate using AllocationSource::Dma when PSRAM is disabled
    // - that is safest for SPI DMA full-frame presents.
    M5Canvas canvas(&display_get());
#if CONFIG_TUTORIAL_0013_CANVAS_USE_PSRAM
    canvas.setPsram(true);
#else
    canvas.setPsram(false);
#endif
    canvas.setColorDepth(16);
    void *buf = canvas.createSprite(w, h);
    if (!buf) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", w, h);
        return;
    }

    ESP_LOGI(TAG, "canvas ok: %u bytes; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             (unsigned)canvas.bufferLength(),
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    // Control plane wiring.
    QueueHandle_t ctrl_q = ctrl_init();
    if (!ctrl_q) {
        return;
    }

    button_init();
    console_start();
    uart_rx_heartbeat_start();
    button_poll_debug_log();

    // Storage + asset registry.
    esp_err_t storage_err = echo_gif_storage_mount();
    if (storage_err != ESP_OK) {
        ESP_LOGW(TAG, "storage mount failed: %s (no GIFs will be available)", esp_err_to_name(storage_err));
    }
    const int n_gifs = echo_gif_registry_refresh();
    ESP_LOGI(TAG, "gif registry: %d asset(s) found under /storage/gifs", n_gifs);

    EchoGifRenderCtx gif_ctx = {};
    gif_ctx.canvas = &canvas;
    gif_ctx.canvas_w = w;
    gif_ctx.canvas_h = h;

    int gif_idx = 0;
    bool playing = false;
    int delay_ms = CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS;
    uint32_t frame = 0;
    uint32_t last_next_ms = 0;

    if (n_gifs > 0) {
        canvas.fillScreen(TFT_BLACK);
        display_present_canvas(canvas);
        if (echo_gif_player_open_index(gif_idx, &gif_ctx)) {
            playing = true;
            printf("playing: %d (%s)\n", gif_idx, echo_gif_path_basename(echo_gif_registry_path(gif_idx)));
        }
    } else {
        printf("no gifs found under /storage/gifs (flash a storage partition image)\n");
    }

    ESP_LOGI(TAG, "starting GIF playback loop (min_delay_ms=%d dma_present=%d psram=%d)",
             CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS,
             (int)(0
#if CONFIG_TUTORIAL_0013_PRESENT_USE_DMA
                   + 1
#endif
                   ),
             (int)(0
#if CONFIG_TUTORIAL_0013_CANVAS_USE_PSRAM
                   + 1
#endif
                   ));

    while (true) {
        TickType_t wait_ticks = portMAX_DELAY;
        if (playing) {
            const int wms = (delay_ms > 0) ? delay_ms : CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS;
            wait_ticks = pdMS_TO_TICKS((uint32_t)wms);
        }

        CtrlEvent ev = {};
        if (xQueueReceive(ctrl_q, &ev, wait_ticks) == pdTRUE) {
            const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
            switch (ev.type) {
                case CtrlType::PlayIndex: {
                    int idx = (int)ev.arg;
                    if (idx >= 0 && idx < echo_gif_registry_count()) {
                        gif_idx = idx;
                        frame = 0;
                        if (echo_gif_player_open_index(gif_idx, &gif_ctx)) {
                            playing = true;
                            delay_ms = CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS;
                            printf("playing: %d (%s)\n", gif_idx, echo_gif_path_basename(echo_gif_registry_path(gif_idx)));
                        } else {
                            playing = false;
                            printf("failed to open: %d\n", gif_idx);
                        }
                    }
                    break;
                }
                case CtrlType::Stop:
                    playing = false;
                    printf("stopped\n");
                    break;
                case CtrlType::Next: {
                    const uint32_t dt = now_ms - last_next_ms;
                    if (dt < (uint32_t)CONFIG_TUTORIAL_0013_BUTTON_DEBOUNCE_MS) {
                        break;
                    }
                    last_next_ms = now_ms;
                    if (echo_gif_registry_count() <= 0) {
                        break;
                    }
                    gif_idx = (gif_idx + 1) % echo_gif_registry_count();
                    frame = 0;
                    if (echo_gif_player_open_index(gif_idx, &gif_ctx)) {
                        playing = true;
                        delay_ms = CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS;
                        printf("next: %d (%s)\n", gif_idx, echo_gif_path_basename(echo_gif_registry_path(gif_idx)));
                    } else {
                        playing = false;
                        printf("failed to open: %d\n", gif_idx);
                    }
                    break;
                }
                case CtrlType::Info: {
                    const char *cur = echo_gif_player_current_path();
                    printf("state: playing=%d gif=%d/%d name=%s bytes=%u frame=%" PRIu32 " min_delay_ms=%d last_delay_ms=%d\n",
                           playing ? 1 : 0,
                           gif_idx,
                           echo_gif_registry_count(),
                           (cur && cur[0]) ? echo_gif_path_basename(cur) : "(none)",
                           (unsigned)echo_gif_player_current_file_size(),
                           frame,
                           CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS,
                           delay_ms);
                    printf("gif geom: canvas=%dx%d frame=%dx%d off=(%d,%d) render_scale=%dx%d render_off=(%d,%d)\n",
                           echo_gif_player_canvas_width(),
                           echo_gif_player_canvas_height(),
                           echo_gif_player_frame_width(),
                           echo_gif_player_frame_height(),
                           echo_gif_player_frame_x_off(),
                           echo_gif_player_frame_y_off(),
                           gif_ctx.scale_x,
                           gif_ctx.scale_y,
                           gif_ctx.off_x,
                           gif_ctx.off_y);
                    break;
                }
                case CtrlType::SetBrightness: {
                    int v = (int)ev.arg;
                    backlight_set_brightness((uint8_t)v);
                    printf("brightness: %d\n", v);
                    break;
                }
            }
            continue;
        }

        if (!playing) {
            continue;
        }

        int frame_delay_ms = 0;
        const int prc = echo_gif_player_play_frame(&frame_delay_ms, &gif_ctx);
        const int last_err = echo_gif_player_last_error();

        // IMPORTANT: per AnimatedGIF docs:
        // - playFrame() can return 0 even if it successfully rendered a frame (EOF reached right after).
        //   In that case last_err will be GIF_SUCCESS and we still must present the canvas at least once.
        if (prc > 0 || last_err == GIF_SUCCESS) {
            display_present_canvas(canvas);
            frame++;

        if (frame_delay_ms < CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS) {
            frame_delay_ms = CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS;
            }
            delay_ms = frame_delay_ms;
        }

        if (prc == 0) {
            canvas.fillScreen(TFT_BLACK);
            echo_gif_player_reset();
            vTaskDelay(pdMS_TO_TICKS(1));
        } else if (prc < 0) {
            ESP_LOGE(TAG, "gif playFrame failed: last_error=%d", last_err);
            canvas.fillScreen(TFT_BLACK);
            echo_gif_player_reset();
            vTaskDelay(pdMS_TO_TICKS(100));
        }

#if CONFIG_TUTORIAL_0013_LOG_EVERY_N_FRAMES > 0
        if ((frame % (uint32_t)CONFIG_TUTORIAL_0013_LOG_EVERY_N_FRAMES) == 0) {
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " free_heap=%" PRIu32, frame, esp_get_free_heap_size());
        }
#endif
    }
}


