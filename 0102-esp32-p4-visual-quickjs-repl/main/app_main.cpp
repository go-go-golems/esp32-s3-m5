// 0102 — ESP32-P4 visual QuickJS REPL skeleton.
// Console = UART0 (CH343 USB-UART bridge; the P4 has no USB Serial/JTAG).
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

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

qjs_service_t *g_qjs = nullptr;

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
    std::printf("keyboard: initialized=%d last_status=0x%02x errors=%u\n",
                kdiag.initialized, kdiag.last_status, (unsigned)kdiag.error_count);
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
            std::printf("usage: lcd fill black|white|red|green|blue|yellow|cyan|magenta\n");
            return 1;
        }
        const int64_t start = esp_timer_get_time();
        esp_err_t err = picocalc_lcd_fill(color);
        const int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
        std::printf("lcd fill color=0x%04x err=%s elapsed_ms=%lld actual_khz=%d\n",
                    color, esp_err_to_name(err), (long long)elapsed_ms, picocalc_lcd_actual_khz());
        return err == ESP_OK ? 0 : 1;
    }
    std::printf("usage: lcd init | lcd fill <color>\n");
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
        .help = "LCD skeleton diagnostics: init | fill <color>",
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
        .help = "Poll PicoCalc keyboard events: kbd [limit]",
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
        esp_err_t visual_err = visual_repl_demo_screen();
        ESP_LOGI(kTag, "visual demo render: %s", esp_err_to_name(visual_err));
    }

    esp_err_t kbd_err = picocalc_keyboard_init();
    ESP_LOGI(kTag, "keyboard init: %s", esp_err_to_name(kbd_err));

    g_qjs = start_quickjs_service();
    ESP_LOGI(kTag, "quickjs service: %s", g_qjs ? "ready" : "unavailable");

    start_debug_console();
}
