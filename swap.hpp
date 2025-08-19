#pragma once

/**
 * @file
 *
 * This file is included by all headers that contain a class with a ```swap``` method.
 * Due to ADL, manually including this header in user code does not provide any extra utilites.
 */

namespace utils {
    /// Make classes that provide a ```swap``` method swappable.
    /// Visible to any namespace by ADL.
    template <typename T, typename U>
    requires requires(T& t, U& u) { t.swap(u); }
    constexpr decltype(auto) swap(T& t, U& u)
    noexcept(noexcept(t.swap(u))) {
        return t.swap(u);
    }
}