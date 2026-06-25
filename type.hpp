#pragma once
#include <type_traits>
#include <limits>
#include <utility>
#include "bitmask.hpp"
#include "meta.hpp"

/**
 * @file
 * @brief Type manipulation and qualification utilities for compile-time type introspection and transformation.
 */

namespace utils {
    /// @brief Bitmask enumeration representing type qualifiers (const and volatile).
    enum class type_qualifiers {
        none = 0, ///< No qualifiers
        c = 0b1,  ///< Const qualifier
        v = 0b10  ///< Volatile qualifier
    };
    UTILS_BITMASK(type_qualifiers);
    /// @{
    /// @brief Extracts the const and volatile qualifiers from a type.
    /// @remark Ignores reference and pointer decorations, extracting qualifiers from the underlying type.
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
    /// @}

    /// @{
    /// @brief Applies specified const and volatile qualifiers to a type.
    /// @remark Preserves reference and pointer qualifiers of the input type @code T@endcode.
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
    /// @}

    /// @{
    /// @brief Copies the qualifiers and reference kind from one type to another.
    /// @remark Applies the qualifiers extracted from @code RefT@endcode to the type @code T@endcode.
    template <typename RefT, typename T>
    struct follow {
        using type = apply_qualifiers_t<T, qualifiers_of_v<RefT>>;
    };
    template <typename RefT, typename T>
    using follow_t = follow<RefT, T>::type;
    /// @}

    /// @{
    /// @brief Checks if two types are equivalent, ignoring cv-references.
    /// @remark Two types are equivalent if they are the same after removing all const, volatile, and reference qualifiers.
    template <typename T, typename U>
    struct is_equiv : std::is_same<std::remove_cvref_t<T>, std::remove_cvref_t<U>> {};
    template <typename T, typename U>
    constexpr bool is_equiv_v = is_equiv<T, U>::value;
    /// @}

    /// @brief Concept: @code T@endcode is equivalent to @code Ref@endcode (ignoring cv-references).
    template <typename T, typename Ref>
    concept equiv_to = is_equiv_v<T, Ref>;

    /// @{
    /// @brief Checks if a type is an instance of a given template.
    /// @remark The check ignores const, volatile, and reference qualifiers on the type being checked.
    template <template<typename...> typename, typename>
    struct is_template_instance : std::false_type {};
    template <template<typename...> typename Tmpl, typename... Ts>
    struct is_template_instance<Tmpl, Tmpl<Ts...>> : std::true_type {};
    template <template<typename...> typename Tmpl, typename T>
    constexpr bool is_template_instance_v = is_template_instance<Tmpl, std::remove_cvref_t<T>>::value;
    /// @}

    namespace detail {
        template <typename T, auto F, typename Seq>
        struct sequence_apply;
        template <typename T, auto F, T... Ns>
        struct sequence_apply<T, F, std::integer_sequence<T, Ns...>> {
            using type = std::integer_sequence<T, F(Ns)...>;
        };
    }
    /// @{
    /// @brief Creates an integer sequence with custom Begin and Step values.
    /// @tparam T The underlying integer type for the sequence.
    /// @tparam End The exclusive end value of the range.
    /// @tparam Begin The starting value of the range (default: 0).
    /// @tparam Step The step size between elements (default: 1).
    /// @remark Generates a sequence [Begin, Begin+Step, Begin+2*Step, ...) up to End.
    template <typename T, T End, T Begin = 0, T Step = 1>
    requires (End >= Begin)
    using make_integer_range = detail::sequence_apply<T, [](T n) { return Begin + n * Step; },
        std::make_integer_sequence<T, (End - Begin) / Step>>::type;

    /// @brief Creates a size_t index sequence with custom Begin and Step values.
    /// @remark Convenience alias for @code make_integer_range@endcode with @code std::size_t@endcode type.
    template <std::size_t End, std::size_t Begin = 0, std::size_t Step = 1>
    requires (End >= Begin)
    using make_index_range = make_integer_range<std::size_t, End, Begin, Step>;
    /// @}

    /// @{
    /// @brief A trait that is always @code true_type@endcode, regardless of template arguments.
    /// @remark Useful as a template argument or specialization fallback.
    template <typename...>
    struct always_true : std::true_type {};

    /// @brief A trait that is always @code false_type@endcode, regardless of template arguments.
    /// @remark Useful as a template argument or specialization fallback.
    template <typename...>
    struct always_false : std::false_type {};
    /// @}

    /// @{
    /// @brief Concept: @code T@endcode has a tag member that is or contains @code Tag@endcode.
    /// @remark Checks if the type has a @code tag@endcode member alias matching or containing @code Tag@endcode.
    template <typename T, typename Tag>
    concept tagged = (std::is_same_v<typename std::remove_cvref_t<T>::tag, Tag> ||
        meta::contained_in_v<typename std::remove_cvref_t<T>::tag, Tag>);

    /// @brief Trait version of the @code tagged@endcode concept.
    template <typename Tag, typename T>
    struct is_tagged : std::bool_constant<tagged<T, Tag>> {};
    template <typename Tag, typename T>
    constexpr bool is_tagged_v = is_tagged<Tag, T>::value;
    /// @}

    /// @{
    /// @brief Converts a type to its fundamental type equivalent.
    /// @remark For fundamental types, returns the type unchanged. Specializations may handle user-defined types.
    template <typename T>
    struct make_fundamental;
    template <typename T>
    requires (std::is_fundamental_v<T>)
    struct make_fundamental<T> { using type = T; };
    template <typename T>
    using make_fundamental_t = make_fundamental<T>::type;

    /// @brief Casts an object to its fundamental type equivalent.
    /// @remark Uses @code make_fundamental_t@endcode for the conversion.
    template <typename T>
    constexpr auto to_fundamental(const T& obj) { return static_cast<make_fundamental_t<T>>(obj); }
    /// @}

    /// @brief Concept: @code T@endcode is an integer-like type (has integer numeric limits and integral fundamental type).
    template <typename T>
    concept integer_like = std::numeric_limits<T>::is_integer && std::is_integral_v<make_fundamental_t<T>>;

    namespace detail {
        template <typename B>
        concept boolean_testable_impl = std::convertible_to<B, bool>;
    }
    /// @brief C++ standard exposition-only concept: <i>boolean-testable</i>.
    template <typename B>
    concept boolean_testable = detail::boolean_testable_impl<B> && requires (B&& b) {
        { !std::forward<B>(b) } -> detail::boolean_testable_impl;
    };

    /// @brief A class that cannot be copied or moved.
    /// @remark Useful as a base class for types that should only be used by reference or unique ownership.
    struct stale_class {
        constexpr stale_class() = default; ///< Default constructor is allowed
        stale_class(const stale_class&) = delete; ///< Deleted copy constructor
        stale_class(stale_class&&) = delete; ///< Deleted move constructor
        stale_class& operator=(const stale_class&) = delete; ///< Deleted copy assignment
        stale_class& operator=(stale_class&&) = delete; ///< Deleted move assignment
    };

    /// @brief A key type that can only be constructed by specified friend types.
    /// @tparam Ts The friend types that can construct instances of this key (C++26 variadic friends).
    /// @remark When __cpp_variadic_friend is not supported, only a single template parameter @code T@endcode is used.
    /// @remark Useful for controlled access to private or protected constructors.
#ifdef __cpp_variadic_friend
    template <typename... Ts>
#else
    template <typename T>
#endif
    class key {
        constexpr key() noexcept = default; ///< Default constructor (accessible only to friends)
#ifdef __cpp_variadic_friend
        friend Ts...;
#else
        friend T;
#endif
    };

    /// @brief A sink type that discards any arguments passed to its constructor.
    /// @remark Useful for testing, forwarding, or handling variadic arguments that should be ignored.
    struct sink {
        constexpr sink(auto&&...) noexcept {} ///< Accepts any arguments and discards them
    };

    /// @{
    /// @brief Casts a forwarding reference to a const reference of the same category.
    /// @param ref A forwarding reference to const-qualify.
    /// @returns A const reference preserving the reference category (lvalue/rvalue) of @code ref@endcode.
    template <typename T>
    constexpr const T&& as_const(T&& ref) noexcept { return ref; }

    /// @brief Casts a forwarding reference to a volatile reference of the same category.
    /// @param ref A forwarding reference to volatile-qualify.
    /// @returns A volatile reference preserving the reference category (lvalue/rvalue) of @code ref@endcode.
    template <typename T>
    constexpr volatile T&& as_volatile(T&& ref) noexcept { return ref; }

    /// @brief Casts a forwarding reference to a const volatile reference of the same category.
    /// @param ref A forwarding reference to const volatile-qualify.
    /// @returns A const volatile reference preserving the reference category (lvalue/rvalue) of @code ref@endcode.
    template <typename T>
    constexpr const volatile T&& as_cv(T&& ref) noexcept { return ref; }
    /// @}

    /// @{
    /// @brief Const-qualifies a pointer.
    /// @param ptr A pointer to const-qualify.
    /// @returns A const-qualified pointer to the same type.
    template <typename T>
    constexpr const T* as_const_ptr(T* ptr) noexcept { return ptr; }

    /// @brief Volatile-qualifies a pointer.
    /// @param ptr A pointer to volatile-qualify.
    /// @returns A volatile-qualified pointer to the same type.
    template <typename T>
    constexpr volatile T* as_volatile_ptr(T* ptr) noexcept { return ptr; }

    /// @brief Const volatile-qualifies a pointer.
    /// @param ptr A pointer to const volatile-qualify.
    /// @returns A const volatile-qualified pointer to the same type.
    template <typename T>
    constexpr const volatile T* as_cv_ptr(T* ptr) noexcept { return ptr; }
    /// @}

    /// @brief Returns a reference to @code u@endcode, which has similar properties to @code T@endcode.
    ///
    /// Behaves like @code std::forward_like@endcode, except when @code T@endcode is not a reference,
    /// it returns an lvalue reference instead of an rvalue reference.
    /// This is useful for forwarding generic member variables of classes.
    template <typename T, typename U>
    constexpr auto&& forward_stored(U&& u) noexcept {
        constexpr auto q = qualifiers_of_v<T>;
        if (std::is_rvalue_reference_v<T>) return static_cast<apply_qualifiers_t<U&&, q>>(u);
        else return static_cast<apply_qualifiers_t<U&, q>>(u);
    }
}