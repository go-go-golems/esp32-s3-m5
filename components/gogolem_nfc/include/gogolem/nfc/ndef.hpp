// SPDX-License-Identifier: MIT
//
// gogolem::nfc NDEF public model and codec.
//
// This is a deterministic, host-testable implementation of the NFC Forum NDEF
// record/message format and the Type 2 Tag TLV framing. It does not depend on
// M5Unit-NFC or on hardware, so encode/decode correctness can be unit-tested
// with round-trips. The Engine converts between upstream NDEF objects and
// these stable public types at the component boundary.
//
// Scope (version one):
//   - NdefRecord with Type Name Format, type, id, payload.
//   - NdefMessage as an ordered list of records.
//   - URI records (well-known type "U") with the NFC Forum URI identifier
//     code table, selecting the shortest matching prefix automatically.
//   - Text records (well-known type "T") with UTF-8 text and a language code.
//   - Short-record (SR) and long-record encoding, single-record and
//     multi-record messages. Chunked records (CF) are rejected on decode.
//   - Type 2 TLV framing: NDEF Message TLV (0x03) + length + terminator (0xFE).

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gogolem::nfc {

enum class NdefTnf : uint8_t {
    Empty = 0x00,
    WellKnown = 0x01,
    Mime = 0x02,
    AbsoluteUri = 0x03,
    External = 0x04,
    Unknown = 0x05,
    Unchanged = 0x06,
    Reserved = 0x07,
};

struct NdefRecord {
    NdefTnf tnf{NdefTnf::Unknown};
    std::vector<uint8_t> type;
    std::vector<uint8_t> id;
    std::vector<uint8_t> payload;
};

struct NdefMessage {
    std::vector<NdefRecord> records;
};

// ---- Record helpers ------------------------------------------------------
NdefRecord make_uri_record(std::string_view uri);
NdefRecord make_text_record(std::string_view text, std::string_view language = "en");

// Resolve a URI record payload back to a full URI string (prefix expansion).
std::string uri_record_to_string(const NdefRecord& record);

// Resolve a text record payload to its text (language is returned via out_lang).
std::string text_record_to_string(const NdefRecord& record, std::string& out_lang);

// ---- Message encode/decode -----------------------------------------------
// Encode an NDEF message to its binary record stream. The first record sets MB,
// the last sets ME; chunking is not used.
bool encode_ndef_message(const NdefMessage& message, std::vector<uint8_t>& out);

// Decode a binary record stream into a message. Returns false on malformed
// input (bad lengths, missing MB/ME, or chunked records).
bool decode_ndef_message(const uint8_t* data, size_t length, NdefMessage& out);

// ---- Type 2 TLV framing --------------------------------------------------
// Encode Type 2 TLVs for an NDEF message: 0x03 <len> <ndef bytes> 0xFE.
// Uses the one-byte length form for messages up to 255 bytes.
bool encode_type2_ndef_tlv(const NdefMessage& message, std::vector<uint8_t>& out);

// Decode the NDEF Message TLV from a Type 2 data area. Returns false if no
// valid NDEF Message TLV is found.
bool decode_type2_ndef_tlv(const uint8_t* data, size_t length, NdefMessage& out);

}  // namespace gogolem::nfc
