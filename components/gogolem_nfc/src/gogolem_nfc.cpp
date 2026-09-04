// SPDX-License-Identifier: MIT
//
// gogolem_nfc implementation for Phase 1: domain-type helpers and version.
// Higher phases add the Engine, Service, NDEF, Classic, and emulation on top
// of the pinned M5Unit-NFC component; this file intentionally stays free of
// any ESP-IDF, NFC-library, console, or NVS dependency.

#include <cstring>

#include "gogolem/nfc/types.hpp"
#include "gogolem/nfc/version.hpp"

namespace gogolem::nfc {

// ---- Error::set_detail ---------------------------------------------------
void Error::set_detail(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        detail[0] = '\0';
        return;
    }
    // Truncate to fit; always NUL-terminate.
    size_t i = 0;
    for (; i + 1 < detail.size() && text[i] != '\0'; ++i) {
        detail[i] = text[i];
    }
    detail[i] = '\0';
}

// ---- TagInfo::uid_equals --------------------------------------------------
bool TagInfo::uid_equals(const uint8_t* bytes, uint8_t length) const {
    if (bytes == nullptr || length != uid_length || uid_length == 0) {
        return false;
    }
    return std::memcmp(uid.data(), bytes, uid_length) == 0;
}

// ---- Name helpers ---------------------------------------------------------
const char* mode_name(Mode mode) {
    switch (mode) {
        case Mode::Reader: return "reader";
        case Mode::EmulationUltralight: return "emulation-ultralight";
        case Mode::EmulationNtag213: return "emulation-ntag213";
        case Mode::EmulationCustom: return "emulation-custom";
        default: return "unknown-mode";
    }
}

const char* lifecycle_state_name(LifecycleState state) {
    switch (state) {
        case LifecycleState::New: return "new";
        case LifecycleState::Initializing: return "initializing";
        case LifecycleState::ReadyReader: return "ready-reader";
        case LifecycleState::ReadyTarget: return "ready-target";
        case LifecycleState::Busy: return "busy";
        case LifecycleState::Stopping: return "stopping";
        case LifecycleState::Stopped: return "stopped";
        case LifecycleState::Faulted: return "faulted";
        default: return "unknown-lifecycle";
    }
}

const char* error_layer_name(ErrorLayer layer) {
    switch (layer) {
        case ErrorLayer::None: return "none";
        case ErrorLayer::Argument: return "argument";
        case ErrorLayer::Lifecycle: return "lifecycle";
        case ErrorLayer::Policy: return "policy";
        case ErrorLayer::Transport: return "transport";
        case ErrorLayer::ChipState: return "chip-state";
        case ErrorLayer::Rf: return "rf";
        case ErrorLayer::Activation: return "activation";
        case ErrorLayer::Collision: return "collision";
        case ErrorLayer::Protocol: return "protocol";
        case ErrorLayer::CardFamily: return "card-family";
        case ErrorLayer::Authentication: return "authentication";
        case ErrorLayer::Access: return "access";
        case ErrorLayer::DataFormat: return "data-format";
        case ErrorLayer::Capacity: return "capacity";
        case ErrorLayer::Verification: return "verification";
        case ErrorLayer::Restoration: return "restoration";
        case ErrorLayer::Internal: return "internal";
        default: return "unknown-layer";
    }
}

const char* operation_name(Operation op) {
    switch (op) {
        case Operation::None: return "none";
        case Operation::Begin: return "begin";
        case Operation::End: return "end";
        case Operation::Scan: return "scan";
        case Operation::Activate: return "activate";
        case Operation::Identify: return "identify";
        case Operation::Read: return "read";
        case Operation::Dump: return "dump";
        case Operation::ReadNdef: return "read-ndef";
        case Operation::WriteNdef: return "write-ndef";
        case Operation::Write: return "write";
        case Operation::InspectClassicValues: return "inspect-classic-values";
        case Operation::Wallet: return "wallet";
        case Operation::StartEmulation: return "start-emulation";
        case Operation::UpdateEmulation: return "update-emulation";
        case Operation::StopEmulation: return "stop-emulation";
        default: return "unknown-operation";
    }
}

const char* tag_family_name(TagFamily family) {
    switch (family) {
        case TagFamily::MifareUltralight: return "MIFARE Ultralight";
        case TagFamily::Ntag21x: return "NTAG21x";
        case TagFamily::MifareClassic: return "MIFARE Classic";
        case TagFamily::MifarePlus: return "MIFARE Plus";
        case TagFamily::Desfire: return "MIFARE DESFire";
        case TagFamily::St25ta: return "ST25TA";
        case TagFamily::IsoDepOther: return "ISO-DEP other";
        case TagFamily::Type1Topaz: return "Type 1 (Topaz)";
        case TagFamily::Type5Vicinity: return "Type 5 (Vicinity)";
        case TagFamily::Unknown:
        default: return "unknown-family";
    }
}

// ---- Version --------------------------------------------------------------
const char* version() { return "0.1.0-dev"; }
unsigned version_major() { return 0; }
unsigned version_minor() { return 1; }
unsigned version_patch() { return 0; }
const char* version_suffix() { return "-dev"; }

}  // namespace gogolem::nfc
