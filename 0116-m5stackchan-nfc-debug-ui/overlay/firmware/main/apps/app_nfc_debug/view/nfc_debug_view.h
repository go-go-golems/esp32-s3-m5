/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "../nfc_debug_service.h"
#include <functional>
#include <lvgl.h>

namespace nfc_debug::view {

enum class Page : uint8_t {
    Reader = 0,
    RfIrq,
    Bus,
    RegistersLog,
};

class NfcDebugView {
public:
    NfcDebugView(Service& service, std::function<void()> close_callback);
    ~NfcDebugView();

    NfcDebugView(const NfcDebugView&) = delete;
    NfcDebugView& operator=(const NfcDebugView&) = delete;

    void update(const Snapshot& snapshot);
    void show_start_error(esp_err_t error);
    Page page() const { return _page; }

private:
    static void close_event(lv_event_t* event);
    static void read_event(lv_event_t* event);
    static void auto_event(lv_event_t* event);
    static void navigation_event(lv_event_t* event);

    void create_frame();
    void create_reader_page();
    void render_header(const Snapshot& snapshot);
    void render_reader(const Snapshot& snapshot);
    void request_read();
    void toggle_auto();
    void select_page(Page page);

    Service& _service;
    std::function<void()> _close_callback;
    Snapshot _snapshot{};
    Page _page = Page::Reader;

    lv_obj_t* _root = nullptr;
    lv_obj_t* _header = nullptr;
    lv_obj_t* _content = nullptr;
    lv_obj_t* _navigation = nullptr;
    lv_obj_t* _health_dot = nullptr;
    lv_obj_t* _error_count = nullptr;

    lv_obj_t* _reader_state = nullptr;
    lv_obj_t* _reader_primary = nullptr;
    lv_obj_t* _reader_secondary = nullptr;
    lv_obj_t* _reader_meta = nullptr;
    lv_obj_t* _reader_stages = nullptr;
    lv_obj_t* _read_button = nullptr;
    lv_obj_t* _read_button_label = nullptr;
    lv_obj_t* _auto_button = nullptr;
    lv_obj_t* _auto_button_label = nullptr;
};

}  // namespace nfc_debug::view
