#pragma once

#include "zk/util/arithmetic.hpp"
#include <type_traits>

namespace zk::util {

template <meta::Arithmetic Ty>
constexpr Ty add(Ty a, Ty b) noexcept { return a + b; }

template <meta::Arithmetic Ty>
constexpr Ty sub(Ty a, Ty b) noexcept { return a - b; }

template <meta::Arithmetic Ty>
constexpr Ty mul(Ty a, Ty b) noexcept { return a * b; }

template <meta::Arithmetic Ty>
constexpr Ty div(Ty a, Ty b) noexcept { return a / b; }

template <meta::Arithmetic Ty, meta::Arithmetic Tz>
constexpr auto add(Ty a, Tz b) noexcept { 
	using rty = std::common_type_t<Ty, Tz>; 
	return static_cast<rty>(a) + static_cast<rty>(b); 
}

template <meta::Arithmetic Ty, meta::Arithmetic Tz>
constexpr auto sub(Ty a, Tz b) noexcept { 
	using rty = std::common_type_t<Ty, Tz>; 
	return static_cast<rty>(a) - static_cast<rty>(b); 
}

template <meta::Arithmetic Ty, meta::Arithmetic Tz>
constexpr auto mul(Ty a, Tz b) noexcept { 
	using rty = std::common_type_t<Ty, Tz>; 
	return static_cast<rty>(a) * static_cast<rty>(b); 
}

template <meta::Arithmetic Ty, meta::Arithmetic Tz>
constexpr auto div(Ty a, Tz b) noexcept { 
	using rty = std::common_type_t<Ty, Tz>; 
	return static_cast<rty>(a) / static_cast<rty>(b); 
}

} // namespace zk::util