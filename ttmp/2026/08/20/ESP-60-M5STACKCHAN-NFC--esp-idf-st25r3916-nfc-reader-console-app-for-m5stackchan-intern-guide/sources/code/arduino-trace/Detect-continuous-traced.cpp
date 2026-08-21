/*
 * Continuous ST25R3916 monitor derived from the official StackChan-BSP
 * Detect.ino. WUPA wakes tags halted by the previous cycle. I2C events are
 * buffered during each poll, summarized over serial, and rendered on screen.
 */
#include <Arduino.h>
#include <Wire.h>
#include <M5StackChan.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedNFC.h>
#include <M5Utility.h>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include "esp60_m5_i2c_trace.h"

using namespace m5::nfc::a;

namespace {
constexpr uint32_t POLL_INTERVAL_MS = 250;
constexpr size_t SCREEN_LOG_LINES = 13;
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

std::array<ScreenLine, SCREEN_LOG_LINES> screen_log{};
size_t screen_log_head{};
size_t screen_log_count{};
uint32_t poll_number{};
uint32_t cumulative_transactions{};
uint32_t cumulative_failures{};
uint32_t no_tag_count{};
std::array<char, 32> last_uid{};

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

void render_screen(const char* state, uint16_t state_color, const char* uid,
                   const char* type, uint16_t atqa, uint8_t sak,
                   uint32_t elapsed_ms, const TraceSummary& trace)
{
    lcd.startWrite();
    lcd.fillScreen(TFT_BLACK);
    lcd.fillRect(0, 0, lcd.width(), 16, TFT_DARKCYAN);
    lcd.setTextColor(TFT_WHITE, TFT_DARKCYAN);
    lcd.setCursor(4, 4);
    lcd.printf("ARDUINO NFC TRACE  poll:%lu", static_cast<unsigned long>(poll_number));

    lcd.setTextColor(state_color, TFT_BLACK);
    lcd.setCursor(4, 20);
    lcd.printf("%-12s %lums", state, static_cast<unsigned long>(elapsed_ms));
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setCursor(4, 32);
    lcd.printf("UID: %s", uid && uid[0] ? uid : "--");
    lcd.setCursor(4, 44);
    lcd.printf("TYPE: %.28s", type && type[0] ? type : "--");
    lcd.setCursor(4, 56);
    lcd.printf("ATQA:%04X SAK:%02X  no-tag:%lu", atqa, sak,
               static_cast<unsigned long>(no_tag_count));
    lcd.setCursor(4, 68);
    lcd.printf("I2C:%lu ok:%lu err:%lu", static_cast<unsigned long>(cumulative_transactions),
               static_cast<unsigned long>(cumulative_transactions - cumulative_failures),
               static_cast<unsigned long>(cumulative_failures));
    lcd.setCursor(4, 80);
    if (trace.has_last) {
        lcd.printf("LAST:%s 0x%02X %luus f:%02X", kind_name(trace.last.kind), trace.last.key,
                   static_cast<unsigned long>(trace.last.elapsed_us), trace.last.failure_stage);
    } else {
        lcd.print("LAST:--");
    }

    lcd.drawFastHLine(0, 92, lcd.width(), TFT_DARKGREY);
    for (size_t i = 0; i < screen_log_count; ++i) {
        const auto& line = screen_log[(screen_log_head + i) % screen_log.size()];
        lcd.setTextColor(line.color, TFT_BLACK);
        lcd.setCursor(4, 96 + static_cast<int>(i) * 11);
        lcd.print(line.text.data());
    }
    lcd.endWrite();
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
    if (!init_ok) {
        append_screen_log(TFT_RED, "INIT FAILED: I2C errors=%lu",
                          static_cast<unsigned long>(trace.stats.failed));
        render_screen("INIT ERROR", TFT_RED, "", "", 0, 0, millis() - started, trace);
        while (true) m5::utility::delay(10000);
    }

    append_screen_log(TFT_GREEN, "INIT OK: %lu transactions",
                      static_cast<unsigned long>(trace.stats.total));
    append_screen_log(TFT_CYAN, "WUPA polling every %lums",
                      static_cast<unsigned long>(POLL_INTERVAL_MS));
    render_screen("READY", TFT_GREEN, "", "", 0, 0, millis() - started, trace);
    M5.Log.println("M5_CONTINUOUS_READY place tags on literal top edge");
}

void loop()
{
    M5StackChan.update();
    Units.update();
    ++poll_number;

    esp60_m5_i2c_trace_reset();
    const uint32_t started = millis();
    PICC picc{};
    const bool woke = nfc_a.wakeup(picc.atqa);
    const bool selected = woke && nfc_a.select(picc);
    const bool identified = selected && nfc_a.identify(picc);
    const uint32_t elapsed = millis() - started;
    const auto trace = collect_trace("poll", identified, elapsed);

    const char* state = "NO TAG";
    uint16_t state_color = TFT_YELLOW;
    std::string uid;
    std::string type;
    if (selected) {
        uid = picc.uidAsString();
        type = picc.typeAsString();
    }

    if (trace.stats.failed) {
        state = "TRANSPORT";
        state_color = TFT_RED;
        append_screen_log(TFT_RED, "#%lu I2C FAIL n=%lu key=%02X",
                          static_cast<unsigned long>(poll_number),
                          static_cast<unsigned long>(trace.stats.failed),
                          trace.has_last ? trace.last.key : 0);
    } else if (identified) {
        state = "TAG FOUND";
        state_color = TFT_GREEN;
        if (std::strncmp(last_uid.data(), uid.c_str(), last_uid.size() - 1) != 0) {
            std::snprintf(last_uid.data(), last_uid.size(), "%s", uid.c_str());
            append_screen_log(TFT_GREEN, "#%lu UID %s", static_cast<unsigned long>(poll_number), uid.c_str());
            append_screen_log(TFT_CYAN, "  %s %04X/%02X", type.c_str(), picc.atqa, picc.sak);
            M5.Speaker.tone(3000, 10);
        }
    } else if (selected) {
        state = "PROTOCOL";
        state_color = TFT_ORANGE;
        append_screen_log(TFT_ORANGE, "#%lu identify failed %s",
                          static_cast<unsigned long>(poll_number), uid.c_str());
    } else {
        ++no_tag_count;
        if (no_tag_count == 1 || (no_tag_count % 10) == 0) {
            append_screen_log(TFT_YELLOW, "#%lu no tag (count %lu)",
                              static_cast<unsigned long>(poll_number),
                              static_cast<unsigned long>(no_tag_count));
        }
    }

    M5.Log.printf(
        "M5_POLL cycle=%lu woke=%u selected=%u identified=%u uid=%s atqa=%04X sak=%02X elapsed_ms=%lu txns=%lu failed=%lu\n",
        static_cast<unsigned long>(poll_number), woke ? 1U : 0U, selected ? 1U : 0U,
        identified ? 1U : 0U, uid.empty() ? "--" : uid.c_str(), picc.atqa, picc.sak,
        static_cast<unsigned long>(elapsed), static_cast<unsigned long>(trace.stats.total),
        static_cast<unsigned long>(trace.stats.failed));

    render_screen(state, state_color, uid.c_str(), type.c_str(), picc.atqa, picc.sak, elapsed, trace);
    m5::utility::delay(POLL_INTERVAL_MS);
}
