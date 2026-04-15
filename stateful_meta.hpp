#pragma once
#include <cstdint>

/**
 * @file
 *
 * This file contains utilities that enable stateful metaprogramming,
 * which allows you to make compile-time <b>modifiable</b> variables.
 * This is done via a technique called "friend injection".
 *
 * In this file, compile-time variables are named by types.
 * That is, such variables are defined, accessed and modified via the types they are associated with.
 * Note that you are allowed to use the same type to name variables of different categories.
 * For example, you can define a @code const_var<T>@endcode and @code <i>counter</i><T>@endcode and they will be separate variables.
 * There are three categories of variables:
 * | Category                  | Written via               | Read via                  | Remarks                                                |
 * |-:-:-----------------------|-:-:-----------------------|-:-:-----------------------|-:-:----------------------------------------------------|
 * | @code const_var@endcode   | @code define@endcode      | @code get@endcode         | A @code const_var@endcode can only be written to once. |
 * | <pre><i>counter</i></pre> | @code get_counter@endcode | @code get_counter@endcode | There is no explicit struct for this category.         |
 * | @code var@endcode         | @code set@endcode         | @code get@endcode         |                                                        |
 *
 * In the implementation, some structs may have @code Id@endcode as one of their template parameters.
 * This is necessary to prevent the compiler from caching previous results.
 * Do not fill in that parameter unless you know what you are doing.
 */

namespace utils::meta {
    /// @brief Read by calling @code get(const_var<Var>{})@endcode.
    /// @remark @code get@endcode does not belong to any namespace - it is searched via ADL.
    template <typename Var>
    class const_var {
        friend consteval auto get(const_var);
    };
    /// @brief Declare a @code const-var@endcode by instantiating the template.
    template <typename Var, auto Value>
    class define {
        friend consteval auto get(const_var<Var>) { return Value; }
    };

    /// @brief Checks the existence of a @code const_var@endcode.
    template <typename Var, auto = get(utils::meta::const_var<Var>{})>
    consteval bool exists(const_var<Var>) { return true; }
    /// @brief Fallback.
    consteval bool exists(...) { return false; }

    namespace detail {
        template <typename Var, std::uintmax_t Value>
        struct counter;
    }
    /// @brief A compile-time counter. An undeclared counter is defined to have the value 0.
    /// @tparam Increment: whether to increment the counter after the read
    /// @tparam Value: sets the value of the counter to the maximum of its original value and @code Value@endcode before the read
    template <typename Var = void, bool Increment = false, std::uintmax_t Value = 0, typename Id = decltype([]{})>
    consteval std::uintmax_t get_counter() {
        if constexpr (exists(const_var<detail::counter<Var, Value>>{})) {
            return get_counter<Var, Increment, Value + 1, Id>();
        } else {
            if constexpr (Increment) {
                (void) define<detail::counter<Var, Value>, true>{};
            }
            return Value;
        }
    }

    /// @brief Read by calling @code get(var<Var, Version>{})@endcode.
    ///
    /// If not specified, @code Version@endcode is defaults to the latest.
    template <typename Var, std::uintmax_t Version = get_counter<Var>() - 1>
    class var {
        friend consteval auto get(var);
    };
    /// @brief Sets @code Var@endcode to @code Value@endcode.
    /// @code Version@endcode is automatically incremented at every instantiation of this template.
    template <typename Var, auto Value, std::uintmax_t Version = get_counter<Var, true>()>
    class set {
        friend consteval auto get(var<Var, Version>) { return Value; }
    };

    /// @brief Checks the existence of a @code var@endcode.
    template <typename Var, std::uintmax_t Version, auto = get(var<Var, Version>{})>
    constexpr bool exists(var<Var, Version>) { return true; }

    /// @brief Helper template for when you want a product type to be a variable name.
    template <typename... Ts>
    struct product_t;
}