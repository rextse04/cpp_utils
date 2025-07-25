#pragma once
#include <string_view>
#include <stdexcept>
#include <algorithm>

namespace utils {
    template <typename CharT, typename Traits = std::char_traits<CharT>>
    struct basic_static_string {
        using traits_type = Traits;
        using value_type = CharT;
        using pointer = CharT*;
        using const_pointer = const CharT*;
        using reference = CharT&;
        using const_reference = const CharT&;
        using const_iterator = const CharT*;
        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using string_view_type = std::basic_string_view<CharT, traits_type>;
        // data and size public to allow passing as template parameter
        const CharT* const data;
        const size_type size;
        static constexpr size_type npos = static_cast<size_type>(-1);
        consteval basic_static_string(const CharT* s, size_type len) : data(s), size(len) {
            (void) (s + len); // no UB at compile time, so this checks len
        }
        consteval basic_static_string(const CharT* s) : data(s), size(Traits::length(s)) {}
        constexpr const_iterator begin() const noexcept { return data; }
        constexpr const_iterator end() const noexcept { return data + size; }
        constexpr auto rbegin() const noexcept { return const_reverse_iterator(end()); }
        constexpr auto rend() const noexcept { return const_reverse_iterator(begin()); }
        constexpr CharT operator[](size_type i) const noexcept { return data[i]; }
        constexpr CharT at(size_type i) const noexcept { return data[i]; }
        constexpr CharT front() const noexcept { return data[0]; }
        constexpr CharT back() const noexcept { return data[size - 1]; }
        constexpr size_type length() const noexcept { return size; }
        constexpr size_type max_size() const noexcept { return npos; }
        constexpr bool empty() const noexcept { return size == 0; }
        constexpr size_type copy(CharT* dest, size_type count, size_type pos = 0) const {
            if (pos > size) throw std::out_of_range("pos > size");
            Traits::copy(dest, data + pos, count);
            return count;
        }
        constexpr string_view_type substr(size_type pos = 0, size_type count = npos) const {
            if (pos > size) throw std::out_of_range("pos > size");
            return {data + pos, std::min(count, size - pos)};
        }
        constexpr int compare(string_view_type v) const noexcept {
            return string_view_type(data, size).compare(v);
        }
        constexpr int compare(size_type pos1, size_type count1, string_view_type v) const {
            return substr(pos1, count1).compare(v);
        }
        constexpr int compare(size_type pos1, size_type count1,
            string_view_type v, size_type pos2, size_type count2) const {
            return substr(pos1, count1).compare(v.substr(pos2, count2));
        }
        constexpr int compare(const CharT* s) const {
            return compare(string_view_type{s});
        }
        constexpr int compare(size_type pos1, size_type count1, const CharT* s) const {
            return substr(pos1, count1).compare(s);
        }
        constexpr int compare(size_type pos1, size_type count1, const CharT* s, size_type count2) const {
            return substr(pos1, count1).compare(s, count2);
        }
        constexpr bool starts_with(string_view_type sv) const noexcept {
            return string_view_type(data, size).starts_with(sv);
        }
        constexpr bool starts_with(CharT ch) const noexcept {
            return string_view_type(data, size).starts_with(ch);
        }
        constexpr bool starts_with(const CharT* s) const {
            return string_view_type(data, size).starts_with(s);
        }
        constexpr bool ends_with(string_view_type sv) const noexcept {
            return string_view_type(data, size).ends_with(sv);
        }
        constexpr bool ends_with(CharT ch) const noexcept {
            return string_view_type(data, size).ends_with(ch);
        }
        constexpr bool ends_with(const CharT* s) const {
            return string_view_type(data, size).ends_with(s);
        }
        constexpr bool contains(string_view_type sv) const noexcept {
            return string_view_type(data, size).contains(sv);
        }
        constexpr bool contains(CharT c) const noexcept {
            return string_view_type(data, size).contains(c);
        }
        constexpr bool contains(const CharT* s) const {
            return string_view_type(data, size).contains(s);
        }
        constexpr size_type find(string_view_type v, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find(v, pos);
        }
        constexpr size_type find(CharT ch, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find(ch, pos);
        }
        constexpr size_type find(const CharT* s, size_type pos, size_type count) const {
            return string_view_type(data, size).find(s, pos, count);
        }
        constexpr size_type find(const CharT* s, size_type pos = 0) const {
            return string_view_type(data, size).find(s, pos);
        }
        constexpr size_type rfind(string_view_type v, size_type pos = 0) const noexcept {
            return string_view_type(data, size).rfind(v, pos);
        }
        constexpr size_type rfind(CharT ch, size_type pos = 0) const noexcept {
            return string_view_type(data, size).rfind(ch, pos);
        }
        constexpr size_type rfind(const CharT* s, size_type pos, size_type count) const {
            return string_view_type(data, size).rfind(s, pos, count);
        }
        constexpr size_type rfind(const CharT* s, size_type pos = 0) const {
            return string_view_type(data, size).rfind(s, pos);
        }
        constexpr size_type find_first_of(string_view_type v, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_first_of(v, pos);
        }
        constexpr size_type find_first_of(CharT ch, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_first_of(ch, pos);
        }
        constexpr size_type find_first_of(const CharT* s, size_type pos, size_type count) const {
            return string_view_type(data, size).find_first_of(s, pos, count);
        }
        constexpr size_type find_first_of(const CharT* s, size_type pos = 0) const {
            return string_view_type(data, size).find_first_of(s, pos);
        }
        constexpr size_type find_last_of(string_view_type v, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_last_of(v, pos);
        }
        constexpr size_type find_last_of(CharT ch, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_last_of(ch, pos);
        }
        constexpr size_type find_last_of(const CharT* s, size_type pos, size_type count) const {
            return string_view_type(data, size).find_last_of(s, pos, count);
        }
        constexpr size_type find_last_of(const CharT* s, size_type pos = 0) const {
            return string_view_type(data, size).find_last_of(s, pos);
        }
        constexpr size_type find_first_not_of(string_view_type v, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_first_not_of(v, pos);
        }
        constexpr size_type find_first_not_of(CharT ch, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_first_not_of(ch, pos);
        }
        constexpr size_type find_first_not_of(const CharT* s, size_type pos, size_type count) const {
            return string_view_type(data, size).find_first_not_of(s, pos, count);
        }
        constexpr size_type find_first_not_of(const CharT* s, size_type pos = 0) const {
            return string_view_type(data, size).find_first_not_of(s, pos);
        }
        constexpr size_type find_last_not_of(string_view_type v, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_last_not_of(v, pos);
        }
        constexpr size_type find_last_not_of(CharT ch, size_type pos = 0) const noexcept {
            return string_view_type(data, size).find_last_not_of(ch, pos);
        }
        constexpr size_type find_last_not_of(const CharT* s, size_type pos, size_type count) const {
            return string_view_type(data, size).find_last_not_of(s, pos, count);
        }
        constexpr size_type find_last_not_of(const CharT* s, size_type pos = 0) const {
            return string_view_type(data, size).find_last_not_of(s, pos);
        }
        constexpr operator string_view_type() const noexcept {
            return string_view_type(data, size);
        }
    };
    using static_string = basic_static_string<char>;
    using wstatic_string = basic_static_string<wchar_t>;
    using u8static_string = basic_static_string<char8_t>;
    using u16static_string = basic_static_string<char16_t>;
    using u32static_string = basic_static_string<char32_t>;

    namespace static_string_literals {
        consteval static_string operator""_ss(const char* s, std::size_t len) { return {s, len}; }
        consteval wstatic_string operator""_ss(const wchar_t* s, std::size_t len) { return {s, len}; }
        consteval u8static_string operator""_ss(const char8_t* s, std::size_t len) { return {s, len}; }
        consteval u16static_string operator""_ss(const char16_t* s, std::size_t len) { return {s, len}; }
        consteval u32static_string operator""_ss(const char32_t* s, std::size_t len) { return {s, len}; }
    }
}

template <typename CharT, typename Traits>
inline constexpr bool std::ranges::enable_borrowed_range<utils::basic_static_string<CharT, Traits>> = true;
template <typename CharT, typename Traits>
inline constexpr bool std::ranges::enable_view<utils::basic_static_string<CharT, Traits>> = true;