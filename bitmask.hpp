#pragma once

/**
 * @file
 * @brief Utility to enable bitmask operations on an enumeration type.
 */

namespace utils {
    /// @brief A convenience macro to enable bitmask operations on an enumeration `T`.
    ///
    /// The macro must be applied in the same namespace as `T` to allow ADL lookup.
    /// The program is ill-formed if `T` is not an enumeration type.
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