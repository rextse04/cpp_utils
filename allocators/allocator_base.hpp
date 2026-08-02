#pragma once
#include <memory>

/**
 * @file
 * @brief Common utilities for custom allocators.
 */

namespace utils::pmr {
    /// @brief A convenience base class providing typedefs for custom allocators.
    /// @remark Due to `std::allocator_traits`, this is not needed most of the time.
    /// However, this can be useful when your allocator depends on a common "control type" and you would like
    /// to synchronize the typedefs across all your allocators.
    /// @tparam T: `value_type`
    /// @tparam Traits: A type which has member types `void_pointer` and `size_type`.
    /// All other member types are derived from `Traits`.
    template <typename T, typename Traits>
    struct allocator_base {
        using value_type = T;
        using void_pointer = Traits::void_pointer;
        using const_void_pointer = std::pointer_traits<void_pointer>::template rebind<const void>;
        using pointer = std::pointer_traits<void_pointer>::template rebind<T>;
        using const_pointer = std::pointer_traits<void_pointer>::template rebind<const T>;
        using difference_type = std::pointer_traits<void_pointer>::difference_type;
        using size_type = Traits::size_type;
    };
}