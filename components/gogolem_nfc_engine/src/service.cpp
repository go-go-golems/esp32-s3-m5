// SPDX-License-Identifier: MIT

#include "gogolem/nfc/service.hpp"

#include <cstring>

namespace gogolem::nfc {

Service::Service() {
    snapshot_mutex_ = xSemaphoreCreateMutex();
}

Service::~Service() {
    stop(pdMS_TO_TICKS(1000));
    if (snapshot_mutex_) vSemaphoreDelete(snapshot_mutex_);
}

Result<void> Service::start(const ServiceConfig& config) {
    if (worker_ != nullptr) {
        Error e;
        e.layer = ErrorLayer::Lifecycle;
        e.operation = Operation::Begin;
        e.set_detail("already started");
        return Result<void>::failure(e);
    }
    config_ = config;

    commands_ = xQueueCreate(config_.command_queue_depth, sizeof(Command));
    snapshots_ = xQueueCreate(1, sizeof(ServiceSnapshot));
    if (!commands_ || !snapshots_) {
        Error e;
        e.layer = ErrorLayer::Internal;
        e.operation = Operation::Begin;
        e.set_detail("queue creation failed");
        return Result<void>::failure(e);
    }

    auto begin = engine_.begin(config_.engine);
    if (!begin.ok()) {
        return begin;
    }

    stopping_ = false;
    snapshot_.engine_state = engine_.state();
    snapshot_.mode = engine_.mode();

    if (xTaskCreate(task_entry, "gogolem_nfc_svc", config_.worker_stack_size, this,
                    config_.worker_priority, &worker_) != pdPASS) {
        engine_.end();
        Error e;
        e.layer = ErrorLayer::Internal;
        e.operation = Operation::Begin;
        e.set_detail("task creation failed");
        return Result<void>::failure(e);
    }

    return Result<void>::success();
}

Result<void> Service::stop(TickType_t timeout) {
    if (worker_ == nullptr) {
        return Result<void>::success();
    }
    stopping_ = true;
    Command shutdown{};
    shutdown.kind = ServiceCommand::Shutdown;
    submit(shutdown, pdMS_TO_TICKS(100));

    // Wait for the worker to exit.
    TickType_t deadline = xTaskGetTickCount() + timeout;
    while (worker_ != nullptr && xTaskGetTickCount() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (commands_) vQueueDelete(commands_);
    if (snapshots_) vQueueDelete(snapshots_);
    commands_ = nullptr;
    snapshots_ = nullptr;
    worker_ = nullptr;
    return Result<void>::success();
}

bool Service::submit(const Command& cmd, TickType_t wait) {
    if (!commands_ || stopping_) return false;
    return xQueueSend(commands_, &cmd, wait) == pdTRUE;
}

bool Service::latest(ServiceSnapshot& out) const {
    if (!snapshot_mutex_) return false;
    if (xSemaphoreTake(snapshot_mutex_, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    out = snapshot_;
    xSemaphoreGive(snapshot_mutex_);
    return true;
}

void Service::task_entry(void* arg) {
    static_cast<Service*>(arg)->task_loop();
}

void Service::task_loop() {
    Command cmd{};
    while (!stopping_) {
        if (xQueueReceive(commands_, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (cmd.kind == ServiceCommand::Shutdown) break;
            execute(cmd);
            publish();
        }
    }
    engine_.end();
    worker_ = nullptr;
    vTaskDelete(nullptr);
}

void Service::execute(const Command& cmd) {
    ++snapshot_.operations;
    snapshot_.last_error = Error{};

    // No-tag is a valid outcome (Rf + NOT_FOUND), not a failure. Only count
    // real errors (transport, protocol, chip-state, etc.) as failures.
    auto count_failure = [this](const Error& e) {
        if (e.layer == ErrorLayer::Rf && e.esp_code == ESP_CODE_ERR_NOT_FOUND) {
            // No tag present — valid outcome, not a failure.
            return;
        }
        if (e.layer == ErrorLayer::CardFamily) {
            // Tag doesn't support the operation — valid for wrong-family ops.
            return;
        }
        snapshot_.last_error = e;
        ++snapshot_.failures;
    };

    switch (cmd.kind) {
        case ServiceCommand::Scan: {
            auto r = engine_.scan(1000);
            snapshot_.tag_present = r.ok() && !r.value().tags.empty();
            if (r.ok() && !r.value().tags.empty()) {
                snapshot_.last_tag = r.value().tags[0];
            }
            if (!r.ok()) {
                snapshot_.last_error = r.error();
                ++snapshot_.failures;
            }
            break;
        }
        case ServiceCommand::ActivateOne: {
            auto r = engine_.activate_one();
            snapshot_.tag_present = r.ok();
            if (r.ok()) {
                snapshot_.last_tag = r.value().tag;
                snapshot_.last_source = r.value().source;
                engine_.deactivate();
            } else {
                count_failure(r.error());
            }
            break;
        }
        case ServiceCommand::RawRead: {
            auto r = engine_.raw_read(cmd.address);
            snapshot_.raw_read_ok = r.ok();
            if (r.ok() && r.value().size() <= 16) {
                std::memcpy(snapshot_.raw_read_data.data(), r.value().data(), r.value().size());
            } else if (!r.ok()) {
                count_failure(r.error());
            }
            break;
        }
        case ServiceCommand::ReadNdef: {
            auto r = engine_.read_ndef();
            snapshot_.ndef_ok = r.ok();
            if (r.ok()) {
                snapshot_.ndef_records = static_cast<uint32_t>(r.value().records.size());
            } else {
                count_failure(r.error());
            }
            break;
        }
        case ServiceCommand::Dump: {
            auto r = engine_.dump();
            snapshot_.dump_ok = r.ok();
            if (!r.ok()) {
                count_failure(r.error());
            }
            break;
        }
        default:
            break;
    }
    snapshot_.engine_state = engine_.state();
}

void Service::publish() {
    if (!snapshot_mutex_) return;
    if (xSemaphoreTake(snapshot_mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Snapshot is already updated in execute(); just signal availability.
        xSemaphoreGive(snapshot_mutex_);
    }
    // Also push to the snapshots queue for event-driven consumers.
    if (snapshots_) {
        xQueueOverwrite(snapshots_, &snapshot_);
    }
}

}  // namespace gogolem::nfc
