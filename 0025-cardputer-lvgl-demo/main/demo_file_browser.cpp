#include "demo_manager.h"

#include <algorithm>
#include <string>
#include <vector>

#include "lvgl_font_util.h"
#include "sdcard_fatfs.h"

namespace {

constexpr int kVisibleRows = 4;

struct FileEntry {
    std::string name;
    bool is_dir = false;
};

struct FileBrowserState {
    DemoManager *mgr = nullptr;
    lv_obj_t *root = nullptr;

    lv_obj_t *path_label = nullptr;
    lv_obj_t *rows[kVisibleRows] = {nullptr};
    lv_obj_t *labels[kVisibleRows] = {nullptr};

    lv_obj_t *footer = nullptr;
    lv_obj_t *footer_label = nullptr;

    std::string cwd;
    std::vector<FileEntry> entries;
    int selected = 0;
    int scroll = 0;
};

static FileBrowserState *s_fb = nullptr;

static bool is_root_dir(const std::string &p) {
    return p == sdcard_mount_path();
}

static std::string parent_dir(const std::string &p) {
    if (p.empty() || is_root_dir(p)) return sdcard_mount_path();
    std::string out = p;
    while (out.size() > 1 && out.back() == '/') out.pop_back();
    const size_t pos = out.find_last_of('/');
    if (pos == std::string::npos || pos == 0) return sdcard_mount_path();
    out.resize(pos);
    return out;
}

static void apply_row_style(FileBrowserState *st, int idx, bool selected) {
    if (!st || idx < 0 || idx >= kVisibleRows) return;
    lv_obj_t *row = st->rows[idx];
    lv_obj_t *label = st->labels[idx];
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

static void render(FileBrowserState *st) {
    if (!st) return;

    const int total = (int)st->entries.size();
    if (total <= 0) {
        st->selected = 0;
        st->scroll = 0;
    } else {
        st->selected = std::clamp(st->selected, 0, total - 1);
        st->scroll = std::clamp(st->scroll, 0, std::max(0, total - kVisibleRows));
        if (st->selected < st->scroll) st->scroll = st->selected;
        if (st->selected >= st->scroll + kVisibleRows) st->scroll = st->selected - (kVisibleRows - 1);
    }

    if (st->path_label) {
        lv_label_set_text(st->path_label, st->cwd.c_str());
    }

    for (int i = 0; i < kVisibleRows; i++) {
        lv_obj_t *row = st->rows[i];
        lv_obj_t *label = st->labels[i];
        if (!row || !label) continue;

        const int idx = st->scroll + i;
        if (idx < 0 || idx >= total) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
        const FileEntry &e = st->entries[(size_t)idx];
        std::string text = e.name;
        if (e.is_dir && text != ".." && !text.empty() && text.back() != '/') text.push_back('/');
        lv_label_set_text(label, text.c_str());
        apply_row_style(st, i, idx == st->selected);
    }
}

static void refresh_dir(FileBrowserState *st) {
    if (!st) return;

    st->entries.clear();

    esp_err_t err = sdcard_mount();
    if (err != ESP_OK) {
        st->cwd = sdcard_mount_path();
        if (st->path_label) lv_label_set_text(st->path_label, "SD mount failed");
        if (st->footer_label) lv_label_set_text(st->footer_label, "Insert SD and reopen");
        render(st);
        return;
    }

    if (st->cwd.empty()) {
        st->cwd = sdcard_mount_path();
    }

    std::vector<SdDirEntry> listed;
    err = sdcard_list_dir(st->cwd.c_str(), &listed);
    if (err != ESP_OK) {
        if (st->path_label) lv_label_set_text(st->path_label, "List failed");
        if (st->footer_label) lv_label_set_text(st->footer_label, "Bksp parent  Fn+` menu");
        render(st);
        return;
    }

    if (!is_root_dir(st->cwd)) {
        FileEntry up;
        up.name = "..";
        up.is_dir = true;
        st->entries.push_back(std::move(up));
    }

    for (const auto &e : listed) {
        FileEntry fe;
        fe.name = e.name;
        fe.is_dir = e.is_dir;
        st->entries.push_back(std::move(fe));
    }

    if (st->footer_label) lv_label_set_text(st->footer_label, "Up/Down select  Enter open  Bksp parent");
    render(st);
}

static void open_selected(FileBrowserState *st) {
    if (!st || !st->mgr) return;
    if (st->entries.empty()) return;

    const FileEntry &e = st->entries[(size_t)st->selected];
    if (e.is_dir) {
        if (e.name == "..") {
            st->cwd = parent_dir(st->cwd);
        } else {
            if (!st->cwd.empty() && st->cwd.back() != '/') st->cwd.push_back('/');
            st->cwd += e.name;
        }
        st->selected = 0;
        st->scroll = 0;
        refresh_dir(st);
        return;
    }

    std::string path = st->cwd;
    if (!path.empty() && path.back() != '/') path.push_back('/');
    path += e.name;

    st->mgr->file_browser_cwd = st->cwd;
    st->mgr->file_viewer_path = path;
    demo_manager_load(st->mgr, DemoId::FileViewer);
}

static void key_cb(lv_event_t *e) {
    auto *st = static_cast<FileBrowserState *>(lv_event_get_user_data(e));
    if (!st) return;
    const uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_UP) {
        if (!st->entries.empty()) {
            st->selected = std::max(0, st->selected - 1);
            render(st);
        }
        return;
    }
    if (key == LV_KEY_DOWN) {
        if (!st->entries.empty()) {
            st->selected = std::min((int)st->entries.size() - 1, st->selected + 1);
            render(st);
        }
        return;
    }
    if (key == LV_KEY_ENTER) {
        open_selected(st);
        return;
    }
    if (key == LV_KEY_BACKSPACE) {
        if (!is_root_dir(st->cwd)) {
            st->cwd = parent_dir(st->cwd);
            st->selected = 0;
            st->scroll = 0;
            refresh_dir(st);
        }
        return;
    }
}

static void root_delete_cb(lv_event_t *e) {
    auto *st = static_cast<FileBrowserState *>(lv_event_get_user_data(e));
    if (!st) return;
    if (s_fb == st) s_fb = nullptr;
    delete st;
}

} // namespace

lv_obj_t *demo_file_browser_create(DemoManager *mgr) {
    auto *st = new FileBrowserState{};
    st->mgr = mgr;
    s_fb = st;

    st->root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(st->root, lv_color_black(), 0);
    lv_obj_set_style_text_color(st->root, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t *title = lv_label_create(st->root);
    lv_label_set_text(title, "Files");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    st->path_label = lv_label_create(st->root);
    lv_obj_set_style_text_font(st->path_label, lvgl_font_small(), 0);
    lv_label_set_long_mode(st->path_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(st->path_label, 240 - 12);
    lv_obj_align(st->path_label, LV_ALIGN_TOP_LEFT, 6, 20);
    lv_label_set_text(st->path_label, sdcard_mount_path());

    for (int i = 0; i < kVisibleRows; i++) {
        lv_obj_t *row = lv_obj_create(st->root);
        st->rows[i] = row;
        lv_obj_set_size(row, 240 - 16, 18);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 40 + i * 20);

        lv_obj_t *label = lv_label_create(row);
        st->labels[i] = label;
        lv_obj_set_style_text_font(label, lvgl_font_body(), 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_set_width(label, 240 - 32);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 8, 0);
    }

    st->footer = lv_obj_create(st->root);
    lv_obj_remove_style_all(st->footer);
    lv_obj_set_size(st->footer, 240, 16);
    lv_obj_set_style_bg_color(st->footer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(st->footer, LV_OPA_COVER, 0);
    lv_obj_align(st->footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(st->footer, LV_OBJ_FLAG_SCROLLABLE);

    st->footer_label = lv_label_create(st->footer);
    lv_obj_set_style_text_font(st->footer_label, lvgl_font_small(), 0);
    lv_label_set_text(st->footer_label, "Up/Down select  Enter open  Bksp parent");
    lv_obj_align(st->footer_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(st->root, key_cb, LV_EVENT_KEY, st);
    lv_obj_add_event_cb(st->root, root_delete_cb, LV_EVENT_DELETE, st);
    lv_obj_add_flag(st->root, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    if (mgr && !mgr->file_browser_cwd.empty()) {
        st->cwd = mgr->file_browser_cwd;
    }
    refresh_dir(st);

    return st->root;
}

void demo_file_browser_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_fb || !s_fb->root) return;
    lv_group_add_obj(mgr->group, s_fb->root);
    lv_group_focus_obj(s_fb->root);
}

