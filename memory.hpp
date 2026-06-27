#pragma once
#include <new>
#include <algorithm>
#include <concepts>

namespace utils {
    /// @brief Calculates the maximum alignment requirement among the types in @code Ts@endcode.
    /// @{
    template <typename... Ts>
    struct max_align {
        static constexpr auto value = static_cast<std::align_val_t>(std::max({alignof(Ts)...}));
    };
    template <typename... Ts>
    constexpr std::align_val_t max_align_v = max_align<Ts...>::value;
    /// @}

    /// @brief A trivial class type that is aligned to @code Alignment@endcode.
    /// @remark It is ill-formed if @code Alignment@endcode is invalid or unsupported.
    template <std::size_t Alignment>
    struct aligned_t {
        alignas(Alignment) std::byte data[Alignment];
    };

    /// @brief @code constexpr@endcode-friendly pointer punning
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