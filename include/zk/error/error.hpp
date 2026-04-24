#pragma once

#include <string>
#include <string_view>
#include <windows.h>

namespace zk::error {

/// Error codes for all fallible operations in the framework.
/// Fatal codes: InitFailed, WindowClassRegistrationFailed, WindowCreationFailed,
///              RenderTargetCreationFailed, WrongThread.
enum class ErrorCode : uint32_t {
    // Initialization
    InitFailed,
    WindowClassRegistrationFailed,
    // Window
    WindowCreationFailed,
    WindowInvalidState,
    // Rendering
    RenderTargetCreationFailed,
    RenderTargetLost,
    // Event
    EventHandlerException,
    // Parsing
    ParseError,
    // Thread safety
    WrongThread,
    // Generic
    Unknown,
};

/// Structured error type used with std::expected<T, Error> throughout the framework.
struct Error {
    ErrorCode code;
    std::string message;
    uint32_t native_error = 0;  // GetLastError() or HRESULT

    /// Returns true if this error is considered fatal (unrecoverable).
    bool is_fatal() const noexcept {
        switch (code) {
            case ErrorCode::InitFailed:
            case ErrorCode::WindowClassRegistrationFailed:
            case ErrorCode::WindowCreationFailed:
            case ErrorCode::RenderTargetCreationFailed:
            case ErrorCode::WrongThread:
                return true;
            default:
                return false;
        }
    }

    /// Returns a static string_view describing the error code.
    std::string_view to_string() const noexcept {
        switch (code) {
            case ErrorCode::InitFailed:                    return "InitFailed";
            case ErrorCode::WindowClassRegistrationFailed: return "WindowClassRegistrationFailed";
            case ErrorCode::WindowCreationFailed:          return "WindowCreationFailed";
            case ErrorCode::WindowInvalidState:            return "WindowInvalidState";
            case ErrorCode::RenderTargetCreationFailed:    return "RenderTargetCreationFailed";
            case ErrorCode::RenderTargetLost:              return "RenderTargetLost";
            case ErrorCode::EventHandlerException:         return "EventHandlerException";
            case ErrorCode::ParseError:                    return "ParseError";
            case ErrorCode::WrongThread:                   return "WrongThread";
            case ErrorCode::Unknown:                       return "Unknown";
            default:                                       return "Unknown";
        }
    }
};

/// Constructs an Error with the given code, message, and optional native error code.
inline Error make_error(ErrorCode code, std::string message, uint32_t native_error = 0) {
    return Error{ code, std::move(message), native_error };
}

/// Constructs an Error, filling native_error from GetLastError().
inline Error make_win32_error(ErrorCode code, std::string message) {
    return Error{ code, std::move(message), static_cast<uint32_t>(::GetLastError()) };
}

} // namespace zk::error
