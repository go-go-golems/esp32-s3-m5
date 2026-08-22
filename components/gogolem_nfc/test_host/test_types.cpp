// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc domain types. No ESP-IDF required.
// Compile with test_host/build.sh.

#include <cstdio>
#include <cstring>

#include "gogolem/nfc/types.hpp"
#include "gogolem/nfc/version.hpp"

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

static int test_name_helpers() {
    CHECK(std::strcmp(mode_name(Mode::Reader), "reader") == 0);
    CHECK(std::strcmp(mode_name(Mode::EmulationNtag213), "emulation-ntag213") == 0);
    CHECK(std::strcmp(lifecycle_state_name(LifecycleState::ReadyReader), "ready-reader") == 0);
    CHECK(std::strcmp(lifecycle_state_name(LifecycleState::Faulted), "faulted") == 0);
    CHECK(std::strcmp(error_layer_name(ErrorLayer::Transport), "transport") == 0);
    CHECK(std::strcmp(error_layer_name(ErrorLayer::Restoration), "restoration") == 0);
    CHECK(std::strcmp(operation_name(Operation::WriteNdef), "write-ndef") == 0);
    CHECK(std::strcmp(tag_family_name(TagFamily::Ntag21x), "NTAG21x") == 0);
    CHECK(std::strcmp(tag_family_name(TagFamily::MifareClassic), "MIFARE Classic") == 0);
    CHECK(std::strcmp(mode_name(static_cast<Mode>(0xFF)), "unknown-mode") == 0);
    return 0;
}

static int test_error_set_detail_truncates_and_terminates() {
    Error err;
    CHECK(err.layer == ErrorLayer::None);
    CHECK(err.is_none());
    CHECK(err.esp_code == ESP_CODE_OK);

    err.set_detail("short");
    CHECK(std::strcmp(err.detail.data(), "short") == 0);

    char long_text[200];
    std::memset(long_text, 'A', sizeof(long_text));
    long_text[sizeof(long_text) - 1] = '\0';
    err.set_detail(long_text);
    CHECK(err.detail[err.detail.size() - 1] == '\0');
    CHECK(std::strlen(err.detail.data()) == err.detail.size() - 1);
    for (size_t i = 0; i < err.detail.size() - 1; ++i) {
        CHECK(err.detail[i] == 'A');
    }

    err.set_detail(nullptr);
    CHECK(err.detail[0] == '\0');
    err.set_detail("");
    CHECK(err.detail[0] == '\0');
    return 0;
}

static int test_tag_info_uid_equals() {
    TagInfo tag{};
    tag.uid = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x80, 0, 0, 0};
    tag.uid_length = 7;

    uint8_t same[7] = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x80};
    uint8_t diff[7] = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x81};
    uint8_t short_uid[4] = {0x04, 0x91, 0xD4, 0x4C};

    CHECK(tag.uid_equals(same, 7));
    CHECK(!tag.uid_equals(diff, 7));
    CHECK(!tag.uid_equals(same, 4));
    CHECK(!tag.uid_equals(short_uid, 4));
    CHECK(!tag.uid_equals(same, 0));
    CHECK(!tag.uid_equals(nullptr, 7));

    TagInfo empty{};
    CHECK(empty.uid_length == 0);
    CHECK(!empty.uid_equals(same, 7));
    return 0;
}

static int test_tag_info_defaults() {
    TagInfo tag{};
    CHECK(tag.uid_length == 0);
    CHECK(tag.atqa == 0);
    CHECK(tag.sak == 0);
    CHECK(tag.family == TagFamily::Unknown);
    CHECK(tag.user_bytes == 0);
    CHECK(tag.supports_ndef == false);
    CHECK(tag.nfc_forum_tag_type == 0);
    return 0;
}

static int test_version_accessors() {
    CHECK(std::strcmp(version(), "0.1.0-dev") == 0);
    CHECK(version_major() == 0);
    CHECK(version_minor() == 1);
    CHECK(version_patch() == 0);
    CHECK(std::strcmp(version_suffix(), "-dev") == 0);
    return 0;
}

int main() {
    int result = 0;
    result |= test_name_helpers();
    result |= test_error_set_detail_truncates_and_terminates();
    result |= test_tag_info_uid_equals();
    result |= test_tag_info_defaults();
    result |= test_version_accessors();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
