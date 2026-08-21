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

using namespace m5::nfc;
using namespace m5::nfc::a;
using namespace m5::nfc::a::mifare;
using namespace m5::nfc::a::mifare::classic;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitNFC unit{};  // I2C
m5::nfc::EmulationLayerA emu_a{unit};

PICC picc{};

constexpr Type type{Type::MIFARE_Ultralight};
constexpr uint8_t uid[] = {0x04, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE};
uint8_t picc_memory[]   = {
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0xA3, 0x00, 0x00,  //
    0xE1, 0x10, 0x06, 0x00,  //
    0x03, 0x25, 0x91, 0x01,  //
    0x0D, 0x55, 0x04, 0x6D,  //
    0x35, 0x73, 0x74, 0x61,  //
    0x63, 0x6B, 0x2E, 0x63,  //
    0x6F, 0x6D, 0x2F, 0x51,  //
    0x01, 0x10, 0x54, 0x02,  //
    0x65, 0x6E, 0x48, 0x65,  //
    0x6C, 0x6C, 0x6F, 0x20,  //
    0x4D, 0x35, 0x53, 0x74,  //
    0x61, 0x63, 0x6B, 0xFE,  //
    0x44, 0x45, 0x46, 0x00,  //
    0x44, 0x45, 0x46, 0x00,  //
};

uint8_t bcc8(const uint8_t* p, const uint8_t len, const uint8_t init = 0)
{
    uint8_t v = init;
    for (uint_fast8_t i = 0; i < len; ++i) {
        v ^= p[i];
    }
    return v;
}

// Correctly embed the Ultralight and NTAG UIDs into memory
void embed_uid(uint8_t mem[9], const uint8_t uid[7])
{
    memcpy(mem, uid, 3);
    mem[3] = bcc8(uid, 3, 0x88 /* CT */);
    memcpy(mem + 4, uid + 3, 4);
    mem[8] = bcc8(uid + 3, 4);
}

constexpr uint16_t color_table[] = {
    //  None,      Off,     Idle,     Ready,   Active,      Halt };
    TFT_BLACK, TFT_RED, TFT_BLUE, TFT_YELLOW, TFT_GREEN, TFT_MAGENTA};
constexpr const char* state_table[] = {"-", "O", "I", "R", "A", "H"};

}  // namespace

void setup()
{
    M5StackChan.begin();

    // Emulation settings
    auto cfg      = unit.config();
    cfg.emulation = true;
    cfg.mode      = NFC::A;
    unit.config(cfg);

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
    lcd.setFont(&fonts::Font2);

    //
    lcd.startWrite();
    lcd.fillScreen(TFT_RED);
    if (picc.emulate(type, uid, sizeof(uid))) {
        embed_uid(picc_memory, uid);
        if (emu_a.begin(picc, picc_memory, sizeof(picc_memory))) {
            lcd.fillScreen(TFT_DARKGREEN);
            lcd.setCursor(0, 16);
            const auto& e_picc = emu_a.emulatePICC();
            M5.Log.printf("Emulation:%s %s ATQA:%04X SAK:%u\n", e_picc.typeAsString().c_str(),
                          e_picc.uidAsString().c_str(), e_picc.atqa, e_picc.sak);
            lcd.printf("%s\n%s\nATQA:%04X SAK:%u", e_picc.typeAsString().c_str(), e_picc.uidAsString().c_str(),
                       e_picc.atqa, e_picc.sak);
        }
    }
    lcd.fillRect(0, 0, 32, 16, color_table[0]);
    lcd.drawString(state_table[0], 0, 0);
    lcd.endWrite();
}

void loop()
{
    M5StackChan.update();
    Units.update();
    emu_a.update();  // Need call in loop

    static EmulationLayerA::State latest{};
    auto state = emu_a.state();
    if (latest != state) {
        latest = state;
        lcd.startWrite();
        lcd.fillRect(0, 0, 32, 16, color_table[m5::stl::to_underlying(state)]);
        lcd.drawString(state_table[m5::stl::to_underlying(state)], 0, 0);
        lcd.endWrite();
    }
}
