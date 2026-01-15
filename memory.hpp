#pragma once
#include <new>
#include <algorithm>
#include <concepts>

namespace utils {
    template <typename... Ts>
    constexpr auto max_align = static_cast<std::align_val_t>(std::max({alignof(Ts)...}));

    /// @brief calculates the minimum offset that is larger than ```offset``` such that it is aligned to ```T```,
    /// provided that ```offset``` is aligned to ```T```
    template <typename T>
    constexpr std::size_t align_to(std::size_t offset) noexcept {
        return (offset + alignof(T) - 1) / alignof(T) * alignof(T);
    }

    /// @brief ```constexpr```-friendly pointer punning
    /// @remark This does not guard against UB.
    template <typename To>
    constexpr To rcast(auto* from) noexcept
    requires requires {static_cast<To>(static_cast<void*>(from));} {
        return static_cast<To>(static_cast<void*>(from));
    }

    /// @brief C++ named requirements: <i>Allocator</i>
    template <typename T>
    concept simple_allocator = requires(T alloc, std::size_t n) {
        { *alloc.allocate(n) } -> std::same_as<typename T::value_type&>;
        { alloc.deallocate(alloc.allocate(n), n) };
    } && std::copy_constructible<T> && std::equality_comparable<T>;
}