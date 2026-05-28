#include <cstdio>
#include <cstring>
#include <cinttypes>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_console.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "m5dial_board.h"
#include "input_events.h"
#include "framebuffer.h"
#include "palette.h"
#include "scene.h"
#include "renderer.h"
#include "renderer3d.h"
#include "terrain_poster.h"
#include "console_commands.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

static const char* TAG = "main";

// ─── App context ────────────────────────────────────────────

struct AppContext {
    tutorial_0072::M5DialBoard board;
    QueueHandle_t input_queue = nullptr;
    uint8_t* fb = nullptr;              // 2-bit packed framebuffer
    uint16_t rgb565_line[FB_WIDTH];     // scanline expansion buffer
    uint64_t last_frame_us = 0;
    float fps_smooth = 0;
};

static AppContext s_app;

// ─── IO task ───────────────────────────────────────────────

constexpr uint32_t kIoTaskStackSize = 4096;
constexpr UBaseType_t kIoTaskPriority = 6;
constexpr uint32_t kIoPollMs = 8;
constexpr size_t kInputQueueLength = 32;

static void io_task(void* arg) {
    auto* ctx = static_cast<AppContext*>(arg);
    ctx->board.set_button_irq_task(xTaskGetCurrentTaskHandle());

    while (true) {
        ctx->board.poll(ctx->input_queue);
        const TickType_t wait_ticks = pdMS_TO_TICKS(kIoPollMs);
        ulTaskNotifyTake(pdTRUE, wait_ticks == 0 ? 1 : wait_ticks);
    }
}

// ─── Input handling ────────────────────────────────────────

static bool handle_input_event(AppContext* ctx, const tutorial_0072::InputEvent& event) {
    render_params_t* p = render_params_get();

    switch (event.type) {
        case tutorial_0072::InputEventType::kEncoderDelta:
            // Default is one full visual orbit per physical knob turn on the
            // current M5Dial: 2π / 12 ≈ 0.5236 rad per tactile click.  The
            // `sensitivity` console command can tune this without reflashing.
            p->camera_angle += event.value * p->encoder_step;
            return true;

        case tutorial_0072::InputEventType::kButtonShortPress:
            palette_cycle_next();
            ESP_LOGI(TAG, "palette: %s", palette_current()->name);
            return true;

        case tutorial_0072::InputEventType::kButtonLongPress:
            // Toggle auto-rotate
            if (p->auto_rotate_speed != 0.0f) {
                p->auto_rotate_speed = 0.0f;
            } else {
                p->auto_rotate_speed = 0.25f;
            }
            ESP_LOGI(TAG, "auto-rotate: %.2f rad/s", p->auto_rotate_speed);
            return true;

        case tutorial_0072::InputEventType::kSwipe: {
            // Swipe cycles scenes
            scene_cycle_next();
            return true;
        }
    }
    return false;
}

// ─── Console REPL ──────────────────────────────────────────

static void start_console_repl() {
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "3d> ";
    repl_config.max_cmdline_length = 256;
    repl_config.task_stack_size = 4096;
    repl_config.task_priority = 3;

    esp_console_dev_usb_serial_jtag_config_t dev_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    esp_console_repl_t* repl = nullptr;
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&dev_config, &repl_config, &repl));

    ESP_ERROR_CHECK(esp_console_register_help_command());
    console_commands_register();

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    ESP_LOGI(TAG, "Console ready on USB Serial/JTAG");
}

// ─── App render loop ───────────────────────────────────────

constexpr uint32_t kAppTaskStackSize = 16384;
constexpr UBaseType_t kAppTaskPriority = 5;

static void app_task(void* arg) {
    auto* ctx = static_cast<AppContext*>(arg);

    // Initialize scene
    scene_set(SCENE_TERRAIN);

    ESP_LOGI(TAG, "render loop started");

    bool dirty = true;
    uint32_t seen_revision = render_params_get()->revision;

    while (true) {
        // Process input events
        tutorial_0072::InputEvent event;
        while (xQueueReceive(ctx->input_queue, &event, 0) == pdTRUE) {
            dirty = handle_input_event(ctx, event) || dirty;
        }

        render_params_t* p = render_params_get();
        scene_def_t* scene = scene_current();
        const palette_t* pal = palette_current();
        if (p->revision != seen_revision) {
            seen_revision = p->revision;
            dirty = true;
        }

        // Auto-rotate.  With auto-rotate disabled we only repaint after input,
        // which removes visible SPI tearing/flicker on the round LCD.
        if (!p->paused && p->auto_rotate_speed != 0.0f) {
            uint64_t now = esp_timer_get_time();
            float dt = (float)(now - ctx->last_frame_us) / 1000000.0f;
            if (dt > 0.1f) dt = 0.1f;  // clamp
            p->camera_angle += p->auto_rotate_speed * dt;
            dirty = true;
        }

        if (p->paused || !dirty) {
            vTaskDelay(pdMS_TO_TICKS(25));
            continue;
        }

        // Update scene animation
        if (scene->update) {
            float time = (float)esp_timer_get_time() / 1000000.0f;
            scene->update(scene, time);
        }

        uint64_t frame_start_us = esp_timer_get_time();
        if (p->backend == RENDER_BACKEND_PLANET3D) {
            renderer3d_render_planet(ctx->fb, p);
        } else if (p->backend == RENDER_BACKEND_TERRAIN3D) {
            renderer3d_render_terrain(ctx->fb, p);
        } else {
            (void)scene;
            poster_render_scene(ctx->fb, scene_current_id(), p);
        }

        // Push framebuffer to display
        tutorial_0072::LGFX_M5Dial& display = ctx->board.display();
        display.startWrite();
        // Our scanline buffer is host-order RGB565.  LovyanGFX must swap bytes
        // when streaming uint16_t RGB565 over SPI, otherwise red/blue values
        // appear as greenish byte-swapped colors on the GC9A01.
        display.setSwapBytes(true);
        display.setAddrWindow(0, 0, FB_WIDTH, FB_HEIGHT);
        for (int y = 0; y < FB_HEIGHT; y++) {
            fb_expand_scanline(ctx->fb, y, ctx->rgb565_line, pal->colors);
            display.writePixels(ctx->rgb565_line, FB_WIDTH);
        }
        display.endWrite();

        uint64_t frame_end_us = esp_timer_get_time();
        if (p->backend == RENDER_BACKEND_PLANET3D) {
            const renderer3d_stats_t* r3d = renderer3d_stats();
            renderer_stats_record(r3d->triangles_submitted,
                                  r3d->triangles_drawn,
                                  r3d->planet_pixels + r3d->terrain_pixels + r3d->ring_pixels + r3d->sun_pixels + r3d->moon_pixels,
                                  frame_end_us - frame_start_us);
        } else {
            renderer_stats_record(0, 0, FB_WIDTH * FB_HEIGHT, frame_end_us - frame_start_us);
        }

        // FPS tracking
        uint64_t now_us = frame_end_us;
        uint64_t total_us = now_us - ctx->last_frame_us;
        ctx->last_frame_us = now_us;
        dirty = false;
        float fps = total_us > 0 ? 1000000.0f / (float)total_us : 0;
        ctx->fps_smooth = ctx->fps_smooth * 0.9f + fps * 0.1f;

        // Yield — target ~10 FPS minimum
        if (total_us < 80000) {  // < 80 ms
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

// ─── Main ───────────────────────────────────────────────────

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== M5Dial Dithered 3D Scene Viewer ===");

    // Init NVS (required for esp_console)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init board
    if (!s_app.board.init()) {
        ESP_LOGE(TAG, "board init failed");
        return;
    }

    // Init framebuffer
    if (!fb_init()) {
        ESP_LOGE(TAG, "framebuffer init failed");
        return;
    }
    s_app.fb = fb_buffer();
    console_commands_set_framebuffer(s_app.fb);

    // Init renderers
    if (!renderer_init()) {
        ESP_LOGE(TAG, "renderer init failed");
        return;
    }
    if (!renderer3d_init()) {
        ESP_LOGE(TAG, "renderer3d init failed");
        return;
    }

    // Create input queue
    s_app.input_queue = xQueueCreate(kInputQueueLength, sizeof(tutorial_0072::InputEvent));
    if (!s_app.input_queue) {
        ESP_LOGE(TAG, "input queue creation failed");
        return;
    }

    s_app.last_frame_us = esp_timer_get_time();

    // Start tasks
    start_console_repl();
    xTaskCreatePinnedToCore(app_task, "3d_app", kAppTaskStackSize, &s_app, kAppTaskPriority, nullptr, 1);
    xTaskCreatePinnedToCore(io_task, "3d_io", kIoTaskStackSize, &s_app, kIoTaskPriority, nullptr, 0);

    ESP_LOGI(TAG, "booted — use 'help' for commands");
}
