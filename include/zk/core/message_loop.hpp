#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "zk/error/error.hpp"

#include <expected>
#include <functional>

namespace zk::core::detail {

/// Non-blocking message pump using PeekMessage (PM_REMOVE).
///
/// Designed for game-engine / continuous-rendering scenarios where the loop
/// must run every frame regardless of whether Win32 events are pending.
///
/// ## Loop behaviour (Req 2.1–2.5)
///   1. Drain all pending Win32 messages via PeekMessage(PM_REMOVE).
///      For each message: TranslateMessage + DispatchMessage.
///   2. If WM_QUIT is encountered, stop and return its exit code.
///   3. After draining, invoke `frame_cb` once per iteration.
///   4. Repeat.
///
/// Satisfies Requirements: 2.1, 2.2, 2.3, 2.4, 2.5
class NonBlockingPump {
public:
    /// Runs the non-blocking message loop.
    ///
    /// @param frame_cb  Called once per iteration after all pending messages
    ///                  have been processed.  May be empty (no-op).
    /// @returns         The WM_QUIT exit code on normal termination, or an
    ///                  error if the pump encounters an unrecoverable failure.
    [[nodiscard]] std::expected<int, error::Error> run(
        std::function<void()> frame_cb) noexcept;
};

/// Blocking message pump using GetMessage.
///
/// Designed for desktop applications where the thread should sleep when idle
/// to conserve CPU resources.
///
/// ## Loop behaviour (Req 3.1–3.5)
///   1. Call GetMessage — blocks until a message is available.
///   2. If GetMessage returns 0 (WM_QUIT), stop and return the exit code.
///   3. If GetMessage returns -1 (error), invoke the error handler and stop.
///   4. Otherwise: TranslateMessage + DispatchMessage, then repeat.
///   5. No periodic frame callback is invoked (Req 3.5).
///
/// Satisfies Requirements: 3.1, 3.2, 3.3, 3.4, 3.5
class BlockingPump {
public:
    /// Runs the blocking message loop.
    ///
    /// @param error_handler  Optional callback invoked if GetMessage returns -1.
    ///                       Receives the structured error describing the failure.
    /// @returns              The WM_QUIT exit code on normal termination, or an
    ///                       error if GetMessage fails.
    [[nodiscard]] std::expected<int, error::Error> run(
        std::function<void(const error::Error&)> error_handler = {}) noexcept;
};

} // namespace zk::core::detail
