/**
 * Property test for Size<T> round-trip clamp behaviour.
 *
 * Property 2: Size round-trip clamp
 *   For all values `v` of type `uint32_t`:
 *     Size<uint16_t>{v}.w == clamp(v, uint32_t{0}, uint32_t{UINT16_MAX})
 *     Size<uint16_t>{v}.h == clamp(v, uint32_t{0}, uint32_t{UINT16_MAX})
 *
 * Validates: Requirements 5.6
 */

#include <catch2/catch_test_macros.hpp>
#include "zk/metric/size.hpp"
#include "zk/util/clamp.hpp"

#include <climits>
#include <cstdint>

using zk::metric::Size;
using zk::util::clamp;

static constexpr uint32_t SIZE_LO = uint32_t{0};
static constexpr uint32_t SIZE_HI = uint32_t{UINT16_MAX};

// ---------------------------------------------------------------------------
// Helper — verify the round-trip property for a single value
// ---------------------------------------------------------------------------
static void check_roundtrip(uint32_t v) {
    const uint16_t expected = static_cast<uint16_t>(clamp(v, SIZE_LO, SIZE_HI));
    const Size<uint16_t> s{v};

    INFO("v = " << v << "  expected = " << expected
         << "  s.w = " << s.w << "  s.h = " << s.h);

    REQUIRE(s.w == expected);
    REQUIRE(s.h == expected);
}

// ---------------------------------------------------------------------------
// Property 2: Size round-trip clamp
// Validates: Requirements 5.6
// ---------------------------------------------------------------------------
TEST_CASE("Property 2: Size<uint16_t> round-trip clamp - w and h equal clamp(v, 0, UINT16_MAX)",
          "[metric][size][property][req-5.6]")
{
    SECTION("values within valid range") {
        check_roundtrip(0);
        check_roundtrip(1);
        check_roundtrip(100);
        check_roundtrip(1000);
        check_roundtrip(32767);
    }

    SECTION("exact boundary values") {
        check_roundtrip(SIZE_LO);
        check_roundtrip(SIZE_HI);
        check_roundtrip(SIZE_LO + 1);
        check_roundtrip(SIZE_HI - 1);
    }

    SECTION("values beyond upper boundary") {
        check_roundtrip(SIZE_HI + 1);
        check_roundtrip(SIZE_HI + 100);
        check_roundtrip(100'000u);
        check_roundtrip(1'000'000u);
        check_roundtrip(UINT32_MAX);
    }

    SECTION("mid-range values") {
        check_roundtrip(256);
        check_roundtrip(512);
        check_roundtrip(1024);
        check_roundtrip(4096);
        check_roundtrip(8192);
        check_roundtrip(16384);
    }
}
