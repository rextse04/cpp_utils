#pragma once
#include <memory>

namespace utils::pmr {
    /// @brief A convenience base class providing typedefs for custom allocators.
    /// @remark Due to @code std::allocator_traits@endcode, this is not needed most of the time.
    /// However, this can be useful when your allocator depends on a common "control type" and you would like
    /// to synchronize the typedefs across all your allocators.
    /// @tparam T: @code value_type@endcode
    /// @tparam Traits: A type which has member types @code void_pointer@endcode and @code size_type@endcode.
    /// All other member types are derived from @code Traits@endcode.
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