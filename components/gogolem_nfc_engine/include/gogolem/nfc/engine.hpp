// SPDX-License-Identifier: MIT
//
// gogolem::nfc synchronous Engine — wraps the pinned M5Unit-NFC protocol layer
// behind a stable, board-independent API. The application owns the ESP-IDF
// I²C bus; the Engine owns NFC protocol state.
//
// This header is deliberately free of M5Unit-NFC includes (pimpl) so consumers
// do not compile against the upstream library. The Engine is NOT thread-safe;
// one task calls it at a time. Use the Service (later phase) for multi-task
// applications.

#pragma once

#include <memory>
#include <vector>

#include "driver/i2c_master.h"

#include "gogolem/nfc/ndef.hpp"
#include "gogolem/nfc/result.hpp"
#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc {

// Target emulation state, mirroring upstream EmulationLayerA::State.
enum class EmulationState : uint8_t {
    None = 0,
    Off,
    Idle,
    Ready,
    Active,
    Halt,
};

// Caller-provided emulation profile. The Engine copies memory into
// internal storage during begin(); the caller may release it after.
struct EmulationProfile {
    TagFamily family{TagFamily::Unknown};  // MifareUltralight or Ntag21x
    std::array<uint8_t, 10> uid{};
    uint8_t uid_length{0};
    std::vector<uint8_t> memory;
};

struct EngineConfig {
    i2c_master_bus_handle_t bus{};   // caller-owned; must outlive the Engine
    uint8_t i2c_address{0x50};       // ST25R3916 default; applied when supported
    Mode mode{Mode::Reader};          // selected before begin(); immutable while ready
    // Used when mode is an emulation mode. The Engine copies the memory image
    // during begin(); the caller may release it after begin() returns.
    EmulationProfile emulation_profile{};
};

struct ScanResult {
    std::vector<TagInfo> tags;       // identified cards; empty when no card found
};

enum class ActivationSource : uint8_t {
    REQA = 0,
    WUPA = 1,
};

struct ActivationResult {
    TagInfo tag;
    ActivationSource source;        // REQA for an IDLE tag, WUPA for a HALT tag
};

class Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Attach to the caller-owned bus and initialize the ST25R3916. The caller
    // must keep the bus valid until end() returns.
    Result<void> begin(const EngineConfig& cfg);

    // Transition to Stopped. Version one does not promise upstream RF teardown;
    // mode switching requires a new Engine after re-creating the bus/reboot.
    Result<void> end();

    LifecycleState state() const;
    Mode mode() const;

    // Multi-card scan. With no card in the field, returns success with an empty
    // tag list (this is a valid no-tag outcome, not an error).
    Result<ScanResult> scan(uint32_t timeout_ms);

    // Single-card activation with REQA→WUPA fallback. Enumeration by scan()
    // leaves a stationary PICC in HALT; activate_one() tries REQA first, then
    // WUPA, so consecutive commands work without moving the tag. After a
    // successful activate_one(), the tag is selected and identified; call
    // deactivate() when done. Returns Rf/Activation error if no tag answers.
    Result<ActivationResult> activate_one();

    // Deactivate the currently selected tag (HLTA). Safe to call when no tag
    // is active.
    Result<void> deactivate();

    // Read 16 bytes (4 Type 2 pages or 1 Classic block) at the given address.
    // Self-activates and deactivates. For Classic, uses default Key A.
    Result<std::vector<uint8_t>> raw_read(uint8_t address);

    // Read and parse NDEF from the tag. Returns success with a (possibly empty)
    // NdefMessage when the tag is NDEF-formatted. Returns a CardFamily error if
    // the tag does not support NDEF, or a DataFormat error if the format is
    // invalid.
    Result<NdefMessage> read_ndef();

    // Dump the entire card through the upstream library. Returns success when
    // the dump completes. (A sink-based API will replace this in a later phase.)
    Result<void> dump();

    // Target emulation: start with the profile from EngineConfig, update in a
    // loop, and query state. These are only valid when mode is an emulation
    // mode.
    Result<void> start_emulation(const EmulationProfile& profile);
    EmulationState update_emulation();  // call in a tight loop (~1ms)
    EmulationState emulation_state() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace gogolem::nfc
