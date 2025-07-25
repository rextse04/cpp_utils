#pragma once
#include <tuple>
#include <cstddef>

namespace utils {
    template <std::ptrdiff_t Idx, typename... Ts>
    struct tuple_at {
        static constexpr std::size_t index = (Idx >= 0) ? Idx : Idx + sizeof...(Ts);
        using type = std::tuple_element_t<index, std::tuple<Ts...>>;
    };
    template <std::ptrdiff_t Idx, typename... Ts>
    struct tuple_at<Idx, std::tuple<Ts...>> : tuple_at<Idx, Ts...> {};
    template <std::ptrdiff_t Idx, typename... Ts>
    using tuple_at_t = typename tuple_at<Idx, Ts...>::type;

    template <typename Tuple0, typename... Tuples>
    struct tuple_concat {
        using type = decltype(std::tuple_cat(std::declval<Tuple0&&>(), std::declval<tuple_concat<Tuples&&...>>()));
    };
    template <typename Tuple0>
    struct tuple_concat<Tuple0> {
        using type = Tuple0;
    };
    template <typename Tuple0, typename... Tuples>
    using tuple_concat_t = tuple_concat<Tuple0, Tuples...>::type;

    namespace detail {
        template <typename Current, std::size_t Begin, std::size_t End, typename... Ts>
        struct tuple_slice {
            using type = std::conditional_t<Begin >= End, Current,
                tuple_slice<tuple_concat_t<Current, tuple_at_t<Begin, Ts...>>, Begin+1u, End, Ts...>>;
        };
    }
    template <std::size_t Begin, std::size_t End, typename... Ts>
    struct tuple_slice {
        using type = detail::tuple_slice<std::tuple<>, Begin, End, Ts...>;
    };
    template <std::size_t Begin, std::size_t End, typename... Ts>
    struct tuple_slice<Begin, End, std::tuple<Ts...>> : tuple_slice<Begin, End, Ts...> {};
    template <std::size_t Begin, std::size_t End, typename... Ts>
    using tuple_slice_t = typename tuple_slice<Begin, End, Ts...>::type;

    template <std::size_t Idx, typename Tuple, typename... Ts>
    struct tuple_insert {
        using type = tuple_concat_t<
            tuple_slice_t<0, Idx, Tuple>,
            std::tuple<Ts...>,
            tuple_slice_t<Idx, std::tuple_size_v<Tuple>, Tuple>>;
    };
    template <std::size_t Idx, typename Tuple, typename... Ts>
    struct tuple_insert<Idx, Tuple, std::tuple<Ts...>> : tuple_insert<Idx, Tuple, Ts...> {};
    template <std::size_t Idx, typename Tuple, typename... Ts>
    using tuple_insert_t = tuple_insert<Idx, Tuple, Ts...>::type;

    template <std::size_t Begin, std::size_t End, typename... Ts>
    struct tuple_erase {
        using type = tuple_concat_t<tuple_slice_t<0, Begin, Ts...>, tuple_slice_t<End, sizeof...(Ts), Ts...>>;
    };
    template <std::size_t Begin, std::size_t End, typename... Ts>
    struct tuple_erase<Begin, End, std::tuple<Ts...>> : tuple_erase<Begin, End, Ts...> {};
    template <std::size_t Begin, std::size_t End, typename... Ts>
    using tuple_erase_t = tuple_erase<Begin, End, Ts...>::type;

    template <template<typename> typename TypeTrait, typename... Ts>
    struct tuple_apply {
        using type = std::tuple<TypeTrait<Ts>...>;
    };
    template <template<typename> typename TypeTrait, typename... Ts>
    struct tuple_apply<TypeTrait, std::tuple<Ts...>> : tuple_apply<TypeTrait, Ts...> {};
    template <template<typename> typename TypeTrait, typename... Ts>
    using tuple_apply_t = tuple_apply<TypeTrait, Ts...>::type;

    template <template<typename...> typename Trait, typename... Ts>
    struct tuple_reduce : Trait<Ts...> {};
    template <template<typename...> typename Trait, typename... Ts>
    struct tuple_reduce<Trait, std::tuple<Ts...>> : tuple_reduce<Trait, Ts...> {};
    template <template<typename...> typename Trait, typename... Ts>
    using tuple_reduce_t = tuple_reduce<Trait, Ts...>::type;

    template <template<typename> typename TypeTrait, typename... Ts>
    struct tuple_fulfills_all : std::bool_constant<(TypeTrait<Ts>::value && ...)> {};
    template <template<typename> typename TypeTrait, typename... Ts>
    struct tuple_fulfills_all<TypeTrait, std::tuple<Ts...>> : tuple_fulfills_all<TypeTrait, Ts...> {};

    template <template<typename> typename TypeTrait, typename... Ts>
    struct tuple_fulfills_any : std::bool_constant<(TypeTrait<Ts>::value || ...)> {};
    template <template<typename> typename TypeTrait, typename... Ts>
    struct tuple_fulfills_any<TypeTrait, std::tuple<Ts...>> : tuple_fulfills_any<TypeTrait, Ts...> {};
}