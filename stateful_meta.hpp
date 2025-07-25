#pragma once
#include <cstdint>

namespace utils::meta {
    template <typename Var>
    class const_var {
        friend consteval const auto& get(const_var);
    };
    template <typename Var, auto Value>
    class define {
        static constexpr auto value = Value;
        friend consteval const auto& get(const_var<Var>) { return value; }
    };
}

template <typename Var, auto = get(utils::meta::const_var<Var>{})>
consteval bool exists(utils::meta::const_var<Var>) { return true; }
constexpr bool exists(...) { return false; }

namespace utils::meta {
    namespace detail {
        template <typename Var, std::uintmax_t Value>
        struct counter;
    }
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

    template <typename Var, std::uintmax_t Version = get_counter<Var>() - 1>
    class var {
        friend consteval auto& get(var);
    };
    template <typename Var, auto Value, std::uintmax_t Version = get_counter<Var, true>()>
    class set {
        static constexpr auto value = Value;
        friend consteval auto& get(var<Var, Version>) { return value; }
    };

    template <typename T, std::uintmax_t>
    struct list_item;
}