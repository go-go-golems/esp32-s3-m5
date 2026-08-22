#include "nfc_explorer.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "nvs.h"

namespace {
constexpr char TAG[] = "nfc_explorer";
constexpr char NVS_NAMESPACE[] = "nfc_explorer";
constexpr char NVS_MODE_KEY[] = "boot_mode";

using m5::nfc::a::Type;
using m5::nfc::a::mifare::classic::DEFAULT_KEY;
using m5::nfc::a::mifare::classic::READ_WRITE_BLOCK;
using m5::nfc::a::mifare::classic::VALUE_BLOCK_NON_RECHARGEABLE;
using m5::nfc::a::mifare::classic::VALUE_BLOCK_RECHARGEABLE;
using m5::nfc::a::mifare::classic::get_sector_trailer_block;
using m5::nfc::ndef::Record;
using m5::nfc::ndef::Tag;
using m5::nfc::ndef::TLV;
using m5::nfc::ndef::TNF;
using m5::nfc::ndef::URIProtocol;

constexpr std::array<uint8_t, 7> ULTRALIGHT_UID{0x04, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE};
constexpr std::array<uint8_t, 7> NTAG213_UID{0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33};

constexpr uint8_t ULTRALIGHT_MEMORY[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xA3, 0x00, 0x00, 0xE1, 0x10, 0x06, 0x00,
    0x03, 0x25, 0x91, 0x01, 0x0D, 0x55, 0x04, 0x6D,
    0x35, 0x73, 0x74, 0x61, 0x63, 0x6B, 0x2E, 0x63,
    0x6F, 0x6D, 0x2F, 0x51, 0x01, 0x10, 0x54, 0x02,
    0x65, 0x6E, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20,
    0x4D, 0x35, 0x53, 0x74, 0x61, 0x63, 0x6B, 0xFE,
    0x44, 0x45, 0x46, 0x00, 0x44, 0x45, 0x46, 0x00,
};

constexpr uint8_t NTAG213_MEMORY[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x00, 0x00, 0xE1, 0x10, 0x12, 0x00,
    0x01, 0x03, 0xA0, 0x0C, 0x34, 0x03, 0x58, 0x91,
    0x01, 0x0D, 0x55, 0x04, 0x6D, 0x35, 0x73, 0x74,
    0x61, 0x63, 0x6B, 0x2E, 0x63, 0x6F, 0x6D, 0x2F,
    0x11, 0x01, 0x11, 0x54, 0x02, 0x7A, 0x68, 0xE4,
    0xBD, 0xA0, 0xE5, 0xA5, 0xBD, 0x20, 0x4D, 0x35,
    0x53, 0x74, 0x61, 0x63, 0x6B, 0x11, 0x01, 0x10,
    0x54, 0x02, 0x65, 0x6E, 0x48, 0x65, 0x6C, 0x6C,
    0x6F, 0x20, 0x4D, 0x35, 0x53, 0x74, 0x61, 0x63,
    0x6B, 0x51, 0x01, 0x1A, 0x54, 0x02, 0x6A, 0x61,
    0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93, 0xE3, 0x81,
    0xAB, 0xE3, 0x81, 0xA1, 0xE3, 0x81, 0xAF, 0x20,
    0x4D, 0x35, 0x53, 0x74, 0x61, 0x63, 0x6B, 0xFE,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xBD, 0x02, 0x00, 0x00, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

uint8_t bcc8(const uint8_t *data, size_t len, uint8_t initial = 0)
{
    uint8_t result = initial;
    for (size_t i = 0; i < len; ++i) {
        result ^= data[i];
    }
    return result;
}

void embed_uid(std::vector<uint8_t> &memory, const std::array<uint8_t, 7> &uid)
{
    if (memory.size() < 9) {
        return;
    }
    std::copy_n(uid.begin(), 3, memory.begin());
    memory[3] = bcc8(uid.data(), 3, 0x88);
    std::copy_n(uid.begin() + 3, 4, memory.begin() + 4);
    memory[8] = bcc8(uid.data() + 3, 4);
}

const char *emulation_state_name(m5::nfc::EmulationLayerA::State state)
{
    using State = m5::nfc::EmulationLayerA::State;
    switch (state) {
        case State::None: return "none";
        case State::Off: return "off";
        case State::Idle: return "idle";
        case State::Ready: return "ready";
        case State::Active: return "active";
        case State::Halt: return "halt";
        default: return "unknown";
    }
}
}  // namespace

const char *nfc_boot_mode_name(NfcBootMode mode)
{
    switch (mode) {
        case NfcBootMode::Reader: return "reader";
        case NfcBootMode::EmulationUltralight: return "emulation-ultralight";
        case NfcBootMode::EmulationNtag213: return "emulation-ntag213";
        default: return "reader";
    }
}

NfcBootMode nfc_load_boot_mode()
{
    nvs_handle_t handle{};
    uint8_t value = static_cast<uint8_t>(NfcBootMode::Reader);
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
        if (nvs_get_u8(handle, NVS_MODE_KEY, &value) != ESP_OK) {
            value = static_cast<uint8_t>(NfcBootMode::Reader);
        }
        nvs_close(handle);
    }
    if (value > static_cast<uint8_t>(NfcBootMode::EmulationNtag213)) {
        value = static_cast<uint8_t>(NfcBootMode::Reader);
    }
    return static_cast<NfcBootMode>(value);
}

esp_err_t nfc_store_boot_mode(NfcBootMode mode)
{
    nvs_handle_t handle{};
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_u8(handle, NVS_MODE_KEY, static_cast<uint8_t>(mode));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

NfcExplorer::NfcExplorer() : reader_(unit_), emulation_(unit_)
{
    mutex_ = xSemaphoreCreateMutex();
}

bool NfcExplorer::begin(i2c_master_bus_handle_t bus, NfcBootMode mode)
{
    if (!bus || !mutex_) {
        return false;
    }
    mode_ = mode;

    auto config = unit_.config();
    config.mode = m5::nfc::NFC::A;
    config.emulation = mode != NfcBootMode::Reader;
    unit_.config(config);

    if (!units_.add(unit_, bus) || !units_.begin()) {
        ESP_LOGE(TAG, "M5Unit-NFC initialization failed");
        return false;
    }

    const std::string debug = units_.debugInfo();
    ESP_LOGI(TAG, "M5Unit-NFC ready mode=%s\n%s", nfc_boot_mode_name(mode), debug.c_str());
    ready_ = mode == NfcBootMode::Reader || start_emulation(mode);
    return ready_;
}

bool NfcExplorer::lock(TickType_t timeout)
{
    return mutex_ && xSemaphoreTake(mutex_, timeout) == pdTRUE;
}

void NfcExplorer::unlock()
{
    xSemaphoreGive(mutex_);
}

void NfcExplorer::update()
{
    if (!ready_ || mode_ == NfcBootMode::Reader || !lock(0)) {
        return;
    }
    units_.update();
    emulation_.update();
    const auto state = emulation_.state();
    if (state != last_emulation_state_) {
        last_emulation_state_ = state;
        printf("NFC_EMULATION state=%s mode=%s\n", emulation_state_name(state), nfc_boot_mode_name(mode_));
    }
    unlock();
}

bool NfcExplorer::require_reader(const char *operation) const
{
    if (!ready_) {
        printf("NFC_RESULT op=%s ok=0 reason=not-ready\n", operation);
        return false;
    }
    if (mode_ != NfcBootMode::Reader) {
        printf("NFC_RESULT op=%s ok=0 reason=emulation-mode current=%s\n", operation, nfc_boot_mode_name(mode_));
        return false;
    }
    return true;
}

void NfcExplorer::print_picc(const char *prefix, const PICC &picc)
{
    printf("%s uid=%s type=\"%s\" atqa=%04X sak=%02X uid_bytes=%u blocks=%u unit=%u user=%u total=%u "
           "first_user=%u last_user=%u ndef=%u forum_tag=%u filesystem=0x%08lX\n",
           prefix, picc.uidAsString().c_str(), picc.typeAsString().c_str(), picc.atqa, picc.sak, picc.size,
           picc.blocks, picc.unitSize(), picc.userAreaSize(), picc.totalSize(), picc.firstUserBlock(),
           picc.lastUserBlock(), picc.supportsNDEF(), static_cast<unsigned>(picc.nfcForumTagType()),
           static_cast<unsigned long>(picc.fileSystemFeature()));
}

void NfcExplorer::print_hex(const char *prefix, const uint8_t *data, size_t len)
{
    printf("%s len=%u hex=", prefix, static_cast<unsigned>(len));
    for (size_t i = 0; i < len; ++i) {
        printf("%02X", data[i]);
        if (i + 1 != len) {
            putchar(':');
        }
    }
    putchar('\n');
}

bool NfcExplorer::activate_one(PICC &picc)
{
    units_.update(true);

    // Enumeration deliberately leaves stationary PICCs in HALT. Try REQA first
    // for an IDLE tag, then WUPA so consecutive console commands work without
    // requiring the operator to remove and replace the tag.
    bool woke_from_halt = false;
    if (!reader_.request(picc.atqa)) {
        if (!reader_.wakeup(picc.atqa)) {
            printf("NFC_ACTIVATE ok=0 phase=request-or-wakeup\n");
            return false;
        }
        woke_from_halt = true;
    }
    if (!reader_.select(picc)) {
        printf("NFC_ACTIVATE ok=0 phase=select atqa=%04X\n", picc.atqa);
        return false;
    }
    printf("NFC_ACTIVATE_DISCOVERY ok=1 source=%s\n", woke_from_halt ? "WUPA" : "REQA");

    if (!reader_.identify(picc)) {
        printf("NFC_ACTIVATE ok=0 phase=identify uid=%s\n", picc.uidAsString().c_str());
        reader_.deactivate();
        return false;
    }
    if (!reader_.reactivate(picc)) {
        printf("NFC_ACTIVATE ok=0 phase=reactivate uid=%s type=\"%s\"\n", picc.uidAsString().c_str(),
               picc.typeAsString().c_str());
        reader_.deactivate();
        return false;
    }
    print_picc("NFC_PICC", picc);
    return true;
}

void NfcExplorer::deactivate()
{
    if (reader_.activatedPICC().valid()) {
        const bool ok = reader_.deactivate();
        printf("NFC_DEACTIVATE ok=%u\n", ok);
    }
}

bool NfcExplorer::scan(uint32_t timeout_ms)
{
    if (!require_reader("scan") || !lock()) {
        return false;
    }
    units_.update(true);
    std::vector<PICC> piccs;
    const bool detected = reader_.detect(piccs, timeout_ms);
    unsigned identified = 0;
    if (detected) {
        for (auto &picc : piccs) {
            if (reader_.identify(picc)) {
                print_picc("NFC_SCAN_PICC", picc);
                ++identified;
            } else {
                printf("NFC_SCAN_IDENTIFY ok=0 uid=%s\n", picc.uidAsString().c_str());
            }
        }
    }
    deactivate();
    printf("NFC_RESULT op=scan ok=%u detected=%u identified=%u timeout_ms=%lu\n", detected, piccs.size(), identified,
           static_cast<unsigned long>(timeout_ms));
    unlock();
    return detected && identified;
}

bool NfcExplorer::info()
{
    if (!require_reader("info") || !lock()) {
        return false;
    }
    PICC picc{};
    const bool ok = activate_one(picc);
    deactivate();
    printf("NFC_RESULT op=info ok=%u\n", ok);
    unlock();
    return ok;
}

bool NfcExplorer::dump()
{
    if (!require_reader("dump") || !lock()) {
        return false;
    }
    PICC picc{};
    bool ok = activate_one(picc);
    if (ok) {
        printf("NFC_DUMP_BEGIN key_a=FFFFFFFFFFFF\n");
        ok = reader_.dump(DEFAULT_KEY);
        printf("NFC_DUMP_END ok=%u\n", ok);
    }
    deactivate();
    printf("NFC_RESULT op=dump ok=%u\n", ok);
    unlock();
    return ok;
}

bool NfcExplorer::raw_read(uint8_t address)
{
    if (!require_reader("raw-read") || !lock()) {
        return false;
    }
    PICC picc{};
    bool ok = activate_one(picc);
    uint8_t data[16]{};
    if (ok && picc.isMifareClassic()) {
        ok = reader_.mifareClassicAuthenticateA(address, DEFAULT_KEY);
        if (!ok) {
            printf("NFC_RAW_AUTH ok=0 block=%u key=A:FFFFFFFFFFFF\n", address);
        }
    }
    if (ok) {
        ok = reader_.read16(data, address);
    }
    if (ok) {
        print_hex("NFC_RAW_READ", data, sizeof(data));
    }
    deactivate();
    printf("NFC_RESULT op=raw-read ok=%u address=%u\n", ok, address);
    unlock();
    return ok;
}

bool NfcExplorer::reversible_write_test(uint8_t address)
{
    if (!require_reader("write-test") || !lock()) {
        return false;
    }
    PICC picc{};
    bool ok = activate_one(picc);
    if (ok && !picc.isUserBlock(address)) {
        printf("NFC_WRITE_TEST ok=0 reason=not-user-area address=%u\n", address);
        ok = false;
    }

    uint8_t original[16]{};
    uint8_t pattern[16]{};
    uint8_t verify[16]{};
    size_t width = 0;
    if (ok && picc.isMifareClassic()) {
        width = 16;
        ok = reader_.mifareClassicAuthenticateA(address, DEFAULT_KEY) && reader_.read16(original, address);
        for (size_t i = 0; i < width; ++i) {
            pattern[i] = static_cast<uint8_t>(0xA0U ^ address ^ i);
        }
    } else if (ok && (picc.isMifareUltralight() || picc.isNTAG2())) {
        width = 4;
        ok = reader_.read16(original, address);
        pattern[0] = 0xD1;
        pattern[1] = 0xA6;
        pattern[2] = address;
        pattern[3] = 0x5A;
    } else if (ok) {
        printf("NFC_WRITE_TEST ok=0 reason=unsupported-type type=\"%s\"\n", picc.typeAsString().c_str());
        ok = false;
    }

    bool write_ok = false;
    bool verify_ok = false;
    bool restore_ok = false;
    if (ok) {
        print_hex("NFC_WRITE_ORIGINAL", original, width);
        print_hex("NFC_WRITE_PATTERN", pattern, width);
        write_ok = width == 4 ? reader_.write4(address, pattern, width) : reader_.write16(address, pattern, width);
        if (write_ok) {
            verify_ok = reader_.read16(verify, address) && std::memcmp(pattern, verify, width) == 0;
            print_hex("NFC_WRITE_VERIFY", verify, width);
        }
        restore_ok = width == 4 ? reader_.write4(address, original, width) : reader_.write16(address, original, width);
        if (restore_ok) {
            std::memset(verify, 0, sizeof(verify));
            restore_ok = reader_.read16(verify, address) && std::memcmp(original, verify, width) == 0;
            print_hex("NFC_WRITE_RESTORED", verify, width);
        }
        ok = write_ok && verify_ok && restore_ok;
    }

    deactivate();
    printf("NFC_RESULT op=write-test ok=%u address=%u width=%u write=%u verify=%u restore=%u\n", ok, address,
           static_cast<unsigned>(width), write_ok, verify_ok, restore_ok);
    unlock();
    return ok;
}

void NfcExplorer::print_ndef_message(const TLV &message)
{
    if (!message.isMessageTLV()) {
        printf("NFC_NDEF message=absent tag=%u\n", static_cast<unsigned>(message.tag()));
        return;
    }
    unsigned index = 0;
    for (const auto &record : message.records()) {
        const std::string text = record.payloadAsString();
        printf("NFC_NDEF_RECORD index=%u tnf=%u type=\"%s\" payload_len=%lu", index,
               static_cast<unsigned>(record.tnf()), record.type(), static_cast<unsigned long>(record.payloadSize()));
        if (record.tnf() == TNF::Wellknown) {
            printf(" value=\"%s\"", text.c_str());
        }
        putchar('\n');
        ++index;
    }
    printf("NFC_NDEF_RECORDS count=%u\n", index);
}

bool NfcExplorer::ndef_read()
{
    if (!require_reader("ndef-read") || !lock()) {
        return false;
    }
    PICC picc{};
    bool ok = activate_one(picc);
    bool valid = false;
    if (ok && !picc.supportsNDEF()) {
        printf("NFC_NDEF supported=0 type=\"%s\"\n", picc.typeAsString().c_str());
        ok = false;
    }
    if (ok) {
        ok = reader_.ndefIsValidFormat(valid);
        printf("NFC_NDEF_FORMAT query_ok=%u valid=%u\n", ok, valid);
    }
    TLV message{};
    if (ok && valid) {
        ok = reader_.ndefRead(message);
        if (ok) {
            print_ndef_message(message);
        }
    }
    deactivate();
    printf("NFC_RESULT op=ndef-read ok=%u valid=%u\n", ok, valid);
    unlock();
    return ok;
}

bool NfcExplorer::ndef_write_demo()
{
    if (!require_reader("ndef-write-demo") || !lock()) {
        return false;
    }
    PICC picc{};
    bool ok = activate_one(picc);
    bool valid = false;
    if (ok && !picc.supportsNDEF()) {
        printf("NFC_NDEF_WRITE ok=0 reason=unsupported type=\"%s\"\n", picc.typeAsString().c_str());
        ok = false;
    }
    if (ok) {
        ok = reader_.ndefIsValidFormat(valid);
    }
    if (ok && !valid) {
        printf("NFC_NDEF_WRITE ok=0 reason=not-existing-ndef-format conversion=refused\n");
        ok = false;
    }

    TLV message{Tag::Message};
    Record uri{};
    Record text{};
    if (ok) {
        ok = uri.setURIPayload("m5stack.com/esp60", URIProtocol::HTTPS) &&
             text.setTextPayload("Native ESP-IDF M5StackChan NFC", "en") &&
             message.push_back(uri) && message.push_back(text);
    }
    if (ok && (picc.userAreaSize() == 0 || message.required() + 1 > picc.userAreaSize())) {
        printf("NFC_NDEF_WRITE ok=0 reason=capacity required=%lu user=%u\n",
               static_cast<unsigned long>(message.required() + 1), picc.userAreaSize());
        ok = false;
    }
    if (ok) {
        printf("NFC_NDEF_WRITE_BEGIN required=%lu user=%u records=2\n",
               static_cast<unsigned long>(message.required()), picc.userAreaSize());
        ok = reader_.ndefWrite(message);
    }
    TLV verify{};
    if (ok) {
        ok = reader_.ndefRead(verify);
        if (ok) {
            print_ndef_message(verify);
        }
    }
    deactivate();
    printf("NFC_RESULT op=ndef-write-demo ok=%u\n", ok);
    unlock();
    return ok;
}

bool NfcExplorer::value_inspect()
{
    if (!require_reader("value-inspect") || !lock()) {
        return false;
    }
    PICC picc{};
    bool ok = activate_one(picc);
    if (ok && !picc.isMifareClassic()) {
        printf("NFC_VALUE_SCAN ok=0 reason=not-classic type=\"%s\"\n", picc.typeAsString().c_str());
        ok = false;
    }

    unsigned found = 0;
    uint16_t active_trailer = 0xFFFF;
    for (uint16_t block = 0; ok && block < picc.blocks; ++block) {
        const uint16_t trailer = get_sector_trailer_block(block);
        if (trailer != active_trailer) {
            active_trailer = trailer;
            if (!reader_.mifareClassicAuthenticateA(static_cast<uint8_t>(trailer), DEFAULT_KEY)) {
                printf("NFC_VALUE_AUTH ok=0 trailer=%u key=A:FFFFFFFFFFFF\n", trailer);
                ok = false;
                break;
            }
        }
        if (block == trailer || !picc.isUserBlock(static_cast<uint8_t>(block))) {
            continue;
        }
        bool is_value = false;
        if (!reader_.mifareClassicIsValueBlock(is_value, static_cast<uint8_t>(block))) {
            printf("NFC_VALUE_CHECK ok=0 block=%u\n", block);
            ok = false;
            break;
        }
        if (is_value) {
            int32_t value = 0;
            const bool read_ok = reader_.mifareClassicReadValueBlock(value, static_cast<uint8_t>(block));
            printf("NFC_VALUE block=%u read_ok=%u value=%ld\n", block, read_ok, static_cast<long>(value));
            ok = ok && read_ok;
            ++found;
        }
    }
    deactivate();
    printf("NFC_RESULT op=value-inspect ok=%u found=%u\n", ok, found);
    unlock();
    return ok;
}

bool NfcExplorer::non_rechargeable_wallet(uint8_t block, const Key &key_a, const Key &key_b)
{
    const auto &picc = reader_.activatedPICC();
    if (block == 0 || !picc.isUserBlock(block) || !picc.isUserBlock(block - 1)) {
        printf("NFC_WALLET ok=0 reason=invalid-user-block block=%u\n", block);
        return false;
    }
    if (!reader_.mifareClassicAuthenticateA(block, key_a) ||
        !reader_.mifareClassicWriteAccessCondition(block, READ_WRITE_BLOCK, key_a, key_b) ||
        !reader_.mifareClassicWriteValueBlock(block, 1234567) ||
        !reader_.mifareClassicWriteAccessCondition(block, VALUE_BLOCK_NON_RECHARGEABLE, key_a, key_b)) {
        return false;
    }
    printf("NFC_WALLET phase=initial value=1234567\n");
    reader_.dump(block);
    if (!reader_.mifareClassicDecrementValueBlock(block, 4567)) {
        return false;
    }
    printf("NFC_WALLET phase=decrement expected=1230000\n");
    reader_.dump(block);
    if (reader_.mifareClassicIncrementValueBlock(block, 99)) {
        printf("NFC_WALLET ok=0 reason=non-rechargeable-increment-succeeded\n");
        return false;
    }
    if (!reader_.reactivate() || !reader_.mifareClassicAuthenticateA(block, key_a)) {
        return false;
    }
    printf("NFC_WALLET phase=increment-rejected expected=1\n");
    if (!reader_.mifareClassicRestoreValueBlock(block) || !reader_.mifareClassicTransferValueBlock(block - 1)) {
        return false;
    }
    printf("NFC_WALLET phase=copy source=%u destination=%u\n", block, block - 1);
    reader_.dump(block);
    return true;
}

bool NfcExplorer::rechargeable_wallet(uint8_t block, const Key &key_a, const Key &key_b)
{
    const auto &picc = reader_.activatedPICC();
    if (block == 0 || !picc.isUserBlock(block) || !picc.isUserBlock(block - 1)) {
        printf("NFC_WALLET ok=0 reason=invalid-user-block block=%u\n", block);
        return false;
    }
    const uint8_t trailer = static_cast<uint8_t>(get_sector_trailer_block(block));
    if (!reader_.mifareClassicAuthenticateA(trailer, key_a) ||
        !reader_.mifareClassicWriteAccessCondition(trailer, 0x03, key_a, key_b) ||
        !reader_.mifareClassicAuthenticateB(block, key_b) ||
        !reader_.mifareClassicWriteAccessCondition(block, READ_WRITE_BLOCK, key_a, key_b) ||
        !reader_.mifareClassicWriteValueBlock(block, 1234567) ||
        !reader_.mifareClassicWriteAccessCondition(block, VALUE_BLOCK_RECHARGEABLE, key_a, key_b)) {
        return false;
    }
    printf("NFC_WALLET phase=initial value=1234567\n");
    reader_.dump(block);
    if (!reader_.mifareClassicDecrementValueBlock(block, 4567)) {
        return false;
    }
    printf("NFC_WALLET phase=decrement expected=1230000\n");
    reader_.dump(block);
    if (!reader_.mifareClassicIncrementValueBlock(block, 99)) {
        return false;
    }
    printf("NFC_WALLET phase=increment expected=1230099\n");
    reader_.dump(block);
    if (!reader_.mifareClassicRestoreValueBlock(block) || !reader_.mifareClassicTransferValueBlock(block - 1)) {
        return false;
    }
    printf("NFC_WALLET phase=copy source=%u destination=%u\n", block, block - 1);
    reader_.dump(block);
    return true;
}

bool NfcExplorer::wallet_demo(uint8_t block, bool rechargeable)
{
    if (!require_reader("wallet-demo") || !lock()) {
        return false;
    }
    PICC picc{};
    bool ok = activate_one(picc);
    if (ok && !picc.isMifareClassic()) {
        printf("NFC_WALLET ok=0 reason=not-classic type=\"%s\"\n", picc.typeAsString().c_str());
        ok = false;
    }

    uint8_t original[16]{};
    uint8_t adjacent[16]{};
    if (ok) {
        ok = block > 0 && picc.isUserBlock(block) && picc.isUserBlock(block - 1) &&
             reader_.mifareClassicAuthenticateA(block, DEFAULT_KEY) && reader_.read16(original, block) &&
             reader_.read16(adjacent, block - 1);
    }
    if (ok) {
        print_hex("NFC_WALLET_ORIGINAL", original, sizeof(original));
        print_hex("NFC_WALLET_ADJACENT_ORIGINAL", adjacent, sizeof(adjacent));
        ok = rechargeable ? rechargeable_wallet(block, DEFAULT_KEY, DEFAULT_KEY)
                          : non_rechargeable_wallet(block, DEFAULT_KEY, DEFAULT_KEY);
    }

    // Best-effort restoration. The command is restricted to sacrificial cards
    // because access-condition failure can prevent complete restoration.
    bool restore_ok = false;
    if (block > 0 && picc.isMifareClassic()) {
        reader_.reactivate();
        const uint8_t trailer = static_cast<uint8_t>(get_sector_trailer_block(block));
        bool auth = reader_.mifareClassicAuthenticateA(trailer, DEFAULT_KEY);
        if (!auth) {
            reader_.reactivate();
            auth = reader_.mifareClassicAuthenticateB(trailer, DEFAULT_KEY);
        }
        if (auth) {
            const bool data_access = reader_.mifareClassicWriteAccessCondition(block, READ_WRITE_BLOCK,
                                                                               DEFAULT_KEY, DEFAULT_KEY);
            const bool data_restore = data_access && reader_.write16(block, original, sizeof(original)) &&
                                      reader_.write16(block - 1, adjacent, sizeof(adjacent));
            const bool trailer_restore = reader_.mifareClassicWriteAccessCondition(trailer, 0x01,
                                                                                    DEFAULT_KEY, DEFAULT_KEY);
            restore_ok = data_restore && trailer_restore;
        }
    }
    deactivate();
    ok = ok && restore_ok;
    printf("NFC_RESULT op=wallet-demo ok=%u block=%u mode=%s restore=%u\n", ok, block,
           rechargeable ? "rechargeable" : "non-rechargeable", restore_ok);
    unlock();
    return ok;
}

bool NfcExplorer::start_emulation(NfcBootMode mode)
{
    PICC picc{};
    const uint8_t *uid = nullptr;
    size_t uid_size = 0;
    Type type = Type::Unknown;

    if (mode == NfcBootMode::EmulationUltralight) {
        emulation_memory_.assign(std::begin(ULTRALIGHT_MEMORY), std::end(ULTRALIGHT_MEMORY));
        embed_uid(emulation_memory_, ULTRALIGHT_UID);
        uid = ULTRALIGHT_UID.data();
        uid_size = ULTRALIGHT_UID.size();
        type = Type::MIFARE_Ultralight;
    } else if (mode == NfcBootMode::EmulationNtag213) {
        emulation_memory_.assign(std::begin(NTAG213_MEMORY), std::end(NTAG213_MEMORY));
        embed_uid(emulation_memory_, NTAG213_UID);
        uid = NTAG213_UID.data();
        uid_size = NTAG213_UID.size();
        type = Type::NTAG_213;
    } else {
        return false;
    }

    if (!picc.emulate(type, uid, static_cast<uint8_t>(uid_size)) ||
        !emulation_.begin(picc, emulation_memory_.data(), emulation_memory_.size())) {
        ESP_LOGE(TAG, "Failed to start NFC-A emulation mode=%s", nfc_boot_mode_name(mode));
        return false;
    }
    last_emulation_state_ = emulation_.state();
    const auto &active = emulation_.emulatePICC();
    print_picc("NFC_EMULATION_PICC", active);
    printf("NFC_EMULATION state=%s memory=%u mode=%s\n", emulation_state_name(last_emulation_state_),
           static_cast<unsigned>(emulation_memory_.size()), nfc_boot_mode_name(mode));
    return true;
}

void NfcExplorer::print_emulation_status()
{
    if (!lock(pdMS_TO_TICKS(100))) {
        printf("NFC_EMULATION_STATUS ok=0 reason=busy\n");
        return;
    }
    if (!ready_ || mode_ == NfcBootMode::Reader) {
        printf("NFC_EMULATION_STATUS ok=0 mode=%s\n", nfc_boot_mode_name(mode_));
    } else {
        printf("NFC_EMULATION_STATUS ok=1 mode=%s state=%s memory=%u\n", nfc_boot_mode_name(mode_),
               emulation_state_name(emulation_.state()), static_cast<unsigned>(emulation_memory_.size()));
        print_picc("NFC_EMULATION_PICC", emulation_.emulatePICC());
    }
    unlock();
}
