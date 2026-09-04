// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc Classic value-block codec.

#include <cstdio>
#include <cstring>

#include "gogolem/nfc/classic.hpp"

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

static int test_round_trip_positive_value() {
    auto block = encode_value_block(1234567, 5);
    int32_t value = 0;
    uint8_t address = 0;
    CHECK(decode_value_block(block, value, address));
    CHECK(value == 1234567);
    CHECK(address == 5);
    return 0;
}

static int test_round_trip_negative_value() {
    auto block = encode_value_block(-1000, 62);
    int32_t value = 0;
    uint8_t address = 0;
    CHECK(decode_value_block(block, value, address));
    CHECK(value == -1000);
    CHECK(address == 62);
    return 0;
}

static int test_round_trip_zero_and_max() {
    for (int32_t v : {int32_t(0), int32_t(0x7FFFFFFF), int32_t(-0x7FFFFFFF), int32_t(-1)}) {
        auto block = encode_value_block(v, 0);
        int32_t value = 999;
        uint8_t address = 99;
        CHECK(decode_value_block(block, value, address));
        CHECK(value == v);
        CHECK(address == 0);
    }
    return 0;
}

static int test_rejects_value_complement_mismatch() {
    auto block = encode_value_block(500, 10);
    block[4] ^= 0x01;  // corrupt the complement copy
    int32_t value = 0;
    uint8_t address = 0;
    CHECK(!decode_value_block(block, value, address));
    return 0;
}

static int test_rejects_value_repeat_mismatch() {
    auto block = encode_value_block(500, 10);
    block[8] ^= 0x01;  // corrupt the repeated value copy
    int32_t value = 0;
    uint8_t address = 0;
    CHECK(!decode_value_block(block, value, address));
    return 0;
}

static int test_rejects_address_complement_mismatch() {
    auto block = encode_value_block(500, 10);
    block[13] ^= 0x01;  // corrupt address complement
    int32_t value = 0;
    uint8_t address = 0;
    CHECK(!decode_value_block(block, value, address));
    return 0;
}

static int test_rejects_address_repeat_mismatch() {
    auto block = encode_value_block(500, 10);
    block[14] ^= 0x01;  // corrupt repeated address
    int32_t value = 0;
    uint8_t address = 0;
    CHECK(!decode_value_block(block, value, address));
    return 0;
}

static int test_rejects_non_value_block() {
    std::array<uint8_t, 16> zeros{};
    int32_t value = 0;
    uint8_t address = 0;
    // All-zero block: value 0 is fine, but address complement check: a0=0, a1=0,
    // ~0 != 0, so it must be rejected as not a value block.
    CHECK(!decode_value_block(zeros, value, address));
    return 0;
}

int main() {
    int result = 0;
    result |= test_round_trip_positive_value();
    result |= test_round_trip_negative_value();
    result |= test_round_trip_zero_and_max();
    result |= test_rejects_value_complement_mismatch();
    result |= test_rejects_value_repeat_mismatch();
    result |= test_rejects_address_complement_mismatch();
    result |= test_rejects_address_repeat_mismatch();
    result |= test_rejects_non_value_block();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
