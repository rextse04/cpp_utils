#pragma once
#include <new>
#include <memory>
#include <cstddef>
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

    /// @brief Align a pointer to the specified alignment.
    /// @param alignment: Desired alignment. Must be a power of 2.
    /// @param size: The size of the storage to be aligned.
    /// @param ptr: Pointer to a contiguous storage.
    /// @param end: Pointer to the end of the storage such that [ @code ptr@endcode, @code end@endcode ) is a valid range.
    /// @returns If it is possible to fit a storage of @code size@endcode in [ @code ptr@endcode, @code end@endcode )
    /// aligned to @code alignment@endcode, a pointer to the first byte of such storage; @code nullptr@endcode otherwise.
    inline void* align(std::size_t alignment, std::size_t size, void* ptr, void* end) {
        std::size_t space = static_cast<const std::byte*>(end) - static_cast<const std::byte*>(ptr);
        return std::align(alignment, size, ptr, space);
    }

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