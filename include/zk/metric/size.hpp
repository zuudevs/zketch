#pragma once

#include "zk/metric/basic_pair.hpp"
#include "zk/util/arithmetic.hpp"
#include "zk/util/clamp.hpp"
#include <compare>

namespace zk::metric {

template <meta::Arithmetic Ty>
class Size : public basic_pair<Size<Ty>> {
public:
	using type   = Ty;
	using base_t = basic_pair<Size<Ty>>;
	static constexpr type MIN = type{0};

	Ty w{};
	Ty h{};

	// -----------------------------------------------------------------------
	// Special members
	// -----------------------------------------------------------------------
	constexpr Size() noexcept = default;
	constexpr Size(const Size&) noexcept = default;
	constexpr Size(Size&&) noexcept = default;
	constexpr Size& operator=(const Size&) noexcept = default;
	constexpr Size& operator=(Size&&) noexcept = default;
	constexpr std::strong_ordering operator<=>(const Size&) const noexcept = default;
	constexpr ~Size() noexcept = default;

	// -----------------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------------
	constexpr Size(meta::Arithmetic auto val) noexcept
	 : w(util::clamp(val, MIN, base_t::MAX))
	 , h(util::clamp(val, MIN, base_t::MAX)) {}

	constexpr Size(meta::Arithmetic auto wval, meta::Arithmetic auto hval) noexcept
	 : w(util::clamp(wval, MIN, base_t::MAX))
	 , h(util::clamp(hval, MIN, base_t::MAX)) {}

	constexpr Size& operator=(meta::Arithmetic auto val) noexcept {
		w = h = util::clamp(val, MIN, base_t::MAX);
		return *this;
	}

	// -----------------------------------------------------------------------
	// Component accessors — required by basic_pair::apply
	// -----------------------------------------------------------------------
	constexpr Ty first()  const noexcept { return w; }
	constexpr Ty second() const noexcept { return h; }
};

// ---------------------------------------------------------------------------
// CTAD — Size{800, 600} deduces Size<int>; Size{1.0f} deduces Size<float>
// ---------------------------------------------------------------------------
template <meta::Arithmetic Ty>
Size(Ty, Ty) -> Size<Ty>;

template <meta::Arithmetic Ty>
Size(Ty) -> Size<Ty>;

} // namespace zk::metric
