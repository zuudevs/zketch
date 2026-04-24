/**
 * Property test for Pos<T> round-trip clamp behaviour.
 *
 * Property 1: Pos round-trip clamp
 *   For all values `v` of type `int`:
 *     Pos<int>{v}.x == clamp(v, Pos<int>::MIN, Pos<int>::base_t::MAX)
 *     Pos<int>{v}.y == clamp(v, Pos<int>::MIN, Pos<int>::base_t::MAX)
 *
 * Validates: Requirements 5.5
 */

#include <catch2/catch_test_macros.hpp>
#include "zk/metric/pos.hpp"
#include "zk/util/clamp.hpp"

#include <climits>

using zk::metric::Pos;
using zk::util::clamp;

// Convenience aliases for the bounds of Pos<int>
static constexpr int POS_MIN = Pos<int>::MIN;
static constexpr int POS_MAX = static_cast<int>(Pos<int>::base_t::MAX);

// ---------------------------------------------------------------------------
// Helper — verify the round-trip property for a single value
// ---------------------------------------------------------------------------
static void check_roundtrip(int v) {
    const int expected = clamp(v, POS_MIN, POS_MAX);
    const Pos<int> p{v};

    INFO("v = " << v << "  expected = " << expected
         << "  p.x = " << p.x << "  p.y = " << p.y);

    REQUIRE(p.x == expected);
    REQUIRE(p.y == expected);
}

// ---------------------------------------------------------------------------
// Property 1: Pos round-trip clamp
// Validates: Requirements 5.5
// ---------------------------------------------------------------------------
TEST_CASE("Property 1: Pos<int> round-trip clamp - x and y equal clamp(v, MIN, MAX)",
          "[metric][pos][property][req-5.5]")
{
    SECTION("values within valid range") {
        check_roundtrip(0);
        check_roundtrip(1);
        check_roundtrip(-1);
        check_roundtrip(100);
        check_roundtrip(-100);
        check_roundtrip(1000);
        check_roundtrip(-1000);
    }

    SECTION("exact boundary values") {
        check_roundtrip(POS_MIN);
        check_roundtrip(POS_MAX);
        check_roundtrip(POS_MIN + 1);
        check_roundtrip(POS_MAX - 1);
    }

    SECTION("values beyond upper boundary") {
        check_roundtrip(POS_MAX + 1);
        check_roundtrip(POS_MAX + 100);
        check_roundtrip(100'000);
        check_roundtrip(1'000'000);
        check_roundtrip(INT_MAX);
    }

    SECTION("values beyond lower boundary") {
        check_roundtrip(POS_MIN - 1);
        check_roundtrip(POS_MIN - 100);
        check_roundtrip(-100'000);
        check_roundtrip(-1'000'000);
        // INT_MIN avoided: negating it is UB for signed int; use INT_MIN+1 instead
        check_roundtrip(INT_MIN + 1);
    }
}
