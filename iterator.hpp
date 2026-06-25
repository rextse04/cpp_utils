#pragma once
#include <type_traits>
#include <concepts>
#include <iterator>
#include <utility>

/**
 * @file
 * @brief Additional iterator utilities and adaptors not provided by the standard library.
 * The purpose of this header is not to replace <ranges>, but rather to provide parallel utilities when standalone
 * iterators are needed, e.g. when writing containers and iterators of desired views depend on their parents.
 */

namespace utils {
    template <typename Iter>
    struct iterator_category {
        using type = std::iterator_traits<Iter>::iterator_category;
    };
    template <typename Iter>
    using iterator_category_t = iterator_category<Iter>::type;

    template <typename T>
    class pointer_iterator {
    public:
        using iterator_type = T*;
        using value_type = std::remove_cv_t<T>;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::contiguous_iterator_tag;
        using iterator_concept = std::contiguous_iterator_tag;
    private:
        pointer ptr_;
    public:
        constexpr pointer_iterator() noexcept : ptr_(nullptr) {}
        constexpr pointer_iterator(pointer ptr) noexcept : ptr_(ptr) {}

        constexpr pointer get() const noexcept { return ptr_; }

        constexpr reference operator*() const noexcept { return *ptr_; }
        constexpr pointer operator->() const noexcept { return ptr_; }
        constexpr reference operator[](difference_type n) const noexcept { return ptr_[n]; }
        constexpr pointer_iterator& operator++() noexcept { ++ptr_; return *this; }
        constexpr pointer_iterator operator++(int) noexcept { return ptr_++; }
        constexpr pointer_iterator& operator--() noexcept { --ptr_; return *this; }
        constexpr pointer_iterator operator--(int) noexcept { return ptr_--; }
        constexpr pointer_iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        constexpr pointer_iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }
        friend constexpr pointer_iterator operator+(pointer_iterator it, difference_type n) noexcept { return it.ptr_ + n; }
        friend constexpr pointer_iterator operator+(difference_type n, pointer_iterator it) noexcept { return it.ptr_ + n; }
        friend constexpr pointer_iterator operator-(pointer_iterator it, difference_type n) noexcept { return it.ptr_ - n; }
        friend constexpr difference_type operator-(pointer_iterator a, pointer_iterator b) noexcept { return a.ptr_ - b.ptr_; }
        friend constexpr auto operator==(pointer_iterator a, pointer_iterator b) noexcept { return a.ptr_ == b.ptr_; }
        friend constexpr auto operator<=>(pointer_iterator a, pointer_iterator b) noexcept { return a.ptr_ <=> b.ptr_; }
    };
    template <typename Iter>
    using iterator_wrapper = std::conditional_t<std::is_pointer_v<Iter>, pointer_iterator<std::remove_pointer_t<Iter>>, Iter>;

    namespace detail {
        template <typename T>
        concept can_reference = requires { typename std::type_identity_t<T&>; };
        template <typename F, typename... Args>
        concept invoke_capturable = requires (F f, Args... args) {
            { f(std::forward<Args>(args)...) } -> can_reference;
        };
    }
    template <typename Iter, typename F>
    requires (
        std::input_or_output_iterator<Iter> &&
        detail::invoke_capturable<F, std::iter_reference_t<Iter>> && std::move_constructible<F>)
    class transform_iterator : public iterator_wrapper<Iter> {
    private:
        using parent = iterator_wrapper<Iter>;
        [[no_unique_address]] F func_;
    public:
        using iterator_type = Iter;
        using iterator_category = std::common_type_t<iterator_category_t<Iter>, std::random_access_iterator_tag>;
        using difference_type = std::iter_difference_t<Iter>;

        constexpr transform_iterator() = default;
        constexpr transform_iterator(parent it, F func) : parent(std::move(it)), func_(std::move(func)) {}
        template <typename U, typename G>
        requires (std::convertible_to<const U&, Iter> && std::convertible_to<const G&, F>)
        constexpr transform_iterator(const transform_iterator<U, G>& other) : parent(other), func_(other.transformer()) {}

        constexpr const F& transformer() const noexcept { return func_; }

        constexpr decltype(auto) operator*() const {
            return func_(**static_cast<const parent*>(this));
        }
        constexpr decltype(auto) operator->() const = delete;
        constexpr decltype(auto) operator[](difference_type n) const {
            return func_(*(*static_cast<const parent*>(this) + n));
        }
        constexpr transform_iterator& operator++() {
            ++static_cast<parent&>(*this);
            return *this;
        }
        constexpr transform_iterator operator++(int) {
            return static_cast<parent&>(*this)++;
        }
        constexpr transform_iterator operator++(int)
        requires (std::incrementable<Iter> && std::is_copy_constructible_v<F>) {
            return {static_cast<parent&>(*this)++, func_};
        }
        constexpr transform_iterator& operator--() {
            --static_cast<parent&>(*this);
            return *this;
        }
        constexpr transform_iterator operator--(int)
        requires (std::is_copy_constructible_v<F>) {
            return {static_cast<parent&>(*this)--, func_};
        }
        constexpr transform_iterator& operator+=(difference_type n) {
            static_cast<parent&>(*this) += n;
            return *this;
        }
        constexpr transform_iterator& operator-=(difference_type n) {
            static_cast<parent&>(*this) -= n;
            return *this;
        }
        friend constexpr transform_iterator operator+(const transform_iterator& it, difference_type n)
        requires (std::is_copy_constructible_v<F>) {
            return {static_cast<const parent&>(it) + n, it.func_};
        }
        friend constexpr transform_iterator operator+(difference_type n, const transform_iterator& it)
        requires (std::is_copy_constructible_v<F>) {
            return {static_cast<const parent&>(it) + n, it.func_};
        }
        friend constexpr transform_iterator operator-(const transform_iterator& it, difference_type n)
        requires (std::is_copy_constructible_v<F>) {
            return {static_cast<const parent&>(it) - n, it.func_};
        }
    };

    namespace detail {
        template <typename T>
        concept move_incrementable = requires (T t) {
            { t++ } -> std::move_constructible;
        };
    }
    template <typename Iter, typename Sent, typename P>
    requires (
        detail::move_incrementable<Iter> && std::sentinel_for<Sent, Iter> &&
        std::predicate<P, std::iter_reference_t<Iter>> && std::move_constructible<P>)
    class filter_iterator : public iterator_wrapper<Iter> {
    private:
        using parent = iterator_wrapper<Iter>;
        Sent st_;
        [[no_unique_address]] P pred_;
    public:
        using iterator_type = Iter;
        using iterator_category = std::common_type_t<iterator_category_t<Iter>, std::forward_iterator_tag>;
        using difference_type = std::iter_difference_t<Iter>;

        constexpr filter_iterator() = default;
        constexpr filter_iterator(Iter it, Sent st, P pred) : parent(std::move(it)), st_(std::move(st)), pred_(std::move(pred)) {}
        template <typename U, typename S, typename Q>
        requires (std::convertible_to<const U&, Iter> && std::convertible_to<const S&, Sent> && std::convertible_to<const P&, Q>)
        constexpr filter_iterator(const filter_iterator<U, S, Q>& other) : parent(other), st_(other.sentinel()), pred_(other.pred_) {}

        constexpr Sent sentinel() const { return st_; }
        constexpr const P& filter() const noexcept { return pred_; }

        constexpr decltype(auto) operator*() const {
            return **static_cast<const parent*>(this);
        }
        constexpr decltype(auto) operator->() const {
            return static_cast<const parent*>(this)->operator->();
        }
        constexpr decltype(auto) operator[](difference_type n) const = delete;
        constexpr filter_iterator& operator++() {
            do { ++static_cast<parent&>(*this); }
            while (*static_cast<Iter*>(this) != st_ && !pred_(**this));
            return *this;
        }
        constexpr filter_iterator operator++(int) {
            auto out = static_cast<parent&>(*this)++;
            ++*this;
            return out;
        }
        constexpr filter_iterator operator++(int)
        requires (std::incrementable<Iter> && std::is_copy_constructible_v<Sent> && std::is_copy_constructible_v<P>) {
            filter_iterator out = *this;
            ++*this;
            return out;
        }
        constexpr filter_iterator& operator--() = delete;
        constexpr filter_iterator operator--(int) = delete;
        constexpr filter_iterator& operator+=(difference_type n) = delete;
        constexpr filter_iterator& operator-=(difference_type n) = delete;
    };
}