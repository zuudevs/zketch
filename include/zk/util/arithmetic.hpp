#pragma once

#include <type_traits>

namespace zk::meta {

template <typename Ty>
concept Arithmetic = std::is_arithmetic_v<Ty>;

} // namespace zk::meta