// SPDX-License-Identifier: MIT
//
// gogolem::nfc Engine implementation — wraps the pinned M5Unit-NFC protocol
// layer. Faithfully ports the proven 0117 begin/scan sequence: UnitUnified
// manages the UnitNFC; NFCLayerA performs detect. No printing, no NVS, no
// reboot, no bus creation — the caller owns the bus.

#include <cstring>

#include "gogolem/nfc/engine.hpp"
#include "gogolem/nfc/lifecycle.hpp"
#include "gogolem/nfc/picc_map.hpp"

#include "M5UnitUnified.h"
#include "M5UnitUnifiedNFC.hpp"

namespace gogolem::nfc {

// Build-time guard that the mirrored upstream Type ordinals still match the
// pinned M5Unit-NFC revision. If this fails after a dependency upgrade, the
// picc_type:: constants in picc_map.hpp must be re-synced.
static_assert(static_cast<uint8_t>(m5::nfc::a::Type::NTAG_215) == picc_type::Ntag215,
              "mirrored NTAG_215 ordinal drifted from upstream");
static_assert(static_cast<uint8_t>(m5::nfc::a::Type::MIFARE_Classic_4K) == picc_type::MifareClassic4K,
              "mirrored MIFARE_Classic_4K ordinal drifted from upstream");

namespace {
using PICC = m5::nfc::a::PICC;

PiccFields picc_to_fields(const PICC& p) {
    PiccFields f{};
    uint8_t n = p.size <= 10 ? p.size : 10;
    f.uid_size = n;
    if (n > 0) std::memcpy(f.uid.data(), p.uid, n);
    f.atqa = p.atqa;
    f.sak = p.sak;
    f.blocks = p.blocks;
    f.unit_size = p.unitSize();
    f.user_area = p.userAreaSize();
    f.total_size = p.totalSize();
    f.first_user_block = p.firstUserBlock();
    f.last_user_block = p.lastUserBlock();
    f.supports_ndef = p.supportsNDEF();
    f.forum_tag_type = static_cast<uint8_t>(p.nfcForumTagType());
    f.type_code = static_cast<uint8_t>(p.type);
    return f;
}
}  // namespace

struct Engine::Impl {
    m5::unit::UnitUnified units{};
    m5::unit::UnitNFC unit{};
    m5::nfc::NFCLayerA reader{unit};
    LifecycleState state{LifecycleState::New};
    Mode mode{Mode::Reader};
    uint8_t address{0x50};
    bool bus_attached{false};
    bool ever_began{false};  // upstream UnitUnified cannot re-begin after end
};

Engine::Engine() : impl_(std::make_unique<Impl>()) {}
Engine::~Engine() = default;

Result<void> Engine::begin(const EngineConfig& cfg) {
    if (!cfg.bus) {
        Error e;
        e.layer = ErrorLayer::Argument;
        e.operation = Operation::Begin;
        e.set_detail("null bus");
        return Result<void>::failure(e);
    }
    auto after = lifecycle_after_begin(cfg.mode, impl_->state);
    if (!after.ok()) {
        return Result<void>::failure(after.error());
    }
    // The pinned M5Unit-NFC revision does not support re-begin on the same
    // UnitUnified/UnitNFC instance after end(): units_.add()/begin() fails the
    // second time. The Engine is therefore initialize-once; construct a new
    // Engine (and re-create the bus if needed) to run again.
    if (impl_->ever_began) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.esp_code = ESP_CODE_ERR_INVALID_STATE;
        e.operation = Operation::Begin;
        e.set_detail("initialize-once; construct a new Engine");
        return Result<void>::failure(e);
    }

    impl_->mode = cfg.mode;
    impl_->address = cfg.i2c_address;
    impl_->state = LifecycleState::Initializing;

    // Faithful port of 0117 begin(): configure NFC-A and emulation flag, then
    // attach to the caller-owned bus and begin the manager.
    auto config = impl_->unit.config();
    config.mode = m5::nfc::NFC::A;
    config.emulation = (cfg.mode != Mode::Reader);
    impl_->unit.config(config);

    if (!impl_->units.add(impl_->unit, cfg.bus) || !impl_->units.begin()) {
        impl_->state = lifecycle_after_fault(impl_->state);
        Error e;
        e.layer = ErrorLayer::ChipState;
        e.operation = Operation::Begin;
        e.set_detail("M5Unit-NFC init failed");
        return Result<void>::failure(e);
    }
    impl_->bus_attached = true;
    impl_->ever_began = true;
    impl_->state = after.value();
    return Result<void>::success();
}

Result<void> Engine::end() {
    auto after = lifecycle_after_end(impl_->state);
    if (!after.ok()) {
        return Result<void>::failure(after.error());
    }
    impl_->state = LifecycleState::Stopping;
    // Version one does not promise upstream RF teardown; the bus remains owned
    // by the caller. Transition to Stopped so a new begin() is legal.
    impl_->bus_attached = false;
    impl_->state = after.value();
    return Result<void>::success();
}

LifecycleState Engine::state() const { return impl_->state; }
Mode Engine::mode() const { return impl_->mode; }

Result<ScanResult> Engine::scan(uint32_t timeout_ms) {
    if (!lifecycle_is_ready(impl_->state)) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::Scan;
        e.set_detail("not ready");
        return Result<ScanResult>::failure(e);
    }
    if (impl_->mode != Mode::Reader) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::Scan;
        e.set_detail("not reader mode");
        return Result<ScanResult>::failure(e);
    }

    impl_->units.update(true);
    std::vector<PICC> piccs;
    const bool detected = impl_->reader.detect(piccs, timeout_ms);
    ScanResult out;
    if (detected) {
        for (auto& picc : piccs) {
            if (impl_->reader.identify(picc)) {
                out.tags.push_back(to_tag_info(picc_to_fields(picc)));
            }
        }
        // Enumeration leaves PICCs in HALT; deactivate to return the reader
        // to a usable state for the next operation.
        impl_->reader.deactivate();
    }
    // No card in the field is a valid no-tag outcome: success with empty tags.
    return Result<ScanResult>::success(std::move(out));
}

Result<ActivationResult> Engine::activate_one() {
    if (!lifecycle_is_ready(impl_->state)) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::Activate;
        e.set_detail("not ready");
        return Result<ActivationResult>::failure(e);
    }
    if (impl_->mode != Mode::Reader) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::Activate;
        e.set_detail("not reader mode");
        return Result<ActivationResult>::failure(e);
    }

    impl_->units.update(true);

    PICC picc{};
    // Faithful port of 0117 activate_one(): REQA first for an IDLE tag, then
    // WUPA so consecutive commands work on a HALT tag without moving it.
    ActivationSource source = ActivationSource::REQA;
    if (!impl_->reader.request(picc.atqa)) {
        if (!impl_->reader.wakeup(picc.atqa)) {
            Error e;
            e.layer = ErrorLayer::Rf;
            e.esp_code = ESP_CODE_ERR_NOT_FOUND;
            e.operation = Operation::Activate;
            e.set_detail("no tag answered REQA or WUPA");
            return Result<ActivationResult>::failure(e);
        }
        source = ActivationSource::WUPA;
    }
    if (!impl_->reader.select(picc)) {
        Error e;
        e.layer = ErrorLayer::Activation;
        e.operation = Operation::Activate;
        e.set_detail("select failed");
        return Result<ActivationResult>::failure(e);
    }
    if (!impl_->reader.identify(picc)) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::Protocol;
        e.operation = Operation::Identify;
        e.set_detail("identify failed");
        return Result<ActivationResult>::failure(e);
    }
    if (!impl_->reader.reactivate(picc)) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::Activation;
        e.operation = Operation::Activate;
        e.set_detail("reactivate failed");
        return Result<ActivationResult>::failure(e);
    }

    ActivationResult out;
    out.tag = to_tag_info(picc_to_fields(picc));
    out.source = source;
    return Result<ActivationResult>::success(std::move(out));
}

Result<void> Engine::deactivate() {
    if (!lifecycle_is_ready(impl_->state)) {
        return Result<void>::success();  // nothing to deactivate
    }
    if (impl_->reader.activatedPICC().valid()) {
        impl_->reader.deactivate();
    }
    return Result<void>::success();
}

}  // namespace gogolem::nfc
