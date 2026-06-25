#pragma once
#include <type_traits>
#include <concepts>
#include <functional>
#include <utility>
#include <compare>

namespace utils {
    namespace detail {
        template <typename R, typename... Ts>
        struct is_strict_weak_order;
        template <typename R>
        struct is_strict_weak_order<R> : std::bool_constant<true> {};
        template <typename R, typename T>
        struct is_strict_weak_order<R, T> : std::bool_constant<std::strict_weak_order<R, T, T>> {};
        template <typename R, typename T, typename... Ts>
        struct is_strict_weak_order<R, T, Ts...> : std::bool_constant<
            (std::strict_weak_order<R, T, Ts> && ...) &&
            is_strict_weak_order<R, Ts...>::value> {};
        template <typename R, typename... Ts>
        concept strict_weak_order = is_strict_weak_order<R, Ts...>::value;
    }

    /// @brief Enumeration representing the type of an interval endpoint (open or closed).
    enum class interval_endpoint_type : unsigned char { open, closed };

    /// @brief Represents an endpoint of an interval, consisting of a value and its type (open or closed).
    ///
    /// Given a value @f$v@f$ and a comparator @f$C:D\times D\rightarrow\textit{boolean-testable}@f$ satisfying <i>Compare</i>,
    /// <table>
    /// <tr><th>Endpoint Type</th><th>Set Representation</th></tr>
    /// <tr><td>Left Open</td><td>@f$\{x\in D\mid C(v,x)\}@f$</td></tr>
    /// <tr><td>Left Closed</td><td>@f$\{x\in D\mid\neg C(x,v)\}@f$</td></tr>
    /// <tr><td>Right Open</td><td>@f$\{x\in D\mid C(x,v)\}@f$</td></tr>
    /// <tr><td>Right Closed</td><td>@f$\{x\in D\mid\neg C(v,x)\}@f$</td></tr>
    /// </table>
    /// @remark Note that we do not assume the domain of @f$C@f$ to be @f$D@f$.
    template <typename T>
    struct interval_endpoint {
        using value_type = T;

        T value;
        interval_endpoint_type type;

        /// @brief Conversion operator.
        ///
        /// Allows conversion to an endpoint with a compatible value type.
        /// @note A conversion operator is chosen over a constructor to keep this class an aggregate.
        template <std::constructible_from<const T&> U>
        constexpr operator interval_endpoint<U>() const noexcept(std::is_nothrow_constructible_v<U, const T&>){
            return {.value{value}, .type{type}};
        }
        /// @brief Equality operator.
        ///
        /// Two endpoints are equivalent if and only if their values and types are equivalent.
        constexpr bool operator==(const interval_endpoint& other) const = default;
        /** @brief Comparison functions.
         *
         * Given endpoints @f$E1@f$ and @f$E2@f$,
         * @f$E1<E2@f$ if and only if the set represented by @f$E1@f$ is a proper subset of the set represented by @f$E2@f$.
         * This defines a strict weak ordering on endpoints.
         *
         * Let @f$v@f$ be @code this->value@endcode and @f$ov@f$ be @code other.value@endcode.
         * The program is ill-formed if there is not a restriction of @code std::less<>@endcode such that
         * @f$v@f$ and @f$ov@f$ are in the domain.
         * It is UB if such a restriction does not satisfy <i>Compare</i>.
         * @{
         */
        /// @brief Three-way comparison operator.
        ///
        /// All endpoints are assumed to be right endpoints. The comparator is taken to be @code std::less<>@endcode.
        template <typename U>
        constexpr std::weak_ordering operator<=>(const interval_endpoint<U>& other) const
        requires (detail::strict_weak_order<std::less<>, const T&, const U&>) {
            return compare(other, true);
        }
        /// @f$E1@f$ is taken to be @code *this@endcode, @f$E2@f$ is taken to be @code other@endcode.
        template <typename U, detail::strict_weak_order<const T&, const U&> Compare = std::less<>>
        constexpr std::weak_ordering compare(const interval_endpoint<U>& other, bool right, const Compare& comp = {}) const {
            if (comp(value, other.value)) return right ? std::weak_ordering::less : std::weak_ordering::greater;
            if (comp(other.value, value)) return right ? std::weak_ordering::greater : std::weak_ordering::less;
            return std::to_underlying(type) <=> std::to_underlying(other.type);
        }
        /**@}*/
    };
    /// @brief Convenience operator overload to construct an interval endpoint.
    template <typename T>
    constexpr interval_endpoint<std::decay_t<T>> operator|(T&& value, interval_endpoint_type type) {
        return {std::forward<T>(value), type};
    }

    /// @brief An interval defined by a left endpoint and a right endpoint.
    ///
    /// The set represented by the interval is defined to be the intersection of that of the two endpoints.
    ///
    /// @warning In any member function which takes a @code point@endcode (resp. an @code interval@endcode @code other@endcode),
    /// the program is ill-formed if there is not a restriction of @code comp@endcode such that @code point@endcode
    /// (resp. @code other.left@endcode, @code other.right@endcode) and the two endpoints are in the domain.
    /// It is UB if such a restriction does not satisfy <i>Compare</i>.
    template <typename L, typename R>
    struct interval {
        using left_value_type = L;
        using right_value_type = R;

        interval_endpoint<L> left;
        interval_endpoint<R> right;

        /// @brief Equality operator.
        ///
        /// Two intervals are equivalent if and only if both left and right endpoints are equivalent.
        constexpr bool operator==(const interval& other) const = default;
        using enum interval_endpoint_type;
        /// @brief Determines whether the interval represents an empty set.
        template <detail::strict_weak_order<const L&, const R&> Compare = std::less<>>
        constexpr bool empty(const Compare& comp = {}) const {
            if (comp(left.value, right.value)) return false;
            if (comp(right.value, left.value)) return true;
            return !(left.type == closed && right.type == closed);
        }
        /// @brief Determines whether @code point@endcode is contained in the set represented by the interval.
        template <typename P, detail::strict_weak_order<const L&, const R&, const P&> Compare = std::less<>>
        constexpr bool contains(const P& point, const Compare& comp = {}) const {
            bool out;
            switch (left.type) {
                case open: out = comp(left.value, point); break;
                case closed: out = !comp(point, left.value); break;
                default: std::unreachable();
            }
            switch (right.type) {
                case open: out = out && comp(point, right.value); break;
                case closed: out = out && !comp(right.value, point); break;
                default: std::unreachable();
            }
            return out;
        }
        /** @brief Determines whether the interval is a superset/strict superset/subset/strict subset of another interval.
         * @{
         */
        template <typename L2, typename R2,
            detail::strict_weak_order<const L&, const R&, const L2&, const R2&> Compare = std::less<>>
        constexpr bool superset_of(const interval<L2, R2>& other, const Compare& comp = {}) const {
            return left.compare(other.left, false, comp) >= 0 && right.compare(other.right, true, comp) >= 0;
        }
        template <typename L2, typename R2,
            detail::strict_weak_order<const L&, const R&, const L2&, const R2&> Compare = std::less<>>
        constexpr bool strict_superset_of(const interval<L2, R2>& other, const Compare& comp = {}) const {
            const auto left_cmp = left.compare(other.left, false, comp),
                right_cmp = right.compare(other.right, true, comp);
            return (left_cmp > 0 && right_cmp >= 0) || (left_cmp >= 0 && right_cmp > 0);
        }
        template <typename L2, typename R2,
            detail::strict_weak_order<const L&, const R&, const L2&, const R2&> Compare = std::less<>>
        constexpr bool subset_of(const interval<L2, R2>& other, const Compare& comp = {}) const {
            return other.superset_of(*this, comp);
        }
        template <typename L2, typename R2,
            detail::strict_weak_order<const L&, const R&, const L2&, const R2&> Compare = std::less<>>
        constexpr bool strict_subset_of(const interval<L2, R2>& other, const Compare& comp = {}) const {
            return other.strict_superset_of(*this, comp);
        }
        /**@}*/
        /// @brief Constructs the intersection between the interval and @code other@endcode.
        /// @{
        template <typename CL, typename CR,
            typename L2, typename R2, detail::strict_weak_order<const L&, const R&, const L2&, const R2&> Compare = std::less<>>
        requires (
            std::is_constructible_v<CL, const L&> && std::is_constructible_v<CL, const L2&> &&
            std::is_constructible_v<CR, const R&> && std::is_constructible_v<CR, const R2&>)
        constexpr interval<CL, CR> intersection_with(const interval<L2, R2>& other, const Compare& comp = {}) const {
            const auto left_cmp = left.compare(other.left, false, comp);
            const auto right_cmp = right.compare(other.right, true, comp);
            if (left_cmp <= 0) {
                if (right_cmp <= 0) return {left, right};
                else return {left, other.right};
            } else {
                if (right_cmp <= 0) return {other.left, right};
                else return {other.left, other.right};
            }
        }
        template <detail::strict_weak_order<const L&, const R&> Compare = std::less<>>
        constexpr interval intersection_with(const interval& other, const Compare& comp = {}) const
        requires requires{ this->intersection_with<L, R, L, R, Compare>(other, comp); } {
            return intersection_with<L, R, L, R, Compare>(other, comp);
        }
        /// @}
        /** @brief Comparison functions.
         *
         * Given intervals @f$I1@f$ and @f$I2@f$,
         * @f$I1<I2@f$ if and only if @f$I1@f$ is a proper subset of @f$I2@f$.
         * This defines a strict partial ordering on intervals.
         */
        /// @brief Three-way comparison operator.
        ///
        /// The comparator is taken to be @code std::less<>@endcode.
        template <typename L2, typename R2>
        constexpr std::partial_ordering operator<=>(const interval<L2, R2>& other) const
        requires (detail::strict_weak_order<std::less<>, const L&, const R&, const L2&, const R2&>) {
            return compare(other);
        }
        /// @f$I1@f$ is taken to be @code *this@endcode, @f$I2@f$ is taken to be @code other@endcode.
        template <typename L2, typename R2,
            detail::strict_weak_order<const L&, const R&, const L2&, const R2&> Compare = std::less<>>
        constexpr std::partial_ordering compare(const interval<L2, R2>& other, const Compare& comp = {}) const {
            const auto left_cmp = left.compare(other.left, false, comp),
            right_cmp = right.compare(other.right, true, comp);
            if (left_cmp == 0 && right_cmp == 0) return std::partial_ordering::equivalent;
            if (left_cmp >= 0 && right_cmp >= 0) return std::partial_ordering::greater;
            if (left_cmp <= 0 && right_cmp <= 0) return std::partial_ordering::less;
            return std::partial_ordering::unordered;
        }
        /**@}*/
    };
    /// @brief Convenience operator overload to construct an interval.
    template <typename LE, typename RE>
    constexpr auto operator,(LE&& left, RE&& right)
    requires requires { interval(std::forward<LE>(left), std::forward<RE>(right)); } {
        return interval(std::forward<LE>(left), std::forward<RE>(right));
    }
}