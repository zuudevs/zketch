#pragma once

#include "zk/util/arithmetic.hpp"
#include <compare>

namespace zk::metric {

template <meta::Arithmetic T>
struct Rect {
	using type = T;

	T x{};
	T y{};
	T w{};
	T h{};

	constexpr Rect() noexcept = default;
	constexpr Rect(const Rect&) noexcept = default;
	constexpr Rect(Rect&&) noexcept = default;
	constexpr Rect& operator=(const Rect&) noexcept = default;
	constexpr Rect& operator=(Rect&&) noexcept = default;
	constexpr std::strong_ordering operator<=>(const Rect&) const noexcept = default;
	constexpr ~Rect() noexcept = default;

	constexpr Rect(T xval, T yval, T wval, T hval) noexcept
	 : x(xval), y(yval), w(wval), h(hval) {}

	/// Returns true if the point (px, py) falls within this rect.
	constexpr bool contains(T px, T py) const noexcept {
		return px >= x && px < x + w
		    && py >= y && py < y + h;
	}

	/// Returns the right edge (x + w).
	constexpr T right()  const noexcept { return x + w; }

	/// Returns the bottom edge (y + h).
	constexpr T bottom() const noexcept { return y + h; }
};

} // namespace zk::metric
