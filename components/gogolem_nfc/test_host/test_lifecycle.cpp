// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc lifecycle rules. No ESP-IDF required.

#include <cstdio>
#include <cstring>

#include "gogolem/nfc/lifecycle.hpp"
#include "gogolem/nfc/types.hpp"

using namespace gogolem::nfc;

static int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);    \
            ++failures;                                                   \
            return 1;                                                     \
        }                                                                 \
    } while (0)

static int test_predicates() {
    CHECK(lifecycle_can_begin(LifecycleState::New));
    CHECK(lifecycle_can_begin(LifecycleState::Stopped));
    CHECK(!lifecycle_can_begin(LifecycleState::ReadyReader));
    CHECK(!lifecycle_can_begin(LifecycleState::Initializing));
    CHECK(!lifecycle_can_begin(LifecycleState::Faulted));

    CHECK(lifecycle_can_end(LifecycleState::Initializing));
    CHECK(lifecycle_can_end(LifecycleState::ReadyReader));
    CHECK(lifecycle_can_end(LifecycleState::ReadyTarget));
    CHECK(lifecycle_can_end(LifecycleState::Busy));
    CHECK(lifecycle_can_end(LifecycleState::Faulted));
    CHECK(lifecycle_can_end(LifecycleState::Stopped));
    CHECK(!lifecycle_can_end(LifecycleState::New));
    CHECK(!lifecycle_can_end(LifecycleState::Stopping));

    CHECK(lifecycle_is_ready(LifecycleState::ReadyReader));
    CHECK(lifecycle_is_ready(LifecycleState::ReadyTarget));
    CHECK(!lifecycle_is_ready(LifecycleState::Busy));
    CHECK(!lifecycle_is_ready(LifecycleState::New));

    CHECK(lifecycle_is_terminal(LifecycleState::Stopped));
    CHECK(lifecycle_is_terminal(LifecycleState::Faulted));
    CHECK(!lifecycle_is_terminal(LifecycleState::ReadyReader));
    return 0;
}

static int test_begin_transitions() {
    auto r = lifecycle_after_begin(Mode::Reader, LifecycleState::New);
    CHECK(r.ok());
    CHECK(r.value() == LifecycleState::ReadyReader);

    auto t = lifecycle_after_begin(Mode::EmulationNtag213, LifecycleState::Stopped);
    CHECK(t.ok());
    CHECK(t.value() == LifecycleState::ReadyTarget);

    auto u = lifecycle_after_begin(Mode::EmulationUltralight, LifecycleState::New);
    CHECK(u.ok());
    CHECK(u.value() == LifecycleState::ReadyTarget);

    auto bad = lifecycle_after_begin(Mode::Reader, LifecycleState::ReadyReader);
    CHECK(!bad.ok());
    CHECK(bad.error().layer == ErrorLayer::Lifecycle);
    CHECK(bad.error().esp_code == ESP_CODE_ERR_INVALID_STATE);
    CHECK(bad.error().operation == Operation::Begin);
    return 0;
}

static int test_end_transitions() {
    auto from_ready = lifecycle_after_end(LifecycleState::ReadyReader);
    CHECK(from_ready.ok());
    CHECK(from_ready.value() == LifecycleState::Stopped);

    auto idempotent = lifecycle_after_end(LifecycleState::Stopped);
    CHECK(idempotent.ok());
    CHECK(idempotent.value() == LifecycleState::Stopped);

    auto from_fault = lifecycle_after_end(LifecycleState::Faulted);
    CHECK(from_fault.ok());
    CHECK(from_fault.value() == LifecycleState::Stopped);

    auto from_new = lifecycle_after_end(LifecycleState::New);
    CHECK(!from_new.ok());
    CHECK(from_new.error().layer == ErrorLayer::Lifecycle);

    auto from_stopping = lifecycle_after_end(LifecycleState::Stopping);
    CHECK(!from_stopping.ok());
    CHECK(from_stopping.error().layer == ErrorLayer::Lifecycle);
    return 0;
}

static int test_fault() {
    CHECK(lifecycle_after_fault(LifecycleState::ReadyReader) == LifecycleState::Faulted);
    CHECK(lifecycle_after_fault(LifecycleState::Busy) == LifecycleState::Faulted);
    CHECK(lifecycle_after_fault(LifecycleState::Stopped) == LifecycleState::Stopped);
    CHECK(lifecycle_after_fault(LifecycleState::Faulted) == LifecycleState::Faulted);
    return 0;
}

int main() {
    int result = 0;
    result |= test_predicates();
    result |= test_begin_transitions();
    result |= test_end_transitions();
    result |= test_fault();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
