/*!
 *  @file Adafruit_TCA8418.h
 *
 * 	I2C Driver for the Adafruit TCA8418 Keypad Matrix / GPIO Expander Breakout
 *
 * 	This is a library for the Adafruit TCA8418 breakout:
 * 	https://www.adafruit.com/products/4918
 *
 *	BSD license (see license.txt)
 */

#ifndef _ADAFRUIT_TCA8418_H
#define _ADAFRUIT_TCA8418_H

#include <M5Unified.hpp>

#include "Adafruit_TCA8418_registers.h"

#define TCA8418_DEFAULT_ADDR 0x34

enum {
    TCA8418_ROW0,
    TCA8418_ROW1,
    TCA8418_ROW2,
    TCA8418_ROW3,
    TCA8418_ROW4,
    TCA8418_ROW5,
    TCA8418_ROW6,
    TCA8418_ROW7,
    TCA8418_COL0,
    TCA8418_COL1,
    TCA8418_COL2,
    TCA8418_COL3,
    TCA8418_COL4,
    TCA8418_COL5,
    TCA8418_COL6,
    TCA8418_COL7,
    TCA8418_COL8,
    TCA8418_COL9
};

enum { TCA8418_LOW = 0, TCA8418_HIGH = 1 };
enum { TCA8418_INPUT = 0, TCA8418_OUTPUT, TCA8418_INPUT_PULLUP };
enum { TCA8418_RISING = 0, TCA8418_FALLING };

class Adafruit_TCA8418 : public m5::I2C_Device {
public:
    Adafruit_TCA8418(std::uint8_t i2c_addr = TCA8418_DEFAULT_ADDR, std::uint32_t freq = 400000,
                     m5::I2C_Class *i2c = &m5::In_I2C);
    ~Adafruit_TCA8418();

    bool begin();
    bool matrix(uint8_t rows, uint8_t columns);

    uint8_t available();
    uint8_t getEvent();
    uint8_t flush();

    void enableInterrupts();
    void disableInterrupts();
};

#endif

