// SPDX-License-Identifier: MIT
//
// gogolem::nfc PICC → TagInfo conversion.
//
// The Engine converts upstream M5Unit-NFC `PICC` objects into the stable
// `TagInfo` at the component boundary. To keep that conversion host-testable
// without pulling M5Unit-NFC into a host compiler, this module operates on a
// plain `PiccFields` value type and a numeric type code that mirrors the
// upstream `m5::nfc::a::Type` enum ordinals. The Engine fills `PiccFields`
// from the upstream PICC and calls `to_tag_info()`.
//
// Upstream Type ordinals (mirrored here so tests need no upstream header):
//   0  Unknown          1-4   MIFARE Classic (Mini/1K/2K/4K)
//   5-9 MIFARE Ultralight (incl EV1/Nano/C)
//   10-16,29 NTAG (203/210u/210/212/213/215/216/4XX)
//   17-20 ST25TA         21 ISO 14443-4
//   22-24 MIFARE Plus    25-28 MIFARE DESFire   30 ISO 18092

#pragma once

#include <array>
#include <cstdint>

#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc {

// Mirrored upstream `m5::nfc::a::Type` ordinals. Keep in sync with the pinned
// M5Unit-NFC revision; the Engine asserts the mapping at build time via a
// static check in engine.cpp.
namespace picc_type {
constexpr uint8_t Unknown = 0;
constexpr uint8_t MifareClassicMini = 1;
constexpr uint8_t MifareClassic1K = 2;
constexpr uint8_t MifareClassic2K = 3;
constexpr uint8_t MifareClassic4K = 4;
constexpr uint8_t MifareUltralight = 5;
constexpr uint8_t MifareUltralightEV1_1 = 6;
constexpr uint8_t MifareUltralightEV1_2 = 7;
constexpr uint8_t MifareUltralightNano = 8;
constexpr uint8_t MifareUltralightC = 9;
constexpr uint8_t Ntag203 = 10;
constexpr uint8_t Ntag210u = 11;
constexpr uint8_t Ntag210 = 12;
constexpr uint8_t Ntag212 = 13;
constexpr uint8_t Ntag213 = 14;
constexpr uint8_t Ntag215 = 15;
constexpr uint8_t Ntag216 = 16;
constexpr uint8_t St25ta512B = 17;
constexpr uint8_t St25ta2K = 18;
constexpr uint8_t St25ta16K = 19;
constexpr uint8_t St25ta64K = 20;
constexpr uint8_t Iso14443_4 = 21;
constexpr uint8_t MifarePlus2K = 22;
constexpr uint8_t MifarePlus4K = 23;
constexpr uint8_t MifarePlusSE = 24;
constexpr uint8_t MifareDesfire2K = 25;
constexpr uint8_t MifareDesfire4K = 26;
constexpr uint8_t MifareDesfire8K = 27;
constexpr uint8_t MifareDesfireLight = 28;
constexpr uint8_t Ntag4xx = 29;
constexpr uint8_t Iso18092 = 30;
}  // namespace picc_type

// Plain upstream PICC snapshot. The Engine populates this from the upstream
// `PICC` and never exposes upstream types through the public API.
struct PiccFields {
    std::array<uint8_t, 10> uid{};
    uint8_t uid_size{0};       // upstream PICC::size (4, 7, or 10)
    uint16_t atqa{0};
    uint8_t sak{0};
    uint16_t blocks{0};
    uint16_t unit_size{0};     // bytes per block/page
    uint32_t user_area{0};
    uint32_t total_size{0};
    uint16_t first_user_block{0};
    uint16_t last_user_block{0};
    bool supports_ndef{false};
    uint8_t forum_tag_type{0};
    uint8_t type_code{picc_type::Unknown};
};

// Map an upstream type code to the stable TagFamily.
TagFamily picc_type_to_family(uint8_t type_code);

// Convert a full PICC snapshot to TagInfo. UID is copied up to uid_size (<=10).
TagInfo to_tag_info(const PiccFields& p);

}  // namespace gogolem::nfc
