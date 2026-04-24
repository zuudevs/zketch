#pragma once

#include "zk/metric/basic_pair.hpp"
#include "zk/util/arithmetic.hpp"
#include "zk/util/clamp.hpp"
#include <compare>

namespace zk::metric {

template <meta::Arithmetic T>
class Pos : public basic_pair<Pos<T>> {
public:
	using type     = T;
	using base_t   = basic_pair<Pos<T>>;
	static constexpr type MIN = -static_cast<type>(base_t::MAX);

	type x{};
	type y{};

	// -----------------------------------------------------------------------
	// Special members
	// -----------------------------------------------------------------------
	constexpr Pos() noexcept = default;
	constexpr Pos(const Pos&) noexcept = default;
	constexpr Pos(Pos&&) noexcept = default;
	constexpr Pos& operator=(const Pos&) noexcept = default;
	constexpr Pos& operator=(Pos&&) noexcept = default;
	constexpr std::strong_ordering operator<=>(const Pos&) const noexcept = default;
	constexpr ~Pos() noexcept = default;

	// -----------------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------------
	constexpr Pos(meta::Arithmetic auto val) noexcept
	 : x(util::clamp(val, MIN, base_t::MAX))
	 , y(util::clamp(val, MIN, base_t::MAX)) {}

	constexpr Pos(meta::Arithmetic auto xval, meta::Arithmetic auto yval) noexcept
	 : x(util::clamp(xval, MIN, base_t::MAX))
	 , y(util::clamp(yval, MIN, base_t::MAX)) {}

	constexpr Pos& operator=(meta::Arithmetic auto val) noexcept {
		x = y = util::clamp(val, MIN, base_t::MAX);
		return *this;
	}

	// -----------------------------------------------------------------------
	// Component accessors — required by basic_pair::apply
	// -----------------------------------------------------------------------
	constexpr type  first()  const noexcept { return x; }
	constexpr type  second() const noexcept { return y; }
};

// ---------------------------------------------------------------------------
// CTAD — Pos{1, 2} deduces Pos<int>; Pos{1.0f, 2.0f} deduces Pos<float>
// ---------------------------------------------------------------------------
template <meta::Arithmetic T>
Pos(T, T) -> Pos<T>;

template <meta::Arithmetic T>
Pos(T) -> Pos<T>;

} // namespace zk::metric
