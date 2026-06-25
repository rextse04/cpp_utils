#pragma once
#include <flat_map>
#include <concepts>
#include <type_traits>
#include <functional>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <algorithm>
#include <bit>
#include <initializer_list>
#include <stdexcept>
#include <iterator>
#include <array>
#include <limits>
#include <numeric>
#include "functional.hpp"
#include "memory.hpp"
#include "ranges.hpp"
#include "iterator.hpp"
#include "interval.hpp"
#include "containers/type.hpp"
#include <type.hpp>
#include "compare.hpp"
#include "swap.hpp"

namespace utils::detail {
    template <typename Base>
    class segment_tree_reference {
    private:
        Base& base_;
        Base::size_type node_;
    public:
        constexpr segment_tree_reference(Base& base, Base::size_type idx) noexcept : base_(base), node_(idx) {}
        constexpr const typename Base::key_type& key() const noexcept { return base_.keys()[node_]; }
        constexpr const typename Base::mapped_type& value() const noexcept { return base_.nodes()[node_]; }
        constexpr operator const typename Base::mapped_type&() const noexcept { return base_.nodes()[node_]; }
        template <typename U>
        requires (std::is_assignable_v<typename Base::mapped_type&, U&&>)
        constexpr const segment_tree_reference& operator=(U&& mapped) const {
            base_.do_assign(node_, std::forward<U>(mapped));
            return *this;
        }
    };
    template <std::size_t I, typename Base>
    constexpr auto&& get(const segment_tree_reference<Base>& ref) noexcept {
        if constexpr (I == 0) return ref.key();
        else if constexpr (I == 1) return ref;
    }
    template <typename T, typename Base>
    constexpr T get(const segment_tree_reference<Base>& ref) noexcept {
        if constexpr (std::is_same_v<T, const typename Base::key_type&>) return ref.key();
        else if constexpr (std::is_same_v<T, const segment_tree_reference<Base>&>) return ref;
    }
}
template <std::size_t I, typename Base>
struct std::tuple_element<I, utils::detail::segment_tree_reference<Base>> {
    using type = decltype(get<I>(std::declval<utils::detail::segment_tree_reference<Base>>()));
};
template <typename Base>
struct std::tuple_size<utils::detail::segment_tree_reference<Base>> : std::integral_constant<std::size_t, 2> {};

namespace utils {
    namespace detail {
        template <typename F, typename T, typename Multiplier>
        concept multiplies_for = requires (const F& multiplies, const T& a, const Multiplier& m) {
            { multiplies(a, m) } -> std::convertible_to<T>;
        };
    }
    /// @brief A disambiguation flag to declare that keys and mapped values given to the constructor
    /// conform to the internal layout of @code segment_tree@endcode.
    inline constexpr struct segment_tree_nodes_t {} segment_tree_nodes;
    /// @brief Enumeration type representing the direction a @code segment_tree_traverser_for@endcode chooses to go,
    /// from root to leaves.
    enum class segment_tree_traverse_direction { stop, left, right, both };
    /// @brief Specifies that @code T@endcode is a traverser for @code segment_tree@endcode @code ST@endcode.
    ///
    /// Given the following values:
    /// | Value | Definition |
    /// | ----- | ---------- |
    /// | @code traverser@endcode | A traverser of type @code T@endcode |
    /// | @code st@endcode | The segment tree (of type @code ST@endcode) @code traverser@endcode is traversing |
    /// | @code info@endcode | A @code segment_tree_node_info@endcode describing the node @code traverser@endcode is at |
    /// The following operations should be supported:
    /// | Statement | Semantics |
    /// | --------- | --------- |
    /// | @code traverser(st, info)@endcode | Returns which child(ren) of the current node @code traverser@endcode should visit next. |
    template <typename T, typename ST>
    concept segment_tree_traverser_for = requires (T traverser, const ST& st, const typename ST::node_info& info) {
        { traverser(st, info) } -> std::convertible_to<segment_tree_traverse_direction>;
    };
    /// @brief Specifies that @code T@endcode is a strategy for traversing @code ST@endcode.
    ///
    /// A traverser strategy produces a value at every node (in @code st@endcode) the traverser visits,
    /// which is "joined" in a bottom-up manner to produce a final output from the traversal.
    /// Suppose a traverser following @code strategy@endcode is currently at a node @code v@endcode. Define the following values:
    /// | Value | Definition |
    /// | ----- | ---------- |
    /// | @code st@endcode | A const reference to the segment tree (of type @code ST@endcode) the traverser is traversing. |
    /// | @code info@endcode | A pointer to a @code segment_tree_node_info@endcode describing the node @code v@endcode, or @code nullptr@endcode if @code v@endcode does not exist. |
    /// | @code L@endcode | The value produced by the left child of @code v@endcode, or @code nullptr@endcode if not visited by @code st@endcode. |
    /// | @code R@endcode | The value produced by the right child of @code v@endcode, or @code nullptr@endcode if not visited by @code st @endcode. |
    /// The following operations are then executed in the order presented:
    /// 1. If @code strategy@endcode does not stop at @code v@endcode and @code strategy.prepare(st, *info)@endcode is well-formed,
    /// it is invoked with return value discarded.
    /// 2. @code L@endcode and @code R@endcode are collected.
    /// 3. @code strategy(st, info, L, R)@endcode is produced at @f$v@f$.
    template <typename T, typename ST>
    concept segment_tree_strategy_for = requires (
        T strategy, const ST& st, const typename ST::node_info& info,
        decltype(strategy(st, nullptr, nullptr, nullptr)) V, const decltype(V)& L, const decltype(V)& R) {
        { strategy(st, &info, nullptr, nullptr) } -> std::same_as<decltype(V)>;
        { strategy(st, &info, &L, nullptr) } -> std::same_as<decltype(V)>;
        { strategy(st, &info, nullptr, &R) } -> std::same_as<decltype(V)>;
        { strategy(st, &info, &L, &R) } -> std::same_as<decltype(V)>;
    };
    /// @brief A key-immutable (multi-)associative container adaptor that supports linear-time construction and
    /// logarithmic-time search and aggregation.
    ///
    /// Segment tree is a data structure which partitions a set of keys into contiguous segments of lengths of powers of 2,
    /// where length is defined as the number of keys contained in the segment.
    /// @code segment_tree@endcode, similar to @code std::flat_multimap@endcode, is a container adaptor which stores
    /// keys and mapped values in two underlying containers of respective types
    /// @code KeyContainer@endcode and @code MappedContainer@endcode.
    /// Given @f$n@f$ key-mapped pairs, @code keys@endcode (of type @code KeyContainer@endcode) and
    /// @code nodes@endcode (of type @code MappedContainer@endcode) conforms to the <i>internal layout</i> of the class
    /// if and only if all of the following hold:
    /// 1. @code keys.size() == n@endcode and contains the keys in non-decreasing order according to @code Compare@endcode.
    /// (Order of equivalent keys is unspecified.)
    /// 2. For @f$i\in\{0,...,n-1\}@f$, @code node[i]@endcode is paired with @code keys[i]@endcode in the original input.
    /// 3. Let @f$L(0)=n@f$ and @f$L(\ell)=\lceil L(\ell-1)/2\rceil@f$ for @f$\ell\in\mathbb{N}@f$.
    /// Further let @f$S@f$ be the partial sum of @f$L@f$, that is, @f$S(\ell)=\sum_{i=0}^{\ell} L(i)@f$.
    /// Denote by @f$h@f$ the height of the tree, that is, the smallest integer such that @f$L(h)=1@f$.
    /// Then, for @f$\ell\in\{0,...,h\}@f$ and @f$j\in\{0,...,L(h)-1@f$,
    /// @f$\mathrm{nodes}[S(\ell)+j]=\sum_{i=2^\ell j}^{2^\ell(j+1)-1}N[j]@f$,
    /// where @f$N[j]@f$ is @code nodes[j]@endcode if @f$j<n@f$ and the identity (defined by @code Identity@endcode otherwise,
    /// and the sum operator is defined by @code Sum@endcode.
    ///
    /// @code segment_tree@endcode is only meaningful on mapped values in a monoid.
    /// More specifically, given an object @code sum@endcode of type @code Sum@endcode and
    /// @code e@endcode of type @code Identity@endcode, constructing the class is undefined unless
    /// all following semantic requirements are met:
    /// 1. Associativity: For (possibly cv-qualified) objects @code a@endcode,
    /// @code b@endcode and @code c@endcode of type @code T@endcode,
    /// @code sum(sum(a,b),c)@endcode and @code sum(a,sum(b,c))@endcode are equivalent.
    /// 2. Identity: For any (possibly cv-qualified) object @code a@endcode of type @code T@endcode,
    /// @code sum(a,e())@endcode, @code sum(e(),a)@endcode and @code a@endcode are equivalent.
    ///
    /// An <i>update record</i> is a container of type @code update_type@endcode
    /// produced by some member functions of @code segment_tree@endcode
    /// that represents extraneous updates to mapped values not reflected in the underlying containers.
    /// Although the internal layout of an update record is implementation-defined,
    /// it can be identified by a map @f$U@f$ from @code keys()@endcode to @code mapped_type@endcode.
    /// Any member function that accepts an update record behaves as if for any @code k@endcode in @code keys()@endcode,
    /// the corresponding mapped value is @code sum()(at(k)+U[k])@endcode.
    /// It is undefined to pass an object of @code update_type@endcode to any such member function that is not
    /// produced by the same @code segment_tree@endcode.
    ///
    /// @code segment_tree@endcode meets the requirements of <i>Container</i>, <i>ReversibleContainer</i>,
    /// optional container requirements, and <i>AssociativeContainer</i>
    /// except all operations that are only supported on non-const instances.
    ///
    /// <h3>Iterator Invalidation</h3>
    /// Iterators and update records are invalidated by any non-const operation.
    /// It is undefined to use any invalidated iterator or pass invalidated update records to any member functions that accept them.
    /// In particular, invalidated update records still remain valid containers.
    ///
    /// <h3>Additional Type Requirements</h3>
    /// In any member function that accepts a @code multiplies@endcode argument,
    /// it is undefined (unless ill-formed) to call to such function unless all the following hold true
    /// for any (possibly cv-qualified) object @code a@endcode of type @code T@endcode:
    /// 1. @code multiplies(a,0)@endcode is equivalent to @code identity()()@endcode.
    /// 2. For any non-zero @code n@endcode of @code size_type@endcode,
    /// @code multiplies(a,n)@endcode is equivalent to @f$\sum_{i=1}^n a@f$ (with respect to @code sum()@endcode).
    ///
    /// @tparam Key: Type of keys
    /// @tparam T: Type of mapped values
    /// @tparam Compare: Functor type for comparing keys that satisfies <i>Compare</i>
    /// @tparam Sum: Functor type providing monoid operation on two mapped values
    /// @tparam Identity: Functor type providing the identity in @code T@endcode
    /// @tparam KeyContainer, MappedContainer: Container types that satisfies <i>SequentialContainer</i>
    /// for storing keys and mapped values respectively.
    /// The iterators of such containers should model @code std::random_access_iterator@endcode.
    /// Invocations of their member functions @code size@endcode and @code max_size@endcode should not exit via an exception.
    template <
        typename Key, typename T,
        std::copyable Compare = std::less<Key>,
        std::copyable Sum = std::plus<T>,
        std::copyable Identity = default_construct<T>,
        typename KeyContainer = std::vector<Key>, typename MappedContainer = std::vector<T>>
    requires (
        std::strict_weak_order<Compare, const Key&, const Key&> &&
        requires (const Sum sum, const T& mapped) { { sum(mapped, mapped) } -> std::convertible_to<T>; } &&
        requires (const Identity identity) { { identity() } -> std::convertible_to<T>; })
    class segment_tree {
        using enum segment_tree_traverse_direction;
    public:
        using key_container_type = KeyContainer;
        using mapped_container_type = MappedContainer;
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<key_type, mapped_type>;
        using key_compare = Compare;
        using mapped_sum = Sum;
        using mapped_identity = Identity;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = detail::segment_tree_reference<segment_tree>;
        using const_reference = std::pair<const Key&, const T&>;
        using update_type = mapped_container_type;
        struct containers {
            key_container_type keys;
            mapped_container_type values;
        };
        class value_compare {
        private:
            key_compare comp_;

            constexpr value_compare(const key_compare& comp) : comp_(comp) {}
        public:
            constexpr bool operator()(const value_type& lhs, const value_type& rhs) const {
                return comp_(lhs.first, rhs.first);
            }
        };
        friend reference;
    private:
        std::byte keys_[sizeof(key_container_type)];
        std::byte nodes_[sizeof(mapped_container_type)];
        std::array<size_type, std::numeric_limits<size_type>::digits + 1> level_offsets_{};
        int levels_{0};
        [[no_unique_address]] key_compare comp_{};
        [[no_unique_address]] mapped_sum sum_{};
        [[no_unique_address]] mapped_identity identity_{};

        constexpr key_container_type& m_keys() noexcept {
            return *std::launder(reinterpret_cast<key_container_type*>(keys_));
        }
        constexpr mapped_container_type& m_nodes() noexcept {
            return *std::launder(reinterpret_cast<mapped_container_type*>(nodes_));
        }
        constexpr void sort() {
            ranges::sort(views::zip(m_keys(), m_nodes()), comp_, [](const auto& pair) -> const key_type& {
                return get<0>(pair);
            });
        }
        constexpr void build() {
            level_offsets_[levels_ = 1] = keys().size();
            size_type prev = 0;
            while (level_offsets_[levels_] - level_offsets_[levels_ - 1] > 1) {
                const size_type prev_end = nodes().size();
                while (prev < prev_end - 1) {
                    m_nodes().push_back(sum_(nodes()[prev], nodes()[prev + 1]));
                    prev += 2;
                }
                if (prev != prev_end) {
                    m_nodes().push_back(sum_(nodes()[prev], identity_()));
                    ++prev;
                }
                level_offsets_[++levels_] = nodes().size();
            }
        }
        template <bool Sort, typename Pairs>
        constexpr void unzip(Pairs&& pairs) {
            if constexpr (ranges::sized_range<Pairs>) {
                const auto size = ranges::size(pairs);
                if constexpr (reservable_container<key_container_type>) m_keys().reserve(size);
                if constexpr (reservable_container<mapped_container_type>) m_nodes().reserve(size);
            }
            for (auto&& pair : pairs) {
                auto&& v = static_cast<follow_t<decltype(pair), value_type&&>>(pair);
                m_keys().push_back(std::move(v.first));
                m_nodes().push_back(std::move(v.second));
            }
            if constexpr (Sort) sort();
            build();
        }
        constexpr void fast_clear() noexcept {
            level_offsets_[1] = 0;
            levels_ = 0;
        }
    public:
        /// @brief Returns a const reference to the underlying container for keys.
        constexpr const key_container_type& keys() const noexcept {
            return *std::launder(reinterpret_cast<const key_container_type*>(keys_));
        }
        /// @brief Returns a const reference to the underlying container for nodes.
        ///
        /// It is guaranteed that the first @code size()@endcode nodes are mapped values corresponding to @code keys()@endcode.
        constexpr const mapped_container_type& nodes() const noexcept {
            return *std::launder(reinterpret_cast<const mapped_container_type*>(nodes_));
        }
        /// @brief An array containing the number of nodes included in and below each level.
        constexpr const decltype(level_offsets_)& level_offsets() const noexcept { return level_offsets_; }
        /// @brief Returns a copy of the key comparison functor.
        constexpr key_compare key_comp() const { return comp_; }
        /// @brief Returns a comparison functor for @code value_type@endcode based on @code key_comp()@endcode.
        constexpr value_compare value_comp() const { return value_compare(comp_); }
        /// @brief Returns a copy of the monoid operation functor.
        constexpr mapped_sum sum() const { return sum_; }
        /// @brief Returns a copy of the identity functor.
        constexpr mapped_identity identity() const { return identity_; }
    private:
        template <typename Allocator, typename... Args>
        constexpr void construct_keys(const Allocator& alloc, Args&&... args) {
            if constexpr (std::is_same_v<Allocator, detail::void_allocator_t>) {
                new(keys_) key_container_type(std::forward<Args>(args)...);
            } else {
                std::uninitialized_construct_using_allocator(keys_, alloc, std::forward<Args>(args)...);
            }
        }
        template <typename Allocator, typename... Args>
        constexpr void construct_nodes(const Allocator& alloc, Args&&... args) {
            if constexpr (std::is_same_v<Allocator, detail::void_allocator_t>) {
                new(nodes_) mapped_container_type(std::forward<Args>(args)...);
            } else {
                std::uninitialized_construct_using_allocator(nodes_, alloc, std::forward<Args>(args)...);
            }
        }
    public:
        /// @defgroup segment_tree<Key,T,Compare,Sum,Identity,KeyContainer,MappedContainer>::segment_tree
        ///
        /// When the @code std::sorted_equivalent@endcode flag is passed to any constructor overload that accepts it,
        /// it indicates that the range of keys passed to the constructor is sorted in non-decreasing order
        /// with respect to @code key_compare()@endcode, and the range of mapped values passed corresponds to it.
        /// The behavior is undefined if the flag is passed without satisfying this requirement.
        ///
        /// When an allocator is passed to @code alloc@endcode in any of the following constructor overloads,
        /// the underlying containers are constructed with uses-allocator construction.
        /// Such overloads only participate in overload resolution only if
        /// @code std::uses_constructor_v<segment_tree, Allocator>@endcode is @code true@endcode.
        /// @{
        /// Default constructor. Constructs an empty container adaptor.
        /// @{
        template <container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        explicit constexpr segment_tree(
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator) :
            comp_(comp), sum_(sum), identity_(identity) {
            construct_keys(alloc);
            construct_nodes(alloc);
        }
        template <typename Allocator>
        explicit constexpr segment_tree(const Allocator& alloc) :
            segment_tree(key_compare(), mapped_sum(), mapped_identity(), alloc) {}
        /// @}
        /// Copy constructor. Copy-constructs all underlying containers and functors.
        /// @{
        constexpr segment_tree(const segment_tree& other)
        noexcept(
            std::is_nothrow_copy_constructible_v<key_container_type> &&
            std::is_nothrow_copy_constructible_v<mapped_container_type>) :
            segment_tree(other, detail::void_allocator) {}
        constexpr segment_tree(const segment_tree& other, const container_allocator<segment_tree> auto& alloc):
            level_offsets_(other.level_offsets_), levels_(other.levels_),
            comp_(other.comp_), sum_(other.sum_), identity_(other.identity_) {
            construct_keys(alloc, other.keys());
            construct_nodes(alloc, other.nodes());
        }
        /// @}
        /// Move constructor. Move-constructs all underlying containers and functors.
        /// @code other@endcode is left in a valid empty state.
        /// @{
        constexpr segment_tree(segment_tree&& other)
        noexcept(
            std::is_nothrow_move_constructible_v<key_container_type> &&
            std::is_nothrow_move_constructible_v<mapped_container_type>) :
            segment_tree(std::move(other), detail::void_allocator) {}
        constexpr segment_tree(segment_tree&& other, const container_allocator<segment_tree> auto& alloc) :
            level_offsets_(other.level_offsets_), levels_(other.levels_),
            comp_(std::move(other.comp_)), sum_(std::move(other.sum_)), identity_(other.identity_) {
            construct_keys(alloc, std::move(other.m_keys()));
            construct_nodes(alloc, std::move(other.m_nodes()));
            other.fast_clear();
        }
        /// @}
        /// Directly constructs the underlying containers using @code keys@endcode and @code nodes@endcode.
        /// The behavior is undefined if they do not conform to the internal layout of @code segment_tree@endcode.
        /// @{
        template <equiv_to<key_container_type> Keys, equiv_to<mapped_container_type> Nodes,
            container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(segment_tree_nodes_t, Keys&& keys, Nodes&& nodes,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator) :
            comp_(comp), sum_(sum), identity_(identity) {
            construct_keys(alloc, std::forward<Keys>(keys));
            construct_nodes(alloc, std::forward<Nodes>(nodes));
            if (this->keys().empty()) return;
            level_offsets_[levels_ = 1] = this->keys().size();
            while (level_offsets_[levels_] > 1) {
                level_offsets_[levels_ + 1] = level_offsets_[levels_] / 2 + level_offsets_[levels_] % 2;
                ++levels_;
            }
            std::partial_sum(level_offsets_.begin() + 1, level_offsets_.begin() + levels_ + 1,
                level_offsets_.begin() + 1);
        }
        template <equiv_to<key_container_type> Keys, equiv_to<mapped_container_type> Nodes,
            container_allocator<segment_tree> Allocator>
        constexpr segment_tree(segment_tree_nodes_t, Keys&& keys, Nodes&& nodes, const Allocator& alloc) :
            segment_tree(segment_tree_nodes, std::forward<Keys>(keys), std::forward<Nodes>(nodes),
                key_compare(), mapped_sum(), mapped_identity(), alloc) {}
        /// @}
        /// Constructs the underlying containers from @code keys@endcode and @code mapped@endcode,
        /// sorts them if unsorted, and builds the segment tree.
        /// The behavior is undefined if @code keys.size() != mapped.size()@endcode.
        /// @{
        template <equiv_to<key_container_type> Keys, equiv_to<mapped_container_type> Mapped,
            container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(Keys&& keys, Mapped&& mapped,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (move_insertable_into<mapped_container_type> && std::swappable<key_type> && std::swappable<mapped_type>) :
            comp_(comp), sum_(sum), identity_(identity) {
            construct_keys(alloc, std::forward<Keys>(keys));
            construct_nodes(alloc, std::forward<Mapped>(mapped));
            sort();
            build();
        }
        template <equiv_to<key_container_type> Keys, equiv_to<mapped_container_type> Mapped,
            container_allocator<segment_tree> Allocator>
        constexpr segment_tree(Keys&& keys, Mapped&& mapped, const Allocator& alloc)
        requires (move_insertable_into<mapped_container_type> && std::swappable<key_type> && std::swappable<mapped_type>) :
            segment_tree(std::forward<Keys>(keys), std::forward<Mapped>(mapped), key_compare(), mapped_sum(), alloc) {}
        template <equiv_to<key_container_type> Keys, equiv_to<mapped_container_type> Mapped,
            container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(std::sorted_equivalent_t, Keys&& keys, Mapped&& mapped,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (move_insertable_into<mapped_container_type>) :
            comp_(comp), sum_(sum), identity_(identity) {
            construct_keys(alloc, std::forward<Keys>(keys));
            construct_nodes(alloc, std::forward<Mapped>(mapped));
            build();
        }
        template <equiv_to<key_container_type> Keys, equiv_to<mapped_container_type> Mapped,
            container_allocator<segment_tree> Allocator>
        constexpr segment_tree(std::sorted_equivalent_t, Keys&& keys, Mapped&& mapped, const Allocator& alloc)
        requires (move_insertable_into<mapped_container_type>) :
            segment_tree(std::sorted_equivalent, std::forward<Keys>(keys), std::forward<Mapped>(mapped),
                key_compare(), mapped_sum(), mapped_identity(), alloc) {}
        /// @}
        /// Inserts key-mapped pairs from @code pairs@endcode, which should be a range of @code value_type@endcode,
        /// sorts them if unsorted, then builds the segment tree.
        /// @{
        template <container_compatible_range<value_type> Pairs,
            container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(std::from_range_t, Pairs&& pairs,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (
            move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type> &&
            std::swappable<key_type> && std::swappable<mapped_type>) :
            comp_(comp), sum_(sum), identity_(identity) {
            construct_keys(alloc);
            construct_nodes(alloc);
            unzip<true>(std::forward<Pairs>(pairs));
        }
        template <container_compatible_range<value_type> Pairs, container_allocator<segment_tree> Allocator>
        constexpr segment_tree(std::from_range_t, Pairs&& pairs, const Allocator& alloc)
        requires (
            move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type> &&
            std::swappable<key_type> && std::swappable<mapped_type>) :
            segment_tree(std::from_range, std::forward<Pairs>(pairs), key_compare(), mapped_sum(), mapped_identity(), alloc) {}
        template <
            container_compatible_range<value_type> Pairs,
            container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(std::sorted_equivalent_t, std::from_range_t, Pairs&& pairs,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type>) :
            comp_(comp), sum_(sum), identity_(identity) {
            construct_keys(alloc);
            construct_nodes(alloc);
            unzip<false>(std::forward<Pairs>(pairs));
        }
        template <container_compatible_range<value_type> Pairs, container_allocator<segment_tree> Allocator>
        constexpr segment_tree(std::sorted_equivalent_t, std::from_range_t, Pairs&& pairs, const Allocator& alloc)
        requires (move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type>) :
            segment_tree(std::sorted_equivalent, std::from_range, std::forward<Pairs>(pairs),
                key_compare(), mapped_sum(), mapped_identity(), alloc) {}
        /// @}
        /// Inserts key-mapped pairs from [ @code pairs_begin@endcode, @code pairs_end@endcode ),
        /// which should be a valid range of @code value_type@endcode,
        /// sorts them if unsorted, then builds the segment tree.
        /// @{
        template <container_compatible_iterator<value_type> PairsIt, std::sentinel_for<PairsIt> PairsSent,
            container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(PairsIt pairs_begin, PairsSent pairs_end,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (
            move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type> &&
            std::swappable<key_type> && std::swappable<mapped_type>):
            segment_tree(std::from_range, ranges::subrange(pairs_begin, pairs_end), comp, sum, identity, alloc) {}
        template <container_compatible_iterator<value_type> PairsIt, std::sentinel_for<PairsIt> PairsSent,
            container_allocator<segment_tree> Allocator>
        constexpr segment_tree(PairsIt pairs_begin, PairsSent pairs_end, const Allocator& alloc)
        requires (
            move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type> &&
            std::swappable<key_type> && std::swappable<mapped_type>) :
            segment_tree(pairs_begin, pairs_end, key_compare(), mapped_sum(), mapped_identity(), alloc) {}
        template <container_compatible_iterator<value_type> PairsIt, std::sentinel_for<PairsIt> PairsSent,
            container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(std::sorted_equivalent_t, PairsIt pairs_begin, PairsSent pairs_end,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type>) :
            segment_tree(std::sorted_equivalent, std::from_range, ranges::subrange(pairs_begin, pairs_end),
                comp, sum, identity, alloc) {}
        template <container_compatible_iterator<value_type> PairsIt, std::sentinel_for<PairsIt> PairsSent,
            container_allocator<segment_tree> Allocator>
        constexpr segment_tree(std::sorted_equivalent_t, PairsIt pairs_begin, PairsSent pairs_end, const Allocator& alloc)
        requires (move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type>) :
            segment_tree(std::sorted_equivalent, pairs_begin, pairs_end,
                key_compare(), mapped_sum(), mapped_identity(), alloc) {}
        /// @}
        /// Inserts key-value pairs from @code pairs@endcode, sorts them if unsorted, then builds the segment tree.
        /// @{
        template <container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(std::initializer_list<value_type> pairs,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (
            move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type> &&
            std::swappable<key_type> && std::swappable<mapped_type>) :
            segment_tree(std::from_range, pairs, comp, sum, identity, alloc) {}
        template <container_allocator<segment_tree> Allocator>
        constexpr segment_tree(std::initializer_list<value_type> pairs, const Allocator& alloc)
        requires (
            move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type> &&
            std::swappable<key_type> && std::swappable<mapped_type>) :
            segment_tree(std::from_range, pairs, alloc) {}
        template <container_allocator<segment_tree> Allocator = detail::void_allocator_t>
        constexpr segment_tree(std::sorted_equivalent_t, std::initializer_list<value_type> pairs,
            const key_compare& comp = key_compare(),
            const mapped_sum& sum = mapped_sum(),
            const mapped_identity& identity = mapped_identity(),
            const Allocator& alloc = detail::void_allocator)
        requires (move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type>) :
            segment_tree(std::sorted_equivalent, std::from_range, pairs, comp, sum, identity, alloc) {}
        template <container_allocator<segment_tree> Allocator>
        constexpr segment_tree(std::sorted_equivalent_t, std::initializer_list<value_type> pairs, const Allocator& alloc)
        requires (move_insertable_into<key_container_type> && move_insertable_into<mapped_container_type>) :
            segment_tree(std::sorted_equivalent, std::from_range, pairs,
                key_compare(), mapped_sum(), mapped_identity(),alloc) {}
        /// @}
        /// @}
        constexpr ~segment_tree()
        requires (
            std::is_trivially_destructible_v<key_container_type> &&
            std::is_trivially_destructible_v<mapped_container_type>) = default;
        constexpr ~segment_tree() {
            std::destroy_at(&m_keys());
            std::destroy_at(&m_nodes());
        }
        /// @defgroup segment_tree<Key,T,Compare,Sum,Identity,KeyContainer,MappedContainer>::operator=
        /// @{
        /// Copy-assigns all underlying containers and functors.
        constexpr segment_tree& operator=(const segment_tree& other)
        noexcept(
            std::is_nothrow_copy_constructible_v<key_container_type> &&
            std::is_nothrow_copy_constructible_v<mapped_container_type> &&
            std::is_nothrow_copy_constructible_v<key_compare> &&
            std::is_nothrow_copy_constructible_v<mapped_sum> &&
            std::is_nothrow_copy_constructible_v<mapped_identity>) {
            m_keys() = other.keys();
            m_nodes() = other.nodes();
            level_offsets_ = other.level_offsets_;
            levels_ = other.levels_;
            comp_ = other.comp_;
            sum_ = other.sum_;
            identity_ = other.identity_;
            return *this;
        }
        /// Move-assigns all underlying containers and functors. @code other@endcode is left in a valid empty state.
        constexpr segment_tree& operator=(segment_tree&& other)
        noexcept(
            std::is_nothrow_move_constructible_v<key_container_type> &&
            std::is_nothrow_move_constructible_v<mapped_container_type> &&
            std::is_nothrow_copy_constructible_v<key_compare> &&
            std::is_nothrow_copy_constructible_v<mapped_sum> &&
            std::is_nothrow_copy_constructible_v<mapped_identity>) {
            m_keys() = std::move(other.keys());
            m_nodes() = std::move(other.nodes());
            level_offsets_ = other.level_offsets_;
            levels_ = other.levels_;
            comp_ = std::move(other.comp_);
            sum_ = std::move(other.sum_);
            identity_ = std::move(other.identity_);
            other.fast_clear();
            return *this;
        }
        /// Clears the underlying containers, inserts key-value pairs @code pairs@endcode,
        /// sorts them if unsorted, then rebuilds the segment tree.
        constexpr segment_tree& operator=(std::initializer_list<value_type> pairs)
        requires (std::swappable<key_type> && std::swappable<mapped_type>) {
            m_keys().clear();
            m_nodes().clear();
            unzip<true>(pairs);
            return *this;
        }
        /// @}
    private:
        template <bool LowerBound>
        constexpr size_type search(const auto& key) const {
            const auto& k = make_search_key<segment_tree>(key);
            if constexpr (LowerBound) {
                return ranges::lower_bound(keys().begin(), keys().begin() + size(), k, comp_) - keys().begin();
            } else {
                return ranges::upper_bound(keys().begin(), keys().begin() + size(), k, comp_) - keys().begin();
            }
        }
        template <bool Throw>
        constexpr size_type exact_search(const auto& key) const {
            const size_type found = search<true>(key);
            if (found == size() || comp_(key, keys()[found]) || comp_(keys()[found], key)) {
                if constexpr (Throw) throw std::out_of_range("key not found in segment_tree.");
                else return size();
            }
            return found;
        }
    public:
        /// Returns a (potentially proxy) reference to the mapped value of @code key@endcode.
        /// The behavior is undefined if
        /// @code key@endcode does not have a @code key_compare()@endcode-equivalent in @code keys()@endcode.
        /// @{
        constexpr reference operator[](const searchable_in<segment_tree> auto& key) {
            return {*this, search<true>(key)};
        }
        constexpr const T& operator[](const searchable_in<segment_tree> auto& key) const {
            return nodes()[search<true>(key)];
        }
        template <detail::multiplies_for<mapped_type, size_type> Mul = std::multiplies<>>
        constexpr T operator[](const searchable_in<segment_tree> auto& key, update_type& update, const Mul& multiplies) const {
            const size_type node = search<true>(key);
            return do_traverse(
                node_traverser(node, node+1), aggregate_with_update_strategy(update, multiplies),
                levels_ - 1, 0, 0, size());
        }
        /// @}
        /// Returns a (potentially proxy) reference to the mapped value of @code key@endcode.
        /// If @code key@endcode does not have a @code key_compare()@endcode-equivalent in @code keys()@endcode,
        /// an exception is thrown with strong exception guarantee.
        /// @throws std::out_of_range:
        /// If @code key@endcode does not have a @code key_compare()@endcode-equivalent in @code keys()@endcode
        /// @{
        constexpr reference at(const searchable_in<segment_tree> auto& key) {
            return {*this, exact_search<true>(key)};
        }
        constexpr const T& at(const searchable_in<segment_tree> auto& key) const {
            return nodes()[exact_search<true>(key)];
        }
        template <detail::multiplies_for<mapped_type, size_type> Mul = std::multiplies<>>
        constexpr T at(const searchable_in<segment_tree> auto& key, update_type& update, const Mul& multiplies) const {
            const size_type node = exact_search<true>(key);
            return do_traverse(
                node_traverser(node, node+1), aggregate_with_update_strategy(update, multiplies),
                levels_ - 1, 0, 0, size());
        }
        /// @}
    private:
        struct const_iterator_func;
        struct iterator_func {
        private:
            segment_tree* base_;
            friend const_iterator_func;
        public:
            constexpr iterator_func() noexcept = default;
            explicit constexpr iterator_func(segment_tree* base) noexcept : base_(base) {}
            reference operator()(size_type node) const noexcept {
                return {*base_, node};
            }
        };
        struct const_iterator_func {
        private:
            const segment_tree* base_;
        public:
            constexpr const_iterator_func() noexcept = default;
            explicit constexpr const_iterator_func(const segment_tree* base) noexcept : base_(base) {}
            constexpr const_iterator_func(iterator_func f) noexcept : base_(f.base_) {}
            const_reference operator()(size_type node) const noexcept {
                return {base_->keys()[node], base_->nodes()[node]};
            }
        };
        using iota_iterator = ranges::iterator_t<ranges::iota_view<size_type, size_type>>;
    public:
        using iterator = transform_iterator<iota_iterator, iterator_func>;
        using const_iterator = transform_iterator<iota_iterator, const_iterator_func>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    private:
        constexpr iterator iter(size_type node) noexcept { return {iota_iterator(node), iterator_func(this)}; }
        constexpr const_iterator iter(size_type node) const noexcept { return {iota_iterator(node), const_iterator_func(this)}; }
    public:
        /// @name Iterator utilities
        /// Returns specified iterators to @code value_type@endcode.
        /// @{
        constexpr auto begin(this auto&& self) noexcept { return self.iter(0); }
        constexpr const_iterator cbegin() const noexcept { return iter(0); }
        constexpr auto end(this auto&& self) noexcept { return self.iter(self.size()); }
        constexpr const_iterator cend() const noexcept { return iter(size()); }
        constexpr auto rbegin(this auto&& self) noexcept { return std::reverse_iterator(self.end()); }
        constexpr const_reverse_iterator crbegin() const noexcept { return std::reverse_iterator(end()); }
        constexpr auto rend(this auto&& self) noexcept { return std::reverse_iterator(self.begin()); }
        constexpr const_reverse_iterator crend() const noexcept { return std::reverse_iterator(end()); }
        /// @}
        /// @brief Checks if the container is in an empty state.
        constexpr bool empty() const noexcept { return size() == 0; }
        /// @brief Returns the number of key-value pairs stored in the container.
        constexpr size_type size() const noexcept { return level_offsets_[1]; }
        /// @brief Returns the theoretical maximum number of key-value pairs that can be stored in a @code segement_tree@endcode.
        constexpr size_type max_size() const noexcept {
            // s: nodes, n: size
            // s = n + ceil(n/2) + ceil(ceil(n/2)/2) + ... + 1
            // = 2n + ceil(lg n) - 1 - popcount(n-1)
            // <= 2n + ceil(lg n) - 1
            return std::min<size_type>(
                keys().max_size(),
                (nodes().max_size() - (std::numeric_limits<typename mapped_container_type::size_type>::digits - 1)) / 2);
        }
    private:
        template <typename U>
        constexpr void do_assign(size_type node, U&& mapped) {
            m_nodes()[node] = std::forward<U>(mapped);
            for (int level = 1; level < levels_; ++level) {
                const bool odd = node % 2;
                const size_type prev = level_offsets_[level - 1] + node;
                node /= 2;
                const size_type current = level_offsets_[level] + node;
                if (odd) {
                    m_nodes()[current] = sum_(m_nodes()[prev], m_nodes()[prev - 1]);
                } else {
                    if (prev + 1 == level_offsets_[level]) {
                        m_nodes()[current] = sum_(m_nodes()[prev], identity_());
                    } else {
                        m_nodes()[current] = sum_(m_nodes()[prev], m_nodes()[prev + 1]);
                    }
                }
            }
        }
    public:
        /// @brief Clear the underlying containers and leave the container in an empty state.
        constexpr void clear() noexcept {
            m_keys().clear();
            m_nodes().clear();
            fast_clear();
        }
        /// @brief Swap the underlying containers and functors with @code other@endcode.
        constexpr void swap(segment_tree& other) noexcept {
            using std::swap;
            swap(keys(), other.keys());
            swap(nodes(), other.nodes());
            swap(level_offsets_, other.level_offsets_);
            swap(levels_, other.levels_);
            swap(comp_, other.comp_);
            swap(sum_, other.sum_);
        }
        /// @brief Returns the number of keys that are @code key_compare()@endcode-equivalent to @code key@endcode.
        constexpr size_type count(const searchable_in<segment_tree> auto& key) const {
            return upper_bound(key) - lower_bound(key);
        }
        /// @brief Returns an iterator to any key-value pair
        /// whose key is @code key_compare()@endcode-equivalent to @code key@endcode.
        /// If such a key does not exist, returns @code end()@endcode.
        constexpr auto find(this auto&& self, const searchable_in<segment_tree> auto& key) {
            return self.iter(self.template exact_search<false>(key));
        }
        /// @brief Checks if any key is @code key_compare()@endcode-equivalent to @code key@endcode.
        constexpr bool contains(const searchable_in<segment_tree> auto& key) const {
            return exact_search<false>(key) != size();
        }
        /// @brief Returns an iterator to the first key-value pair whose key is not less than @code key@endcode.
        /// If such a key does not exist, returns @code end()@endcode.
        constexpr auto lower_bound(this auto&& self, const searchable_in<segment_tree> auto& key) {
            return self.iter(self.template search<true>(key));
        }
        /// @brief Returns an iterator to the first key-value pair whose key is greater than @code key@endcode.
        /// If such a key does not exist, returns @code end()@endcode.
        constexpr auto upper_bound(this auto&& self, const searchable_in<segment_tree> auto& key) {
            return self.iter(self.template search<false>(key));
        }
        /// @brief Returns a pair of iterators to the range of key-value pairs
        /// whose keys are @code key_compare()@endcode-equivalent to @code key@endcode.
        /// @returns [ @code lower_bound(key)@endcode, @code upper_bound(key)@endcode )
        constexpr auto equal_range(this auto&& self, const searchable_in<segment_tree> auto& key) {
            return std::pair(self.lower_bound(key), self.upper_bound(key));
        }
    public:
        /// @brief Stores properties of the node the traverser is visiting.
        ///
        /// Let @f$v@f$ be the subject of the struct, and @code left@endcode and @code right@endcode be its children.
        /// Then @code level@endcode is the height of @f$v@f$ (distance of @f$v@f$ to the nearest leaf),
        /// and @code level_idx@endcode is the number of nodes preceding @f$v@f$ is its level.
        /// @code *_level@endcode and @code *_idx@endcode represent the level and index of
        /// @code left@endcode / @code right@endcode, respectively.
        /// Level @code-1@endcode indicates a non-existent node.
        ///
        /// [ @code begin@endcode, @code end@endcode ) gives the range of indices of mapped values
        /// the sub-tree rooted at @f$v@f$ encompasses,
        /// and the sub-trees rooted at the left and right child of the current node encompass
        /// [ @code begin@endcode, @code div_node@endcode ) and [ @code div_node@endcode, @code end@endcode ), respectively.
        struct node_info {
            int level, left_level, right_level;
            size_type level_idx, left_idx, right_idx, begin, end, div_node;
        };
    private:
        constexpr auto do_traverse(this auto&& self, const auto& traverser, const auto& strategy,
            int level, size_type level_idx, size_type begin, size_type end) {
            using enum interval_endpoint_type;
            node_info info;
            info.level = level;
            info.level_idx = level_idx;
            info.begin = begin;
            info.end = end;
            info.div_node = begin + (std::bit_ceil(end - begin) >> 1);
            info.left_level = level - 1;
            info.left_idx = level_idx * 2;
            info.right_level = std::bit_width(end - info.div_node - 1);
            info.right_idx = ((level_idx * 2 + 1) << (level - info.right_level)) >> 1;
            const node_info* info_ptr = std::addressof(info);
            const segment_tree_traverse_direction dir = traverser(self, info);
            if (dir == stop) return strategy(self, info_ptr, nullptr, nullptr);
            if (level == 0) return strategy(self, nullptr, nullptr, nullptr);
            if constexpr (requires { strategy.prepare(self, *info_ptr); }) strategy.prepare(self, *info_ptr);
            const auto collect_left = [&] {
                return self.do_traverse(traverser, strategy, info.left_level, info.left_idx, info.begin, info.div_node);
            };
            const auto collect_right = [&] {
                return self.do_traverse(traverser, strategy, info.right_level, info.right_idx, info.div_node, info.end);
            };
            if (dir == left) {
                const auto& left = collect_left();
                return strategy(self, info_ptr, std::addressof(left), nullptr);
            }
            if (dir == right) {
                const auto& right = collect_right();
                return strategy(self, info_ptr, nullptr, std::addressof(right));
            }
            if (dir == both) {
                const auto& left = collect_left();
                const auto& right = collect_right();
                return strategy(self, info_ptr, std::addressof(left), std::addressof(right));
            }
            std::unreachable();
        }
        template <typename K1, typename K2>
        struct key_traverser {
            interval<K1, K2> itv_;

            constexpr auto operator()(const segment_tree& base, const node_info& info) const
            -> segment_tree_traverse_direction {
                using enum interval_endpoint_type;
                const interval<const Key&, const Key&> root_itv(
                    {base.keys()[info.begin], closed}, {base.keys()[info.end - 1], closed});
                if (itv_.superset_of(root_itv, base.comp_)) return stop;
                const interval_endpoint<const Key&> div_right(base.keys()[info.div_node], open);
                if (itv_.right.compare(div_right, true, base.comp_) <= 0) return left;
                const interval_endpoint<const Key&> div_left(base.keys()[info.div_node], closed);
                if (itv_.left.compare(div_left, false, base.comp_) <= 0) return right;
                return both;
            }
        };
        struct node_traverser {
            size_type begin_, end_;

            constexpr auto operator()(const segment_tree&, const node_info& info) const noexcept
            -> segment_tree_traverse_direction {
                if (begin_ <= info.begin && end_ >= info.end) return stop;
                if (end_ <= info.div_node) return left;
                if (begin_ >= info.div_node) return right;
                return both;
            }
        };
        struct aggregate_strategy {
            template <typename Info, typename L, typename R>
            constexpr mapped_type operator()(const segment_tree& base, Info info, L left, R right) const noexcept {
                if constexpr (std::is_null_pointer_v<L> && std::is_null_pointer_v<R>) {
                    if constexpr (std::is_null_pointer_v<Info>) return base.identity_();
                    else return base.nodes()[base.level_offsets_[info->level] + info->level_idx];
                }
                else if constexpr (std::is_null_pointer_v<R>) return *left;
                else if constexpr (std::is_null_pointer_v<L>) return *right;
                else return base.sum_(*left, *right);
            }
        };
        template <typename Mul>
        struct aggregate_with_update_strategy {
        private:
            update_type& update_;
            const Mul& multiplies_;
        public:
            constexpr aggregate_with_update_strategy(update_type& update, const Mul& multiplies) noexcept :
                update_(update), multiplies_(multiplies) {}
            constexpr void prepare(const segment_tree& base, const node_info& info) const noexcept {
                const size_type node = base.level_offsets_[info.level] + info.level_idx,
                                left = base.level_offsets_[info.left_level] + info.left_idx,
                                right = base.level_offsets_[info.right_level] + info.right_idx;
                mapped_type& lazy = update_[base.nodes().size() + node];
                mapped_type& lazy_left = update_[base.nodes().size() + left];
                mapped_type& lazy_right = update_[base.nodes().size() + right];
                update_[left] = base.sum_(update_[left], multiplies_(lazy, 1 << info.left_level));
                update_[right] = base.sum_(update_[right], multiplies_(lazy, 1 << info.right_level));
                lazy_left = base.sum_(lazy_left, lazy);
                lazy_right = base.sum_(lazy_right, lazy);
                lazy = base.identity_();
            }
            template <typename Info, typename L, typename R>
            constexpr mapped_type operator()(const segment_tree& base, Info info, L left, R right) const noexcept {
                if constexpr (std::is_null_pointer_v<L> && std::is_null_pointer_v<R>) {
                    if constexpr (std::is_null_pointer_v<Info>) return base.identity_();
                    else {
                        const size_type node = base.level_offsets_[info->level] + info->level_idx;
                        return base.sum_(base.nodes()[node], update_[node]);
                    }
                }
                else if constexpr (std::is_null_pointer_v<L>) return *right;
                else if constexpr (std::is_null_pointer_v<R>) return *left;
                else return base.sum_(*left, *right);
            }
        };
        template <typename Mul>
        struct update_strategy {
        private:
            update_type& update_;
            const mapped_type& diff_;
            const Mul& multiplies_;
        public:
            constexpr update_strategy(update_type& update, const mapped_type& diff, const Mul& multiplies) noexcept :
                update_(update), diff_(diff), multiplies_(multiplies) {}
            template <typename Info, typename L, typename R>
            constexpr int operator()(const segment_tree& base, Info info, L, R) const noexcept {
                if constexpr (std::is_null_pointer_v<Info>) return 0;
                else {
                    const size_type node = base.level_offsets_[info->level] + info->level_idx;
                    if constexpr (std::is_null_pointer_v<L> && std::is_null_pointer_v<R>) {
                        mapped_type& update = update_[node];
                        mapped_type& lazy = update_[base.nodes().size() + node];
                        update = base.sum_(update, multiplies_(diff_, 1 << info->level));
                        lazy = base.sum_(lazy, diff_);
                    } else {
                        const size_type left = base.level_offsets_[info->left_level] + info->left_idx,
                                        right = base.level_offsets_[info->right_level] + info->right_idx;
                        update_[node] = base.sum_(update_[left], update_[right]);
                    }
                    return 0;
                }
            }
        };
    public:
        /// @brief Returns an empty update record.
        constexpr update_type make_update() const
        requires (copy_insertable_into<mapped_container_type>) {
            return update_type(nodes().size() * 2, identity_());
        }
        /// @brief Applies a ranged update to an update record.
        ///
        /// Suppose @f$K@f$ is the set of keys contained in the given interval.
        /// Let @f$U@f$ be the map that identifies @code update@endcode before the call.
        /// @code update@endcode is modified such that the new map @f$U'@f$ satisfies the following:
        /// 1. For @f$k\in K@f$, @f$U'[k]@f$ is equivalent to @code sum()(U[k],diff)@endcode.
        /// 2. For @f$k@f$ in @code keys()@endcode but not in @f$K@f$, @f$U'[k]@f$ is equivalent to @f$U[k]@f$.
        /// @{
        template <searchable_in<segment_tree> K1, searchable_in<segment_tree> K2,
            detail::multiplies_for<mapped_type, size_type> Mul = std::multiplies<>>
        constexpr void make_update(update_type& update,
            const interval<K1, K2>& key_itv, const mapped_type& diff, const Mul& multiplies = {}) const {
            const auto& k1 = make_search_key<segment_tree>(key_itv.left.value);
            const auto& k2 = make_search_key<segment_tree>(key_itv.right.value);
            const interval<decltype(k1), decltype(k2)> k_itv({k1, key_itv.left.type}, {k2, key_itv.right.type});
            traverse(key_traverser(k_itv), update_strategy(update, diff, multiplies));
        }
        template <detail::multiplies_for<mapped_type, size_type> Mul = std::multiplies<>>
        constexpr void make_update(update_type& update,
            const_iterator begin, const_iterator end, const mapped_type& diff, const Mul& multiplies = {}) const {
            traverse(node_traverser(begin - begin(), end - begin()), update_strategy(update, diff, multiplies));
        }
        /// @}
        /// @brief Traverse the tree using @code traverser@endcode, and produce a value using @code strategy@endcode.
        ///
        /// Check concepts @code segment_tree_traverser_for@endcode and @code segment_tree_strategy_for@endcode
        /// for more details.
        constexpr decltype(auto) traverse(
            const segment_tree_traverser_for<segment_tree> auto& traverser,
            const segment_tree_strategy_for<segment_tree> auto& strategy) const {
            if (levels_ <= 0) return strategy(*this, nullptr, nullptr, nullptr);
            else return do_traverse(traverser, strategy, levels_ - 1, 0, 0, size());
        }
        /// @brief Aggregate values in the given key or iterator interval.
        /// @returns A value equivalent to that produced by the following process:
        /// 1. Gather all key-value pairs @f$S@f$ contained in the given interval, in the order defined by @code keys()@endcode.
        /// 2. Compute @f$\sum_{(k,v)\in S} v@f$ using @code sum()@endcode.
        /// @warning For overloads that accept an @code update@endcode,
        /// the returned value is unspecified (but still valid) if @code sum()@endcode is not commutative on @code mapped_type@endcode.
        /// @{
        template <searchable_in<segment_tree> K1, searchable_in<segment_tree> K2>
        constexpr T aggregate(const interval<K1, K2>& key_itv) const {
            const auto& k1 = make_search_key<segment_tree>(key_itv.left.value);
            const auto& k2 = make_search_key<segment_tree>(key_itv.right.value);
            const interval<decltype(k1), decltype(k2)> k_itv({k1, key_itv.left.type}, {k2, key_itv.right.type});
            return traverse(key_traverser(k_itv), aggregate_strategy());
        }
        template <searchable_in<segment_tree> K1, searchable_in<segment_tree> K2,
            detail::multiplies_for<mapped_type, size_type> Mul = std::multiplies<>>
        constexpr T aggregate(const interval<K1, K2>& key_itv, update_type& update, const Mul& multiplies = {}) const {
            const auto& k1 = make_search_key<segment_tree>(key_itv.left.value);
            const auto& k2 = make_search_key<segment_tree>(key_itv.right.value);
            const interval<decltype(k1), decltype(k2)> k_itv({k1, key_itv.left.type}, {k2, key_itv.right.type});
            return traverse(key_traverser(k_itv), aggregate_with_update_strategy(update, multiplies));
        }
        constexpr T aggregate(const_iterator begin, const_iterator end) const {
            return traverse(node_traverser(begin - this->begin(), end - this->begin()), aggregate_strategy());
        }
        template <detail::multiplies_for<mapped_type, size_type> Mul = std::multiplies<>>
        constexpr T aggregate(const_iterator begin, const_iterator end, update_type& update, const Mul& multiplies = {}) const {
            return traverse(
                node_traverser(begin - this->begin(), end - this->begin()),
                aggregate_with_update_strategy(update, multiplies));
        }
        /// @}
        /// @brief Gives the sum of all stored mapped values.
        /// @returns A value equivalent to @code aggregate(begin(), end())@endcode.
        /// @{
        constexpr T total() const {
            return empty() ? identity_() : nodes().back();
        }
        /// @warning The returned value is unspecified (but still valid) if
        /// @code sum()@endcode is not commutative on @code mapped_type@endcode.
        template <detail::multiplies_for<mapped_type, size_type> Mul = std::multiplies<>>
        constexpr T total(const update_type& update, const Mul& multiplies = {}) const {
            return empty() ? identity_() : sum_(
                sum_(nodes().back(),  update[nodes().size() - 1]),
                multiplies(update.back(), size()));
        }
        /// @}
        /// @brief Equality comparison function.
        ///
        /// Delegates to the equality comparison functions of underlying containers.
        friend constexpr bool operator==(const segment_tree& lhs, const segment_tree& rhs) {
            return lhs.keys() == rhs.keys() && lhs.nodes() == rhs.nodes();
        }
        /// @brief Three-way comparison function.
        ///
        /// Delegates to the three-way comparison functions of underlying containers.
        /// @note The function does not use @code key_compare()@endcode of @code lhs@endcode or @code rhs@endcode.
        friend constexpr decltype(auto) operator<=>(const segment_tree& lhs, const segment_tree& rhs)
        requires (container_three_way_comparable<segment_tree>) {
            return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), synth_three_way);
        }
    };
    template <typename KeyContainer, typename MappedContainer,
        typename Key = KeyContainer::value_type, typename T = MappedContainer::value_type,
        typename Compare = std::less<Key>, typename Sum = std::plus<T>, typename Identity = default_construct<T>,
        typename Allocator = detail::void_allocator_t>
    segment_tree(segment_tree_nodes_t, KeyContainer, MappedContainer,
        Compare = Compare(), Sum = Sum(), Identity = Identity(), Allocator = Allocator())
    -> segment_tree<Key, T, Compare, Sum, Identity, KeyContainer, MappedContainer>;
    template <typename KeyContainer, typename MappedContainer, simple_allocator Allocator,
        typename Key = KeyContainer::value_type, typename T = MappedContainer::value_type>
    segment_tree(segment_tree_nodes_t, KeyContainer, MappedContainer, Allocator)
    -> segment_tree<Key, T, std::less<Key>, std::plus<T>, default_construct<T>, KeyContainer, MappedContainer>;

    template <typename KeyContainer, typename MappedContainer,
        typename Key = KeyContainer::value_type, typename T = MappedContainer::value_type,
        typename Compare = std::less<Key>, typename Sum = std::plus<T>, typename Identity = default_construct<T>,
        typename Allocator = detail::void_allocator_t>
    segment_tree(KeyContainer, MappedContainer, Compare = Compare(), Sum = Sum(), Identity = Identity(), Allocator = Allocator())
    -> segment_tree<Key, T, Compare, Sum, Identity, KeyContainer, MappedContainer>;
    template <typename KeyContainer, typename MappedContainer, simple_allocator Allocator,
        typename Key = KeyContainer::value_type, typename T = MappedContainer::value_type>
    segment_tree(KeyContainer, MappedContainer, Allocator)
    -> segment_tree<Key, T, std::less<Key>, std::plus<T>, default_construct<T>, KeyContainer, MappedContainer>;
    template <typename KeyContainer, typename MappedContainer,
        typename Key = KeyContainer::value_type, typename T = MappedContainer::value_type,
        typename Compare = std::less<Key>, typename Sum = std::plus<T>, typename Identity = default_construct<T>,
        typename Allocator = detail::void_allocator_t>
    segment_tree(std::sorted_equivalent_t, KeyContainer, MappedContainer,
        Compare = Compare(), Sum = Sum(), Identity = Identity(), Allocator = Allocator())
    -> segment_tree<Key, T, Compare, Sum, Identity, KeyContainer, MappedContainer>;
    template <typename KeyContainer, typename MappedContainer, simple_allocator Allocator,
        typename Key = KeyContainer::value_type, typename T = MappedContainer::value_type>
    segment_tree(std::sorted_equivalent_t, KeyContainer, MappedContainer, Allocator)
    -> segment_tree<Key, T, std::less<Key>, std::plus<T>, default_construct<T>, KeyContainer, MappedContainer>;

    namespace detail {
        template <typename It, typename Allocator,
            typename PairT = std::iter_value_t<It>,
            typename KeyT = std::remove_cvref_t<std::tuple_element_t<0, PairT>>,
            typename TT = std::remove_cvref_t<std::tuple_element_t<1, PairT>>>
        struct segment_tree_traits {
            using Pair = PairT;
            using Key = KeyT;
            using T = TT;
            using Compare = std::less<Key>;
            using Sum = std::plus<T>;
            using Identity = default_construct<T>;
            using Traits = std::allocator_traits<Allocator>;
            using KeyContainer = std::vector<Key, typename Traits::template rebind_alloc<Key>>;
            using MappedContainer = std::vector<T, typename Traits::template rebind_alloc<T>>;
        };
    }
    template <typename Pairs, typename Allocator = std::allocator<int>,
        typename STT = detail::segment_tree_traits<ranges::iterator_t<Pairs>, Allocator>,
        typename Compare = STT::Compare, typename Sum = STT::Sum, typename Identity = STT::Identity>
    segment_tree(std::from_range_t, Pairs&&, Compare = Compare(), Sum = Sum(), Identity = Identity(), Allocator = Allocator())
    -> segment_tree<typename STT::Key, typename STT::T, Compare, Sum, Identity,
        typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename Pairs, simple_allocator Allocator,
        typename STT = detail::segment_tree_traits<ranges::iterator_t<Pairs>, Allocator>>
    segment_tree(std::from_range_t, Pairs&&, Allocator)
    -> segment_tree<typename STT::Key, typename STT::T,
        typename STT::Compare, typename STT::Sum, typename STT::Identity, typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename Pairs, typename Allocator = std::allocator<int>,
        typename STT = detail::segment_tree_traits<ranges::iterator_t<Pairs>, Allocator>,
        typename Compare = STT::Compare, typename Sum = STT::Sum, typename Identity = STT::Identity>
    segment_tree(std::sorted_equivalent_t, std::from_range_t, Pairs&&,
        Compare = Compare(), Sum = Sum(), Identity = Identity(), Allocator = Allocator())
    -> segment_tree<typename STT::Key, typename STT::T, Compare, Sum, Identity,
        typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename Pairs, simple_allocator Allocator,
        typename STT = detail::segment_tree_traits<ranges::iterator_t<Pairs>, Allocator>>
    segment_tree(std::sorted_equivalent_t, std::from_range_t, Pairs&&, Allocator)
    -> segment_tree<typename STT::Key, typename STT::T,
        typename STT::Compare, typename STT::Sum, typename STT::Identity, typename STT::KeyContainer, typename STT::MappedContainer>;

    template <typename PairsIt, typename PairsSent, typename Allocator = std::allocator<int>,
        typename STT = detail::segment_tree_traits<PairsIt, Allocator>,
        typename Compare = STT::Compare, typename Sum = STT::Sum, typename Identity = STT::Identity>
    segment_tree(PairsIt, PairsSent, Compare = Compare(), Sum = Sum(), Identity = Identity(), Allocator = Allocator())
    -> segment_tree<typename STT::Key, typename STT::T, Compare, Sum, Identity,
        typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename PairsIt, typename PairsSent, simple_allocator Allocator,
        typename STT = detail::segment_tree_traits<PairsIt, Allocator>>
    segment_tree(PairsIt, PairsSent, Allocator)
    -> segment_tree<typename STT::Key, typename STT::T,
        typename STT::Compare, typename STT::Sum, typename STT::Identity, typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename PairsIt, typename PairsSent, typename Allocator = std::allocator<int>,
        typename STT = detail::segment_tree_traits<PairsIt, Allocator>,
        typename Compare = STT::Compare, typename Sum = STT::Sum, typename Identity = STT::Identity>
    segment_tree(std::sorted_equivalent_t, PairsIt, PairsSent, Compare = Compare(), Sum = Sum(), Allocator = Allocator())
    -> segment_tree<typename STT::Key, typename STT::T, Compare, Sum, Identity,
        typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename PairsIt, typename PairsSent, simple_allocator Allocator,
        typename STT = detail::segment_tree_traits<PairsIt, Allocator>>
    segment_tree(std::sorted_equivalent_t, PairsIt, PairsSent, Allocator)
    -> segment_tree<typename STT::Key, typename STT::T,
        typename STT::Compare, typename STT::Sum, typename STT::Identity, typename STT::KeyContainer, typename STT::MappedContainer>;

    template <typename Key, typename T, typename Allocator = std::allocator<int>,
        typename STT = detail::segment_tree_traits<const std::pair<Key, T>*, Allocator>,
        typename Compare = STT::Compare, typename Sum = STT::Sum, typename Identity = STT::Identity>
    segment_tree(std::initializer_list<std::pair<Key, T>>, Compare = Compare(), Sum = Sum(), Allocator = Allocator())
    -> segment_tree<typename STT::Key, typename STT::T, Compare, Sum, Identity,
        typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename Key, typename T, simple_allocator Allocator,
        typename STT = detail::segment_tree_traits<const std::pair<Key, T>*, Allocator>>
    segment_tree(std::initializer_list<std::pair<Key, T>>, Allocator)
    -> segment_tree<typename STT::Key, typename STT::T,
        typename STT::Compare, typename STT::Sum, typename STT::Identity, typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename Key, typename T, typename Allocator = std::allocator<int>,
        typename STT = detail::segment_tree_traits<const std::pair<Key, T>*, Allocator>,
        typename Compare = STT::Compare, typename Sum = STT::Sum, typename Identity = STT::Identity>
    segment_tree(std::sorted_equivalent_t,
        std::initializer_list<std::pair<Key, T>>, Compare = Compare(), Sum = Sum(), Identity = Identity(), Allocator = Allocator())
    -> segment_tree<typename STT::Key, typename STT::T, Compare, Sum, Identity,
        typename STT::KeyContainer, typename STT::MappedContainer>;
    template <typename Key, typename T, simple_allocator Allocator,
        typename STT = detail::segment_tree_traits<const std::pair<Key, T>*, Allocator>>
    segment_tree(std::sorted_equivalent_t, std::initializer_list<std::pair<Key, T>>, Allocator)
    -> segment_tree<typename STT::Key, typename STT::T,
        typename STT::Compare, typename STT::Sum, typename STT::Identity, typename STT::KeyContainer, typename STT::MappedContainer>;
}
template <typename Alloc, typename... Args>
struct std::uses_allocator<utils::segment_tree<Args...>, Alloc> : std::conjunction<
    std::uses_allocator<typename utils::segment_tree<Args...>::key_container_type, Alloc>,
    std::uses_allocator<typename utils::segment_tree<Args...>::mapped_container_type, Alloc>> {};