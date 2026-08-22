// SPDX-License-Identifier: MIT
#pragma once

#include <functional>
#include <lvgl.h>

#include "gogolem/nfc/service.hpp"

namespace nfc_debug::view {

class NfcDebugView {
public:
    NfcDebugView(gogolem::nfc::Service& service, std::function<void()> close_callback,
                std::function<void(bool)> auto_poll_callback);
    ~NfcDebugView();

    NfcDebugView(const NfcDebugView&) = delete;
    NfcDebugView& operator=(const NfcDebugView&) = delete;

    void update(const gogolem::nfc::ServiceSnapshot& snapshot);
    void show_start_error(const gogolem::nfc::Error& error);
    void request_read();
    void toggle_auto();

private:
    void create_frame();

    gogolem::nfc::Service& _service;
    std::function<void()> _close_callback;
    std::function<void(bool)> _auto_poll_callback;
    gogolem::nfc::ServiceSnapshot _snapshot{};
    bool _auto_poll = false;

    lv_obj_t* _root = nullptr;
    lv_obj_t* _header = nullptr;
    lv_obj_t* _health_dot = nullptr;
    lv_obj_t* _error_count = nullptr;
    lv_obj_t* _content = nullptr;

    lv_obj_t* _uid_label = nullptr;
    lv_obj_t* _type_label = nullptr;
    lv_obj_t* _atqa_sak_label = nullptr;
    lv_obj_t* _size_label = nullptr;
    lv_obj_t* _ndef_label = nullptr;
    lv_obj_t* _raw_label = nullptr;

    lv_obj_t* _read_button = nullptr;
    lv_obj_t* _auto_button = nullptr;
};

}  // namespace nfc_debug::view
