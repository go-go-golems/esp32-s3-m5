/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "qrcode_m14.h"
#include "debug.h"

/* -------------------------------------------------------------------------- */
/*                                Communication                               */
/* -------------------------------------------------------------------------- */
QRCodeM14::CmdResult_t QRCodeM14::sendCmd(const uint8_t* cmd, size_t cmd_len, const uint8_t* cmd_ack, size_t ack_len,
                                          uint32_t timeout_ms)
{
    if (!_qrcode_serial || !cmd || cmd_len == 0) {
        return CmdResult_t::INVALID_PARAM;
    }

    // Clear rx buffer
    while (_qrcode_serial->available()) {
        _qrcode_serial->read();
    }

    // Send command
    _LOG_DEBUG("tx: ");
    debug_print_buffer(cmd, cmd_len);

    _qrcode_serial->write(cmd, cmd_len);

    // If no response is expected
    if (!cmd_ack || ack_len == 0) {
        return CmdResult_t::SUCCESS;
    }

    delay(100);

    // Wait for response
    uint32_t start_time = millis();
    while (millis() - start_time < timeout_ms) {
        if (_qrcode_serial->available() >= ack_len) {
            uint8_t rx[ack_len];
            _qrcode_serial->readBytes(rx, ack_len);

            _LOG_DEBUG("rx: ");
            debug_print_buffer(rx, ack_len);

            if (memcmp(rx, cmd_ack, ack_len) == 0) {
                return CmdResult_t::SUCCESS;
            } else {
                return CmdResult_t::ACK_MISMATCH;
            }
        }
        delay(5);
    }

    return CmdResult_t::TIMEOUT;
}

void QRCodeM14::waitResponse(std::vector<uint8_t>& response, uint32_t timeout_ms)
{
    response.clear();
    uint32_t start_time = millis();
    do {
        int bytes_num = _qrcode_serial->available();
        if (bytes_num > 0) {
            response.resize(bytes_num);
            _qrcode_serial->readBytes(response.data(), bytes_num);
            break;
        }
        delay(20);
    } while (millis() - start_time < timeout_ms);
}

uint16_t QRCodeM14::getResponseDataSize(std::vector<uint8_t>& response)
{
    if (response.size() < 5) {
        _LOG_ERROR("invaild response size: %d\n", response.size());
        return 0;
    }

    uint16_t size = (static_cast<uint16_t>(response[3]) << 8) | response[4];
    return size;
}

uint16_t QRCodeM14::checkResponseDataSize(std::vector<uint8_t>& response)
{
    uint16_t data_size = getResponseDataSize(response);
    if (data_size == 0) {
        return 0;
    }

    size_t data_start_index = 5;

    if (response.size() < data_start_index + data_size) {
        _LOG_ERROR("invaild data size: %d < %d\n", response.size(), data_start_index + data_size);
        return 0;
    }

    return data_size;
}

void QRCodeM14::waitScanResult(std::string& result, uint32_t timeout_ms)
{
    result.clear();
    std::vector<uint8_t> response;
    waitResponse(response, timeout_ms);
    result = std::string(response.begin(), response.end());
}

std::string QRCodeM14::cmdResultToString(QRCodeM14::CmdResult_t result)
{
    switch (result) {
        case QRCodeM14::CmdResult_t::SUCCESS:
            return "success";
        case QRCodeM14::CmdResult_t::INVALID_PARAM:
            return "invalid param";
            break;
        case QRCodeM14::CmdResult_t::TIMEOUT:
            return "timeout";
        case QRCodeM14::CmdResult_t::ACK_MISMATCH:
            return "ack mismatch";
            break;
        default:
            return "unknown result";
    }
}

/* -------------------------------------------------------------------------- */
/*                                     API                                    */
/* -------------------------------------------------------------------------- */
void QRCodeM14::startDecode()
{
    const uint8_t cmd[] = {0x32, 0x75, 0x01};
    sendCmd(cmd, sizeof(cmd));
}

void QRCodeM14::stopDecode()
{
    const uint8_t cmd[]     = {0x32, 0x75, 0x02};
    const uint8_t cmd_ack[] = {0x33, 0x75, 0x02, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 150);
}

void QRCodeM14::setTriggerMode(TriggerMode_t mode)
{
    const uint8_t cmd[]     = {0x21, 0x61, 0x41, (uint8_t)mode};
    const uint8_t cmd_ack[] = {0x22, 0x61, 0x41, (uint8_t)mode, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setDecodeDelay(int delay_ms)
{
    uint8_t high_byte       = (delay_ms >> 8) & 0xFF;
    uint8_t low_byte        = delay_ms & 0xFF;
    const uint8_t cmd[]     = {0x21, 0x61, 0x8A, high_byte, low_byte};
    const uint8_t cmd_ack[] = {0x22, 0x61, 0x8A, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setTriggerTimeout(int timeout_ms)
{
    uint8_t high_byte       = (timeout_ms >> 8) & 0xFF;
    uint8_t low_byte        = timeout_ms & 0xFF;
    const uint8_t cmd[]     = {0x21, 0x61, 0x82, high_byte, low_byte};
    const uint8_t cmd_ack[] = {0x22, 0x61, 0x82, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setMotionSensitivity(int level)
{
    const uint8_t cmd[]     = {0x21, 0x61, 0x44, (uint8_t)level};
    const uint8_t cmd_ack[] = {0x22, 0x61, 0x44, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setContinuousDecodeDelay(int delay_ms)
{
    uint8_t high_byte       = (delay_ms >> 8) & 0xFF;
    uint8_t low_byte        = delay_ms & 0xFF;
    const uint8_t cmd[]     = {0x21, 0x61, 0x8C, high_byte, low_byte};
    const uint8_t cmd_ack[] = {0x22, 0x61, 0x8C, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setTriggerDecodeDelay(int delay_ms)
{
    uint8_t high_byte       = (delay_ms >> 8) & 0xFF;
    uint8_t low_byte        = delay_ms & 0xFF;
    const uint8_t cmd[]     = {0x21, 0x61, 0x85, high_byte, low_byte};
    const uint8_t cmd_ack[] = {0x22, 0x61, 0x85, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setSameCodeInterval(int interval_ms)
{
    uint8_t high_byte       = (interval_ms >> 8) & 0xFF;
    uint8_t low_byte        = interval_ms & 0xFF;
    const uint8_t cmd[]     = {0x21, 0x64, 0x82, high_byte, low_byte};
    const uint8_t cmd_ack[] = {0x22, 0x64, 0x82, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setDiffCodeInterval(int interval_ms)
{
    uint8_t high_byte       = (interval_ms >> 8) & 0xFF;
    uint8_t low_byte        = interval_ms & 0xFF;
    const uint8_t cmd[]     = {0x21, 0x64, 0x81, high_byte, low_byte};
    const uint8_t cmd_ack[] = {0x22, 0x64, 0x81, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setSameCodeNoDelay(bool enable)
{
    uint8_t mode            = enable ? 0x01 : 0x00;
    const uint8_t cmd[]     = {0x21, 0x64, 0x43, mode};
    const uint8_t cmd_ack[] = {0x22, 0x64, 0x43, mode, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setFillLightMode(FillLightMode_t mode)
{
    const uint8_t cmd[]     = {0x21, 0x62, 0x41, (uint8_t)mode};
    const uint8_t cmd_ack[] = {0x22, 0x62, 0x41, (uint8_t)mode, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setFillLightBrightness(int brightness)
{
    brightness              = max(0, min(100, brightness));
    const uint8_t cmd[]     = {0x21, 0x62, 0x48, (uint8_t)brightness};
    const uint8_t cmd_ack[] = {0x22, 0x62, 0x48, (uint8_t)brightness, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 100);
}

void QRCodeM14::setPosLightMode(PosLightMode_t mode)
{
    const uint8_t cmd[]     = {0x21, 0x62, 0x42, (uint8_t)mode};
    const uint8_t cmd_ack[] = {0x22, 0x62, 0x42, (uint8_t)mode, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 100);
}

void QRCodeM14::setStartupTone(int mode)
{
    const uint8_t cmd[]     = {0x21, 0x63, 0x45, (uint8_t)mode};
    const uint8_t cmd_ack[] = {0x22, 0x63, 0x45, (uint8_t)mode};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 150);
}

void QRCodeM14::setDecodeSuccessBeep(int count)
{
    const uint8_t cmd[]     = {0x21, 0x63, 0x42, (uint8_t)count};
    const uint8_t cmd_ack[] = {0x22, 0x63, 0x42, (uint8_t)count};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 150);
}

void QRCodeM14::setCaseConversion(int mode)
{
    const uint8_t cmd[]     = {0x21, 0x51, 0x48, (uint8_t)mode};
    const uint8_t cmd_ack[] = {0x22, 0x51, 0x48, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setProtocolFormat(int mode)
{
    const uint8_t cmd[]     = {0x21, 0x51, 0x43, (uint8_t)mode};
    const uint8_t cmd_ack[] = {0x22, 0x51, 0x43, 0x00};
    sendCmd(cmd, sizeof(cmd), cmd_ack, sizeof(cmd_ack), 200);
}

void QRCodeM14::setModeUsbSerial()
{
    const uint8_t cmd[] = {0x21, 0x42, 0x40, 0x02};
    sendCmd(cmd, sizeof(cmd));
}

void QRCodeM14::setModeUsbKeyboard()
{
    const uint8_t cmd[] = {0x21, 0x42, 0x40, 0x01};
    sendCmd(cmd, sizeof(cmd));
}

void QRCodeM14::setModeUsbPos()
{
    const uint8_t cmd[] = {0x21, 0x42, 0x40, 0x03};
    sendCmd(cmd, sizeof(cmd));
}

std::string QRCodeM14::getInfos(uint8_t id)
{
    const uint8_t cmd[] = {0x43, 0x02, id};
    std::vector<uint8_t> response;

    sendCmd(cmd, sizeof(cmd));
    delay(10);
    waitResponse(response);

    uint16_t data_size = checkResponseDataSize(response);
    if (data_size == 0) {
        return "";
    }

    std::string data(response.begin() + 5, response.begin() + 5 + data_size);

    return data;
}
