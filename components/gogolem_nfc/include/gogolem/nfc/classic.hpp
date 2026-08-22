// SPDX-License-Identifier: MIT
//
// gogolem::nfc MIFARE Classic value blocks and credentials.
//
// A Classic value block stores a signed 32-bit value and an address byte with
// full redundancy so an interrupted write is detectable. The format is:
//   bytes  0..3:  value, little-endian
//   bytes  4..7:  bitwise complement of value, little-endian
//   bytes  8..11: value repeated
//   byte      12: address
//   byte      13: complement of address
//   byte      14: address repeated
//   byte      15: complement of address repeated
//
// encode/decode are pure and host-testable. The Engine validates a block this
// way before treating it as a value block for increment/decrement/transfer.

#pragma once

#include <array>
#include <cstdint>

namespace gogolem::nfc {

struct ClassicKey {
    std::array<uint8_t, 6> bytes{};
};

struct ClassicCredentials {
    ClassicKey key_a{};
    ClassicKey key_b{};
    bool has_key_b{false};
};

// Encode a signed 32-bit value and an 8-bit address into a 16-byte value block.
std::array<uint8_t, 16> encode_value_block(int32_t value, uint8_t address);

// Validate all redundancy and recover value and address. Returns false if any
// redundant copy disagrees.
bool decode_value_block(const std::array<uint8_t, 16>& block, int32_t& out_value,
                         uint8_t& out_address);

}  // namespace gogolem::nfc
