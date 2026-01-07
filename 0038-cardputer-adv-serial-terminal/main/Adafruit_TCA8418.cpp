/**
 *  @file Adafruit_TCA8418.cpp
 *
 *  @mainpage I2C Driver for the Adafruit TCA8418 Keypad Matrix / GPIO Expander Breakout
 *
 *  @section author Author
 *
 *  Limor Fried (Adafruit Industries)
 *
 * BSD license (see license.txt)
 */

#include "Adafruit_TCA8418.h"

Adafruit_TCA8418::Adafruit_TCA8418(std::uint8_t i2c_addr, std::uint32_t freq, m5::I2C_Class *i2c)
    : I2C_Device(i2c_addr, freq, i2c) {}

Adafruit_TCA8418::~Adafruit_TCA8418() {}

bool Adafruit_TCA8418::begin() {
    bool ret = false;

    // set default all GIO pins to INPUT
    ret = writeRegister8(TCA8418_REG_GPIO_DIR_1, 0x00);
    ret = writeRegister8(TCA8418_REG_GPIO_DIR_2, 0x00);
    ret = writeRegister8(TCA8418_REG_GPIO_DIR_3, 0x00);

    // add all pins to key events
    ret = writeRegister8(TCA8418_REG_GPI_EM_1, 0xFF);
    ret = writeRegister8(TCA8418_REG_GPI_EM_2, 0xFF);
    ret = writeRegister8(TCA8418_REG_GPI_EM_3, 0xFF);

    // set all pins to FALLING interrupts
    ret = writeRegister8(TCA8418_REG_GPIO_INT_LVL_1, 0x00);
    ret = writeRegister8(TCA8418_REG_GPIO_INT_LVL_2, 0x00);
    ret = writeRegister8(TCA8418_REG_GPIO_INT_LVL_3, 0x00);

    // add all pins to interrupts
    ret = writeRegister8(TCA8418_REG_GPIO_INT_EN_1, 0xFF);
    ret = writeRegister8(TCA8418_REG_GPIO_INT_EN_2, 0xFF);
    ret = writeRegister8(TCA8418_REG_GPIO_INT_EN_3, 0xFF);

    return ret;
}

bool Adafruit_TCA8418::matrix(uint8_t rows, uint8_t columns) {
    if ((rows > 8) || (columns > 10)) return false;

    if ((rows != 0) && (columns != 0)) {
        uint8_t mask = 0x00;
        for (int r = 0; r < rows; r++) {
            mask <<= 1;
            mask |= 1;
        }
        writeRegister8(TCA8418_REG_KP_GPIO_1, mask);

        mask = 0x00;
        for (int c = 0; c < columns && c < 8; c++) {
            mask <<= 1;
            mask |= 1;
        }
        writeRegister8(TCA8418_REG_KP_GPIO_2, mask);

        if (columns > 8) {
            if (columns == 9) {
                mask = 0x01;
            } else {
                mask = 0x03;
            }
            writeRegister8(TCA8418_REG_KP_GPIO_3, mask);
        }
    }
    return true;
}

uint8_t Adafruit_TCA8418::available() {
    uint8_t eventCount = readRegister8(TCA8418_REG_KEY_LCK_EC);
    eventCount &= 0x0F;
    return eventCount;
}

uint8_t Adafruit_TCA8418::getEvent() {
    return readRegister8(TCA8418_REG_KEY_EVENT_A);
}

uint8_t Adafruit_TCA8418::flush() {
    uint8_t count = 0;
    while (getEvent() != 0) count++;
    readRegister8(TCA8418_REG_GPIO_INT_STAT_1);
    readRegister8(TCA8418_REG_GPIO_INT_STAT_2);
    readRegister8(TCA8418_REG_GPIO_INT_STAT_3);
    writeRegister8(TCA8418_REG_INT_STAT, 3);
    return count;
}

void Adafruit_TCA8418::enableInterrupts() {
    uint8_t cfg = readRegister8(TCA8418_REG_CFG);
    cfg |= (uint8_t)(TCA8418_REG_CFG_GPI_IEN | TCA8418_REG_CFG_KE_IEN);
    writeRegister8(TCA8418_REG_CFG, cfg);
}

void Adafruit_TCA8418::disableInterrupts() {
    uint8_t cfg = readRegister8(TCA8418_REG_CFG);
    cfg &= (uint8_t) ~(TCA8418_REG_CFG_GPI_IEN | TCA8418_REG_CFG_KE_IEN);
    writeRegister8(TCA8418_REG_CFG, cfg);
}

