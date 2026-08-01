#pragma once
#include <concepts>
#include <type_traits>
#include <limits>
#include <climits>
#include <utility>
#include <numeric>
#include <stdexcept>
#include <stdckdint.h>
#include <bit>
#include <cstdint>
#include <cstddef>
#include <ostream>
#include <format>
#include "operators.hpp"
#include "type.hpp"

namespace utils {
    namespace detail {
        template <typename>
        struct is_integer_like : std::false_type {};
        template <typename T>
        struct is_integer_like<const T> : is_integer_like<std::remove_const_t<T>> {};
        template <typename T>
        struct is_integer_like<volatile T> : is_integer_like<std::remove_volatile_t<T>> {};
        template <typename T>
        struct is_integer_like<const volatile T> : is_integer_like<std::remove_cv_t<T>> {};
        template <std::integral T>
        struct is_integer_like<T> : std::true_type {};
    }
    /// @group{is_integer_like}
    /// @brief Determines if a type is integer like.
    ///
    /// The construct is used by the library to determine if a type (ignoring cv-qualifiers) is integer like.
    /// The program is ill-formed, no diagnostics required, if the user specializes `utils::is_integer_like`
    /// such that there exists a type `T` where `std::remove_cvref_t<T>` is not a integer-like type
    /// (as specified by the standard library), but `utils::is_integer_like<T>::value` is `true`.
    /// @{
    template <typename T>
    struct is_integer_like : detail::is_integer_like<T> {};
    template <typename T>
    constexpr bool is_integer_like_v = is_integer_like<T>::value;
    template <typename T>
    concept integer_like = is_integer_like<T>::value;
    /// @}

    /// @group{width_of}
    /// @brief Determines the width of `T` based on `Info`.
    ///
    /// "width" here is defined as the number of bits that participate in the determination of the value of the significand (mantissa)
    /// in base-2 scientific notation.
    /// @tparam Info: A type which provides the interface of `std::numeric_limits` and gives information about `T`.
    /// @{
    template <typename T, typename Info = std::numeric_limits<T>>
    struct width_of : std::integral_constant<int, Info::digits + Info::is_signed> {};
    template <typename T>
    constexpr int width_of_v = width_of<T>::value;
    /// @}

    namespace detail {
        template <typename T, typename U>
        struct sane_common_type : std::common_type<T, U> {};
        template <typename T, typename U>
        requires (std::numeric_limits<std::decay_t<T>>::is_specialized && std::numeric_limits<std::decay_t<U>>::is_specialized)
        struct sane_common_type<T, U> {
        private:
            using TD = std::decay_t<T>; using UD = std::decay_t<U>;
            using TInfo = std::numeric_limits<TD>; using UInfo = std::numeric_limits<UD>;
            static constexpr bool TUS = !TInfo::is_signed, UUS = !UInfo::is_signed;
            static constexpr int TW = width_of_v<TD>, UW = width_of_v<UD>;
        public:
            using type = std::conditional_t<(TW > UW), TD,
                std::conditional_t<TW < UW, UD,
                std::conditional_t<TUS || UUS, std::conditional_t<TUS, TD, UD>,
                TD
            >>>;
        };
    }
    /// @group{sane_common_type}
    /// @brief Similar to usual arithmetic conversion, except that the promotion step is replaced by `utils::sane_promotion`.
    ///
    /// Given any types `T` and `U`, let `TD` and `UD` be `std::decay_t<T>` and `std::decay_t<U>` respectively.
    /// Their `sane_common_type` `C` is determined through the following steps:
    /// 1. If any of `TD` and `UD` is not specialized for `std::numeric_limits`, `C` is `std::common_type_t<T, U>`.
    /// 2. Otherwise, if `TD` and `UD` have different widths, `C` is the one with the greater width.
    /// 3. Otherwise, if at least one of `TD` and `UD` is unsigned, `C` is the first unsigned type out of the two.
    /// 4. Otherwise, `C` is `TD`.
    ///
    /// In the above, the definition of `width` is identical to that in `utils::width_of`.
    /// @{
    template <typename T, typename U>
    struct sane_common_type : detail::sane_common_type<T, U> {};
    template <typename T, typename U>
    using sane_common_type_t = sane_common_type<T, U>::type;
    /// @}

    /// @group{is_same_sign}
    /// @brief Determines if `T` and `U` have the same sign.
    /// Calculation is based on `TInfo` and `UInfo`, which default to respective instantiations of
    /// `std::numeric_limits`. They can be replaced by class types that provide the same interface as `std::numeric_limits`.
    /// @{
    template <typename T, typename U, typename TInfo = std::numeric_limits<T>, typename UInfo = std::numeric_limits<U>>
    struct is_same_sign : std::bool_constant<TInfo::is_signed == UInfo::is_signed> {};
    template <typename T, typename U>
    constexpr bool is_same_sign_v = is_same_sign<T, U>::value;
    template <typename T, typename U>
    concept same_sign_as = is_same_sign<T, U>::value;
    /// @}

    /// @group{epsilon_of}
    /// @brief Extends the definition of `epsilon` in `std::numeric_limits` to integral types.
    ///
    /// The epsilon of a floating-point type is given by `Info::epsilon()`, while that of an integral type is `T(1)`.
    /// The classification of `T` is based on `Info`, which can be replaced by a class type that provides the same interface
    /// as `std::numeric_limits`.
    /// @{
    template <typename T, typename Info = std::numeric_limits<T>>
    struct epsilon_of : std::integral_constant<T, Info::is_integer ? T(1) : Info::epsilon()> {};
    template <typename T>
    constexpr T epsilon_of_v = epsilon_of<T>::value;
    /// @}

    /// @group{is_lossless_convertible}
    /// @brief Determines if `From` can be converted to `To` without loss of information.
    ///
    /// Calculation is based on `FromInfo` and `ToInfo`, which default to respective instantiations of
    /// `std::numeric_limits`. They can be replaced by class types that provide the same interface as `std::numeric_limits`.
    /// @remark This does not check if `From` is actually convertible to `To`.
    /// @{
    template <
        typename From, typename To,
        typename FromInfo = std::numeric_limits<std::remove_cvref_t<From>>,
        typename ToInfo = std::numeric_limits<std::remove_cvref_t<To>>>
    struct is_lossless_convertible : std::bool_constant<requires {
        requires FromInfo::is_specialized && ToInfo::is_specialized;
        requires !FromInfo::is_signed || ToInfo::is_signed;
        requires +FromInfo::max() <= +ToInfo::max();
        requires +FromInfo::lowest() >= +ToInfo::lowest();
        requires +epsilon_of<std::remove_cvref_t<From>, FromInfo>::value >= +epsilon_of<std::remove_cvref_t<To>, ToInfo>::value;
    }> {};
    template <typename From, typename To>
    constexpr bool is_lossless_convertible_v = is_lossless_convertible<From, To>::value;
    template <typename From, typename To>
    concept lossless_convertible_to = is_lossless_convertible<From, To>::value;
    /// @}

    namespace detail {
        template <template<typename, typename> typename Trait, typename Self, typename A, typename B,
            typename SelfD = std::remove_cvref_t<Self>,
            typename AU = decltype(SelfD::to_underlying(std::declval<A>())),
            typename BU = decltype(SelfD::to_underlying(std::declval<B>()))>
        using integer_common_t = Trait<AU, BU>::type;
        template <auto F, template<typename...> typename Trait>
        struct integer_transform :
            common_cast_transform<F, meta::composite<Trait, std::add_const, std::add_lvalue_reference>::template trait> {};
        template <auto F>
        struct integer_asg_wrap {
            template <typename A, typename B>
            static constexpr A& operator()(A& a, B&& b) {
                return a = F(a, static_cast<const A&>(std::forward<B>(b)));
            }
        };
    }
    /// @namespace utils::integral_behavior
    /// @brief Common strategies for integral arithmetics.
    namespace integral_behavior {
        /// @brief An [<i>IntegerBehaviorProfileTemplate</i>](IntegerBehaviorProfile) for integral arithmetics.
        template <integral_op_functors Funcs, integral_asg_op_functors AsgFuncs>
        struct profile {
            static constexpr auto op_functors = Funcs;
            static constexpr auto asg_op_functors = AsgFuncs;
        };
        /// @brief Default trait set for binary operators for `utils::integral_behavior::profile_from`.
        /// @tparam CommonTypeTrait, ResultTrait: [<i>TypeTraits</i>](Trait).
        template <template<typename, typename> typename CommonTypeTrait, template<typename, typename> typename ResultTrait>
        struct default_binary_op_traits {
            /// `a @ b` passes the constraint if and only if `A`, `B` both satisfy `utils::lossless_convertible_to<C>`,
            /// where `A` and `B` are the types of expressions `a` and `b` respectively,
            /// and `C` is `CommonTypeTrait` applied to the underlying types of `A` and `B`, in that order.
            template <typename Self, typename A, typename B, typename C = detail::integer_common_t<CommonTypeTrait, Self, A, B>>
            struct constraint : std::conjunction<is_lossless_convertible<A, C>, is_lossless_convertible<B, C>> {};
            /// The result type is `Self` rebound to `R`,
            /// which is `ResultTrait` applied to the underlying types of `A` and `B`, in that order.
            template <typename Self, typename A, typename B, typename R = detail::integer_common_t<ResultTrait, Self, A, B>>
            struct result { using type = std::remove_cvref_t<Self>::template rebind<R>; };
        };
        /// @brief Default trait set for assignment operators for `utils::integral_behavior::profile_from`.
        struct default_asg_op_traits {
            /// `a @= b` passes the constraint if `B` satisfies `utils::lossless_convertible_to<A>`,
            /// where `A` and `B` are the types of expressions `a` and `b` respectively,
            /// and `a` is a lvalue-reference.
            template <typename Self, typename A, typename B>
            struct constraint : std::conjunction<std::is_lvalue_reference<Self>, is_lossless_convertible<B, A>> {};
        };
        /// @brief Convenience variable template to make an [<i>IntegerBehaviorProfile</i>](IntegerBehaviorProfile)
        /// for integral arithmetics.
        ///
        /// For every relevant operator `@` and the corresponding functor `F` (supplied in template parameters),
        /// the synthesized functor `op` behaves as follows:
        /// 1. If `@` is a binary operator, `op(a, b)` is equivalent to `F(static_cast<const C&>(a), static_cast<const C&>(b))`;
        /// where `a` and `b` are expressions of types `A` and `B` respectively,
        /// and `C` is `CommonTypeTrait<A, B>::type`.
        /// 2. If `@` is an assignment operator, `op(a, b)` is equivalent to `a = F(a, static_cast<const A&>(b))`,
        /// where `a` and `b` are expressions of types `A` and `B` respectively.
        /// @tparam Plus, Minus, Mul, Div, Mod:
        /// Base functors from which functors supplied to `utils::integral_behavior::profile` are synthesized.
        /// They must return an integer-like type on every well-formed invocation.
        /// @tparam BinaryTraits: Trait set supplied to `utils::integral_op_functors`.
        /// @tparam AsgTraits: Trait set supplied to `utils::integral_asg_op_functors`.
        template <
            auto Plus, auto Minus, auto Mul, auto Div, auto Mod,
            template<typename, typename> typename CommonTypeTrait = sane_common_type,
            template<typename, typename> typename ResultTrait = CommonTypeTrait,
            typename BinaryTraits = default_binary_op_traits<CommonTypeTrait, ResultTrait>,
            typename AsgTraits = default_asg_op_traits>
        constexpr profile<{
            .plus = detail::integer_transform<Plus, CommonTypeTrait>{},
            .minus = detail::integer_transform<Minus, CommonTypeTrait>{},
            .multiplies = detail::integer_transform<Mul, CommonTypeTrait>{},
            .divides = detail::integer_transform<Div, CommonTypeTrait>{},
            .modulus = detail::integer_transform<Mod, CommonTypeTrait>{},
            .binary_traits = BinaryTraits{}
        }, {
            .plus_asg = detail::integer_asg_wrap<Plus>{},
            .minus_asg = detail::integer_asg_wrap<Minus>{},
            .multiplies_asg = detail::integer_asg_wrap<Mul>{},
            .divides_asg = detail::integer_asg_wrap<Div>{},
            .modulus_asg = detail::integer_asg_wrap<Mod>{},
            .asg_traits = AsgTraits{}
        }> profile_from;

        /// @brief Default C++ integral arithmetic rules.
        inline constexpr auto standard = profile_from<
            std::plus{}, std::minus{}, std::multiplies{}, std::divides{}, std::modulus{}, std::common_type>;

        /// @brief Default C++ integral arithmetic rules, but the result type is determined by `utils::sane_common_type`.
        ///
        /// This prevents `(unsigned short)-1 * 2` from being undefined on a machine where `sizeof(int) == sizeof(short)`.
        /// It instead wraps around as expected.
        inline constexpr auto sane = profile_from<
            std::plus{}, std::minus{}, std::multiplies{}, std::divides{}, std::modulus{}>;

        namespace detail {
            struct ub_plus {
                template <typename T>
                static constexpr decltype(auto) operator()(const T& a, const T& b) noexcept {
                    [[assume(std::numeric_limits<T>::is_signed || a <= std::numeric_limits<T>::max() - b)]];
                    return a + b;
                }
            };
            struct ub_minus {
                template <typename T>
                static constexpr decltype(auto) operator()(const T& a, const T& b) noexcept {
                    [[assume(std::numeric_limits<T>::is_signed || a >= b)]];
                    return a - b;
                }
            };
            struct ub_multiplies {
                template <typename T>
                static constexpr decltype(auto) operator()(const T& a, const T& b) noexcept {
                    [[assume(std::numeric_limits<T>::is_signed || b == 0 || a <= std::numeric_limits<T>::max() / b)]];
                    return a * b;
                }
            };
        }
        /// @brief Invokes undefined behavior for any overflow (even for unsigned types), in addition to standard C++ arithmetic rules.
        inline constexpr auto ub = profile_from<
            detail::ub_plus{}, detail::ub_minus{}, detail::ub_multiplies{}, std::divides{}, std::modulus{}>;

        namespace detail {
            struct wrap_divides {
                template <typename T>
                static constexpr auto operator()(const T& a, const T& b) -> decltype(a / b) {
                    if (std::numeric_limits<T>::is_signed && a == std::numeric_limits<T>::min() && b == -1) return a;
                    return a / b;
                }
            };
        }
        /// @brief Wraps around for any overflow, even for signed types and division by -1.
        inline constexpr auto wrap = profile_from<
            std::plus{}, std::minus{}, std::multiplies{}, detail::wrap_divides{}, std::modulus{},
            meta::composite<sane_common_type, std::make_unsigned>::trait, sane_common_type,
            default_binary_op_traits<sane_common_type, sane_common_type>>;

#ifdef __cpp_lib_saturation_arithmetic
#if __cpp_lib_saturation_arithmetic >= 202603L
#define UTILS_INTEGER_SAT_FUNC(op) saturating_##op
#elif __cpp_lib_saturation_arithmetic >= 202311L
#define UTILS_INTEGER_SAT_FUNC(op) op##_sat
#endif
#define UTILS_INTEGER_SAT_CLASS(op_name, func_name)\
    struct sat_##op_name {\
        template <typename T>\
        static constexpr T operator()(const T& a, const T& b) noexcept {\
            using std:: UTILS_INTEGER_SAT_FUNC(func_name);\
            return UTILS_INTEGER_SAT_FUNC(func_name)(a, b);\
        }\
    };
        namespace detail {
            UTILS_INTEGER_SAT_CLASS(plus, add)
            UTILS_INTEGER_SAT_CLASS(minus, sub)
            UTILS_INTEGER_SAT_CLASS(multiplies, mul)
            UTILS_INTEGER_SAT_CLASS(divides, div)
            struct sat_modulus {
                template <typename T>
                static constexpr auto operator()(const T& a, const T& b) noexcept -> decltype(a % b) {
                    if (std::numeric_limits<T>::is_signed && a == std::numeric_limits<T>::min() && b == -1) return 0;
                    return a % b;
                }
            };
        }
        /// @brief Saturation arithmetic.
        ///
        /// In particular, `signed_min % -1` is defined to be 0.
        inline constexpr auto saturation = profile_from<
            detail::sat_plus{}, detail::sat_minus{}, detail::sat_multiplies{}, detail::sat_divides{}, detail::sat_modulus{}>;
#undef UTILS_INTEGER_SAT_CLASS
#undef UTILS_INTEGER_SAT_FUNC
#endif

#define UTILS_INTEGER_CKD_CLASS(op_name, func_name)\
    struct ckd_##op_name {\
        template <typename T>\
        static constexpr T operator()(const T& a, const T& b) {\
            T out;\
            if (ckd_##func_name(&out, a, b)) throw std::overflow_error("integer overflow");\
            return out;\
        }\
    };
        namespace detail {
            UTILS_INTEGER_CKD_CLASS(plus, add)
            UTILS_INTEGER_CKD_CLASS(minus, sub)
            UTILS_INTEGER_CKD_CLASS(multiplies, mul)
            struct ckd_divides {
                template <typename T>
                static void check(const T& a, const T& b) {
                    if (std::is_signed_v<T> && a == std::numeric_limits<T>::min() && b == -1) {
                        throw std::overflow_error("integer overflow");
                    }
                    if (b == 0) throw std::domain_error("integer division by zero");
                }
                template <typename T>
                static constexpr decltype(auto) operator()(const T& a, const T& b) {
                    check<T>(a, b);
                    return a / b;
                }
            };
            struct ckd_modulus {
                template <typename T>
                static constexpr decltype(auto) operator()(const T& a, const T& b) {
                    ckd_divides::check<T>(a, b);
                    return a % b;
                }
            };
        }
        /// @brief Throws an exception for any overflow (including division) and division by 0.
        /// @remark Every arithmetic operation is defined under this trait.
        inline constexpr auto checked = profile_from<
            detail::ckd_plus{}, detail::ckd_minus{}, detail::ckd_multiplies{}, detail::ckd_divides{}, detail::ckd_modulus{}>;
#undef UTILS_INTEGER_CKD_CLASS
    }

    /// @namespace utils::bit_behavior
    /// @brief Common strategies for bit operations.
    namespace bit_behavior {
        /// @brief An [<i>IntegerBehaviorProfileTemplate</i>](IntegerBehaviorProfile) for bit operations.
        template <bit_op_functors Funcs, bit_asg_op_functors AsgFuncs>
        struct profile {
            static constexpr auto op_functors = Funcs;
            static constexpr auto asg_op_functors = AsgFuncs;
        };
        /// @brief Default trait set for binary operations for `utils::bit_behavior::profile_from`.
        /// /// @tparam ResultTrait: [<i>TypeTraits</i>](Trait).
        template <template<typename, typename> typename ResultTrait>
        struct default_binary_op_traits {
            /// `a @ b` passes the constraint if and only if `a` and `b` have the same width.
            template <typename, typename A, typename B>
            struct constraint : std::bool_constant<width_of_v<std::remove_cvref_t<A>> == width_of_v<std::remove_cvref_t<B>>> {};
            /// The result type is `Self` rebound to the type obtained by applying `ResultTrait`
            /// to the underlying types of `A` and `B`, in that order.
            template <typename Self, typename A, typename B, typename R = detail::integer_common_t<ResultTrait, Self, A, B>>
            struct result { using type = std::remove_cvref_t<Self>::template rebind<R>; };
        };
        /// @brief Default trait set for assignment operations for `utils::bit_behavior::profile_from`.
        struct default_asg_op_traits {
            /// `a @= b` passes constraint if and only if `a` is an lvalue reference and `a` and `b` have the same width.
            template <typename Self, typename A, typename B>
            struct constraint : std::conjunction<
                std::is_lvalue_reference<Self>,
                std::bool_constant<width_of_v<std::remove_cvref_t<A>> == width_of_v<std::remove_cvref_t<B>>>> {};
        };
        /// @brief Convenience variable template to make an [<i>IntegerBehaviorProfile</i>](IntegerBehaviorProfile)
        /// for bit operations.
        ///
        /// For every relevant operator `@` and the corresponding functor `F` (supplied in template parameters),
        /// the synthesized functor `op` behaves as follows:
        /// 1. If `@` is a binary operator, `op(a, b)` is equivalent to `F(static_cast<const C&>(a), static_cast<const C&>(b))`;
        /// where `a` and `b` are expressions of types `A` and `B` respectively,
        /// and `C` is `CommonTypeTrait<A, B>::type`.
        /// 2. If `@` is an assignment operator, `op(a, b)` is equivalent to `a = F(a, static_cast<const A&>(b))`,
        /// where `a` and `b` are expressions of types `A` and `B` respectively.
        /// @tparam BitAnd, BitOr, BitXor, BitNot:
        /// Base functors from which functors supplied to `utils::bit_behavior::profile` are synthesized.
        /// They must return an integer-like type on every well-formed invocation.
        /// @tparam BinaryTraits: Trait set supplied to `utils::bit_op_functors`.
        /// @tparam AsgTraits: Trait set supplied to `utils::bit_asg_op_functors`.
        template <
            auto BitAnd, auto BitOr, auto BitXor, auto BitNot,
            template<typename, typename> typename CommonTypeTrait = sane_common_type,
            template<typename, typename> typename ResultTrait = CommonTypeTrait,
            typename BinaryTraits = default_binary_op_traits<ResultTrait>,
            typename AsgTraits = default_asg_op_traits>
        constexpr profile<{
            .bit_and = utils::detail::integer_transform<BitAnd, CommonTypeTrait>{},
            .bit_or = utils::detail::integer_transform<BitOr, CommonTypeTrait>{},
            .bit_xor = utils::detail::integer_transform<BitXor, CommonTypeTrait>{},
            .bit_not = BitNot,
            .binary_traits = BinaryTraits{}
        }, {
            .bit_and_asg = utils::detail::integer_asg_wrap<BitAnd>{},
            .bit_or_asg = utils::detail::integer_asg_wrap<BitOr>{},
            .bit_xor_asg = utils::detail::integer_asg_wrap<BitXor>{},
            .asg_traits = AsgTraits{}
        }> profile_from;

        /// @brief Default C++ bit operation rules.
        inline constexpr auto standard = profile_from<
            std::bit_and{}, std::bit_or{}, std::bit_xor{}, std::bit_not{}, std::common_type>;

        /// @brief Default C++ bit operation rules, but the result type is determined by `utils::sane_common_type`.
        inline constexpr auto sane = profile_from<
            std::bit_and{}, std::bit_or{}, std::bit_xor{}, std::bit_not{}, sane_common_type>;
    }

    /// @namespace utils::shift_behavior
    /// @brief Common strategies for bit shifts.
    namespace shift_behavior {
        /// @brief An [<i>IntegerBehaviorProfileTemplate</i>](IntegerBehaviorProfile) for bit shifts.
        template <shift_op_functors Funcs, shift_asg_op_functors AsgFuncs>
        struct profile {
            static constexpr auto op_functors = Funcs;
            static constexpr auto asg_op_functors = AsgFuncs;
        };
        /// @brief Default trait set for binary operations for `utils::shift_behavior::profile_from`.
        struct default_binary_op_traits : utils::default_binary_op_traits {
            /// The result type is `Self` rebound to the underlying type of `A`.
            template <typename Self, typename A, typename,
                typename SelfD = std::remove_cvref_t<Self>,
                typename AU = decltype(SelfD::to_underlying(std::declval<A>()))>
            struct result { using type = std::remove_cvref_t<Self>::template rebind<std::remove_cvref_t<AU>>; };
        };
        /// @brief Convenience variable template to make an [<i>IntegerBehaviorProfile</i>](IntegerBehaviorProfile)
        /// for bit shifts.
        ///
        /// For every relevant operator `@` and the corresponding functor `F` (supplied in template parameters),
        /// the synthesized functor `op` behaves as follows:
        /// 1. If `@` is a binary operator, `op(a, b)` is equivalent to `F(a, b)`.
        /// In other words, `op` is identical to `F` in this case.
        /// 2. If `@` is an assignment operator, `op(a, b)` is equivalent to `a = F(a, b)`.
        /// @tparam ShiftLeft, ShiftRight:
        /// Base functors from which functors supplied to `utils::shift_behavior::profile` are synthesized.
        /// They must return an integer-like type on every well-formed invocation.
        /// @tparam BinaryTraits: Trait set supplied to `utils::shift_op_functors`.
        /// @tparam AsgTraits: Trait set supplied to `utils::shift_asg_op_functors`.
        template <auto ShiftLeft, auto ShiftRight,
            typename BinaryTraits = default_binary_op_traits,
            typename AsgTraits = default_asg_op_traits>
        constexpr profile<{
            .shift_left = ShiftLeft,
            .shift_right = ShiftRight,
            .binary_traits = BinaryTraits{}
        }, {
            .shift_left_asg = asg_wrap<ShiftLeft>{},
            .shift_right_asg = asg_wrap<ShiftRight>{},
            .asg_traits = AsgTraits{}
        }> profile_from;

        /// @brief Default C++ bit-shift operation rules.
        inline constexpr auto standard = profile_from<shift_left{}, shift_right{}>;

        namespace detail {
            struct scalar_shift_left {
                template <typename T, typename S>
                static constexpr auto operator()(T&& t, S&& s) noexcept
                -> decltype(std::declval<T&&>() << std::declval<S&&>());
            };
            struct scalar_shift_right {
                template <typename T, typename S>
                static constexpr auto operator()(T&& t, S&& s) noexcept
                -> decltype(std::declval<T&&>() >> std::declval<S&&>());
            };
            template <typename T, typename S>
            constexpr auto scalar_shift_left::operator()(T&& t, S&& s) noexcept
            -> decltype(std::declval<T&&>() << std::declval<S&&>()) {
                if (s < 0) return scalar_shift_right{}(std::forward<T>(t), -std::forward<S>(s));
                if (s >= width_of_v<std::remove_cvref_t<T>>) return 0;
                return std::forward<T>(t) << std::forward<S>(s);
            }
            template <typename T, typename S>
            constexpr auto scalar_shift_right::operator()(T&& t, S&& s) noexcept
            -> decltype(std::declval<T&&>() >> std::declval<S&&>()) {
                using TD = std::remove_cvref_t<T>;
                if (s < 0) return scalar_shift_left{}(std::forward<T>(t), -std::forward<S>(s));
                if (s >= width_of_v<TD>) {
                    if constexpr (std::numeric_limits<TD>::is_signed) return -1;
                    else return 0;
                }
                return std::forward<T>(t) >> std::forward<S>(s);
            }
        }
        /// @brief Treats bit-shifting as a scalar operation.
        ///
        /// Shifting by a negative `n` means shifting in the other direction,
        /// and we assume `a` has an infinite range before truncating it to the original type.
        /// @remark Every shift operation is defined under this trait.
        inline constexpr auto scalar = profile_from<detail::scalar_shift_left{}, detail::scalar_shift_right{}>;

#define UTILS_INTEGER_CIRC_CLASS(op_name, func_name)\
    struct circ_##op_name {\
        template <typename T, typename S>\
        static constexpr decltype(auto) operator()(T&& t, S&& s) noexcept {\
            using SInfo = std::numeric_limits<S>;\
            using std:: func_name;\
            if constexpr (!std::has_single_bit<unsigned>(width_of_v<T>) && (SInfo::min() < INT_MIN || SInfo::max() > INT_MAX)) {\
                return func_name(std::forward<T>(t), std::forward<S>(s) % width_of_v<T>);\
            } else {\
                return func_name(std::forward<T>(t), std::forward<S>(s));\
            }\
        }\
    };
        namespace detail {
            UTILS_INTEGER_CIRC_CLASS(shift_left, rotl)
            UTILS_INTEGER_CIRC_CLASS(shift_right, rotr)
        }
        /// @brief Circular bit-shifting.
        /// @remark Every shift operation is defined under this trait.
        inline constexpr auto circular = profile_from<detail::circ_shift_left{}, detail::circ_shift_right{}>;
#undef UTILS_INTEGER_CIRC_CLASS

        namespace detail {
            template <auto F>
            struct ckd_base {
                template <typename T, typename N>
                static constexpr decltype(auto) operator()(T&& t, N&& n) {
                    if (n < 0 || n >= width_of_v<std::remove_cvref_t<T>>) throw std::domain_error("shift out of range");
                    return F(std::forward<T>(t), std::forward<N>(n));
                }
            };
        }
        /// @brief Throws an exception for invalid `n`.
        /// @remark Every shift operation is defined under this trait.
        inline constexpr auto checked = profile_from<detail::ckd_base<shift_left{}>{}, detail::ckd_base<shift_right{}>{}>;
    }

    namespace detail {
        template <auto IB>
        inline constexpr fix_op_functors integer_fix_ops = {
            .pre_increment = [](auto& self) {
                return IB.asg_op_functors.plus_asg(self, 1);
            },
            .post_increment = [](auto& self) {
                const auto old = self;
                IB.asg_op_functors.plus_asg(self, 1);
                return old;
            },
            .pre_decrement = [](auto& self) {
                return IB.asg_op_functors.minus_asg(self, 1);
            },
            .post_decrement = [](auto& self) {
                const auto old = self;
                IB.asg_op_functors.minus_asg(self, 1);
                return old;
            }
        };
    }
    /// @brief An integer wrapper with added type safety and customizable behavior in arithmetic operations.
    ///
    /// This class template is meant as a near drop-in replacement of built-in integers in the language.
    /// It provides better type safety: an explicit conversion call is needed when there is potential value change;
    /// and fully customizable behavior in arithmetic operations,
    /// which is controlled by the behavior profiles in template parameters.
    /// One may, for example, define overflow and underflow for signed integers to provide better safety guarantees.
    ///
    /// The template is also flexible, in that the underlying type `T` can be any <i>integer-like</i> type.
    /// In other words, it is possible to use the class template on a non-standard big integer type as long as
    /// it meets all requirements specified in the standard for <i>integer-like</i> types.
    /// Specialize `utils::is_integer_like` to declare as a type as <i>integer-like</i>.
    ///
    /// @tparam IB: [<i>IntegerBehaviorProfile</i>](IntegerBehaviorProfile) for integral operations.
    /// @tparam BB: [<i>IntegerBehaviorProfile</i>](IntegerBehaviorProfile) for bit operations.
    /// @tparam SB: [<i>IntegerBehaviorProfile</i>](IntegerBehaviorProfile) for bit shifts.
    template <
        integer_like T,
        integral_behavior::profile IB = integral_behavior::sane,
        bit_behavior::profile BB = bit_behavior::sane,
        shift_behavior::profile SB = shift_behavior::standard>
    struct integer : arithmetic_ops<
        IB.op_functors, IB.asg_op_functors,
        BB.op_functors, BB.asg_op_functors,
        SB.op_functors, SB.asg_op_functors,
        {},
        detail::integer_fix_ops<IB>
    > {
        using tag = struct integer_tag;
        using underlying_type = T;
        static constexpr integral_behavior::profile integral_behavior = IB;
        static constexpr bit_behavior::profile bit_behavior = BB;
        static constexpr shift_behavior::profile shift_behavior = SB;
        template <integer_like U>
        using rebind = integer<U, IB, BB, SB>;
        template <integral_behavior::profile ToIB>
        using rebind_integral_behavior = integer<T, ToIB, BB, SB>;
        template <bit_behavior::profile ToBB>
        using rebind_bit_behavior = integer<T, IB, ToBB, SB>;
        template <shift_behavior::profile ToSB>
        using rebind_shift_behavior = integer<T, IB, BB, ToSB>;

        /// @brief The underlying integer.
        ///
        /// The behavior is undefined if it is used in user code.
        /// @remark The member is only made public to preserve structural type.
        T under_;

        /// @defgroup integerto_underlying utils::integer::to_underlying
        /// @brief Get a reference to the underlying object of `x`.
        ///
        /// The static member function only has overloads for
        /// 1. `utils::integer_like` types, and
        /// 2. `utils::integer` instances with identical behavior profiles.
        /// @{
        template <typename U>
        requires (is_integer_like_v<std::remove_cvref_t<U>>)
        static constexpr U&& to_underlying(U&& x) noexcept { return std::forward<U>(x); }
        template <tagged<integer_tag> U>
        requires (std::is_same_v<typename std::remove_cvref_t<U>::template rebind<T>, integer>)
        static constexpr decltype(auto) to_underlying(U&& x) noexcept { return std::forward_like<U>(x.under_); }
        /// @}

        /// @defgroup integerinteger utils::integer::integer
        /// @{
        /// @brief Default-initializes `under_`.
        constexpr integer() = default;
        /// @brief Conversion from a (possibly wrapped) integer-like value `other`.
        ///
        /// If `T` is unsigned and `other < 0`, it wraps around.
        /// Explicit if `utils::lossless_convertible_to<U, T>` is false.
        template <typename Other, typename U = decltype(to_underlying(std::declval<Other&&>()))>
        explicit(!lossless_convertible_to<U, T>)
        constexpr integer(Other&& other) noexcept(std::is_nothrow_constructible_v<T, U>) :
            under_(to_underlying(std::forward<Other>(other))) {}
        /// @brief Conversion from a floating point value `other`.
        /// The constructor is explicit as there is potential precision loss.
        template <typename Other>
        requires (std::is_floating_point_v<std::remove_cvref_t<Other>>)
        explicit constexpr integer(Other&& other) noexcept(std::is_nothrow_constructible_v<T, Other&&>) :
            under_(std::forward<Other>(other)) {}
        /// @brief This allows implicit conversions from numbers known at compile time.
        /// @throw std::overflow_error (at compile time) if `other` cannot be represented by `T`,
        /// after wrapping around `other` if `T` is unsigned.
        /// @remark `Ts` exists solely to lower the priority of this constructor in overload resolution.
        template <typename Other, typename... Ts, typename OtherD = std::remove_cvref_t<Other>>
        requires ((std::is_arithmetic_v<OtherD> || is_integer_like_v<OtherD>) && sizeof...(Ts) == 0)
        consteval integer(Other&& other, Ts...) : under_(std::forward<Other>(other)) {
            using TInfo = std::numeric_limits<T>;
            const auto& x = (!TInfo::is_signed && under_ < 0) ? (TInfo::max() + under_) : under_;
            if (x > TInfo::max()) throw std::overflow_error("integer overflow");
            if (x < TInfo::min()) throw std::underflow_error("integer underflow");
        }
        /// @}

        /// @defgroup integeroperatorcomparison utils::integer::operator==, utils::integer::operator<=>
        /// `utils::integer` can be compared with an `utils::integer_like` type if and only if
        /// both have the same signedness (either both signed, or both unsigned).
        /// @{
        template <typename Self, typename Other,
            typename SelfD = std::remove_cvref_t<Self>, typename OtherD = std::remove_cvref_t<Other>>
        requires (is_same_sign_v<SelfD, OtherD>)
        constexpr decltype(auto) operator==(this Self&& self, Other&& other)
        noexcept(noexcept(std::forward_like<Self>(self.under_) == to_underlying(std::forward<Other>(other)))) {
            return std::forward_like<Self>(self.under_) == to_underlying(std::forward<Other>(other));
        }
        template <typename Self, typename Other,
            typename SelfD = std::remove_cvref_t<Self>, typename OtherD = std::remove_cvref_t<Other>>
        requires (is_same_sign_v<SelfD, OtherD>)
        constexpr decltype(auto) operator<=>(this Self&& self, Other&& other)
        noexcept(noexcept(std::forward_like<Self>(self.under_) <=> to_underlying(std::forward<Other>(other)))) {
            return std::forward_like<Self>(self.under_) <=> to_underlying(std::forward<Other>(other));
        }
        /// @}

        /// @defgroup integeroperatorconversion utils::integer::operator U&, utils::integer::operator const U&, utils::integer::operator U&&, utils::integer::operator const U&&
        /// @brief Conversion to references to underlying type.
        /// @{
        template <std::same_as<T> U>
        constexpr operator U&() & noexcept { return under_; }
        template <std::same_as<T> U>
        constexpr operator const U&() const& noexcept { return under_; }
        template <std::same_as<T> U>
        constexpr operator U&&() && noexcept { return std::move(under_); }
        template <std::same_as<T> U>
        constexpr operator const U&&() const&& noexcept { return std::move(under_); }
        /// @}
        /// @brief Conversion to integer-like type `U`.
        ///
        /// Explicit if `utils::lossless_convertible_to<T, U>` is false.
        template <typename U>
        requires (!std::is_same_v<U, T> && is_explicitly_convertible_v<T, U>)
        explicit(!lossless_convertible_to<T, U>)
        constexpr operator U() const noexcept { return static_cast<U>(under_); }

        /// @brief Returns the underlying object.
        constexpr T operator+() const noexcept { return under_; }

        /// @brief `operator<<` overload for output streams.
        template <typename CharT, typename Traits>
        friend decltype(auto) operator<<(std::basic_ostream<CharT, Traits>& is, const integer& value)
        requires requires { is << value.under_; } {
            return is << value.under_;
        }
    };

    namespace integer_alias {
        using schar = integer<signed char>;
        using uchar = integer<unsigned char>;
        using sshort = integer<signed short>;
        using ushort = integer<unsigned short>;
        using sint = integer<signed int>;
        using uint = integer<unsigned int>;
        using slong = integer<signed long>;
        using ulong = integer<unsigned long>;
        using sllong = integer<signed long long>;
        using ullong = integer<unsigned long long>;
#ifdef INT8_MAX
        using s8 = integer<int8_t>;
#endif
#ifdef UINT8_MAX
        using u8 = integer<uint8_t>;
#endif
#ifdef INT16_MAX
        using s16 = integer<int16_t>;
#endif
#ifdef UINT16_MAX
        using u16 = integer<uint16_t>;
#endif
#ifdef INT32_MAX
        using s32 = integer<int32_t>;
#endif
#ifdef UINT32_MAX
        using u32 = integer<uint32_t>;
#endif
#ifdef INT64_MAX
        using s64 = integer<int64_t>;
#endif
#ifdef UINT64_MAX
        using u64 = integer<uint64_t>;
#endif
        using sleast8 = integer<std::int_least8_t>;
        using uleast8 = integer<std::uint_least8_t>;
        using sleast16 = integer<std::int_least16_t>;
        using uleast16 = integer<std::uint_least16_t>;
        using sleast32 = integer<std::int_least32_t>;
        using uleast32 = integer<std::uint_least32_t>;
        using sleast64 = integer<std::int_least64_t>;
        using uleast64 = integer<std::uint_least64_t>;
        using sfast8 = integer<std::int_fast8_t>;
        using ufast8 = integer<std::uint_fast8_t>;
        using sfast16 = integer<std::int_fast16_t>;
        using ufast16 = integer<std::uint_fast16_t>;
        using sfast32 = integer<std::int_fast32_t>;
        using ufast32 = integer<std::uint_fast32_t>;
        using sfast64 = integer<std::int_fast64_t>;
        using ufast64 = integer<std::uint_fast64_t>;
        using smax = integer<std::intmax_t>;
        using umax = integer<std::uintmax_t>;
        using size_t = integer<std::size_t>;
        using ptrdiff_t = integer<std::ptrdiff_t>;
#ifdef INTPTR_MAX
        using intptr_t = integer<std::intptr_t>;
#endif
#ifdef UINTPTR_MAX
        using uintptr_t = integer<std::uintptr_t>;
#endif
    }
}

template <utils::tagged<utils::integer_tag> T>
struct std::numeric_limits<T> : std::numeric_limits<typename T::underlying_type> {
private:
    using base_type = std::numeric_limits<typename T::underlying_type>;
public:
    static constexpr T min() noexcept { return base_type::min(); }
    static constexpr T max() noexcept { return base_type::max(); }
    static constexpr T lowest() noexcept { return base_type::lowest(); }
    static constexpr T epsilon() noexcept { return base_type::epsilon(); }
    static constexpr T round_error() noexcept { return base_type::round_error(); }
    static constexpr T infinity() noexcept { return base_type::infinity(); }
    static constexpr T quiet_NaN() noexcept { return base_type::quiet_NaN(); }
    static constexpr T signaling_NaN() noexcept { return base_type::signaling_NaN(); }
    static constexpr T denorm_min() noexcept { return base_type::denorm_min(); }
};

template <utils::tagged<utils::integer_tag> T, typename CharT>
struct std::formatter<T, CharT> : std::formatter<typename T::underlying_type, CharT> {
private:
    using base_type = std::formatter<typename T::underlying_type, CharT>;
public:
    constexpr auto parse(auto& parse_ctx) {
        return base_type::parse(parse_ctx);
    }
    constexpr auto format(const T& arg, auto& fmt_ctx) const {
        return base_type::format(T::to_underlying(arg), fmt_ctx);
    }
};