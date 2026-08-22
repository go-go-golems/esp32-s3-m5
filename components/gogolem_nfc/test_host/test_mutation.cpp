// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc mutation permits and write reports.

#include <cstdio>
#include <cstring>

#include "gogolem/nfc/mutation.hpp"
#include "gogolem/nfc/types.hpp"

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

static TagInfo make_tag(const uint8_t* uid, uint8_t len) {
    TagInfo t{};
    for (uint8_t i = 0; i < len; ++i) t.uid[i] = uid[i];
    t.uid_length = len;
    return t;
}

static int test_permit_allows_uid_and_kind() {
    uint8_t uid[7] = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x80};
    TagInfo tag = make_tag(uid, 7);

    MutationPermit permit{};
    permit.expected_uid = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x80, 0, 0, 0};
    permit.expected_uid_length = 7;
    permit.allowed = MutationKind::ReversibleWrite;

    CHECK(permit_allows(permit, MutationKind::ReversibleWrite, tag));

    // Wrong kind rejected even with matching UID.
    CHECK(!permit_allows(permit, MutationKind::ReplaceNdef, tag));
    // None rejected.
    CHECK(!permit_allows(permit, MutationKind::None, tag));
    // Wrong UID rejected.
    uint8_t other[7] = {0x04, 0x91, 0xD4, 0x4C, 0x9E, 0x61, 0x81};
    TagInfo other_tag = make_tag(other, 7);
    CHECK(!permit_allows(permit, MutationKind::ReversibleWrite, other_tag));
    // Length mismatch rejected.
    MutationPermit short_permit = permit;
    short_permit.expected_uid_length = 4;
    CHECK(!permit_allows(short_permit, MutationKind::ReversibleWrite, tag));
    // allowed=None rejected.
    MutationPermit none_permit = permit;
    none_permit.allowed = MutationKind::None;
    CHECK(!permit_allows(none_permit, MutationKind::ReversibleWrite, tag));
    return 0;
}

static int test_write_report_ok_full_success() {
    WriteReport r;
    r.write_attempted = r.write_succeeded = true;
    r.verification_attempted = r.verification_succeeded = true;
    r.restoration_required = false;
    CHECK(write_report_ok(r));
    return 0;
}

static int test_write_report_ok_with_restoration() {
    WriteReport r;
    r.write_attempted = r.write_succeeded = true;
    r.verification_attempted = r.verification_succeeded = true;
    r.restoration_required = true;
    r.restoration_attempted = r.restoration_succeeded = true;
    CHECK(write_report_ok(r));
    return 0;
}

static int test_write_report_not_ok_verification_failed_restoration_succeeded() {
    // Write OK, verification failed, restoration succeeded: tag may be clean,
    // but the operation is not a success because verification failed.
    WriteReport r;
    r.write_attempted = r.write_succeeded = true;
    r.verification_attempted = true;
    r.verification_succeeded = false;
    r.restoration_required = true;
    r.restoration_attempted = r.restoration_succeeded = true;
    CHECK(!write_report_ok(r));
    CHECK(write_report_primary_failure(r) == ErrorLayer::Verification);
    return 0;
}

static int test_write_report_restoration_failure_is_high_severity() {
    WriteReport r;
    r.write_attempted = r.write_succeeded = true;
    r.verification_attempted = r.verification_succeeded = true;
    r.restoration_required = true;
    r.restoration_attempted = true;
    r.restoration_succeeded = false;
    CHECK(!write_report_ok(r));
    // first_error unset, so primary failure is derived from flags.
    CHECK(write_report_primary_failure(r) == ErrorLayer::Restoration);
    return 0;
}

static int test_write_report_restoration_not_attempted() {
    WriteReport r;
    r.write_attempted = r.write_succeeded = true;
    r.verification_attempted = r.verification_succeeded = true;
    r.restoration_required = true;
    r.restoration_attempted = false;
    CHECK(!write_report_ok(r));
    CHECK(write_report_primary_failure(r) == ErrorLayer::Restoration);
    return 0;
}

static int test_write_report_prefers_recorded_first_error() {
    WriteReport r;
    r.write_attempted = true;
    r.write_succeeded = false;
    r.first_error.layer = ErrorLayer::Transport;
    r.first_error.esp_code = ESP_CODE_ERR_NOT_FOUND;
    CHECK(!write_report_ok(r));
    CHECK(write_report_primary_failure(r) == ErrorLayer::Transport);
    return 0;
}

static int test_write_report_nothing_attempted() {
    WriteReport r;
    CHECK(!write_report_ok(r));
    CHECK(write_report_primary_failure(r) == ErrorLayer::None);
    return 0;
}

int main() {
    int result = 0;
    result |= test_permit_allows_uid_and_kind();
    result |= test_write_report_ok_full_success();
    result |= test_write_report_ok_with_restoration();
    result |= test_write_report_not_ok_verification_failed_restoration_succeeded();
    result |= test_write_report_restoration_failure_is_high_severity();
    result |= test_write_report_restoration_not_attempted();
    result |= test_write_report_prefers_recorded_first_error();
    result |= test_write_report_nothing_attempted();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
