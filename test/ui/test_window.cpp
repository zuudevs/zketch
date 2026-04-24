// test/ui/test_window.cpp
//
// Unit tests for Window creation logic.
//
// ## Testing strategy
//
// Window::create() calls Win32 APIs (RegisterClassExW, CreateWindowExW) directly
// with no virtual seam, so full isolation from Win32 is not possible without
// refactoring the production code.  We therefore split the tests into two tiers:
//
//   Tier 1 — Pure logic tests (no Win32 window created)
//     These test the error-type contract: that a WindowCreationFailed error is
//     fatal, carries the right ErrorCode, and has a non-empty message.  They
//     exercise zk::error helpers directly and do not depend on Win32 state.
//
//   Tier 2 — Integration-style tests (real Win32 window created)
//     These call Window::create() for real and verify observable post-conditions
//     such as the default-size substitution (Req 4.3) and the is_valid() flag.
//     They require a message-loop-capable Windows environment (normal CI/dev
//     machine).  The window is destroyed immediately after the assertion via
//     RAII (HwndWrapper calls DestroyWindow in the destructor).
//
// NOTE: A test that forces CreateWindowExW to fail without modifying production
// code would require either (a) a virtual seam in Window or (b) a Win32 API
// hook/detour library.  Neither is available in the current codebase.  The
// error-path contract is therefore validated at the error-type level (Tier 1)
// and documented as a known gap for a future refactor.
// See: TODO in Window::create — add a factory-function injection point.

#include <catch2/catch_test_macros.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "zk/ui/window.hpp"
#include "zk/error/error.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

using namespace zk::ui;
using zk::metric::Pos;
using zk::metric::Size;
using zk::error::Error;
using zk::error::ErrorCode;
using zk::error::make_error;
using zk::error::make_win32_error;

// ===========================================================================
// Tier 1 — Error-type contract (no Win32 window)
// ===========================================================================

// ---------------------------------------------------------------------------
// Req 4.6 — WindowCreationFailed error properties
// ---------------------------------------------------------------------------

TEST_CASE("Window error: WindowCreationFailed is a fatal error code",
          "[ui][window][error][req-4.6]")
{
    // Req 4.6 — IF CreateWindowEx fails, THEN the framework SHALL return an
    // error.  This test verifies that the error code used for that path
    // (WindowCreationFailed) is classified as fatal by the error system.
    const Error err = make_error(
        ErrorCode::WindowCreationFailed,
        "CreateWindowEx failed",
        ERROR_INVALID_PARAMETER
    );

    REQUIRE(err.code == ErrorCode::WindowCreationFailed);
    REQUIRE(err.is_fatal());
}

TEST_CASE("Window error: WindowCreationFailed carries a non-empty message",
          "[ui][window][error][req-4.6]")
{
    const Error err = make_error(
        ErrorCode::WindowCreationFailed,
        "CreateWindowEx failed"
    );

    REQUIRE_FALSE(err.message.empty());
}

TEST_CASE("Window error: WindowCreationFailed to_string returns non-empty view",
          "[ui][window][error][req-4.6]")
{
    const Error err = make_error(ErrorCode::WindowCreationFailed, "test");
    REQUIRE_FALSE(err.to_string().empty());
    REQUIRE(err.to_string() == "WindowCreationFailed");
}

TEST_CASE("Window error: make_win32_error fills native_error from GetLastError",
          "[ui][window][error][req-4.6]")
{
    // Simulate the exact call made inside Window::create on failure.
    // Set a known last-error value, then verify make_win32_error captures it.
    ::SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
    const Error err = make_win32_error(
        ErrorCode::WindowCreationFailed,
        "CreateWindowEx failed"
    );

    REQUIRE(err.code == ErrorCode::WindowCreationFailed);
    REQUIRE(err.native_error == static_cast<uint32_t>(ERROR_CLASS_DOES_NOT_EXIST));
}

TEST_CASE("Window error: WindowClassRegistrationFailed is also fatal",
          "[ui][window][error][req-4.6]")
{
    // The registration step that precedes CreateWindowEx uses this code.
    const Error err = make_error(
        ErrorCode::WindowClassRegistrationFailed,
        "RegisterClassEx failed"
    );

    REQUIRE(err.is_fatal());
}

// ===========================================================================
// Tier 2 — Integration tests (real Win32 window)
// ===========================================================================

// ---------------------------------------------------------------------------
// Req 4.3 — Default size 800×600 when size.w == 0 or size.h == 0
// ---------------------------------------------------------------------------

TEST_CASE("Window::create: default size 800x600 used when size.w == 0",
          "[ui][window][integration][req-4.3]")
{
    // Req 4.3 — WHEN Window is created with Size{0, 0}, the framework SHALL
    // use 800×600 as the window dimensions.
    auto result = Window::create(
        "test_default_size_w0",
        Pos<int>{ CW_USEDEFAULT, CW_USEDEFAULT },
        Size<uint16_t>{ 0u, 0u }
    );

    REQUIRE(result.has_value());

    const Window& win = *result;
    REQUIRE(win.size().w == 800u);
    REQUIRE(win.size().h == 600u);
}

TEST_CASE("Window::create: default size 800x600 used when size.h == 0",
          "[ui][window][integration][req-4.3]")
{
    // Req 4.3 — h == 0 alone also triggers the default.
    auto result = Window::create(
        "test_default_size_h0",
        Pos<int>{ CW_USEDEFAULT, CW_USEDEFAULT },
        Size<uint16_t>{ 400u, 0u }
    );

    REQUIRE(result.has_value());

    const Window& win = *result;
    REQUIRE(win.size().w == 800u);
    REQUIRE(win.size().h == 600u);
}

TEST_CASE("Window::create: default size 800x600 used when size.w == 0 and size.h != 0",
          "[ui][window][integration][req-4.3]")
{
    // Req 4.3 — w == 0 alone also triggers the default.
    auto result = Window::create(
        "test_default_size_w0_h_nonzero",
        Pos<int>{ CW_USEDEFAULT, CW_USEDEFAULT },
        Size<uint16_t>{ 0u, 400u }
    );

    REQUIRE(result.has_value());

    const Window& win = *result;
    REQUIRE(win.size().w == 800u);
    REQUIRE(win.size().h == 600u);
}

TEST_CASE("Window::create: explicit non-zero size is preserved",
          "[ui][window][integration][req-4.3]")
{
    // Verify the default-size path is NOT taken when both dimensions are > 0.
    auto result = Window::create(
        "test_explicit_size",
        Pos<int>{ CW_USEDEFAULT, CW_USEDEFAULT },
        Size<uint16_t>{ 1024u, 768u }
    );

    REQUIRE(result.has_value());

    const Window& win = *result;
    REQUIRE(win.size().w == 1024u);
    REQUIRE(win.size().h == 768u);
}

// ---------------------------------------------------------------------------
// Req 4.6 — Successful create returns a valid Window
// ---------------------------------------------------------------------------

TEST_CASE("Window::create: returns a valid Window on success",
          "[ui][window][integration][req-4.6]")
{
    auto result = Window::create(
        "test_valid_window",
        Pos<int>{ CW_USEDEFAULT, CW_USEDEFAULT },
        Size<uint16_t>{ 800u, 600u }
    );

    REQUIRE(result.has_value());
    REQUIRE(result->is_valid());
    REQUIRE(result->native_handle() != nullptr);
}

TEST_CASE("Window::create: title is stored correctly",
          "[ui][window][integration]")
{
    auto result = Window::create(
        "My Test Window",
        Pos<int>{ CW_USEDEFAULT, CW_USEDEFAULT },
        Size<uint16_t>{ 800u, 600u }
    );

    REQUIRE(result.has_value());
    REQUIRE(result->title() == "My Test Window");
}

// ---------------------------------------------------------------------------
// Req 4.6 — Error path: CreateWindowEx failure
//
// NOTE: Without a virtual seam or Win32 API hook, we cannot force
// CreateWindowEx to fail in a unit test without modifying production code.
// The error-type contract is validated in Tier 1 above.
//
// The test below documents the expected API shape: Window::create returns
// std::unexpected<Error> with code == WindowCreationFailed on failure.
// It is marked as a documentation test and does not attempt to trigger
// the failure path at runtime.
// ---------------------------------------------------------------------------

TEST_CASE("Window::create: return type is std::expected<Window, Error>",
          "[ui][window][req-4.6][api-shape]")
{
    // Verify the return type carries the correct error type by constructing
    // a failure value manually and checking its properties.
    using ResultType = std::expected<Window, Error>;

    ResultType failure = std::unexpected(make_error(
        ErrorCode::WindowCreationFailed,
        "simulated CreateWindowEx failure",
        ERROR_INVALID_PARAMETER
    ));

    REQUIRE_FALSE(failure.has_value());
    REQUIRE(failure.error().code == ErrorCode::WindowCreationFailed);
    REQUIRE(failure.error().is_fatal());
    REQUIRE(failure.error().native_error == static_cast<uint32_t>(ERROR_INVALID_PARAMETER));
}
