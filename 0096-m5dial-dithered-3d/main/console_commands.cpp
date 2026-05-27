#include "console_commands.h"
#include "scene.h"
#include "palette.h"
#include "renderer.h"
#include "framebuffer.h"

#include <esp_console.h>
#include <esp_log.h>
#include <argtable3/argtable3.h>
#include <cinttypes>
#include <cstdio>
#include <cstring>

static const char* TAG = "console_cmd";
static const uint8_t* s_dump_framebuffer = nullptr;

void console_commands_set_framebuffer(const uint8_t* framebuffer) {
    s_dump_framebuffer = framebuffer;
}

// ─── scene command ──────────────────────────────────────────

static struct {
    struct arg_str* name;
    struct arg_end* end;
} scene_args;

static int cmd_scene(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&scene_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, scene_args.end, argv[0]);
        return 1;
    }

    if (scene_args.name->count == 0) {
        // Show current scene
        printf("Current scene: %s\n", scene_current()->name);
        return 0;
    }

    const char* name = scene_args.name->sval[0];
    const char* names[] = {"terrain", "torus", "ocean", "planet", "tunnel"};
    for (int i = 0; i < 5; i++) {
        if (strcasecmp(name, names[i]) == 0) {
            scene_set((SceneId)i);
            printf("Scene: %s\n", scene_current()->name);
            return 0;
        }
    }
    printf("Unknown scene: %s (terrain|torus|ocean|planet|tunnel)\n", name);
    return 1;
}

// ─── palette command ────────────────────────────────────────

static struct {
    struct arg_str* name;
    struct arg_end* end;
} palette_args;

static int cmd_palette(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&palette_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, palette_args.end, argv[0]);
        return 1;
    }

    if (palette_args.name->count == 0) {
        printf("Current palette: %s\n", palette_current()->name);
        return 0;
    }

    const char* name = palette_args.name->sval[0];
    const char* names[] = {"classic", "inverted", "red", "blue", "amber"};
    for (int i = 0; i < 5; i++) {
        if (strcasecmp(name, names[i]) == 0) {
            palette_set((PaletteId)i);
            render_params_touch();
            printf("Palette: %s\n", palette_current()->name);
            return 0;
        }
    }
    printf("Unknown palette: %s (classic|inverted|red|blue|amber)\n", name);
    return 1;
}

// ─── rotate command ─────────────────────────────────────────

static struct {
    struct arg_dbl* speed;
    struct arg_end* end;
} rotate_args;

static int cmd_rotate(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&rotate_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, rotate_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (rotate_args.speed->count > 0) {
        p->auto_rotate_speed = (float)rotate_args.speed->dval[0];
        render_params_touch();
        printf("Auto-rotate speed: %.2f rad/s\n", p->auto_rotate_speed);
    } else {
        printf("Auto-rotate speed: %.2f rad/s\n", p->auto_rotate_speed);
    }
    return 0;
}

// ─── contrast command ───────────────────────────────────────

static struct {
    struct arg_dbl* value;
    struct arg_end* end;
} contrast_args;

static int cmd_contrast(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&contrast_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, contrast_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (contrast_args.value->count > 0) {
        p->contrast = (float)contrast_args.value->dval[0];
        if (p->contrast < 0.6f) p->contrast = 0.6f;
        if (p->contrast > 2.5f) p->contrast = 2.5f;
        render_params_touch();
        printf("Contrast: %.2f\n", p->contrast);
    } else {
        printf("Contrast: %.2f\n", p->contrast);
    }
    return 0;
}

// ─── aperture command ───────────────────────────────────────

static struct {
    struct arg_dbl* value;
    struct arg_end* end;
} aperture_args;

static int cmd_aperture(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&aperture_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, aperture_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (aperture_args.value->count > 0) {
        p->aperture = (float)aperture_args.value->dval[0] / 100.0f;
        if (p->aperture < 0.4f) p->aperture = 0.4f;
        if (p->aperture > 1.0f) p->aperture = 1.0f;
        render_params_touch();
        printf("Aperture: %d%%\n", (int)(p->aperture * 100));
    } else {
        printf("Aperture: %d%%\n", (int)(p->aperture * 100));
    }
    return 0;
}

// ─── pixel command ──────────────────────────────────────────

static struct {
    struct arg_int* value;
    struct arg_end* end;
} pixel_args;

static int cmd_pixel(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&pixel_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, pixel_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (pixel_args.value->count > 0) {
        p->pixel_size = pixel_args.value->ival[0];
        if (p->pixel_size < 1) p->pixel_size = 1;
        if (p->pixel_size > 6) p->pixel_size = 6;
        render_params_touch();
        printf("Pixel size: %d\n", p->pixel_size);
    } else {
        printf("Pixel size: %d\n", p->pixel_size);
    }
    return 0;
}

// ─── sensitivity command ────────────────────────────────────

static struct {
    struct arg_dbl* clicks;
    struct arg_end* end;
} sensitivity_args;

static int cmd_sensitivity(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&sensitivity_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, sensitivity_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (sensitivity_args.clicks->count > 0) {
        float clicks = (float)sensitivity_args.clicks->dval[0];
        if (clicks < 2.0f) clicks = 2.0f;
        if (clicks > 96.0f) clicks = 96.0f;
        p->encoder_step = 6.2831853f / clicks;
        render_params_touch();
    }
    printf("Encoder sensitivity: %.2f clicks/rev (%.4f rad/click)\n",
           6.2831853f / p->encoder_step, p->encoder_step);
    return 0;
}

// ─── debug-ui command ───────────────────────────────────────

static struct {
    struct arg_str* mode;
    struct arg_end* end;
} debug_args;

static int cmd_debug(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&debug_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, debug_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (debug_args.mode->count > 0) {
        const char* mode = debug_args.mode->sval[0];
        if (strcasecmp(mode, "on") == 0) p->debug_ui = true;
        else if (strcasecmp(mode, "off") == 0) p->debug_ui = false;
        else { printf("Usage: debug [on|off]\n"); return 1; }
        render_params_touch();
    }
    printf("Debug UI: %s\n", p->debug_ui ? "ON" : "OFF");
    return 0;
}

// ─── dumpfb command ─────────────────────────────────────────

static int cmd_dumpfb(int argc, char** argv) {
    if (!s_dump_framebuffer) {
        printf("No framebuffer registered\n");
        return 1;
    }

    const palette_t* pal = palette_current();
    printf("DUMPFB_BEGIN width=%d height=%d bpp=2 bytes=%d palette=%s\n",
           FB_WIDTH, FB_HEIGHT, FB_TOTAL_BYTES, pal->name);
    printf("PALETTE %04X %04X %04X %04X\n",
           pal->colors[0], pal->colors[1], pal->colors[2], pal->colors[3]);
    for (int y = 0; y < FB_HEIGHT; ++y) {
        const uint8_t* row = s_dump_framebuffer + y * FB_BYTES_PER_ROW;
        printf("ROW %03d ", y);
        for (int i = 0; i < FB_BYTES_PER_ROW; ++i) {
            printf("%02X", row[i]);
        }
        printf("\n");
    }
    printf("DUMPFB_END\n");
    return 0;
}

// ─── fps command ────────────────────────────────────────────

static int cmd_fps(int argc, char** argv) {
    const render_stats_t* s = renderer_stats();
    printf("Frame time: %llu us (%.1f FPS)\n",
           (unsigned long long)s->frame_time_us,
           s->frame_time_us > 0 ? 1000000.0f / (float)s->frame_time_us : 0.0f);
    printf("Mode: %s\n", s->triangles_submitted == 0 ? "poster" : "triangle");
    printf("Triangles: %" PRIu32 " submitted, %" PRIu32 " drawn\n",
           s->triangles_submitted, s->triangles_drawn);
    printf("Pixels written: %" PRIu32 "\n", s->pixels_written);
    return 0;
}

// ─── wireframe command ─────────────────────────────────────

static struct {
    struct arg_str* mode;
    struct arg_end* end;
} wireframe_args;

static int cmd_wireframe(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&wireframe_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wireframe_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (wireframe_args.mode->count > 0) {
        const char* mode = wireframe_args.mode->sval[0];
        if (strcasecmp(mode, "on") == 0) p->wireframe = true;
        else if (strcasecmp(mode, "off") == 0) p->wireframe = false;
        else { printf("Usage: wireframe [on|off]\n"); return 1; }
        render_params_touch();
    }
    printf("Wireframe: %s\n", p->wireframe ? "ON" : "OFF");
    return 0;
}

// ─── pause command ─────────────────────────────────────────

static int cmd_pause(int argc, char** argv) {
    render_params_t* p = render_params_get();
    p->paused = !p->paused;
    render_params_touch();
    printf("Rendering: %s\n", p->paused ? "PAUSED" : "RUNNING");
    return 0;
}

// ─── angle command ──────────────────────────────────────────

static struct {
    struct arg_dbl* value;
    struct arg_end* end;
} angle_args;

static int cmd_angle(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**)&angle_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, angle_args.end, argv[0]);
        return 1;
    }

    render_params_t* p = render_params_get();
    if (angle_args.value->count > 0) {
        p->camera_angle = (float)angle_args.value->dval[0];
        render_params_touch();
        printf("Camera angle: %.2f rad\n", p->camera_angle);
    } else {
        printf("Camera angle: %.2f rad\n", p->camera_angle);
    }
    return 0;
}

// ─── Register all commands ─────────────────────────────────

void console_commands_register(void) {
    scene_args.name = arg_str0(NULL, NULL, "<name>", "Scene: terrain|torus|ocean|planet|tunnel");
    scene_args.end = arg_end(1);
    const esp_console_cmd_t scene_cmd = {
        .command = "scene",
        .help = "Select or show current 3D scene",
        .hint = NULL,
        .func = &cmd_scene,
        .argtable = &scene_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&scene_cmd));

    palette_args.name = arg_str0(NULL, NULL, "<name>", "Palette: classic|inverted|red|blue|amber");
    palette_args.end = arg_end(1);
    const esp_console_cmd_t palette_cmd = {
        .command = "palette",
        .help = "Select or show current color palette",
        .hint = NULL,
        .func = &cmd_palette,
        .argtable = &palette_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&palette_cmd));

    rotate_args.speed = arg_dbl0(NULL, NULL, "<speed>", "Auto-rotation speed in rad/s (-1.5 to 1.5)");
    rotate_args.end = arg_end(1);
    const esp_console_cmd_t rotate_cmd = {
        .command = "rotate",
        .help = "Set or show auto-rotation speed",
        .hint = NULL,
        .func = &cmd_rotate,
        .argtable = &rotate_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&rotate_cmd));

    contrast_args.value = arg_dbl0(NULL, NULL, "<value>", "Contrast (0.6–2.5)");
    contrast_args.end = arg_end(1);
    const esp_console_cmd_t contrast_cmd = {
        .command = "contrast",
        .help = "Set or show contrast",
        .hint = NULL,
        .func = &cmd_contrast,
        .argtable = &contrast_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&contrast_cmd));

    aperture_args.value = arg_dbl0(NULL, NULL, "<pct>", "Aperture percentage (40–100)");
    aperture_args.end = arg_end(1);
    const esp_console_cmd_t aperture_cmd = {
        .command = "aperture",
        .help = "Set or show circular mask radius",
        .hint = NULL,
        .func = &cmd_aperture,
        .argtable = &aperture_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&aperture_cmd));

    pixel_args.value = arg_int0(NULL, NULL, "<size>", "Poster pixel block size (1–6)");
    pixel_args.end = arg_end(1);
    const esp_console_cmd_t pixel_cmd = {
        .command = "pixel",
        .help = "Set or show poster pixel block size",
        .hint = NULL,
        .func = &cmd_pixel,
        .argtable = &pixel_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&pixel_cmd));

    sensitivity_args.clicks = arg_dbl0(NULL, NULL, "<clicks>", "Encoder clicks per full visual revolution (2–96)");
    sensitivity_args.end = arg_end(1);
    const esp_console_cmd_t sensitivity_cmd = {
        .command = "sensitivity",
        .help = "Set or show encoder rotation sensitivity",
        .hint = NULL,
        .func = &cmd_sensitivity,
        .argtable = &sensitivity_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&sensitivity_cmd));

    debug_args.mode = arg_str0(NULL, NULL, "<on|off>", "Diagnostic UI overlays");
    debug_args.end = arg_end(1);
    const esp_console_cmd_t debug_cmd = {
        .command = "debug",
        .help = "Toggle palette probes and diagnostic overlays",
        .hint = NULL,
        .func = &cmd_debug,
        .argtable = &debug_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&debug_cmd));

    const esp_console_cmd_t dumpfb_cmd = {
        .command = "dumpfb",
        .help = "Dump the packed 2-bit framebuffer as hex rows for host-side screenshot reconstruction",
        .hint = NULL,
        .func = &cmd_dumpfb,
        .argtable = NULL,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&dumpfb_cmd));

    const esp_console_cmd_t fps_cmd = {
        .command = "fps",
        .help = "Show frame rate and render stats",
        .hint = NULL,
        .func = &cmd_fps,
        .argtable = NULL,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&fps_cmd));

    wireframe_args.mode = arg_str0(NULL, NULL, "<on|off>", "Wireframe overlay");
    wireframe_args.end = arg_end(1);
    const esp_console_cmd_t wireframe_cmd = {
        .command = "wireframe",
        .help = "Toggle wireframe overlay",
        .hint = NULL,
        .func = &cmd_wireframe,
        .argtable = &wireframe_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wireframe_cmd));

    const esp_console_cmd_t pause_cmd = {
        .command = "pause",
        .help = "Pause/resume rendering",
        .hint = NULL,
        .func = &cmd_pause,
        .argtable = NULL,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&pause_cmd));

    angle_args.value = arg_dbl0(NULL, NULL, "<radians>", "Camera orbit angle in radians");
    angle_args.end = arg_end(1);
    const esp_console_cmd_t angle_cmd = {
        .command = "angle",
        .help = "Set or show camera orbit angle",
        .hint = NULL,
        .func = &cmd_angle,
        .argtable = &angle_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&angle_cmd));

    ESP_LOGI(TAG, "console commands registered");
}
