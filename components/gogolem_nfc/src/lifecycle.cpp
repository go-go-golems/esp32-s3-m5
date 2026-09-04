// SPDX-License-Identifier: MIT

#include "gogolem/nfc/lifecycle.hpp"

namespace gogolem::nfc {

bool lifecycle_can_begin(LifecycleState current) {
    return current == LifecycleState::New || current == LifecycleState::Stopped;
}

bool lifecycle_can_end(LifecycleState current) {
    return current == LifecycleState::Initializing ||
           current == LifecycleState::ReadyReader ||
           current == LifecycleState::ReadyTarget ||
           current == LifecycleState::Busy ||
           current == LifecycleState::Faulted ||
           current == LifecycleState::Stopped;
}

bool lifecycle_is_ready(LifecycleState state) {
    return state == LifecycleState::ReadyReader ||
           state == LifecycleState::ReadyTarget;
}

bool lifecycle_is_terminal(LifecycleState state) {
    return state == LifecycleState::Stopped || state == LifecycleState::Faulted;
}

Result<LifecycleState> lifecycle_after_begin(Mode mode, LifecycleState current) {
    if (!lifecycle_can_begin(current)) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.esp_code = ESP_CODE_ERR_INVALID_STATE;
        e.operation = Operation::Begin;
        e.set_detail("begin requires New or Stopped");
        return Result<LifecycleState>::failure(e);
    }
    switch (mode) {
        case Mode::Reader:
            return Result<LifecycleState>::success(LifecycleState::ReadyReader);
        case Mode::EmulationUltralight:
        case Mode::EmulationNtag213:
        case Mode::EmulationCustom:
            return Result<LifecycleState>::success(LifecycleState::ReadyTarget);
        default: {
            Error e;
            e.layer = ErrorLayer::Argument;
            e.esp_code = ESP_CODE_ERR_INVALID_ARG;
            e.operation = Operation::Begin;
            e.set_detail("unknown mode");
            return Result<LifecycleState>::failure(e);
        }
    }
}

Result<LifecycleState> lifecycle_after_end(LifecycleState current) {
    if (current == LifecycleState::New) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.esp_code = ESP_CODE_ERR_INVALID_STATE;
        e.operation = Operation::End;
        e.set_detail("end from New: nothing started");
        return Result<LifecycleState>::failure(e);
    }
    if (current == LifecycleState::Stopping) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.esp_code = ESP_CODE_ERR_INVALID_STATE;
        e.operation = Operation::End;
        e.set_detail("end from Stopping: already stopping");
        return Result<LifecycleState>::failure(e);
    }
    return Result<LifecycleState>::success(LifecycleState::Stopped);
}

LifecycleState lifecycle_after_fault(LifecycleState current) {
    if (lifecycle_is_terminal(current)) {
        return current;
    }
    return LifecycleState::Faulted;
}

}  // namespace gogolem::nfc
