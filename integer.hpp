#pragma once
#include <concepts>
#include <type_traits>
#include <limits>
#include <utility>
#include <numeric>
#include <stdexcept>
#include <stdckdint.h>
#include <bit>
#include <cstdint>
#include <cstddef>
#include <istream>
#include <ostream>
#include <format>
#include "operators.hpp"
#include "type.hpp"

namespace utils {
    /// Same rules as integer promotion in the standard, except that the promoted type always preserves sign.
    template <typename T>
    struct sane_promotion {
    private:
        using P = decltype(+std::declval<T>());
    public:
        using type = std::conditional_t<std::is_signed_v<T>, P, std::make_unsigned_t<P>>;
        static constexpr bool conv = !std::is_same_v<type, T>;
    };
    template <typename T>
    using sane_promotion_t = typename sane_promotion<T>::type;
    template <typename T>
    constexpr sane_promotion_t<T> sane_promote(T n) { return n; }

    template <typename T>
    constexpr T epsilon_of = std::numeric_limits<T>::is_integer ? T(1) : std::numeric_limits<T>::epsilon();
    template <typename From, typename To>
    concept lossless_convertible_to = requires {
        requires std::is_convertible_v<From, To>;
        requires std::numeric_limits<From>::is_specialized && std::numeric_limits<To>::is_specialized;
        requires !std::numeric_limits<From>::is_signed || std::numeric_limits<To>::is_signed;
        requires std::numeric_limits<From>::max() <= std::numeric_limits<To>::max();
        requires std::numeric_limits<From>::lowest() >= std::numeric_limits<To>::lowest();
        requires epsilon_of<From> >= epsilon_of<To>;
    };
    template <typename From, typename To>
    struct is_lossless_convertible : std::bool_constant<lossless_convertible_to<From, To>> {};
    template <typename From, typename To>
    constexpr bool is_lossless_convertible_v = is_lossless_convertible<From, To>::value;

    namespace integral_behavior {
        /// Default C++ integral arithmetic rules.
        template <std::integral>
        struct standard {
            static constexpr integral_op_functors ops{};
            static constexpr integral_asg_op_functors asg_ops{};
        };

        /// Default C++ integral arithmetic rules, but using ```sane_promotion```.
        /// This prevents ```unsigned short(-1) * 2``` from being undefined (it instead wraps around as expected).
        template <std::integral Under>
        struct sane {
        private:
            using U = sane_promotion_t<Under>;
        public:
            static constexpr integral_op_functors ops = {
                .plus = std::plus<U>{},
                .minus = std::minus<U>{},
                .multiplies = std::multiplies<U>{},
                .divides = std::divides<U>{},
                .modulus = std::modulus<U>{}
            };
            static constexpr integral_asg_op_functors asg_ops = {
                .plus_asg = plus_asg<U>{},
                .minus_asg = minus_asg<U>{},
                .multiplies_asg = multiplies_asg<U>{},
                .divides_asg = divides_asg<U>{},
                .modulus_asg = modulus_asg<U>{}
            };
        };

        /// Invokes UB for any overflow (even for unsigned types), in addition to standard C++ arithmetic rules.
        template <std::integral Under>
        struct ub {
        private:
            using SP = sane_promotion<Under>;
            using U = SP::type;
            static constexpr U max = std::numeric_limits<Under>::max(),
            min = std::numeric_limits<Under>::min();
            static constexpr U plus(U lhs, U rhs) noexcept {
                [[assume(rhs < 0 || lhs <= max - rhs)]];
                [[assume(rhs > 0 || lhs >= min - rhs)]];
                return lhs + rhs;
            }
            static constexpr U minus(U lhs, U rhs) noexcept {
                if constexpr (SP::conv) {
                    [[assume(lhs - rhs >= min)]];
                    [[assume(lhs - rhs <= max)]];
                } else if constexpr (std::is_unsigned_v<Under>) {
                    [[assume(lhs >= rhs)]];
                }
                return lhs - rhs;
            }
            static constexpr U multiplies(U lhs, U rhs) noexcept {
                if constexpr (SP::conv) {
                    // if lhs * rhs overflow even as promoted integers, it would overflow in Under anyway
                    [[assume(lhs * rhs >= min)]];
                    [[assume(lhs * rhs <= max)]];
                } else if constexpr (std::is_unsigned_v<Under>) {
                    [[assume(rhs == 0 || lhs <= max / rhs)]];
                }
                return lhs * rhs;
            }
        public:
            static constexpr integral_op_functors ops = {
                .plus = plus,
                .minus = minus,
                .multiplies = multiplies,
                .divides = std::divides<U>{},
                .modulus = std::modulus<U>{}
            };
            static constexpr integral_asg_op_functors asg_ops = {
                .plus_asg = asg_wrap<plus>{},
                .minus_asg = asg_wrap<minus>{},
                .multiplies_asg = asg_wrap<multiplies>{},
                .divides_asg = divides_asg<U>{},
                .modulus_asg = modulus_asg<U>{}
            };
        };

        /// Wraps around for any overflow, even for signed types and division by -1.
        template <std::integral Under>
        struct wrap {
        private:
            using U = sane_promotion_t<Under>;
            using UU = std::make_unsigned_t<U>;
            static constexpr U divides(U lhs, U rhs) noexcept {
                if (std::is_signed_v<Under> && lhs == std::numeric_limits<U>::min() && rhs == -1) return lhs;
                return lhs / rhs;
            }
        public:
            static constexpr integral_op_functors ops = {
                .plus = std::plus<UU>{},
                .minus = std::minus<UU>{},
                .multiplies = std::multiplies<UU>{},
                .divides = divides,
                .modulus = std::modulus<U>{}
            };
            static constexpr integral_asg_op_functors asg_ops = {
                .plus_asg = plus_asg<UU>{},
                .minus_asg = minus_asg<UU>{},
                .multiplies_asg = multiplies_asg<UU>{},
                .divides_asg = asg_wrap<divides>{},
                .modulus_asg = modulus_asg<U>{}
            };
        };

        /// Saturated arithmetic.
        /// In particular, ```signed_min % -1``` is defined to be 0.
        template <std::integral Under>
        struct sat {
        private:
            using U = sane_promotion_t<Under>;
            static constexpr U modulus(U lhs, U rhs) noexcept {
                if (std::is_signed_v<Under> && lhs == std::numeric_limits<U>::min() && rhs == -1) return 0;
                return lhs % rhs;
            }
        public:
            static constexpr integral_op_functors ops = {
                .plus = std::add_sat<Under>,
                .minus = std::sub_sat<Under>,
                .multiplies = std::mul_sat<Under>,
                .divides = std::div_sat<Under>,
                .modulus = modulus
            };
            static constexpr integral_asg_op_functors asg_ops = {
                .plus_asg = asg_wrap<std::add_sat<Under>>{},
                .minus_asg = asg_wrap<std::sub_sat<Under>>{},
                .multiplies_asg = asg_wrap<std::mul_sat<Under>>{},
                .divides_asg = asg_wrap<std::div_sat<Under>>{},
                .modulus_asg = asg_wrap<modulus>{}
            };
        };

        /// Throws an exception for any overflow (including division) and division by 0.
        /// In other words, every arithmetic operation is defined under this trait.
        template <std::integral Under>
        struct checked {
        private:
            static constexpr Under plus(Under lhs, Under rhs) {
                Under res;
                if (ckd_add(&res, lhs, rhs)) throw std::overflow_error("integer overflow");
                return res;
            }
            static constexpr Under minus(Under lhs, Under rhs) {
                Under res;
                if (ckd_sub(&res, lhs, rhs)) throw std::overflow_error("integer underflow");
                return res;
            }
            static constexpr Under multiplies(Under lhs, Under rhs) {
                Under res;
                if (ckd_mul(&res, lhs, rhs)) throw std::overflow_error("integer overflow");
                return res;
            }
            static constexpr Under divides(Under lhs, Under rhs) {
                if (std::is_signed_v<Under> && lhs == std::numeric_limits<Under>::min() && rhs == -1) {
                    throw std::overflow_error("integer overflow");
                }
                if (rhs == 0) throw std::domain_error("integer division by zero");
                return lhs / rhs;
            }
            static constexpr Under modulus(Under lhs, Under rhs) {
                if (std::is_signed_v<Under> && lhs == std::numeric_limits<Under>::min() && rhs == -1) {
                    throw std::overflow_error("integer overflow");
                }
                if (rhs == 0) throw std::domain_error("integer division by zero");
                return lhs % rhs;
            }
        public:
            static constexpr integral_op_functors ops = {
                .plus = plus,
                .minus = minus,
                .multiplies = multiplies,
                .divides = divides,
                .modulus = modulus
            };
            static constexpr integral_asg_op_functors asg_ops = {
                .plus_asg = asg_wrap<plus>{},
                .minus_asg = asg_wrap<minus>{},
                .multiplies_asg = asg_wrap<multiplies>{},
                .divides_asg = asg_wrap<divides>{},
                .modulus_asg = asg_wrap<modulus>{}
            };
        };
    }

    namespace shift_behavior {
        /// Default C++ bit-shift operation rules.
        template <std::integral>
        struct standard {
            static constexpr shift_op_functors ops{};
            static constexpr shift_asg_op_functors asg_ops{};
        };

        /// Treats bit-shifting as a scalar operation,
        /// i.e. shifting by a negative ```n``` means shifting in the other direction,
        /// and we assume ```lhs``` has an infinite range before truncating it to the original type.
        /// Every operation is defined under this trait.
        template <std::integral Under>
        struct scalar {
        private:
            static constexpr int bits = std::numeric_limits<std::make_unsigned_t<Under>>::digits;
            struct shift_right;
            struct shift_left {
                static Under operator()(Under lhs, auto n) noexcept {
                    if (n < 0) return shift_right{}(lhs, -n);
                    if (n >= bits) return 0;
                    return lhs << n;
                }
            };
            struct shift_right {
                static Under operator()(Under lhs, auto n) noexcept {
                    if (n < 0) return shift_left{}(lhs, -n);
                    if (n >= bits) return lhs < 0 ? -1 : 0;
                    return lhs >> n;
                }
            };
        public:
            static constexpr shift_op_functors ops = {
                .shift_left = shift_left{},
                .shift_right = shift_right{}
            };
            static constexpr shift_asg_op_functors asg_ops = {
                .shift_left_asg = asg_wrap<shift_left{}>{},
                .shift_right_asg = asg_wrap<shift_right{}>{}
            };
        };

        /// Circular bit-shifting. Every operation is defined under this trait.
        template <std::unsigned_integral Under>
        struct circular {
        private:
            using U = std::make_unsigned_t<Under>;
            template <typename Self, typename Other>
            struct convertible_to_int : is_lossless_convertible<make_fundamental_t<Other>, int> {};
        public:
            static constexpr shift_op_functors ops = {
                .shift_left = std::rotl<U>,
                .shift_right = std::rotr<U>,
                // because ```std::rotl``` and ```std::rotr``` takes ```int``` as argument
                .binary_traits = binary_op_traits<detail::get_self, convertible_to_int>{}
            };
            static constexpr shift_asg_op_functors asg_ops = {
                .shift_left_asg = asg_wrap<std::rotl<U>>{},
                .shift_right_asg = asg_wrap<std::rotr<U>>{},
                .asg_traits = asg_op_traits<convertible_to_int>{}
            };
        };

        /// Throws an exception for invalid ```n```. Every operation is defined under this trait.
        template <std::integral Under>
        struct checked {
        private:
            static constexpr int bits = std::numeric_limits<std::make_unsigned_t<Under>>::digits;
            static constexpr auto shift_left = [](Under lhs, auto n) {
                if (n < 0 || n >= bits) throw std::domain_error("shift out of range");
                return lhs << n;
            };
            static constexpr auto shift_right = [](Under lhs, auto n) {
                if (n < 0 || n >= bits) throw std::domain_error("shift out of range");
                return lhs >> n;
            };
        public:
            static constexpr shift_op_functors ops = {
                .shift_left = shift_left,
                .shift_right = shift_right,
            };
            static constexpr shift_asg_op_functors asg_ops = {
                .shift_left_asg = asg_wrap<shift_left>{},
                .shift_right_asg = asg_wrap<shift_right>{}
            };
        };
    }

    template <
        typename Self, typename Other,
        typename SelfInfo = std::numeric_limits<Self>,
        typename OtherInfo = std::numeric_limits<std::remove_cvref_t<Other>>>
    struct is_same_width : std::bool_constant<requires {
        requires integer_like<std::remove_cvref_t<Self>>;
        requires integer_like<std::remove_cvref_t<Other>>;
        requires SelfInfo::digits + SelfInfo::is_signed == OtherInfo::digits + OtherInfo::is_signed;
    }> {};
    template <typename T, typename Ref>
    concept same_sign_as = requires {
        requires integer_like<T>;
        requires std::numeric_limits<T>::is_signed == std::numeric_limits<Ref>::is_signed;
    };

    namespace detail {

    }

    template <
        std::integral Under,
        template<typename> typename IBTrait = integral_behavior::sane,
        template<typename> typename SBTrait = shift_behavior::standard>
    requires (
        tagged<decltype(IBTrait<Under>::ops), integral_op_functors_tag> &&
        tagged<decltype(IBTrait<Under>::asg_ops), integral_asg_op_functors_tag> &&
        tagged<decltype(SBTrait<Under>::ops), shift_op_functors_tag> &&
        tagged<decltype(SBTrait<Under>::asg_ops), shift_asg_op_functors_tag>
    )
    struct integer : arithmetic_ops<
        IBTrait<Under>::ops, IBTrait<Under>::asg_ops,
        {.binary_traits = binary_op_traits<detail::get_self, is_same_width>{}},
        {.asg_traits = asg_op_traits<is_same_width>{}},
        SBTrait<Under>::ops, SBTrait<Under>::asg_ops
    > {
        using tag = struct integer_tag;
        using underlying_type = Under;
        using integral_behavior = IBTrait<Under>;
        using shift_behavior = SBTrait<Under>;
        template <std::integral ToUnder>
        using to = integer<ToUnder, IBTrait>;
        template <template<typename> typename ToIBTrait = IBTrait, template<typename> typename ToSBTrait = SBTrait>
        using adopt = integer<Under, ToIBTrait, ToSBTrait>;
        /// Made public to preserve structural type. Use at your own risk.
        underlying_type under_;
        constexpr integer() noexcept = default;
        /// Normal integer conversion from a number-like value ```other```.
        /// Explicit if ```other``` is not ```lossless_convertible_to``` ```Under```.
        template <typename T, typename U = std::remove_cvref_t<T>>
        requires (std::is_convertible_v<make_fundamental_t<U>, underlying_type>)
        explicit(!lossless_convertible_to<make_fundamental_t<U>, underlying_type>)
        constexpr integer(T&& other) noexcept : under_(to_fundamental(other)) {}
        /// This allows implicit conversions from number literals.
        /// @throw std::overflow_error (at compile time)
        /// if the number (with sign preserved) cannot be represented by ```underlying_type```.
        /// @remark ```Ts``` is used only for decreasing the priority of this constructor.
        template <std::integral T, typename... Ts>
        requires (qualifiers_of_v<T> == type_qualifiers::none && sizeof...(Ts) == 0)
        consteval integer(T&& other, Ts...) : under_(other) {
            if (std::is_unsigned_v<Under> && other < 0) {
                throw std::overflow_error("negative number assigned to an unsigned type. "
                    "Use the explicit constructor if you want it to wrap around.");
            }
            if (other > std::numeric_limits<underlying_type>::max()) {
                throw std::overflow_error("integer overflow");
            }
            if (other < std::numeric_limits<underlying_type>::min()) {
                throw std::overflow_error("integer underflow");
            }
        }
        constexpr underlying_type& to_underlying() noexcept { return under_; }
        constexpr underlying_type to_underlying() const noexcept { return under_; }
        /// Conversion to integer-like type ```T```.
        /// Explicit if ```underlying_type``` is not ```lossless_convertible_to``` ```T```.
        template <typename T>
        explicit(!lossless_convertible_to<underlying_type, T>)
        constexpr operator T() const noexcept { return under_; }
        constexpr bool operator==(const same_sign_as<integer> auto& other) const noexcept {
            return under_ == to_fundamental(other);
        }
        constexpr auto operator<=>(const same_sign_as<integer> auto& other) const noexcept {
            return under_ <=> to_fundamental(other);
        }
        using integer::utils_ops_base_type::operator+;
        constexpr underlying_type operator+() const noexcept { return under_; }
        template <typename CharT, typename Traits>
        friend auto& operator<<(std::basic_ostream<CharT, Traits>& os, const integer& i) {
            return os << i.under_;
        }
        template <typename CharT, typename Traits>
        friend auto& operator>>(std::basic_istream<CharT, Traits>& is, integer& i) {
            using max_t = std::conditional_t<std::is_signed_v<underlying_type>, std::intmax_t, std::uintmax_t>;
            max_t u;
            is >> u;
            if (u > std::numeric_limits<underlying_type>::max() || u < std::numeric_limits<underlying_type>::min()) {
                is.setstate(std::ios_base::failbit);
            }
            i.under_ = static_cast<underlying_type>(u);
            return is;
        }
    };
    template <tagged<integer_tag> T>
    struct make_fundamental<T> { using type = typename T::underlying_type; };

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
    static constexpr T min() noexcept { return std::numeric_limits<typename T::underlying_type>::min(); }
    static constexpr T max() noexcept { return std::numeric_limits<typename T::underlying_type>::max(); }
    static constexpr T lowest() noexcept { return std::numeric_limits<typename T::underlying_type>::lowest(); }
    static constexpr T epsilon() noexcept { return std::numeric_limits<typename T::underlying_type>::epsilon(); }
    static constexpr T round_error() noexcept {
        return std::numeric_limits<typename T::underlying_type>::round_error();
    }
    static constexpr T infinity() noexcept { return std::numeric_limits<typename T::underlying_type>::infinity(); }
    static constexpr T quiet_NaN() noexcept { return std::numeric_limits<typename T::underlying_type>::quiet_NaN(); }
    static constexpr T signaling_NaN() noexcept {
        return std::numeric_limits<typename T::underlying_type>::signaling_NaN();
    }
    static constexpr T denorm_min() noexcept { return std::numeric_limits<typename T::underlying_type>::denorm_min(); }
};

template <utils::tagged<utils::integer_tag> T, typename CharT>
struct std::formatter<T, CharT> : std::formatter<typename T::underlying_type, CharT> {
private:
    using parent = std::formatter<typename T::underlying_type, CharT>;
public:
    constexpr auto parse(auto& parse_ctx) {
        return parent::parse(parse_ctx);
    }
    constexpr auto format(T arg, auto& fmt_ctx) const {
        return parent::format(arg.to_underlying(), fmt_ctx);
    }
};