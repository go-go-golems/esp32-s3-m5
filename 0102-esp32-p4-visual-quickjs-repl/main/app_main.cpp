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
#include "picojs_runtime.h"
#include "picoos_core.h"
#include "qjs_service.h"
#include "visual_repl.h"

namespace {
constexpr const char *kTag = "0102";
constexpr size_t kQuickJsMemoryLimit = 2 * 1024 * 1024;
constexpr size_t kQuickJsStackLimit = 64 * 1024;
constexpr uint32_t kEvalTimeoutMs = 1000;
constexpr size_t kMaxEvalSource = 2048;
constexpr TickType_t kKeyboardPollDelay = pdMS_TO_TICKS(20);
constexpr const char *kPicoJsHelloSource = R"JS(
var app = OS.app('hello');
var p = app.panel('main').frame('rounded').title(' hello ');
p.text('HELLO DEVICE').at('center', 2).bold().fg('cyan');
app.statusbar('native picojs minimal');
app.mount();
'picojs hello loaded';
)JS";
constexpr const char *kPicoJsDashboardSource = R"JS(
var app = OS.app('dashboard');
app.layout(function (l) { l.row(1, 'bar').row('*', 'body'); });
app.panel('bar').text('PicoJS Dashboard').at(0, 0).bold().fg('cyan');
var body = app.panel('body').frame('single').title(' system ');
body.text('ESP32-P4 native DSL').at('center', 1).bold().fg('cyan');
body.gauge().at(2, 3).label('batt').value('battery').width(20).showPct();
body.gauge().at(2, 5).label('heap').value(62).width(20).showPct();
app.statusbar('dashboard native picojs');
app.mount();
'picojs dashboard loaded';
)JS";
constexpr const char *kPicoJsInteractiveSource = R"JS(
var app = OS.app('interactive');
var ticks = 0;
var key = 'none';
app.layout(function (l) { l.row(1, 'bar').row('*', 'body'); });
app.panel('bar').text(function () { return 'Interactive ticks=' + ticks; }).at(0, 0).bold();
var body = app.panel('body').frame('single').title(' input ');
body.text(function () { return 'last key: ' + key; }).at('center', 2).bold();
body.text(function () { return 'ticks: ' + ticks; }).at('center', 4);
body.gauge().at(4, 6).label('tick').value(function () { return (ticks * 20) % 100; }).width(18).showPct();
app.on('tick', 1000, function () { ticks++; });
app.loop(2, function () { /* loop smoke */ });
app.compute(function () { /* compute smoke */ });
app.key('left', function () { key = 'left'; print('KEY left'); });
app.key('right', function () { key = 'right'; print('KEY right'); });
app.key('up', function () { key = 'up'; print('KEY up'); });
app.key('down', function () { key = 'down'; print('KEY down'); });
app.key('enter', function () { key = 'enter'; print('KEY enter'); });
app.key('a', function () { key = 'a'; print('KEY a'); });
app.statusbar(function () { return 'mode app ticks=' + ticks; });
app.mount();
'picojs interactive loaded';
)JS";
constexpr const char *kPicoJsHomeSource = R"JS(
var home = OS.app('home');
home.layout(function(l){ l.row(2,'bar').row('*','body'); });
home.panel('bar').text(function(){ return 'PicoOS  ' + OS.clock('HH:mm') + '  batt ' + OS.battery + '%'; }).at(0,0).bold();
home.panel('bar').gauge().at(0,1).label('batt').value(function(){ return OS.battery; }).width(20).style('blocks').showPct();
var body = home.panel('body');
body.text('Apps').at(0,0).bold();
var menu = body.menu().at(0,2).width(38).grid(2).items(['repl','hello','sysmon','snake','calc','settings','notes']).marker('>').accent('cyan').onPick(function(name){ OS.launch(name); });
home.key('left', function(){ menu.move(-1,0); });
home.key('right', function(){ menu.move(1,0); });
home.key('up', function(){ menu.move(0,-1); });
home.key('down', function(){ menu.move(0,1); });
home.key('enter', function(){ menu.pick(); });
home.key('escape', function(){ OS.launch('repl'); });
home.statusbar(function(){ return 'select ' + menu.value() + ' | enter open | brk repl'; });
home.mount();
'picojs home loaded';
)JS";
constexpr const char *kPicoJsSysmonSource = R"JS(
var mon = OS.app('sysmon');
var ui = mon.panel('main').frame('single').title(' sysmon ');
['cpu','mem','tmp'].forEach(function(k,i){ ui.gauge().at(2,1+i).label(k).value(function(){ return OS.metrics[k]; }).width(20).style('bar').showPct(); });
ui.spark().at(2,5).label('load').data(function(){ return OS.history('load',26); }).range(0,100).glyphs('._-=+#');
ui.table().at(2,7).columns(['pid','name','cpu','mem']).rows(function(){ return OS.processes(); }).sortBy('cpu','desc').select(0).marker('>');
mon.key('q', function(m){ m.exit(); });
mon.on('tick', 1000, function(m){ m.refresh(); });
mon.statusbar('q quit | up/down select | sysmon native subset');
mon.mount();
'picojs sysmon loaded';
)JS";
constexpr const char *kPicoJsSnakeSource = R"JS(
var game = OS.app('snake');
var st = game.state({score:0, status:'playing', x:4, y:3, fx:12, fy:3});
var board = game.panel('board').frame('single').title(' snake ').titleRight(function(){ return 'score '+st.score; });
board.grid().at(1,1).size(19,9).cell('. ').layer('head', function(){ return [{x:st.x,y:st.y}]; }, 'O').layer('food', function(){ return [{x:st.fx,y:st.fy}]; }, '*');
game.loop(4, function(){ st.x = (st.x + 1) % 19; if (st.x === st.fx && st.y === st.fy) st.score += 10; });
game.key('left', function(){ st.x = Math.max(0, st.x - 1); });
game.key('right', function(){ st.x = Math.min(18, st.x + 1); });
game.key('up', function(){ st.y = Math.max(0, st.y - 1); });
game.key('down', function(){ st.y = Math.min(8, st.y + 1); });
game.statusbar('arrows move | demo grid/layers');
game.mount();
'picojs snake loaded';
)JS";
constexpr const char *kPicoJsCalcSource = R"JS(
var calc = OS.app('calc');
var st = calc.state({expr:'', result:''});
var p = calc.panel('main').frame('single').title(' calc ');
p.text(function(){ return 'expr: ' + (st.expr || '0'); }).at(2,2).bold();
p.text(function(){ return ' = ' + st.result; }).at(2,4);
p.text('keys: 0-9 + - * / . enter =').at(2,7).dim();
function add(k){ if(st.expr.length < 24) st.expr += k; }
'0123456789+-*/.'.split('').forEach(function(k){ calc.key(k, function(){ add(k); }); });
calc.key('enter', function(){ try { st.result = String(OS.eval(st.expr)); } catch(e) { st.result = 'ERR'; } });
calc.key('backspace', function(){ st.expr = st.expr.slice(0,-1); });
calc.key('delete', function(){ st.expr=''; st.result=''; });
calc.statusbar('type expression | enter eval | del clear');
calc.mount();
'picojs calc loaded';
)JS";
constexpr const char *kPicoJsSettingsSource = R"JS(
var app = OS.app('settings');
var st = app.state({sel:0, bright:80, theme:'amber', echo:false});
var p = app.panel('main').frame('single').title(' settings ');
var rows = ['brightness','theme','echo','back'];
function line(i){ var mark = i === st.sel ? '>' : ' '; var value = ''; if(i===0)value=st.bright+'%'; if(i===1)value=st.theme; if(i===2)value=st.echo?'on':'off'; if(i===3)value='home'; return mark + ' ' + rows[i] + ' ' + value; }
[0,1,2,3].forEach(function(i){ p.text(function(){ return line(i); }).at(2,2+i); });
app.key('up', function(){ st.sel=Math.max(0,st.sel-1); });
app.key('down', function(){ st.sel=Math.min(rows.length-1,st.sel+1); });
app.key('left', function(){ if(st.sel===0)st.bright=Math.max(0,st.bright-10); if(st.sel===1)st.theme='amber'; });
app.key('right', function(){ if(st.sel===0)st.bright=Math.min(100,st.bright+10); if(st.sel===1)st.theme='cyan'; });
app.key('enter', function(){ if(st.sel===2)st.echo=!st.echo; if(st.sel===3)OS.launch('home'); });
app.statusbar('arrows adjust | enter toggle/open');
app.mount();
'picojs settings loaded';
)JS";
constexpr const char *kPicoJsNotesSource = R"JS(
var app = OS.app('notes');
var st = app.state({text:'type notes here'});
var p = app.panel('main').frame('single').title(' notes ');
p.text(function(){ return st.text.slice(0,34); }).at(2,2);
p.text(function(){ return st.text.slice(34,68); }).at(2,3);
'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!?'.split('').forEach(function(k){ app.key(k, function(){ if(st.text.length<68) st.text += k; }); });
app.key('backspace', function(){ st.text = st.text.slice(0,-1); });
app.key('delete', function(){ st.text = ''; });
app.statusbar('type text | backspace | del clear');
app.mount();
'picojs notes loaded';
)JS";

qjs_service_t *g_qjs = nullptr;
picojs_runtime_t *g_picojs = nullptr;
picoos_supervisor_t *g_picoos_os = nullptr;
TaskHandle_t g_keyboard_task = nullptr;
char g_input[VISUAL_REPL_INPUT_MAX + 1] = {};
size_t g_input_len = 0;
size_t g_input_cursor = 0;

esp_err_t render_picojs_to_lcd();
esp_err_t clear_picojs_on_js_task();
esp_err_t install_picojs_runtime();

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

bool evaluate_visual_input(const char *source)
{
    if (!source || source[0] == 0) {
        return true;
    }
    if (!g_qjs) {
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, "QUICKJS SERVICE UNAVAILABLE");
        return true;
    }

    if (source[0] == '/') {
        char cmdline[VISUAL_REPL_INPUT_MAX + 1] = {};
        std::snprintf(cmdline, sizeof(cmdline), "%s", source + 1);
        char *argv[8] = {};
        int argc = 0;
        for (char *tok = std::strtok(cmdline, " "); tok && argc < 8; tok = std::strtok(nullptr, " ")) {
            argv[argc++] = tok;
        }
        if (argc == 0) return true;
        if (std::strcmp(argv[0], "picoos") == 0 && argc > 1) {
            for (int i = 1; i < argc; ++i) argv[i - 1] = argv[i];
            --argc;
        }

        if (std::strcmp(argv[0], "reset") == 0) {
            esp_err_t clear_err = g_picojs ? clear_picojs_on_js_task() : ESP_OK;
            esp_err_t err = clear_err == ESP_OK ? qjs_service_reset(g_qjs, 2000) : clear_err;
            esp_err_t install_err = err == ESP_OK && g_picojs ? install_picojs_runtime() : err;
            char line[VISUAL_REPL_COLS + 1] = {};
            std::snprintf(line, sizeof(line), "RESET: %s PICOJS=%s", esp_err_to_name(err), esp_err_to_name(install_err));
            (void)visual_repl_append_line(err == ESP_OK && install_err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
            return true;
        }
        if (std::strcmp(argv[0], "status") == 0) {
            append_visual_status();
            if (g_picoos_os) {
                picoos_status_t st = {};
                esp_err_t err = picoos_get_status(g_picoos_os, &st);
                char line[VISUAL_REPL_COLS + 1] = {};
                std::snprintf(line, sizeof(line), "OS %s APP=%.12s R=%d", picoos_surface_name(st.surface), st.active_app_id[0] ? st.active_app_id : "-", st.running);
                (void)visual_repl_append_line(err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
            }
            return true;
        }
        if (std::strcmp(argv[0], "apps") == 0 || std::strcmp(argv[0], "ps") == 0) {
            if (!g_picoos_os) {
                (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, "PICOOS UNAVAILABLE");
                return true;
            }
            picoos_app_info_t apps[PICOOS_MAX_APPS] = {};
            size_t count = 0;
            esp_err_t err = picoos_list_apps(g_picoos_os, apps, PICOOS_MAX_APPS, &count);
            char line[VISUAL_REPL_COLS + 1] = {};
            if (std::strcmp(argv[0], "ps") == 0) {
                std::snprintf(line, sizeof(line), "PS: %s COUNT=%u", esp_err_to_name(err), (unsigned)count);
                (void)visual_repl_append_line(err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
                for (size_t i = 0; i < count && i < PICOOS_MAX_APPS; ++i) {
                    std::snprintf(line, sizeof(line), "%-11.11s %-10.10s f%lu e%lu", apps[i].id, picoos_app_state_name(apps[i].state),
                                  (unsigned long)apps[i].frame_count, (unsigned long)apps[i].error_count);
                    (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, line);
                }
            } else {
                std::snprintf(line, sizeof(line), "APPS: %s COUNT=%u", esp_err_to_name(err), (unsigned)count);
                (void)visual_repl_append_line(err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
                for (size_t i = 0; i < count && i < PICOOS_MAX_APPS; i += 2) {
                    const picoos_app_info_t &a = apps[i];
                    const picoos_app_info_t *b = (i + 1 < count && i + 1 < PICOOS_MAX_APPS) ? &apps[i + 1] : nullptr;
                    if (b) std::snprintf(line, sizeof(line), "%s%-11.11s %s%-11.11s", a.system ? "*" : " ", a.id, b->system ? "*" : " ", b->id);
                    else std::snprintf(line, sizeof(line), "%s%-11.11s", a.system ? "*" : " ", a.id);
                    (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, line);
                }
            }
            return true;
        }
        if (std::strcmp(argv[0], "help") == 0) {
            (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "/help /status /apps /ps /reset /kbd");
            (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "/launch ID /home /repl /start [fps]");
            (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "/stop /frame [ms] /key TOKEN");
            (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, "JS: print(1+2), throw Error()...");
            return true;
        }
        if (std::strcmp(argv[0], "launch") == 0 || std::strcmp(argv[0], "app") == 0) {
            if (!g_picoos_os || argc < 2) {
                (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, "USAGE: /launch <id>");
                return true;
            }
            esp_err_t err = picoos_launch(g_picoos_os, argv[1]);
            if (err != ESP_OK) {
                char line[VISUAL_REPL_COLS + 1] = {};
                std::snprintf(line, sizeof(line), "LAUNCH %s: %s", argv[1], esp_err_to_name(err));
                (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, line);
                return true;
            }
            if (std::strcmp(argv[1], "repl") == 0) return true;
            return render_picojs_to_lcd() != ESP_OK;
        }
        if (std::strcmp(argv[0], "home") == 0 || std::strcmp(argv[0], "launcher") == 0) {
            if (!g_picoos_os) return true;
            esp_err_t err = picoos_launch(g_picoos_os, "home");
            if (err != ESP_OK) {
                char line[VISUAL_REPL_COLS + 1] = {};
                std::snprintf(line, sizeof(line), "HOME: %s", esp_err_to_name(err));
                (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, line);
                return true;
            }
            return render_picojs_to_lcd() != ESP_OK;
        }
        if (std::strcmp(argv[0], "repl") == 0) {
            if (g_picoos_os) (void)picoos_show_repl(g_picoos_os);
            return true;
        }
        if (std::strcmp(argv[0], "start") == 0) {
            if (!g_picoos_os) return true;
            uint32_t fps = 0;
            if (argc >= 2) {
                bool ok = false;
                int parsed = parse_int_arg(argv[1], 1, 60, &ok);
                if (!ok) {
                    (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, "USAGE: /start [fps]");
                    return true;
                }
                fps = static_cast<uint32_t>(parsed);
            }
            esp_err_t err = picoos_start(g_picoos_os, fps);
            char line[VISUAL_REPL_COLS + 1] = {};
            std::snprintf(line, sizeof(line), "START: %s", esp_err_to_name(err));
            (void)visual_repl_append_line(err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
            return true;
        }
        if (std::strcmp(argv[0], "stop") == 0) {
            esp_err_t err = g_picoos_os ? picoos_stop(g_picoos_os) : ESP_ERR_INVALID_STATE;
            char line[VISUAL_REPL_COLS + 1] = {};
            std::snprintf(line, sizeof(line), "STOP: %s", esp_err_to_name(err));
            (void)visual_repl_append_line(err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
            return true;
        }
        if (std::strcmp(argv[0], "frame") == 0) {
            if (!g_picoos_os) return true;
            uint32_t dt = 100;
            if (argc >= 2) {
                bool ok = false;
                int parsed = parse_int_arg(argv[1], 0, 60000, &ok);
                if (!ok) return true;
                dt = static_cast<uint32_t>(parsed);
            }
            esp_err_t err = picoos_frame(g_picoos_os, dt);
            if (err != ESP_OK) {
                char line[VISUAL_REPL_COLS + 1] = {};
                std::snprintf(line, sizeof(line), "FRAME: %s", esp_err_to_name(err));
                (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, line);
                return true;
            }
            return false;
        }
        if (std::strcmp(argv[0], "key") == 0) {
            if (!g_picoos_os || argc < 2) {
                (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, "USAGE: /key <token>");
                return true;
            }
            esp_err_t err = picoos_key(g_picoos_os, argv[1]);
            if (err != ESP_OK) {
                char line[VISUAL_REPL_COLS + 1] = {};
                std::snprintf(line, sizeof(line), "KEY: %s", esp_err_to_name(err));
                (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, line);
                return true;
            }
            return false;
        }
        if (std::strcmp(argv[0], "kbd") == 0) {
            if (argc >= 2 && std::strcmp(argv[1], "recover") == 0) {
                esp_err_t err = picocalc_keyboard_recover();
                char line[VISUAL_REPL_COLS + 1] = {};
                std::snprintf(line, sizeof(line), "KBD RECOVER: %s", esp_err_to_name(err));
                (void)visual_repl_append_line(err == ESP_OK ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, line);
                return true;
            }
            picocalc_keyboard_diag_t diag = {};
            picocalc_keyboard_get_diag(&diag);
            char line[VISUAL_REPL_COLS + 1] = {};
            std::snprintf(line, sizeof(line), "KBD init=%d err=%u rec=%u", diag.initialized, (unsigned)diag.error_count, (unsigned)diag.recover_count);
            (void)visual_repl_append_line(VISUAL_REPL_STYLE_STATUS, line);
            return true;
        }

        char line[VISUAL_REPL_COLS + 1] = {};
        std::snprintf(line, sizeof(line), "UNKNOWN COMMAND: /%s", argv[0]);
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, line);
        return true;
    }

    qjs_eval_result_t r = {};
    esp_err_t err = qjs_service_eval(g_qjs, source, std::strlen(source), kEvalTimeoutMs, "<lcd-repl>", &r);
    if (err != ESP_OK) {
        char line[VISUAL_REPL_COLS + 1] = {};
        std::snprintf(line, sizeof(line), "SERVICE ERROR: %s", esp_err_to_name(err));
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, line);
        return true;
    }

    char meta[VISUAL_REPL_COLS + 1] = {};
    std::snprintf(meta, sizeof(meta), "OK=%d TIMEOUT=%d %uMS", r.ok, r.timed_out, (unsigned)r.elapsed_ms);
    (void)visual_repl_append_line(r.ok && !r.timed_out ? VISUAL_REPL_STYLE_STATUS : VISUAL_REPL_STYLE_ERROR, meta);
    append_visual_text(VISUAL_REPL_STYLE_OUTPUT, r.output);
    append_visual_text(VISUAL_REPL_STYLE_ERROR, r.error);
    qjs_eval_result_free(&r);
    return true;
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
    const bool render_repl = evaluate_visual_input(source);
    if (render_repl) (void)visual_repl_render();
}

struct PicoFrameJob {
    picojs_runtime_t *rt = nullptr;
    uint32_t dt_ms = 0;
};

struct PicoKeyJob {
    picojs_runtime_t *rt = nullptr;
    char token[16] = {};
};

esp_err_t picojs_frame_job(JSContext *ctx, void *user)
{
    auto *job = static_cast<PicoFrameJob *>(user);
    return job ? picojs_runtime_frame_js(ctx, job->rt, job->dt_ms) : ESP_ERR_INVALID_ARG;
}

esp_err_t picojs_key_job(JSContext *ctx, void *user)
{
    auto *job = static_cast<PicoKeyJob *>(user);
    return job ? picojs_runtime_key_js(ctx, job->rt, job->token) : ESP_ERR_INVALID_ARG;
}

esp_err_t picojs_reset_job(JSContext *, void *user)
{
    return picojs_runtime_reset(static_cast<picojs_runtime_t *>(user));
}

esp_err_t run_picojs_frame(uint32_t dt_ms)
{
    if (!g_qjs || !g_picojs) return ESP_ERR_INVALID_STATE;
    PicoFrameJob frame = {g_picojs, dt_ms};
    qjs_job_t job = {};
    job.fn = picojs_frame_job;
    job.user = &frame;
    job.timeout_ms = kEvalTimeoutMs;
    return qjs_service_run(g_qjs, &job);
}

esp_err_t send_picojs_key_token(const char *token)
{
    if (!g_qjs || !g_picojs || !token || token[0] == 0) return ESP_ERR_INVALID_STATE;
    PicoKeyJob key = {};
    key.rt = g_picojs;
    std::snprintf(key.token, sizeof(key.token), "%s", token);
    qjs_job_t job = {};
    job.fn = picojs_key_job;
    job.user = &key;
    job.timeout_ms = kEvalTimeoutMs;
    return qjs_service_run(g_qjs, &job);
}

esp_err_t clear_picojs_on_js_task()
{
    if (!g_qjs || !g_picojs) return ESP_ERR_INVALID_STATE;
    qjs_job_t job = {};
    job.fn = picojs_reset_job;
    job.user = g_picojs;
    job.timeout_ms = kEvalTimeoutMs;
    return qjs_service_run(g_qjs, &job);
}

esp_err_t render_picojs_to_lcd()
{
    if (!g_picojs) return ESP_ERR_INVALID_STATE;
    char dump[(5 + PICOJS_RUNTIME_DEFAULT_COLS + 1) * PICOJS_RUNTIME_DEFAULT_ROWS + 1] = {};
    esp_err_t dump_err = picojs_runtime_dump_text(g_picojs, dump, sizeof(dump));
    if (dump_err != ESP_OK) return dump_err;
    return visual_repl_render_dump_frame(dump);
}

esp_err_t render_picojs_to_lcd_callback(void *)
{
    return render_picojs_to_lcd();
}

esp_err_t render_repl_callback(void *)
{
    return visual_repl_render();
}

bool key_to_picojs_token(uint8_t key, char *dst, size_t dst_len)
{
    if (!dst || dst_len == 0) return false;
    const char *token = nullptr;
    switch (key) {
        case 0x08: token = "backspace"; break;
        case 0xb4: token = "left"; break;
        case 0xb5: token = "up"; break;
        case 0xb6: token = "down"; break;
        case 0xb7: token = "right"; break;
        case 0x0a:
        case 0x0d: token = "enter"; break;
        case 0xb1: token = "escape"; break;
        case 0xd0: token = "escape"; break;
        case 0xd2: token = "home"; break;
        case 0xd4: token = "delete"; break;
        case 0xd5: token = "end"; break;
        default: break;
    }
    if (!token && key >= 0x20 && key <= 0x7e) {
        dst[0] = static_cast<char>(key);
        dst[1] = 0;
        return true;
    }
    if (!token) return false;
    std::snprintf(dst, dst_len, "%s", token);
    return true;
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
        case 0xd0: // Break (Shift+Esc on PicoCalc)
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
            bool handled = false;
            if (ev.key == 0xd2 && g_picoos_os) { // Home (Shift+Tab on PicoCalc) always opens the PicoOS launcher, including from REPL edit mode.
                esp_err_t key_err = picoos_key(g_picoos_os, "home");
                handled = key_err == ESP_OK;
                if (key_err != ESP_OK) ESP_LOGW(kTag, "global home key failed: %s", esp_err_to_name(key_err));
                ESP_LOGI(kTag, "kbd global home key=0x%02x(%s) state=%s handled=%d",
                         ev.key, picocalc_keyboard_key_name(ev.key),
                         picocalc_keyboard_state_name(ev.state), handled);
                continue;
            }
            picoos_status_t ost = {};
            const bool picoos_active = g_picoos_os && picoos_get_status(g_picoos_os, &ost) == ESP_OK && ost.surface != PICOOS_SURFACE_REPL;
            if (picoos_active) {
                char token[16] = {};
                handled = key_to_picojs_token(ev.key, token, sizeof(token));
                if (handled) {
                    esp_err_t key_err = picoos_key(g_picoos_os, token);
                    if (key_err != ESP_OK) ESP_LOGW(kTag, "picoos key token=%s failed: %s", token, esp_err_to_name(key_err));
                }
                ESP_LOGI(kTag, "kbd picoos key=0x%02x(%s) token=%s state=%s handled=%d surface=%s active=%s",
                         ev.key, picocalc_keyboard_key_name(ev.key), token,
                         picocalc_keyboard_state_name(ev.state), handled,
                         picoos_surface_name(ost.surface), ost.active_app_id[0] ? ost.active_app_id : "-");
            } else if (picojs_runtime_app_mode(g_picojs)) {
                if (ev.key == 0xb1 || ev.key == 0xd0) { // Escape/Break returns to REPL edit mode.
                    (void)picojs_runtime_set_app_mode(g_picojs, false);
                    handled = true;
                } else {
                    char token[16] = {};
                    handled = key_to_picojs_token(ev.key, token, sizeof(token));
                    if (handled) {
                        esp_err_t key_err = send_picojs_key_token(token);
                        if (key_err == ESP_OK) {
                            esp_err_t render_err = render_picojs_to_lcd();
                            if (render_err != ESP_OK) ESP_LOGW(kTag, "picojs render after key failed: %s", esp_err_to_name(render_err));
                        } else {
                            ESP_LOGW(kTag, "picojs key token=%s failed: %s", token, esp_err_to_name(key_err));
                        }
                    }
                }
                ESP_LOGI(kTag, "kbd app key=0x%02x(%s) state=%s handled=%d app_mode=%d",
                         ev.key, picocalc_keyboard_key_name(ev.key),
                         picocalc_keyboard_state_name(ev.state), handled,
                         picojs_runtime_app_mode(g_picojs));
            } else {
                handled = handle_editor_key(ev.key);
                ESP_LOGI(kTag, "kbd editor key=0x%02x(%s) state=%s handled=%d input_len=%u cursor=%u",
                         ev.key, picocalc_keyboard_key_name(ev.key),
                         picocalc_keyboard_state_name(ev.state), handled,
                         (unsigned)g_input_len, (unsigned)g_input_cursor);
            }
        }
    }
}

esp_err_t install_picojs_job(JSContext *ctx, void *user)
{
    return picojs_runtime_install(ctx, static_cast<picojs_runtime_t *>(user));
}

esp_err_t install_picojs_runtime()
{
    if (!g_qjs || !g_picojs) return ESP_ERR_INVALID_STATE;
    qjs_job_t job = {};
    job.fn = install_picojs_job;
    job.user = g_picojs;
    job.timeout_ms = kEvalTimeoutMs;
    return qjs_service_run(g_qjs, &job);
}

esp_err_t register_picoos_builtin_apps()
{
    if (!g_picoos_os) return ESP_ERR_INVALID_STATE;
    const picoos_app_descriptor_t apps[] = {
        {.id = "home", .title = "PicoOS Home", .source = kPicoJsHomeSource, .filename = "<picoos-home>", .system = true, .autostart = true, .allow_background_ticks = false, .preferred_fps = 1},
        {.id = "repl", .title = "QuickJS REPL", .source = nullptr, .filename = "<native-repl>", .system = true, .autostart = false, .allow_background_ticks = false, .preferred_fps = 0},
        {.id = "hello", .title = "Hello", .source = kPicoJsHelloSource, .filename = "<picojs-hello>", .system = false, .autostart = false, .allow_background_ticks = false, .preferred_fps = 1},
        {.id = "dashboard", .title = "Dashboard", .source = kPicoJsDashboardSource, .filename = "<picojs-dashboard>", .system = false, .autostart = false, .allow_background_ticks = false, .preferred_fps = 1},
        {.id = "interactive", .title = "Interactive", .source = kPicoJsInteractiveSource, .filename = "<picojs-interactive>", .system = false, .autostart = false, .allow_background_ticks = false, .preferred_fps = 2},
        {.id = "sysmon", .title = "System Monitor", .source = kPicoJsSysmonSource, .filename = "<picojs-sysmon>", .system = false, .autostart = false, .allow_background_ticks = true, .preferred_fps = 1},
        {.id = "snake", .title = "Snake", .source = kPicoJsSnakeSource, .filename = "<picojs-snake>", .system = false, .autostart = false, .allow_background_ticks = false, .preferred_fps = 4},
        {.id = "calc", .title = "Calculator", .source = kPicoJsCalcSource, .filename = "<picojs-calc>", .system = false, .autostart = false, .allow_background_ticks = false, .preferred_fps = 1},
        {.id = "settings", .title = "Settings", .source = kPicoJsSettingsSource, .filename = "<picojs-settings>", .system = false, .autostart = false, .allow_background_ticks = false, .preferred_fps = 1},
        {.id = "notes", .title = "Notes", .source = kPicoJsNotesSource, .filename = "<picojs-notes>", .system = false, .autostart = false, .allow_background_ticks = false, .preferred_fps = 1},
    };
    for (const auto &app : apps) {
        esp_err_t err = picoos_register_app(g_picoos_os, &app);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t init_picoos_supervisor()
{
    if (!g_qjs || !g_picojs) return ESP_ERR_INVALID_STATE;
    picoos_supervisor_config_t cfg = {};
    cfg.qjs = g_qjs;
    cfg.runtime = g_picojs;
    cfg.cols = VISUAL_REPL_COLS;
    cfg.rows = VISUAL_REPL_ROWS;
    cfg.default_fps = 4;
    cfg.render_active = render_picojs_to_lcd_callback;
    cfg.render_repl = render_repl_callback;
    cfg.render_user = nullptr;
    esp_err_t err = picoos_supervisor_create(&cfg, &g_picoos_os);
    if (err != ESP_OK) return err;
    return register_picoos_builtin_apps();
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
    if (std::strcmp(argv[1], "eval") == 0 || std::strcmp(argv[1], "cmd") == 0) {
        if (argc < 3) {
            std::printf("usage: screen eval <visual-repl-source>\n");
            return 1;
        }
        char source[VISUAL_REPL_INPUT_MAX + 1] = {};
        if (!join_source(argc, argv, 2, source, sizeof(source))) return 1;
        char submitted[VISUAL_REPL_INPUT_MAX + 3] = {};
        std::snprintf(submitted, sizeof(submitted), "> %s", source);
        (void)visual_repl_append_line(VISUAL_REPL_STYLE_PROMPT, submitted);
        const bool render_repl = evaluate_visual_input(source);
        if (render_repl) (void)visual_repl_render();
        std::printf("screen eval: ESP_OK render_repl=%d\n", render_repl);
        return 0;
    }
    if (std::strcmp(argv[1], "dump") == 0) {
        char dump[(5 + VISUAL_REPL_COLS + 1) * VISUAL_REPL_ROWS + 1] = {};
        esp_err_t err = visual_repl_dump_text(dump, sizeof(dump));
        if (err != ESP_OK) {
            std::printf("screen dump: %s\n", esp_err_to_name(err));
            return 1;
        }
        std::printf("screen dump: %s rows=%u cols=%u\n", esp_err_to_name(err), VISUAL_REPL_ROWS, VISUAL_REPL_COLS);
        std::printf("%s", dump);
        return 0;
    }
    std::printf("usage: screen demo | dump | eval <visual-repl-source>\n");
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
    if (argc >= 2 && std::strcmp(argv[1], "version") == 0) {
        uint8_t ver[2] = {};
        uint8_t off[2] = {};
        esp_err_t ver_err = picocalc_keyboard_read_register(0x01, ver, sizeof(ver));
        esp_err_t off_err = picocalc_keyboard_read_register(0x0e, off, sizeof(off));
        const uint8_t bios = ver_err == ESP_OK ? ver[1] : 0;
        const char *label = "unknown";
        if (bios == 0x12 || (bios == 0 && off_err == ESP_OK && off[1] == 0)) label = "BIOS 1.2 or earlier";
        else if (bios == 0x14 || (bios == 0 && off_err == ESP_OK && off[1] == 1)) label = "BIOS 1.4-compatible";
        else if (bios == 0x16) label = "BIOS 1.6";
        std::printf("kbd version: ver_err=%s reg01=[0x%02x 0x%02x] off_err=%s reg0e=[0x%02x 0x%02x] detected=0x%02x %s\n",
                    esp_err_to_name(ver_err), ver[0], ver[1], esp_err_to_name(off_err), off[0], off[1], bios, label);
        return ver_err == ESP_OK ? 0 : 1;
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

bool js_smoke_eval(const char *name, const char *source, bool expect_ok, const char *output_substr, const char *error_substr)
{
    qjs_eval_result_t r = {};
    esp_err_t err = qjs_service_eval(g_qjs, source, std::strlen(source), kEvalTimeoutMs, "<0102-smoke>", &r);
    const bool output_ok = !output_substr || (r.output && std::strstr(r.output, output_substr));
    const bool error_ok = !error_substr || (r.error && std::strstr(r.error, error_substr));
    const bool pass = err == ESP_OK && r.ok == expect_ok && !r.timed_out && output_ok && error_ok;
    std::printf("js smoke %-10s: %s err=%s ok=%d timeout=%d elapsed=%ums\n",
                name, pass ? "PASS" : "FAIL", esp_err_to_name(err), r.ok, r.timed_out, (unsigned)r.elapsed_ms);
    if (!pass) {
        if (r.output && r.output[0]) std::printf("  output: %s", r.output);
        if (r.error && r.error[0]) std::printf("  error: %s\n", r.error);
    }
    qjs_eval_result_free(&r);
    return pass;
}

int cmd_js_smoke()
{
    bool ok = true;
    ok = js_smoke_eval("print", "print('picojs-smoke-print'); 1 + 2", true, "picojs-smoke-print", nullptr) && ok;
    ok = js_smoke_eval("globals", "gc(); print(millis() >= 0); 'picojs-smoke-done'", true, "picojs-smoke-done", nullptr) && ok;
    ok = js_smoke_eval("throw", "throw new Error('picojs-smoke-boom')", false, nullptr, "picojs-smoke-boom") && ok;
    std::printf("js smoke: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}

int cmd_picoos(int argc, char **argv)
{
    if (!g_picoos_os) {
        std::printf("picoos unavailable\n");
        return 1;
    }
    if (argc < 2 || std::strcmp(argv[1], "status") == 0) {
        picoos_status_t st = {};
        esp_err_t err = picoos_get_status(g_picoos_os, &st);
        if (err != ESP_OK) {
            std::printf("picoos status: %s\n", esp_err_to_name(err));
            return 1;
        }
        std::printf("picoos: initialized=%d running=%d surface=%s active=%s cols=%u rows=%u default_fps=%u apps=%u frames=%u errors=%u\n",
                    st.initialized, st.running, picoos_surface_name(st.surface),
                    st.active_app_id[0] ? st.active_app_id : "-", st.cols, st.rows,
                    (unsigned)st.default_fps, (unsigned)st.app_count,
                    (unsigned)st.frame_count, (unsigned)st.error_count);
        return 0;
    }
    if (std::strcmp(argv[1], "launch") == 0) {
        if (argc < 3) {
            std::printf("usage: picoos launch <id>\n");
            return 1;
        }
        esp_err_t err = picoos_launch(g_picoos_os, argv[2]);
        std::printf("picoos launch %s: %s\n", argv[2], esp_err_to_name(err));
        if (err == ESP_OK && std::strcmp(argv[2], "repl") != 0) {
            esp_err_t render_err = render_picojs_to_lcd();
            std::printf("picoos render after launch: %s\n", esp_err_to_name(render_err));
            if (render_err != ESP_OK) return 1;
        }
        if (err == ESP_OK && std::strcmp(argv[2], "repl") == 0) {
            (void)visual_repl_render();
        }
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "launcher") == 0) {
        esp_err_t err = picoos_launch(g_picoos_os, "home");
        std::printf("picoos launcher: %s\n", esp_err_to_name(err));
        if (err == ESP_OK) {
            esp_err_t render_err = render_picojs_to_lcd();
            std::printf("picoos render after launcher: %s\n", esp_err_to_name(render_err));
            if (render_err != ESP_OK) return 1;
        }
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "repl") == 0) {
        esp_err_t err = picoos_show_repl(g_picoos_os);
        std::printf("picoos repl: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "key") == 0) {
        if (argc < 3) {
            std::printf("usage: picoos key <token>\n");
            return 1;
        }
        esp_err_t err = picoos_key(g_picoos_os, argv[2]);
        std::printf("picoos key: %s token=%s\n", esp_err_to_name(err), argv[2]);
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "start") == 0) {
        uint32_t fps = 0;
        if (argc >= 3) {
            bool ok = false;
            int parsed = parse_int_arg(argv[2], 1, 60, &ok);
            if (!ok) {
                std::printf("usage: picoos start [fps:1..60]\n");
                return 1;
            }
            fps = static_cast<uint32_t>(parsed);
        }
        esp_err_t err = picoos_start(g_picoos_os, fps);
        std::printf("picoos start: %s fps=%u\n", esp_err_to_name(err), (unsigned)(fps ? fps : 4));
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "stop") == 0) {
        esp_err_t err = picoos_stop(g_picoos_os);
        std::printf("picoos stop: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "frame") == 0) {
        uint32_t dt = 100;
        if (argc >= 3) {
            bool ok = false;
            int parsed = parse_int_arg(argv[2], 0, 60000, &ok);
            if (!ok) {
                std::printf("usage: picoos frame [dt_ms]\n");
                return 1;
            }
            dt = static_cast<uint32_t>(parsed);
        }
        esp_err_t err = picoos_frame(g_picoos_os, dt);
        std::printf("picoos frame: %s dt_ms=%u\n", esp_err_to_name(err), (unsigned)dt);
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "apps") == 0 || std::strcmp(argv[1], "ps") == 0) {
        picoos_app_info_t apps[PICOOS_MAX_APPS] = {};
        size_t count = 0;
        esp_err_t err = picoos_list_apps(g_picoos_os, apps, PICOOS_MAX_APPS, &count);
        const bool ps = std::strcmp(argv[1], "ps") == 0;
        std::printf("picoos %s: %s count=%u\n", ps ? "ps" : "apps", esp_err_to_name(err), (unsigned)count);
        const size_t shown = count < PICOOS_MAX_APPS ? count : PICOOS_MAX_APPS;
        for (size_t i = 0; i < shown; ++i) {
            if (ps) {
                std::printf("%-11s state=%-10s frames=%u errors=%u fps=%u\n",
                            apps[i].id, picoos_app_state_name(apps[i].state),
                            (unsigned)apps[i].frame_count, (unsigned)apps[i].error_count,
                            (unsigned)apps[i].preferred_fps);
            } else {
                std::printf("[%u] id=%s title=%s state=%s system=%d autostart=%d bg_ticks=%d fps=%u frames=%u errors=%u\n",
                            (unsigned)i, apps[i].id, apps[i].title, picoos_app_state_name(apps[i].state),
                            apps[i].system, apps[i].autostart, apps[i].allow_background_ticks,
                            (unsigned)apps[i].preferred_fps, (unsigned)apps[i].frame_count,
                            (unsigned)apps[i].error_count);
            }
        }
        return err == ESP_OK ? 0 : 1;
    }
    std::printf("usage: picoos status | apps | ps | launch <id> | launcher | repl | key <token> | start [fps] | stop | frame [dt_ms]\n");
    return 1;
}

int cmd_picojs(int argc, char **argv)
{
    if (!g_picojs) {
        std::printf("picojs unavailable\n");
        return 1;
    }
    if (argc < 2 || std::strcmp(argv[1], "status") == 0) {
        picojs_runtime_status_t st = {};
        esp_err_t err = picojs_runtime_get_status(g_picojs, &st);
        if (err != ESP_OK) {
            std::printf("picojs status: %s\n", esp_err_to_name(err));
            return 1;
        }
        std::printf("picojs: initialized=%d js_installed=%d app_mode=%d cols=%u rows=%u apps=%u mounted=%u frames=%u last_frame_ms=%u errors=%u\n",
                    st.initialized, st.js_installed, st.app_mode, st.cols, st.rows, (unsigned)st.app_count,
                    (unsigned)st.mounted_app_count, (unsigned)st.frame_count,
                    (unsigned)st.last_frame_ms, (unsigned)st.last_error_count);
        return 0;
    }
    if (std::strcmp(argv[1], "install") == 0) {
        esp_err_t err = install_picojs_runtime();
        std::printf("picojs install: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "load") == 0) {
        if (argc < 3 || (std::strcmp(argv[2], "hello") != 0 && std::strcmp(argv[2], "dashboard") != 0 &&
                         std::strcmp(argv[2], "interactive") != 0 && std::strcmp(argv[2], "home") != 0 &&
                         std::strcmp(argv[2], "sysmon") != 0 && std::strcmp(argv[2], "snake") != 0 &&
                         std::strcmp(argv[2], "calc") != 0 && std::strcmp(argv[2], "settings") != 0 &&
                         std::strcmp(argv[2], "notes") != 0)) {
            std::printf("usage: picojs load hello|dashboard|interactive|home|sysmon|snake|calc|settings|notes\n");
            return 1;
        }
        const char *source = kPicoJsHelloSource;
        const char *filename = "<picojs-hello>";
        if (std::strcmp(argv[2], "dashboard") == 0) {
            source = kPicoJsDashboardSource;
            filename = "<picojs-dashboard>";
        } else if (std::strcmp(argv[2], "interactive") == 0) {
            source = kPicoJsInteractiveSource;
            filename = "<picojs-interactive>";
        } else if (std::strcmp(argv[2], "home") == 0) {
            source = kPicoJsHomeSource;
            filename = "<picojs-home>";
        } else if (std::strcmp(argv[2], "sysmon") == 0) {
            source = kPicoJsSysmonSource;
            filename = "<picojs-sysmon>";
        } else if (std::strcmp(argv[2], "snake") == 0) {
            source = kPicoJsSnakeSource;
            filename = "<picojs-snake>";
        } else if (std::strcmp(argv[2], "calc") == 0) {
            source = kPicoJsCalcSource;
            filename = "<picojs-calc>";
        } else if (std::strcmp(argv[2], "settings") == 0) {
            source = kPicoJsSettingsSource;
            filename = "<picojs-settings>";
        } else if (std::strcmp(argv[2], "notes") == 0) {
            source = kPicoJsNotesSource;
            filename = "<picojs-notes>";
        }
        esp_err_t install_err = install_picojs_runtime();
        if (install_err != ESP_OK) {
            std::printf("picojs install before load: %s\n", esp_err_to_name(install_err));
            return 1;
        }
        qjs_eval_result_t r = {};
        esp_err_t err = qjs_service_eval(g_qjs, source, std::strlen(source), kEvalTimeoutMs, filename, &r);
        std::printf("picojs load %s: %s ok=%d timeout=%d elapsed=%ums\n",
                    argv[2], esp_err_to_name(err), r.ok, r.timed_out, (unsigned)r.elapsed_ms);
        if (r.output && r.output[0]) std::printf("%s", r.output);
        if (r.error && r.error[0]) std::printf("error: %s\n", r.error);
        const bool ok = err == ESP_OK && r.ok && !r.timed_out;
        qjs_eval_result_free(&r);
        if (ok) {
            esp_err_t render_err = render_picojs_to_lcd();
            std::printf("picojs render after load: %s\n", esp_err_to_name(render_err));
        }
        return ok ? 0 : 1;
    }
    if (std::strcmp(argv[1], "dump") == 0) {
        char dump[(5 + PICOJS_RUNTIME_DEFAULT_COLS + 1) * PICOJS_RUNTIME_DEFAULT_ROWS + 1] = {};
        esp_err_t err = picojs_runtime_dump_text(g_picojs, dump, sizeof(dump));
        if (err != ESP_OK) {
            std::printf("picojs dump: %s\n", esp_err_to_name(err));
            return 1;
        }
        std::printf("picojs dump: %s\n", esp_err_to_name(err));
        std::printf("%s", dump);
        return 0;
    }
    if (std::strcmp(argv[1], "frame") == 0) {
        uint32_t dt = 100;
        if (argc >= 3) {
            bool ok = false;
            int parsed = parse_int_arg(argv[2], 0, 60000, &ok);
            if (!ok) {
                std::printf("usage: picojs frame [dt_ms]\n");
                return 1;
            }
            dt = static_cast<uint32_t>(parsed);
        }
        esp_err_t err = run_picojs_frame(dt);
        std::printf("picojs frame: %s dt_ms=%u\n", esp_err_to_name(err), (unsigned)dt);
        if (err == ESP_OK) {
            esp_err_t render_err = render_picojs_to_lcd();
            std::printf("picojs render after frame: %s\n", esp_err_to_name(render_err));
        }
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "run") == 0) {
        if (argc < 4) {
            std::printf("usage: picojs run <count> <dt_ms>\n");
            return 1;
        }
        bool count_ok = false;
        bool dt_ok = false;
        int count = parse_int_arg(argv[2], 1, 100, &count_ok);
        int dt = parse_int_arg(argv[3], 0, 60000, &dt_ok);
        if (!count_ok || !dt_ok) {
            std::printf("usage: picojs run <count:1..100> <dt_ms>\n");
            return 1;
        }
        esp_err_t err = ESP_OK;
        for (int i = 0; i < count; ++i) {
            err = run_picojs_frame((uint32_t)dt);
            if (err != ESP_OK) break;
        }
        std::printf("picojs run: %s count=%d dt_ms=%d\n", esp_err_to_name(err), count, dt);
        if (err == ESP_OK) {
            esp_err_t render_err = render_picojs_to_lcd();
            std::printf("picojs render after run: %s\n", esp_err_to_name(render_err));
        }
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "key") == 0) {
        if (argc < 3) {
            std::printf("usage: picojs key <token>\n");
            return 1;
        }
        esp_err_t err = send_picojs_key_token(argv[2]);
        std::printf("picojs key: %s token=%s\n", esp_err_to_name(err), argv[2]);
        if (err == ESP_OK) {
            esp_err_t render_err = render_picojs_to_lcd();
            std::printf("picojs render after key: %s\n", esp_err_to_name(render_err));
        }
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "render") == 0) {
        esp_err_t err = render_picojs_to_lcd();
        std::printf("picojs render: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    if (std::strcmp(argv[1], "mode") == 0) {
        if (argc < 3 || (std::strcmp(argv[2], "app") != 0 && std::strcmp(argv[2], "repl") != 0)) {
            std::printf("usage: picojs mode app|repl\n");
            return 1;
        }
        const bool enabled = std::strcmp(argv[2], "app") == 0;
        esp_err_t err = picojs_runtime_set_app_mode(g_picojs, enabled);
        if (err == ESP_OK && !enabled) (void)visual_repl_render();
        std::printf("picojs mode: %s app_mode=%d\n", esp_err_to_name(err), enabled);
        return err == ESP_OK ? 0 : 1;
    }
    std::printf("usage: picojs status | install | load hello|dashboard|interactive|home|sysmon|snake | dump | render | frame [dt_ms] | run <count> <dt_ms> | key <token> | mode app|repl\n");
    return 1;
}

int cmd_js(int argc, char **argv)
{
    if (!g_qjs) {
        std::printf("QuickJS service unavailable\n");
        return 1;
    }
    if (argc < 2) {
        std::printf("usage: js eval <source> | js smoke | js status | js reset\n");
        return 0;
    }
    if (std::strcmp(argv[1], "status") == 0) {
        return cmd_status(argc, argv);
    }
    if (std::strcmp(argv[1], "smoke") == 0) {
        return cmd_js_smoke();
    }
    if (std::strcmp(argv[1], "reset") == 0) {
        esp_err_t clear_err = g_picojs ? clear_picojs_on_js_task() : ESP_OK;
        esp_err_t err = clear_err == ESP_OK ? qjs_service_reset(g_qjs, 2000) : clear_err;
        esp_err_t install_err = err == ESP_OK ? install_picojs_runtime() : err;
        std::printf("js reset: %s picojs_clear=%s picojs_reinstall=%s\n",
                    esp_err_to_name(err), esp_err_to_name(clear_err), esp_err_to_name(install_err));
        return (err == ESP_OK && clear_err == ESP_OK && install_err == ESP_OK) ? 0 : 1;
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
    std::printf("usage: js eval <source> | js smoke | js status | js reset\n");
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
        .help = "Visual REPL screen diagnostics: demo | dump | eval <source>",
        .func = cmd_screen,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&screen_cmd));

    const esp_console_cmd_t kbd_cmd = {
        .command = "kbd",
        .help = "PicoCalc keyboard: kbd [limit] | status | version | recover | probe [addr] | scan",
        .func = cmd_kbd,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&kbd_cmd));

    const esp_console_cmd_t js_cmd = {
        .command = "js",
        .help = "QuickJS debug: eval <source> | smoke | status | reset",
        .func = cmd_js,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&js_cmd));

    const esp_console_cmd_t picoos_cmd = {
        .command = "picoos",
        .help = "PicoOS supervisor: status | apps | ps | launch <id> | launcher | repl | key <token> | start [fps] | stop | frame [dt_ms]",
        .func = cmd_picoos,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&picoos_cmd));

    const esp_console_cmd_t picojs_cmd = {
        .command = "picojs",
        .help = "PicoJS runtime: status | install | load hello|dashboard|interactive|home|sysmon|snake | dump | render | frame [dt_ms] | run <count> <dt_ms> | key <token> | mode app|repl",
        .func = cmd_picojs,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&picojs_cmd));

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
        BaseType_t task_ok = xTaskCreate(keyboard_task, "kbd0102", 12288, nullptr, 5, &g_keyboard_task);
        ESP_LOGI(kTag, "keyboard editor task create: %s", task_ok == pdPASS ? "ok" : "failed");
    }

    g_qjs = start_quickjs_service();
    ESP_LOGI(kTag, "quickjs service: %s", g_qjs ? "ready" : "unavailable");

    picojs_runtime_config_t picojs_cfg = {};
    picojs_cfg.cols = VISUAL_REPL_COLS;
    picojs_cfg.rows = VISUAL_REPL_ROWS;
    picojs_cfg.frame_interval_ms = 100;
    esp_err_t picojs_err = picojs_runtime_create(&picojs_cfg, &g_picojs);
    ESP_LOGI(kTag, "picojs runtime init: %s", esp_err_to_name(picojs_err));
    if (picojs_err == ESP_OK) {
        esp_err_t install_err = install_picojs_runtime();
        ESP_LOGI(kTag, "picojs QuickJS install: %s", esp_err_to_name(install_err));
        esp_err_t picoos_err = init_picoos_supervisor();
        ESP_LOGI(kTag, "picoos supervisor init: %s", esp_err_to_name(picoos_err));
    }

    start_debug_console();
}
