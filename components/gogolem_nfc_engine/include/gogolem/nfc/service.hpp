// SPDX-License-Identifier: MIT
//
// gogolem::nfc Service — single-owner FreeRTOS worker that serializes Engine
// access for multi-task applications. UI callbacks and other tasks submit
// commands; the worker executes them one at a time. Snapshots are published
// by value so consumers never touch mutable Engine state.
//
// The Engine is initialize-once (proven on hardware); the Service is therefore
// start-once. If the Engine faults, the snapshot reports it.

#pragma once

#include <atomic>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "gogolem/nfc/engine.hpp"
#include "gogolem/nfc/result.hpp"
#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc {

struct ServiceConfig {
    EngineConfig engine;
    uint32_t command_queue_depth{8};
    uint32_t worker_stack_size{8192};
    UBaseType_t worker_priority{5};
};

enum class ServiceCommand : uint8_t {
    None = 0,
    Scan,
    ActivateOne,
    RawRead,
    ReadNdef,
    Dump,
    Shutdown,
};

struct Command {
    ServiceCommand kind{ServiceCommand::None};
    uint8_t address{0};  // for RawRead
};

struct ServiceSnapshot {
    LifecycleState engine_state{LifecycleState::New};
    Mode mode{Mode::Reader};
    bool tag_present{false};
    TagInfo last_tag{};
    ActivationSource last_source{ActivationSource::REQA};
    uint32_t operations{0};
    uint32_t failures{0};
    Error last_error{};
    bool raw_read_ok{false};
    std::array<uint8_t, 16> raw_read_data{};
    bool ndef_ok{false};
    uint32_t ndef_records{0};
    bool dump_ok{false};
};

class Service {
public:
    Service();
    ~Service();

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    Result<void> start(const ServiceConfig& config);
    Result<void> stop(TickType_t timeout = pdMS_TO_TICKS(5000));
    bool submit(const Command& cmd, TickType_t wait = 0);
    bool latest(ServiceSnapshot& out) const;
    bool running() const { return worker_ != nullptr; }

private:
    static void task_entry(void* arg);
    void task_loop();
    void execute(const Command& cmd);
    void publish();

    Engine engine_;
    ServiceConfig config_{};
    QueueHandle_t commands_{};
    QueueHandle_t snapshots_{};
    TaskHandle_t worker_{};
    std::atomic<bool> stopping_{false};
    ServiceSnapshot snapshot_{};
    mutable SemaphoreHandle_t snapshot_mutex_{};
};

}  // namespace gogolem::nfc
