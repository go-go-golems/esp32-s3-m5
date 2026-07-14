/**
 * @file ioex.cpp
 * @brief PI4IOE5V6408 I2C 8-bit IO Expander driver
 * @version 1.0
 */

#include "ioex.h"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "IOEX";

namespace HAL
{

    IOExpander::IOExpander(i2c_master_bus_handle_t bus, uint8_t addr)
        : _bus(bus), _dev(nullptr), _addr(addr), _initialized(false)
    {
    }

    IOExpander::~IOExpander() { deinit(); }

    bool IOExpander::init()
    {
        if (_initialized)
            return true;

        if (!_bus)
        {
            ESP_LOGE(TAG, "No I2C bus handle");
            return false;
        }

        i2c_device_config_t cfg = {};
        cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        cfg.device_address = _addr;
        cfg.scl_speed_hz = 400000;

        esp_err_t ret = i2c_master_bus_add_device(_bus, &cfg, &_dev);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to add I2C device 0x%02X: %s", _addr, esp_err_to_name(ret));
            return false;
        }

        // Probe: read Device ID register
        uint8_t id = 0;
        if (!readRegister(REG_DEVICE_ID, &id))
        {
            ESP_LOGW(TAG, "PI4IOE5V6408 not found at 0x%02X", _addr);
            i2c_master_bus_rm_device(_dev);
            _dev = nullptr;
            return false;
        }

        _initialized = true;
        ESP_LOGI(TAG, "PI4IOE5V6408 detected at 0x%02X (id=0x%02X)", _addr, id);
        return true;
    }

    void IOExpander::deinit()
    {
        if (_dev)
        {
            i2c_master_bus_rm_device(_dev);
            _dev = nullptr;
        }
        _initialized = false;
    }

    // ── Register helpers ──────────────────────────────────────────────────

    bool IOExpander::readRegister(uint8_t reg, uint8_t* val)
    {
        esp_err_t ret = i2c_master_transmit_receive(_dev, &reg, 1, val, 1, 100);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Read reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    bool IOExpander::writeRegister(uint8_t reg, uint8_t val)
    {
        uint8_t buf[2] = {reg, val};
        esp_err_t ret = i2c_master_transmit(_dev, buf, 2, 100);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Write reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    bool IOExpander::setBit(uint8_t reg, uint8_t pin, bool value)
    {
        if (pin > 7)
            return false;
        uint8_t cur = 0;
        if (!readRegister(reg, &cur))
            return false;
        if (value)
            cur |= (1 << pin);
        else
            cur &= ~(1 << pin);
        return writeRegister(reg, cur);
    }

    // ── Public API ────────────────────────────────────────────────────────

    bool IOExpander::setDirection(uint8_t pin, bool output) { return setBit(REG_DIRECTION, pin, output); }

    bool IOExpander::setHighImpedance(uint8_t pin, bool enable) { return setBit(REG_HIGH_Z, pin, enable); }

    bool IOExpander::digitalWrite(uint8_t pin, bool value) { return setBit(REG_OUTPUT, pin, value); }

    bool IOExpander::digitalRead(uint8_t pin, bool* value)
    {
        if (pin > 7 || !value)
            return false;
        uint8_t reg = 0;
        if (!readRegister(REG_INPUT_STATUS, &reg))
            return false;
        *value = (reg >> pin) & 1;
        return true;
    }

    bool IOExpander::setPullEnable(uint8_t pin, bool enable) { return setBit(REG_PULL_ENABLE, pin, enable); }

    bool IOExpander::setPullDirection(uint8_t pin, bool pullup) { return setBit(REG_PULL_SELECT, pin, pullup); }

} // namespace HAL
