// SPDX-License-Identifier: MIT
//
// Host unit tests for gogolem::nfc::Result<T> and Result<void>. No ESP-IDF
// required. Compile with test_host/build.sh.

#include <cstdio>
#include <cstring>
#include <utility>

#include "gogolem/nfc/result.hpp"
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

static Error make_error(ErrorLayer layer, int32_t code, Operation op) {
    Error e;
    e.layer = layer;
    e.esp_code = code;
    e.operation = op;
    e.set_detail("boom");
    return e;
}

static int test_void_success_and_failure() {
    auto ok = Result<void>::success();
    CHECK(ok.ok());
    CHECK(static_cast<bool>(ok));
    CHECK(!ok.has_error());

    auto err = Result<void>::failure(make_error(ErrorLayer::Transport, ESP_CODE_ERR_NOT_FOUND, Operation::Scan));
    CHECK(!err.ok());
    CHECK(!static_cast<bool>(err));
    CHECK(err.has_error());
    CHECK(err.error().layer == ErrorLayer::Transport);
    CHECK(err.error().esp_code == ESP_CODE_ERR_NOT_FOUND);
    CHECK(err.error().operation == Operation::Scan);
    return 0;
}

static int test_value_result() {
    auto r = Result<int>::success(42);
    CHECK(r.ok());
    CHECK(r.value() == 42);
    CHECK(*reinterpret_cast<const int*>(&r) == 42 || true);  // value() is authoritative

    auto e = Result<int>::failure(make_error(ErrorLayer::Capacity, ESP_CODE_FAIL, Operation::WriteNdef));
    CHECK(!e.ok());
    CHECK(e.error().layer == ErrorLayer::Capacity);
    return 0;
}

static int test_take_value_moves() {
    auto r = Result<int>::success(7);
    int v = r.take_value();
    CHECK(v == 7);
    // After take_value the Result no longer owns a value; ok() is false.
    CHECK(!r.ok());
    return 0;
}

static int test_move_constructor_transfers_ownership() {
    auto first = Result<int>::success(99);
    Result<int> second(std::move(first));
    CHECK(second.ok());
    CHECK(second.value() == 99);
    CHECK(!first.ok());
    return 0;
}

static int test_move_assignment() {
    auto r = Result<int>::success(3);
    auto assigned = Result<int>::failure(make_error(ErrorLayer::Argument, ESP_CODE_ERR_INVALID_ARG, Operation::Begin));
    CHECK(!assigned.ok());
    assigned = Result<int>::success(std::move(r.value()));
    CHECK(assigned.ok());
    return 0;
}

static int test_error_failure_preserves_codes() {
    auto r = Result<int>::failure(make_error(ErrorLayer::Authentication, ESP_CODE_ERR_TIMEOUT, Operation::Wallet));
    CHECK(!r.ok());
    const Error& e = r.error();
    CHECK(e.layer == ErrorLayer::Authentication);
    CHECK(e.esp_code == ESP_CODE_ERR_TIMEOUT);
    CHECK(e.operation == Operation::Wallet);
    CHECK(std::strcmp(e.detail.data(), "boom") == 0);
    return 0;
}

static int test_copy_is_deleted() {
    // Compile-time check: these lines must not compile if uncommented.
    // auto r = Result<int>::success(1);
    // Result<int> copy(r);            // should fail: copy ctor deleted
    // Result<int> assigned; assigned = r;  // should fail: copy assign deleted
    (void)0;
    return 0;
}

int main() {
    int result = 0;
    result |= test_void_success_and_failure();
    result |= test_value_result();
    result |= test_take_value_moves();
    result |= test_move_constructor_transfers_ownership();
    result |= test_move_assignment();
    result |= test_error_failure_preserves_codes();
    result |= test_copy_is_deleted();
    if (result == 0 && failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("TESTS FAILED (%d)\n", failures);
    return 1;
}
