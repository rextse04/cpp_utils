#pragma once
#include <cstddef>
#include "type.hpp"

namespace utils {
    template <typename T>
    struct as {
        using tag = struct as_tag;
        using type = T;
        template <typename U>
        static constexpr T cast(U&& obj) { return static_cast<T>(std::forward<U>(obj)); }
    };

    template <std::ptrdiff_t Exp>
    constexpr auto pow(auto base, tagged<as_tag> auto as_type)
    -> typename decltype(as_type)::type {
        if constexpr (Exp == 0) return 1;
        else if constexpr (Exp > 0) return base * pow<Exp-1>(base, as_type);
        else return as_type.cast(1) / pow<-Exp>(base, as_type);
    }
    template <std::ptrdiff_t Exp>
    constexpr auto pow(auto base) {
        return pow<Exp>(base, as<decltype(base)>{});
    }
}