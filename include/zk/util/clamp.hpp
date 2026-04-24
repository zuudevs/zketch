#pragma once

#include "zk/util/arithmetic.hpp"

namespace zk::util {

template <meta::Arithmetic Ty>
constexpr Ty clamp(
	Ty val, 
	meta::Arithmetic auto low, 
	meta::Arithmetic auto high
) noexcept {
	Ty _low = static_cast<Ty>(low);
	Ty _high = static_cast<Ty>(high);
	return ((val < _low) ? _low : ((val > _high) ? _high : val));
}

} // namespace zk::util