#pragma once
#include <utility>
#include <tuple>
#include <type_traits>
#include "type.hpp"
#include "meta.hpp"

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

    template <typename T>
    struct function_decay;
    template <typename T>
    requires requires {
        requires !std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, T>;
        typename function_decay<std::remove_pointer_t<std::remove_cvref_t<T>>>::type;
    }
    struct function_decay<T> : function_decay<std::remove_pointer_t<std::remove_cvref_t<T>>> {};
    template <typename T>
    requires requires {
        requires std::is_same_v<std::remove_cv_t<T>, T>;
        typename function_decay<decltype(&T::operator())>::type;
    }
    struct function_decay<T> : function_decay<decltype(&T::operator())> {};
    template <typename R, typename... Args>
    struct function_decay<R(Args...)> { using type = R(Args...); };
    template <typename R, typename... Args>
    struct function_decay<R(Args...) noexcept> { using type = R(Args...); };
#define UTILS_FUNCTION_TYPE_DECL(qualifiers)\
    template <typename C, typename R, typename... Args>\
    struct function_decay<R(C::*)(Args...) qualifiers> { using type = R(Args...); };\
    template <typename C, typename R, typename... Args>\
    struct function_decay<R(C::*)(Args...) qualifiers noexcept> { using type = R(Args...); };\
    template <typename C, typename R, typename... Args>\
    struct function_decay<R(C::*)(Args...) qualifiers &> { using type = R(Args...); };\
    template <typename C, typename R, typename... Args>\
    struct function_decay<R(C::*)(Args...) qualifiers & noexcept> { using type = R(Args...); };\
    template <typename C, typename R, typename... Args>\
    struct function_decay<R(C::*)(Args...) qualifiers &&> { using type = R(Args...); };\
    template <typename C, typename R, typename... Args>\
    struct function_decay<R(C::*)(Args...) qualifiers && noexcept> { using type = R(Args...); };
    UTILS_FUNCTION_TYPE_DECL()
    UTILS_FUNCTION_TYPE_DECL(const)
    UTILS_FUNCTION_TYPE_DECL(volatile)
    UTILS_FUNCTION_TYPE_DECL(const volatile)
#undef UTILS_FUNCTION_TYPE_DECL
    template <typename T>
    using function_decay_t = function_decay<T>::type;

    template <typename T>
    struct lambda_decay;
    template <typename T>
    requires requires(T t) { typename lambda_decay<decltype(*t)>::type; }
    struct lambda_decay<T> : lambda_decay<decltype(*std::declval<T>())> {};
    template <typename R, typename... Args>
    struct lambda_decay<R(&)(Args...)> { using type = R(Args...); };
    template <typename R, typename... Args>
    struct lambda_decay<R(&)(Args...) noexcept> { using type = R(Args...); };
    template <typename T>
    using lambda_decay_t = lambda_decay<T>::type;

    template <typename F>
    struct result_of;
    template <typename R, typename... Args>
    struct result_of<R(Args...)> { using type = R; };
    template <typename F>
    using result_of_t = result_of<F>::type;

    template <typename F>
    struct arguments_of;
    template <typename R, typename... Args>
    struct arguments_of<R(Args...)> { using type = std::tuple<Args...>; };
    template <typename F>
    using arguments_of_t = arguments_of<F>::type;

    template <typename F>
    struct arity_of;
    template <typename R, typename... Args>
    struct arity_of<R(Args...)> : std::integral_constant<std::size_t, sizeof...(Args)> {};
    template <typename F>
    constexpr auto arity_of_v = arity_of<F>::value;

    template <std::size_t Arity>
    struct arity {};
    using variadic_arity = arity<static_cast<std::size_t>(-1)>;
    struct owning {};
    template <typename F, std::size_t Arity, bool Owning = false>
    struct with {
        using tag = struct with_tag;
        using type = F;
        static constexpr std::size_t arity = Arity;
        static constexpr bool is_variadic = arity == static_cast<std::size_t>(-1);
        F func;
        constexpr with(F func) noexcept requires(!Owning) : func(func) {}
        template <typename G>
        constexpr with(G&& func) requires(Owning) : func(std::forward<G>(func)) {}
        constexpr with(utils::arity<Arity>, F func) noexcept requires(!Owning) : func(func) {}
        template <typename G>
        constexpr with(utils::arity<Arity>, owning, G&& func) requires(Owning) : func(std::forward<G>(func)) {}
        constexpr with<std::remove_cvref_t<F>, Arity, true> to_owning() const {
            return {std::forward<F>(func)};
        }
        template <std::size_t Begin = 0>
        constexpr decltype(auto) operator()(meta::tuple_like auto&& args) const {
            const auto call = [this, &args]<std::size_t... Idxs>(std::index_sequence<Idxs...>) {
                return func(std::get<Idxs>(std::forward<decltype(args)>(args))...);
            };
            static constexpr std::size_t End = is_variadic
                ? std::tuple_size_v<std::remove_cvref_t<decltype(args)>>
                : Begin + arity;
            return call(make_index_range<End, Begin>{});
        }
    };
    template <typename G, std::size_t Arity>
    with(arity<Arity>, G&&) -> with<G&&, Arity>;
    template <typename G, std::size_t Arity>
    requires (!std::is_function_v<std::remove_cvref_t<G>>)
    with(arity<Arity>, owning, G&&) -> with<std::remove_cvref_t<G>, Arity, true>;
    template <typename G, std::size_t Arity>
    requires (std::is_function_v<std::remove_cvref_t<G>>)
    with(arity<Arity>, owning, G&&) -> with<std::add_pointer_t<std::remove_cvref_t<G>>, Arity, true>;

    template <typename T>
    struct bind {
        using tag = struct bind_tag;
        using type = T;
        T obj;
    };
    template <typename T>
    bind(T&&) -> bind<T&&>;

    namespace detail {
        template <typename WrapsT, std::size_t Idx = std::tuple_size_v<WrapsT>>
        struct arity_psum {
        private:
            using prev = arity_psum<WrapsT, Idx-1>;
            static_assert(!prev::variadic || Idx == std::tuple_size_v<WrapsT>,
                "no function can be bound after a variadic-arity function");
            static constexpr std::size_t arity = std::tuple_element_t<Idx-1, WrapsT>::arity;
        public:
            static constexpr bool variadic = arity == static_cast<std::size_t>(-1);
            static constexpr std::size_t value = prev::value + arity;
        };
        template <typename WrapsT>
        struct arity_psum<WrapsT, 0> {
            static constexpr bool variadic = false;
            static constexpr std::size_t value = 0;
        };
    }

    namespace detail {
        constexpr auto to_func_wrap = []<typename G>(G&& g) {
            if constexpr (tagged<G, with_tag>) {
                return g.to_owning();
            } else if constexpr(tagged<G, bind_tag>) {
                // forward obj to an owning lambda that returns obj
                return with(arity<0>{}, owning{}, [obj = std::forward<typename G::type>(g.obj)]() {return obj;});
            } else {
                return with(arity<arity_of_v<function_decay_t<G>>>{}, owning{}, std::forward<G>(g));
            }
        };
        template <typename... Gs>
        constexpr auto get_wraps(Gs&&... gs) {
            return meta::transform(to_func_wrap, std::forward_as_tuple(std::forward<Gs>(gs)...));
        }
    }
    template <typename F, typename... Gs>
    constexpr auto composite(F&& f, Gs&&... gs) {
        using wraps_type = decltype(detail::get_wraps(std::forward<Gs>(gs)...));
        return [f = std::forward<F>(f), wraps = detail::get_wraps(std::forward<Gs>(gs)...)]
            <typename... Args>(Args&&... args)
            requires(detail::arity_psum<wraps_type>::variadic ||
                detail::arity_psum<wraps_type>::value == sizeof...(Args)) {
            const auto call = [&f, &wraps, args_tuple = std::forward_as_tuple(std::forward<Args>(args)...)]
                <std::size_t... Idxs>(std::index_sequence<Idxs...>) {
                return f(std::get<Idxs>(wraps).template operator()
                    <detail::arity_psum<decltype(wraps), Idxs>::value>(std::move(args_tuple))...);
            };
            return call(make_index_range<std::tuple_size_v<decltype(wraps)>>{});
        };
    }
}
