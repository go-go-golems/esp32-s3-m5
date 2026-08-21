/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#include "nfc_debug_service.h"

#include "st25r3916/st25r3916.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <esp_log.h>
#include <esp_timer.h>

namespace nfc_debug {
namespace {
constexpr char TAG[] = "nfc-debug-service";
constexpr uint32_t AUTO_POLL_INTERVAL_MS = 333;
constexpr uint32_t SAMPLE_INTERVAL_MS = 200;
constexpr uint32_t SAMPLE_DURATION_MS = 10000;
constexpr uint32_t STOP_TIMEOUT_MS = 2000;
constexpr uint8_t IRQ_RXS = 0x20;
constexpr uint8_t IRQ_RXE = 0x10;

bool is_transport_error(esp_err_t error)
{
    return error == ESP_ERR_TIMEOUT || error == ESP_ERR_INVALID_STATE ||
           error == ESP_ERR_INVALID_RESPONSE;
}

const char* command_name(CommandType type)
{
    switch (type) {
    case CommandType::ReadOnce: return "READ_ONCE";
    case CommandType::SetAutoPoll: return "SET_AUTO";
    case CommandType::Probe: return "PROBE";
    case CommandType::VerifyRegisters: return "VERIFY";
    case CommandType::SampleIrqWindow: return "SAMPLE_IRQ";
    case CommandType::ClearCounters: return "CLEAR_COUNTERS";
    case CommandType::ResetBus: return "REINIT_NFC";
    case CommandType::SetField: return "SET_FIELD";
    case CommandType::Shutdown: return "SHUTDOWN";
    }
    return "UNKNOWN";
}

void format_uid(const Snapshot& snapshot, char* output, size_t capacity)
{
    if (capacity == 0) return;
    size_t offset = 0;
    output[0] = '\0';
    for (uint8_t i = 0; i < snapshot.uid_len && offset < capacity; ++i) {
        const int written = snprintf(output + offset, capacity - offset,
                                     i == 0 ? "%02X" : ":%02X", snapshot.uid[i]);
        if (written < 0 || static_cast<size_t>(written) >= capacity - offset) break;
        offset += static_cast<size_t>(written);
    }
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
    _sample_deadline_us = 0;
    _sample_next_us = 0;
    _sample_wupa = false;
    _verification_remaining = 0;
    publish();

    ESP_LOGI(TAG, "NFC_SERVICE event=start queue_depth=8 stack=8192 priority=5");
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
        const bool background = _snapshot.auto_poll || _snapshot.sample_active ||
                                _snapshot.verification_active;
        Command command{};
        const TickType_t wait = background ? pdMS_TO_TICKS(20) : portMAX_DELAY;
        if (xQueueReceive(_commands, &command, wait) == pdTRUE) {
            if (command.type == CommandType::Shutdown) {
                stopping = true;
            } else {
                execute(command);
            }
        }

        if (stopping) break;
        const int64_t now = esp_timer_get_time();
        if (_snapshot.sample_active && now >= _sample_next_us) {
            run_sample_step();
        } else if (_snapshot.verification_active) {
            run_verification_step();
        } else if (_snapshot.auto_poll && now >= next_auto_poll_us) {
            execute(Command{CommandType::ReadOnce, 0});
            next_auto_poll_us = now + AUTO_POLL_INTERVAL_MS * 1000LL;
        }
    }

    _snapshot.auto_poll = false;
    _snapshot.sample_active = false;
    _snapshot.verification_active = false;
    if (_snapshot.initialized) {
        (void)st25r3916_field_off();
        refresh_driver_snapshot(false);
    }
    st25r3916_deinit();
    _snapshot.initialized = false;
    _snapshot.field_on = false;
    _snapshot.reader_state = ReaderState::Stopped;
    publish();
    ESP_LOGI(TAG,
             "NFC_SERVICE event=stopped commands=%lu txns=%lu ok=%lu failed=%lu no_tag=%lu",
             (unsigned long)_snapshot.commands_executed,
             (unsigned long)_snapshot.counters.transactions,
             (unsigned long)_snapshot.counters.succeeded,
             (unsigned long)_snapshot.counters.failed,
             (unsigned long)_snapshot.no_tag_count);

    _task = nullptr;
    vTaskDelete(nullptr);
}

void Service::initialize_driver()
{
    ESP_LOGI(TAG, "NFC_INIT event=begin bus=%p address=0x50 frequency_hz=400000", _bus);
    st25r3916_reset_transport_stats();
    const int64_t started = esp_timer_get_time();
    const esp_err_t result = st25r3916_init(_bus);
    const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - started);
    _snapshot.commands_executed++;

    if (result == ESP_OK) {
        _snapshot.initialized = true;
        _snapshot.reader_state = ReaderState::Ready;
        _snapshot.transport_state = TransportState::Healthy;

        st25r3916_id_t id{};
        if (st25r3916_read_id(&id) == ESP_OK) {
            _snapshot.chip_type = id.type;
            _snapshot.chip_revision = id.revision;
        }
        (void)st25r3916_measure_capacitance();
        refresh_driver_snapshot(true);
        ESP_LOGI(TAG,
                 "NFC_INIT event=ready type=0x%02X revision=0x%02X capacitance=%u elapsed_us=%lu txns=%lu failed=%lu",
                 _snapshot.chip_type, _snapshot.chip_revision, _snapshot.rf.capacitance,
                 (unsigned long)elapsed, (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed);
    } else {
        refresh_driver_snapshot(false);
        record_result(Command{CommandType::Probe, 0}, result, elapsed);
        ESP_LOGE(TAG,
                 "NFC_INIT event=failed err=%s(0x%x) elapsed_us=%lu txns=%lu failed=%lu last_op=%u last_key=0x%02X",
                 esp_err_to_name(result), (unsigned)result, (unsigned long)elapsed,
                 (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed,
                 _snapshot.last_error.transport_operation, _snapshot.last_error.transport_key);
        st25r3916_deinit();
    }
    publish();
}

void Service::execute(const Command& command)
{
    if (command.type == CommandType::SetAutoPoll) {
        _snapshot.auto_poll = command.argument != 0;
        ESP_LOGI(TAG, "NFC_CONTROL command=SET_AUTO enabled=%u",
                 _snapshot.auto_poll ? 1U : 0U);
        if (_snapshot.reader_state != ReaderState::TransportError &&
            _snapshot.reader_state != ReaderState::ProtocolError) {
            _snapshot.reader_state = ReaderState::Ready;
        }
        publish();
        return;
    }
    if (command.type == CommandType::ClearCounters) {
        ESP_LOGI(TAG,
                 "NFC_CONTROL command=CLEAR_COUNTERS previous_txns=%lu previous_failed=%lu previous_no_tag=%lu",
                 (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed,
                 (unsigned long)_snapshot.no_tag_count);
        st25r3916_reset_transport_stats();
        _snapshot.counters = {};
        _snapshot.last_error = {};
        _snapshot.commands_executed = 0;
        _snapshot.no_tag_count = 0;
        _snapshot.sample_attempts = 0;
        _snapshot.sample_events = 0;
        _snapshot.verification_passes = 0;
        _snapshot.transport_state = TransportState::Healthy;
        refresh_driver_snapshot(false);
        publish();
        return;
    }
    if (command.type == CommandType::VerifyRegisters) {
        _verification_remaining = static_cast<uint8_t>(
            std::clamp<uint32_t>(command.argument == 0 ? 20 : command.argument, 1, 100));
        _snapshot.verification_active = true;
        _snapshot.verification_passes = 0;
        ESP_LOGI(TAG, "NFC_VERIFY event=begin requested_passes=%u",
                 (unsigned)_verification_remaining);
        _snapshot.counters.mismatches = 0;
        publish();
        return;
    }
    if (command.type == CommandType::SampleIrqWindow) {
        const int64_t now = esp_timer_get_time();
        _sample_deadline_us = now + SAMPLE_DURATION_MS * 1000LL;
        _sample_next_us = now;
        _sample_wupa = false;
        _snapshot.sample_active = true;
        _snapshot.sample_attempts = 0;
        ESP_LOGI(TAG, "NFC_SAMPLE event=begin duration_ms=%u interval_ms=%u",
                 SAMPLE_DURATION_MS, SAMPLE_INTERVAL_MS);
        _snapshot.sample_events = 0;
        publish();
        return;
    }

    _snapshot.commands_executed++;
    const uint32_t failed_before = _snapshot.counters.failed;
    const int64_t started = esp_timer_get_time();
    esp_err_t result = ESP_ERR_NOT_SUPPORTED;
    bool read_rf = false;

    if (!_snapshot.initialized && command.type != CommandType::ResetBus) {
        result = ESP_ERR_INVALID_STATE;
    } else {
        switch (command.type) {
        case CommandType::ReadOnce: {
            _snapshot.reader_state = ReaderState::Scanning;
            _snapshot.protocol_stage = ProtocolStage::None;
            publish();

            nfc_picc_t picc{};
            result = st25r3916_poll_nfca(&picc);
            read_rf = true;
            if (result != ESP_ERR_INVALID_STATE && result != ESP_ERR_TIMEOUT) {
                _snapshot.field_on = true;
            }
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
            read_rf = true;
            break;
        case CommandType::ResetBus:
            if (_snapshot.initialized) (void)st25r3916_field_off();
            st25r3916_deinit();
            _snapshot.initialized = false;
            result = st25r3916_init(_bus);
            if (result == ESP_OK) {
                _snapshot.initialized = true;
                st25r3916_id_t id{};
                if (st25r3916_read_id(&id) == ESP_OK) {
                    _snapshot.chip_type = id.type;
                    _snapshot.chip_revision = id.revision;
                }
                (void)st25r3916_measure_capacitance();
                _snapshot.reader_state = ReaderState::Ready;
                read_rf = true;
            } else {
                st25r3916_deinit();
            }
            break;
        case CommandType::VerifyRegisters:
        case CommandType::SampleIrqWindow:
        case CommandType::SetAutoPoll:
        case CommandType::ClearCounters:
        case CommandType::Shutdown:
            break;
        }
    }

    const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - started);
    refresh_driver_snapshot(read_rf);
    if (result != ESP_OK && result != ESP_ERR_NOT_FOUND &&
        _snapshot.counters.failed == failed_before) {
        _snapshot.last_error.transport_operation = 0;
        _snapshot.last_error.transport_key = 0;
        _snapshot.last_error.elapsed_us = elapsed;
    }
    record_result(command, result, elapsed);

    const uint32_t transport_failures = _snapshot.counters.failed - failed_before;
    if (result == ESP_OK && command.type == CommandType::ReadOnce) {
        char uid[3 * 10]{};
        format_uid(_snapshot, uid, sizeof(uid));
        ESP_LOGI(TAG,
                 "NFC_READ result=tag uid=%s uid_len=%u atqa=0x%04X sak=0x%02X type=\"%s\" elapsed_us=%lu txns=%lu failed=%lu delta_failed=%lu",
                 uid, _snapshot.uid_len, _snapshot.atqa, _snapshot.sak,
                 _snapshot.type_name.data(), (unsigned long)elapsed,
                 (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed,
                 (unsigned long)transport_failures);
    } else if (result == ESP_ERR_NOT_FOUND) {
        ESP_LOGD(TAG,
                 "NFC_READ result=no_tag count=%lu elapsed_us=%lu txns=%lu failed=%lu delta_failed=%lu irq=%06lX fifo=%u",
                 (unsigned long)_snapshot.no_tag_count, (unsigned long)elapsed,
                 (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed,
                 (unsigned long)transport_failures,
                 (unsigned long)_snapshot.rf.main_irq, _snapshot.rf.fifo_bytes);
        if (_snapshot.no_tag_count == 1 || (_snapshot.no_tag_count % 10) == 0) {
            ESP_LOGI(TAG,
                     "NFC_READ result=no_tag count=%lu elapsed_us=%lu txns=%lu failed=%lu delta_failed=%lu",
                     (unsigned long)_snapshot.no_tag_count, (unsigned long)elapsed,
                     (unsigned long)_snapshot.counters.transactions,
                     (unsigned long)_snapshot.counters.failed,
                     (unsigned long)transport_failures);
        }
    } else if (result != ESP_OK) {
        ESP_LOG_LEVEL(is_transport_error(result) ? ESP_LOG_ERROR : ESP_LOG_WARN, TAG,
                      "NFC_COMMAND command=%s result=failed err=%s(0x%x) state=%s elapsed_us=%lu txns=%lu failed=%lu delta_failed=%lu last_op=%u last_key=0x%02X last_elapsed_us=%lu",
                      command_name(command.type), esp_err_to_name(result), (unsigned)result,
                      reader_state_name(_snapshot.reader_state), (unsigned long)elapsed,
                      (unsigned long)_snapshot.counters.transactions,
                      (unsigned long)_snapshot.counters.failed,
                      (unsigned long)transport_failures,
                      _snapshot.last_error.transport_operation,
                      _snapshot.last_error.transport_key,
                      (unsigned long)_snapshot.last_error.elapsed_us);
    } else {
        ESP_LOGI(TAG,
                 "NFC_COMMAND command=%s result=ok argument=%lu elapsed_us=%lu txns=%lu failed=%lu delta_failed=%lu",
                 command_name(command.type), (unsigned long)command.argument,
                 (unsigned long)elapsed, (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed,
                 (unsigned long)transport_failures);
    }

    /* A command can return a protocol/no-tag result after an internal I2C
     * failure was swallowed by a diagnostic read. Always emit that mismatch. */
    if (transport_failures != 0 && !is_transport_error(result)) {
        ESP_LOGE(TAG,
                 "NFC_COMMAND command=%s result=%s hidden_transport_failures=%lu last_op=%u last_key=0x%02X last_err=%s(0x%x)",
                 command_name(command.type), esp_err_to_name(result),
                 (unsigned long)transport_failures,
                 _snapshot.last_error.transport_operation,
                 _snapshot.last_error.transport_key,
                 esp_err_to_name(_snapshot.last_error.code),
                 (unsigned)_snapshot.last_error.code);
    }
    publish();
}

void Service::run_sample_step()
{
    const int64_t now = esp_timer_get_time();
    if (now >= _sample_deadline_us) {
        _snapshot.sample_active = false;
        ESP_LOGI(TAG,
                 "NFC_SAMPLE event=complete attempts=%u events=%u txns=%lu failed=%lu irq=%06lX timer=%02X error=%02X",
                 _snapshot.sample_attempts, _snapshot.sample_events,
                 (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed,
                 (unsigned long)_snapshot.rf.main_irq,
                 _snapshot.rf.timer_irq, _snapshot.rf.error_irq);
        publish();
        return;
    }

    uint16_t atqa = 0;
    const esp_err_t result = _sample_wupa ? st25r3916_wupa(&atqa) : st25r3916_reqa(&atqa);
    _sample_wupa = !_sample_wupa;
    _snapshot.sample_attempts++;
    _snapshot.field_on = true;
    refresh_driver_snapshot(true);
    if (result == ESP_OK || (_snapshot.rf.main_irq & (IRQ_RXS | IRQ_RXE)) != 0) {
        _snapshot.sample_events++;
    }
    if (result != ESP_OK && result != ESP_ERR_NOT_FOUND && result != ESP_FAIL) {
        record_result(Command{CommandType::SampleIrqWindow, 0}, result, 0);
    }
    _sample_next_us = now + SAMPLE_INTERVAL_MS * 1000LL;
    publish();
}

void Service::run_verification_step()
{
    if (_verification_remaining == 0) {
        _snapshot.verification_active = false;
        ESP_LOGI(TAG,
                 "NFC_VERIFY event=complete passes=%u mismatches=%lu txns=%lu failed=%lu",
                 _snapshot.verification_passes,
                 (unsigned long)_snapshot.counters.mismatches,
                 (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed);
        publish();
        return;
    }

    st25r3916_register_check_t checks[12]{};
    size_t count = 0;
    const int64_t started = esp_timer_get_time();
    const esp_err_t result = st25r3916_verify_configuration(checks, 12, &count);
    const uint32_t elapsed = static_cast<uint32_t>(esp_timer_get_time() - started);
    _snapshot.register_count = static_cast<uint8_t>(std::min<size_t>(count, _snapshot.registers.size()));
    for (uint8_t i = 0; i < _snapshot.register_count; ++i) {
        auto& destination = _snapshot.registers[i];
        std::strncpy(destination.name.data(), checks[i].name, destination.name.size() - 1);
        destination.space_b = checks[i].space_b;
        destination.address = checks[i].address;
        destination.expected = checks[i].expected;
        destination.actual = checks[i].actual;
        destination.error = checks[i].error;
        if (checks[i].error != ESP_OK || checks[i].actual != checks[i].expected) {
            _snapshot.counters.mismatches++;
        }
    }

    _snapshot.verification_passes++;
    _verification_remaining--;
    if (_verification_remaining == 0) _snapshot.verification_active = false;
    refresh_driver_snapshot(false);
    if (!_snapshot.verification_active) {
        ESP_LOGI(TAG,
                 "NFC_VERIFY event=complete passes=%u mismatches=%lu txns=%lu failed=%lu",
                 _snapshot.verification_passes,
                 (unsigned long)_snapshot.counters.mismatches,
                 (unsigned long)_snapshot.counters.transactions,
                 (unsigned long)_snapshot.counters.failed);
    }
    if (result != ESP_OK) {
        record_result(Command{CommandType::VerifyRegisters, 0}, result, elapsed);
    }
    publish();
}

void Service::refresh_driver_snapshot(bool read_rf)
{
    if (read_rf && _snapshot.initialized) {
        st25r3916_diagnostics_t diagnostics{};
        (void)st25r3916_get_diagnostics(&diagnostics);
        _snapshot.rf.operation_control = diagnostics.operation_control;
        _snapshot.rf.rssi = diagnostics.rssi;
        _snapshot.rf.nrt = diagnostics.nrt;
        _snapshot.rf.main_irq = diagnostics.main_irq;
        _snapshot.rf.timer_irq = diagnostics.timer_irq;
        _snapshot.rf.error_irq = diagnostics.error_irq;
        _snapshot.rf.collision = diagnostics.collision;
        _snapshot.rf.fifo_bytes = diagnostics.fifo_bytes;
        _snapshot.rf.capacitance = diagnostics.capacitance;
    }

    st25r3916_transport_stats_t stats{};
    st25r3916_get_transport_stats(&stats);
    _snapshot.counters.transactions = stats.total;
    _snapshot.counters.succeeded = stats.succeeded;
    _snapshot.counters.failed = stats.failed;
    _snapshot.counters.timeouts = stats.timeouts;
    _snapshot.counters.invalid_state = stats.invalid_state;
    _snapshot.counters.other_errors = stats.other_errors;
    if (stats.last_error != ESP_OK) {
        _snapshot.last_error.code = stats.last_error;
        _snapshot.last_error.transport_operation = static_cast<uint8_t>(stats.last_operation);
        _snapshot.last_error.transport_key = stats.last_key;
        _snapshot.last_error.elapsed_us = stats.last_elapsed_us;
    }
}

void Service::record_result(const Command& command, esp_err_t result, uint32_t elapsed_us)
{
    if (result == ESP_OK) {
        _snapshot.transport_state = _snapshot.counters.failed == 0
            ? TransportState::Healthy : TransportState::Warning;
        return;
    }

    if (result == ESP_ERR_NOT_FOUND) {
        _snapshot.no_tag_count++;
        _snapshot.reader_state = ReaderState::NoTag;
        _snapshot.transport_state = _snapshot.counters.failed == 0
            ? TransportState::Healthy : TransportState::Warning;
        return;
    }

    _snapshot.last_error.code = result;
    _snapshot.last_error.command = command.type;
    if (_snapshot.last_error.transport_operation == 0) {
        _snapshot.last_error.elapsed_us = elapsed_us;
    }

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
