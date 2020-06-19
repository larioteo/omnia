#pragma once

#include "Omnia/Omnia.pch"
#include <type_traits>

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
constexpr inline auto BitMask(T x) { return (1 << x); }

template<typename E>
constexpr auto GetEnumType(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}
