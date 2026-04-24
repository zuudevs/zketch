/**
 * Property test for arithmetic clamp safety on Pos<T> and Size<T>.
 *
 * Property 3: Arithmetic clamp safety
 *   For all `a`, `b` of type `Pos<int>`, the result of `a + b` always lies
 *   within [Pos<int>::MIN, Pos<int>::base_t::MAX].
 *
 *   The same invariant is verified for -, *, / and for Size<uint16_t>.
 *
 * Validates: Requirements 5.7
 */

#include <catch2/catch_test_macros.hpp>
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

#include <climits>
#include <cstdint>

using zk::metric::Pos;
using zk::metric::Size;

// ---------------------------------------------------------------------------
// Bounds for Pos<int>
// ---------------------------------------------------------------------------
static constexpr int POS_MIN = Pos<int>::MIN;
static constexpr int POS_MAX = static_cast<int>(Pos<int>::base_t::MAX);

// ---------------------------------------------------------------------------
// Bounds for Size<uint16_t>
// Note: SIZE_MAX is a system macro on Windows (defined in <limits.h> as
// 0xffffffffffffffff for size_t). Use a distinct name to avoid the collision.
// ---------------------------------------------------------------------------
static constexpr uint16_t SZ_MIN = Size<uint16_t>::MIN;
static constexpr uint16_t SZ_MAX = static_cast<uint16_t>(Size<uint16_t>::base_t::MAX);

// ---------------------------------------------------------------------------
// Helper — assert that every component of `p` is within [lo, hi]
// ---------------------------------------------------------------------------
static void check_pos_in_bounds(const Pos<int>& p) {
    INFO("p.x = " << p.x << "  p.y = " << p.y
         << "  [" << POS_MIN << ", " << POS_MAX << "]");
    REQUIRE(p.x >= POS_MIN);
    REQUIRE(p.x <= POS_MAX);
    REQUIRE(p.y >= POS_MIN);
    REQUIRE(p.y <= POS_MAX);
}

static void check_size_in_bounds(const Size<uint16_t>& s) {
    INFO("s.w = " << s.w << "  s.h = " << s.h
         << "  [" << SZ_MIN << ", " << SZ_MAX << "]");
    REQUIRE(s.w >= SZ_MIN);
    REQUIRE(s.w <= SZ_MAX);
    REQUIRE(s.h >= SZ_MIN);
    REQUIRE(s.h <= SZ_MAX);
}

// ===========================================================================
// Property 3: Arithmetic clamp safety — Pos<int>
// Validates: Requirements 5.7
// ===========================================================================
TEST_CASE("Property 3: Pos<int> arithmetic never overflows bounds",
          "[metric][pos][property][req-5.7]")
{
    // Representative pairs covering: zero, mid-range, boundary, and extreme values
    const int values[] = {
        POS_MIN, POS_MIN + 1, -10000, -1, 0, 1, 10000, POS_MAX - 1, POS_MAX
    };

    SECTION("addition stays within [MIN, MAX]") {
        for (int a : values) {
            for (int b : values) {
                const Pos<int> pa{a, a};
                const Pos<int> pb{b, b};
                check_pos_in_bounds(pa + pb);
            }
        }
    }

    SECTION("subtraction stays within [MIN, MAX]") {
        for (int a : values) {
            for (int b : values) {
                const Pos<int> pa{a, a};
                const Pos<int> pb{b, b};
                check_pos_in_bounds(pa - pb);
            }
        }
    }

    SECTION("multiplication stays within [MIN, MAX]") {
        for (int a : values) {
            for (int b : values) {
                const Pos<int> pa{a, a};
                const Pos<int> pb{b, b};
                check_pos_in_bounds(pa * pb);
            }
        }
    }

    SECTION("division stays within [MIN, MAX]") {
        // Skip divisor == 0 to avoid UB in the underlying arithmetic_op helpers
        for (int a : values) {
            for (int b : values) {
                if (b == 0) continue;
                const Pos<int> pa{a, a};
                const Pos<int> pb{b, b};
                check_pos_in_bounds(pa / pb);
            }
        }
    }

    SECTION("compound assignment += stays within [MIN, MAX]") {
        for (int a : values) {
            for (int b : values) {
                Pos<int> pa{a, a};
                pa += Pos<int>{b, b};
                check_pos_in_bounds(pa);
            }
        }
    }

    SECTION("compound assignment -= stays within [MIN, MAX]") {
        for (int a : values) {
            for (int b : values) {
                Pos<int> pa{a, a};
                pa -= Pos<int>{b, b};
                check_pos_in_bounds(pa);
            }
        }
    }

    SECTION("scalar addition stays within [MIN, MAX]") {
        const int scalars[] = { INT_MIN + 1, POS_MIN, -1, 0, 1, POS_MAX, INT_MAX };
        for (int a : values) {
            for (int s : scalars) {
                check_pos_in_bounds(Pos<int>{a, a} + s);
            }
        }
    }

    SECTION("scalar subtraction stays within [MIN, MAX]") {
        const int scalars[] = { INT_MIN + 1, POS_MIN, -1, 0, 1, POS_MAX, INT_MAX };
        for (int a : values) {
            for (int s : scalars) {
                check_pos_in_bounds(Pos<int>{a, a} - s);
            }
        }
    }
}

// ===========================================================================
// Property 3 (extended): Arithmetic clamp safety — Size<uint16_t>
// Validates: Requirements 5.7
// ===========================================================================
TEST_CASE("Property 3: Size<uint16_t> arithmetic never overflows bounds",
          "[metric][size][property][req-5.7]")
{
    const uint16_t values[] = {
        SZ_MIN, uint16_t(1), uint16_t(100), uint16_t(1000),
        uint16_t(SZ_MAX / 2), uint16_t(SZ_MAX - 1), SZ_MAX
    };

    SECTION("addition stays within [0, UINT16_MAX]") {
        for (uint16_t a : values) {
            for (uint16_t b : values) {
                check_size_in_bounds(Size<uint16_t>{a, a} + Size<uint16_t>{b, b});
            }
        }
    }

    SECTION("subtraction stays within [0, UINT16_MAX]") {
        for (uint16_t a : values) {
            for (uint16_t b : values) {
                check_size_in_bounds(Size<uint16_t>{a, a} - Size<uint16_t>{b, b});
            }
        }
    }

    SECTION("multiplication stays within [0, UINT16_MAX]") {
        for (uint16_t a : values) {
            for (uint16_t b : values) {
                check_size_in_bounds(Size<uint16_t>{a, a} * Size<uint16_t>{b, b});
            }
        }
    }

    SECTION("division stays within [0, UINT16_MAX]") {
        for (uint16_t a : values) {
            for (uint16_t b : values) {
                if (b == 0) continue;
                check_size_in_bounds(Size<uint16_t>{a, a} / Size<uint16_t>{b, b});
            }
        }
    }

    SECTION("compound assignment += stays within [0, UINT16_MAX]") {
        for (uint16_t a : values) {
            for (uint16_t b : values) {
                Size<uint16_t> s{a, a};
                s += Size<uint16_t>{b, b};
                check_size_in_bounds(s);
            }
        }
    }
}
