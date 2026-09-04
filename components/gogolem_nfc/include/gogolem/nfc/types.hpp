// SPDX-License-Identifier: MIT
//
// gogolem::nfc domain types for the reusable native ESP-IDF NFC component.
//
// These types carry no ESP-IDF, console, NVS, or NFC-library dependency on
// purpose. They compile under a plain host C++ compiler so that safety,
// lifecycle, and result semantics can be unit-tested without hardware. On the
// target, callers convert upstream M5Unit-NFC objects into these stable types
// at the component boundary; upstream types never leak through the public API.
//
// The integer field `Error::esp_code` mirrors an `esp_err_t` value. We store it
// as `int32_t` here to avoid pulling `esp_err.h` into a host-testable header.
// Target code passes `static_cast<int32_t>(esp_err)`; host code uses the
// constants defined below.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gogolem::nfc {

// ---- NFC device operating mode -------------------------------------------
//
// Mode is selected before initialization and is immutable while the device is
// ready in version one. Reader mode interrogates physical cards; the
// emulation modes configure the ST25R3916 as an NFC-A target.
enum class Mode : uint8_t {
    Reader = 0,
    EmulationUltralight = 1,
    EmulationNtag213 = 2,
    EmulationCustom = 3,
};

// ---- Component lifecycle --------------------------------------------------
enum class LifecycleState : uint8_t {
    New = 0,
    Initializing,
    ReadyReader,
    ReadyTarget,
    Busy,
    Stopping,
    Stopped,
    Faulted,
};

// ---- Failure classification layer ----------------------------------------
//
// A single `bool` cannot distinguish an I2C NACK from an absent tag, a legal
// collision, a bad NDEF TLV, a rejected Classic key, or a failed restoration.
// Callers branch on `ErrorLayer`; the raw upstream/ESP-IDF code is preserved
// alongside it for diagnostics.
enum class ErrorLayer : uint8_t {
    None = 0,
    Argument,
    Lifecycle,
    Policy,
    Transport,
    ChipState,
    Rf,
    Activation,
    Collision,
    Protocol,
    CardFamily,
    Authentication,
    Access,
    DataFormat,
    Capacity,
    Verification,
    Restoration,
    Internal,
};

// ---- Operation that produced an error -------------------------------------
enum class Operation : uint8_t {
    None = 0,
    Begin,
    End,
    Scan,
    Activate,
    Identify,
    Read,
    Dump,
    ReadNdef,
    WriteNdef,
    Write,
    InspectClassicValues,
    Wallet,
    StartEmulation,
    UpdateEmulation,
    StopEmulation,
};

// ---- Identified card family ----------------------------------------------
//
// The family is the programmatic contract; a human-readable name is derived
// from it by `tag_family_name()`. Family is determined from SAK, ATQA, product
// version, and memory behavior together, never from ATQA alone.
enum class TagFamily : uint8_t {
    Unknown = 0,
    MifareUltralight,
    Ntag21x,
    MifareClassic,
    MifarePlus,
    Desfire,
    St25ta,
    IsoDepOther,
    Type1Topaz,
    Type5Vicinity,
};

// ---- Mirrored ESP error codes (host-clean) -------------------------------
//
// These mirror the common `esp_err_t` values used by the NFC path. Keeping
// them here lets host tests assert error preservation without `esp_err.h`.
constexpr int32_t ESP_CODE_OK = 0;
constexpr int32_t ESP_CODE_FAIL = -1;
constexpr int32_t ESP_CODE_ERR_INVALID_ARG = 0x102;
constexpr int32_t ESP_CODE_ERR_INVALID_STATE = 0x103;
constexpr int32_t ESP_CODE_ERR_NOT_FOUND = 0x105;
constexpr int32_t ESP_CODE_ERR_TIMEOUT = 0x107;

// ---- Structured error -----------------------------------------------------
//
// `layer` and `operation` are the machine-readable classifiers. `esp_code` and
// `upstream_code` preserve the raw codes for diagnostics. `detail` is a short,
// fixed-size diagnostic string; it is context, not the classifier, so callers
// must never branch on its contents.
struct Error {
    ErrorLayer layer{ErrorLayer::None};
    int32_t esp_code{ESP_CODE_OK};
    int32_t upstream_code{0};
    Operation operation{Operation::None};
    uint8_t address{0};
    bool retryable{false};
    std::array<char, 96> detail{};

    void set_detail(const char* text);
    bool is_none() const { return layer == ErrorLayer::None; }
};

// ---- Tag identity and memory geometry ------------------------------------
struct TagInfo {
    std::array<uint8_t, 10> uid{};
    uint8_t uid_length{0};
    uint16_t atqa{0};
    uint8_t sak{0};
    TagFamily family{TagFamily::Unknown};
    uint16_t block_or_page_count{0};
    uint16_t unit_size{0};          // bytes per block or page
    uint32_t user_bytes{0};
    uint32_t total_bytes{0};
    uint16_t first_user_unit{0};
    uint16_t last_user_unit{0};
    bool supports_ndef{false};
    uint8_t nfc_forum_tag_type{0};

    bool uid_equals(const uint8_t* bytes, uint8_t length) const;
};

// ---- Name helpers ---------------------------------------------------------
const char* mode_name(Mode mode);
const char* lifecycle_state_name(LifecycleState state);
const char* error_layer_name(ErrorLayer layer);
const char* operation_name(Operation op);
const char* tag_family_name(TagFamily family);

}  // namespace gogolem::nfc
