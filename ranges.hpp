#pragma once
#include <ranges>
#include <iterator>

namespace utils {
    namespace ranges = std::ranges;
    namespace views = std::ranges::views;

    template <typename Container>
    concept reservable_container =
        std::ranges::sized_range<Container> &&
        requires (Container& c, std::ranges::range_size_t<Container> n) {
            c.reserve(n);
            { c.capacity() } -> std::same_as<decltype(n)>;
            { c.max_size() } -> std::same_as<decltype(n)>;
        };

    template <typename Container, typename Reference>
    concept appendable_container = requires (Container& c, Reference&& ref) {
        requires (
            requires { c.emplace_back(std::forward<Reference>(ref)); }     ||
            requires { c.push_back(std::forward<Reference>(ref)); }        ||
            requires { c.emplace(c.end(), std::forward<Reference>(ref)); } ||
            requires { c.insert(c.end(), std::forward<Reference>(ref)); }
        );
    };

    template <typename C>
    constexpr auto container_appender(C& c) {
        return [&c]<class Reference>(Reference&& ref) {
            if constexpr (requires { c.emplace_back(std::declval<Reference>()); })
                c.emplace_back(std::forward<Reference>(ref));
            else if constexpr (requires { c.push_back(std::declval<Reference>()); })
                c.push_back(std::forward<Reference>(ref));
            else if constexpr (requires { c.emplace(c.end(), std::declval<Reference>()); })
                c.emplace(c.end(), std::forward<Reference>(ref));
            else
                c.insert(c.end(), std::forward<Reference>(ref));
        };
    }

    template <typename I, typename T>
    concept container_compatible_iterator =
        std::input_iterator<I> &&
        std::convertible_to<std::iter_reference_t<I>, T>;
    template <typename R, typename T>
    concept container_compatible_range =
        ranges::input_range<R> &&
        std::convertible_to<ranges::range_reference_t<R>, T>;
}