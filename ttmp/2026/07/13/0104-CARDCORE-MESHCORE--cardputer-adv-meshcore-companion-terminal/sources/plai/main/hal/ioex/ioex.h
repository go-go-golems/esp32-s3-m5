/**
 * @file ioex.h
 * @brief PI4IOE5V6408 I2C 8-bit IO Expander driver
 * @details Used on Cap LoRa-1262 module for RF front-end control.
 *          Presence at I2C address 0x43 distinguishes LoRa-1262 from LoRa868.
 * @version 1.0
 */

#pragma once

#include "driver/i2c_master.h"
#include <stdint.h>

namespace HAL
{

    class IOExpander
    {
    public:
        static constexpr uint8_t DEFAULT_ADDR = 0x43;

        explicit IOExpander(i2c_master_bus_handle_t bus, uint8_t addr = DEFAULT_ADDR);
        ~IOExpander();

        /**
         * @brief Probe and initialise the expander.
         * @return true if the device ACKs on the bus
         */
        bool init();
        void deinit();
        bool is_initialized() const { return _initialized; }

        /**
         * @brief Set pin direction.
         * @param pin  0-7
         * @param output true = output, false = input
         */
        bool setDirection(uint8_t pin, bool output);

        /**
         * @brief Control the output high-impedance gate.
         *        Must be disabled (false) for an output pin to drive the line.
         */
        bool setHighImpedance(uint8_t pin, bool enable);

        /**
         * @brief Write an output pin.
         */
        bool digitalWrite(uint8_t pin, bool value);

        /**
         * @brief Read the current input level of a pin.
         */
        bool digitalRead(uint8_t pin, bool* value);

        /**
         * @brief Enable / disable internal pull resistor.
         */
        bool setPullEnable(uint8_t pin, bool enable);

        /**
         * @brief Select pull direction (requires pull enabled).
         * @param pullup true = pull-up, false = pull-down
         */
        bool setPullDirection(uint8_t pin, bool pullup);

    private:
        // PI4IOE5V6408 register addresses
        static constexpr uint8_t REG_DEVICE_ID = 0x01;
        static constexpr uint8_t REG_DIRECTION = 0x03; // 1 = output, 0 = input
        static constexpr uint8_t REG_OUTPUT = 0x05;
        static constexpr uint8_t REG_HIGH_Z = 0x07; // 1 = high-Z, 0 = push-pull
        static constexpr uint8_t REG_INPUT_DEFAULT = 0x09;
        static constexpr uint8_t REG_PULL_ENABLE = 0x0B;
        static constexpr uint8_t REG_PULL_SELECT = 0x0D; // 1 = pull-up, 0 = pull-down
        static constexpr uint8_t REG_INPUT_STATUS = 0x0F;
        static constexpr uint8_t REG_IRQ_MASK = 0x11;
        static constexpr uint8_t REG_IRQ_STATUS = 0x13;

        bool readRegister(uint8_t reg, uint8_t* val);
        bool writeRegister(uint8_t reg, uint8_t val);
        bool setBit(uint8_t reg, uint8_t pin, bool value);

        i2c_master_bus_handle_t _bus;
        i2c_master_dev_handle_t _dev;
        uint8_t _addr;
        bool _initialized;
    };

} // namespace HAL
