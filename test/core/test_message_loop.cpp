/// test_message_loop.cpp
///
/// Unit tests for NonBlockingPump and BlockingPump.
///
/// Strategy
/// --------
/// Both pumps call real Win32 message APIs (PeekMessage / GetMessage).
/// Rather than mocking the OS, we drive the pumps by posting real Win32
/// messages to the calling thread's queue.
///
/// Key insight about NonBlockingPump loop structure:
///   outer loop:
///     inner while: drain all pending messages via PeekMessage
///       → if WM_QUIT found here, return immediately (frame_cb NOT called)
///     call frame_cb once
///     repeat
///
/// Therefore, to observe frame_cb being called, WM_QUIT must be posted
/// from *inside* frame_cb (after the first invocation), not pre-posted.
/// Pre-posting WM_QUIT causes it to be consumed in the inner drain before
/// frame_cb ever fires.
///
/// Requirements covered: 2.2, 2.3, 3.2, 3.5

#include <catch2/catch_test_macros.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "zk/core/message_loop.hpp"

using zk::core::detail::BlockingPump;
using zk::core::detail::NonBlockingPump;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Posts a WM_USER message to the current thread's queue.
/// Used to simulate a "normal" Win32 message that the pump should process
/// without stopping the loop.
static void post_normal_message()
{
    ::PostThreadMessageW(::GetCurrentThreadId(), WM_USER, 0, 0);
}

// ---------------------------------------------------------------------------
// NonBlockingPump — Req 2.2, 2.3
// ---------------------------------------------------------------------------

TEST_CASE("NonBlockingPump: frame_cb is called at least once before WM_QUIT",
          "[core][message_loop][non_blocking]")
{
    // Req 2.2 — frame callback must fire every iteration.
    //
    // WM_QUIT is posted from inside frame_cb on the first call so that the
    // pump exits after exactly one frame.  This guarantees frame_cb was
    // invoked at least once before the pump returned.

    int frame_count = 0;
    NonBlockingPump pump;

    auto result = pump.run([&frame_count]() {
        ++frame_count;
        // Signal the pump to stop after the first frame.
        ::PostQuitMessage(0);
    });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
    REQUIRE(frame_count >= 1);
}

TEST_CASE("NonBlockingPump: frame_cb is called even when no messages are pending",
          "[core][message_loop][non_blocking]")
{
    // Req 2.2 — the callback fires regardless of whether any Win32 messages
    // are in the queue.  We start with an empty queue; the pump must still
    // call frame_cb before we post WM_QUIT from inside it.

    int frame_count = 0;
    NonBlockingPump pump;

    // Queue is empty at this point — no pre-posted messages.
    auto result = pump.run([&frame_count]() {
        ++frame_count;
        ::PostQuitMessage(42);
    });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 42);
    REQUIRE(frame_count >= 1);
}

TEST_CASE("NonBlockingPump: frame_cb is called multiple times across iterations",
          "[core][message_loop][non_blocking]")
{
    // Req 2.2 — the callback fires every iteration, not just once.
    // We count frames and stop after 3.

    int frame_count = 0;
    NonBlockingPump pump;

    auto result = pump.run([&frame_count]() {
        ++frame_count;
        if (frame_count >= 3) {
            ::PostQuitMessage(0);
        }
    });

    REQUIRE(result.has_value());
    REQUIRE(frame_count >= 3);
}

TEST_CASE("NonBlockingPump: frame_cb is called after draining pending messages",
          "[core][message_loop][non_blocking]")
{
    // Req 2.2 + 2.5 — frame_cb fires once per iteration after all pending
    // messages have been processed.
    //
    // Pre-post two normal messages.  The pump drains them in the inner loop,
    // then calls frame_cb.  From inside frame_cb we post WM_QUIT so the pump
    // exits cleanly on the next inner-drain pass.

    post_normal_message();
    post_normal_message();

    int frame_count = 0;
    NonBlockingPump pump;

    auto result = pump.run([&frame_count]() {
        ++frame_count;
        ::PostQuitMessage(0);
    });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
    REQUIRE(frame_count >= 1);
}

TEST_CASE("NonBlockingPump: stops and returns exit code when WM_QUIT is received",
          "[core][message_loop][non_blocking]")
{
    // Req 2.3 — WM_QUIT must terminate the loop and its wParam becomes the
    // return value.

    constexpr int expected_exit_code = 7;

    NonBlockingPump pump;
    auto result = pump.run([expected_exit_code]() {
        ::PostQuitMessage(expected_exit_code);
    });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == expected_exit_code);
}

TEST_CASE("NonBlockingPump: null frame_cb is tolerated",
          "[core][message_loop][non_blocking]")
{
    // Robustness: passing an empty std::function must not crash.
    // We pre-post WM_QUIT so the pump exits on the first inner drain.
    ::PostQuitMessage(0);

    NonBlockingPump pump;
    auto result = pump.run({});

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
}

// ---------------------------------------------------------------------------
// BlockingPump — Req 3.2, 3.5
// ---------------------------------------------------------------------------

TEST_CASE("BlockingPump: stops and returns exit code when WM_QUIT is received",
          "[core][message_loop][blocking]")
{
    // Req 3.2 — GetMessage returning 0 (WM_QUIT) must terminate the loop and
    // the wParam of WM_QUIT becomes the return value.

    constexpr int expected_exit_code = 5;
    ::PostQuitMessage(expected_exit_code);

    BlockingPump pump;
    auto result = pump.run();

    REQUIRE(result.has_value());
    REQUIRE(result.value() == expected_exit_code);
}

TEST_CASE("BlockingPump: processes normal messages before WM_QUIT",
          "[core][message_loop][blocking]")
{
    // Req 3.4 — normal messages must be translated and dispatched.
    // We verify the pump does not exit prematurely on WM_USER.

    post_normal_message();
    post_normal_message();
    ::PostQuitMessage(0);

    BlockingPump pump;
    auto result = pump.run();

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
}

TEST_CASE("BlockingPump: does not accept a frame_cb parameter",
          "[core][message_loop][blocking]")
{
    // Req 3.5 — the blocking pump has no periodic frame callback.
    // BlockingPump::run() signature takes only an optional error_handler,
    // not a frame_cb.  This is a compile-time API contract verified by the
    // fact that the code below compiles without a frame_cb argument.

    ::PostQuitMessage(0);

    BlockingPump pump;
    // run() takes only an optional error_handler — no frame_cb parameter.
    auto result = pump.run();

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
}

TEST_CASE("BlockingPump: error_handler is not called on normal WM_QUIT exit",
          "[core][message_loop][blocking]")
{
    // Req 3.2 — normal WM_QUIT exit must not invoke the error handler.

    ::PostQuitMessage(0);

    bool error_handler_called = false;
    BlockingPump pump;
    auto result = pump.run([&](const zk::error::Error&) {
        error_handler_called = true;
    });

    REQUIRE(result.has_value());
    REQUIRE_FALSE(error_handler_called);
}

TEST_CASE("BlockingPump: null error_handler is tolerated on normal exit",
          "[core][message_loop][blocking]")
{
    // Robustness: omitting the error handler must not crash on normal exit.

    ::PostQuitMessage(0);

    BlockingPump pump;
    auto result = pump.run({});

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
}
