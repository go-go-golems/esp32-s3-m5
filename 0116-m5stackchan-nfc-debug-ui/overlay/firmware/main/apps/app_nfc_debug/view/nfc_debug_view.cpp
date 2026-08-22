// SPDX-License-Identifier: MIT
//
// NFC.LAB view — reader page matching the Arduino sketch output.
// Compact layout: UID, type, ATQA/SAK, sizes, NDEF, raw hex.

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

    // Content: compact data lines, larger font
    _content = lv_obj_create(_root);
    lv_obj_set_size(_content, LV_PCT(100), LV_PCT(100) - 28 - 36);
    lv_obj_align(_content, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_set_style_bg_color(_content, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_radius(_content, 4, 0);
    lv_obj_set_style_pad_all(_content, 6, 0);
    lv_obj_set_style_pad_gap(_content, 2, 0);
    lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(_content, LV_SCROLLBAR_MODE_AUTO);

    auto make_label = [this](lv_obj_t** target, const char* text) {
        *target = lv_label_create(_content);
        lv_label_set_text(*target, text);
        lv_obj_set_style_text_color(*target, lv_color_hex(0xC9D1D9), 0);
        lv_obj_set_style_text_font(*target, &lv_font_montserrat_14, 0);
        lv_obj_set_width(*target, LV_PCT(100));
        lv_label_set_long_mode(*target, LV_LABEL_LONG_WRAP);
    };

    make_label(&_uid_label, "UID: ---");
    make_label(&_type_label, "Type: ---");
    make_label(&_atqa_sak_label, "ATQA: ---- SAK: --");
    make_label(&_size_label, "User: 0  Total: 0");
    make_label(&_ndef_label, "NDEF: ---");
    make_label(&_raw_label, "Raw: ---");

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

    if (snap.tag_present) {
        // UID
        char buf[64] = "UID: ";
        for (uint8_t i = 0; i < snap.last_tag.uid_length; ++i) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X", snap.last_tag.uid[i]);
            strcat(buf, hex);
            if (i + 1 < snap.last_tag.uid_length) strcat(buf, ":");
        }
        lv_label_set_text(_uid_label, buf);

        // Type
        snprintf(buf, sizeof(buf), "Type: %s", tag_family_name(snap.last_tag.family));
        lv_label_set_text(_type_label, buf);

        // ATQA / SAK
        snprintf(buf, sizeof(buf), "ATQA: %04X  SAK: %02X",
                 snap.last_tag.atqa, snap.last_tag.sak);
        lv_label_set_text(_atqa_sak_label, buf);

        // Sizes
        snprintf(buf, sizeof(buf), "User: %lu  Total: %lu  Pages: %u",
                 static_cast<unsigned long>(snap.last_tag.user_bytes),
                 static_cast<unsigned long>(snap.last_tag.total_bytes),
                 snap.last_tag.block_or_page_count);
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
}

void NfcDebugView::request_read() {
    Command cmd{};
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
