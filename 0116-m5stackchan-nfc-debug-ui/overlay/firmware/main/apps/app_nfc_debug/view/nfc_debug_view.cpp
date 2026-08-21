/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#include "nfc_debug_view.h"

#include <cstdio>
#include <esp_err.h>
#include <utility>

namespace nfc_debug::view {
namespace {
constexpr lv_color_t COLOR_BACKGROUND = LV_COLOR_MAKE(0x0C, 0x0F, 0x14);
constexpr lv_color_t COLOR_PANEL = LV_COLOR_MAKE(0x17, 0x1B, 0x22);
constexpr lv_color_t COLOR_BORDER = LV_COLOR_MAKE(0x36, 0x3C, 0x48);
constexpr lv_color_t COLOR_TEXT = LV_COLOR_MAKE(0xF3, 0xF5, 0xF7);
constexpr lv_color_t COLOR_MUTED = LV_COLOR_MAKE(0xA2, 0xA9, 0xB5);
constexpr lv_color_t COLOR_PURPLE = LV_COLOR_MAKE(0x6D, 0x4A, 0xFF);
constexpr lv_color_t COLOR_GREEN = LV_COLOR_MAKE(0x31, 0xC4, 0x78);
constexpr lv_color_t COLOR_AMBER = LV_COLOR_MAKE(0xF0, 0xAE, 0x42);
constexpr lv_color_t COLOR_RED = LV_COLOR_MAKE(0xF0, 0x4E, 0x5E);
constexpr lv_color_t COLOR_MAGENTA = LV_COLOR_MAKE(0xD8, 0x4A, 0xFF);
constexpr uint8_t IRQ_RXS = 0x20;
constexpr uint8_t IRQ_RXE = 0x10;
constexpr uint8_t IRQ_COL = 0x04;
constexpr uint8_t IRQ_NRE = 0x40;

lv_obj_t* make_panel(lv_obj_t* parent)
{
    lv_obj_t* panel = lv_obj_create(parent);
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_size(panel, 320, 168);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_bg_color(panel, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    return panel;
}

lv_obj_t* make_label(lv_obj_t* parent, int x, int y, int width, int height,
                     const lv_font_t* font, lv_color_t color,
                     lv_text_align_t alignment = LV_TEXT_ALIGN_LEFT)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, width, height);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_align(label, alignment, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    return label;
}

lv_obj_t* make_button(lv_obj_t* parent, int x, int y, int width, int height,
                      lv_color_t color)
{
    lv_obj_t* button = lv_button_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_bg_color(button, color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, COLOR_BORDER, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    return button;
}

lv_obj_t* make_button_label(lv_obj_t* button, int width, const char* text)
{
    lv_obj_t* label = make_label(button, 0, 13, width, 18, &lv_font_montserrat_14,
                                 COLOR_TEXT, LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(label, text);
    return label;
}

lv_color_t reader_color(ReaderState state)
{
    switch (state) {
    case ReaderState::TagFound: return COLOR_GREEN;
    case ReaderState::TransportError: return COLOR_RED;
    case ReaderState::ProtocolError: return COLOR_MAGENTA;
    case ReaderState::Scanning: return COLOR_PURPLE;
    case ReaderState::NoTag: return COLOR_MUTED;
    default: return COLOR_TEXT;
    }
}

const char* operation_name(uint8_t operation)
{
    switch (operation) {
    case 1: return "READ A";
    case 2: return "WRITE A";
    case 3: return "READ B";
    case 4: return "WRITE B";
    case 5: return "DIRECT";
    case 6: return "FIFO READ";
    case 7: return "FIFO WRITE";
    default: return "NONE";
    }
}
}  // namespace

NfcDebugView::NfcDebugView(Service& service, std::function<void()> close_callback)
    : _service(service), _close_callback(std::move(close_callback))
{
    create_frame();
    create_reader_page();
    create_rf_page();
    create_bus_page();
    select_page(Page::Reader);
}

NfcDebugView::~NfcDebugView()
{
    if (_root != nullptr) {
        lv_obj_delete(_root);
        _root = nullptr;
    }
}

void NfcDebugView::create_frame()
{
    _root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_root, 320, 240);
    lv_obj_align(_root, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(_root, 0, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_radius(_root, 0, 0);
    lv_obj_set_style_bg_color(_root, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(_root, LV_OBJ_FLAG_SCROLLABLE);

    _header = lv_obj_create(_root);
    lv_obj_set_pos(_header, 0, 0);
    lv_obj_set_size(_header, 320, 28);
    lv_obj_set_style_pad_all(_header, 0, 0);
    lv_obj_set_style_border_width(_header, 0, 0);
    lv_obj_set_style_radius(_header, 0, 0);
    lv_obj_set_style_bg_color(_header, COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(_header, LV_OPA_COVER, 0);
    lv_obj_remove_flag(_header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = make_label(_header, 8, 6, 82, 18, &lv_font_montserrat_14, COLOR_TEXT);
    lv_label_set_text(title, "NFC LAB");
    lv_obj_t* i2c = make_label(_header, 100, 6, 30, 18, &lv_font_montserrat_14, COLOR_MUTED);
    lv_label_set_text(i2c, "I2C");

    _health_dot = lv_obj_create(_header);
    lv_obj_set_pos(_health_dot, 133, 9);
    lv_obj_set_size(_health_dot, 10, 10);
    lv_obj_set_style_radius(_health_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_health_dot, 0, 0);
    lv_obj_set_style_bg_color(_health_dot, COLOR_MUTED, 0);
    lv_obj_set_style_bg_opa(_health_dot, LV_OPA_COVER, 0);

    _error_count = make_label(_header, 154, 6, 91, 18, &lv_font_montserrat_14, COLOR_MUTED);
    lv_label_set_text(_error_count, "err:000");

    _content = lv_obj_create(_root);
    lv_obj_set_pos(_content, 0, 28);
    lv_obj_set_size(_content, 320, 168);
    lv_obj_set_style_pad_all(_content, 0, 0);
    lv_obj_set_style_border_width(_content, 0, 0);
    lv_obj_set_style_radius(_content, 0, 0);
    lv_obj_set_style_bg_color(_content, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(_content, LV_OPA_COVER, 0);
    lv_obj_remove_flag(_content, LV_OBJ_FLAG_SCROLLABLE);

    _navigation = lv_buttonmatrix_create(_root);
    static const char* map[] = {"READ", "RF/IRQ", "BUS", "REGS/LOG", ""};
    lv_buttonmatrix_set_map(_navigation, map);
    lv_obj_set_pos(_navigation, 0, 196);
    lv_obj_set_size(_navigation, 320, 44);
    lv_obj_set_style_pad_all(_navigation, 0, 0);
    lv_obj_set_style_pad_column(_navigation, 0, 0);
    lv_obj_set_style_pad_row(_navigation, 0, 0);
    lv_obj_set_style_radius(_navigation, 0, 0);
    lv_obj_set_style_bg_color(_navigation, COLOR_PANEL, 0);
    lv_obj_set_style_border_width(_navigation, 0, 0);
    lv_obj_set_style_text_font(_navigation, &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(_navigation, COLOR_PANEL, LV_PART_ITEMS);
    lv_obj_set_style_text_color(_navigation, COLOR_MUTED, LV_PART_ITEMS);
    lv_obj_set_style_radius(_navigation, 0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(_navigation, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(_navigation, COLOR_BORDER, LV_PART_ITEMS);
    const auto checked_items = static_cast<lv_style_selector_t>(
        static_cast<uint32_t>(LV_PART_ITEMS) | static_cast<uint32_t>(LV_STATE_CHECKED));
    lv_obj_set_style_bg_color(_navigation, COLOR_PURPLE, checked_items);
    lv_obj_set_style_text_color(_navigation, COLOR_TEXT, checked_items);
    lv_buttonmatrix_set_button_ctrl_all(_navigation, LV_BUTTONMATRIX_CTRL_CHECKABLE);
    lv_buttonmatrix_set_one_checked(_navigation, true);
    lv_buttonmatrix_set_button_ctrl(_navigation, 3, LV_BUTTONMATRIX_CTRL_DISABLED);
    lv_obj_add_event_cb(_navigation, navigation_event, LV_EVENT_VALUE_CHANGED, this);
}

void NfcDebugView::create_reader_page()
{
    _reader_page = make_panel(_content);
    _reader_state = make_label(_reader_page, 8, 3, 304, 27, &lv_font_montserrat_20,
                               COLOR_TEXT, LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(_reader_state, "STARTING");
    _reader_primary = make_label(_reader_page, 12, 34, 296, 38, &lv_font_montserrat_16,
                                 COLOR_TEXT, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(_reader_primary, LV_LABEL_LONG_WRAP);
    lv_label_set_text(_reader_primary, "Starting NFC worker...");
    _reader_secondary = make_label(_reader_page, 12, 72, 296, 18, &lv_font_montserrat_14,
                                   COLOR_MUTED, LV_TEXT_ALIGN_CENTER);
    _reader_meta = make_label(_reader_page, 12, 90, 296, 18, &lv_font_montserrat_14,
                              COLOR_MUTED, LV_TEXT_ALIGN_CENTER);
    _reader_stages = make_label(_reader_page, 12, 105, 296, 18, &lv_font_montserrat_14,
                                COLOR_MUTED, LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(_reader_stages, "DETECT -   SELECT -   IDENTIFY -");

    _read_button = make_button(_reader_page, 16, 120, 138, 44, COLOR_PURPLE);
    lv_obj_add_event_cb(_read_button, read_event, LV_EVENT_CLICKED, this);
    _read_button_label = make_button_label(_read_button, 138, "READ ONCE");
    _auto_button = make_button(_reader_page, 166, 120, 138, 44, COLOR_PANEL);
    lv_obj_add_event_cb(_auto_button, auto_event, LV_EVENT_CLICKED, this);
    _auto_button_label = make_button_label(_auto_button, 138, "AUTO: OFF");
}

void NfcDebugView::create_rf_page()
{
    _rf_page = make_panel(_content);
    _rf_line1 = make_label(_rf_page, 10, 6, 300, 18, &lv_font_montserrat_14, COLOR_TEXT);
    _rf_line2 = make_label(_rf_page, 10, 28, 300, 18, &lv_font_montserrat_14, COLOR_TEXT);
    _rf_irq = make_label(_rf_page, 10, 52, 300, 18, &lv_font_montserrat_14, COLOR_TEXT);
    _rf_flags = make_label(_rf_page, 10, 74, 300, 18, &lv_font_montserrat_14, COLOR_MUTED);
    _rf_raw = make_label(_rf_page, 10, 94, 300, 18, &lv_font_montserrat_14, COLOR_MUTED);
    _rf_sample = make_label(_rf_page, 10, 112, 300, 18, &lv_font_montserrat_14, COLOR_AMBER);

    lv_obj_t* sample = make_button(_rf_page, 16, 122, 138, 42, COLOR_PURPLE);
    lv_obj_add_event_cb(sample, sample_event, LV_EVENT_CLICKED, this);
    _rf_sample_button_label = make_button_label(sample, 138, "SAMPLE 10s");
    lv_obj_t* clear = make_button(_rf_page, 166, 122, 138, 42, COLOR_PANEL);
    lv_obj_add_event_cb(clear, clear_event, LV_EVENT_CLICKED, this);
    make_button_label(clear, 138, "CLEAR");
}

void NfcDebugView::create_bus_page()
{
    _bus_page = make_panel(_content);
    _bus_identity = make_label(_bus_page, 8, 5, 304, 18, &lv_font_montserrat_14, COLOR_TEXT);
    _bus_backend = make_label(_bus_page, 8, 25, 304, 18, &lv_font_montserrat_14, COLOR_MUTED);
    _bus_totals = make_label(_bus_page, 8, 48, 304, 18, &lv_font_montserrat_14, COLOR_TEXT);
    _bus_errors = make_label(_bus_page, 8, 68, 304, 18, &lv_font_montserrat_14, COLOR_TEXT);
    _bus_last = make_label(_bus_page, 8, 91, 304, 18, &lv_font_montserrat_14, COLOR_AMBER);
    _bus_last_detail = make_label(_bus_page, 8, 109, 304, 18, &lv_font_montserrat_14, COLOR_MUTED);

    lv_obj_t* probe = make_button(_bus_page, 8, 124, 94, 40, COLOR_PANEL);
    lv_obj_add_event_cb(probe, probe_event, LV_EVENT_CLICKED, this);
    make_button_label(probe, 94, "PROBE");
    lv_obj_t* verify = make_button(_bus_page, 108, 124, 96, 40, COLOR_PURPLE);
    lv_obj_add_event_cb(verify, verify_event, LV_EVENT_CLICKED, this);
    _bus_verify_button_label = make_button_label(verify, 96, "VERIFY 20x");
    lv_obj_t* reinitialize = make_button(_bus_page, 210, 124, 102, 40, COLOR_PANEL);
    lv_obj_add_event_cb(reinitialize, reinitialize_event, LV_EVENT_CLICKED, this);
    make_button_label(reinitialize, 102, "REINIT NFC");
}

void NfcDebugView::update(const Snapshot& snapshot)
{
    _snapshot = snapshot;
    render_header(snapshot);
    render_reader(snapshot);
    render_rf(snapshot);
    render_bus(snapshot);
}

void NfcDebugView::show_start_error(esp_err_t error)
{
    Snapshot snapshot{};
    snapshot.reader_state = ReaderState::TransportError;
    snapshot.transport_state = TransportState::Failed;
    snapshot.last_error.code = error;
    snapshot.counters.failed = 1;
    update(snapshot);
}

void NfcDebugView::render_header(const Snapshot& snapshot)
{
    lv_color_t color = COLOR_MUTED;
    if (snapshot.transport_state == TransportState::Healthy) color = COLOR_GREEN;
    else if (snapshot.transport_state == TransportState::Warning) color = COLOR_AMBER;
    else if (snapshot.transport_state == TransportState::Failed) color = COLOR_RED;
    lv_obj_set_style_bg_color(_health_dot, color, 0);

    char error_text[20];
    std::snprintf(error_text, sizeof(error_text), "err:%03lu",
                  static_cast<unsigned long>(snapshot.counters.failed % 1000));
    lv_label_set_text(_error_count, error_text);
}

void NfcDebugView::render_reader(const Snapshot& snapshot)
{
    lv_label_set_text(_reader_state, reader_state_name(snapshot.reader_state));
    lv_obj_set_style_text_color(_reader_state, reader_color(snapshot.reader_state), 0);

    char primary[96] = {};
    char secondary[64] = {};
    char meta[80] = {};
    const char* stages = "DETECT -   SELECT -   IDENTIFY -";
    switch (snapshot.reader_state) {
    case ReaderState::Starting:
        std::snprintf(primary, sizeof(primary), "Starting NFC worker...");
        break;
    case ReaderState::Ready:
        std::snprintf(primary, sizeof(primary), "Place ONE tag across the literal\nTOP EDGE of the head");
        std::snprintf(secondary, sizeof(secondary), "Then touch READ ONCE");
        break;
    case ReaderState::Scanning:
        std::snprintf(primary, sizeof(primary), "Polling ISO14443-A...");
        std::snprintf(secondary, sizeof(secondary), "Keep the tag still");
        break;
    case ReaderState::TagFound: {
        size_t used = std::snprintf(primary, sizeof(primary), "UID  ");
        for (uint8_t i = 0; i < snapshot.uid_len && used < sizeof(primary); ++i) {
            used += std::snprintf(primary + used, sizeof(primary) - used,
                                  "%s%02X", i == 0 ? "" : ":", snapshot.uid[i]);
        }
        std::snprintf(secondary, sizeof(secondary), "%s", snapshot.type_name.data());
        std::snprintf(meta, sizeof(meta), "ATQA %04X   SAK %02X   UID %u bytes",
                      snapshot.atqa, snapshot.sak, snapshot.uid_len);
        stages = "DETECT OK   SELECT OK   IDENTIFY OK";
        break;
    }
    case ReaderState::NoTag:
        std::snprintf(primary, sizeof(primary), "No ISO14443-A response");
        std::snprintf(secondary, sizeof(secondary), "Transport completed without a tag");
        break;
    case ReaderState::TransportError:
        std::snprintf(primary, sizeof(primary), "%s", esp_err_to_name(snapshot.last_error.code));
        std::snprintf(secondary, sizeof(secondary), "raw transport failure; op %u key %02X",
                      snapshot.last_error.transport_operation, snapshot.last_error.transport_key);
        std::snprintf(meta, sizeof(meta), "elapsed %.1f ms",
                      snapshot.last_error.elapsed_us / 1000.0);
        break;
    case ReaderState::ProtocolError:
        std::snprintf(primary, sizeof(primary), "%s", esp_err_to_name(snapshot.last_error.code));
        std::snprintf(secondary, sizeof(secondary), "NFC frame/select failed after transport");
        std::snprintf(meta, sizeof(meta), "elapsed %.1f ms",
                      snapshot.last_error.elapsed_us / 1000.0);
        break;
    case ReaderState::Stopped:
        std::snprintf(primary, sizeof(primary), "NFC worker stopped");
        break;
    }
    lv_label_set_text(_reader_primary, primary);
    lv_label_set_text(_reader_secondary, secondary);
    lv_label_set_text(_reader_meta, meta);
    lv_label_set_text(_reader_stages, stages);

    if (snapshot.reader_state == ReaderState::Scanning) {
        lv_obj_add_state(_read_button, LV_STATE_DISABLED);
        lv_label_set_text(_read_button_label, "READING...");
    } else {
        lv_obj_remove_state(_read_button, LV_STATE_DISABLED);
        lv_label_set_text(_read_button_label,
                          snapshot.reader_state == ReaderState::TagFound ? "READ AGAIN" : "READ ONCE");
    }
    lv_label_set_text(_auto_button_label, snapshot.auto_poll ? "AUTO: ON" : "AUTO: OFF");
    lv_obj_set_style_bg_color(_auto_button, snapshot.auto_poll ? COLOR_PURPLE : COLOR_PANEL, 0);
}

void NfcDebugView::render_rf(const Snapshot& snapshot)
{
    char text[96];
    std::snprintf(text, sizeof(text), "RF FIELD %-3s                 CAP %3u",
                  snapshot.field_on ? "ON" : "OFF", snapshot.rf.capacitance);
    lv_label_set_text(_rf_line1, text);
    std::snprintf(text, sizeof(text), "RSSI %02X      FIFO %03u      NRT %04X",
                  snapshot.rf.rssi, snapshot.rf.fifo_bytes, snapshot.rf.nrt);
    lv_label_set_text(_rf_line2, text);
    std::snprintf(text, sizeof(text), "LAST IRQ  main=%02lX timer=%02X err=%02X",
                  static_cast<unsigned long>(snapshot.rf.main_irq),
                  snapshot.rf.timer_irq, snapshot.rf.error_irq);
    lv_label_set_text(_rf_irq, text);
    std::snprintf(text, sizeof(text), "RXS %u  RXE %u  COL %u  NRE %u  ERR %u",
                  (snapshot.rf.main_irq & IRQ_RXS) != 0,
                  (snapshot.rf.main_irq & IRQ_RXE) != 0,
                  (snapshot.rf.main_irq & IRQ_COL) != 0,
                  (snapshot.rf.timer_irq & IRQ_NRE) != 0,
                  snapshot.rf.error_irq != 0);
    lv_label_set_text(_rf_flags, text);
    std::snprintf(text, sizeof(text), "OPC %02X   COLL %02X   no-tag %lu",
                  snapshot.rf.operation_control, snapshot.rf.collision,
                  static_cast<unsigned long>(snapshot.no_tag_count));
    lv_label_set_text(_rf_raw, text);
    std::snprintf(text, sizeof(text), "sample events %u / %u%s",
                  snapshot.sample_events, snapshot.sample_attempts,
                  snapshot.sample_active ? "  RUNNING" : "");
    lv_label_set_text(_rf_sample, text);
    lv_label_set_text(_rf_sample_button_label,
                      snapshot.sample_active ? "SAMPLING..." : "SAMPLE 10s");
}

void NfcDebugView::render_bus(const Snapshot& snapshot)
{
    char text[112];
    std::snprintf(text, sizeof(text), "ST25R3916 0x50     type %02X rev %02X",
                  snapshot.chip_type, snapshot.chip_revision);
    lv_label_set_text(_bus_identity, text);
    lv_label_set_text(_bus_backend, "backend idf-new     speed 400 kHz");
    std::snprintf(text, sizeof(text), "txns %lu   ok %lu   fail %lu",
                  static_cast<unsigned long>(snapshot.counters.transactions),
                  static_cast<unsigned long>(snapshot.counters.succeeded),
                  static_cast<unsigned long>(snapshot.counters.failed));
    lv_label_set_text(_bus_totals, text);
    std::snprintf(text, sizeof(text), "timeout %lu   invalid %lu   mismatch %lu",
                  static_cast<unsigned long>(snapshot.counters.timeouts),
                  static_cast<unsigned long>(snapshot.counters.invalid_state),
                  static_cast<unsigned long>(snapshot.counters.mismatches));
    lv_label_set_text(_bus_errors, text);
    std::snprintf(text, sizeof(text), "LAST: %s key %02X",
                  operation_name(snapshot.last_error.transport_operation),
                  snapshot.last_error.transport_key);
    lv_label_set_text(_bus_last, text);
    std::snprintf(text, sizeof(text), "%s   %.1f ms   verify %u%s",
                  esp_err_to_name(snapshot.last_error.code),
                  snapshot.last_error.elapsed_us / 1000.0,
                  snapshot.verification_passes,
                  snapshot.verification_active ? " RUNNING" : "");
    lv_label_set_text(_bus_last_detail, text);
    lv_label_set_text(_bus_verify_button_label,
                      snapshot.verification_active ? "VERIFYING" : "VERIFY 20x");
}

void NfcDebugView::request_read()
{
    (void)_service.enqueue(Command{CommandType::ReadOnce, 0});
}

void NfcDebugView::toggle_auto()
{
    (void)_service.enqueue(Command{CommandType::SetAutoPoll, _snapshot.auto_poll ? 0U : 1U});
}

void NfcDebugView::select_page(Page page)
{
    _page = page;
    lv_obj_add_flag(_reader_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_rf_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_bus_page, LV_OBJ_FLAG_HIDDEN);
    if (page == Page::Reader) lv_obj_remove_flag(_reader_page, LV_OBJ_FLAG_HIDDEN);
    else if (page == Page::RfIrq) lv_obj_remove_flag(_rf_page, LV_OBJ_FLAG_HIDDEN);
    else if (page == Page::Bus) lv_obj_remove_flag(_bus_page, LV_OBJ_FLAG_HIDDEN);

    for (uint32_t i = 0; i < 3; ++i) {
        lv_buttonmatrix_clear_button_ctrl(_navigation, i, LV_BUTTONMATRIX_CTRL_CHECKED);
    }
    lv_buttonmatrix_set_button_ctrl(_navigation, static_cast<uint32_t>(page),
                                    LV_BUTTONMATRIX_CTRL_CHECKED);
}

void NfcDebugView::close_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr && self->_close_callback) self->_close_callback();
}

void NfcDebugView::read_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr) self->request_read();
}

void NfcDebugView::auto_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr) self->toggle_auto();
}

void NfcDebugView::sample_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr && !self->_snapshot.sample_active) {
        (void)self->_service.enqueue(Command{CommandType::SampleIrqWindow, 10});
    }
}

void NfcDebugView::clear_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr) (void)self->_service.enqueue(Command{CommandType::ClearCounters, 0});
}

void NfcDebugView::probe_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr) (void)self->_service.enqueue(Command{CommandType::Probe, 0});
}

void NfcDebugView::verify_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr && !self->_snapshot.verification_active) {
        (void)self->_service.enqueue(Command{CommandType::VerifyRegisters, 20});
    }
}

void NfcDebugView::reinitialize_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    if (self != nullptr) (void)self->_service.enqueue(Command{CommandType::ResetBus, 0});
}

void NfcDebugView::navigation_event(lv_event_t* event)
{
    auto* self = static_cast<NfcDebugView*>(lv_event_get_user_data(event));
    lv_obj_t* matrix = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (self == nullptr || matrix == nullptr) return;
    const uint32_t selected = lv_buttonmatrix_get_selected_button(matrix);
    if (selected <= 2) self->select_page(static_cast<Page>(selected));
}

}  // namespace nfc_debug::view
