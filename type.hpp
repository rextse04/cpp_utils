#pragma once
#include <type_traits>
#include <limits>
#include <utility>
#include "bitmask.hpp"
#include "meta.hpp"

namespace utils {
    enum class type_qualifiers {
        none = 0, c = 0b1, v = 0b10
    };
    UTILS_BITMASK(type_qualifiers);
    template <typename T>
    struct qualifiers_of {
    private:
        using enum type_qualifiers;
        using DT = std::remove_reference_t<std::remove_pointer_t<T>>;
    public:
        static constexpr type_qualifiers value{+c * std::is_const_v<DT> + +v * std::is_volatile_v<DT>};
    };
    template <typename T>
    constexpr type_qualifiers qualifiers_of_v = qualifiers_of<T>::value;

    template <typename T, type_qualifiers Q>
    struct apply_qualifiers {
    private:
        using enum type_qualifiers;
        using DT = std::remove_reference_t<std::remove_pointer_t<T>>;
        using VDT = std::conditional_t<Q * v, volatile DT, DT>;
        using CVDT = std::conditional_t<Q * c, const VDT, VDT>;
    public:
        using type = std::conditional_t<std::is_lvalue_reference_v<T>, std::add_lvalue_reference_t<CVDT>,
            std::conditional_t<std::is_rvalue_reference_v<T>, std::add_rvalue_reference_t<CVDT>,
                std::conditional_t<std::is_pointer_v<T>, CVDT*, CVDT>>>;
    };
    template <typename T, type_qualifiers Q>
    using apply_qualifiers_t = apply_qualifiers<T, Q>::type;

    template <typename RefT, typename T>
    struct follow {
        using type = apply_qualifiers_t<T, qualifiers_of_v<RefT>>;
    };
    template <typename RefT, typename T>
    using follow_t = follow<RefT, T>::type;

    template <typename T, typename U>
    struct is_equiv : std::is_same<std::remove_cvref_t<T>, std::remove_cvref_t<U>> {};
    template <typename T, typename U>
    constexpr bool is_equiv_v = is_equiv<T, U>::value;
    template <typename T, typename Ref>
    concept equiv_to = is_equiv_v<T, Ref>;

    template <template<typename...> typename, typename>
    struct is_template_instance : std::false_type {};
    template <template<typename...> typename Tmpl, typename... Ts>
    struct is_template_instance<Tmpl, Tmpl<Ts...>> : std::true_type {};
    template <template<typename...> typename Tmpl, typename T>
    constexpr bool is_template_instance_v = is_template_instance<Tmpl, std::remove_cvref_t<T>>::value;

    namespace detail {
        template <typename T, auto F, typename Seq>
        struct sequence_apply;
        template <typename T, auto F, T... Ns>
        struct sequence_apply<T, F, std::integer_sequence<T, Ns...>> {
            using type = std::integer_sequence<T, F(Ns)...>;
        };
    }
    template <typename T, T End, T Begin = 0, T Step = 1>
    requires (End >= Begin)
    using make_integer_range = detail::sequence_apply<T, [](T n) { return Begin + n * Step; },
        std::make_integer_sequence<T, (End - Begin) / Step>>::type;
    template <std::size_t End, std::size_t Begin = 0, std::size_t Step = 1>
    requires (End >= Begin)
    using make_index_range = make_integer_range<std::size_t, End, Begin, Step>;

    template <typename...>
    struct always_true : std::true_type {};
    template <typename...>
    struct always_false : std::false_type {};

    template <typename Tag, typename T>
    struct is_tagged : std::false_type {};
    template <typename Tag, typename T>
    requires (std::is_same_v<typename std::remove_cv_t<T>::tag, Tag> ||
        meta::contained_in_v<T, typename std::remove_cv_t<T>::tag>)
    struct is_tagged<Tag, T> : std::true_type {};
    template <typename Tag, typename T>
    constexpr bool is_tagged_v = is_tagged<Tag, T>::value;
    template <typename T, typename Tag>
    concept tagged = is_tagged_v<Tag, T>;

    template <typename T>
    struct make_fundamental;
    template <typename T>
    requires (std::is_fundamental_v<T>)
    struct make_fundamental<T> { using type = T; };
    template <typename T>
    using make_fundamental_t = make_fundamental<T>::type;
    template <typename T>
    constexpr auto to_fundamental(const T& obj) { return static_cast<make_fundamental_t<T>>(obj); }

    template <typename T>
    concept integer_like = std::numeric_limits<T>::is_integer && std::is_integral_v<make_fundamental_t<T>>;

    struct stale_class {
        constexpr stale_class() = default;
        stale_class(const stale_class&) = delete;
        stale_class(stale_class&&) = delete;
        stale_class& operator=(const stale_class&) = delete;
        stale_class& operator=(stale_class&&) = delete;
    };

    template <typename... Ts>
    struct join : Ts... {};

#ifdef __cpp_variadic_friend
    template <typename... Ts>
#else
    template <typename T>
#endif
    class key {
        constexpr key() noexcept = default;
#ifdef __cpp_variadic_friend
        friend Ts...;
#else
        friend T;
#endif
    };

    template <typename T>
    constexpr const T&& as_const(T&& ref) noexcept { return ref; }
    template <typename T>
    constexpr volatile T&& as_volatile(T&& ref) noexcept { return ref; }
    template <typename T>
    constexpr const volatile T&& as_cv(T&& ref) noexcept { return ref; }

    template <typename T>
    constexpr const T* as_const_ptr(T* ptr) noexcept { return ptr; }
    template <typename T>
    constexpr volatile T* as_volatile_ptr(T* ptr) noexcept { return ptr; }
    template <typename T>
    constexpr const volatile T* as_cv_ptr(T* ptr) noexcept { return ptr; }
}