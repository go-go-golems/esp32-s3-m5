#include "cardputer_kb/scanner.h"

#include <cstring>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "cardputer_kb/layout.h"

#include "tca8418.h"

static const char *TAG = "cardputer_kb";

cardputer_kb::UnifiedScanner::~UnifiedScanner() = default;

struct cardputer_kb::UnifiedScanner::TcaState {
    bool inited = false;

    i2c_master_bus_handle_t bus = nullptr;
    tca8418_t tca{};

    bool pressed[cardputer_kb::kRows * cardputer_kb::kCols + 1] = {false}; // index 1..56

    ~TcaState() { deinit_bus(); }

    esp_err_t init_i2c_bus(const UnifiedScannerConfig &cfg) {
        if (bus) {
            return ESP_OK;
        }
        i2c_master_bus_config_t bus_cfg;
        std::memset(&bus_cfg, 0, sizeof(bus_cfg));
        bus_cfg.i2c_port = cfg.i2c_port;
        bus_cfg.sda_io_num = (gpio_num_t)cfg.i2c_sda_gpio;
        bus_cfg.scl_io_num = (gpio_num_t)cfg.i2c_scl_gpio;
        bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.intr_priority = 0;
        bus_cfg.trans_queue_depth = 0;
        bus_cfg.flags.enable_internal_pullup = 1;

        return i2c_new_master_bus(&bus_cfg, &bus);
    }

    void deinit_bus() {
        if (tca.dev) {
            // Best-effort remove device; ignore errors.
            (void)i2c_master_bus_rm_device(tca.dev);
            tca.dev = nullptr;
        }
        if (bus) {
            (void)i2c_del_master_bus(bus);
            bus = nullptr;
        }
        inited = false;
        std::memset(pressed, 0, sizeof(pressed));
    }

    esp_err_t probe(const UnifiedScannerConfig &cfg) {
        esp_err_t err = init_i2c_bus(cfg);
        if (err != ESP_OK) return err;

        // 7x8 is the common Cardputer(-ADV) logical size.
        err = tca8418_open(&tca, bus, cfg.tca8418_addr7, cfg.i2c_hz, 7, 8);
        if (err != ESP_OK) return err;

        // A successful register read is a strong "device exists" signal.
        uint8_t v = 0;
        err = tca8418_read_reg8(&tca, TCA8418_REG_CFG, &v);
        return err;
    }

    esp_err_t init_hw(const UnifiedScannerConfig &cfg) {
        esp_err_t err = probe(cfg);
        if (err != ESP_OK) return err;

        err = tca8418_begin(&tca);
        if (err != ESP_OK) return err;

        inited = true;
        return ESP_OK;
    }

    inline bool decode_and_apply_event(uint8_t evt_raw) {
        if (evt_raw == 0) {
            return false;
        }

        // TCA8418 KEY_EVENT: bit7 is state, bits6..0 are key number (1-based).
        // This firmware family treats bit7=1 as "pressed".
        const bool is_pressed = (evt_raw & 0x80) != 0;
        uint8_t keynum = (uint8_t)(evt_raw & 0x7F);
        if (keynum == 0) {
            return false;
        }
        keynum--; // 0-based

        // Convert 1..80 key number into row/col for a 10-wide internal matrix, then remap to Cardputer picture-space.
        const uint8_t row = (uint8_t)(keynum / 10);
        const uint8_t col = (uint8_t)(keynum % 10);

        const uint8_t x = (uint8_t)(row * 2 + ((col > 3) ? 1 : 0));
        const uint8_t y = (uint8_t)((col + 4) % 4);

        if (x >= cardputer_kb::kCols || y >= cardputer_kb::kRows) {
            return false;
        }

        const uint8_t kn = cardputer_kb::keynum_from_xy(x, y);
        if (kn == 0) {
            return false;
        }

        pressed[kn] = is_pressed;
        return true;
    }

    void drain_events() {
        if (!inited) {
            return;
        }

        for (;;) {
            uint8_t count = 0;
            esp_err_t err = tca8418_available(&tca, &count);
            if (err != ESP_OK || count == 0) {
                break;
            }

            for (uint8_t i = 0; i < count; i++) {
                uint8_t evt = 0;
                err = tca8418_get_event(&tca, &evt);
                if (err != ESP_OK) {
                    break;
                }
                if (evt == 0) {
                    break;
                }
                (void)decode_and_apply_event(evt);
            }
        }

        // Best-effort clear K_INT once we've drained events.
        (void)tca8418_write_reg8(&tca, TCA8418_REG_INT_STAT, TCA8418_INTSTAT_K_INT);
    }

    ScanSnapshot snapshot() {
        ScanSnapshot snap;
        snap.use_alt_in01 = false;

        for (uint8_t kn = 1; kn <= (cardputer_kb::kRows * cardputer_kb::kCols); kn++) {
            if (!pressed[kn]) {
                continue;
            }
            int x = -1;
            int y = -1;
            cardputer_kb::xy_from_keynum(kn, &x, &y);
            if (x < 0 || y < 0) {
                continue;
            }
            snap.pressed.push_back(KeyPos{.x = x, .y = y});
            snap.pressed_keynums.push_back(kn);
        }

        return snap;
    }
};

esp_err_t cardputer_kb::UnifiedScanner::init(const UnifiedScannerConfig &cfg) {
    if (inited_) {
        return ESP_OK;
    }

    switch (cfg.backend) {
    case ScannerBackend::Auto:
        // Probe TCA8418 first; fall back to GPIO matrix.
        tca_ = std::make_unique<TcaState>();
        if (tca_->init_hw(cfg) == ESP_OK) {
            backend_ = ScannerBackend::Tca8418;
            ESP_LOGI(TAG, "UnifiedScanner autodetect: using TCA8418 backend");
            inited_ = true;
            return ESP_OK;
        }
        tca_->deinit_bus();
        tca_.reset();
        backend_ = ScannerBackend::GpioMatrix;
        ESP_LOGI(TAG, "UnifiedScanner autodetect: using GPIO matrix backend");
        break;
    case ScannerBackend::GpioMatrix:
        backend_ = ScannerBackend::GpioMatrix;
        break;
    case ScannerBackend::Tca8418:
        tca_ = std::make_unique<TcaState>();
        {
            esp_err_t err = tca_->init_hw(cfg);
            if (err != ESP_OK) {
                tca_->deinit_bus();
                tca_.reset();
                return err;
            }
        }
        backend_ = ScannerBackend::Tca8418;
        inited_ = true;
        return ESP_OK;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    matrix_.init();
    inited_ = true;
    return ESP_OK;
}

cardputer_kb::ScanSnapshot cardputer_kb::UnifiedScanner::scan() {
    if (!inited_) {
        return ScanSnapshot{};
    }

    if (backend_ == ScannerBackend::Tca8418 && tca_) {
        tca_->drain_events();
        return tca_->snapshot();
    }

    return matrix_.scan();
}
