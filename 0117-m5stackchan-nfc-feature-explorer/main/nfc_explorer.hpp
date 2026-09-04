#pragma once

#include <cstdint>
#include <vector>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "M5UnitUnified.h"
#include "M5UnitUnifiedNFC.h"

enum class NfcBootMode : uint8_t {
    Reader = 0,
    EmulationUltralight = 1,
    EmulationNtag213 = 2,
};

const char *nfc_boot_mode_name(NfcBootMode mode);
NfcBootMode nfc_load_boot_mode();
esp_err_t nfc_store_boot_mode(NfcBootMode mode);

class NfcExplorer {
public:
    NfcExplorer();
    NfcExplorer(const NfcExplorer &) = delete;
    NfcExplorer &operator=(const NfcExplorer &) = delete;

    bool begin(i2c_master_bus_handle_t bus, NfcBootMode mode);
    void update();

    NfcBootMode mode() const { return mode_; }
    bool ready() const { return ready_; }

    bool scan(uint32_t timeout_ms);
    bool info();
    bool dump();
    bool raw_read(uint8_t address);
    bool reversible_write_test(uint8_t address);
    bool ndef_read();
    bool ndef_write_demo();
    bool value_inspect();
    bool wallet_demo(uint8_t block, bool rechargeable);
    void print_emulation_status();

private:
    using PICC = m5::nfc::a::PICC;
    using Key = m5::nfc::a::mifare::classic::Key;

    bool lock(TickType_t timeout = portMAX_DELAY);
    void unlock();
    bool require_reader(const char *operation) const;
    bool activate_one(PICC &picc);
    void deactivate();
    static void print_picc(const char *prefix, const PICC &picc);
    static void print_hex(const char *prefix, const uint8_t *data, size_t len);
    static void print_ndef_message(const m5::nfc::ndef::TLV &message);

    bool start_emulation(NfcBootMode mode);
    bool non_rechargeable_wallet(uint8_t block, const Key &key_a, const Key &key_b);
    bool rechargeable_wallet(uint8_t block, const Key &key_a, const Key &key_b);

    m5::unit::UnitUnified units_{};
    m5::unit::UnitNFC unit_{};
    m5::nfc::NFCLayerA reader_;
    m5::nfc::EmulationLayerA emulation_;
    std::vector<uint8_t> emulation_memory_{};
    SemaphoreHandle_t mutex_{};
    NfcBootMode mode_{NfcBootMode::Reader};
    bool ready_{};
    m5::nfc::EmulationLayerA::State last_emulation_state_{};
};
