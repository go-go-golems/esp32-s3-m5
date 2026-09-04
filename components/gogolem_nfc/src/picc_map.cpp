// SPDX-License-Identifier: MIT

#include <cstring>

#include "gogolem/nfc/picc_map.hpp"

namespace gogolem::nfc {

TagFamily picc_type_to_family(uint8_t type_code) {
    if (type_code >= picc_type::MifareClassicMini && type_code <= picc_type::MifareClassic4K) {
        return TagFamily::MifareClassic;
    }
    if (type_code >= picc_type::MifareUltralight && type_code <= picc_type::MifareUltralightC) {
        return TagFamily::MifareUltralight;
    }
    if ((type_code >= picc_type::Ntag203 && type_code <= picc_type::Ntag216) ||
        type_code == picc_type::Ntag4xx) {
        return TagFamily::Ntag21x;
    }
    if (type_code >= picc_type::St25ta512B && type_code <= picc_type::St25ta64K) {
        return TagFamily::St25ta;
    }
    if (type_code == picc_type::Iso14443_4) {
        return TagFamily::IsoDepOther;
    }
    if (type_code >= picc_type::MifarePlus2K && type_code <= picc_type::MifarePlusSE) {
        return TagFamily::MifarePlus;
    }
    if (type_code >= picc_type::MifareDesfire2K && type_code <= picc_type::MifareDesfireLight) {
        return TagFamily::Desfire;
    }
    // ISO 18092 (FeliCa/NFC-DEP) and Unknown map to Unknown in version one;
    // the explorer does not expose a FeliCa family yet.
    return TagFamily::Unknown;
}

TagInfo to_tag_info(const PiccFields& p) {
    TagInfo t{};
    uint8_t n = p.uid_size;
    if (n > 10) n = 10;
    if (n > 0) {
        std::memcpy(t.uid.data(), p.uid.data(), n);
    }
    t.uid_length = n;
    t.atqa = p.atqa;
    t.sak = p.sak;
    t.family = picc_type_to_family(p.type_code);
    t.block_or_page_count = p.blocks;
    t.unit_size = p.unit_size;
    t.user_bytes = p.user_area;
    t.total_bytes = p.total_size;
    t.first_user_unit = p.first_user_block;
    t.last_user_unit = p.last_user_block;
    t.supports_ndef = p.supports_ndef;
    t.nfc_forum_tag_type = p.forum_tag_type;
    return t;
}

}  // namespace gogolem::nfc
