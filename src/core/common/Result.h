#pragma once

#include <string>
#include <utility>

namespace core {

enum class DiagErr {
    Ok = 0,
    Timeout,
    Busy,
    InvalidChannel,
    DeviceNotOpen,
    Unsupported,
    BufferOverflow,
    ProtocolViolation,
    NegativeResponse,
    SecurityDenied,
    DriverError,
    InternalError,
};

template <typename T>
struct Result {
    T value{};
    DiagErr err{DiagErr::Ok};
    std::string message{};

    [[nodiscard]] bool ok() const { return err == DiagErr::Ok; }

    static Result<T> success(T val) {
        Result<T> r;
        r.value = std::move(val);
        return r;
    }

    static Result<T> failure(DiagErr e, std::string msg) {
        Result<T> r;
        r.err = e;
        r.message = std::move(msg);
        return r;
    }
};

template <>
struct Result<void> {
    DiagErr err{DiagErr::Ok};
    std::string message{};

    [[nodiscard]] bool ok() const { return err == DiagErr::Ok; }

    static Result<void> success() { return {}; }

    static Result<void> failure(DiagErr e, std::string msg) {
        Result<void> r;
        r.err = e;
        r.message = std::move(msg);
        return r;
    }
};

}  // namespace core
