// SPDX-License-Identifier: MIT
//
// gogolem::nfc mutation safety: UID-bound permits and write-report precedence.
//
// The Engine enforces invariant safety; the application supplies operator
// consent. A MutationPermit binds a mutation kind to the selected physical
// UID so a confirmation phrase cannot mutate the wrong tag. A WriteReport
// records every phase of a reversible write separately, because a single
// boolean cannot represent "write succeeded, verification failed, restoration
// succeeded" or "restoration failed" — both of which leave the tag changed.
//
// These are pure functions over value types, host-testable without hardware.

#pragma once

#include <cstdint>

#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc {

enum class MutationKind : uint8_t {
    None = 0,
    ReversibleWrite = 1,
    ReplaceNdef = 2,
    ClassicWallet = 3,
};

struct MutationPermit {
    std::array<uint8_t, 10> expected_uid{};
    uint8_t expected_uid_length{0};
    MutationKind allowed{MutationKind::None};
    bool require_readback{true};
    bool require_restoration{false};
};

// True only when `kind` is permitted, the kind is not None, and the selected
// tag's UID exactly matches the permit. The Engine consults this before any
// mutation; it never branches on human confirmation text.
bool permit_allows(const MutationPermit& permit, MutationKind kind, const TagInfo& tag);

struct WriteReport {
    bool write_attempted{false};
    bool write_succeeded{false};
    bool verification_attempted{false};
    bool verification_succeeded{false};
    bool restoration_required{false};
    bool restoration_attempted{false};
    bool restoration_succeeded{false};
    Error first_error{};
};

// Overall success requires a successful write, successful verification, and
// (when restoration was required) successful restoration.
bool write_report_ok(const WriteReport& report);

// The layer that best describes the first failure, preferring the recorded
// first_error when set.
ErrorLayer write_report_primary_failure(const WriteReport& report);

}  // namespace gogolem::nfc
