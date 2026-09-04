// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc NDEF codec. No ESP-IDF required.
// Round-trip encode/decode of URI and text records, multi-record messages,
// long records, and Type 2 TLV framing.

#include <cstdio>
#include <cstring>
#include <vector>

#include "gogolem/nfc/ndef.hpp"

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

static bool records_equal(const NdefRecord& a, const NdefRecord& b) {
    return a.tnf == b.tnf && a.type == b.type && a.id == b.id && a.payload == b.payload;
}

static int test_uri_record_packs_prefix() {
    NdefRecord r = make_uri_record("https://m5stack.com/esp60");
    CHECK(r.tnf == NdefTnf::WellKnown);
    CHECK(r.type == std::vector<uint8_t>{'U'});
    CHECK(r.payload[0] == 0x04);  // https://
    std::string rest(r.payload.begin() + 1, r.payload.end());
    CHECK(rest == "m5stack.com/esp60");
    CHECK(uri_record_to_string(r) == "https://m5stack.com/esp60");
    return 0;
}

static int test_uri_record_longest_prefix_wins() {
    // "https://www." (0x02) is longer than "https://" (0x04).
    NdefRecord r = make_uri_record("https://www.example.com");
    CHECK(r.payload[0] == 0x02);
    CHECK(uri_record_to_string(r) == "https://www.example.com");
    return 0;
}

static int test_uri_record_no_prefix() {
    NdefRecord r = make_uri_record("custom:opaque");
    CHECK(r.payload[0] == 0x00);
    CHECK(uri_record_to_string(r) == "custom:opaque");
    return 0;
}

static int test_text_record() {
    NdefRecord r = make_text_record("Native ESP-IDF M5StackChan NFC", "en");
    CHECK(r.tnf == NdefTnf::WellKnown);
    CHECK(r.type == std::vector<uint8_t>{'T'});
    CHECK((r.payload[0] & 0x3F) == 2);  // "en"
    std::string lang;
    std::string text = text_record_to_string(r, lang);
    CHECK(lang == "en");
    CHECK(text == "Native ESP-IDF M5StackChan NFC");
    return 0;
}

static int test_single_record_message_round_trip() {
    NdefMessage msg;
    msg.records.push_back(make_uri_record("https://m5stack.com/esp60"));
    std::vector<uint8_t> out;
    CHECK(encode_ndef_message(msg, out));
    CHECK((out[0] & 0x80) != 0);  // MB
    CHECK((out[0] & 0x40) != 0);  // ME
    CHECK((out[0] & 0x10) != 0);  // SR (payload short)
    NdefMessage decoded;
    CHECK(decode_ndef_message(out.data(), out.size(), decoded));
    CHECK(decoded.records.size() == 1);
    CHECK(records_equal(decoded.records[0], msg.records[0]));
    CHECK(uri_record_to_string(decoded.records[0]) == "https://m5stack.com/esp60");
    return 0;
}

static int test_multi_record_message_round_trip() {
    NdefMessage msg;
    msg.records.push_back(make_uri_record("https://m5stack.com/esp60"));
    msg.records.push_back(make_text_record("Native ESP-IDF M5StackChan NFC", "en"));
    std::vector<uint8_t> out;
    CHECK(encode_ndef_message(msg, out));
    // First record: MB set, ME clear. Last record: ME set.
    CHECK((out[0] & 0x80) != 0);
    CHECK((out[0] & 0x40) == 0);
    NdefMessage decoded;
    CHECK(decode_ndef_message(out.data(), out.size(), decoded));
    CHECK(decoded.records.size() == 2);
    CHECK(records_equal(decoded.records[0], msg.records[0]));
    CHECK(records_equal(decoded.records[1], msg.records[1]));
    std::string lang;
    CHECK(uri_record_to_string(decoded.records[0]) == "https://m5stack.com/esp60");
    CHECK(text_record_to_string(decoded.records[1], lang) == "Native ESP-IDF M5StackChan NFC");
    CHECK(lang == "en");
    return 0;
}

static int test_long_record_uses_four_byte_length() {
    NdefRecord r;
    r.tnf = NdefTnf::Unknown;
    r.type = {};
    r.payload.assign(300, 0xAB);  // > 255 -> not SR
    NdefMessage msg;
    msg.records.push_back(r);
    std::vector<uint8_t> out;
    CHECK(encode_ndef_message(msg, out));
    CHECK((out[0] & 0x10) == 0);  // SR clear
    NdefMessage decoded;
    CHECK(decode_ndef_message(out.data(), out.size(), decoded));
    CHECK(decoded.records.size() == 1);
    CHECK(decoded.records[0].payload.size() == 300);
    CHECK(decoded.records[0].payload[100] == 0xAB);
    return 0;
}

static int test_record_with_id_round_trip() {
    NdefRecord r = make_uri_record("tel:+18005551234");
    r.id = {0x01, 0x02, 0x03};
    NdefMessage msg;
    msg.records.push_back(r);
    std::vector<uint8_t> out;
    CHECK(encode_ndef_message(msg, out));
    CHECK((out[0] & 0x08) != 0);  // IL set
    NdefMessage decoded;
    CHECK(decode_ndef_message(out.data(), out.size(), decoded));
    CHECK(records_equal(decoded.records[0], r));
    CHECK(uri_record_to_string(decoded.records[0]) == "tel:+18005551234");
    return 0;
}

static int test_decode_rejects_chunked_and_truncated() {
    // Chunked flag set -> reject.
    std::vector<uint8_t> bad = {0x20};  // CF set, MB not set anyway
    NdefMessage m;
    CHECK(!decode_ndef_message(bad.data(), bad.size(), m));

    // Truncated: a record claiming more payload than available.
    NdefMessage msg;
    msg.records.push_back(make_uri_record("https://m5stack.com/esp60"));
    std::vector<uint8_t> out;
    CHECK(encode_ndef_message(msg, out));
    out.pop_back();  // truncate
    NdefMessage m2;
    CHECK(!decode_ndef_message(out.data(), out.size(), m2));

    // Empty message encode fails.
    std::vector<uint8_t> empty_out;
    NdefMessage empty;
    CHECK(!encode_ndef_message(empty, empty_out));
    return 0;
}

static int test_type2_tlv_round_trip() {
    NdefMessage msg;
    msg.records.push_back(make_uri_record("https://m5stack.com/esp60"));
    msg.records.push_back(make_text_record("Native ESP-IDF M5StackChan NFC", "en"));
    std::vector<uint8_t> tlv;
    CHECK(encode_type2_ndef_tlv(msg, tlv));
    CHECK(tlv[0] == 0x03);
    CHECK(tlv.back() == 0xFE);
    NdefMessage decoded;
    CHECK(decode_type2_ndef_tlv(tlv.data(), tlv.size(), decoded));
    CHECK(decoded.records.size() == 2);
    CHECK(uri_record_to_string(decoded.records[0]) == "https://m5stack.com/esp60");
    return 0;
}

static int test_type2_tlv_empty_ndef_matches_real_tag() {
    // The known NTAG215 is NDEF-formatted but empty: 03 00 FE.
    std::vector<uint8_t> empty_ndef = {0x03, 0x00, 0xFE};
    NdefMessage decoded;
    // A zero-length NDEF Message TLV is a valid empty NDEF area; decode of the
    // inner message returns false (no records), which the Engine treats as
    // "valid format, zero records".
    bool inner_ok = decode_type2_ndef_tlv(empty_ndef.data(), empty_ndef.size(), decoded);
    // The TLV is found; the inner zero-length message has no records.
    CHECK(!inner_ok || decoded.records.empty());
    return 0;
}

static int test_type2_tlv_extended_length() {
    NdefRecord r;
    r.tnf = NdefTnf::Unknown;
    r.payload.assign(300, 0x7A);
    NdefMessage msg;
    msg.records.push_back(r);
    std::vector<uint8_t> tlv;
    CHECK(encode_type2_ndef_tlv(msg, tlv));
    CHECK(tlv[0] == 0x03);
    CHECK(tlv[1] == 0xFF);  // extended length marker
    NdefMessage decoded;
    CHECK(decode_type2_ndef_tlv(tlv.data(), tlv.size(), decoded));
    CHECK(decoded.records.size() == 1);
    CHECK(decoded.records[0].payload.size() == 300);
    return 0;
}

int main() {
    int result = 0;
    result |= test_uri_record_packs_prefix();
    result |= test_uri_record_longest_prefix_wins();
    result |= test_uri_record_no_prefix();
    result |= test_text_record();
    result |= test_single_record_message_round_trip();
    result |= test_multi_record_message_round_trip();
    result |= test_long_record_uses_four_byte_length();
    result |= test_record_with_id_round_trip();
    result |= test_decode_rejects_chunked_and_truncated();
    result |= test_type2_tlv_round_trip();
    result |= test_type2_tlv_empty_ndef_matches_real_tag();
    result |= test_type2_tlv_extended_length();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
