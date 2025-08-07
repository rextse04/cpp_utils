#pragma once
#include <cstdint>

/**
 * This file contains utilities that enable stateful metaprogramming,
 * which allows you to make compile-time <b>modifiable</b> variables.
 * This is done via a technique called "friend injection".
 *
 * In this file, compile-time variables are named by types.
 * That is, such variables are defined, accessed and modified via the types they are associated with.
 * Note that you are allowed to use the same type to name variables of different categories.
 * For example, you can define a ```const_var<T>``` and ```<i>counter</i><T>``` and they will be separate variables.
 * There are three categories of variables:
 * |          Category         |    Written via    |      Read via     |                     Remarks                    |
 * |:-------------------------:|:-----------------:|:-----------------:|:----------------------------------------------:|
 * |      ```const_var```      |    ```define```   |     ```get```     | A ```const_var``` can only be written to once. |
 * | <pre><i>counter</i></pre> | ```get_counter``` | ```get_counter``` | There is no explicit struct for this category. |
 * |         ```var```         |     ```set```     |     ```get```     |                                                |
 *
 * In the implementation, some structs may have ```Id``` as one of their template parameters.
 * This is necessary to prevent the compiler from caching previous results.
 * Do not fill in that parameter unless you know what you are doing.
 */

namespace utils::meta {
    /// Read by calling ```get(const_var<Var>{})```.
    /// @remark ```get``` does not belong to any namespace - it is searched via ADL.
    template <typename Var>
    class const_var {
        friend consteval const auto& get(const_var);
    };
    /// Declare a ```const-var``` by instantiating the template.
    template <typename Var, auto Value>
    class define {
        static constexpr auto value = Value;
        friend consteval const auto& get(const_var<Var>) { return value; }
    };

    /// Checks the existence of a ```const_var```.
    template <typename Var, auto = get(utils::meta::const_var<Var>{})>
    consteval bool exists(const_var<Var>) { return true; }
    /// Fallback.
    consteval bool exists(...) { return false; }

    namespace detail {
        template <typename Var, std::uintmax_t Value>
        struct counter;
    }
    /// A compile-time counter. An undeclared counter is defined to have the value 0.
    /// @tparam Increment: whether to increment the counter after the read
    /// @tparam Value: sets the value of the counter to the maximum of its original value and ```Value``` before the read
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

    /// Read by calling ```get(var<Var, Version>{})```.
    /// If not specified, ```Version``` is defaults to the latest.
    template <typename Var, std::uintmax_t Version = get_counter<Var>() - 1>
    class var {
        friend consteval auto& get(var);
    };
    /// Sets ```Var``` to ```Value```.
    /// ```Version``` is automatically incremented at every instantiation of this template.
    template <typename Var, auto Value, std::uintmax_t Version = get_counter<Var, true>()>
    class set {
        static constexpr auto value = Value;
        friend consteval auto& get(var<Var, Version>) { return value; }
    };

    /// Checks the existence of a ```var```.
    template <typename Var, std::uintmax_t Version, auto = get(var<Var, Version>{})>
    constexpr bool exists(var<Var, Version>) { return true; }
}