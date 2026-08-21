/*
 * Continuous multi-tag ST25R3916 monitor derived from the official StackChan
 * Detect.ino. WUPA wakes tags halted by the previous cycle; the official vector
 * detect path then enumerates PICCs during a bounded collection window.
 */
#include <Arduino.h>
#include <Wire.h>
#include <M5StackChan.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedNFC.h>
#include <M5Utility.h>
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "esp60_m5_i2c_trace.h"

using namespace m5::nfc::a;

namespace {
constexpr uint32_t DETECT_WINDOW_MS = 120;
constexpr uint32_t POLL_INTERVAL_MS = 250;
constexpr size_t MAX_DISPLAY_TAGS = 4;
constexpr size_t SCREEN_LOG_LINES = 10;
constexpr size_t SCREEN_LOG_WIDTH = 48;

auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitNFC unit{};
m5::nfc::NFCLayerA nfc_a{unit};

struct ScreenLine {
    std::array<char, SCREEN_LOG_WIDTH> text{};
    uint16_t color = TFT_LIGHTGREY;
};

struct TraceSummary {
    esp60_m5_i2c_stats_t stats{};
    esp60_m5_i2c_event_t last{};
    bool has_last = false;
};

struct TagView {
    std::array<char, 24> uid{};
    std::array<char, 28> type{};
    uint16_t atqa{};
    uint8_t sak{};
    bool identified{};
};

std::array<ScreenLine, SCREEN_LOG_LINES> screen_log{};
size_t screen_log_head{};
size_t screen_log_count{};
uint32_t poll_number{};
uint32_t cumulative_transactions{};
uint32_t cumulative_failures{};
uint32_t no_tag_count{};
std::array<char, 128> last_signature{};

void append_screen_log(uint16_t color, const char* format, ...)
{
    ScreenLine line{};
    line.color = color;
    va_list args;
    va_start(args, format);
    vsnprintf(line.text.data(), line.text.size(), format, args);
    va_end(args);

    const size_t index = (screen_log_head + screen_log_count) % screen_log.size();
    screen_log[index] = line;
    if (screen_log_count == screen_log.size()) {
        screen_log_head = (screen_log_head + 1) % screen_log.size();
    } else {
        ++screen_log_count;
    }
}

const char* kind_name(uint8_t kind)
{
    switch (kind) {
        case ESP60_M5_I2C_WRITE: return "W";
        case ESP60_M5_I2C_READ: return "R";
        case ESP60_M5_I2C_WRITE_READ: return "WR";
        default: return "?";
    }
}

TraceSummary collect_trace(const char* phase, bool phase_ok, uint32_t elapsed_ms)
{
    TraceSummary summary{};
    esp60_m5_i2c_trace_get_stats(&summary.stats);
    cumulative_transactions += summary.stats.total;
    cumulative_failures += summary.stats.failed;

    esp60_m5_i2c_event_t events[24]{};
    while (true) {
        const size_t count = esp60_m5_i2c_trace_drain(events, sizeof(events) / sizeof(events[0]));
        if (count == 0) break;
        for (size_t i = 0; i < count; ++i) {
            summary.last = events[i];
            summary.has_last = true;
            if (events[i].failure_stage != ESP60_M5_I2C_FAIL_NONE) {
                M5.Log.printf(
                    "M5_I2C_FAIL phase=%s txn=%lu elapsed_us=%lu kind=%s key=0x%02X wlen=%u rlen=%u failure_stage=0x%02X\n",
                    phase, static_cast<unsigned long>(events[i].sequence),
                    static_cast<unsigned long>(events[i].elapsed_us), kind_name(events[i].kind),
                    events[i].key, events[i].write_len, events[i].read_len,
                    events[i].failure_stage);
            }
        }
    }

    M5.Log.printf(
        "M5_PHASE phase=%s ok=%u elapsed_ms=%lu txns=%lu succeeded=%lu failed=%lu dropped=%lu cumulative_txns=%lu cumulative_failed=%lu\n",
        phase, phase_ok ? 1U : 0U, static_cast<unsigned long>(elapsed_ms),
        static_cast<unsigned long>(summary.stats.total),
        static_cast<unsigned long>(summary.stats.succeeded),
        static_cast<unsigned long>(summary.stats.failed),
        static_cast<unsigned long>(summary.stats.dropped),
        static_cast<unsigned long>(cumulative_transactions),
        static_cast<unsigned long>(cumulative_failures));
    return summary;
}

void render_screen(const char* state, uint16_t state_color,
                   const std::array<TagView, MAX_DISPLAY_TAGS>& tags, size_t tag_count,
                   uint32_t elapsed_ms, const TraceSummary& trace)
{
    lcd.startWrite();
    lcd.fillScreen(TFT_BLACK);
    lcd.fillRect(0, 0, lcd.width(), 16, TFT_DARKCYAN);
    lcd.setTextColor(TFT_WHITE, TFT_DARKCYAN);
    lcd.setCursor(4, 4);
    lcd.printf("ARDUINO NFC MULTI  poll:%lu", static_cast<unsigned long>(poll_number));

    lcd.setTextColor(state_color, TFT_BLACK);
    lcd.setCursor(4, 20);
    lcd.printf("%-18s %lums", state, static_cast<unsigned long>(elapsed_ms));
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setCursor(4, 32);
    lcd.printf("I2C:%lu ok:%lu err:%lu drop:%lu",
               static_cast<unsigned long>(cumulative_transactions),
               static_cast<unsigned long>(cumulative_transactions - cumulative_failures),
               static_cast<unsigned long>(cumulative_failures),
               static_cast<unsigned long>(trace.stats.dropped));
    lcd.drawFastHLine(0, 43, lcd.width(), TFT_DARKGREY);

    for (size_t i = 0; i < MAX_DISPLAY_TAGS; ++i) {
        const int y = 47 + static_cast<int>(i) * 18;
        if (i < tag_count) {
            lcd.setTextColor(tags[i].identified ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
            lcd.setCursor(4, y);
            lcd.printf("%u %-16s %04X/%02X", static_cast<unsigned>(i + 1),
                       tags[i].uid.data(), tags[i].atqa, tags[i].sak);
            lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            lcd.setCursor(16, y + 9);
            lcd.printf("%.28s%s", tags[i].type.data(), tags[i].identified ? "" : " ?");
        } else {
            lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
            lcd.setCursor(4, y);
            lcd.printf("%u --", static_cast<unsigned>(i + 1));
        }
    }

    lcd.drawFastHLine(0, 120, lcd.width(), TFT_DARKGREY);
    for (size_t i = 0; i < screen_log_count; ++i) {
        const auto& line = screen_log[(screen_log_head + i) % screen_log.size()];
        lcd.setTextColor(line.color, TFT_BLACK);
        lcd.setCursor(4, 124 + static_cast<int>(i) * 11);
        lcd.print(line.text.data());
    }
    lcd.endWrite();
}

std::string make_signature(const std::array<TagView, MAX_DISPLAY_TAGS>& tags, size_t count)
{
    std::string signature;
    for (size_t i = 0; i < count; ++i) {
        if (!signature.empty()) signature += ',';
        signature += tags[i].uid.data();
    }
    return signature;
}
}  // namespace

void setup()
{
    M5StackChan.begin();
    if (lcd.width() < lcd.height()) lcd.setRotation(1);
    lcd.setFont(&fonts::Font0);
    lcd.setTextWrap(false);

    esp60_m5_i2c_trace_reset();
    const uint32_t started = millis();
    const bool init_ok = Units.add(unit, M5.In_I2C) && Units.begin();
    const auto trace = collect_trace("init", init_ok, millis() - started);
    std::array<TagView, MAX_DISPLAY_TAGS> no_tags{};
    if (!init_ok) {
        append_screen_log(TFT_RED, "INIT FAILED: I2C errors=%lu",
                          static_cast<unsigned long>(trace.stats.failed));
        render_screen("INIT ERROR", TFT_RED, no_tags, 0, millis() - started, trace);
        while (true) m5::utility::delay(10000);
    }

    append_screen_log(TFT_GREEN, "INIT OK: %lu transactions",
                      static_cast<unsigned long>(trace.stats.total));
    append_screen_log(TFT_CYAN, "WUPA + %lums collect window",
                      static_cast<unsigned long>(DETECT_WINDOW_MS));
    render_screen("READY", TFT_GREEN, no_tags, 0, millis() - started, trace);
    M5.Log.println("M5_MULTI_READY place up to four tags on literal top edge");
}

void loop()
{
    M5StackChan.update();
    Units.update();
    ++poll_number;

    esp60_m5_i2c_trace_reset();
    const uint32_t started = millis();
    uint16_t wake_atqa{};
    const bool woke = nfc_a.wakeup(wake_atqa);
    std::vector<PICC> piccs;
    const bool detected = nfc_a.detect(piccs, DETECT_WINDOW_MS);

    std::array<TagView, MAX_DISPLAY_TAGS> tags{};
    const size_t displayed = std::min(piccs.size(), tags.size());
    size_t identified_count = 0;
    for (size_t i = 0; i < displayed; ++i) {
        const bool identified = nfc_a.identify(piccs[i]);
        tags[i].identified = identified;
        tags[i].atqa = piccs[i].atqa;
        tags[i].sak = piccs[i].sak;
        std::snprintf(tags[i].uid.data(), tags[i].uid.size(), "%s", piccs[i].uidAsString().c_str());
        std::snprintf(tags[i].type.data(), tags[i].type.size(), "%s", piccs[i].typeAsString().c_str());
        identified_count += identified ? 1U : 0U;
        M5.Log.printf(
            "M5_TAG cycle=%lu index=%u uid=%s identified=%u type=\"%s\" atqa=%04X sak=%02X\n",
            static_cast<unsigned long>(poll_number), static_cast<unsigned>(i), tags[i].uid.data(),
            identified ? 1U : 0U, tags[i].type.data(), tags[i].atqa, tags[i].sak);
    }

    const uint32_t elapsed = millis() - started;
    const auto trace = collect_trace("multi_poll", detected, elapsed);
    const char* state = "NO TAG";
    uint16_t state_color = TFT_YELLOW;
    char state_text[32]{};

    if (trace.stats.failed) {
        state = "TRANSPORT ERROR";
        state_color = TFT_RED;
        append_screen_log(TFT_RED, "#%lu I2C FAIL n=%lu key=%02X",
                          static_cast<unsigned long>(poll_number),
                          static_cast<unsigned long>(trace.stats.failed),
                          trace.has_last ? trace.last.key : 0);
    } else if (displayed) {
        std::snprintf(state_text, sizeof(state_text), "%u TAGS / %u OK",
                      static_cast<unsigned>(displayed), static_cast<unsigned>(identified_count));
        state = state_text;
        state_color = identified_count == displayed ? TFT_GREEN : TFT_ORANGE;
        const std::string signature = make_signature(tags, displayed);
        if (std::strncmp(last_signature.data(), signature.c_str(), last_signature.size() - 1) != 0) {
            std::snprintf(last_signature.data(), last_signature.size(), "%s", signature.c_str());
            append_screen_log(state_color, "#%lu found %u tags (%u identified)",
                              static_cast<unsigned long>(poll_number),
                              static_cast<unsigned>(displayed),
                              static_cast<unsigned>(identified_count));
            for (size_t i = 0; i < displayed; ++i) {
                append_screen_log(tags[i].identified ? TFT_GREEN : TFT_ORANGE,
                                  "  %u %s %s", static_cast<unsigned>(i + 1),
                                  tags[i].uid.data(), tags[i].identified ? "OK" : "?");
            }
            M5.Speaker.tone(3000, 10);
        }
    } else {
        ++no_tag_count;
        if (no_tag_count == 1 || (no_tag_count % 10) == 0) {
            append_screen_log(TFT_YELLOW, "#%lu no tags (count %lu)",
                              static_cast<unsigned long>(poll_number),
                              static_cast<unsigned long>(no_tag_count));
        }
    }

    M5.Log.printf(
        "M5_MULTI cycle=%lu woke=%u wake_atqa=%04X detected=%u piccs=%u displayed=%u identified=%u elapsed_ms=%lu txns=%lu failed=%lu dropped=%lu\n",
        static_cast<unsigned long>(poll_number), woke ? 1U : 0U, wake_atqa,
        detected ? 1U : 0U, static_cast<unsigned>(piccs.size()),
        static_cast<unsigned>(displayed), static_cast<unsigned>(identified_count),
        static_cast<unsigned long>(elapsed), static_cast<unsigned long>(trace.stats.total),
        static_cast<unsigned long>(trace.stats.failed), static_cast<unsigned long>(trace.stats.dropped));

    render_screen(state, state_color, tags, displayed, elapsed, trace);
    m5::utility::delay(POLL_INTERVAL_MS);
}
