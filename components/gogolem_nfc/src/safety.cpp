// SPDX-License-Identifier: MIT

#include "gogolem/nfc/safety.hpp"

namespace gogolem::nfc {

// ---- Type 2 ---------------------------------------------------------------
bool is_type2_user_page(uint16_t address, uint16_t first_user, uint16_t last_user) {
    return address >= first_user && address <= last_user;
}

bool is_type2_protected_page(uint16_t address, uint16_t first_user, uint16_t last_user) {
    return address < first_user || address > last_user;
}

// ---- MIFARE Classic -------------------------------------------------------
//
// 1K: 16 sectors, 4 blocks each (blocks 0..63).
// 4K: 32 small sectors (4 blocks, blocks 0..127) + 8 large sectors
//     (16 blocks, blocks 128..255). Total 40 sectors.
uint8_t classic_sector_count(bool is_4k) {
    return is_4k ? 40 : 16;
}

uint8_t classic_blocks_in_sector(uint8_t sector, bool is_4k) {
    if (is_4k && sector >= 32) {
        return 16;
    }
    return 4;
}

uint8_t classic_sector_trailer_block(uint8_t sector, bool is_4k) {
    if (is_4k && sector >= 32) {
        // Large sectors start at block 128, 16 blocks each.
        return 128 + (sector - 32) * 16 + 15;
    }
    return sector * 4 + 3;
}

uint8_t classic_sector_of_block(uint8_t block, bool is_4k) {
    if (is_4k && block >= 128) {
        return 32 + (block - 128) / 16;
    }
    return block / 4;
}

bool is_classic_manufacturer_block(uint8_t block) {
    return block == 0;
}

bool is_classic_trailer_block(uint8_t block, bool is_4k) {
    return block == classic_sector_trailer_block(classic_sector_of_block(block, is_4k), is_4k);
}

bool is_classic_user_data_block(uint8_t block, bool is_4k, uint8_t total_blocks) {
    if (block >= total_blocks) {
        return false;
    }
    if (is_classic_manufacturer_block(block)) {
        return false;
    }
    return !is_classic_trailer_block(block, is_4k);
}

// ---- Combined gate --------------------------------------------------------
bool is_safe_write_target(TagFamily family, uint16_t address, uint16_t first_user,
                          uint16_t last_user, bool is_4k_classic, uint8_t classic_blocks) {
    switch (family) {
        case TagFamily::MifareUltralight:
        case TagFamily::Ntag21x:
            return is_type2_user_page(address, first_user, last_user);
        case TagFamily::MifareClassic:
            return is_classic_user_data_block(static_cast<uint8_t>(address), is_4k_classic,
                                              classic_blocks);
        default:
            return false;
    }
}

}  // namespace gogolem::nfc
