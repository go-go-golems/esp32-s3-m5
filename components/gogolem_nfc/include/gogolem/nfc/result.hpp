// SPDX-License-Identifier: MIT
//
// gogolem::nfc::Result — a compact expected-style success/failure value that
// does not use C++ exceptions. ESP-IDF commonly builds without exception
// support, and NFC failure is an expected runtime result, not an exceptional
// condition.
//
// A `Result<T>` owns either a value of type `T` or an `Error`. It is move-only:
// copying would duplicate ownership of the value and surprise callers. Use
// `Result<T>::success(v)` / `Result<T>::failure(err)` to construct, `ok()` or
// the explicit `operator bool` to test, `value()` / `take_value()` to access
// the value, and `error()` to read the failure.

#pragma once

#include <cassert>
#include <new>
#include <utility>

#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc {

template <typename T>
class Result {
public:
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    Result(Result&& other) noexcept : is_ok_(other.is_ok_) {
        if (is_ok_) {
            new (&storage_) T(std::move(*other.value_ptr()));
        } else {
            err_ = std::move(other.err_);
        }
        other.destroy();
        other.is_ok_ = false;
    }
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            destroy();
            is_ok_ = other.is_ok_;
            if (is_ok_) {
                new (&storage_) T(std::move(*other.value_ptr()));
            } else {
                err_ = std::move(other.err_);
            }
            other.destroy();
            other.is_ok_ = false;
        }
        return *this;
    }
    ~Result() { destroy(); }

    static Result success(T value) {
        Result r;
        r.is_ok_ = true;
        new (&r.storage_) T(std::move(value));
        return r;
    }
    static Result failure(Error error) {
        Result r;
        r.is_ok_ = false;
        r.err_ = std::move(error);
        return r;
    }

    bool ok() const { return is_ok_; }
    explicit operator bool() const { return is_ok_; }
    bool has_error() const { return !is_ok_; }

    const T& value() const {
        assert(is_ok_);
        return *value_ptr();
    }
    T& value() {
        assert(is_ok_);
        return *value_ptr();
    }
    T take_value() {
        assert(is_ok_);
        T out = std::move(*value_ptr());
        destroy();
        is_ok_ = false;
        return out;
    }
    const Error& error() const {
        assert(!is_ok_);
        return err_;
    }

private:
    Result() = default;
    void destroy() {
        if (is_ok_) value_ptr()->~T();
    }
    T* value_ptr() { return reinterpret_cast<T*>(&storage_); }
    const T* value_ptr() const { return reinterpret_cast<const T*>(&storage_); }

    bool is_ok_{};
    alignas(T) unsigned char storage_[sizeof(T)];
    Error err_{};
};

template <>
class Result<void> {
public:
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    Result(Result&& other) noexcept : is_ok_(other.is_ok_), err_(std::move(other.err_)) {
        other.is_ok_ = false;
    }
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            is_ok_ = other.is_ok_;
            err_ = std::move(other.err_);
            other.is_ok_ = false;
        }
        return *this;
    }
    ~Result() = default;

    static Result success() {
        Result r;
        r.is_ok_ = true;
        return r;
    }
    static Result failure(Error error) {
        Result r;
        r.is_ok_ = false;
        r.err_ = std::move(error);
        return r;
    }

    bool ok() const { return is_ok_; }
    explicit operator bool() const { return is_ok_; }
    bool has_error() const { return !is_ok_; }
    const Error& error() const { return err_; }

private:
    Result() = default;
    bool is_ok_{};
    Error err_{};
};

}  // namespace gogolem::nfc
