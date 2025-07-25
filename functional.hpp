#pragma once
#include <utility>

namespace utils {
#define UTILS_ASG_FUNCTOR(name, op)\
    template <typename T = void>\
    struct name {\
        static constexpr T& operator()(T& lhs, const T& rhs) {\
            return lhs op rhs;\
        }\
    };\
    template <>\
    struct name<void> {\
        static constexpr decltype(auto) operator()(auto&& lhs, auto&& rhs) {\
            return std::forward<decltype(lhs)>(lhs) op std::forward<decltype(rhs)>(rhs);\
        }\
    };
    UTILS_ASG_FUNCTOR(plus_asg, +=)
    UTILS_ASG_FUNCTOR(minus_asg, -=)
    UTILS_ASG_FUNCTOR(multiplies_asg, *=)
    UTILS_ASG_FUNCTOR(divides_asg, /=)
    UTILS_ASG_FUNCTOR(modulus_asg, %=)
    UTILS_ASG_FUNCTOR(bit_and_asg, &=)
    UTILS_ASG_FUNCTOR(bit_or_asg, |=)
    UTILS_ASG_FUNCTOR(bit_xor_asg, ^=)
#undef UTILS_ASG_FUNCTOR

    template <typename T = void>
    struct shift_left {
        static constexpr T operator()(const T& lhs, int rhs) {
            return lhs << rhs;
        }
    };
    template <>
    struct shift_left<void> {
        static constexpr decltype(auto) operator()(auto&& lhs, auto&& rhs) {
            return std::forward<decltype(lhs)>(lhs) << std::forward<decltype(rhs)>(rhs);
        }
    };

    template <typename T = void>
    struct shift_right {
        static constexpr T operator()(const T& lhs, int rhs) {
            return lhs >> rhs;
        }
    };
    template <>
    struct shift_right<void> {
        static constexpr decltype(auto) operator()(auto&& lhs, auto&& rhs) {
            return std::forward<decltype(lhs)>(lhs) >> std::forward<decltype(rhs)>(rhs);
        }
    };

    template <typename T = void>
    struct shift_left_asg {
        static constexpr T& operator()(T& lhs, int rhs) {
            return lhs <<= rhs;
        }
    };
    template <>
    struct shift_left_asg<void> {
        static constexpr decltype(auto) operator()(auto&& lhs, auto&& rhs) {
            return std::forward<decltype(lhs)>(lhs) <<= std::forward<decltype(rhs)>(rhs);
        }
    };

    template <typename T = void>
    struct shift_right_asg {
        static constexpr T& operator()(T& lhs, int rhs) {
            return lhs >>= rhs;
        }
    };
    template <>
    struct shift_right_asg<void> {
        static constexpr decltype(auto) operator()(auto&& lhs, auto&& rhs) {
            return std::forward<decltype(lhs)>(lhs) >>= std::forward<decltype(rhs)>(rhs);
        }
    };

    template <typename T = void>
    struct promote {
        static constexpr auto operator()(const T& arg) {
            return +arg;
        }
    };
    template <>
    struct promote<void> {
        static constexpr decltype(auto) operator()(auto&& arg) {
            return +std::forward<decltype(arg)>(arg);
        }
    };

    template <typename T = void>
    struct pre_increment {
        static constexpr T& operator()(T& arg) {
            return ++arg;
        }
    };
    template <>
    struct pre_increment<void> {
        static constexpr decltype(auto) operator()(auto&& arg) {
            return ++std::forward<decltype(arg)>(arg);
        }
    };

    template <typename T = void>
    struct post_increment {
        static constexpr T operator()(const T& arg) {
            return arg++;
        }
    };
    template <>
    struct post_increment<> {
        static constexpr decltype(auto) operator()(auto&& arg) {
            return std::forward<decltype(arg)>(arg)++;
        }
    };

    template <typename T = void>
    struct pre_decrement {
        static constexpr T& operator()(T& arg) {
            return --arg;
        }
    };
    template <>
    struct pre_decrement<void> {
        static constexpr decltype(auto) operator()(auto&& arg) {
            return --std::forward<decltype(arg)>(arg);
        }
    };

    template <typename T = void>
    struct post_decrement {
        static constexpr T operator()(const T& arg) {
            return arg--;
        }
    };
    template <>
    struct post_decrement<> {
        static constexpr decltype(auto) operator()(auto&& arg) {
            return std::forward<decltype(arg)>(arg)--;
        }
    };

    template <auto Op, typename T = void>
    struct asg_wrap {
        static constexpr T operator()(T& lhs, const T& rhs) {
            return lhs = Op(lhs, rhs);
        }
    };
    template <auto Op>
    struct asg_wrap<Op, void> {
        static constexpr decltype(auto) operator()(auto& lhs, auto&& rhs) {
            return lhs = Op(lhs, std::forward<decltype(rhs)>(rhs));
        }
    };
}