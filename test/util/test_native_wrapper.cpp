#include <catch2/catch_test_macros.hpp>
#include "zk/util/native_wrapper.hpp"

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

// A fake "handle" type — just an int pointer so we can track deletions.
using FakeHandle = int*;

static int g_delete_count = 0;

static void fake_deleter(FakeHandle h) {
    if (h) ++g_delete_count;
}

using FakeWrapper = zk::util::NativeWrapper<FakeHandle, &fake_deleter>;

// RAII guard that resets the global counter before each test section.
struct DeleteCountGuard {
    DeleteCountGuard()  { g_delete_count = 0; }
    ~DeleteCountGuard() { g_delete_count = 0; }
};

// ---------------------------------------------------------------------------
// Destructor calls deleter exactly once
// ---------------------------------------------------------------------------

TEST_CASE("Destructor calls deleter exactly once", "[native_wrapper][destructor]") {
    DeleteCountGuard guard;
    int dummy = 0;

    {
        FakeWrapper w{&dummy};
        REQUIRE(g_delete_count == 0);
    } // destructor fires here

    REQUIRE(g_delete_count == 1);
}

TEST_CASE("Destructor does not call deleter for null handle", "[native_wrapper][destructor]") {
    DeleteCountGuard guard;

    {
        FakeWrapper w{nullptr};
    }

    REQUIRE(g_delete_count == 0);
}

TEST_CASE("Default-constructed wrapper does not call deleter on destruction", "[native_wrapper][destructor]") {
    DeleteCountGuard guard;

    {
        FakeWrapper w;
    }

    REQUIRE(g_delete_count == 0);
}

// ---------------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------------

TEST_CASE("Move constructor transfers handle; source becomes invalid", "[native_wrapper][move]") {
    DeleteCountGuard guard;
    int dummy = 0;

    FakeWrapper src{&dummy};
    REQUIRE(src.valid());

    FakeWrapper dst{std::move(src)};

    // Source must be invalid after move
    REQUIRE_FALSE(src.valid());
    REQUIRE(src.get() == nullptr);

    // Destination holds the handle
    REQUIRE(dst.valid());
    REQUIRE(dst.get() == &dummy);

    // Deleter not yet called
    REQUIRE(g_delete_count == 0);
} // dst destroyed here — deleter called once

TEST_CASE("Move constructor: deleter called exactly once after move", "[native_wrapper][move]") {
    DeleteCountGuard guard;
    int dummy = 0;

    {
        FakeWrapper src{&dummy};
        FakeWrapper dst{std::move(src)};
        REQUIRE(g_delete_count == 0);
    }

    REQUIRE(g_delete_count == 1);
}

TEST_CASE("Move assignment transfers handle; source becomes invalid", "[native_wrapper][move]") {
    DeleteCountGuard guard;
    int dummy = 0;

    FakeWrapper src{&dummy};
    FakeWrapper dst;

    dst = std::move(src);

    REQUIRE_FALSE(src.valid());
    REQUIRE(src.get() == nullptr);
    REQUIRE(dst.valid());
    REQUIRE(dst.get() == &dummy);
}

TEST_CASE("Move assignment releases existing handle before taking new one", "[native_wrapper][move]") {
    DeleteCountGuard guard;
    int a = 0, b = 0;

    FakeWrapper dst{&a};
    FakeWrapper src{&b};

    dst = std::move(src);

    // &a should have been deleted when dst was overwritten
    REQUIRE(g_delete_count == 1);
    REQUIRE(dst.get() == &b);
    REQUIRE_FALSE(src.valid());
}

// ---------------------------------------------------------------------------
// valid() and operator bool()
// ---------------------------------------------------------------------------

TEST_CASE("valid() returns true for non-null handle", "[native_wrapper][valid]") {
    int dummy = 0;
    FakeWrapper w{&dummy};
    REQUIRE(w.valid());
    REQUIRE(static_cast<bool>(w));
}

TEST_CASE("valid() returns false for null handle (default-constructed)", "[native_wrapper][valid]") {
    FakeWrapper w;
    REQUIRE_FALSE(w.valid());
    REQUIRE_FALSE(static_cast<bool>(w));
}

TEST_CASE("valid() returns false on moved-from wrapper", "[native_wrapper][valid]") {
    DeleteCountGuard guard;
    int dummy = 0;

    FakeWrapper src{&dummy};
    FakeWrapper dst{std::move(src)};

    REQUIRE_FALSE(src.valid());
}

// ---------------------------------------------------------------------------
// get()
// ---------------------------------------------------------------------------

TEST_CASE("get() returns the stored handle", "[native_wrapper][get]") {
    int dummy = 42;
    FakeWrapper w{&dummy};
    REQUIRE(w.get() == &dummy);
    REQUIRE(*w.get() == 42);
}

TEST_CASE("get() returns nullptr for default-constructed wrapper", "[native_wrapper][get]") {
    FakeWrapper w;
    REQUIRE(w.get() == nullptr);
}
