// 0102 — ESP32-P4 visual QuickJS REPL skeleton.
// Console = UART0 (CH343 USB-UART bridge; the P4 has no USB Serial/JTAG).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "picocalc_keyboard.h"
#include "picocalc_lcd.h"
#include "qjs_service.h"
#include "visual_repl.h"

namespace {
constexpr const char *kTag = "0102";
constexpr size_t kQuickJsMemoryLimit = 2 * 1024 * 1024;
constexpr size_t kQuickJsStackLimit = 64 * 1024;
constexpr uint32_t kEvalTimeoutMs = 1000;
constexpr size_t kMaxEvalSource = 2048;
constexpr TickType_t kKeyboardPollDelay = pdMS_TO_TICKS(20);

qjs_service_t *g_qjs = nullptr;
TaskHandle_t g_keyboard_task = nullptr;
char g_input[VISUAL_REPL_INPUT_MAX + 1] = {};
size_t g_input_len = 0;
size_t g_input_cursor = 0;

uint16_t color_from_name(const char *name, bool *ok)
{
    *ok = true;
    if (!name || std::strcmp(name, "black") == 0) return PICOCALC_LCD_RGB565_BLACK;
    if (std::strcmp(name, "white") == 0) return PICOCALC_LCD_RGB565_WHITE;
    if (std::strcmp(name, "red") == 0) return PICOCALC_LCD_RGB565_RED;
    if (std::strcmp(name, "green") == 0) return PICOCALC_LCD_RGB565_GREEN;
    if (std::strcmp(name, "blue") == 0) return PICOCALC_LCD_RGB565_BLUE;
    if (std::strcmp(name, "yellow") == 0) return PICOCALC_LCD_RGB565_YELLOW;
    if (std::strcmp(name, "cyan") == 0) return PICOCALC_LCD_RGB565_CYAN;
    if (std::strcmp(name, "magenta") == 0) return PICOCALC_LCD_RGB565_MAGENTA;
    *ok = false;
    return 0;
}

void print_color_names()
{
    std::printf("colors: black white red green blue yellow cyan magenta\n");
}

int parse_int_arg(const char *s, int min_value, int max_value, bool *ok)
{
    if (!s) {
        *ok = false;
        return 0;
    }
    char *end = nullptr;
    long value = std::strtol(s, &end, 0);
    if (!end || *end != 0 || value < min_value || value > max_value) {
        *ok = false;
        return 0;
    }
    *ok = true;
    return static_cast<int>(value);
}

esp_err_t draw_color_swatches()
{
    struct Swatch {
        const char *name;
        uint16_t color;
        uint16_t x;
        uint16_t y;
    };
    constexpr uint16_t kCellW = 140;
    constexpr uint16_t kCellH = 60;
    const Swatch swatches[] = {
        {"black", PICOCALC_LCD_RGB565_BLACK, 10, 10},
        {"white", PICOCALC_LCD_RGB565_WHITE, 170, 10},
        {"red", PICOCALC_LCD_RGB565_RED, 10, 86},
        {"green", PICOCALC_LCD_RGB565_GREEN, 170, 86},
        {"blue", PICOCALC_LCD_RGB565_BLUE, 10, 162},
        {"yellow", PICOCALC_LCD_RGB565_YELLOW, 170, 162},
        {"cyan", PICOCALC_LCD_RGB565_CYAN, 10, 238},
        {"magenta", PICOCALC_LCD_RGB565_MAGENTA, 170, 238},
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(picocalc_lcd_fill(0x8410)); // neutral RGB565 gray background
    for (const auto &s : swatches) {
        esp_err_t err = picocalc_lcd_fill_rect(s.x, s.y, kCellW, kCellH, s.color);
        if (err != ESP_OK) {
            return err;
        }
        std::printf("swatch %-7s rgb565=0x%04x rect=(%u,%u,%u,%u)\n",
                    s.name, s.color, s.x, s.y, kCellW, kCellH);
    }
    std::printf("layout: left column x=10, right column x=170; rows y=10 black/white, y=86 red/green, y=162 blue/yellow, y=238 cyan/magenta\n");
    return ESP_OK;
}

void sync_visual_input()
{
    (void)visual_repl_set_input(g_input, g_input_cursor);
    (void)visual_repl_render_input();
}

void reset_input_line()
{
    g_input[0] = 0;
    g_input_len = 0;
    g_input_cursor = 0;
    sync_visual_input();
}

void clear_input_model_without_render()
{
    g_input[0] = 0;
    g_input_len = 0;
    g_input_cursor = 0;
    (void)visual_repl_set_input(g_input, g_input_cursor);
}

void append_visual_text(visual_repl_style_t style, const char *text)
{
    if (!text || text[0] == 0) {
        return;
    }

    char line[VISUAL_REPL_COLS + 1] = {};
    size_t line_len = 0;
    bool emitted = false;
    for (const char *p = text; *p; ++p) {
        const char ch = *p;
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            line[line_len] = 0;
            (void)visual_repl_append_line(style, line);
            line_len = 0;
            emitted = true;
            continue;
        }
        line[line_len++] = (ch >= 0x20 && ch <= 0x7e) ? ch : '?';
        if (line_len == VISUAL_REPL_COLS) {
            line[line_len] = 0;
            (void)visual_repl_append_line(style, line);
            line_len = 0;
            emitted = true;
        }
    }
    if (line_len > 0 || !emitted) {
        line[line_len] = 0;
        (void)visual_repl_append_line(style, line);
    }
}

void append_visual_status()
{
    qjs_service_status_t qst = {};
    esp_err_t qerr = g_qjs ? qjs_service_get_status(g_qjs, &qst, 1000) : ESP_ERR_INVALID_STATE;
    char line[VISUAL_REPL_COLS + 1] = {};
    std::snprintf(line, sizeof(line), "QJS %s READY=%d EVALS=%u", esp_err_to_name(qerr), qst.ready, (unsigned)qst.eval_count);
    (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, line);
    std::snprintf(line, sizeof(line), "HEAP INT=%u PSRAM=%u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, line);
}

void evaluate_visual_input(const char *source)
{
    if (!source || source[0] == 0) {
        return;
    }
    if (!g_qjs) {
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, "QUICKJS SERVICE UNAVAILABLE");
        return;
    }

    if (std::strcmp(source, "/reset") == 0) {
        esp_err_t err = qjs_service_reset(g_qjs, 2000);
        char line[VISUAL_REPL_COLS + 1] = {};
        std::snprintf(line, sizeof(line), "RESET: %s", esp_err_to_name(err));
        (void)visual_repl_append_line(err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
        return;
    }
    if (std::strcmp(source, "/status") == 0) {
        append_visual_status();
        return;
    }
    if (std::strcmp(source, "/help") == 0) {
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "COMMANDS: /help /status /reset");
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "JS: PRINT(1+2), THROW NEW ERROR()...");
        return;
    }

    qjs_eval_result_t r = {};
    esp_err_t err = qjs_service_eval(g_qjs, source, std::strlen(source), kEvalTimeoutMs, "<lcd-repl>", &r);
    if (err != ESP_OK) {
        char line[VISUAL_REPL_COLS + 1] = {};
        std::snprintf(line, sizeof(line), "SERVICE ERROR: %s", esp_err_to_name(err));
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, line);
        return;
    }

    char meta[VISUAL_REPL_COLS + 1] = {};
    std::snprintf(meta, sizeof(meta), "OK=%d TIMEOUT=%d %uMS", r.ok, r.timed_out, (unsigned)r.elapsed_ms);
    (void)visual_repl_append_line(r.ok && !r.timed_out ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, meta);
    append_visual_text(VISUAL_REPL_STYLE_OUTPUT, r.output);
    append_visual_text(VISUAL_REPL_STYLE_ERROR, r.error);
    qjs_eval_result_free(&r);
}

void insert_input_char(char ch)
{
    if (g_input_len >= VISUAL_REPL_INPUT_MAX) {
        return;
    }
    std::memmove(g_input + g_input_cursor + 1,
                 g_input + g_input_cursor,
                 g_input_len - g_input_cursor + 1);
    g_input[g_input_cursor] = ch;
    ++g_input_cursor;
    ++g_input_len;
    sync_visual_input();
}

void backspace_input_char()
{
    if (g_input_cursor == 0) {
        return;
    }
    std::memmove(g_input + g_input_cursor - 1,
                 g_input + g_input_cursor,
                 g_input_len - g_input_cursor + 1);
    --g_input_cursor;
    --g_input_len;
    sync_visual_input();
}

void delete_input_char()
{
    if (g_input_cursor >= g_input_len) {
        return;
    }
    std::memmove(g_input + g_input_cursor,
                 g_input + g_input_cursor + 1,
                 g_input_len - g_input_cursor);
    --g_input_len;
    sync_visual_input();
}

void submit_input_line()
{
    char source[VISUAL_REPL_INPUT_MAX + 1] = {};
    std::memcpy(source, g_input, sizeof(source));

    char submitted[VISUAL_REPL_INPUT_MAX + 3] = {};
    std::snprintf(submitted, sizeof(submitted), "> %s", source);
    (void)visual_repl_append_line(VISUAL_REPL_STYLE_PROMPT, submitted);
    clear_input_model_without_render();
    evaluate_visual_input(source);
    (void)visual_repl_render();
}

bool handle_editor_key(uint8_t key)
{
    switch (key) {
        case 0x08: // Backspace
            backspace_input_char();
            return true;
        case 0x0a: // Enter
        case 0x0d:
            submit_input_line();
            return true;
        case 0xb1: // Escape
            reset_input_line();
            return true;
        case 0xb4: // Left
            if (g_input_cursor > 0) {
                --g_input_cursor;
                sync_visual_input();
            }
            return true;
        case 0xb7: // Right
            if (g_input_cursor < g_input_len) {
                ++g_input_cursor;
                sync_visual_input();
            }
            return true;
        case 0xd2: // Home
            g_input_cursor = 0;
            sync_visual_input();
            return true;
        case 0xd4: // Delete
            delete_input_char();
            return true;
        case 0xd5: // End
            g_input_cursor = g_input_len;
            sync_visual_input();
            return true;
        default:
            break;
    }

    if (key >= 0x20 && key <= 0x7e) {
        insert_input_char(static_cast<char>(key));
        return true;
    }
    return false;
}

void keyboard_task(void *)
{
    ESP_LOGI(kTag, "visual keyboard editor task started");
    vTaskDelay(pdMS_TO_TICKS(1000));
    uint32_t consecutive_errors = 0;
    while (true) {
        picocalc_key_event_t ev = {};
        esp_err_t err = picocalc_keyboard_poll_event(&ev);
        if (err == ESP_ERR_NOT_FOUND) {
            consecutive_errors = 0;
            vTaskDelay(kKeyboardPollDelay);
            continue;
        }
        if (err != ESP_OK) {
            ++consecutive_errors;
            if (consecutive_errors == 1 || consecutive_errors % 10 == 0) {
                ESP_LOGW(kTag, "keyboard poll failed: %s consecutive_errors=%u",
                         esp_err_to_name(err), (unsigned)consecutive_errors);
            }
            if (consecutive_errors == 5 || consecutive_errors % 30 == 0) {
                esp_err_t rec = picocalc_keyboard_recover();
                ESP_LOGW(kTag, "keyboard recovery after poll errors: %s", esp_err_to_name(rec));
            }
            const TickType_t delay = consecutive_errors < 5 ? pdMS_TO_TICKS(250) : pdMS_TO_TICKS(1000);
            vTaskDelay(delay);
            continue;
        }
        consecutive_errors = 0;
        if (!ev.valid) {
            continue;
        }
        if (ev.state == PICOCALC_KBD_STATE_PRESSED || ev.state == PICOCALC_KBD_STATE_REPEATED) {
            const bool handled = handle_editor_key(ev.key);
            ESP_LOGI(kTag, "kbd editor key=0x%02x(%s) state=%s handled=%d input_len=%u cursor=%u",
                     ev.key, picocalc_keyboard_key_name(ev.key),
                     picocalc_keyboard_state_name(ev.state), handled,
                     (unsigned)g_input_len, (unsigned)g_input_cursor);
        }
    }
}

qjs_service_t *start_quickjs_service()
{
    qjs_service_config_t cfg = {};
    cfg.task_name = "qjs0102";
    cfg.task_stack_words = 32768;
    cfg.task_priority = 8;
    cfg.task_core_id = -1;
    cfg.queue_len = 8;
    cfg.memory_limit_bytes = kQuickJsMemoryLimit;
    cfg.stack_limit_bytes = kQuickJsStackLimit;
    cfg.can_block = false;

    qjs_service_t *svc = nullptr;
    esp_err_t err = qjs_service_start(&cfg, &svc);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "qjs_service_start failed: %s", esp_err_to_name(err));
        return nullptr;
    }
    return svc;
}

bool join_source(int argc, char **argv, int first, char *buf, size_t buf_len)
{
    size_t n = 0;
    for (int i = first; i < argc; ++i) {
        const size_t l = std::strlen(argv[i]);
        if (n + l + (i > first ? 1 : 0) + 1 > buf_len) {
            std::printf("source too long (max %zu)\n", buf_len - 1);
            return false;
        }
        if (i > first) buf[n++] = ' ';
        std::memcpy(buf + n, argv[i], l);
        n += l;
    }
    buf[n] = 0;
    return true;
}

int cmd_status(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    picocalc_keyboard_diag_t kdiag = {};
    picocalc_keyboard_get_diag(&kdiag);

    qjs_service_status_t qst = {};
    esp_err_t qerr = g_qjs ? qjs_service_get_status(g_qjs, &qst, 1000) : ESP_ERR_INVALID_STATE;
    visual_repl_status_t vst = {};
    visual_repl_get_status(&vst);

    std::printf("0102 status: lcd_requested=%d actual_khz=%d max_transfer=%u\n",
                picocalc_lcd_requested_hz(), picocalc_lcd_actual_khz(),
                (unsigned)picocalc_lcd_max_transfer_bytes());
    std::printf("keyboard: initialized=%d last_status=0x%02x errors=%u recoveries=%u last_error=%s\n",
                kdiag.initialized, kdiag.last_status, (unsigned)kdiag.error_count,
                (unsigned)kdiag.recover_count, esp_err_to_name(kdiag.last_error));
    std::printf("quickjs: status_err=%s ready=%d evals=%u resets=%u last_eval_ms=%u used=%u atoms=%u\n",
                esp_err_to_name(qerr), qst.ready, (unsigned)qst.eval_count,
                (unsigned)qst.reset_count, (unsigned)qst.last_eval_ms,
                (unsigned)qst.memory_used_size, (unsigned)qst.atom_count);
    std::printf("visual: initialized=%d grid=%ux%u cell=%ux%u history=%u renders=%u last_render_ms=%u\n",
                vst.initialized, (unsigned)vst.cols, (unsigned)vst.rows,
                (unsigned)vst.cell_w, (unsigned)vst.cell_h, (unsigned)vst.history_count,
                (unsigned)vst.render_count, (unsigned)vst.last_render_ms);
    std::printf("heap: internal=%u 8bit=%u psram=%u\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return 0;
}

int cmd_lcd(int argc, char **argv)
{
    if (argc < 2 || std::strcmp(argv[1], "init") == 0) {
        esp_err_t err = picocalc_lcd_init();
        std::printf("lcd init: %s actual_khz=%d\n", esp_err_to_name(err), picocalc_lcd_actual_khz());
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "fill") == 0) {
        bool ok = false;
        uint16_t color = color_from_name(argc >= 3 ? argv[2] : "black", &ok);
        if (!ok) {
            std::printf("usage: lcd fill <color>\n");
            print_color_names();
            return 1;
        }
        const int64_t start = esp_timer_get_time();
        esp_err_t err = picocalc_lcd_fill(color);
        const int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        std::printf("lcd fill color=0x%04x err=%s elapsed_ms=%lld actual_khz=%d\n",
                    color, esp_err_to_name(err), (long long)elapsed_ms, picocalc_lcd_actual_khz());
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "rect") == 0) {
        if (argc < 7) {
            std::printf("usage: lcd rect <x> <y> <w> <h> <color>\n");
            print_color_names();
            return 1;
        }
        bool x_ok = false;
        bool y_ok = false;
        bool w_ok = false;
        bool h_ok = false;
        bool color_ok = false;
        int x = parse_int_arg(argv[2], 0, PICOCALC_LCD_WIDTH - 1, &x_ok);
        int y = parse_int_arg(argv[3], 0, PICOCALC_LCD_HEIGHT - 1, &y_ok);
        int w = parse_int_arg(argv[4], 1, PICOCALC_LCD_WIDTH, &w_ok);
        int h = parse_int_arg(argv[5], 1, PICOCALC_LCD_HEIGHT, &h_ok);
        uint16_t color = color_from_name(argv[6], &color_ok);
        if (!x_ok || !y_ok || !w_ok || !h_ok || !color_ok ||
            x + w > PICOCALC_LCD_WIDTH || y + h > PICOCALC_LCD_HEIGHT) {
            std::printf("usage: lcd rect <x> <y> <w> <h> <color>, rectangle must fit inside 320x320\n");
            print_color_names();
            return 1;
        }
        const int64_t start = esp_timer_get_time();
        esp_err_t err = picocalc_lcd_fill_rect(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                                               static_cast<uint16_t>(w), static_cast<uint16_t>(h), color);
        const int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        std::printf("lcd rect x=%d y=%d w=%d h=%d color=%s/0x%04x err=%s elapsed_ms=%lld\n",
                    x, y, w, h, argv[6], color, esp_err_to_name(err), (long long)elapsed_ms);
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "swatches") == 0 || std::strcmp(argv[1], "swatch") == 0) {
        const int64_t start = esp_timer_get_time();
        esp_err_t err = draw_color_swatches();
        const int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        std::printf("lcd swatches: %s elapsed_ms=%lld\n", esp_err_to_name(err), (long long)elapsed_ms);
        return err == ESP_OK ? 0 : 1;
    }
    std::printf("usage: lcd init | lcd fill <color> | lcd rect <x> <y> <w> <h> <color> | lcd swatches\n");
    print_color_names();
    return 1;
}

int cmd_screen(int argc, char **argv)
{
    if (argc < 2 || std::strcmp(argv[1], "demo") == 0) {
        const int64_t start = esp_timer_get_time();
        esp_err_t err = visual_repl_demo_screen();
        const int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        visual_repl_status_t vst = {};
        visual_repl_get_status(&vst);
        std::printf("screen demo: %s elapsed_ms=%lld render_ms=%u grid=%ux%u\n",
                    esp_err_to_name(err), (long long)elapsed_ms, (unsigned)vst.last_render_ms,
                    (unsigned)vst.cols, (unsigned)vst.rows);
        return err == ESP_OK ? 0 : 1;
    }
    std::printf("usage: screen demo\n");
    return 1;
}

int cmd_kbd(int argc, char **argv)
{
    if (argc >= 2 && std::strcmp(argv[1], "recover") == 0) {
        esp_err_t err = picocalc_keyboard_recover();
        picocalc_keyboard_diag_t diag = {};
        picocalc_keyboard_get_diag(&diag);
        std::printf("kbd recover: %s initialized=%d errors=%u recoveries=%u last_error=%s\n",
                    esp_err_to_name(err), diag.initialized,
                    (unsigned)diag.error_count, (unsigned)diag.recover_count,
                    esp_err_to_name(diag.last_error));
        return err == ESP_OK ? 0 : 1;
    }
    if (argc >= 2 && std::strcmp(argv[1], "status") == 0) {
        picocalc_keyboard_diag_t diag = {};
        picocalc_keyboard_get_diag(&diag);
        std::printf("kbd status: initialized=%d last_status=0x%02x errors=%u recoveries=%u last_error=%s\n",
                    diag.initialized, diag.last_status, (unsigned)diag.error_count,
                    (unsigned)diag.recover_count, esp_err_to_name(diag.last_error));
        return 0;
    }
    if (argc >= 2 && std::strcmp(argv[1], "probe") == 0) {
        uint8_t addr = PICOCALC_KBD_I2C_ADDR;
        if (argc >= 3) {
            bool ok = false;
            int parsed = parse_int_arg(argv[2], 0x03, 0x77, &ok);
            if (!ok) {
                std::printf("usage: kbd probe [addr], addr may be decimal or 0xNN\n");
                return 1;
            }
            addr = static_cast<uint8_t>(parsed);
        }
        esp_err_t err = picocalc_keyboard_probe_address(addr, 100);
        std::printf("kbd probe addr=0x%02x: %s\n", addr, esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (argc >= 2 && std::strcmp(argv[1], "scan") == 0) {
        int found = 0;
        std::printf("kbd scan:");
        for (uint8_t addr = 0x03; addr <= 0x77; ++addr) {
            esp_err_t err = picocalc_keyboard_probe_address(addr, 20);
            if (err == ESP_OK) {
                std::printf(" 0x%02x", addr);
                ++found;
            }
        }
        std::printf(" (%d found)\n", found);
        return found > 0 ? 0 : 1;
    }
    int limit = 10;
    if (argc >= 2) {
        limit = std::atoi(argv[1]);
        if (limit <= 0) limit = 1;
        if (limit > 100) limit = 100;
    }
    for (int i = 0; i < limit; ++i) {
        picocalc_key_event_t ev = {};
        esp_err_t err = picocalc_keyboard_poll_event(&ev);
        if (err == ESP_ERR_NOT_FOUND) {
            std::printf("kbd: no event\n");
            return 0;
        }
        if (err != ESP_OK) {
            std::printf("kbd: err=%s\n", esp_err_to_name(err));
            return 1;
        }
        std::printf("kbd: state=%u(%s) key=0x%02x(%s) valid=%d\n",
                    ev.state, picocalc_keyboard_state_name(ev.state),
                    ev.key, picocalc_keyboard_key_name(ev.key), ev.valid);
    }
    return 0;
}

int cmd_js(int argc, char **argv)
{
    if (!g_qjs) {
        std::printf("QuickJS service unavailable\n");
        return 1;
    }
    if (argc < 2) {
        std::printf("usage: js eval <source> | js status | js reset\n");
        return 0;
    }
    if (std::strcmp(argv[1], "status") == 0) {
        return cmd_status(argc, argv);
    }
    if (std::strcmp(argv[1], "reset") == 0) {
        esp_err_t err = qjs_service_reset(g_qjs, 2000);
        std::printf("js reset: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "eval") == 0) {
        if (argc < 3) {
            std::printf("usage: js eval <source>\n");
            return 1;
        }
        char src[kMaxEvalSource];
        if (!join_source(argc, argv, 2, src, sizeof(src))) return 1;
        qjs_eval_result_t r = {};
        esp_err_t err = qjs_service_eval(g_qjs, src, std::strlen(src), kEvalTimeoutMs, "<0102-uart>", &r);
        if (err != ESP_OK) {
            std::printf("service error: %s\n", esp_err_to_name(err));
            return 1;
        }
        std::printf("[eval] ok=%d timed_out=%d elapsed=%ums\n", r.ok, r.timed_out, (unsigned)r.elapsed_ms);
        if (r.output && r.output[0]) std::printf("%s", r.output);
        if (r.error && r.error[0]) std::printf("error: %s\n", r.error);
        const bool ok = r.ok && !r.timed_out;
        qjs_eval_result_free(&r);
        return ok ? 0 : 1;
    }
    std::printf("usage: js eval <source> | js status | js reset\n");
    return 1;
}

void start_debug_console()
{
    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "0102> ";
    repl_cfg.task_stack_size = 8192;
    esp_console_register_help_command();

    const esp_console_cmd_t status_cmd = {
        .command = "status",
        .help = "Show 0102 LCD/keyboard/QuickJS status",
        .func = cmd_status,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&status_cmd));

    const esp_console_cmd_t lcd_cmd = {
        .command = "lcd",
        .help = "LCD diagnostics: init | fill <color> | rect <x> <y> <w> <h> <color> | swatches",
        .func = cmd_lcd,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&lcd_cmd));

    const esp_console_cmd_t screen_cmd = {
        .command = "screen",
        .help = "Visual REPL screen diagnostics: demo",
        .func = cmd_screen,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&screen_cmd));

    const esp_console_cmd_t kbd_cmd = {
        .command = "kbd",
        .help = "PicoCalc keyboard: kbd [limit] | status | recover | probe [addr] | scan",
        .func = cmd_kbd,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&kbd_cmd));

    const esp_console_cmd_t js_cmd = {
        .command = "js",
        .help = "QuickJS debug: eval <source> | status | reset",
        .func = cmd_js,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&js_cmd));

    esp_console_dev_uart_config_t hw_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_cfg, &repl_cfg, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "0102 ESP32-P4 visual QuickJS REPL skeleton (ticket ESP32-P4-VISUAL-QUICKJS-REPL)");

    esp_err_t lcd_err = picocalc_lcd_init();
    ESP_LOGI(kTag, "lcd init: %s actual_khz=%d", esp_err_to_name(lcd_err), picocalc_lcd_actual_khz());
    if (lcd_err == ESP_OK) {
        ESP_ERROR_CHECK(picocalc_lcd_fill(PICOCALC_LCD_RGB565_BLACK));
        ESP_ERROR_CHECK(visual_repl_init());
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_SYSTEM, "ESP32-P4 VISUAL QUICKJS REPL");
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "TYPE JAVASCRIPT; ENTER EVALUATES");
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "COMMANDS: /HELP /STATUS /RESET");
        esp_err_t visual_err = visual_repl_render();
        ESP_LOGI(kTag, "visual initial render: %s", esp_err_to_name(visual_err));
    }

    esp_log_level_set("i2c.master", ESP_LOG_NONE);
    esp_err_t kbd_err = picocalc_keyboard_init();
    ESP_LOGI(kTag, "keyboard init: %s", esp_err_to_name(kbd_err));
    if (kbd_err == ESP_OK) {
        BaseType_t task_ok = xTaskCreate(keyboard_task, "kbd0102", 4096, nullptr, 5, &g_keyboard_task);
        ESP_LOGI(kTag, "keyboard editor task create: %s", task_ok == pdPASS ? "ok" : "failed");
    }

    g_qjs = start_quickjs_service();
    ESP_LOGI(kTag, "quickjs service: %s", g_qjs ? "ready" : "unavailable");

    start_debug_console();
}
