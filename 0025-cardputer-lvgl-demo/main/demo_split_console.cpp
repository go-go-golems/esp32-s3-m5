#include "demo_manager.h"

#include <algorithm>
#include <inttypes.h>
#include <cctype>
#include <deque>
#include <stdio.h>
#include <string>
#include <string_view>
#include <vector>

#include "esp_heap_caps.h"
#include "esp_system.h"

#include "lvgl_font_util.h"

namespace {

struct SplitConsoleState {
    DemoManager *mgr = nullptr;
    lv_obj_t *root = nullptr;
    lv_obj_t *title = nullptr;
    lv_obj_t *out = nullptr;
    lv_obj_t *in = nullptr;
    bool follow_tail = true;
    std::deque<std::string> lines;
};

static SplitConsoleState *s_console = nullptr;

static std::string trim_copy(std::string_view in) {
    size_t start = 0;
    while (start < in.size() && std::isspace((unsigned char)in[start])) start++;
    size_t end = in.size();
    while (end > start && std::isspace((unsigned char)in[end - 1])) end--;
    return std::string(in.substr(start, end - start));
}

static std::vector<std::string_view> split_ws(std::string_view in) {
    std::vector<std::string_view> out;
    size_t i = 0;
    while (i < in.size()) {
        while (i < in.size() && std::isspace((unsigned char)in[i])) i++;
        if (i >= in.size()) break;
        const size_t start = i;
        while (i < in.size() && !std::isspace((unsigned char)in[i])) i++;
        out.emplace_back(in.substr(start, i - start));
    }
    return out;
}

static void set_out_text_from_lines(SplitConsoleState *st) {
    if (!st || !st->out) return;
    std::string joined;
    size_t total = 0;
    for (const auto &line : st->lines) total += line.size() + 1;
    joined.reserve(total);
    for (const auto &line : st->lines) {
        joined.append(line);
        joined.push_back('\n');
    }
    lv_textarea_set_text(st->out, joined.c_str());
}

static void append_line(SplitConsoleState *st, std::string_view line) {
    if (!st) return;
    if (line.empty()) return;

    st->lines.emplace_back(line);
    while ((int)st->lines.size() > 200) {
        st->lines.pop_front();
    }

    const bool follow = st->follow_tail;
    set_out_text_from_lines(st);
    if (!st->out) return;

    if (follow) {
        lv_textarea_set_cursor_pos(st->out, LV_TEXTAREA_CURSOR_LAST);
        lv_obj_scroll_to_y(st->out, lv_obj_get_scroll_y(st->out) + lv_obj_get_scroll_bottom(st->out), LV_ANIM_OFF);
    }
}

static void out_scroll_cb(lv_event_t *e) {
    auto *st = static_cast<SplitConsoleState *>(lv_event_get_user_data(e));
    if (!st || !st->out) return;
    st->follow_tail = lv_obj_get_scroll_bottom(st->out) <= 4;
}

static void cmd_help(SplitConsoleState *st) {
    append_line(st, "Commands:");
    append_line(st, "  help");
    append_line(st, "  heap");
    append_line(st, "  menu | basics | pomodoro | sysmon");
    append_line(st, "  setmins <1-99>");
    append_line(st, "  screenshot (host-only)");
}

static void cmd_heap(SplitConsoleState *st) {
    const uint32_t heap_free = esp_get_free_heap_size();
    const uint32_t dma_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA);
    char buf[96];
    snprintf(buf, sizeof(buf), "heap_free=%" PRIu32 " dma_free=%" PRIu32, heap_free, dma_free);
    append_line(st, buf);
}

static void run_line(SplitConsoleState *st, std::string_view line) {
    if (!st || !st->mgr) return;
    const std::string trimmed = trim_copy(line);
    if (trimmed.empty()) return;

    append_line(st, std::string("> ").append(trimmed));

    const auto toks = split_ws(trimmed);
    if (toks.empty()) return;

    const std::string_view cmd = toks[0];
    if (cmd == "help") {
        cmd_help(st);
        return;
    }
    if (cmd == "heap") {
        cmd_heap(st);
        return;
    }

    if (cmd == "menu") {
        demo_manager_load(st->mgr, DemoId::Menu);
        return;
    }
    if (cmd == "basics") {
        demo_manager_load(st->mgr, DemoId::Basics);
        return;
    }
    if (cmd == "pomodoro") {
        demo_manager_load(st->mgr, DemoId::Pomodoro);
        return;
    }
    if (cmd == "sysmon") {
        demo_manager_load(st->mgr, DemoId::SystemMonitor);
        return;
    }

    if (cmd == "setmins") {
        if (toks.size() < 2) {
            append_line(st, "ERR: usage: setmins <minutes>");
            return;
        }
        int minutes = 0;
        for (const char c : toks[1]) {
            if (c < '0' || c > '9') {
                append_line(st, "ERR: invalid minutes");
                return;
            }
            minutes = minutes * 10 + (c - '0');
            if (minutes > 999) break;
        }
        if (minutes < 1) minutes = 1;
        if (minutes > 99) minutes = 99;
        demo_manager_pomodoro_set_minutes(st->mgr, minutes);
        char buf[64];
        snprintf(buf, sizeof(buf), "OK minutes=%d", minutes);
        append_line(st, buf);
        return;
    }

    if (cmd == "screenshot") {
        append_line(st, "ERR: screenshot is host-only (use esp_console)");
        return;
    }

    append_line(st, std::string("ERR: unknown command: ").append(std::string(cmd)));
}

static void in_key_cb(lv_event_t *e) {
    auto *st = static_cast<SplitConsoleState *>(lv_event_get_user_data(e));
    const uint32_t key = lv_event_get_key(e);
    if (!st || !st->in) return;

    if (key != LV_KEY_ENTER) return;

    const char *txt = lv_textarea_get_text(st->in);
    run_line(st, txt ? txt : "");
    if (!st->in) return;
    lv_textarea_set_text(st->in, "");
    if (st->mgr && st->mgr->group) {
        lv_group_focus_obj(st->in);
    }
}

static void root_delete_cb(lv_event_t *e) {
    auto *st = static_cast<SplitConsoleState *>(lv_event_get_user_data(e));
    if (!st) return;

    st->root = nullptr;
    st->title = nullptr;
    st->out = nullptr;
    st->in = nullptr;

    if (s_console == st) s_console = nullptr;
    delete st;
}

} // namespace

lv_obj_t *demo_split_console_create(DemoManager *mgr) {
    auto *st = new SplitConsoleState{};
    st->mgr = mgr;
    s_console = st;

    st->root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(st->root, lv_color_black(), 0);
    lv_obj_set_style_text_color(st->root, lv_palette_main(LV_PALETTE_GREEN), 0);

    st->title = lv_label_create(st->root);
    lv_label_set_text(st->title, "Console");
    lv_obj_align(st->title, LV_ALIGN_TOP_MID, 0, 4);

    const lv_coord_t input_h = 26;
    const lv_coord_t margin = 6;
    const lv_coord_t y_top = 22;
    const lv_coord_t w = 240 - (margin * 2);
    const lv_coord_t out_h = 135 - y_top - input_h - margin;

    st->out = lv_textarea_create(st->root);
    lv_obj_set_size(st->out, w, out_h);
    lv_obj_align(st->out, LV_ALIGN_TOP_MID, 0, y_top);
    lv_textarea_set_one_line(st->out, false);
    lv_textarea_set_text_selection(st->out, false);
    lv_textarea_set_cursor_click_pos(st->out, false);
    lv_textarea_set_text(st->out, "");
    lv_obj_set_style_text_font(st->out, lvgl_font_small(), 0);
    lv_obj_set_style_bg_opa(st->out, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(st->out, 1, 0);
    lv_obj_set_style_border_color(st->out, lv_palette_darken(LV_PALETTE_GREEN, 3), 0);
    lv_obj_set_style_pad_all(st->out, 4, 0);
    lv_obj_set_scrollbar_mode(st->out, LV_SCROLLBAR_MODE_ACTIVE);

    lv_obj_add_event_cb(st->out, out_scroll_cb, LV_EVENT_SCROLL, st);
    lv_obj_add_flag(st->out, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    st->in = lv_textarea_create(st->root);
    lv_obj_set_size(st->in, w, input_h);
    lv_obj_align(st->in, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_textarea_set_one_line(st->in, true);
    lv_textarea_set_placeholder_text(st->in, "> ");
    lv_textarea_set_text_selection(st->in, false);
    lv_textarea_set_cursor_click_pos(st->in, false);
    lv_obj_set_style_text_font(st->in, lvgl_font_body(), 0);
    lv_obj_set_style_bg_opa(st->in, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(st->in, 1, 0);
    lv_obj_set_style_border_color(st->in, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_pad_all(st->in, 4, 0);
    lv_obj_add_flag(st->in, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(st->in, in_key_cb, LV_EVENT_KEY, st);

    lv_obj_t *hint = lv_label_create(st->root);
    lv_obj_set_style_text_font(hint, lvgl_font_small(), 0);
    lv_label_set_text(hint, "Enter submit  Tab focus  Fn+` menu");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_obj_add_event_cb(st->root, root_delete_cb, LV_EVENT_DELETE, st);

    append_line(st, "lvgl console ready; type 'help'");

    return st->root;
}

void demo_split_console_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_console || !s_console->out || !s_console->in) return;

    lv_group_add_obj(mgr->group, s_console->out);
    lv_group_add_obj(mgr->group, s_console->in);
    lv_group_focus_obj(s_console->in);
}

void split_console_ui_log_line(const char *line) {
    if (!s_console) return;
    append_line(s_console, line ? line : "");
}
