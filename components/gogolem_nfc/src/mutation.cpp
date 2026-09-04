// SPDX-License-Identifier: MIT

#include "gogolem/nfc/mutation.hpp"

namespace gogolem::nfc {

bool permit_allows(const MutationPermit& permit, MutationKind kind, const TagInfo& tag) {
    if (kind == MutationKind::None) {
        return false;
    }
    if (permit.allowed != kind) {
        return false;
    }
    return tag.uid_equals(permit.expected_uid.data(), permit.expected_uid_length);
}

bool write_report_ok(const WriteReport& r) {
    if (!r.write_attempted || !r.write_succeeded) {
        return false;
    }
    if (!r.verification_attempted || !r.verification_succeeded) {
        return false;
    }
    if (r.restoration_required) {
        if (!r.restoration_attempted || !r.restoration_succeeded) {
            return false;
        }
    }
    return true;
}

ErrorLayer write_report_primary_failure(const WriteReport& r) {
    if (r.first_error.layer != ErrorLayer::None) {
        return r.first_error.layer;
    }
    if (r.write_attempted && !r.write_succeeded) {
        return ErrorLayer::Transport;
    }
    if (r.verification_attempted && !r.verification_succeeded) {
        return ErrorLayer::Verification;
    }
    if (r.restoration_required && r.restoration_attempted && !r.restoration_succeeded) {
        return ErrorLayer::Restoration;
    }
    if (r.restoration_required && !r.restoration_attempted) {
        return ErrorLayer::Restoration;
    }
    return ErrorLayer::None;
}

}  // namespace gogolem::nfc
