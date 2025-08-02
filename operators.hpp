#pragma once
#include <functional>
#include <type_traits>
#include "functional.hpp"
#include "type.hpp"

/**
 * This header file contains *_op structs that add operator overloads to their parent classes using the mixin pattern.
 * Operator signature follows usual semantics to encourage proper use, although their behavior can be highly customized.
 * This is done by passing your custom *_op_functors as a template parameter to a *_op struct.
 *
 * Requirement(s) for a parent class of *_op:\n
 * Let ```P``` be the parent class, and ```O``` be the child class.
 * For any operator ```@``` defined by ```O```,
 * denote its corresponding functor member in ```*_op_functors``` (```OF```) by ```F```. Then,
 * 1. If ```@``` is a binary operator
 * ```static_cast<result_type>(F(this->to_underlying(), other))``` must be well-formed in the context of ```const P```;
 * 2. If ```@``` is a unary operator,
 * ```static_cast<result_type>(F(this->to_underlying()))``` must be well-formed in the context of ```const P```;
 * 3. If ```@``` is an assignment operator,
 * ```F(this->to_underlying(), other)``` must be well-formed in the context of ```P```;
 * 4. If ```@``` is a pre-fix operator, ```F(this->to_underlying())``` must be well-formed in the context of ```P```;
 * 5. If ```@``` is a post-fix operator,
 * ```static_cast<P>(F(this->to_underlying()))``` must be well-formed in the context of ```P```;
 * 6. If ```@``` is a binary or assignment operator,
 * for any (possibly cv-qualified) ```Other``` (the type of ```other```) such that
 * ```decltype(OF::*_traits)::constraint<P, Other>``` is true,
 * either ```std::remove_cvref_t<Other>``` is ```P```, or ```std::is_constructible_v<P, Other>``` must be true,
 * .
 * where ```result_type``` is ```decltype(OF::*_traits)::result<P, decltype(other)>```.
 *
 * Note: You can disable an operation by setting ```OF::@``` to ```invalid_op```.
 */

namespace utils {
#define UTILS_BINARY_OP(op, name)\
    template <typename Self, typename Other>\
    constexpr decltype(auto) operator op(this const Self& self, Other&& other)\
    requires (\
        !std::is_same_v<decltype(Funcs.name), disable_op> &&\
        decltype(Funcs.binary_traits)::template constraint<Self, Other>::value\
    ) {\
        using result_type = decltype(Funcs.binary_traits)::template result<Self, Other>::type;\
        if constexpr (is_equiv_v<Self, Other>) {\
            return static_cast<result_type>(Funcs.name(self.to_underlying(), other.to_underlying()));\
        } else {\
            Self conv(std::forward<Other>(other));\
            return static_cast<result_type>(Funcs.name(self.to_underlying(), conv.to_underlying()));\
        }\
    }
#define UTILS_UNARY_OP(op, name)\
    template <typename Self>\
    constexpr decltype(auto) operator op(this const Self& self)\
    requires (!std::is_same_v<decltype(Funcs.name), disable_op>) {\
        using result_type = decltype(Funcs.unary_traits)::template result<Self>::type;\
        return static_cast<result_type>(Funcs.name(self.to_underlying()));\
    }
#define UTILS_ASG_OP(op, name)\
    template <typename Self, typename Other>\
    constexpr Self& operator op(this Self& self, Other&& other)\
    requires (\
        !std::is_same_v<decltype(Funcs.name), disable_op> &&\
        decltype(Funcs.asg_traits)::template constraint<Self, Other>::value\
    ) {\
        if constexpr (is_equiv_v<Self, Other>) {\
            Funcs.name(self.to_underlying(), other.to_underlying());\
        } else {\
            Self conv(std::forward<Other>(other));\
            Funcs.name(self.to_underlying(), conv.to_underlying());\
        }\
        return self;\
    }
#define UTILS_FIX_OP(op, name)\
    template <typename Self>\
    constexpr Self& operator op(this Self& self)\
    requires (!std::is_same_v<decltype(Funcs.pre_##name), disable_op>) {\
        Funcs.pre_##name(self.to_underlying());\
        return self;\
    }\
    template <typename Self>\
    constexpr Self operator op(this Self& self, int)\
    requires (!std::is_same_v<decltype(Funcs.post_##name), disable_op>) {\
        return static_cast<Self>(Funcs.post_##name(self.to_underlying()));\
    }

    struct disable_op {};
    namespace detail {
        template <typename Self, typename Other>
        struct get_self : std::type_identity<Self> {};
        template <typename Self, typename Other>
        struct is_compatible : std::bool_constant<equiv_to<Self, Other> || std::convertible_to<Other, Self>> {};
    }
    template <template<typename Self> typename ResultTrait = std::type_identity>
    struct unary_op_traits {
        using tag = struct unary_op_traits_tag;
        template <typename Self>
        struct result : ResultTrait<Self> {};
    };
    template <
        template<typename Self, typename Other> typename ResultTrait = detail::get_self,
        template<typename Self, typename Other> typename ConstraintTrait = detail::is_compatible>
    struct binary_op_traits {
        using tag = struct binary_op_traits_tag;
        template <typename Self, typename Other>
        struct result : ResultTrait<Self, Other> {};
        template <typename Self, typename Other>
        struct constraint : ConstraintTrait<Self, Other> {};
    };
    template <template<typename Self, typename Other> typename ConstraintTrait = detail::is_compatible>
    struct asg_op_traits {
        using tag = struct asg_op_traits_tag;
        template <typename Self, typename Other>
        struct constraint : ConstraintTrait<Self, Other> {};
    };

    template <
        typename Plus = std::plus<>,
        typename Minus = std::minus<>,
        typename Mul = std::multiplies<>,
        typename Div = std::divides<>,
        typename Mod = std::modulus<>,
        tagged<binary_op_traits_tag> BinaryTraits = binary_op_traits<>>
    struct integral_op_functors {
        using tag = struct integral_op_functors_tag;
        Plus plus{};
        Minus minus{};
        Mul multiplies{};
        Div divides{};
        Mod modulus{};
        BinaryTraits binary_traits{};
    };
    template <integral_op_functors Funcs = {}>
    struct integral_ops {
    protected:
        using utils_ops_base_type = integral_ops;
    public:
        UTILS_BINARY_OP(+, plus)
        UTILS_BINARY_OP(-, minus)
        UTILS_BINARY_OP(*, multiplies)
        UTILS_BINARY_OP(/, divides)
        UTILS_BINARY_OP(%, modulus)
    };

    template <
        typename PlusAsg = plus_asg<>,
        typename MinusAsg = minus_asg<>,
        typename MulAsg = multiplies_asg<>,
        typename DivAsg = divides_asg<>,
        typename ModAsg = modulus_asg<>,
        tagged<asg_op_traits_tag> AsgTraits = asg_op_traits<>>
    struct integral_asg_op_functors {
        using tag = struct integral_asg_op_functors_tag;
        PlusAsg plus_asg{};
        MinusAsg minus_asg{};
        MulAsg multiplies_asg{};
        DivAsg divides_asg{};
        ModAsg modulus_asg{};
        AsgTraits asg_traits{};
    };
    template <integral_asg_op_functors Funcs = {}>
    struct integral_asg_ops {
    protected:
        using utils_ops_base_type = integral_asg_ops;
    public:
        UTILS_ASG_OP(+=, plus_asg)
        UTILS_ASG_OP(-=, minus_asg)
        UTILS_ASG_OP(*=, multiplies_asg)
        UTILS_ASG_OP(/=, divides_asg)
        UTILS_ASG_OP(%=, modulus_asg)
    };
    template <integral_op_functors Funcs = {}, integral_asg_op_functors AsgFuncs = {}>
    struct integral_full_ops : integral_ops<Funcs>, integral_asg_ops<AsgFuncs> {
    protected:
        using utils_ops_base_type = integral_full_ops;
    };

    template <
        typename BitAnd = std::bit_and<>,
        typename BitOr = std::bit_or<>,
        typename BitXor = std::bit_xor<>,
        typename BitNot = std::bit_not<>,
        tagged<binary_op_traits_tag> BinaryTraits = binary_op_traits<>,
        tagged<unary_op_traits_tag> UnaryTraits = unary_op_traits<>>
    struct bit_op_functors {
        using tag = struct bit_op_functors_tag;
        BitAnd bit_and{};
        BitOr bit_or{};
        BitXor bit_xor{};
        BitNot bit_not{};
        BinaryTraits binary_traits{};
        UnaryTraits unary_traits{};
    };
    template <bit_op_functors Funcs = {}>
    struct bit_ops {
    protected:
        using utils_ops_base_type = bit_ops;
    public:
        UTILS_BINARY_OP(&, bit_and)
        UTILS_BINARY_OP(|, bit_or)
        UTILS_BINARY_OP(^, bit_xor)
        UTILS_UNARY_OP(~, bit_not)
    };

    template <
        typename BitAndAsg = bit_and_asg<>,
        typename BitOrAsg = bit_or_asg<>,
        typename BitXorAsg = bit_xor_asg<>,
        tagged<asg_op_traits_tag> AsgTraits = asg_op_traits<>>
    struct bit_asg_op_functors {
        using tag = struct bit_asg_op_functors_tag;
        BitAndAsg bit_and_asg{};
        BitOrAsg bit_or_asg{};
        BitXorAsg bit_xor_asg{};
        AsgTraits asg_traits{};
    };
    template <bit_asg_op_functors Funcs = {}>
    struct bit_asg_ops {
    protected:
        using utils_ops_base_type = bit_asg_ops;
    public:
        UTILS_ASG_OP(&=, bit_and_asg)
        UTILS_ASG_OP(|=, bit_or_asg)
        UTILS_ASG_OP(^=, bit_xor_asg)
    };
    template <bit_op_functors Funcs = {}, bit_asg_op_functors AsgFuncs = {}>
    struct bit_full_ops : bit_ops<Funcs>, bit_asg_ops<AsgFuncs> {
    protected:
        using utils_ops_base_type = bit_full_ops;
    };

    template <
        typename ShiftLeft = shift_left<>,
        typename ShiftRight = shift_right<>,
        tagged<binary_op_traits_tag> BinaryTraits = binary_op_traits<detail::get_self, always_true>>
    struct shift_op_functors {
        using tag = struct shift_op_functors_tag;
        ShiftLeft shift_left{};
        ShiftRight shift_right{};
        BinaryTraits binary_traits{};
    };
    template <shift_op_functors Funcs = {}>
    struct shift_ops {
    protected:
        using utils_ops_base_type = shift_ops;
    public:
        template <typename Self, integer_like Other>
        constexpr decltype(auto) operator<<(this const Self& self, const Other& n)
        requires (
            !std::is_same_v<decltype(Funcs.shift_left), disable_op> &&
            decltype(Funcs.binary_traits)::template constraint<Self, Other>::value
        ) {
            using result_type = decltype(Funcs.binary_traits)::template result<Self, Other>::type;
            return static_cast<result_type>(Funcs.shift_left(self.to_underlying(), to_fundamental(n)));
        }
        template <typename Self, integer_like Other>
        constexpr decltype(auto) operator>>(this const Self& self, const Other& n)
        requires (
            !std::is_same_v<decltype(Funcs.shift_right), disable_op> &&
            decltype(Funcs.binary_traits)::template constraint<Self, Other>::value
        ) {
            using result_type = decltype(Funcs.binary_traits)::template result<Self, Other>::type;
            return static_cast<result_type>(Funcs.shift_right(self.to_underlying(), to_fundamental(n)));
        }
    };

    template <
        typename ShiftLeftAsg = shift_left_asg<>,
        typename ShiftRightAsg = shift_right_asg<>,
        tagged<asg_op_traits_tag> AsgTraits = asg_op_traits<always_true>>
    struct shift_asg_op_functors {
        using tag = struct shift_asg_op_functors_tag;
        ShiftLeftAsg shift_left_asg{};
        ShiftRightAsg shift_right_asg{};
        AsgTraits asg_traits{};
    };
    template <shift_asg_op_functors Funcs = {}>
    struct shift_asg_ops {
    protected:
        using utils_ops_base_type = shift_asg_ops;
    public:
        template <typename Self, integer_like Other>
        constexpr Self& operator<<=(this Self& self, const Other& n)
        requires (
            !std::is_same_v<decltype(Funcs.shift_left_asg), disable_op> &&
            decltype(Funcs.asg_traits)::template constraint<Self, Other>::value
        ) {
            Funcs.shift_left_asg(self.to_underlying(), to_fundamental(n));
            return self;
        }
        template <typename Self, integer_like Other>
        constexpr Self& operator>>=(this Self& self, const Other& n)
        requires (
            !std::is_same_v<decltype(Funcs.shift_right_asg), disable_op> &&
            decltype(Funcs.asg_traits)::template constraint<Self, Other>::value
        ) {
            Funcs.shift_right_asg(self.to_underlying(), to_fundamental(n));
            return self;
        }
    };
    template <shift_op_functors Funcs = {}, shift_asg_op_functors AsgFuncs = {}>
    struct shift_full_ops : shift_ops<Funcs>, shift_asg_ops<AsgFuncs> {
    protected:
        using utils_ops_base_type = shift_full_ops;
    };

    template <
        typename Neg = std::negate<>,
        tagged<unary_op_traits_tag> UnaryTraits = unary_op_traits<>>
    struct sign_op_functors {
        using tag = struct sign_op_functors_tag;
        Neg negate{};
        UnaryTraits unary_traits{};
    };
    template <sign_op_functors Funcs = {}>
    struct sign_ops {
    protected:
        using utils_ops_base_type = sign_ops;
    public:
        UTILS_UNARY_OP(-, negate)
    };

    template <
        typename PreInc = pre_increment<>,
        typename PostInc = post_increment<>,
        typename PreDec = pre_decrement<>,
        typename PostDec = post_decrement<>>
    struct fix_op_functors {
        using tag = struct fix_op_functors_tag;
        PreInc pre_increment{};
        PostInc post_increment{};
        PreDec pre_decrement{};
        PostDec post_decrement{};
    };
    template <fix_op_functors Funcs = {}>
    struct fix_ops {
    protected:
        using utils_ops_base_type = fix_ops;
    public:
        UTILS_FIX_OP(++, increment)
        UTILS_FIX_OP(--, decrement)
    };

    template <
        integral_op_functors IntFuncs = {}, integral_asg_op_functors IntAsgFuncs = {},
        bit_op_functors BitFuncs = {}, bit_asg_op_functors BitAsgFuncs = {},
        shift_op_functors ShiftFuncs = {}, shift_asg_op_functors ShiftAsgFuncs = {},
        sign_op_functors SignFuncs = {}, fix_op_functors FixFuncs = {}>
    struct arithmetic_ops :
        integral_full_ops<IntFuncs, IntAsgFuncs>,
        bit_full_ops<BitFuncs, BitAsgFuncs>,
        shift_full_ops<ShiftFuncs, ShiftAsgFuncs>,
        sign_ops<SignFuncs>,
        fix_ops<FixFuncs> {
    protected:
        using utils_ops_base_type = arithmetic_ops;
    public:
        using integral_full_ops<IntFuncs, IntAsgFuncs>::operator-;
        using sign_ops<SignFuncs>::operator-;
    };

    template <
        typename And = std::logical_and<>,
        typename Or = std::logical_or<>,
        typename Not = std::logical_not<>>
    struct logical_op_functors {
        using tag = struct logical_op_functors_tag;
        And logical_and{};
        Or logical_or{};
        Not logical_not{};
    };
    template <logical_op_functors Funcs = {}>
    struct logical_ops {
    protected:
        using utils_ops_base_type = logical_ops;
    public:
        UTILS_BINARY_OP(&&, logical_and)
        UTILS_BINARY_OP(||, logical_or)
        UTILS_UNARY_OP(!, logical_not)
    };
#undef UTILS_BINARY_OP
#undef UTILS_ASG_OP
#undef UTILS_UNARY_OP
#undef UTILS_FIX_OP
}