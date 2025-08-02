#pragma once
#include <utility>

namespace utils {
#define UTILS_BITMASK(T)\
    static_assert(std::is_enum_v<T>, "T must be an enumeration type");\
    constexpr auto operator+(T a) noexcept {\
        return std::to_underlying(a);\
    }\
    constexpr T operator|(T a, T b) noexcept {\
        return static_cast<T>(+a | +b);\
    }\
    constexpr T& operator|=(T& a, T b) noexcept {\
        return a = a | b;\
    }\
    constexpr T operator&(T a, T b) noexcept {\
        return static_cast<T>(+a & +b);\
    }\
    constexpr T& operator&=(T& a, T b) noexcept {\
        return a = a & b;\
    }\
    constexpr T operator^(T a, T b) noexcept {\
        return static_cast<T>(+a ^ +b);\
    }\
    constexpr T& operator^=(T& a, T b) noexcept {\
        return a = a ^ b;\
    }\
    constexpr T operator~(T a) noexcept {\
        return static_cast<T>(~+a);\
    }\
    constexpr bool operator*(T a, T b) noexcept {\
        return +a & +b;\
    }
}