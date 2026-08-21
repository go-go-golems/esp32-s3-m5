/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <array>
#include <cstdint>
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace nfc_debug {

enum class CommandType : uint8_t {
    ReadOnce,
    SetAutoPoll,
    Probe,
    VerifyRegisters,
    SampleIrqWindow,
    ClearCounters,
    ResetBus,
    SetField,
    Shutdown,
};

struct Command {
    CommandType type = CommandType::Probe;
    uint32_t argument = 0;
};

enum class ReaderState : uint8_t {
    Starting,
    Ready,
    Scanning,
    TagFound,
    NoTag,
    TransportError,
    ProtocolError,
    Stopped,
};

enum class TransportState : uint8_t {
    Unknown,
    Healthy,
    Warning,
    Failed,
};

enum class ProtocolStage : uint8_t {
    None,
    Detect,
    Select,
    Identify,
};

struct TransportCounters {
    uint32_t commands = 0;
    uint32_t succeeded = 0;
    uint32_t failed = 0;
    uint32_t no_tag = 0;
    uint32_t timeouts = 0;
    uint32_t invalid_state = 0;
    uint32_t other_errors = 0;
};

struct LastError {
    esp_err_t code = ESP_OK;
    CommandType command = CommandType::Probe;
    uint32_t elapsed_us = 0;
};

struct Snapshot {
    uint32_t generation = 0;
    int64_t timestamp_us = 0;
    ReaderState reader_state = ReaderState::Stopped;
    TransportState transport_state = TransportState::Unknown;
    ProtocolStage protocol_stage = ProtocolStage::None;
    bool initialized = false;
    bool auto_poll = false;
    bool field_on = false;

    uint8_t chip_type = 0;
    uint8_t chip_revision = 0;
    std::array<uint8_t, 10> uid{};
    uint8_t uid_len = 0;
    uint16_t atqa = 0;
    uint8_t sak = 0;
    std::array<char, 32> type_name{};

    TransportCounters counters{};
    LastError last_error{};
};

class Service {
public:
    Service() = default;
    ~Service();

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    esp_err_t start(i2c_master_bus_handle_t bus);
    void stop();
    bool enqueue(const Command& command, TickType_t wait = 0);
    bool latest(Snapshot& out) const;
    bool running() const { return _task != nullptr; }

private:
    static void task_entry(void* argument);
    void task_loop();
    void initialize_driver();
    void execute(const Command& command);
    void publish();
    void record_result(const Command& command, esp_err_t result, uint32_t elapsed_us);

    i2c_master_bus_handle_t _bus = nullptr;
    QueueHandle_t _commands = nullptr;
    QueueHandle_t _snapshots = nullptr;
    TaskHandle_t _task = nullptr;
    Snapshot _snapshot{};
};

const char* reader_state_name(ReaderState state);

}  // namespace nfc_debug
