/*
 * Instrumented derivative of M5Stack StackChan-BSP UnitNFC Detect.ino.
 * The NFC behavior is unchanged. I2C records are buffered in RAM by the
 * patched M5Unified I2C_Class and printed only after each high-level phase.
 */
#include <Arduino.h>
#include <Wire.h>
#include <M5StackChan.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedNFC.h>
#include <M5Utility.h>
#include <vector>

#include "esp60_m5_i2c_trace.h"

using namespace m5::nfc::a;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitNFC unit{};
m5::nfc::NFCLayerA nfc_a{unit};
uint32_t cycle_number{};

const char* kind_name(uint8_t kind)
{
    switch (kind) {
        case ESP60_M5_I2C_WRITE: return "W";
        case ESP60_M5_I2C_READ: return "R";
        case ESP60_M5_I2C_WRITE_READ: return "WR";
        default: return "?";
    }
}

void dump_trace(const char* phase, bool phase_ok, uint32_t phase_elapsed_ms)
{
    esp60_m5_i2c_stats_t stats{};
    esp60_m5_i2c_trace_get_stats(&stats);
    M5.Log.printf(
        "M5_PHASE phase=%s ok=%u elapsed_ms=%lu txns=%lu succeeded=%lu failed=%lu buffered=%lu dropped=%lu\n",
        phase, phase_ok ? 1U : 0U, static_cast<unsigned long>(phase_elapsed_ms),
        static_cast<unsigned long>(stats.total), static_cast<unsigned long>(stats.succeeded),
        static_cast<unsigned long>(stats.failed), static_cast<unsigned long>(stats.buffered),
        static_cast<unsigned long>(stats.dropped));

    esp60_m5_i2c_event_t events[24]{};
    while (true) {
        const size_t count = esp60_m5_i2c_trace_drain(events, sizeof(events) / sizeof(events[0]));
        if (count == 0) break;
        for (size_t i = 0; i < count; ++i) {
            const auto& event = events[i];
            M5.Log.printf(
                "M5_I2C txn=%lu t_us=%lu elapsed_us=%lu kind=%s key=0x%02X wlen=%u rlen=%u ok=%u failure_stage=0x%02X\n",
                static_cast<unsigned long>(event.sequence),
                static_cast<unsigned long>(event.timestamp_us),
                static_cast<unsigned long>(event.elapsed_us), kind_name(event.kind), event.key,
                event.write_len, event.read_len, event.failure_stage == 0 ? 1U : 0U,
                event.failure_stage);
        }
    }
}
}  // namespace

void setup()
{
    M5StackChan.begin();

    esp60_m5_i2c_trace_reset();
    const uint32_t init_started = millis();
    const bool init_ok = Units.add(unit, M5.In_I2C) && Units.begin();
    dump_trace("init", init_ok, millis() - init_started);
    if (!init_ok) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) m5::utility::delay(10000);
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    if (lcd.width() < lcd.height()) lcd.setRotation(1);
    lcd.setFont(&fonts::Font0);
    lcd.fillScreen(0);
    lcd.setCursor(0, 0);
    M5.Log.println("M5_TRACE_READY place tag on literal top edge");
}

void loop()
{
    M5StackChan.update();
    Units.update();

    ++cycle_number;
    std::vector<PICC> piccs;
    esp60_m5_i2c_trace_reset();
    const uint32_t detect_started = millis();
    const bool detected = nfc_a.detect(piccs);
    const uint32_t detect_elapsed = millis() - detect_started;
    dump_trace("detect", detected, detect_elapsed);
    M5.Log.printf("M5_DETECT cycle=%lu found=%u piccs=%u elapsed_ms=%lu\n",
                  static_cast<unsigned long>(cycle_number), detected ? 1U : 0U,
                  static_cast<unsigned>(piccs.size()), static_cast<unsigned long>(detect_elapsed));

    if (detected) {
        lcd.fillScreen(0);
        lcd.setCursor(0, 0);
        uint16_t idx{};
        for (auto&& u : piccs) {
            M5.Speaker.tone(6000, 5);
            esp60_m5_i2c_trace_reset();
            const uint32_t identify_started = millis();
            const bool identified = nfc_a.identify(u);
            dump_trace("identify", identified, millis() - identify_started);
            if (identified) {
                M5.Log.printf("PICC:%s %s %04X/%02X %u/%u\n", u.uidAsString().c_str(),
                              u.typeAsString().c_str(), u.atqa, u.sak, u.userAreaSize(), u.totalSize());
                lcd.printf("[%2u]:PICC:<%s> %s\n", idx, u.uidAsString().c_str(), u.typeAsString().c_str());
                ++idx;
            } else {
                M5_LOGW("Failed to identify %s %s %04X/%02X %u/%u", u.uidAsString().c_str(),
                        u.typeAsString().c_str(), u.atqa, u.sak, u.userAreaSize(), u.totalSize());
            }
        }
        if (idx) {
            M5.Speaker.tone(3000, 10);
            lcd.printf("==> %u PICC\n", idx);
            M5.Log.printf("==> %u PICC\n", idx);
        }
        nfc_a.deactivate();
    }
}
