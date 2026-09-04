// SPDX-License-Identifier: MIT
//
// NFC.LAB view — reader page with persistent multi-tag UID list.
// Detail fields show the first/last tag; a compact UID list stays visible
// below as long as tags are present.

#include "nfc_debug_view.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace gogolem::nfc;

namespace nfc_debug::view {

static void read_event_cb(lv_event_t* event) {
    auto* view = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (view) view->request_read();
}

static void auto_event_cb(lv_event_t* event) {
    auto* view = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (view) view->toggle_auto();
}

NfcDebugView::NfcDebugView(Service& service, std::function<void()> close_callback,
                           std::function<void(bool)> auto_poll_callback)
    : _service(service), _close_callback(std::move(close_callback)),
      _auto_poll_callback(std::move(auto_poll_callback)) {
    create_frame();
}

NfcDebugView::~NfcDebugView() {
    if (_root) lv_obj_del(_root);
}

static void make_label(lv_obj_t* parent, lv_obj_t** target, const char* text) {
    *target = lv_label_create(parent);
    lv_label_set_text(*target, text);
    lv_obj_set_style_text_color(*target, lv_color_hex(0xC9D1D9), 0);
    lv_obj_set_style_text_font(*target, &lv_font_montserrat_14, 0);
    lv_obj_set_width(*target, LV_PCT(100));
    lv_label_set_long_mode(*target, LV_LABEL_LONG_WRAP);
}

void NfcDebugView::create_frame() {
    _root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 4, 0);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);

    // Header bar
    _header = lv_obj_create(_root);
    lv_obj_set_size(_header, LV_PCT(100), 28);
    lv_obj_align(_header, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(_header, lv_color_hex(0x6D4AFF), 0);
    lv_obj_set_style_radius(_header, 4, 0);
    lv_obj_set_style_pad_all(_header, 2, 0);
    lv_obj_clear_flag(_header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(_header);
    lv_label_set_text(title, "NFC.LAB");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 4, 0);

    _health_dot = lv_obj_create(_header);
    lv_obj_set_size(_health_dot, 8, 8);
    lv_obj_set_style_bg_color(_health_dot, lv_color_hex(0x00FF64), 0);
    lv_obj_set_style_radius(_health_dot, 4, 0);
    lv_obj_set_style_border_width(_health_dot, 0, 0);
    lv_obj_set_style_pad_all(_health_dot, 0, 0);
    lv_obj_align(_health_dot, LV_ALIGN_RIGHT_MID, -4, 0);

    _error_count = lv_label_create(_header);
    lv_label_set_text(_error_count, "");
    lv_obj_set_style_text_color(_error_count, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(_error_count, &lv_font_montserrat_14, 0);
    lv_obj_align(_error_count, LV_ALIGN_RIGHT_MID, -16, 0);

    // Main content: single-tag details (scrollable)
    _content = lv_obj_create(_root);
    lv_obj_set_size(_content, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(_content, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_set_style_bg_color(_content, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_radius(_content, 4, 0);
    lv_obj_set_style_pad_all(_content, 6, 0);
    lv_obj_set_style_pad_gap(_content, 2, 0);
    lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    make_label(_content, &_uid_label, "UID: ---");
    make_label(_content, &_type_label, "Type: ---");
    make_label(_content, &_atqa_sak_label, "ATQA: ----  SAK: --");
    make_label(_content, &_size_label, "User: 0  Total: 0");
    make_label(_content, &_ndef_label, "NDEF: ---");
    make_label(_content, &_raw_label, "Raw: ---");

    // Tag list panel: fixed position below content, above buttons.
    // Shows "N tags:" + compact UIDs. Stays visible while tags are present.
    _tag_list_panel = lv_obj_create(_root);
    lv_obj_set_size(_tag_list_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(_tag_list_panel, lv_color_hex(0x1C2333), 0);
    lv_obj_set_style_radius(_tag_list_panel, 4, 0);
    lv_obj_set_style_pad_all(_tag_list_panel, 4, 0);
    lv_obj_set_style_border_width(_tag_list_panel, 1, 0);
    lv_obj_set_style_border_color(_tag_list_panel, lv_color_hex(0x30363D), 0);
    lv_obj_clear_flag(_tag_list_panel, LV_OBJ_FLAG_SCROLLABLE);
    // Position will be set dynamically in update() to sit below content.

    _tag_list_label = lv_label_create(_tag_list_panel);
    lv_label_set_text(_tag_list_label, "");
    lv_obj_set_style_text_color(_tag_list_label, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(_tag_list_label, &lv_font_montserrat_14, 0);
    lv_obj_set_width(_tag_list_label, LV_PCT(100));
    lv_label_set_long_mode(_tag_list_label, LV_LABEL_LONG_WRAP);

    // Buttons at bottom
    _read_button = lv_btn_create(_root);
    lv_obj_set_size(_read_button, 76, 28);
    lv_obj_align(_read_button, LV_ALIGN_BOTTOM_LEFT, 4, -4);
    lv_obj_set_style_bg_color(_read_button, lv_color_hex(0x238636), 0);
    lv_obj_set_style_radius(_read_button, 4, 0);
    lv_obj_t* rbl = lv_label_create(_read_button);
    lv_label_set_text(rbl, "Read");
    lv_obj_set_style_text_font(rbl, &lv_font_montserrat_14, 0);
    lv_obj_center(rbl);
    lv_obj_add_event_cb(_read_button, read_event_cb, LV_EVENT_CLICKED, this);

    _auto_button = lv_btn_create(_root);
    lv_obj_set_size(_auto_button, 76, 28);
    lv_obj_align(_auto_button, LV_ALIGN_BOTTOM_LEFT, 84, -4);
    lv_obj_set_style_bg_color(_auto_button, lv_color_hex(0x1F6FEB), 0);
    lv_obj_set_style_radius(_auto_button, 4, 0);
    lv_obj_t* abl = lv_label_create(_auto_button);
    lv_label_set_text(abl, "Auto");
    lv_obj_set_style_text_font(abl, &lv_font_montserrat_14, 0);
    lv_obj_center(abl);
    lv_obj_add_event_cb(_auto_button, auto_event_cb, LV_EVENT_CLICKED, this);
}

void NfcDebugView::show_start_error(const Error& error) {
    if (_uid_label) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Error: %s", error_layer_name(error.layer));
        lv_label_set_text(_uid_label, buf);
    }
}

static void format_uid(char* buf, size_t buflen, const TagInfo& tag) {
    snprintf(buf, buflen, "UID: ");
    for (uint8_t i = 0; i < tag.uid_length; ++i) {
        char hex[4];
        snprintf(hex, sizeof(hex), "%02X", tag.uid[i]);
        strcat(buf, hex);
        if (i + 1 < tag.uid_length) strcat(buf, ":");
    }
}

void NfcDebugView::update(const ServiceSnapshot& snap) {
    _snapshot = snap;

    lv_obj_set_style_bg_color(_health_dot,
        snap.failures > 0 ? lv_color_hex(0xFF4444) : lv_color_hex(0x00FF64), 0);

    if (snap.failures > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(snap.failures));
        lv_label_set_text(_error_count, buf);
    } else {
        lv_label_set_text(_error_count, "");
    }

    // --- Tag list panel (persistent) ---
    // Show the compact UID list whenever tags were detected by the last Scan.
    // This persists across ActivateOne/RawRead/ReadNdef because only Scan
    // updates tag_count. Only hide when there are truly no tags (count=0 and
    // no single-tag activation succeeded either).
    if (snap.tag_count > 0) {
        char buf[256];
        if (snap.tag_count == 1) {
            snprintf(buf, sizeof(buf), "1 tag:");
        } else {
            snprintf(buf, sizeof(buf), "%u tags:", snap.tag_count);
        }
        for (uint8_t i = 0; i < snap.tag_count && i < 4; ++i) {
            strcat(buf, "\n");
            for (uint8_t j = 0; j < snap.tags[i].uid_length; ++j) {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X", snap.tags[i].uid[j]);
                strcat(buf, hex);
                if (j + 1 < snap.tags[i].uid_length) strcat(buf, ":");
            }
            strcat(buf, " ");
            strcat(buf, tag_family_name(snap.tags[i].family));
        }
        lv_label_set_text(_tag_list_label, buf);
        lv_obj_clear_flag(_tag_list_panel, LV_OBJ_FLAG_HIDDEN);
    } else if (!snap.tag_present) {
        // No tags at all: hide the panel.
        lv_obj_add_flag(_tag_list_panel, LV_OBJ_FLAG_HIDDEN);
    }
    // If tag_count == 0 but tag_present (ActivateOne found one without a
    // preceding Scan), keep the panel's current state — don't clear it.

    // --- Detail fields (first/last tag) ---
    if (snap.tag_present) {
        const auto& tag = snap.last_tag;
        char buf[64];
        format_uid(buf, sizeof(buf), tag);
        lv_label_set_text(_uid_label, buf);

        snprintf(buf, sizeof(buf), "Type: %s", tag_family_name(tag.family));
        lv_label_set_text(_type_label, buf);

        snprintf(buf, sizeof(buf), "ATQA: %04X  SAK: %02X", tag.atqa, tag.sak);
        lv_label_set_text(_atqa_sak_label, buf);

        snprintf(buf, sizeof(buf), "User: %lu  Total: %lu  Pages: %u",
                 static_cast<unsigned long>(tag.user_bytes),
                 static_cast<unsigned long>(tag.total_bytes),
                 tag.block_or_page_count);
        lv_label_set_text(_size_label, buf);
    } else {
        lv_label_set_text(_uid_label, "UID: ---");
        lv_label_set_text(_type_label, "Type: ---");
        lv_label_set_text(_atqa_sak_label, "ATQA: ----  SAK: --");
        lv_label_set_text(_size_label, "User: 0  Total: 0");
    }

    // NDEF
    if (snap.ndef_ok) {
        char buf[32];
        snprintf(buf, sizeof(buf), "NDEF: %lu records",
                 static_cast<unsigned long>(snap.ndef_records));
        lv_label_set_text(_ndef_label, buf);
    } else {
        lv_label_set_text(_ndef_label, "NDEF: ---");
    }

    // Raw hex
    if (snap.raw_read_ok) {
        char buf[80] = "Raw: ";
        for (size_t i = 0; i < 16 && i < snap.raw_read_data.size(); ++i) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X", snap.raw_read_data[i]);
            strcat(buf, hex);
        }
        lv_label_set_text(_raw_label, buf);
    } else {
        lv_label_set_text(_raw_label, "Raw: ---");
    }

    // Position the tag list panel below the content area.
    lv_obj_align(_tag_list_panel, LV_ALIGN_BOTTOM_LEFT, 4, -36);
}

void NfcDebugView::request_read() {
    Command cmd{};
    cmd.kind = ServiceCommand::Scan;
    _service.submit(cmd);
    cmd.kind = ServiceCommand::ActivateOne;
    _service.submit(cmd);
    cmd.kind = ServiceCommand::RawRead;
    cmd.address = 0;
    _service.submit(cmd);
    cmd.kind = ServiceCommand::ReadNdef;
    _service.submit(cmd);
}

void NfcDebugView::toggle_auto() {
    _auto_poll = !_auto_poll;
    lv_obj_t* label = lv_obj_get_child(_auto_button, 0);
    if (label) lv_label_set_text(label, _auto_poll ? "Stop" : "Auto");
    if (_auto_poll_callback) _auto_poll_callback(_auto_poll);
}

}  // namespace nfc_debug::view
