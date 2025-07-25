#pragma once
#include <utility>

namespace utils {
    template <typename T>
    requires (std::is_enum_v<T>)
    struct is_bitmask : std::false_type {};
    template <typename T>
    concept bitmask = is_bitmask<T>::value;
    template <bitmask T>
    constexpr auto operator+(T a) noexcept {
        return std::to_underlying(a);
    }
    template <bitmask T>
    constexpr T operator|(T a, T b) noexcept {
        return static_cast<T>(+a | +b);
    }
    template <bitmask T>
    constexpr T& operator|=(T& a, T b) noexcept {
        return a = a | b;
    }
    template <bitmask T>
    constexpr T operator&(T a, T b) noexcept {
        return static_cast<T>(+a & +b);
    }
    template <bitmask T>
    constexpr T& operator&=(T& a, T b) noexcept {
        return a = a & b;
    }
    template <bitmask T>
    constexpr T operator^(T a, T b) noexcept {
        return static_cast<T>(+a ^ +b);
    }
    template <bitmask T>
    constexpr T& operator^=(T& a, T b) noexcept {
        return a = a ^ b;
    }
    template <bitmask T>
    constexpr T operator~(T a) noexcept {
        return static_cast<T>(~+a);
    }
    template <bitmask T>
    constexpr bool operator*(T a, T b) noexcept {
        return +a & +b;
    }
}