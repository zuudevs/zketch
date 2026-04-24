#include <catch2/catch_test_macros.hpp>
#include "zk/error/error.hpp"

using namespace zk::error;

// ---------------------------------------------------------------------------
// is_fatal()
// ---------------------------------------------------------------------------

TEST_CASE("is_fatal returns true for fatal error codes", "[error][is_fatal]") {
    REQUIRE(make_error(ErrorCode::InitFailed,                    "").is_fatal());
    REQUIRE(make_error(ErrorCode::WindowClassRegistrationFailed, "").is_fatal());
    REQUIRE(make_error(ErrorCode::WindowCreationFailed,          "").is_fatal());
    REQUIRE(make_error(ErrorCode::RenderTargetCreationFailed,    "").is_fatal());
    REQUIRE(make_error(ErrorCode::WrongThread,                   "").is_fatal());
}

TEST_CASE("is_fatal returns false for non-fatal error codes", "[error][is_fatal]") {
    REQUIRE_FALSE(make_error(ErrorCode::WindowInvalidState,    "").is_fatal());
    REQUIRE_FALSE(make_error(ErrorCode::RenderTargetLost,      "").is_fatal());
    REQUIRE_FALSE(make_error(ErrorCode::EventHandlerException, "").is_fatal());
    REQUIRE_FALSE(make_error(ErrorCode::ParseError,            "").is_fatal());
    REQUIRE_FALSE(make_error(ErrorCode::Unknown,               "").is_fatal());
}

// ---------------------------------------------------------------------------
// to_string()
// ---------------------------------------------------------------------------

TEST_CASE("to_string returns non-empty string for every ErrorCode", "[error][to_string]") {
    constexpr ErrorCode all_codes[] = {
        ErrorCode::InitFailed,
        ErrorCode::WindowClassRegistrationFailed,
        ErrorCode::WindowCreationFailed,
        ErrorCode::WindowInvalidState,
        ErrorCode::RenderTargetCreationFailed,
        ErrorCode::RenderTargetLost,
        ErrorCode::EventHandlerException,
        ErrorCode::ParseError,
        ErrorCode::WrongThread,
        ErrorCode::Unknown,
    };

    for (auto code : all_codes) {
        auto sv = make_error(code, "").to_string();
        INFO("ErrorCode " << static_cast<uint32_t>(code) << " returned empty string");
        REQUIRE_FALSE(sv.empty());
    }
}

TEST_CASE("to_string returns distinct strings for each ErrorCode", "[error][to_string]") {
    // Spot-check a few to ensure they are not all returning the same fallback
    auto a = make_error(ErrorCode::InitFailed,           "").to_string();
    auto b = make_error(ErrorCode::WindowCreationFailed, "").to_string();
    auto c = make_error(ErrorCode::ParseError,           "").to_string();
    REQUIRE(a != b);
    REQUIRE(b != c);
    REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// make_win32_error
// ---------------------------------------------------------------------------

TEST_CASE("make_win32_error captures GetLastError into native_error", "[error][make_win32_error]") {
    // Set a known Win32 error code before calling make_win32_error
    constexpr DWORD expected = ERROR_FILE_NOT_FOUND;  // 2
    ::SetLastError(expected);

    auto err = make_win32_error(ErrorCode::InitFailed, "test");

    REQUIRE(err.native_error == static_cast<uint32_t>(expected));
    REQUIRE(err.code == ErrorCode::InitFailed);
    REQUIRE(err.message == "test");
}

TEST_CASE("make_win32_error with zero last error stores zero", "[error][make_win32_error]") {
    ::SetLastError(0);
    auto err = make_win32_error(ErrorCode::Unknown, "no error");
    REQUIRE(err.native_error == 0u);
}

// ---------------------------------------------------------------------------
// make_error
// ---------------------------------------------------------------------------

TEST_CASE("make_error stores code, message, and optional native_error", "[error][make_error]") {
    auto err = make_error(ErrorCode::ParseError, "bad input", 42u);
    REQUIRE(err.code         == ErrorCode::ParseError);
    REQUIRE(err.message      == "bad input");
    REQUIRE(err.native_error == 42u);
}

TEST_CASE("make_error native_error defaults to zero", "[error][make_error]") {
    auto err = make_error(ErrorCode::Unknown, "msg");
    REQUIRE(err.native_error == 0u);
}
