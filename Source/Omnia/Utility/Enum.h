#pragma once

#include "Omnia/Omnia.pch"

/**
* @brief Converts enumeration value to BitMask value
*/
template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
constexpr inline auto BitMask(T x) { return (1 << x); }

/**
* @brief Converts enumeration to base type
*/
template<typename E>
constexpr auto GetEnumType(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}
