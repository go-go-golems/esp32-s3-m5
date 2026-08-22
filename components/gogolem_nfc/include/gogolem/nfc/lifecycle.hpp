// SPDX-License-Identifier: MIT
//
// gogolem::nfc lifecycle state machine for the reusable NFC component.
//
// These rules are pure functions over `LifecycleState` and `Mode`. They carry
// no hardware or library dependency so that Engine lifecycle correctness can
// be unit-tested on the host. The Engine calls these to decide whether a
// transition is legal and to produce a typed error when it is not.
//
// Rules (version one):
//   - begin() is only legal from New or Stopped.
//   - begin() moves to ReadyReader (reader mode) or ReadyTarget (target modes).
//   - end() is legal from any active or faulted state, idempotent from Stopped,
//     and rejected from New (nothing started) and Stopping (already stopping).
//   - A fault from a non-terminal state moves to Faulted; terminal states are
//     unchanged.

#pragma once

#include "gogolem/nfc/result.hpp"
#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc {

bool lifecycle_can_begin(LifecycleState current);
bool lifecycle_can_end(LifecycleState current);
bool lifecycle_is_ready(LifecycleState state);
bool lifecycle_is_terminal(LifecycleState state);

// Result of a successful begin(): the ready state implied by `mode`. Returns a
// Lifecycle-layer error if `current` is not New or Stopped.
Result<LifecycleState> lifecycle_after_begin(Mode mode, LifecycleState current);

// Result of end(): Stopped. Idempotent from Stopped. Rejected (Lifecycle error)
// from New and from Stopping.
Result<LifecycleState> lifecycle_after_end(LifecycleState current);

// Result of a fault: Faulted unless already terminal.
LifecycleState lifecycle_after_fault(LifecycleState current);

}  // namespace gogolem::nfc
