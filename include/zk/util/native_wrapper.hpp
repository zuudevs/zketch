#pragma once

#include <utility>
#include <windows.h>

namespace zk::util {

template <typename Handle, auto Deleter>
class NativeWrapper {
public:
    NativeWrapper() noexcept : handle_(nullptr) {}
    explicit NativeWrapper(Handle h) noexcept : handle_(h) {}

    ~NativeWrapper() noexcept {
        if (handle_) Deleter(handle_);
    }

    NativeWrapper(const NativeWrapper&) = delete;
    NativeWrapper& operator=(const NativeWrapper&) = delete;

    NativeWrapper(NativeWrapper&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}

    NativeWrapper& operator=(NativeWrapper&& other) noexcept {
        if (this != &other) {
            if (handle_) Deleter(handle_);
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    Handle get() const noexcept { return handle_; }
    bool valid() const noexcept { return handle_ != nullptr; }
    explicit operator bool() const noexcept { return valid(); }

private:
    Handle handle_;
};

using HwndWrapper = NativeWrapper<HWND, &DestroyWindow>;

} // namespace zk::util
