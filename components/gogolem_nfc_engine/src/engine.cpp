// SPDX-License-Identifier: MIT
//
// gogolem::nfc Engine implementation — wraps the pinned M5Unit-NFC protocol
// layer. Faithfully ports the proven 0117 begin/scan sequence: UnitUnified
// manages the UnitNFC; NFCLayerA performs detect. No printing, no NVS, no
// reboot, no bus creation — the caller owns the bus.

#include <cstring>

#include "gogolem/nfc/engine.hpp"
#include "gogolem/nfc/lifecycle.hpp"
#include "gogolem/nfc/mutation.hpp"
#include "gogolem/nfc/ndef.hpp"
#include "gogolem/nfc/picc_map.hpp"
#include "gogolem/nfc/safety.hpp"

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
    m5::nfc::EmulationLayerA emulation{unit};
    std::vector<uint8_t> emulation_memory{};
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

    // Start emulation if configured for a target mode.
    if (cfg.mode != Mode::Reader) {
        auto emu = start_emulation(cfg.emulation_profile);
        if (!emu.ok()) {
            impl_->state = lifecycle_after_fault(impl_->state);
            return emu;
        }
    }

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

Result<std::vector<uint8_t>> Engine::raw_read(uint8_t address) {
    if (!lifecycle_is_ready(impl_->state)) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::Read;
        e.set_detail("not ready");
        return Result<std::vector<uint8_t>>::failure(e);
    }
    impl_->units.update(true);
    PICC picc{};
    if (!impl_->reader.request(picc.atqa) && !impl_->reader.wakeup(picc.atqa)) {
        Error e;
        e.layer = ErrorLayer::Rf;
        e.esp_code = ESP_CODE_ERR_NOT_FOUND;
        e.operation = Operation::Read;
        e.set_detail("no tag");
        return Result<std::vector<uint8_t>>::failure(e);
    }
    if (!impl_->reader.select(picc) || !impl_->reader.identify(picc) || !impl_->reader.reactivate(picc)) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::Activation;
        e.operation = Operation::Read;
        e.set_detail("activation failed");
        return Result<std::vector<uint8_t>>::failure(e);
    }
    std::vector<uint8_t> data(16, 0);
    bool ok = impl_->reader.read16(data.data(), address);
    impl_->reader.deactivate();
    if (!ok) {
        Error e;
        e.layer = ErrorLayer::Protocol;
        e.operation = Operation::Read;
        e.set_detail("read16 failed");
        return Result<std::vector<uint8_t>>::failure(e);
    }
    return Result<std::vector<uint8_t>>::success(std::move(data));
}

Result<NdefMessage> Engine::read_ndef() {
    if (!lifecycle_is_ready(impl_->state)) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::ReadNdef;
        e.set_detail("not ready");
        return Result<NdefMessage>::failure(e);
    }
    impl_->units.update(true);
    PICC picc{};
    if (!impl_->reader.request(picc.atqa) && !impl_->reader.wakeup(picc.atqa)) {
        Error e;
        e.layer = ErrorLayer::Rf;
        e.esp_code = ESP_CODE_ERR_NOT_FOUND;
        e.operation = Operation::ReadNdef;
        e.set_detail("no tag");
        return Result<NdefMessage>::failure(e);
    }
    if (!impl_->reader.select(picc) || !impl_->reader.identify(picc) || !impl_->reader.reactivate(picc)) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::Activation;
        e.operation = Operation::ReadNdef;
        e.set_detail("activation failed");
        return Result<NdefMessage>::failure(e);
    }
    if (!picc.supportsNDEF()) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::CardFamily;
        e.operation = Operation::ReadNdef;
        e.set_detail("tag does not support NDEF");
        return Result<NdefMessage>::failure(e);
    }
    bool valid = false;
    if (!impl_->reader.ndefIsValidFormat(valid) || !valid) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::DataFormat;
        e.operation = Operation::ReadNdef;
        e.set_detail(valid ? "format query failed" : "invalid NDEF format");
        return Result<NdefMessage>::failure(e);
    }
    m5::nfc::ndef::TLV tlv{};
    if (!impl_->reader.ndefRead(tlv)) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::DataFormat;
        e.operation = Operation::ReadNdef;
        e.set_detail("ndefRead failed");
        return Result<NdefMessage>::failure(e);
    }
    impl_->reader.deactivate();

    // Convert upstream TLV records to the public NdefMessage.
    NdefMessage msg;
    if (tlv.isMessageTLV()) {
        for (const auto& rec : tlv.records()) {
            NdefRecord r;
            r.tnf = static_cast<NdefTnf>(static_cast<uint8_t>(rec.tnf()));
            const char* type = rec.type();
            if (type) r.type.assign(type, type + std::strlen(type));
            r.payload.assign(rec.payload(), rec.payload() + rec.payloadSize());
            const uint8_t* id = rec.identifier();
            if (id && rec.identifierSize() > 0) r.id.assign(id, id + rec.identifierSize());
            msg.records.push_back(std::move(r));
        }
    }
    // Empty valid NDEF (zero records) is success with an empty message.
    return Result<NdefMessage>::success(std::move(msg));
}

Result<WriteReport> Engine::reversible_write(uint8_t address, const MutationPermit& permit) {
    WriteReport report{};
    if (!lifecycle_is_ready(impl_->state)) {
        report.first_error.layer = ErrorLayer::Lifecycle;
        report.first_error.operation = Operation::Write;
        report.first_error.set_detail("not ready");
        return Result<WriteReport>::failure(report.first_error);
    }
    impl_->units.update(true);
    PICC picc{};
    if (!impl_->reader.request(picc.atqa) && !impl_->reader.wakeup(picc.atqa)) {
        report.first_error.layer = ErrorLayer::Rf;
        report.first_error.esp_code = ESP_CODE_ERR_NOT_FOUND;
        report.first_error.operation = Operation::Write;
        report.first_error.set_detail("no tag");
        return Result<WriteReport>::failure(report.first_error);
    }
    if (!impl_->reader.select(picc) || !impl_->reader.identify(picc) || !impl_->reader.reactivate(picc)) {
        impl_->reader.deactivate();
        report.first_error.layer = ErrorLayer::Activation;
        report.first_error.operation = Operation::Write;
        report.first_error.set_detail("activation failed");
        return Result<WriteReport>::failure(report.first_error);
    }

    // Validate permit against the selected tag.
    TagInfo tag = to_tag_info(picc_to_fields(picc));
    if (!permit_allows(permit, MutationKind::ReversibleWrite, tag)) {
        impl_->reader.deactivate();
        report.first_error.layer = ErrorLayer::Policy;
        report.first_error.operation = Operation::Write;
        report.first_error.set_detail("permit denied (UID or kind mismatch)");
        return Result<WriteReport>::failure(report.first_error);
    }

    // Safety gate: reject protected regions.
    if (!is_safe_write_target(tag.family, address, tag.first_user_unit, tag.last_user_unit,
                              false, 0)) {
        impl_->reader.deactivate();
        report.first_error.layer = ErrorLayer::Policy;
        report.first_error.operation = Operation::Write;
        report.first_error.set_detail("protected address rejected");
        return Result<WriteReport>::failure(report.first_error);
    }

    // Type 2 write: 4-byte page.
    const uint8_t width = 4;
    uint8_t original[16]{};
    uint8_t pattern[4]{};
    uint8_t verify[16]{};

    // Read original.
    if (!impl_->reader.read16(original, address)) {
        impl_->reader.deactivate();
        report.first_error.layer = ErrorLayer::Protocol;
        report.first_error.operation = Operation::Read;
        report.first_error.set_detail("original read failed");
        return Result<WriteReport>::failure(report.first_error);
    }

    // Test pattern (distinct, recoverable).
    pattern[0] = 0xD1; pattern[1] = 0xA6; pattern[2] = address; pattern[3] = 0x5A;

    // Write test pattern.
    report.write_attempted = true;
    report.write_succeeded = impl_->reader.write4(address, pattern, width);

    // Verify.
    report.verification_attempted = true;
    if (report.write_succeeded) {
        if (impl_->reader.read16(verify, address)) {
            report.verification_succeeded = (std::memcmp(pattern, verify, width) == 0);
        }
    }

    // Restore original regardless of verification result.
    report.restoration_required = true;
    report.restoration_attempted = impl_->reader.write4(address, original, width);
    if (report.restoration_attempted) {
        std::memset(verify, 0, sizeof(verify));
        if (impl_->reader.read16(verify, address)) {
            report.restoration_succeeded = (std::memcmp(original, verify, width) == 0);
        }
    }

    impl_->reader.deactivate();

    if (!write_report_ok(report)) {
        if (report.first_error.layer == ErrorLayer::None) {
            report.first_error.layer = write_report_primary_failure(report);
            report.first_error.operation = Operation::Write;
        }
        return Result<WriteReport>::failure(report.first_error);
    }
    return Result<WriteReport>::success(std::move(report));
}

Result<void> Engine::dump() {
    if (!lifecycle_is_ready(impl_->state)) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::Dump;
        e.set_detail("not ready");
        return Result<void>::failure(e);
    }
    impl_->units.update(true);
    PICC picc{};
    if (!impl_->reader.request(picc.atqa) && !impl_->reader.wakeup(picc.atqa)) {
        Error e;
        e.layer = ErrorLayer::Rf;
        e.esp_code = ESP_CODE_ERR_NOT_FOUND;
        e.operation = Operation::Dump;
        e.set_detail("no tag");
        return Result<void>::failure(e);
    }
    if (!impl_->reader.select(picc) || !impl_->reader.identify(picc) || !impl_->reader.reactivate(picc)) {
        impl_->reader.deactivate();
        Error e;
        e.layer = ErrorLayer::Activation;
        e.operation = Operation::Dump;
        e.set_detail("activation failed");
        return Result<void>::failure(e);
    }
    bool ok = impl_->reader.dump();
    impl_->reader.deactivate();
    if (!ok) {
        Error e;
        e.layer = ErrorLayer::Protocol;
        e.operation = Operation::Dump;
        e.set_detail("dump failed");
        return Result<void>::failure(e);
    }
    return Result<void>::success();
}

// ---- Target emulation ----------------------------------------------------

// Map the public EmulationState to the upstream EmulationLayerA::State.
static EmulationState to_emulation_state(m5::nfc::EmulationLayerA::State s) {
    using S = m5::nfc::EmulationLayerA::State;
    switch (s) {
        case S::None:  return EmulationState::None;
        case S::Off:   return EmulationState::Off;
        case S::Idle:  return EmulationState::Idle;
        case S::Ready: return EmulationState::Ready;
        case S::Active:return EmulationState::Active;
        case S::Halt:  return EmulationState::Halt;
        default:       return EmulationState::None;
    }
}

// Compute BCC for UID blocks (same as 0117 bcc8).
static uint8_t bcc8(const uint8_t* data, size_t len, uint8_t initial = 0) {
    uint8_t result = initial;
    for (size_t i = 0; i < len; ++i) result ^= data[i];
    return result;
}

// Embed a 7-byte UID into a Type 2 memory image (same as 0117 embed_uid).
static void embed_uid(std::vector<uint8_t>& memory, const uint8_t* uid, uint8_t uid_len) {
    if (uid_len != 7 || memory.size() < 9) return;
    std::memcpy(&memory[0], uid, 3);
    memory[3] = bcc8(uid, 3, 0x88);
    std::memcpy(&memory[4], uid + 3, 4);
    memory[8] = bcc8(uid + 3, 4);
}

Result<void> Engine::start_emulation(const EmulationProfile& profile) {
    if (impl_->mode == Mode::Reader) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::StartEmulation;
        e.set_detail("not emulation mode");
        return Result<void>::failure(e);
    }

    // Map family to upstream Type.
    m5::nfc::a::Type type = m5::nfc::a::Type::Unknown;
    if (profile.family == TagFamily::MifareUltralight) {
        type = m5::nfc::a::Type::MIFARE_Ultralight;
    } else if (profile.family == TagFamily::Ntag21x) {
        type = m5::nfc::a::Type::NTAG_213;
    } else {
        Error e;
        e.layer = ErrorLayer::Argument;
        e.operation = Operation::StartEmulation;
        e.set_detail("unsupported emulation family");
        return Result<void>::failure(e);
    }

    // Copy and embed UID into memory (faithful port of 0117 start_emulation).
    impl_->emulation_memory.assign(profile.memory.begin(), profile.memory.end());
    embed_uid(impl_->emulation_memory, profile.uid.data(), profile.uid_length);

    PICC picc{};
    if (!picc.emulate(type, profile.uid.data(), profile.uid_length) ||
        !impl_->emulation.begin(picc, impl_->emulation_memory.data(),
                               impl_->emulation_memory.size())) {
        Error e;
        e.layer = ErrorLayer::ChipState;
        e.operation = Operation::StartEmulation;
        e.set_detail("emulation begin failed");
        return Result<void>::failure(e);
    }
    return Result<void>::success();
}

EmulationState Engine::update_emulation() {
    if (impl_->mode == Mode::Reader || !lifecycle_is_ready(impl_->state)) {
        return EmulationState::None;
    }
    impl_->units.update();
    impl_->emulation.update();
    return to_emulation_state(impl_->emulation.state());
}

EmulationState Engine::emulation_state() const {
    if (impl_->mode == Mode::Reader) return EmulationState::None;
    return to_emulation_state(impl_->emulation.state());
}

}  // namespace gogolem::nfc
