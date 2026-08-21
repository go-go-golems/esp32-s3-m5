/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#include "nfc_debug_service.h"

#include "st25r3916/st25r3916.h"
#include <algorithm>
#include <cstring>
#include <esp_log.h>
#include <esp_timer.h>

namespace nfc_debug {
namespace {
constexpr char TAG[] = "nfc-debug-service";
constexpr uint32_t AUTO_POLL_INTERVAL_MS = 333;
constexpr uint32_t STOP_TIMEOUT_MS = 2000;

bool is_transport_error(esp_err_t error)
{
    return error == ESP_ERR_TIMEOUT || error == ESP_ERR_INVALID_STATE ||
           error == ESP_ERR_INVALID_RESPONSE;
}
}  // namespace

Service::~Service()
{
    stop();
}

esp_err_t Service::start(i2c_master_bus_handle_t bus)
{
    if (bus == nullptr) return ESP_ERR_INVALID_ARG;
    if (_task != nullptr) return ESP_ERR_INVALID_STATE;

    _commands = xQueueCreate(8, sizeof(Command));
    _snapshots = xQueueCreate(1, sizeof(Snapshot));
    if (_commands == nullptr || _snapshots == nullptr) {
        if (_commands != nullptr) vQueueDelete(_commands);
        if (_snapshots != nullptr) vQueueDelete(_snapshots);
        _commands = nullptr;
        _snapshots = nullptr;
        return ESP_ERR_NO_MEM;
    }

    _bus = bus;
    _snapshot = {};
    _snapshot.reader_state = ReaderState::Starting;
    _snapshot.timestamp_us = esp_timer_get_time();
    publish();

    BaseType_t created = xTaskCreate(
        task_entry, "nfc-debug", 8192, this, 5, &_task);
    if (created != pdPASS) {
        vQueueDelete(_commands);
        vQueueDelete(_snapshots);
        _commands = nullptr;
        _snapshots = nullptr;
        _task = nullptr;
        _bus = nullptr;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void Service::stop()
{
    if (_task != nullptr && _commands != nullptr) {
        const Command stop_command{CommandType::Shutdown, 0};
        (void)xQueueSend(_commands, &stop_command, pdMS_TO_TICKS(100));
        const int64_t deadline = esp_timer_get_time() + STOP_TIMEOUT_MS * 1000LL;
        while (_task != nullptr && esp_timer_get_time() < deadline) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (_task != nullptr) {
            ESP_LOGE(TAG, "worker did not stop within %u ms", STOP_TIMEOUT_MS);
            return;
        }
    }

    if (_commands != nullptr) vQueueDelete(_commands);
    if (_snapshots != nullptr) vQueueDelete(_snapshots);
    _commands = nullptr;
    _snapshots = nullptr;
    _bus = nullptr;
}

bool Service::enqueue(const Command& command, TickType_t wait)
{
    return _commands != nullptr && xQueueSend(_commands, &command, wait) == pdTRUE;
}

bool Service::latest(Snapshot& out) const
{
    return _snapshots != nullptr && xQueuePeek(_snapshots, &out, 0) == pdTRUE;
}

void Service::task_entry(void* argument)
{
    static_cast<Service*>(argument)->task_loop();
}

void Service::task_loop()
{
    initialize_driver();
    int64_t next_auto_poll_us = esp_timer_get_time();
    bool stopping = false;

    while (!stopping) {
        Command command{};
        const TickType_t wait = _snapshot.auto_poll ? pdMS_TO_TICKS(50) : portMAX_DELAY;
        if (xQueueReceive(_commands, &command, wait) == pdTRUE) {
            if (command.type == CommandType::Shutdown) {
                stopping = true;
            } else {
                execute(command);
            }
        }

        const int64_t now = esp_timer_get_time();
        if (!stopping && _snapshot.auto_poll && now >= next_auto_poll_us) {
            execute(Command{CommandType::ReadOnce, 0});
            next_auto_poll_us = now + AUTO_POLL_INTERVAL_MS * 1000LL;
        }
    }

    if (_snapshot.initialized) {
        (void)st25r3916_field_off();
        st25r3916_deinit();
    }
    _snapshot.initialized = false;
    _snapshot.auto_poll = false;
    _snapshot.field_on = false;
    _snapshot.reader_state = ReaderState::Stopped;
    publish();

    _task = nullptr;
    vTaskDelete(nullptr);
}

void Service::initialize_driver()
{
    const int64_t started = esp_timer_get_time();
    const esp_err_t result = st25r3916_init(_bus);
    const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - started);

    _snapshot.counters.commands++;
    if (result == ESP_OK) {
        _snapshot.initialized = true;
        _snapshot.reader_state = ReaderState::Ready;
        _snapshot.transport_state = TransportState::Healthy;
        _snapshot.counters.succeeded++;

        st25r3916_id_t id{};
        if (st25r3916_read_id(&id) == ESP_OK) {
            _snapshot.chip_type = id.type;
            _snapshot.chip_revision = id.revision;
        }
    } else {
        record_result(Command{CommandType::Probe, 0}, result, elapsed);
    }
    publish();
}

void Service::execute(const Command& command)
{
    if (command.type == CommandType::SetAutoPoll) {
        _snapshot.auto_poll = command.argument != 0;
        _snapshot.reader_state = ReaderState::Ready;
        publish();
        return;
    }
    if (command.type == CommandType::ClearCounters) {
        _snapshot.counters = {};
        _snapshot.last_error = {};
        _snapshot.transport_state = TransportState::Healthy;
        publish();
        return;
    }

    _snapshot.counters.commands++;
    const int64_t started = esp_timer_get_time();
    esp_err_t result = ESP_ERR_NOT_SUPPORTED;

    if (!_snapshot.initialized) {
        result = ESP_ERR_INVALID_STATE;
    } else {
        switch (command.type) {
        case CommandType::ReadOnce: {
            _snapshot.reader_state = ReaderState::Scanning;
            _snapshot.protocol_stage = ProtocolStage::None;
            publish();

            nfc_picc_t picc{};
            result = st25r3916_poll_nfca(&picc);
            if (result == ESP_OK) {
                _snapshot.uid.fill(0);
                _snapshot.uid_len = std::min<uint8_t>(picc.uid_len, _snapshot.uid.size());
                std::copy_n(picc.uid, _snapshot.uid_len, _snapshot.uid.begin());
                _snapshot.atqa = picc.atqa;
                _snapshot.sak = picc.sak;
                std::strncpy(_snapshot.type_name.data(), picc.type_str,
                             _snapshot.type_name.size() - 1);
                _snapshot.protocol_stage = ProtocolStage::Identify;
                _snapshot.reader_state = ReaderState::TagFound;
                _snapshot.field_on = true;
            }
            break;
        }
        case CommandType::Probe: {
            st25r3916_id_t id{};
            result = st25r3916_read_id(&id);
            if (result == ESP_OK) {
                _snapshot.chip_type = id.type;
                _snapshot.chip_revision = id.revision;
                _snapshot.reader_state = ReaderState::Ready;
            }
            break;
        }
        case CommandType::SetField:
            result = command.argument ? st25r3916_field_on() : st25r3916_field_off();
            if (result == ESP_OK) _snapshot.field_on = command.argument != 0;
            break;
        case CommandType::VerifyRegisters:
        case CommandType::SampleIrqWindow:
        case CommandType::ResetBus:
            result = ESP_ERR_NOT_SUPPORTED;
            break;
        case CommandType::SetAutoPoll:
        case CommandType::ClearCounters:
        case CommandType::Shutdown:
            break;
        }
    }

    const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - started);
    record_result(command, result, elapsed);
    publish();
}

void Service::record_result(const Command& command, esp_err_t result, uint32_t elapsed_us)
{
    if (result == ESP_OK) {
        _snapshot.counters.succeeded++;
        _snapshot.transport_state = TransportState::Healthy;
        return;
    }

    if (result == ESP_ERR_NOT_FOUND) {
        _snapshot.counters.no_tag++;
        _snapshot.reader_state = ReaderState::NoTag;
        _snapshot.transport_state = TransportState::Healthy;
        return;
    }

    _snapshot.counters.failed++;
    _snapshot.last_error = LastError{result, command.type, elapsed_us};
    if (result == ESP_ERR_TIMEOUT) _snapshot.counters.timeouts++;
    else if (result == ESP_ERR_INVALID_STATE) _snapshot.counters.invalid_state++;
    else _snapshot.counters.other_errors++;

    if (is_transport_error(result)) {
        _snapshot.reader_state = ReaderState::TransportError;
        _snapshot.transport_state = TransportState::Failed;
    } else {
        _snapshot.reader_state = ReaderState::ProtocolError;
        _snapshot.transport_state = TransportState::Warning;
    }
}

void Service::publish()
{
    _snapshot.generation++;
    _snapshot.timestamp_us = esp_timer_get_time();
    if (_snapshots != nullptr) xQueueOverwrite(_snapshots, &_snapshot);
}

const char* reader_state_name(ReaderState state)
{
    switch (state) {
    case ReaderState::Starting: return "STARTING";
    case ReaderState::Ready: return "READY";
    case ReaderState::Scanning: return "SCANNING";
    case ReaderState::TagFound: return "TAG FOUND";
    case ReaderState::NoTag: return "NO TAG";
    case ReaderState::TransportError: return "TRANSPORT ERROR";
    case ReaderState::ProtocolError: return "PROTOCOL ERROR";
    case ReaderState::Stopped: return "STOPPED";
    }
    return "UNKNOWN";
}

}  // namespace nfc_debug
