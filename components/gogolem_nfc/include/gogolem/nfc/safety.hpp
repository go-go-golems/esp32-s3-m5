// SPDX-License-Identifier: MIT
//
// gogolem::nfc safety validators for NFC tag memory regions.
//
// These are pure functions over tag geometry. They encode the rule that generic
// write commands must reject manufacturer, lock, configuration, and
// sector-trailer regions, and must only touch identified ordinary user memory.
// They carry no hardware dependency so they can be unit-tested on the host
// with NTAG21x and MIFARE Classic fixtures.
//
// Type 2 (MIFARE Ultralight / NTAG21x):
//   - user pages are [first_user, last_user] inclusive;
//   - everything else (UID/manufacturer, static lock, capability container,
//     dynamic lock, configuration, password) is protected.
//
// MIFARE Classic (1K and 4K aware):
//   - block 0 is the manufacturer block;
//   - the last block of every sector is the sector trailer (Key A / access /
//     Key B);
//   - the remaining blocks are ordinary data blocks.
//   - 4K cards have 32 small sectors (4 blocks) and 8 large sectors (16 blocks).

#pragma once

#include <cstdint>

#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc {

// ---- Type 2 (Ultralight / NTAG21x) ---------------------------------------
bool is_type2_user_page(uint16_t address, uint16_t first_user, uint16_t last_user);
bool is_type2_protected_page(uint16_t address, uint16_t first_user, uint16_t last_user);

// ---- MIFARE Classic ------------------------------------------------------
uint8_t classic_sector_count(bool is_4k);
uint8_t classic_blocks_in_sector(uint8_t sector, bool is_4k);
uint8_t classic_sector_trailer_block(uint8_t sector, bool is_4k);
uint8_t classic_sector_of_block(uint8_t block, bool is_4k);
bool is_classic_manufacturer_block(uint8_t block);
bool is_classic_trailer_block(uint8_t block, bool is_4k);
bool is_classic_user_data_block(uint8_t block, bool is_4k, uint8_t total_blocks);

// True when a write to `address` is safe for the identified family. This is the
// single gate the Engine consults before any mutation; family-specific rules
// live behind it so callers cannot bypass them with a generic address.
bool is_safe_write_target(TagFamily family, uint16_t address, uint16_t first_user,
                          uint16_t last_user, bool is_4k_classic, uint8_t classic_blocks);

}  // namespace gogolem::nfc
