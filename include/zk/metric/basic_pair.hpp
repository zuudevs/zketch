#pragma once

#include "zk/util/arithmetic.hpp"
#include "zk/util/arithmetic_op.hpp"
#include "zk/util/clamp.hpp"
#include <compare>
#include <cstdint>
#include <type_traits>

namespace zk::metric {

template <meta::Arithmetic>
class Pos;

template <meta::Arithmetic>
class Size;

namespace guard {

template <typename>
struct is_pair_unit : std::false_type {};

template <meta::Arithmetic T>
struct is_pair_unit<Pos<T>> : std::true_type {};

template <meta::Arithmetic T>
struct is_pair_unit<Size<T>> : std::true_type {};

} // namespace guard

// ---------------------------------------------------------------------------
// basic_pair<Derived> — CRTP base for Pos<T> and Size<T>
//
// Provides:
//   - Shared MAX / UMIN / SMIN constants (single source of truth)
//   - All arithmetic operators (+, -, *, /, +=, -=, *=, /=) for both
//     same-type and scalar operands, with clamping applied via the
//     derived class's own MIN/MAX (accessed through CRTP).
//   - Spaceship operator (defaulted on the base)
//
// Contract for Derived:
//   - Must expose `type` (the underlying arithmetic type)
//   - Must expose `static constexpr type MIN` and `static constexpr type MAX`
//   - Must expose two public data members that are the pair components
//     (x/y for Pos, w/h for Size) — operators access them via derived()
//   - Must provide a two-argument constructor `Derived(type, type)`
// ---------------------------------------------------------------------------
template <typename Derived>
requires (guard::is_pair_unit<Derived>::value)
class basic_pair {
public:
	using type     = Derived;

	static constexpr uint16_t MAX  = ~uint16_t{0};   // 65535 — shared upper bound

	// -----------------------------------------------------------------------
	// Special members — all defaulted; derived classes inherit them.
	// -----------------------------------------------------------------------
	constexpr basic_pair() noexcept = default;
	constexpr basic_pair(const basic_pair&) noexcept = default;
	constexpr basic_pair(basic_pair&&) noexcept = default;
	constexpr basic_pair& operator=(const basic_pair&) noexcept = default;
	constexpr basic_pair& operator=(basic_pair&&) noexcept = default;
	constexpr std::strong_ordering operator<=>(const basic_pair&) const noexcept = default;
	constexpr ~basic_pair() noexcept = default;

	// -----------------------------------------------------------------------
	// Arithmetic operators — same-type operand
	// Lambdas are used instead of passing util::add/sub/mul/div directly
	// because those are function templates (overload sets), and the compiler
	// cannot deduce the template parameter Fn from an unresolved overload set.
	// -----------------------------------------------------------------------
	constexpr Derived operator+(const Derived& rhs) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::add(a, b); }, rhs);
	}
	constexpr Derived operator-(const Derived& rhs) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::sub(a, b); }, rhs);
	}
	constexpr Derived operator*(const Derived& rhs) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::mul(a, b); }, rhs);
	}
	constexpr Derived operator/(const Derived& rhs) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::div(a, b); }, rhs);
	}

	constexpr Derived& operator+=(const Derived& rhs) noexcept { return self() = self() + rhs; }
	constexpr Derived& operator-=(const Derived& rhs) noexcept { return self() = self() - rhs; }
	constexpr Derived& operator*=(const Derived& rhs) noexcept { return self() = self() * rhs; }
	constexpr Derived& operator/=(const Derived& rhs) noexcept { return self() = self() / rhs; }

	// -----------------------------------------------------------------------
	// Arithmetic operators — scalar operand
	// -----------------------------------------------------------------------
	constexpr Derived operator+(meta::Arithmetic auto val) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::add(a, b); }, val);
	}
	constexpr Derived operator-(meta::Arithmetic auto val) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::sub(a, b); }, val);
	}
	constexpr Derived operator*(meta::Arithmetic auto val) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::mul(a, b); }, val);
	}
	constexpr Derived operator/(meta::Arithmetic auto val) const noexcept {
		return apply([](auto a, auto b) noexcept { return util::div(a, b); }, val);
	}

	constexpr Derived& operator+=(meta::Arithmetic auto val) noexcept { return self() = self() + val; }
	constexpr Derived& operator-=(meta::Arithmetic auto val) noexcept { return self() = self() - val; }
	constexpr Derived& operator*=(meta::Arithmetic auto val) noexcept { return self() = self() * val; }
	constexpr Derived& operator/=(meta::Arithmetic auto val) noexcept { return self() = self() / val; }

private:
	constexpr       Derived& self()       noexcept { return static_cast<Derived&>(*this); }
	constexpr const Derived& self() const noexcept { return static_cast<const Derived&>(*this); }

	// Applies fn(a, b) component-wise and clamps to Derived::MIN / Derived::MAX.
	template <typename Fn>
	constexpr Derived apply(Fn&& fn, const Derived& rhs) const noexcept {
		const auto& d  = self();
		const auto  lo = Derived::MIN;
		const auto  hi = Derived::MAX;
		return Derived{
			util::clamp(fn(d.first(),  rhs.first()),  lo, hi),
			util::clamp(fn(d.second(), rhs.second()), lo, hi)
		};
	}

	template <typename Fn, meta::Arithmetic Scalar>
	constexpr Derived apply(Fn&& fn, Scalar val) const noexcept {
		const auto& d  = self();
		const auto  lo = Derived::MIN;
		const auto  hi = Derived::MAX;
		return Derived{
			util::clamp(fn(d.first(),  val), lo, hi),
			util::clamp(fn(d.second(), val), lo, hi)
		};
	}
};

} // namespace zk::metric
