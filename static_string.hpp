#pragma once
#include <string_view>
#include <stdexcept>
#include <algorithm>
#include <exception>
#include <format>
#include "type.hpp"
#include "integer.hpp"
#include "ranges.hpp"
#include "swap.hpp"

namespace utils {
    struct static_string_tag;
    namespace detail {
        template <typename CharT, typename Traits>
        struct string_interface {
            using tag = static_string_tag;
            using traits_type = Traits;
            using value_type = CharT;
            using pointer = CharT*;
            using const_pointer = const CharT*;
            using reference = CharT&;
            using const_reference = const CharT&;
            using size_type = integer_alias::size_t;
            using difference_type = std::ptrdiff_t;
            using base_string_type = std::basic_string<CharT, traits_type>;
            using base_string_view_type = std::basic_string_view<CharT, traits_type>;
            static constexpr size_type npos = static_cast<size_type>(-1);

            constexpr auto begin(this const auto&& self) noexcept { return self.data(); }
            constexpr auto end(this const auto&& self) noexcept { return self.begin() + self.size(); }
            constexpr auto rbegin(this const auto&& self) noexcept { return std::reverse_iterator(self.end()); }
            constexpr auto rend(this const auto&& self) noexcept { return std::reverse_iterator(self.begin()); }
            constexpr decltype(auto) operator[](this auto&& self, size_type i) noexcept { return *(self.begin() + i); }
            constexpr decltype(auto) at(this auto&& self, size_type i) noexcept {
                if (i >= self.size()) throw std::out_of_range("i >= size");
                return self[i];
            }
            constexpr decltype(auto) front(this auto&& self) noexcept { return self[0]; }
            constexpr decltype(auto) back(this auto&& self) noexcept { return self[self.size() - 1]; }
            constexpr size_type length(this const auto& self) noexcept { return self.size(); }
            constexpr bool empty(this const auto& self) noexcept { return self.size() == 0; }
            constexpr size_type copy(this const auto& self, CharT* dest, size_type count, size_type pos = 0) {
                if (pos > self.size()) throw std::out_of_range("pos > size");
                Traits::copy(dest, self.data() + pos, count);
                return count;
            }
            constexpr int compare(this const auto& self, base_string_view_type v) noexcept {
                return base_string_view_type(self).compare(v);
            }
            constexpr int compare(this const auto& self, size_type pos1, size_type count1, base_string_view_type v) {
                return base_string_view_type(self).compare(pos1, count1, v);
            }
            constexpr int compare(this const auto& self, size_type pos1, size_type count1,
                base_string_view_type v, size_type pos2, size_type count2) {
                return base_string_view_type(self).compare(pos1, count1, v, pos2, count2);
            }
            constexpr int compare(this const auto& self, const CharT* s) {
                return base_string_view_type(self).compare(s);
            }
            constexpr int compare(this const auto& self, size_type pos1, size_type count1, const CharT* s) {
                return base_string_view_type(self).compare(pos1, count1, s);
            }
            constexpr int compare(this const auto& self,
                size_type pos1, size_type count1, const CharT* s, size_type count2) {
                return base_string_view_type(self).compare(pos1, count1, s, count2);
            }
            constexpr bool starts_with(this const auto& self, base_string_view_type sv) noexcept {
                return base_string_view_type(self).starts_with(sv);
            }
            constexpr bool starts_with(this const auto& self, CharT ch) noexcept {
                return base_string_view_type(self).starts_with(ch);
            }
            constexpr bool starts_with(this const auto& self, const CharT* s) {
                return base_string_view_type(self).starts_with(s);
            }
            constexpr bool ends_with(this const auto& self, base_string_view_type sv) noexcept {
                return base_string_view_type(self).ends_with(sv);
            }
            constexpr bool ends_with(this const auto& self, CharT ch) noexcept {
                return base_string_view_type(self).ends_with(ch);
            }
            constexpr bool ends_with(this const auto& self, const CharT* s) {
                return base_string_view_type(self).ends_with(s);
            }
            constexpr bool contains(this const auto& self, base_string_view_type sv) noexcept {
                return base_string_view_type(self).contains(sv);
            }
            constexpr bool contains(this const auto& self, CharT c) noexcept {
                return base_string_view_type(self).contains(c);
            }
            constexpr bool contains(this const auto& self, const CharT* s) {
                return base_string_view_type(self).contains(s);
            }
            constexpr size_type find(this const auto& self, base_string_view_type v, size_type pos = 0) noexcept {
                return base_string_view_type(self).find(v, pos);
            }
            constexpr size_type find(this const auto& self, CharT ch, size_type pos = 0) noexcept {
                return base_string_view_type(self).find(ch, pos);
            }
            constexpr size_type find(this const auto& self, const CharT* s, size_type pos, size_type count) {
                return base_string_view_type(self).find(s, pos, count);
            }
            constexpr size_type find(this const auto& self, const CharT* s, size_type pos = 0) {
                return base_string_view_type(self).find(s, pos);
            }
            constexpr size_type rfind(this const auto& self, base_string_view_type v, size_type pos = 0) noexcept {
                return base_string_view_type(self).rfind(v, pos);
            }
            constexpr size_type rfind(this const auto& self, CharT ch, size_type pos = 0) noexcept {
                return base_string_view_type(self).rfind(ch, pos);
            }
            constexpr size_type rfind(this const auto& self, const CharT* s, size_type pos, size_type count) {
                return base_string_view_type(self).rfind(s, pos, count);
            }
            constexpr size_type rfind(this const auto& self, const CharT* s, size_type pos = 0) {
                return base_string_view_type(self).rfind(s, pos);
            }
            constexpr size_type find_first_of(this const auto& self,
                base_string_view_type v, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_first_of(v, pos);
            }
            constexpr size_type find_first_of(this const auto& self, CharT ch, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_first_of(ch, pos);
            }
            constexpr size_type find_first_of(this const auto& self, const CharT* s, size_type pos, size_type count) {
                return base_string_view_type(self).find_first_of(s, pos, count);
            }
            constexpr size_type find_first_of(this const auto& self, const CharT* s, size_type pos = 0) {
                return base_string_view_type(self).find_first_of(s, pos);
            }
            constexpr size_type find_last_of(this const auto& self,
                base_string_view_type v, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_last_of(v, pos);
            }
            constexpr size_type find_last_of(this const auto& self, CharT ch, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_last_of(ch, pos);
            }
            constexpr size_type find_last_of(this const auto& self, const CharT* s, size_type pos, size_type count) {
                return base_string_view_type(self).find_last_of(s, pos, count);
            }
            constexpr size_type find_last_of(this const auto& self, const CharT* s, size_type pos = 0) {
                return base_string_view_type(self).find_last_of(s, pos);
            }
            constexpr size_type find_first_not_of(this const auto& self,
                base_string_view_type v, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_first_not_of(v, pos);
            }
            constexpr size_type find_first_not_of(this const auto& self, CharT ch, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_first_not_of(ch, pos);
            }
            constexpr size_type find_first_not_of(this const auto& self,
                const CharT* s, size_type pos, size_type count) {
                return base_string_view_type(self).find_first_not_of(s, pos, count);
            }
            constexpr size_type find_first_not_of(this const auto& self, const CharT* s, size_type pos = 0) {
                return base_string_view_type(self).find_first_not_of(s, pos);
            }
            constexpr size_type find_last_not_of(this const auto& self,
                base_string_view_type v, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_last_not_of(v, pos);
            }
            constexpr size_type find_last_not_of(this const auto& self, CharT ch, size_type pos = 0) noexcept {
                return base_string_view_type(self).find_last_not_of(ch, pos);
            }
            constexpr size_type find_last_not_of(this const auto& self,
                const CharT* s, size_type pos, size_type count) {
                return base_string_view_type(self).find_last_not_of(s, pos, count);
            }
            constexpr size_type find_last_not_of(this const auto& self, const CharT* s, size_type pos = 0) {
                return base_string_view_type(self).find_last_not_of(s, pos);
            }
            constexpr operator base_string_view_type(this const auto& self) noexcept {
                return base_string_view_type(self.data(), self.size());
            }
            constexpr bool operator==(this const auto& self, base_string_view_type other) noexcept {
                return base_string_view_type(self) == other;
            }
            constexpr auto operator<=>(this const auto& self, base_string_view_type other) noexcept {
                return base_string_view_type(self) <=> other;
            }
        };
    }

    /// Identical interface to ```std::basic_string_view```,
    /// except that this class can only be constructed at compile time,
    /// thus ensuring that it refers to a "static string".
    /// @remark ```data_``` and ```size_``` are public to make this a structural type,
    /// thus allowing it to be passed as a template parameter.
    /// However, as pointers to string literals cannot be part of template parameters,
    /// ```basic_static_string``` may be a better choice in this case.
    template <typename CharT, typename Traits = std::char_traits<CharT>>
    struct basic_static_string_view : detail::string_interface<CharT, Traits> {
    private:
        using parent = detail::string_interface<CharT, Traits>;
        using typename parent::size_type;
        using typename parent::base_string_view_type;
        using parent::npos;
    public:
        using const_iterator = const CharT*;
        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;

        const CharT* const data_;
        const size_type size_;

        consteval basic_static_string_view(const CharT* s, size_type len) : data_(s), size_(len) {
            (void) (s + integer_alias::ptrdiff_t(len)); // no UB at compile time, so this checks len
        }
        consteval basic_static_string_view(const CharT* s) : data_(s), size_(Traits::length(s)) {}

        constexpr const CharT* data() const noexcept { return data_; }
        constexpr size_type size() const noexcept { return size_; }
        constexpr size_type max_size(this const auto&) noexcept { return npos; }
        constexpr base_string_view_type substr(size_type pos = 0, size_type count = npos) const {
            if (pos > size()) throw std::out_of_range("pos > size()");
            return {data() + pos, std::min(count, size() - pos)};
        }
    };
    using static_string_view = basic_static_string_view<char>;
    using wstatic_string_view = basic_static_string_view<wchar_t>;
    using u8static_string_view = basic_static_string_view<char8_t>;
    using u16static_string_view = basic_static_string_view<char16_t>;
    using u32static_string_view = basic_static_string_view<char32_t>;

    /// This class represents an owning string defined at compile time.
    /// It implements a subset of the interface of ```std::basic_string```,
    /// excluding members that would require the string to be resizable.
    /// In addition, all modifying operations are consteval,
    /// maintaining the variant that the class is immutable at runtime.
    /// @note The underlying character sequence (```data```) is null-terminated.
    template <typename CharT, std::size_t N, typename Traits = std::char_traits<CharT>>
    struct basic_static_string : detail::string_interface<CharT, Traits> {
    private:
        using parent = detail::string_interface<CharT, Traits>;
        using typename parent::size_type;
        using parent::npos;

        consteval basic_static_string() = default;
        template <typename CharU, std::size_t M, typename TraitsU>
        friend struct basic_static_string;
    public:
        using iterator = CharT*;
        using const_iterator = const CharT*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using string_view_type = basic_static_string_view<CharT, Traits>;
        template <std::size_t M>
        using resize_t = basic_static_string<CharT, M, Traits>;

        CharT data_[N];
        size_type size_;

        consteval basic_static_string(const CharT (&s)[N]) { assign(s); }
        template <typename...>
        consteval basic_static_string(const CharT* s) { assign(s); }
        consteval basic_static_string(const CharT* s, size_type len) { assign(s, len); }
        consteval basic_static_string(size_type count, const CharT c) { assign(count, c); }
        consteval basic_static_string(std::nullptr_t) = delete;
        consteval basic_static_string(string_view_type sv, size_type pos = 0, size_type count = npos) {
            assign(sv, pos, count);
        }
        template <typename I, typename S>
        requires (container_compatible_iterator<I, CharT> && std::sentinel_for<S, I>)
        consteval basic_static_string(I begin, S end) { assign(begin, end); }
        consteval basic_static_string(std::from_range_t, container_compatible_range<CharT> auto&& rng) {
            assign_range(rng);
        }
        consteval basic_static_string(std::initializer_list<CharT> ilist) { assign(ilist); }

        consteval basic_static_string& assign(const CharT (&s)[N]) {
            if (s[N-1] != CharT{}) throw std::invalid_argument("s is not null-terminated");
            Traits::copy(data_, s, N);
            size_ = N-1;
            return *this;
        }
        template <typename...>
        consteval basic_static_string& assign(const CharT* s) {
            assign(s, Traits::length(s));
            return *this;
        }
        consteval basic_static_string& assign(const CharT* s, size_type len) {
            if (len > max_size()) throw std::length_error("len > max_size()");
            Traits::copy(data_, size_ = len, s);
            Traits::assign(data_ + len, N - len, CharT{});
            return *this;
        }
        consteval basic_static_string& assign(size_type count, const CharT c) {
            if (count > max_size()) throw std::length_error("len > max_size()");
            Traits::assign(data_, size_ = count, c);
            Traits::assign(data_ + count, N - count, CharT{});
            return *this;
        }
        consteval basic_static_string& assign(string_view_type sv, size_type pos = 0, size_type count = npos) {
            assign(sv.data() + pos, std::min(count, sv.size() - pos));
            return *this;
        }
        template <typename I, typename S>
        requires (container_compatible_iterator<I, CharT> && std::sentinel_for<S, I>)
        consteval basic_static_string& assign(I begin, S end) {
            size_ = 0;
            for (; begin != end; ++begin) {
                if (size_ >= max_size()) throw std::length_error("{begin, end} larger than max_size()");
                Traits::assign(data_[+size_++], *begin);
            }
            Traits::assign(data_ + size_, N - size_, CharT{});
            return *this;
        }
        consteval basic_static_string& assign(std::initializer_list<CharT> ilist) {
            if (ilist.size() > max_size()) throw std::length_error("ilist.size() > max_size()");
            Traits::copy(data_, ilist.begin(), size_ = ilist.size());
            Traits::assign(data_ + ilist.size(), N - ilist.size(), CharT{});
            return *this;
        }
        consteval basic_static_string& assign_range(container_compatible_range<CharT> auto&& rng) {
            return assign(ranges::begin(rng), ranges::end(rng));
        }

        consteval iterator begin() { return data_; }
        constexpr const_iterator begin() const noexcept { return data_; }

        constexpr CharT* data() noexcept { return data_; }
        constexpr const CharT* data() const noexcept { return data_; }
        constexpr const CharT* c_str() const noexcept { return data_; }
        constexpr size_type size() const noexcept { return size_; }
        static constexpr size_type max_size() noexcept { return N - 1; }
        static consteval void reserve(size_type new_cap) {
            if (new_cap > max_size()) throw std::bad_alloc();
        }
        static constexpr size_type capacity() noexcept { return N - 1; }
        static consteval void shrink_to_fit() noexcept {}

        consteval void clear() { Traits::assign(data_[+(size_ = 0)], CharT{}); }
        consteval basic_static_string& insert(size_type index, size_type count, CharT c) {
            if (index > size()) throw std::out_of_range("index > size()");
            insert(begin() + index, count, c);
            return *this;
        }
        consteval basic_static_string& insert(size_type index, const CharT* s) {
            return insert(index, s, Traits::length(s));
        }
        consteval basic_static_string& insert(size_type index, const CharT* s, size_type count) {
            if (index > size()) throw std::out_of_range("index > size()");
            if (count > max_size() - size_) throw std::length_error("count > max_size() - size_");
            Traits::move(begin() + index + count, begin() + index, size_ - index);
            Traits::copy(begin() + index, s, count);
            Traits::assign(data_[+(size_ += count)], CharT{});
            return *this;
        }
        consteval iterator insert(const_iterator pos, const CharT c) {
            return insert(pos, 1, c);
        }
        consteval iterator insert(const_iterator pos, size_type count, CharT c) {
            if (count > max_size() - size_) throw std::length_error("count > max_size() - size_");
            const auto it = const_cast<iterator>(pos);
            Traits::move(it + count, pos, data_ + size_ + 1 - pos);
            return Traits::assign(it, count, c);
        }
        consteval basic_static_string& insert(size_type index, string_view_type sv,
            size_type pos = 0, size_type count = npos) {
            sv = sv.substr(pos, count);
            return insert(index, sv.data(), sv.size());
        }
        template <typename I, typename S>
        requires (container_compatible_iterator<I, CharT> && std::sentinel_for<S, I>)
        consteval iterator insert(const_iterator pos, I begin, S end) {
            CharT src[max_size()];
            size_type src_size = 0;
            const size_type max_src_size = max_size() - size_;
            for (; begin != end; ++begin) {
                CharT* const src_ptr = src + ++src_size;
                if (src_size > max_src_size)
                    throw std::length_error("{begin, end} larger than max_size() - size_");
                Traits::assign(*src_ptr, *begin);
            }
            const auto it = const_cast<iterator>(pos);
            Traits::move(it + src_size, pos, data_ + size_ + 1 - pos);
            return Traits::copy(it, src, src_size);
        }
        consteval iterator insert(const_iterator pos, std::initializer_list<CharT> ilist) {
            return insert(pos, ilist.begin(), ilist.end());
        }
        consteval iterator insert_range(const_iterator pos, container_compatible_range<CharT> auto&& rng) {
            return insert(pos, ranges::begin(rng), ranges::end(rng));
        }
        consteval basic_static_string& erase(size_type index = 0, size_type count = npos) {
            if (index > size()) throw std::out_of_range("index > size()");
            erase(begin() + index, begin() + index + std::min(count, size_ - index));
            return *this;
        }
        consteval iterator erase(const_iterator pos) {
            return erase(pos, pos+1);
        }
        consteval iterator erase(const_iterator first, const_iterator last) {
            const auto it = const_cast<iterator>(first);
            Traits::move(it, last, data_ + size_ + 1 - last);
            size_ -= last - first;
            return it;
        }
        consteval void push_back(CharT c) {
            if (size_ >= max_size()) throw std::length_error("size_ >= max_size()");
            Traits::assign(data_[+size_], c);
            Traits::assign(data_[+++size_], CharT{});
        }
        consteval void pop_back() { data_[+--size_] = CharT{}; }
        consteval basic_static_string& append(size_type count, CharT c) { return insert(size(), count, c); }
        consteval basic_static_string& append(const CharT* s, size_type count) { return insert(size(), s, count); }
        consteval basic_static_string& append(const CharT* s) { return insert(size(), s); }
        consteval basic_static_string& append(string_view_type sv, size_type pos = 0, size_type count = npos) {
            return insert(size(), sv, pos, count);
        }
        template <typename I, typename S>
        requires (container_compatible_iterator<I, CharT> && std::sentinel_for<S, I>)
        consteval basic_static_string& append(I begin, S end) {
            insert(this->end(), begin, end);
            return *this;
        }
        consteval basic_static_string& append(std::initializer_list<CharT> ilist) {
            insert(this->end(), ilist.begin(), ilist.end());
            return *this;
        }
        consteval basic_static_string& append_range(container_compatible_range<CharT> auto&& rng) {
            return append(ranges::begin(rng), ranges::end(rng));
        }
        consteval basic_static_string& operator+=(CharT c) { return append(1, c); }
        consteval basic_static_string& operator+=(const CharT* s) { return append(s); }
        consteval basic_static_string& operator+=(string_view_type sv) { return append(sv); }
        consteval basic_static_string& operator+=(std::initializer_list<CharT> ilist) { return append(ilist); }
        template <std::size_t M>
        consteval auto operator+(const basic_static_string<CharT, M, Traits>& str) {
            basic_static_string<CharT, N+M, Traits> out;
            Traits::copy(out.data(), data_, size_);
            Traits::copy(out.data() + size_, str.data(), str.size());
            Traits::assign(out.data() + size_ + str.size(), N + M - size_ - str.size(), CharT{});
            return out;
        }
        consteval auto operator+(CharT c) {
            basic_static_string<CharT, N+1, Traits> out;
            Traits::copy(out.data(), data_, size_);
            Traits::assign(*(out.data() + size_), c);
            Traits::assign(out.data() + size_ + 1, N - size_ - 1, CharT{});
            return out;
        }
        consteval basic_static_string& replace(size_type pos, size_type count, string_view_type sv,
            size_type pos2 = 0, size_type count2 = npos) {
            sv = sv.substr(pos2, count2);
            return replace(pos, count, sv.data(), sv.size());
        }
        consteval basic_static_string& replace(const_iterator first, const_iterator last, string_view_type sv,
            size_type pos2 = 0, size_type count2 = npos) {
            sv = sv.substr(pos2, count2);
            return replace(first, last, sv.data(), sv.size());
        }
        consteval basic_static_string& replace(size_type pos, size_type count, const CharT* s, size_type count2) {
            if (pos > size()) throw std::out_of_range("pos > size()");
            if (count > size() - pos) throw std::out_of_range("count > size() - pos");
            return replace(begin() + pos, begin() + pos + count, s, count2);
        }
        consteval basic_static_string& replace(
            const_iterator first, const_iterator last, const CharT* s, size_type count2) {
            const size_type count = last - first;
            if (count2 > max_size() - size() + count) throw std::length_error("count2 > max_size() - size_ + count");
            const auto it = const_cast<iterator>(first);
            Traits::move(it + count2, last, data_ + size_ + 1 - last);
            Traits::copy(it, s, count2);
            size_ += count2 - count;
            return *this;
        }
        consteval basic_static_string& replace(size_type pos, size_type count, const CharT* s) {
            return replace(pos, count, s, Traits::length(s));
        }
        consteval basic_static_string& replace(const_iterator first, const_iterator last, const CharT* s) {
            return replace(first, last, s, Traits::length(s));
        }
        consteval basic_static_string& replace(size_type pos, size_type count, size_type count2, CharT c) {
            if (pos > size()) throw std::out_of_range("pos > size()");
            if (count > size() - pos) throw std::out_of_range("count > size() - pos");
            return replace(begin() + pos, begin() + pos + count, count2, c);
        }
        consteval basic_static_string& replace(const_iterator first, const_iterator last, size_type count2, CharT c) {
            const auto it = const_cast<iterator>(first);
            Traits::move(it + count2, last, data_ + size_ - last);
            Traits::assign(it, count2, c);
            size_ += count2 - (last - first);
            return *this;
        }
        template <typename I, typename S>
        requires (container_compatible_iterator<I, CharT> && std::sentinel_for<S, I>)
        consteval basic_static_string& replace(const_iterator first, const_iterator last, S first2, I last2) {
            return replace(first, last, basic_static_string(first2, last2));
        }
        consteval basic_static_string& replace(const_iterator first, const_iterator last,
            std::initializer_list<CharT> ilist) {
            return replace(first, last, ilist.begin(), ilist.end());
        }
        consteval basic_static_string& replace_range(const_iterator first, const_iterator last,
            container_compatible_range<CharT> auto&& rng) {
            return replace(first, last, ranges::begin(rng), ranges::end(rng));
        }
        consteval void resize(size_type count) {
            if (count > max_size()) throw std::length_error("count > max_size()");
            Traits::assign(data_[+(size_ = count)], CharT{});
        }
        consteval void resize(size_type count, [[maybe_unused]] CharT c) { resize(count); }
        consteval void swap(basic_static_string& other) { ranges::swap_ranges(data_, other.data_); }

        consteval basic_static_string substr(size_type pos = 0, size_type count = npos) const {
            return {data_ + pos, std::min(count, size() - pos)};
        }

        consteval operator basic_static_string_view<CharT, Traits>() { return {data_, size_}; }
    };
    template <basic_static_string S>
    struct trim {
        static constexpr decltype(S)::template resize_t<S.size()+1> value{S.data()};
    };
    template <basic_static_string S>
    constexpr auto trim_v = trim<S>::value;
    template <std::size_t N>
    using static_string = basic_static_string<char, N>;
    template <std::size_t N>
    using wstatic_string = basic_static_string<wchar_t, N>;
    template <std::size_t N>
    using u8static_string = basic_static_string<char8_t, N>;
    template <std::size_t N>
    using u16static_string = basic_static_string<char16_t, N>;
    template <std::size_t N>
    using u32static_string = basic_static_string<char32_t, N>;

    namespace static_string_literals {
        consteval static_string_view operator""_ssv(const char* s, std::size_t len) { return {s, len}; }
        consteval wstatic_string_view operator""_ssv(const wchar_t* s, std::size_t len) { return {s, len}; }
        consteval u8static_string_view operator""_ssv(const char8_t* s, std::size_t len) { return {s, len}; }
        consteval u16static_string_view operator""_ssv(const char16_t* s, std::size_t len) { return {s, len}; }
        consteval u32static_string_view operator""_ssv(const char32_t* s, std::size_t len) { return {s, len}; }

        template <basic_static_string S>
        consteval auto operator""_ss() { return S; }
    }

    std::ostream& operator<<(std::ostream& os, const tagged<static_string_tag> auto& str) {
        return os << str.c_str();
    }
}

template <typename CharT, typename Traits>
inline constexpr bool std::ranges::enable_borrowed_range<utils::basic_static_string_view<CharT, Traits>> = true;
template <typename CharT, typename Traits>
inline constexpr bool std::ranges::enable_view<utils::basic_static_string_view<CharT, Traits>> = true;

template <utils::tagged<utils::static_string_tag> S>
struct std::formatter<S, typename S::value_type> : formatter<typename S::base_string_view_type, typename S::value_type> {
private:
    using parent = formatter<typename S::base_string_view_type, typename S::value_type>;
public:
    template <typename FmtCtx>
    constexpr FmtCtx::iterator format(const S& str, FmtCtx& ctx) const {
        return parent::format(typename S::base_string_view_type(str), ctx);
    }
};