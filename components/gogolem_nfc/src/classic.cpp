// SPDX-License-Identifier: MIT

#include <cstring>

#include "gogolem/nfc/classic.hpp"

namespace gogolem::nfc {

namespace {

void put_le32(uint8_t* dst, int32_t v) {
    uint32_t u = static_cast<uint32_t>(v);
    dst[0] = static_cast<uint8_t>(u & 0xFF);
    dst[1] = static_cast<uint8_t>((u >> 8) & 0xFF);
    dst[2] = static_cast<uint8_t>((u >> 16) & 0xFF);
    dst[3] = static_cast<uint8_t>((u >> 24) & 0xFF);
}

int32_t get_le32(const uint8_t* src) {
    uint32_t u = static_cast<uint32_t>(src[0]) |
                 (static_cast<uint32_t>(src[1]) << 8) |
                 (static_cast<uint32_t>(src[2]) << 16) |
                 (static_cast<uint32_t>(src[3]) << 24);
    return static_cast<int32_t>(u);
}

}  // namespace

std::array<uint8_t, 16> encode_value_block(int32_t value, uint8_t address) {
    std::array<uint8_t, 16> block{};
    uint8_t inv_address = static_cast<uint8_t>(~address);
    put_le32(&block[0], value);
    put_le32(&block[4], ~value);
    put_le32(&block[8], value);
    block[12] = address;
    block[13] = inv_address;
    block[14] = address;
    block[15] = inv_address;
    return block;
}

bool decode_value_block(const std::array<uint8_t, 16>& block, int32_t& out_value,
                        uint8_t& out_address) {
    int32_t v0 = get_le32(&block[0]);
    int32_t v1 = get_le32(&block[4]);  // complement
    int32_t v2 = get_le32(&block[8]);
    // Complement check: v1 must equal ~v0.
    if (v1 != ~v0) return false;
    if (v2 != v0) return false;
    uint8_t a0 = block[12];
    uint8_t a1 = block[13];
    uint8_t a2 = block[14];
    uint8_t a3 = block[15];
    if (a1 != static_cast<uint8_t>(~a0)) return false;
    if (a2 != a0) return false;
    if (a3 != static_cast<uint8_t>(~a0)) return false;
    out_value = v0;
    out_address = a0;
    return true;
}

}  // namespace gogolem::nfc
