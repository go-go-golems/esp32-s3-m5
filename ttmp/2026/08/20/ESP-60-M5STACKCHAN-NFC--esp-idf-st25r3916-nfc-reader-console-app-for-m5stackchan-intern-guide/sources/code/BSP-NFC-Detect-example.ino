/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
// More examples on: https://github.com/m5stack/M5Unit-NFC/tree/main/examples
#include <Arduino.h>
#include <M5StackChan.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedNFC.h>
#include <M5Utility.h>
#include <vector>

using namespace m5::nfc::a;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitNFC unit{};  // I2C
m5::nfc::NFCLayerA nfc_a{unit};
}  // namespace

void setup()
{
    M5StackChan.begin();

    if (!Units.add(unit, M5.In_I2C) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    if (lcd.width() < lcd.height()) {
        lcd.setRotation(1);
    }
    lcd.setFont(&fonts::Font0);
    lcd.fillScreen(0);
    lcd.setCursor(0, 0);
}

void loop()
{
    M5StackChan.update();
    Units.update();

    std::vector<PICC> piccs;
    if (nfc_a.detect(piccs)) {
        lcd.fillScreen(0);
        lcd.setCursor(0, 0);
        uint16_t idx{};
        for (auto&& u : piccs) {
            M5.Speaker.tone(6000, 5);
            // detect only performs a provisional classification based on sak, so further identification is required
            if (nfc_a.identify(u)) {
                M5.Log.printf("PICC:%s %s %04X/%02X %u/%u\n", u.uidAsString().c_str(), u.typeAsString().c_str(), u.atqa,
                              u.sak, u.userAreaSize(), u.totalSize());
                lcd.printf("[%2u]:PICC:<%s> %s\n", idx, u.uidAsString().c_str(), u.typeAsString().c_str());
                ++idx;
            } else {
                M5_LOGW("Failed to identify %s %s %04X/%02X %u/%u", u.uidAsString().c_str(), u.typeAsString().c_str(),
                        u.atqa, u.sak, u.userAreaSize(), u.totalSize());
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
