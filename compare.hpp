#pragma once
#include <compare>
#include "type.hpp"

namespace utils {
    /// @brief C++ standard exposition-only functor: <i>synth-three-way</i>.
    inline constexpr auto synth_three_way = []<typename T, typename U>(const T& t, const U& u)
    requires requires {
        { t < u } -> boolean_testable;
        { u < t } -> boolean_testable;
    } {
        if constexpr (std::three_way_comparable_with<T, U>) {
            return t <=> u;
        } else {
            if (t < u)
                return std::weak_ordering::less;
            if (u < t)
                return std::weak_ordering::greater;
            return std::weak_ordering::equivalent;
        }
    };
    /// @brief Trait to deduce the result type of the <i>synth-three-way</i> functor for types @code T@endcode and @code U@endcode.
    template <typename T, typename U = T>
    struct synth_three_way_result {
        using type = decltype(synth_three_way(std::declval<T&>(), std::declval<U&>()));
    };
    /// @brief Result type of the <i>synth-three-way</i> functor for types @code T@endcode and @code U@endcode.
    template <typename T, typename U = T>
    using synth_three_way_result_t = synth_three_way_result<T, U>::type;
}