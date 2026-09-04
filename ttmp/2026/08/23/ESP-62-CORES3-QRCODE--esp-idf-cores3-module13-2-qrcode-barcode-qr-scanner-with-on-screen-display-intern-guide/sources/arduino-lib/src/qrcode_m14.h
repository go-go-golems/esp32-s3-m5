/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <Arduino.h>
#include <string>
#include <vector>

class QRCodeM14 {
public:
    enum CmdResult_t { SUCCESS = 0, INVALID_PARAM = 1, TIMEOUT = 2, ACK_MISMATCH = 3 };

    enum TriggerMode_t {
        TRIGGER_MODE_KEY = 0,  // In Key Mode, Triggers a single decode; decoding stops after a successful read.
        TRIGGER_MODE_CONTINUOUS =
            1,  // In Continuous Mode, pressing the button once starts decoding, and pressing the button again stops
        TRIGGER_MODE_AUTO = 2,  // In Auto Mode, the module starts decoding when powered on and cannot be stopped.
        TRIGGER_MODE_PULSE =
            4,  // In Pulse Mode, set the TRIG pin to hold a low level for more than 20ms to trigger decoding once.
        TRIGGER_MODE_MOTION_SENSING = 5  // In Motion Sensing Mode, the module automatically triggers decoding when it
                                         // detects a change in the scene based on visual recognition information.
    };

    enum FillLightMode_t {
        FILL_LIGHT_OFF       = 0,  // Light off
        FILL_LIGHT_ON_DECODE = 2,  // Light on during decoding
        FILL_LIGHT_ON        = 3   // Light on
    };

    enum PosLightMode_t {
        POS_LIGHT_OFF             = 0,  // Light off
        POS_LIGHT_FLASH_ON_DECODE = 1,  // Light flashing during decoding
        POS_LIGHT_ON_DECODE       = 2   // Light on during decoding
    };

    /**
     * @brief Start decoding.
     */
    void startDecode();

    /**
     * @brief Stop decoding.
     */
    void stopDecode();

    /**
     * @brief Set trigger mode for QR code decoding.
     * @param mode Trigger mode from TriggerMode_t enum
     */
    void setTriggerMode(TriggerMode_t mode);

    /**
     * @brief Set decode delay time.
     * @param delay_ms Delay time in milliseconds
     */
    void setDecodeDelay(int delay_ms);

    /**
     * @brief Set trigger timeout duration.
     * @param timeout_ms Timeout duration in milliseconds
     */
    void setTriggerTimeout(int timeout_ms);

    /**
     * @brief Set motion sensing sensitivity level (1~5).
     * @param level Sensitivity level (default: 3)
     */
    void setMotionSensitivity(int level = 3);

    /**
     * @brief Set delay for continuous decode mode.
     * @param delay_ms Delay time in milliseconds
     */
    void setContinuousDecodeDelay(int delay_ms);

    /**
     * @brief Set delay for trigger decode mode.
     * @param delay_ms Delay time in milliseconds
     */
    void setTriggerDecodeDelay(int delay_ms);

    /**
     * @brief Set interval between same QR codes.
     * @param interval_ms Interval time in milliseconds
     */
    void setSameCodeInterval(int interval_ms);

    /**
     * @brief Set interval between different QR codes.
     * @param interval_ms Interval time in milliseconds
     */
    void setDiffCodeInterval(int interval_ms);

    /**
     * @brief Enable or disable no delay for same QR codes.
     * @param enable True to enable, false to disable
     */
    void setSameCodeNoDelay(bool enable);

    /**
     * @brief Set fill light mode.
     * @param mode Fill light mode from FillLightMode_t enum (default: FILL_LIGHT_ON_DECODE)
     */
    void setFillLightMode(FillLightMode_t mode = FILL_LIGHT_ON_DECODE);

    /**
     * @brief Set fill light brightness.
     * @param brightness Brightness level (default: 60)
     */
    void setFillLightBrightness(int brightness = 60);

    /**
     * @brief Set position light mode.
     * @param mode Position light mode from PosLightMode_t enum (default: POS_LIGHT_ON_DECODE)
     */
    void setPosLightMode(PosLightMode_t mode = POS_LIGHT_ON_DECODE);

    /**
     * @brief Set startup tone mode.
     * @param mode Tone mode
     */
    void setStartupTone(int mode);

    /**
     * @brief Set decode success beep count.
     * @param count Number of beeps
     */
    void setDecodeSuccessBeep(int count);

    /**
     * @brief Set case conversion mode for decoded text.
     * @param mode Conversion mode (default: 0)
     */
    void setCaseConversion(int mode = 0);

    /**
     * @brief Set protocol format mode.
     * @param mode Protocol format mode
     */
    void setProtocolFormat(int mode);

    /**
     * @brief Set communication mode to USB Serial.
     *
     */
    void setModeUsbSerial();

    /**
     * @brief Set communication mode to USB Keyboard.
     *
     */
    void setModeUsbKeyboard();

    /**
     * @brief Set communication mode to USB POS.
     *
     */
    void setModeUsbPos();

    /**
     * @brief Get device information by ID.
     * @param id Information ID
     * @return Information string
     */
    std::string getInfos(uint8_t id);

    /**
     * @brief Get software version.
     * @return Software version string
     */
    inline std::string getSoftwareVersion()
    {
        return getInfos(0xC2);
    }

    /**
     * @brief Get firmware version.
     * @return Firmware version string
     */
    inline std::string getFirmwareVersion()
    {
        return getInfos(0xC1);
    }

    /**
     * @brief Check if serial data is available.
     * @return Number of bytes available
     */
    inline int available() const
    {
        return _qrcode_serial->available();
    }

    /**
     * @brief Send command to QR code module.
     * @param cmd Command data pointer
     * @param cmd_len Command data length
     * @param cmd_ack Expected acknowledgment data (optional)
     * @param ack_len Acknowledgment data length (optional)
     * @param timeout_ms Timeout in milliseconds (default: 1000)
     * @return Command execution result
     */
    CmdResult_t sendCmd(const uint8_t* cmd, size_t cmd_len, const uint8_t* cmd_ack = nullptr, size_t ack_len = 0,
                        uint32_t timeout_ms = 1000);

    /**
     * @brief Send command to QR code module (vector version).
     * @param cmd Command data vector
     * @return Command execution result
     */
    inline CmdResult_t sendCmd(std::vector<uint8_t> cmd)
    {
        return sendCmd(cmd.data(), cmd.size());
    }

    /**
     * @brief Wait for response from QR code module.
     * @param response Response data buffer
     * @param timeout_ms Timeout in milliseconds (default: 1000)
     */
    void waitResponse(std::vector<uint8_t>& response, uint32_t timeout_ms = 1000);

    /**
     * @brief Get response data size.
     * @param response Response data buffer
     * @return Data size
     */
    uint16_t getResponseDataSize(std::vector<uint8_t>& response);

    /**
     * @brief Check response data size.
     * @param response Response data buffer
     * @return Data size
     */
    uint16_t checkResponseDataSize(std::vector<uint8_t>& response);

    /**
     * @brief Wait for QR code scan result.
     * @param result Scan result string
     * @param timeout_ms Timeout in milliseconds (default: infinite)
     */
    void waitScanResult(std::string& result, uint32_t timeout_ms = 0xFFFFFFFF);

    /**
     * @brief Convert command result to string.
     * @param result Command result enum
     * @return Result description string
     */
    std::string cmdResultToString(CmdResult_t result);

protected:
    HardwareSerial* _qrcode_serial = nullptr;

    void _setup(HardwareSerial* serial)
    {
        _qrcode_serial = serial;
    }
};
