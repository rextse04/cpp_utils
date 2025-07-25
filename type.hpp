#pragma once
#include <type_traits>
#include "bitmask.hpp"
#include "tuple.hpp"

namespace utils {
    enum class type_qualifiers {
        none = 0, c = 0b1, v = 0b10
    };
    template <>
    struct is_bitmask<type_qualifiers> : std::true_type {};
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
        using type = std::conditional_t<std::is_reference_v<T>, CVDT&,
            std::conditional_t<std::is_pointer_v<T>, CVDT*, CVDT>>;
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

    template <template<typename...> typename Trait, typename... Args>
    struct bind_front {
        template <typename... Ts>
        struct trait : Trait<Args..., Ts...> {};
    };
    template <template<typename...> typename Trait, typename... Args>
    struct bind_back {
        template <typename... Ts>
        struct trait : Trait<Ts..., Args...> {};
    };

    template <typename...>
    struct always_true : std::true_type {};
    template <typename...>
    struct always_false : std::false_type {};

    template <typename Tag, typename T>
    struct is_tagged : std::false_type {};
    template <typename Tag, typename T>
    requires (std::is_same_v<typename std::remove_cv_t<T>::tag, Tag> ||
        tuple_fulfills_any<bind_front<std::is_same, Tag>::template trait, typename std::remove_cv_t<T>::tag>::value)
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