#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "zk/core/message_loop.hpp"
#include "zk/error/error.hpp"

#include <expected>
#include <functional>
#include <string>
#include <string_view>

namespace zk::core {

/// Selects the Win32 message pump strategy for the application.
/// - NonBlocking: uses PeekMessage (game/continuous-render loop)
/// - Blocking:    uses GetMessage  (desktop/event-driven app)
enum class PumpMode : uint8_t {
    NonBlocking,
    Blocking,
};

/// Top-level application object.  The single entry point for the zketch
/// framework (Req 1.1).
///
/// ## Lifecycle
///   1. Call `Application::create(class_name, mode)` — registers the Win32
///      window class and captures the main-thread ID.
///   2. Optionally call `set_frame_callback` (NonBlocking mode only) and
///      `set_error_handler`.
///   3. Call `run()` — blocks until WM_QUIT is received.
///
/// ## Thread safety
///   All operations must be called from the Main_Thread (Req 1.6, 11.1).
///   In debug builds a thread-id assertion fires if this contract is violated
///   (Req 11.2).
///
/// ## Error handling
///   `create()` and `run()` return `std::expected<T, Error>` so callers can
///   handle failures without exceptions (Req 14.1).
///
/// Satisfies Requirements: 1.1–1.6, 11.1, 11.2
class Application {
public:
    // -----------------------------------------------------------------------
    // Factory — the only way to construct a valid Application (Req 1.1)
    // -----------------------------------------------------------------------

    /// Registers a Win32 window class with the given name and initialises the
    /// framework.
    ///
    /// @param class_name  Unique name for the Win32 WNDCLASSEX (Req 1.2).
    /// @param mode        Pump strategy: NonBlocking or Blocking (Req 1.3, 1.4).
    /// @returns           A valid Application, or an error if registration fails
    ///                    (Req 1.5).
    [[nodiscard]] static std::expected<Application, error::Error> create(
        std::string_view class_name,
        PumpMode         mode = PumpMode::Blocking) noexcept;

    // -----------------------------------------------------------------------
    // Special members — move-only (WNDCLASSEX registration is unique)
    // -----------------------------------------------------------------------
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    Application(Application&&) noexcept;
    Application& operator=(Application&&) noexcept;

    /// Unregisters the Win32 window class on destruction.
    ~Application() noexcept;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /// Registers a callback invoked every frame in NonBlocking mode (Req 2.2).
    /// Has no effect in Blocking mode (Req 3.5).
    void set_frame_callback(std::function<void()> cb) noexcept;

    /// Registers a global error handler invoked when the message loop
    /// encounters an unrecoverable error (Req 1.5, 3.3).
    void set_error_handler(std::function<void(const error::Error&)> handler) noexcept;

    // -----------------------------------------------------------------------
    // Run
    // -----------------------------------------------------------------------

    /// Starts the message loop and blocks until WM_QUIT is received.
    ///
    /// Delegates to NonBlockingPump or BlockingPump depending on the mode
    /// selected at construction (Req 1.3, 1.4).
    ///
    /// @returns  The WM_QUIT exit code on normal termination, or an error if
    ///           the pump fails.
    [[nodiscard]] std::expected<int, error::Error> run() noexcept;

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /// Returns the pump mode this application was created with.
    [[nodiscard]] PumpMode mode() const noexcept { return mode_; }

    /// Returns the registered Win32 window class name (UTF-8).
    [[nodiscard]] const std::string& class_name() const noexcept { return class_name_; }

private:
    // Private constructor — use create().
    Application() noexcept = default;

    /// Asserts that the calling thread is the Main_Thread (Req 1.6, 11.1, 11.2).
    /// In release builds this is a no-op.
    void assert_main_thread() const noexcept;

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------
    PumpMode    mode_       { PumpMode::Blocking };
    std::string class_name_ {};
    std::wstring wclass_name_{};   // UTF-16 copy for Win32 UnregisterClassW

    std::function<void()>                    frame_cb_      {};
    std::function<void(const error::Error&)> error_handler_ {};

    DWORD main_thread_id_ { 0 };

    /// True once the WNDCLASSEX has been successfully registered so the
    /// destructor knows whether to call UnregisterClassW.
    bool class_registered_ { false };
};

} // namespace zk::core

// ---------------------------------------------------------------------------
// to_string helper (Req 13.6)
// ---------------------------------------------------------------------------
namespace zk::util {

[[nodiscard]] constexpr std::string_view to_string(core::PumpMode mode) noexcept {
    using core::PumpMode;
    switch (mode) {
        case PumpMode::NonBlocking: return "NonBlocking";
        case PumpMode::Blocking:    return "Blocking";
        default:                    return "Unknown";
    }
}

} // namespace zk::util
