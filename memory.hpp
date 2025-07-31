#pragma once
#include <new>
#include <algorithm>

namespace utils {
    template <typename... Ts>
    constexpr auto max_align = static_cast<std::align_val_t>(std::max({alignof(Ts)...}));

    /// Calculates the minimum offset that is larger than ```offset``` such that it is aligned to ```T```,
    /// provided that ```offset``` is aligned to ```T```.
    template <typename T>
    constexpr std::size_t align_to(std::size_t offset) noexcept {
        return (offset + alignof(T) - 1) / alignof(T) * alignof(T);
    }

    /// ```constexpr```-friendly pointer punning.
    /// @remark: This does not guard against UB.
    template <typename To>
    constexpr To rcast(auto* from) noexcept
    requires requires {static_cast<To>(static_cast<void*>(from));} {
        return static_cast<To>(static_cast<void*>(from));
    }
}