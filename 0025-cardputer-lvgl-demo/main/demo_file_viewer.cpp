#include "demo_manager.h"

#include <string>

#include "lvgl_font_util.h"
#include "sdcard_fatfs.h"

namespace {

struct FileViewerState {
    DemoManager *mgr = nullptr;
    lv_obj_t *root = nullptr;
    lv_obj_t *title = nullptr;
    lv_obj_t *textarea = nullptr;
    lv_obj_t *footer = nullptr;
    lv_obj_t *footer_label = nullptr;
};

static FileViewerState *s_fv = nullptr;

static const char *basename_cstr(const std::string &path) {
    const size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return path.c_str();
    if (pos + 1 >= path.size()) return path.c_str();
    return path.c_str() + pos + 1;
}

static void key_cb(lv_event_t *e) {
    auto *st = static_cast<FileViewerState *>(lv_event_get_user_data(e));
    if (!st || !st->mgr) return;
    const uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_UP) {
        if (st->textarea) lv_obj_scroll_by(st->textarea, 0, 16, LV_ANIM_OFF);
        return;
    }
    if (key == LV_KEY_DOWN) {
        if (st->textarea) lv_obj_scroll_by(st->textarea, 0, -16, LV_ANIM_OFF);
        return;
    }
    if (key == LV_KEY_BACKSPACE) {
        demo_manager_load(st->mgr, DemoId::FileBrowser);
        return;
    }
}

static void root_delete_cb(lv_event_t *e) {
    auto *st = static_cast<FileViewerState *>(lv_event_get_user_data(e));
    if (!st) return;
    if (s_fv == st) s_fv = nullptr;
    delete st;
}

} // namespace

lv_obj_t *demo_file_viewer_create(DemoManager *mgr) {
    auto *st = new FileViewerState{};
    st->mgr = mgr;
    s_fv = st;

    st->root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(st->root, lv_color_black(), 0);
    lv_obj_set_style_text_color(st->root, lv_palette_main(LV_PALETTE_GREEN), 0);

    st->title = lv_label_create(st->root);
    lv_label_set_text(st->title, "View");
    lv_obj_align(st->title, LV_ALIGN_TOP_MID, 0, 4);

    st->textarea = lv_textarea_create(st->root);
    lv_textarea_set_one_line(st->textarea, false);
    lv_textarea_set_text_selection(st->textarea, false);
    lv_textarea_set_cursor_click_pos(st->textarea, false);
    lv_obj_set_style_text_font(st->textarea, lvgl_font_small(), 0);
    lv_obj_set_style_bg_opa(st->textarea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(st->textarea, lv_palette_darken(LV_PALETTE_GREEN, 3), 0);
    lv_obj_set_style_border_width(st->textarea, 1, 0);
    lv_obj_set_style_pad_all(st->textarea, 4, 0);
    lv_obj_set_size(st->textarea, 240 - 12, 92);
    lv_obj_align(st->textarea, LV_ALIGN_TOP_MID, 0, 20);

    st->footer = lv_obj_create(st->root);
    lv_obj_remove_style_all(st->footer);
    lv_obj_set_size(st->footer, 240, 16);
    lv_obj_set_style_bg_color(st->footer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(st->footer, LV_OPA_COVER, 0);
    lv_obj_align(st->footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(st->footer, LV_OBJ_FLAG_SCROLLABLE);

    st->footer_label = lv_label_create(st->footer);
    lv_obj_set_style_text_font(st->footer_label, lvgl_font_small(), 0);
    lv_label_set_text(st->footer_label, "Up/Down scroll  Bksp back");
    lv_obj_align(st->footer_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(st->root, key_cb, LV_EVENT_KEY, st);
    lv_obj_add_event_cb(st->root, root_delete_cb, LV_EVENT_DELETE, st);
    lv_obj_add_flag(st->root, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    std::string path;
    if (mgr) {
        path = mgr->file_viewer_path;
    }

    if (!path.empty()) {
        lv_label_set_text(st->title, basename_cstr(path));

        bool truncated = false;
        std::string content;
        const esp_err_t err = sdcard_read_file_preview(path.c_str(), 4096, &content, &truncated);
        if (err == ESP_OK) {
            if (truncated) {
                content.append("\n\n... (truncated)\n");
            }
            lv_textarea_set_text(st->textarea, content.c_str());
        } else {
            lv_textarea_set_text(st->textarea, "ERR: read failed");
        }
    } else {
        lv_textarea_set_text(st->textarea, "ERR: no file selected");
    }

    return st->root;
}

void demo_file_viewer_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_fv || !s_fv->root) return;
    lv_group_add_obj(mgr->group, s_fv->root);
    lv_group_focus_obj(s_fv->root);
}

