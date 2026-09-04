// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc PICC → TagInfo conversion.

#include <cstdio>
#include <cstring>

#include "gogolem/nfc/picc_map.hpp"
#include "gogolem/nfc/types.hpp"

using namespace gogolem::nfc;

static int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);    \
            ++failures;                                                   \
            return 1;                                                     \
        }                                                                 \
    } while (0)

static int test_family_mapping() {
    CHECK(picc_type_to_family(picc_type::Unknown) == TagFamily::Unknown);
    CHECK(picc_type_to_family(picc_type::MifareClassic1K) == TagFamily::MifareClassic);
    CHECK(picc_type_to_family(picc_type::MifareClassic4K) == TagFamily::MifareClassic);
    CHECK(picc_type_to_family(picc_type::MifareUltralight) == TagFamily::MifareUltralight);
    CHECK(picc_type_to_family(picc_type::MifareUltralightC) == TagFamily::MifareUltralight);
    CHECK(picc_type_to_family(picc_type::Ntag213) == TagFamily::Ntag21x);
    CHECK(picc_type_to_family(picc_type::Ntag215) == TagFamily::Ntag21x);
    CHECK(picc_type_to_family(picc_type::Ntag216) == TagFamily::Ntag21x);
    CHECK(picc_type_to_family(picc_type::Ntag4xx) == TagFamily::Ntag21x);
    CHECK(picc_type_to_family(picc_type::St25ta2K) == TagFamily::St25ta);
    CHECK(picc_type_to_family(picc_type::Iso14443_4) == TagFamily::IsoDepOther);
    CHECK(picc_type_to_family(picc_type::MifarePlus4K) == TagFamily::MifarePlus);
    CHECK(picc_type_to_family(picc_type::MifareDesfire4K) == TagFamily::Desfire);
    CHECK(picc_type_to_family(picc_type::Iso18092) == TagFamily::Unknown);
    CHECK(picc_type_to_family(0xFF) == TagFamily::Unknown);
    return 0;
}

static int test_ntag215_fixture_to_tag_info() {
    PiccFields p{};
    p.uid = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x80, 0, 0, 0};
    p.uid_size = 7;
    p.atqa = 0x0044;
    p.sak = 0x00;
    p.blocks = 135;
    p.unit_size = 4;
    p.user_area = 504;
    p.total_size = 540;
    p.first_user_block = 4;
    p.last_user_block = 129;
    p.supports_ndef = true;
    p.forum_tag_type = 2;
    p.type_code = picc_type::Ntag215;

    TagInfo t = to_tag_info(p);
    CHECK(t.uid_length == 7);
    uint8_t expected[7] = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x80};
    CHECK(t.uid_equals(expected, 7));
    CHECK(t.atqa == 0x0044);
    CHECK(t.sak == 0x00);
    CHECK(t.family == TagFamily::Ntag21x);
    CHECK(t.block_or_page_count == 135);
    CHECK(t.unit_size == 4);
    CHECK(t.user_bytes == 504);
    CHECK(t.total_bytes == 540);
    CHECK(t.first_user_unit == 4);
    CHECK(t.last_user_unit == 129);
    CHECK(t.supports_ndef);
    CHECK(t.nfc_forum_tag_type == 2);
    return 0;
}

static int test_uid_truncation_and_empty() {
    PiccFields p{};
    p.uid_size = 0;
    TagInfo t = to_tag_info(p);
    CHECK(t.uid_length == 0);

    PiccFields big{};
    big.uid_size = 99;  // out of range
    TagInfo tb = to_tag_info(big);
    CHECK(tb.uid_length == 10);  // clamped
    return 0;
}

static int test_classic_4k_fixture() {
    PiccFields p{};
    p.uid = {0x04, 0x11, 0x22, 0x33};
    p.uid_size = 4;
    p.atqa = 0x0002;
    p.sak = 0x18;
    p.blocks = 256;
    p.unit_size = 16;
    p.user_area = 752;
    p.total_size = 4096;
    p.first_user_block = 1;
    p.last_user_block = 255;
    p.supports_ndef = true;
    p.forum_tag_type = 4;
    p.type_code = picc_type::MifareClassic4K;

    TagInfo t = to_tag_info(p);
    CHECK(t.family == TagFamily::MifareClassic);
    CHECK(t.uid_length == 4);
    CHECK(t.unit_size == 16);
    CHECK(t.block_or_page_count == 256);
    CHECK(t.nfc_forum_tag_type == 4);
    return 0;
}

int main() {
    int result = 0;
    result |= test_family_mapping();
    result |= test_ntag215_fixture_to_tag_info();
    result |= test_uid_truncation_and_empty();
    result |= test_classic_4k_fixture();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
