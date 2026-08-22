// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc safety validators. No ESP-IDF required.
// Fixtures: NTAG215 (first_user=4, last_user=129) and MIFARE Classic 1K/4K.

#include <cstdio>
#include <cstring>

#include "gogolem/nfc/safety.hpp"
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

static int test_type2_ntag215() {
    // NTAG215: user pages 4..129.
    CHECK(is_type2_user_page(4, 4, 129));
    CHECK(is_type2_user_page(129, 4, 129));
    CHECK(is_type2_user_page(64, 4, 129));
    CHECK(!is_type2_user_page(3, 4, 129));     // UID/manufacturer/CC
    CHECK(!is_type2_user_page(130, 4, 129));   // dynamic lock/config
    CHECK(!is_type2_user_page(0, 4, 129));

    CHECK(is_type2_protected_page(0, 4, 129));
    CHECK(is_type2_protected_page(3, 4, 129));
    CHECK(is_type2_protected_page(130, 4, 129));
    CHECK(is_type2_protected_page(134, 4, 129));
    CHECK(!is_type2_protected_page(4, 4, 129));
    CHECK(!is_type2_protected_page(129, 4, 129));
    return 0;
}

static int test_classic_1k_geometry() {
    constexpr bool k4k = false;
    CHECK(classic_sector_count(k4k) == 16);
    CHECK(classic_blocks_in_sector(0, k4k) == 4);
    CHECK(classic_blocks_in_sector(15, k4k) == 4);

    // Trailers at the last block of each 4-block sector.
    CHECK(classic_sector_trailer_block(0, k4k) == 3);
    CHECK(classic_sector_trailer_block(1, k4k) == 7);
    CHECK(classic_sector_trailer_block(15, k4k) == 63);

    CHECK(classic_sector_of_block(0, k4k) == 0);
    CHECK(classic_sector_of_block(3, k4k) == 0);
    CHECK(classic_sector_of_block(4, k4k) == 1);
    CHECK(classic_sector_of_block(63, k4k) == 15);

    CHECK(is_classic_manufacturer_block(0));
    CHECK(!is_classic_manufacturer_block(1));

    CHECK(is_classic_trailer_block(3, k4k));
    CHECK(is_classic_trailer_block(7, k4k));
    CHECK(is_classic_trailer_block(63, k4k));
    CHECK(!is_classic_trailer_block(0, k4k));
    CHECK(!is_classic_trailer_block(1, k4k));
    CHECK(!is_classic_trailer_block(2, k4k));

    // 1K has 64 blocks total.
    CHECK(is_classic_user_data_block(1, k4k, 64));
    CHECK(is_classic_user_data_block(2, k4k, 64));
    CHECK(is_classic_user_data_block(62, k4k, 64));
    CHECK(!is_classic_user_data_block(0, k4k, 64));    // manufacturer
    CHECK(!is_classic_user_data_block(3, k4k, 64));   // trailer
    CHECK(!is_classic_user_data_block(7, k4k, 64));   // trailer
    CHECK(!is_classic_user_data_block(64, k4k, 64));  // out of range
    return 0;
}

static int test_classic_4k_geometry() {
    constexpr bool k4k = true;
    CHECK(classic_sector_count(k4k) == 40);
    CHECK(classic_blocks_in_sector(0, k4k) == 4);
    CHECK(classic_blocks_in_sector(31, k4k) == 4);
    CHECK(classic_blocks_in_sector(32, k4k) == 16);
    CHECK(classic_blocks_in_sector(39, k4k) == 16);

    // Small-sector trailers.
    CHECK(classic_sector_trailer_block(0, k4k) == 3);
    CHECK(classic_sector_trailer_block(31, k4k) == 127);
    // Large-sector trailers: sector 32 -> 128..143, trailer 143.
    CHECK(classic_sector_trailer_block(32, k4k) == 143);
    CHECK(classic_sector_trailer_block(39, k4k) == 255);

    CHECK(classic_sector_of_block(127, k4k) == 31);
    CHECK(classic_sector_of_block(128, k4k) == 32);
    CHECK(classic_sector_of_block(143, k4k) == 32);
    CHECK(classic_sector_of_block(255, k4k) == 39);

    CHECK(is_classic_trailer_block(127, k4k));
    CHECK(is_classic_trailer_block(143, k4k));
    CHECK(is_classic_trailer_block(255, k4k));
    CHECK(!is_classic_trailer_block(128, k4k));
    CHECK(!is_classic_trailer_block(142, k4k));

    // 4K has 256 blocks total.
    CHECK(is_classic_user_data_block(1, k4k, 255));
    CHECK(is_classic_user_data_block(128, k4k, 255));
    CHECK(is_classic_user_data_block(142, k4k, 255));
    CHECK(!is_classic_user_data_block(0, k4k, 255));    // manufacturer
    CHECK(!is_classic_user_data_block(143, k4k, 255)); // trailer
    CHECK(!is_classic_user_data_block(255, k4k, 255)); // trailer
    return 0;
}

static int test_safe_write_gate() {
    // NTAG215 user page is safe; protected pages are not.
    CHECK(is_safe_write_target(TagFamily::Ntag21x, 4, 4, 129, false, 0));
    CHECK(is_safe_write_target(TagFamily::Ntag21x, 129, 4, 129, false, 0));
    CHECK(!is_safe_write_target(TagFamily::Ntag21x, 3, 4, 129, false, 0));
    CHECK(!is_safe_write_target(TagFamily::Ntag21x, 130, 4, 129, false, 0));

    // Classic data block is safe; trailer/manufacturer are not.
    CHECK(is_safe_write_target(TagFamily::MifareClassic, 1, 0, 0, false, 64));
    CHECK(is_safe_write_target(TagFamily::MifareClassic, 62, 0, 0, false, 64));
    CHECK(!is_safe_write_target(TagFamily::MifareClassic, 0, 0, 0, false, 64));
    CHECK(!is_safe_write_target(TagFamily::MifareClassic, 3, 0, 0, false, 64));
    CHECK(!is_safe_write_target(TagFamily::MifareClassic, 7, 0, 0, false, 64));

    // Unknown family is never a safe write target.
    CHECK(!is_safe_write_target(TagFamily::Unknown, 4, 4, 129, false, 64));
    return 0;
}

int main() {
    int result = 0;
    result |= test_type2_ntag215();
    result |= test_classic_1k_geometry();
    result |= test_classic_4k_geometry();
    result |= test_safe_write_gate();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
