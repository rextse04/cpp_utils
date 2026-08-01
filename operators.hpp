#pragma once
#include <functional>
#include <type_traits>
#include <concepts>
#include <utility>
#include "functional.hpp"
#include "type.hpp"

namespace utils {
#define UTILS_BINARY_OP(op, name, classname)\
    template <typename A, typename B,\
        typename AD = std::remove_cvref_t<A>, typename BD = std::remove_cvref_t<B>,\
        typename Self = std::conditional_t<std::derived_from<AD, classname>, A&&, B&&>, typename SelfD = std::remove_cvref_t<Self>>\
    friend constexpr decltype(auto) operator op(A&& a, B&& b)\
    requires (\
        !std::is_same_v<decltype(Funcs.name), disable_op_t> &&\
        (std::derived_from<AD, classname> || std::derived_from<BD, classname>) &&\
        decltype(Funcs.binary_traits)::template constraint<Self, AD, BD>::value &&\
        requires { Funcs.name(SelfD::to_underlying(std::forward<A>(a)), SelfD::to_underlying(std::forward<B>(b))); }\
    ) {\
        using R = decltype(Funcs.binary_traits)::template result<Self, A&&, B&&>::type;\
        const auto ret = [&a, &b] {\
            return Funcs.name(SelfD::to_underlying(std::forward<A>(a)), SelfD::to_underlying(std::forward<B>(b)));\
        };\
        if constexpr (std::is_same_v<R, deduce_t>) return ret();\
        else return static_cast<R>(ret());\
    }
#define UTILS_ASG_OP(op, name)\
    template <typename Self, typename Other, typename SelfD = std::remove_cvref_t<Self>>\
    constexpr decltype(auto) operator op(this Self&& self, Other&& other)\
    requires (\
        !std::is_same_v<decltype(Funcs.name), disable_op_t> &&\
        decltype(Funcs.asg_traits)::template constraint<Self&&, Self&&, Other&&>::value &&\
        requires { Funcs.name(SelfD::to_underlying(std::forward<Self>(self)), SelfD::to_underlying(std::forward<Other>(other))); }\
    ) {\
        Funcs.name(SelfD::to_underlying(std::forward<Self>(self)), SelfD::to_underlying(std::forward<Other>(other)));\
        return self;\
    }
#define UTILS_UNARY_OP(op, name, ...)\
    template <typename Self, typename SelfD = std::remove_cvref_t<Self>>\
    constexpr decltype(auto) operator op(this Self&& self __VA_OPT__(,) __VA_ARGS__)\
    requires (\
        !std::is_same_v<decltype(Funcs.name), disable_op_t> &&\
        decltype(Funcs.unary_traits)::template constraint<Self&&>::value &&\
        requires { Funcs.name(SelfD::to_underlying(std::forward<Self>(self))); }\
    ) {\
        const auto ret = [&self] { return Funcs.name(SelfD::to_underlying(std::forward<Self>(self))); };\
        using R = decltype(Funcs.unary_traits)::template result<Self&&>::type;\
        if constexpr (std::is_same_v<R, deduce_t>) return ret();\
        else return static_cast<R>(ret());\
    }
#define UTILS_FIX_OP(op, name)\
    template <typename Self, typename SelfD = std::remove_cvref_t<Self>>\
    constexpr decltype(auto) operator op(this Self&& self)\
    requires (\
        !std::is_same_v<decltype(Funcs.pre_##name), disable_op_t> &&\
        decltype(Funcs.unary_traits)::template constraint<Self&&>::value &&\
        requires { Funcs.pre_##name(SelfD::to_underlying(std::forward<Self>(self))); }\
    ) {\
        Funcs.pre_##name(SelfD::to_underlying(std::forward<Self>(self)));\
        return self;\
    }\
    UTILS_UNARY_OP(op, post_##name, int)

    /// @brief Denotes an operator should be disabled.
    inline constexpr struct disable_op_t {} disable_op;
    /// @brief Denotes the return type of the operator overloading function should be automatically deduced (by `decltype(auto)`).
    struct deduce_t;
    /// @brief The default trait set for binary operators, except assignment and prefix operators.
    struct default_binary_op_traits {
        template <typename, typename, typename>
        struct constraint : std::true_type {};
        template <typename Self, typename, typename>
        struct result : std::remove_cvref<Self> {};
    };
    /// @brief The default trait set for assignment operators.
    struct default_asg_op_traits {
        template <typename Self, typename, typename>
        struct constraint : std::is_lvalue_reference<Self> {};
    };
    /// @brief The default trait set for unary operators, except postfix operators.
    struct default_unary_op_traits {
        template <typename>
        struct constraint : std::true_type {};
        template <typename Self>
        struct result : std::decay<Self> {};
    };
    /// @brief The default trait set for prefix operators (i.e. `++t` and `--t`).
    struct default_prefix_op_traits {
        template <typename Self>
        struct constraint : std::is_lvalue_reference<Self> {};
        template <typename Self>
        struct result : std::type_identity<Self> {};
    };
    /// @brief The default trait set for postfix operators (i.e. `t++` and `t--`).
    struct default_postfix_op_traits {
        template <typename Self>
        struct constraint : std::is_lvalue_reference<Self> {};
        template <typename Self>
        struct result : std::decay<Self> {};
    };

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::integral_ops`.
    template <
        typename Plus = std::plus<>,
        typename Minus = std::minus<>,
        typename Mul = std::multiplies<>,
        typename Div = std::divides<>,
        typename Mod = std::modulus<>,
        typename BinaryTraits = default_binary_op_traits>
    struct integral_op_functors {
        Plus plus{};
        Minus minus{};
        Mul multiplies{};
        Div divides{};
        Mod modulus{};
        BinaryTraits binary_traits{};
    };
    /// @brief Adds support for `a + b`, `a - b`, `a * b`, `a / b` and `a % b`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::integral_ops`.
    template <integral_op_functors Funcs = {}>
    struct integral_ops {
        UTILS_BINARY_OP(+, plus, integral_ops)
        UTILS_BINARY_OP(-, minus, integral_ops)
        UTILS_BINARY_OP(*, multiplies, integral_ops)
        UTILS_BINARY_OP(/, divides, integral_ops)
        UTILS_BINARY_OP(%, modulus, integral_ops)
    };

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::integral_asg_ops`.
    template <
        typename PlusAsg = plus_asg<>,
        typename MinusAsg = minus_asg<>,
        typename MulAsg = multiplies_asg<>,
        typename DivAsg = divides_asg<>,
        typename ModAsg = modulus_asg<>,
        typename AsgTraits = default_asg_op_traits>
    struct integral_asg_op_functors {
        PlusAsg plus_asg{};
        MinusAsg minus_asg{};
        MulAsg multiplies_asg{};
        DivAsg divides_asg{};
        ModAsg modulus_asg{};
        AsgTraits asg_traits{};
    };
    /// @brief Adds support for `a += b`, `a -= b`, `a *= b`, `a /= b`, `a %= b`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::integral_asg_ops`.
    template <integral_asg_op_functors Funcs = {}>
    struct integral_asg_ops {
        UTILS_ASG_OP(+=, plus_asg)
        UTILS_ASG_OP(-=, minus_asg)
        UTILS_ASG_OP(*=, multiplies_asg)
        UTILS_ASG_OP(/=, divides_asg)
        UTILS_ASG_OP(%=, modulus_asg)
    };
    /// @brief A convenience template inheriting from `utils::integral_full_ops` and `utils::integral_asg_ops`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::integral_ops`.
    /// @tparam AsgFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::integral_asg_ops`.
    template <integral_op_functors Funcs = {}, integral_asg_op_functors AsgFuncs = {}>
    struct integral_full_ops : integral_ops<Funcs>, integral_asg_ops<AsgFuncs> {};

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::bit_op`.
    template <
        typename BitAnd = std::bit_and<>,
        typename BitOr = std::bit_or<>,
        typename BitXor = std::bit_xor<>,
        typename BitNot = std::bit_not<>,
        typename BinaryTraits = default_binary_op_traits,
        typename UnaryTraits = default_unary_op_traits>
    struct bit_op_functors {
        BitAnd bit_and{};
        BitOr bit_or{};
        BitXor bit_xor{};
        BitNot bit_not{};
        BinaryTraits binary_traits{};
        UnaryTraits unary_traits{};
    };
    /// @brief Adds support for `a & b`, `a | b`, `a ^ b` and `~t`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::bit_ops`.
    template <bit_op_functors Funcs = {}>
    struct bit_ops {
        UTILS_BINARY_OP(&, bit_and, bit_ops)
        UTILS_BINARY_OP(|, bit_or, bit_ops)
        UTILS_BINARY_OP(^, bit_xor, bit_ops)
        UTILS_UNARY_OP(~, bit_not)
    };

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::bit_asg_ops`.
    template <
        typename BitAndAsg = bit_and_asg<>,
        typename BitOrAsg = bit_or_asg<>,
        typename BitXorAsg = bit_xor_asg<>,
        typename AsgTraits = default_asg_op_traits>
    struct bit_asg_op_functors {
        BitAndAsg bit_and_asg{};
        BitOrAsg bit_or_asg{};
        BitXorAsg bit_xor_asg{};
        AsgTraits asg_traits{};
    };
    /// @brief Adds support for `a &= b`, `a |= b` and `a ^= b`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::bit_asg_ops`.
    template <bit_asg_op_functors Funcs = {}>
    struct bit_asg_ops {
        UTILS_ASG_OP(&=, bit_and_asg)
        UTILS_ASG_OP(|=, bit_or_asg)
        UTILS_ASG_OP(^=, bit_xor_asg)
    };
    /// @brief A convenience template inheriting from `utils::bit_ops` and `utils::bit_asg_ops`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::bit_ops`.
    /// @tparam AsgFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::bit_asg_ops`.
    template <bit_op_functors Funcs = {}, bit_asg_op_functors AsgFuncs = {}>
    struct bit_full_ops : bit_ops<Funcs>, bit_asg_ops<AsgFuncs> {};

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::shift_ops`.
    template <
        typename ShiftLeft = shift_left<>,
        typename ShiftRight = shift_right<>,
        typename BinaryTraits = default_binary_op_traits>
    struct shift_op_functors {
        ShiftLeft shift_left{};
        ShiftRight shift_right{};
        BinaryTraits binary_traits{};
    };
    /// @brief Adds support for `a << b` and `a >> b`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::shift_ops`.
    template <shift_op_functors Funcs = {}>
    struct shift_ops {
        UTILS_BINARY_OP(<<, shift_left, shift_ops)
        UTILS_BINARY_OP(>>, shift_right, shift_ops)
    };

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::shift_asg_ops`.
    template <
        typename ShiftLeftAsg = shift_left_asg<>,
        typename ShiftRightAsg = shift_right_asg<>,
        typename AsgTraits = default_asg_op_traits>
    struct shift_asg_op_functors {
        ShiftLeftAsg shift_left_asg{};
        ShiftRightAsg shift_right_asg{};
        AsgTraits asg_traits{};
    };
    /// @brief Adds support for `a <<= b` and `a >>= b`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::shift_asg_ops`.
    template <shift_asg_op_functors Funcs = {}>
    struct shift_asg_ops {
        UTILS_ASG_OP(<<=, shift_left_asg)
        UTILS_ASG_OP(>>=, shift_right_asg)
    };
    /// @brief A convenience template inheriting from `utils::shift_ops` and `utils::shift_asg_ops`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::shift_ops`.
    /// @tparam AsgFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::shift_asg_ops`.
    template <shift_op_functors Funcs = {}, shift_asg_op_functors AsgFuncs = {}>
    struct shift_full_ops : shift_ops<Funcs>, shift_asg_ops<AsgFuncs> {};

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::sign_ops`.
    template <
        typename Neg = std::negate<>,
        typename UnaryTraits = default_unary_op_traits>
    struct sign_op_functors {
        Neg negate{};
        UnaryTraits unary_traits{};
    };
    /// @brief Adds support for `+t` and `-t`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::sign_ops`.
    template <sign_op_functors Funcs = {}>
    struct sign_ops {
        UTILS_UNARY_OP(-, negate)
    };

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::fix_ops`.
    template <
        typename PreInc = pre_increment<>,
        typename PostInc = post_increment<>,
        typename PreDec = pre_decrement<>,
        typename PostDec = post_decrement<>,
        typename PrefixTraits = default_prefix_op_traits,
        typename UnaryTraits = default_postfix_op_traits>
    struct fix_op_functors {
        PreInc pre_increment{};
        PostInc post_increment{};
        PreDec pre_decrement{};
        PostDec post_decrement{};
        PrefixTraits prefix_traits{};
        UnaryTraits unary_traits{};
    };
    /// @brief Adds support for `++t`, `t++`, `--t` and `t--`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::fix_ops`.
    template <fix_op_functors Funcs = {}>
    struct fix_ops {
        UTILS_FIX_OP(++, increment)
        UTILS_FIX_OP(--, decrement)
    };

    /// @brief A convenience template inheriting from
    /// `utils::integral_full_ops`, `utils::bit_full_ops`, `utils::shift_full_ops`, `utils::sign_ops` and `utils::fix_ops`.
    /// @tparam IntFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::integral_ops`.
    /// @tparam IntAsgFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::integral_asg_ops`.
    /// @tparam BitFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::bit_ops`.
    /// @tparam BitAsgFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::bit_asg_ops`.
    /// @tparam ShiftFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::shift_ops`.
    /// @tparam ShiftAsgFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::shift_asg_ops`.
    /// @tparam SignFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::sign_ops`.
    /// @tparam FixFuncs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::fix_ops`.
    template <
        integral_op_functors IntFuncs = {}, integral_asg_op_functors IntAsgFuncs = {},
        bit_op_functors BitFuncs = {}, bit_asg_op_functors BitAsgFuncs = {},
        shift_op_functors ShiftFuncs = {}, shift_asg_op_functors ShiftAsgFuncs = {},
        sign_op_functors SignFuncs = {},
        fix_op_functors FixFuncs = {}>
    struct arithmetic_ops :
        integral_full_ops<IntFuncs, IntAsgFuncs>,
        bit_full_ops<BitFuncs, BitAsgFuncs>,
        shift_full_ops<ShiftFuncs, ShiftAsgFuncs>,
        sign_ops<SignFuncs>,
        fix_ops<FixFuncs> {};

    /// @brief [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet) for `utils::logical_ops`.
    template <
        typename And = std::logical_and<>,
        typename Or = std::logical_or<>,
        typename Not = std::logical_not<>,
        typename BinaryTraits = default_binary_op_traits,
        typename UnaryTraits = default_unary_op_traits>
    struct logical_op_functors {
        And logical_and{};
        Or logical_or{};
        Not logical_not{};
        BinaryTraits binary_traits{};
        UnaryTraits unary_traits{};
    };
    /// @brief Adds support for `a && b`, `a || b` and `!t`.
    /// @tparam Funcs: An [<i>OperatorFunctorSet</i>](OperatorFunctorSet) for `utils::logical_ops`.
    template <logical_op_functors Funcs = {}>
    struct logical_ops {
        UTILS_BINARY_OP(&&, logical_and, logical_ops)
        UTILS_BINARY_OP(||, logical_or, logical_ops)
        UTILS_UNARY_OP(!, logical_not)
    };
#undef UTILS_BINARY_OP
#undef UTILS_ASG_OP
#undef UTILS_UNARY_OP
#undef UTILS_FIX_OP
}