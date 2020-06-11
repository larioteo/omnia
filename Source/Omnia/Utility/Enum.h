#pragma once

#include "Omnia/Omnia.pch"

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
constexpr inline auto BitMask(T x) { return (1 << x); }
