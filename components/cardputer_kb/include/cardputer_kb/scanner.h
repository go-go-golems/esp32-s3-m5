#pragma once

#include <stdint.h>

#include <memory>
#include <vector>

#include "esp_err.h"

namespace cardputer_kb {

struct KeyPos {
    int x; // 0..13
    int y; // 0..3

    bool operator==(const KeyPos &o) const { return x == o.x && y == o.y; }
};

struct ScanSnapshot {
    bool use_alt_in01 = false;
    std::vector<KeyPos> pressed;
    std::vector<uint8_t> pressed_keynums;
};

// Matrix scanner for Cardputer keyboard.
// Returns physical key positions and vendor-style keyNum values (1..56).
class MatrixScanner {
  public:
    void init();
    ScanSnapshot scan();

  private:
    bool use_alt_in01_ = false;
};

enum class ScannerBackend : uint8_t {
    Auto = 0,
    GpioMatrix = 1,
    Tca8418 = 2,
};

struct UnifiedScannerConfig {
    ScannerBackend backend = ScannerBackend::Auto;
    // Cardputer-ADV keyboard (TCA8418) defaults.
    int i2c_port = 0;
    int i2c_sda_gpio = 8;
    int i2c_scl_gpio = 9;
    int int_gpio = 11;
    uint8_t tca8418_addr7 = 0x34;
    uint32_t i2c_hz = 400000;
};

// Unified scanner facade intended to become the single entrypoint for Cardputer keyboard scanning.
//
// Design goal:
// - callers should not need to know if the keyboard is a GPIO-scanned matrix (Cardputer) or a TCA8418
//   keypad controller over I2C (Cardputer-ADV).
//
// Implementation status:
// - GPIO matrix backend is implemented (wraps MatrixScanner)
// - TCA8418 backend + runtime auto-detect will be added next
class UnifiedScanner {
  public:
    ~UnifiedScanner();
    esp_err_t init(const UnifiedScannerConfig &cfg = {});
    ScanSnapshot scan();
    ScannerBackend backend() const { return backend_; }

  private:
    struct TcaState;
    struct TcaDeleter {
        void operator()(TcaState *p) const;
    };
    MatrixScanner matrix_{};
    std::unique_ptr<TcaState, TcaDeleter> tca_{};
    ScannerBackend backend_ = ScannerBackend::Auto;
    bool inited_ = false;
};

} // namespace cardputer_kb
