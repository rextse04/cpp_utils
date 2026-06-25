#pragma once
#include <type_traits>
#include <concepts>
#include <memory>
#include <compare>
#include "memory.hpp"

namespace utils {
    namespace detail {
        inline constexpr struct void_allocator_t {} void_allocator;
    }
    /// @brief Specifies if @code T@endcode is an allocator which can be used in a constructor of @code C@endcode.
    /// @tparam C: <i>Container</i>
    template <typename T, typename C>
    concept container_allocator =
        std::is_same_v<T, detail::void_allocator_t> || (simple_allocator<T> && std::uses_allocator_v<C, T>);

    /// @brief C++ named requirements: <i>EmplaceConstructible</i>
    /// @{
    template <typename T, typename Allocator, typename... Args>
    concept emplace_constructible = requires(Allocator m, T* p, Args&&... args) {
        std::allocator_traits<Allocator>::construct(m, p, static_cast<Args&&>(args)...);
    };
    /// @tparam C: <i>AllocatorAwareContainer</i>
    template <typename C, typename... Args>
    concept emplace_constructible_into = emplace_constructible<typename C::value_type, typename C::allocator_type, Args...>;
    /// @}

    /// @brief C++ named requirements: <i>DefaultInsertable</i>
    /// @{
    template <typename T, typename Allocator>
    concept default_insertable = emplace_constructible<T, Allocator>;
    /// @tparam C: <i>AllocatorAwareContainer</i>
    template <typename C>
    concept default_insertable_into = default_insertable<typename C::value_type, typename C::allocator_type>;
    /// @}

    /// @brief C++ named requirements: <i>MoveInsertable</i>
    /// @{
    template <typename T, typename Allocator>
    concept move_insertable = emplace_constructible<T, Allocator, T&&>;
    /// @tparam C: <i>AllocatorAwareContainer</i>
    template <typename C>
    concept move_insertable_into = move_insertable<typename C::value_type, typename C::allocator_type>;
    /// @}

    /// @brief C++ named requirements: <i>CopyInsertable</i>
    /// @{
    template <typename T, typename Allocator>
    concept copy_insertable = move_insertable<T, Allocator> &&
        emplace_constructible<T, Allocator, T&> &&
        emplace_constructible<T, Allocator, const T&> &&
        emplace_constructible<T, Allocator, const T&&>;
    /// @tparam C: <i>AllocatorAwareContainer</i>
    template <typename C>
    concept copy_insertable_into = copy_insertable<typename C::value_type, typename C::allocator_type>;
    /// @}

    /// @brief C++ named requirements: <i>Erasable</i>
    /// @{
    template <typename T, typename Allocator>
    concept erasable = requires(Allocator m, T* p) {
        std::allocator_traits<Allocator>::destroy(m, p);
    };
    /// @tparam C: <i>AllocatorAwareContainer</i>
    template <typename C>
    concept erasable_from = erasable<typename C::value_type, typename C::allocator_type>;
    /// @}

    /// @brief Precondition of optional container requirements.
    /// @tparam C: <i>Container</i>
    template <typename C>
    concept container_three_way_comparable = (
        std::three_way_comparable<typename C::value_type> ||
        requires (typename C::value_type t, const typename C::value_type ct) {
            { t < t } -> boolean_testable;
            { t < ct } -> boolean_testable;
            { ct < t } -> boolean_testable;
            { ct < ct } -> boolean_testable;
        });

    /// @brief Checks if @code Key@endcode can be used to query @code C@endcode.
    /// @tparam C: an <i>AssociativeContainer</i> or <i>UnorderedAssociativeContainer</i>
    template <typename Key, typename C>
    concept searchable_in = std::convertible_to<Key, typename C::key_type> || (
        std::strict_weak_order<typename C::key_compare, const typename C::key_type&, const Key&> &&
        requires { typename C::key_compare::is_transparent; }
    );
    /// @brief Make a search key for container @code C@endcode from @code Key@endcode, converting if necessary.
    /// @tparam C: an <i>AssociativeContainer</i> or <i>UnorderedAssociativeContainer</i>
    template <typename C, searchable_in<C> Key>
    constexpr decltype(auto) make_search_key(const Key& key) {
        if constexpr (std::is_same_v<Key, typename C::key_type> || requires { typename C::key_compare::is_transparent; }) {
            return key;
        } else {
            return static_cast<C::key_type>(key);
        }
    }
}