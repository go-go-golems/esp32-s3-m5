// Stable status/result vocabulary for the s3paper reader stack.
//
// Pure header: stdint only. Shared by firmware, host tests, and future C/JS
// bindings, so codes must never be renumbered.
#pragma once

#include <stdint.h>

namespace s3paper {

enum class StatusCode : uint8_t {
    Ok = 0,
    InvalidArgument,
    CapacityExceeded,
    Busy,
    Timeout,
    CorruptData,
    OutOfMemory,
    Unimplemented,
};

const char *StatusCodeName(StatusCode code);

struct Status {
    StatusCode code = StatusCode::Ok;
    constexpr bool ok() const { return code == StatusCode::Ok; }
};

constexpr Status OkStatus() { return Status{StatusCode::Ok}; }
constexpr Status ErrStatus(StatusCode code) { return Status{code}; }

// Result<T>: value is meaningful only when ok(). No exceptions, no silent
// boolean failures.
template <typename T>
struct Result {
    StatusCode code;
    T value;

    constexpr bool ok() const { return code == StatusCode::Ok; }
    static constexpr Result Ok(T v) { return Result{StatusCode::Ok, v}; }
    static constexpr Result Err(StatusCode c) { return Result{c, T{}}; }
};

}  // namespace s3paper
