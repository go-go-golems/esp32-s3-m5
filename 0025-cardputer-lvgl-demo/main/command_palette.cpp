#include "command_palette.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "action_registry.h"
#include "lvgl_font_util.h"

namespace {

constexpr int kVisibleRows = 5;
constexpr lv_coord_t kRowH = 14;
constexpr lv_coord_t kRowY0 = 26;
constexpr const char *kHintText = "Enter run  Esc close";
constexpr lv_coord_t kFooterH = 18;

struct PaletteState {
    bool inited = false;
    bool open = false;

    QueueHandle_t ctrl_q = nullptr;
    lv_indev_t *kb_indev = nullptr;
    lv_group_t *app_group = nullptr;
    lv_group_t *palette_group = nullptr;
    lv_obj_t *prev_focused = nullptr;

    lv_obj_t *backdrop = nullptr;
    lv_obj_t *panel = nullptr;
    lv_obj_t *query = nullptr;
    lv_obj_t *status = nullptr;
    lv_obj_t *rows[kVisibleRows] = {nullptr};
    lv_obj_t *labels[kVisibleRows] = {nullptr};

    std::vector<const Action *> filtered;
    int selected = 0;
};

static PaletteState s_pal;

static std::string lower_copy(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (const char c : s) {
        out.push_back((char)std::tolower((unsigned char)c));
    }
    return out;
}

static bool contains_ci(std::string_view hay, std::string_view needle) {
    if (needle.empty()) return true;
    const std::string hay_l = lower_copy(hay);
    const std::string needle_l = lower_copy(needle);
    return hay_l.find(needle_l) != std::string::npos;
}

static void set_status(const char *s) {
    if (!s_pal.status) return;
    if (!s || s[0] == '\0') {
        lv_label_set_text(s_pal.status, kHintText);
        return;
    }
    lv_label_set_text(s_pal.status, s);
}

static void apply_row_style(int idx, bool selected) {
    if (idx < 0 || idx >= kVisibleRows) return;
    lv_obj_t *row = s_pal.rows[idx];
    lv_obj_t *label = s_pal.labels[idx];
    if (!row || !label) return;

    if (selected) {
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(row, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_set_style_text_color(label, lv_color_black(), 0);
    } else {
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

static void render_results(void) {
    const int total = (int)s_pal.filtered.size();
    int start = 0;
    if (total > kVisibleRows) {
        start = std::clamp(s_pal.selected - (kVisibleRows - 1), 0, total - kVisibleRows);
    }

    for (int i = 0; i < kVisibleRows; i++) {
        lv_obj_t *row = s_pal.rows[i];
        lv_obj_t *label = s_pal.labels[i];
        if (!row || !label) continue;

        const int idx = start + i;
        if (idx >= total) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(label, s_pal.filtered[(size_t)idx]->title ? s_pal.filtered[(size_t)idx]->title : "");
        apply_row_style(i, idx == s_pal.selected);
    }
}

static void refilter(void) {
    s_pal.filtered.clear();

    const char *q = s_pal.query ? lv_textarea_get_text(s_pal.query) : "";
    const std::string query = q ? std::string(q) : std::string();

    size_t action_count = 0;
    const Action *actions = action_registry_actions(&action_count);
    for (size_t i = 0; i < action_count; i++) {
        const Action &a = actions[i];
        const char *title = a.title ? a.title : "";
        const char *keywords = a.keywords ? a.keywords : "";

        if (query.empty() || contains_ci(title, query) || contains_ci(keywords, query)) {
            s_pal.filtered.push_back(&a);
        }
    }

    if (s_pal.filtered.empty()) {
        s_pal.selected = 0;
    } else {
        s_pal.selected = std::clamp(s_pal.selected, 0, (int)s_pal.filtered.size() - 1);
    }
    set_status("");
    render_results();
}

static void run_selected(void) {
    if (s_pal.filtered.empty()) {
        set_status("No matches");
        return;
    }

    const Action *a = s_pal.filtered[(size_t)s_pal.selected];
    if (!a) return;

    if (!s_pal.ctrl_q) {
        set_status("ERR: no ctrl queue");
        return;
    }

    if (!action_registry_enqueue(s_pal.ctrl_q, a->id, 0)) {
        set_status("ERR: busy");
        return;
    }

    set_status("OK");
    command_palette_close();
}

static void query_key_cb(lv_event_t *e) {
    const uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ESC) {
        command_palette_close();
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_UP) {
        if (!s_pal.filtered.empty()) {
            s_pal.selected = (s_pal.selected + (int)s_pal.filtered.size() - 1) % (int)s_pal.filtered.size();
            render_results();
        }
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_DOWN) {
        if (!s_pal.filtered.empty()) {
            s_pal.selected = (s_pal.selected + 1) % (int)s_pal.filtered.size();
            render_results();
        }
        lv_event_stop_processing(e);
        return;
    }

    if (key == LV_KEY_ENTER) {
        run_selected();
        lv_event_stop_processing(e);
        return;
    }
}

static void query_changed_cb(lv_event_t *e) {
    (void)e;
    refilter();
}

static void destroy_overlay(void) {
    if (s_pal.backdrop) {
        lv_obj_del(s_pal.backdrop);
    }
    s_pal.backdrop = nullptr;
    s_pal.panel = nullptr;
    s_pal.query = nullptr;
    s_pal.status = nullptr;
    for (int i = 0; i < kVisibleRows; i++) {
        s_pal.rows[i] = nullptr;
        s_pal.labels[i] = nullptr;
    }
}

} // namespace

void command_palette_init(lv_indev_t *kb_indev, lv_group_t *app_group, QueueHandle_t ctrl_q) {
    s_pal.inited = true;
    s_pal.ctrl_q = ctrl_q;
    s_pal.kb_indev = kb_indev;
    s_pal.app_group = app_group;
    if (!s_pal.palette_group) {
        s_pal.palette_group = lv_group_create();
        lv_group_set_wrap(s_pal.palette_group, true);
    }
}

bool command_palette_is_open(void) {
    return s_pal.open;
}

void command_palette_close(void) {
    if (!s_pal.open) return;

    destroy_overlay();
    s_pal.open = false;

    if (s_pal.kb_indev && s_pal.app_group) {
        lv_indev_set_group(s_pal.kb_indev, s_pal.app_group);
        if (s_pal.prev_focused && lv_obj_is_valid(s_pal.prev_focused)) {
            lv_group_focus_obj(s_pal.prev_focused);
        }
    }

    s_pal.prev_focused = nullptr;
}

static void open_palette(void) {
    if (!s_pal.inited || s_pal.open) return;
    s_pal.open = true;
    s_pal.selected = 0;

    if (s_pal.app_group) {
        s_pal.prev_focused = lv_group_get_focused(s_pal.app_group);
    }

    lv_obj_t *top = lv_layer_top();
    s_pal.backdrop = lv_obj_create(top);
    lv_obj_remove_style_all(s_pal.backdrop);
    lv_obj_set_size(s_pal.backdrop, 240, 135);
    lv_obj_set_style_bg_color(s_pal.backdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_pal.backdrop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_pal.backdrop, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_width(s_pal.backdrop, 2, 0);
    lv_obj_set_style_pad_left(s_pal.backdrop, 8, 0);
    lv_obj_set_style_pad_right(s_pal.backdrop, 8, 0);
    lv_obj_set_style_pad_top(s_pal.backdrop, 8, 0);
    lv_obj_set_style_pad_bottom(s_pal.backdrop, 8, 0);
    lv_obj_clear_flag(s_pal.backdrop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *footer = lv_obj_create(s_pal.backdrop);
    lv_obj_remove_style_all(footer);
    lv_obj_set_width(footer, lv_pct(100));
    lv_obj_set_height(footer, kFooterH);
    lv_obj_set_style_bg_color(footer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    s_pal.panel = lv_obj_create(s_pal.backdrop);
    lv_obj_remove_style_all(s_pal.panel);
    lv_obj_set_width(s_pal.panel, lv_pct(100));
    lv_obj_set_height(s_pal.panel, 135 - kFooterH - 16);
    lv_obj_align(s_pal.panel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(s_pal.panel, LV_OBJ_FLAG_SCROLLABLE);

    s_pal.query = lv_textarea_create(s_pal.panel);
    lv_obj_set_width(s_pal.query, lv_pct(100));
    lv_textarea_set_one_line(s_pal.query, true);
    lv_textarea_set_placeholder_text(s_pal.query, "Search...");
    lv_textarea_set_text_selection(s_pal.query, false);
    lv_obj_set_style_text_font(s_pal.query, lvgl_font_body(), 0);
    lv_obj_set_style_bg_opa(s_pal.query, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(s_pal.query, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_width(s_pal.query, 1, 0);
    lv_obj_set_style_pad_all(s_pal.query, 4, 0);
    lv_obj_align(s_pal.query, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_add_event_cb(s_pal.query, query_key_cb, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(s_pal.query, query_changed_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Results area.
    for (int i = 0; i < kVisibleRows; i++) {
        lv_obj_t *row = lv_obj_create(s_pal.panel);
        s_pal.rows[i] = row;
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, kRowH);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 4, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, kRowY0 + i * kRowH);

        lv_obj_t *label = lv_label_create(row);
        s_pal.labels[i] = label;
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_style_text_font(label, lvgl_font_small(), 0);
    }

    s_pal.status = lv_label_create(footer);
    lv_obj_set_style_text_font(s_pal.status, lvgl_font_small(), 0);
    lv_obj_set_style_text_color(s_pal.status, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_text_align(s_pal.status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(s_pal.status, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_pal.status, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(s_pal.status, 2, 0);
    lv_obj_set_style_pad_right(s_pal.status, 2, 0);
    lv_label_set_text(s_pal.status, "");
    lv_obj_center(s_pal.status);
    set_status("");

    if (s_pal.kb_indev && s_pal.palette_group) {
        lv_group_remove_all_objs(s_pal.palette_group);
        lv_group_add_obj(s_pal.palette_group, s_pal.query);
        lv_indev_set_group(s_pal.kb_indev, s_pal.palette_group);
        lv_group_focus_obj(s_pal.query);
    }

    refilter();
}

void command_palette_toggle(void) {
    if (s_pal.open) {
        command_palette_close();
        return;
    }
    open_palette();
}
