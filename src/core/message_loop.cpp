#include "zk/core/message_loop.hpp"

#include "zk/core/main_thread_queue.hpp"
#include "zk/log/logger.hpp"

namespace zk::core::detail {

// ---------------------------------------------------------------------------
// NonBlockingPump
// ---------------------------------------------------------------------------

std::expected<int, error::Error> NonBlockingPump::run(
    std::function<void()> frame_cb) noexcept
{
    ZK_LOG(zk::log::Level::Debug, zk::log::Domain::Core,
           "NonBlockingPump: starting");

    MSG msg{};

    for (;;) {
        // Drain all pending Win32 messages before executing one frame (Req 2.5).
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            // WM_QUIT terminates the loop immediately (Req 2.3).
            if (msg.message == WM_QUIT) {
                ZK_LOG(zk::log::Level::Debug, zk::log::Domain::Core,
                       "NonBlockingPump: WM_QUIT received, stopping");
                return static_cast<int>(msg.wParam);
            }

            // Translate virtual-key messages into character messages (Req 2.4).
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }

        // Flush the post-to-main-thread queue after draining Win32 messages
        // and before the frame callback (Req 11.4, 18.7).
        MainThreadQueue::instance().flush();

        // Execute the frame callback once per iteration (Req 2.2).
        // Guard against a null callback — callers may omit it.
        if (frame_cb) {
            frame_cb();
        }
    }
}

// ---------------------------------------------------------------------------
// BlockingPump
// ---------------------------------------------------------------------------

std::expected<int, error::Error> BlockingPump::run(
    std::function<void(const error::Error&)> error_handler) noexcept
{
    ZK_LOG(zk::log::Level::Debug, zk::log::Domain::Core,
           "BlockingPump: starting");

    MSG msg{};

    for (;;) {
        // GetMessage blocks until a message is available (Req 3.1).
        const BOOL result = ::GetMessageW(&msg, nullptr, 0, 0);

        if (result == 0) {
            // WM_QUIT received — return the exit code (Req 3.2).
            ZK_LOG(zk::log::Level::Debug, zk::log::Domain::Core,
                   "BlockingPump: WM_QUIT received, stopping");
            return static_cast<int>(msg.wParam);
        }

        if (result == -1) {
            // GetMessage error — build a structured error and invoke the
            // error handler if one is registered (Req 3.3).
            auto err = error::make_win32_error(
                error::ErrorCode::Unknown,
                "GetMessage returned -1");

            ZK_LOG(zk::log::Level::Error, zk::log::Domain::Core,
                   "BlockingPump: GetMessage failed");

            if (error_handler) {
                error_handler(err);
            }

            return std::unexpected(std::move(err));
        }

        // Normal message — translate and dispatch (Req 3.4).
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);

        // Flush the post-to-main-thread queue after each dispatched message
        // so cross-thread callables are executed promptly (Req 11.4, 18.7).
        MainThreadQueue::instance().flush();

        // No periodic frame callback in blocking mode (Req 3.5).
    }
}

} // namespace zk::core::detail
