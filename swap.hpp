#pragma once

namespace utils {
    template <typename T, typename U>
    requires requires(T& t, U& u) { t.swap(u); }
    constexpr decltype(auto) swap(T& t, U& u)
    noexcept(noexcept(t.swap(u))) {
        return t.swap(u);
    }
}