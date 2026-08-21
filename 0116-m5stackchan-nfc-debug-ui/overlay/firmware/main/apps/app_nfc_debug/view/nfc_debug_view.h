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
    static void sample_event(lv_event_t* event);
    static void clear_event(lv_event_t* event);
    static void probe_event(lv_event_t* event);
    static void verify_event(lv_event_t* event);
    static void reinitialize_event(lv_event_t* event);
    static void navigation_event(lv_event_t* event);

    void create_frame();
    void create_reader_page();
    void create_rf_page();
    void create_bus_page();
    void render_header(const Snapshot& snapshot);
    void render_reader(const Snapshot& snapshot);
    void render_rf(const Snapshot& snapshot);
    void render_bus(const Snapshot& snapshot);
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

    lv_obj_t* _reader_page = nullptr;
    lv_obj_t* _rf_page = nullptr;
    lv_obj_t* _bus_page = nullptr;

    lv_obj_t* _reader_state = nullptr;
    lv_obj_t* _reader_primary = nullptr;
    lv_obj_t* _reader_secondary = nullptr;
    lv_obj_t* _reader_meta = nullptr;
    lv_obj_t* _reader_stages = nullptr;
    lv_obj_t* _read_button = nullptr;
    lv_obj_t* _read_button_label = nullptr;
    lv_obj_t* _auto_button = nullptr;
    lv_obj_t* _auto_button_label = nullptr;

    lv_obj_t* _rf_line1 = nullptr;
    lv_obj_t* _rf_line2 = nullptr;
    lv_obj_t* _rf_irq = nullptr;
    lv_obj_t* _rf_flags = nullptr;
    lv_obj_t* _rf_raw = nullptr;
    lv_obj_t* _rf_sample = nullptr;
    lv_obj_t* _rf_sample_button_label = nullptr;

    lv_obj_t* _bus_identity = nullptr;
    lv_obj_t* _bus_backend = nullptr;
    lv_obj_t* _bus_totals = nullptr;
    lv_obj_t* _bus_errors = nullptr;
    lv_obj_t* _bus_last = nullptr;
    lv_obj_t* _bus_last_detail = nullptr;
    lv_obj_t* _bus_verify_button_label = nullptr;
};

}  // namespace nfc_debug::view
