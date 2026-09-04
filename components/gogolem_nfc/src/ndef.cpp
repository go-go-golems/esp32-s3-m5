// SPDX-License-Identifier: MIT

#include <cstring>

#include "gogolem/nfc/ndef.hpp"

namespace gogolem::nfc {

namespace {

// NFC Forum RTD-URI identifier codes 0x00..0x23.
const char* const kUriPrefixes[0x24] = {
    "",                                  // 0x00
    "http://www.",                        // 0x01
    "https://www.",                       // 0x02
    "http://",                            // 0x03
    "https://",                           // 0x04
    "tel:",                               // 0x05
    "mailto:",                           // 0x06
    "ftp://anonymous:anonymous@",        // 0x07
    "ftp://ftp.",                         // 0x08
    "ftps://",                            // 0x09
    "smb://",                             // 0x0A
    "nfs://",                             // 0x0B
    "ftp://",                             // 0x0C
    "dav://",                             // 0x0D
    "news:",                              // 0x0E
    "telnet://",                          // 0x0F
    "imap:",                              // 0x10
    "rtsp://",                            // 0x11
    "urn:",                               // 0x12
    "pop:",                               // 0x13
    "sip:",                               // 0x14
    "sips:",                              // 0x15
    "tftp:",                              // 0x16
    "btspp://",                           // 0x17
    "btl2cap://",                         // 0x18
    "btgoep://",                          // 0x19
    "tcpobex://",                         // 0x1A
    "irdaobex://",                        // 0x1B
    "file://",                            // 0x1C
    "urn:epc:id:",                        // 0x1D
    "urn:epc:tag:",                       // 0x1E
    "urn:epc:pat:",                       // 0x1F
    "urn:epc:raw:",                       // 0x20
    "urn:epc:",                           // 0x21
    "urn:nfc:",                           // 0x22
    "wsp://",                             // 0x23
};

constexpr size_t kUriPrefixCount = sizeof(kUriPrefixes) / sizeof(kUriPrefixes[0]);

size_t prefix_length(const char* p) {
    size_t n = 0;
    while (p[n] != '\0') ++n;
    return n;
}

}  // namespace

// ---- URI records ----------------------------------------------------------
NdefRecord make_uri_record(std::string_view uri) {
    NdefRecord r;
    r.tnf = NdefTnf::WellKnown;
    r.type = {'U'};
    uint8_t best_code = 0x00;
    size_t best_len = 0;
    for (size_t code = 0; code < kUriPrefixCount; ++code) {
        size_t plen = prefix_length(kUriPrefixes[code]);
        if (plen == 0) continue;
        if (uri.size() >= plen && uri.compare(0, plen, kUriPrefixes[code], plen) == 0) {
            // Pick the longest matching prefix (most compact encoding).
            if (plen > best_len) {
                best_len = plen;
                best_code = static_cast<uint8_t>(code);
            }
        }
    }
    r.payload.push_back(best_code);
    const size_t offset = best_len;
    r.payload.insert(r.payload.end(), uri.begin() + offset, uri.end());
    return r;
}

std::string uri_record_to_string(const NdefRecord& record) {
    if (record.tnf != NdefTnf::WellKnown || record.type.size() != 1 ||
        record.type[0] != 'U' || record.payload.empty()) {
        return {};
    }
    uint8_t code = record.payload[0];
    if (code >= kUriPrefixCount) {
        return {};
    }
    std::string result(kUriPrefixes[code]);
    result.append(reinterpret_cast<const char*>(record.payload.data() + 1),
                   record.payload.size() - 1);
    return result;
}

// ---- Text records ---------------------------------------------------------
NdefRecord make_text_record(std::string_view text, std::string_view language) {
    NdefRecord r;
    r.tnf = NdefTnf::WellKnown;
    r.type = {'T'};
    if (language.size() > 0x3F) {
        language = language.substr(0, 0x3F);
    }
    uint8_t status = static_cast<uint8_t>(language.size() & 0x3F);  // UTF-8, bit7 = 0
    r.payload.push_back(status);
    r.payload.insert(r.payload.end(), language.begin(), language.end());
    r.payload.insert(r.payload.end(), text.begin(), text.end());
    return r;
}

std::string text_record_to_string(const NdefRecord& record, std::string& out_lang) {
    out_lang.clear();
    if (record.tnf != NdefTnf::WellKnown || record.type.size() != 1 ||
        record.type[0] != 'T' || record.payload.empty()) {
        return {};
    }
    uint8_t status = record.payload[0];
    if (status & 0x80) {
        // UTF-16 encoding is not supported in version one.
        return {};
    }
    size_t lang_len = status & 0x3F;
    if (1 + lang_len > record.payload.size()) {
        return {};
    }
    out_lang.assign(reinterpret_cast<const char*>(record.payload.data() + 1), lang_len);
    size_t text_offset = 1 + lang_len;
    return std::string(reinterpret_cast<const char*>(record.payload.data() + text_offset),
                       record.payload.size() - text_offset);
}

// ---- Message encode/decode ------------------------------------------------
namespace {

bool write_u8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
    return true;
}
bool write_u32_be(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    return true;
}

}  // namespace

bool encode_ndef_message(const NdefMessage& message, std::vector<uint8_t>& out) {
    if (message.records.empty()) {
        return false;
    }
    for (size_t i = 0; i < message.records.size(); ++i) {
        const NdefRecord& r = message.records[i];
        if (r.type.size() > 0xFF || r.id.size() > 0xFF) {
            return false;
        }
        bool first = (i == 0);
        bool last = (i + 1 == message.records.size());
        bool sr = r.payload.size() <= 0xFF;
        bool il = !r.id.empty();
        uint8_t flags = 0;
        if (first) flags |= 0x80;  // MB
        if (last) flags |= 0x40;   // ME
        // CF (0x20) always 0
        if (sr) flags |= 0x10;     // SR
        if (il) flags |= 0x08;     // IL
        flags |= static_cast<uint8_t>(r.tnf) & 0x07;
        write_u8(out, flags);
        write_u8(out, static_cast<uint8_t>(r.type.size()));
        if (sr) {
            write_u8(out, static_cast<uint8_t>(r.payload.size()));
        } else {
            write_u32_be(out, static_cast<uint32_t>(r.payload.size()));
        }
        if (il) {
            write_u8(out, static_cast<uint8_t>(r.id.size()));
        }
        out.insert(out.end(), r.type.begin(), r.type.end());
        if (il) out.insert(out.end(), r.id.begin(), r.id.end());
        out.insert(out.end(), r.payload.begin(), r.payload.end());
    }
    return true;
}

bool decode_ndef_message(const uint8_t* data, size_t length, NdefMessage& out) {
    out.records.clear();
    if (data == nullptr || length == 0) return false;
    size_t pos = 0;
    bool saw_begin = false;
    bool done = false;
    while (pos < length && !done) {
        uint8_t flags = data[pos++];
        bool mb = flags & 0x80;
        bool me = flags & 0x40;
        bool cf = flags & 0x20;
        bool sr = flags & 0x10;
        bool il = flags & 0x08;
        uint8_t tnf = flags & 0x07;
        if (!mb && !saw_begin) return false;  // first record must set MB
        if (cf) return false;                 // chunked not supported
        saw_begin = true;
        if (pos >= length) return false;
        uint8_t type_len = data[pos++];
        uint32_t payload_len = 0;
        if (sr) {
            if (pos >= length) return false;
            payload_len = data[pos++];
        } else {
            if (pos + 4 > length) return false;
            payload_len = (static_cast<uint32_t>(data[pos]) << 24) |
                          (static_cast<uint32_t>(data[pos + 1]) << 16) |
                          (static_cast<uint32_t>(data[pos + 2]) << 8) |
                          static_cast<uint32_t>(data[pos + 3]);
            pos += 4;
        }
        uint8_t id_len = 0;
        if (il) {
            if (pos >= length) return false;
            id_len = data[pos++];
        }
        if (pos + type_len + id_len + payload_len > length) return false;
        NdefRecord r;
        r.tnf = static_cast<NdefTnf>(tnf);
        r.type.assign(data + pos, data + pos + type_len);
        pos += type_len;
        r.id.assign(data + pos, data + pos + id_len);
        pos += id_len;
        r.payload.assign(data + pos, data + pos + payload_len);
        pos += payload_len;
        out.records.push_back(std::move(r));
        if (me) done = true;
    }
    return done && !out.records.empty();
}

// ---- Type 2 TLV framing ---------------------------------------------------
bool encode_type2_ndef_tlv(const NdefMessage& message, std::vector<uint8_t>& out) {
    std::vector<uint8_t> ndef_bytes;
    if (!encode_ndef_message(message, ndef_bytes)) {
        return false;
    }
    out.push_back(0x03);  // NDEF Message TLV tag
    if (ndef_bytes.size() <= 0xFE) {
        out.push_back(static_cast<uint8_t>(ndef_bytes.size()));
    } else {
        out.push_back(0xFF);
        uint16_t len = static_cast<uint16_t>(ndef_bytes.size());
        out.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>(len & 0xFF));
    }
    out.insert(out.end(), ndef_bytes.begin(), ndef_bytes.end());
    out.push_back(0xFE);  // Terminator TLV
    return true;
}

bool decode_type2_ndef_tlv(const uint8_t* data, size_t length, NdefMessage& out) {
    if (data == nullptr) return false;
    size_t pos = 0;
    while (pos < length) {
        uint8_t tag = data[pos++];
        if (tag == 0xFE) {
            return false;  // terminator before any NDEF TLV
        }
        if (tag == 0x00) {
            // NULL TLV: skip (length byte follows but is normally 0)
            if (pos >= length) return false;
            uint8_t len = data[pos++];
            pos += len;  // NULL TLVs carry no payload; advance defensively
            continue;
        }
        if (pos >= length) return false;
        uint32_t tlv_len = 0;
        uint8_t len_byte = data[pos++];
        if (len_byte == 0xFF) {
            if (pos + 2 > length) return false;
            tlv_len = (static_cast<uint32_t>(data[pos]) << 8) | data[pos + 1];
            pos += 2;
        } else {
            tlv_len = len_byte;
        }
        if (pos + tlv_len > length) return false;
        if (tag == 0x03) {
            return decode_ndef_message(data + pos, tlv_len, out);
        }
        pos += tlv_len;  // skip lock/memory/proprietary TLVs
    }
    return false;
}

}  // namespace gogolem::nfc
