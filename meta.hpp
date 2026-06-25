#pragma once
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

/**
 * @file
 *
 * Key concepts and conventions:
 * - A <i>Result</i> is a type that either has a member type alias @code type@endcode,
 * or a constexpr static member @code value@endcode, named <i>TypeResult</i> and <i>ValueResult</i> respectively.
 * For example, @code std::integer_constant@endcode instances are <i>ValueResult</i>s.
 * This can be seen as a generalization of types obtained by applying <i>Trait</i>s.
 * - An <i>ErasedResult</i> is not a <i>Result</i>. It has a member type alias @code result@endcode,
 * which is defined to be a <i>TypeResult</i> or a <i>ValueResult</i>.
 * It can be inferred to a <i>Result</i> via @code infer@endcode, however.
 * - A <i>Trait</i> is a template that accepts a fixed or variadic number of template type arguments.
 * Any instance of a <i>Trait</i> should be a <i>Result</i>, or the program should be ill-formed
 * (which is usually guaranteed by setting @code static_assert@endcode checks).
 * In addition, valid instances of <i>Trait</i> should either all be <i>TypeResult</i>s, or all be <i>ValueResult</i>s.
 * They are named <i>TypeTrait</i>s and <i>ValueTrait</i>s respectively.
 * - A <i>MetaTrait</i> is a template whose valid instances have an inner template which satisfies <i>Trait</i>.
 */

namespace utils::meta {
    /// @{
    /// @brief Checks if @code T@endcode is a (real) tuple.
    template <typename T>
    struct is_tuple : std::false_type {};
    template <typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type {};
    template <typename T>
    constexpr bool is_tuple_v = is_tuple<T>::value;
    /// @}

    /// @brief C++ exposition-only concept: @code tuple-like@endcode.
    template <typename T>
    concept tuple_like = std::tuple_size<std::remove_cvref_t<T>>::value >= 0;
    /// @brief C++ exposition-only concept: @code pair-like@endcode.
    template <typename T>
    concept pair_like = std::tuple_size<std::remove_cvref_t<T>>::value == 2;

    /// @{
    /// @brief Wraps a parameter pack into a tuple type.
    /// @remark This is useful for converting variadic template parameters into a tuple type.
    template <typename... Ts>
    struct pack { using type = std::tuple<Ts...>; };
    template <typename... Ts>
    using pack_t = pack<Ts...>::type;
    /// @}

    /// @brief Wraps a parameter pack into a tuple type, unless exactly one tuple is given.
    /// @{
    template <typename... Ts>
    struct smart_pack { using type = std::tuple<Ts...>; };
    template <typename... Ts>
    struct smart_pack<std::tuple<Ts...>> { using type = std::tuple<Ts...>; };
    template <typename... Ts>
    using smart_pack_t = smart_pack<Ts...>::type;
    /// @}

    namespace detail {
        template <typename T, typename IdxSeq>
        struct make_tuple;
        template <typename T, std::size_t... Idxs>
        struct make_tuple<T, std::index_sequence<Idxs...>> {
            using type = std::tuple<std::tuple_element_t<Idxs, T>...>;
        };
    }
    /// @{
    /// @brief Makes a tuple from a @code tuple-like@endcode type @code T@endcode.
    template <tuple_like T>
    struct make_tuple : detail::make_tuple<T, std::make_index_sequence<std::tuple_size_v<T>>> {};
    template <tuple_like T>
    using make_tuple_t = make_tuple<T>::type;
    /// @}

    /// @{
    /// @brief Calculates a normalized index (in @f$ [0, N)@f$) from a possibly negative index @code Idx@endcode
    /// and a tuple-like type @code Tuple@endcode of size @code N@endcode.
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    struct actual_index : std::integral_constant<std::size_t, (Idx >= 0) ? Idx : (Idx + std::tuple_size_v<Tuple>)> {};
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    constexpr std::size_t actual_index_v = actual_index<Tuple, Idx>::value;
    /// @}

    /// @{
    /// @brief Similar to @code std::tuple_element@endcode but also accepts negative @code Idx@endcode.
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    struct at {
        using type = std::tuple_element_t<actual_index_v<Tuple, Idx>, Tuple>;
    };
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    using at_t = at<Tuple, Idx>::type;
    /// @}

    /// @{
    /// @brief Finds the first index @code Idx@endcode in @f$ [\text{Begin},\text{End})@f$ such that
    /// @code PredTrait<at_t<Tuple, Idx>, T>::value@endcode is true.
    /// @remark If such an element does not exist, @code End@endcode is returned.
    template <tuple_like Tuple, typename T, template<typename, typename> typename PredTrait = std::is_same,
        std::ptrdiff_t Begin = 0, std::ptrdiff_t End = std::tuple_size_v<Tuple>>
    struct search : std::integral_constant<std::size_t, PredTrait<at_t<Tuple, Begin>, T>::value
        ? Begin
        : search<Tuple, T, PredTrait, Begin + 1, End>::value> {};
    template <tuple_like Tuple, typename T, template<typename, typename> typename PredTrait, std::ptrdiff_t End>
    struct search<Tuple, T, PredTrait, End, End> : std::integral_constant<std::size_t, actual_index_v<Tuple, End>> {};
    template <tuple_like Tuple, typename T, template<typename, typename> typename PredTrait = std::is_same>
    constexpr std::size_t search_v = search<Tuple, T, PredTrait>::value;
    template <tuple_like Tuple, typename T>
    struct search_trait : search<Tuple, T> {};
    /// @}

    /// @{
    /// @brief Checks if there exists an index @code Idx@endcode in @f$ [\text{Begin},\text{End})@f$ such that
    /// @code PredTrait<at_t<Tuple, Idx>, T>::value@endcode is true.
    template <tuple_like Tuple, typename T, template<typename, typename> typename PredTrait = std::is_same,
        std::ptrdiff_t Begin = 0, std::ptrdiff_t End = std::tuple_size_v<Tuple>>
    struct contained_in :
        std::bool_constant<search<Tuple, T, PredTrait, Begin, End>::value != actual_index_v<Tuple, End>> {};
    template <tuple_like Tuple, typename T, template<typename, typename> typename PredTrait = std::is_same>
    constexpr bool contained_in_v = contained_in<Tuple, T, PredTrait>::value;
    template <tuple_like Tuple, typename T>
    struct contained_in_trait : contained_in<Tuple, T> {};
    /// @}

    /// @{
    /// @brief Concatenate multiple tuple-like types into a single tuple type.
    template <tuple_like... Tuples>
    struct concat {
        using type = decltype(std::tuple_cat(std::declval<Tuples>()...));
    };
    template <tuple_like... Tuples>
    using concat_t = concat<Tuples...>::type;
    /// @}

    namespace detail {
        template <typename Tuple, std::size_t Begin, typename RelIdxSeq>
        struct slice;
        template <typename Tuple, std::size_t Begin, std::size_t... RelIdxs>
        struct slice<Tuple, Begin, std::index_sequence<RelIdxs...>> {
            using type = std::tuple<std::tuple_element_t<Begin + RelIdxs, Tuple>...>;
        };
    }
    /// @{
    /// @brief Extracts a slice from a tuple type from index @code Begin@endcode to index @code End@endcode (exclusive).
    /// @remark Both @code Begin@endcode and @code End@endcode can be negative indices, which are normalized relative to the tuple size.
    /// @remark If @code Begin >= End@endcode, an empty tuple is returned.
    template <tuple_like Tuple, std::ptrdiff_t Begin = 0, std::ptrdiff_t End = std::tuple_size_v<Tuple>>
    struct slice {
    private:
        static constexpr std::size_t begin = actual_index_v<Tuple, Begin>, end = actual_index_v<Tuple, End>;
    public:
        using type = decltype([]() {
            if constexpr (begin < end) return detail::slice<Tuple, begin, std::make_index_sequence<end - begin>>{};
            else return std::type_identity<std::tuple<>>{};
        }())::type;
    };
    template <tuple_like Tuple, std::ptrdiff_t Begin = 0, std::ptrdiff_t End = std::tuple_size_v<Tuple>>
    using slice_t = slice<Tuple, Begin, End>::type;
    /// @}

    /// @{
    /// @brief Inserts a tuple-like type @code Inserted@endcode into a tuple type @code Tuple@endcode at index @code Idx@endcode.
    /// @remark The index @code Idx@endcode can be negative, which is normalized relative to the tuple size.
    template <tuple_like Tuple, std::ptrdiff_t Idx, tuple_like Inserted>
    struct insert : concat<slice_t<Tuple, 0, Idx>, Inserted, slice_t<Tuple, Idx>> {};
    template <tuple_like Tuple, std::ptrdiff_t Idx, tuple_like Inserted>
    using insert_t = insert<Tuple, Idx, Inserted>::type;
    /// @}

    /// @{
    /// @brief Erases elements from a tuple type @code Tuple@endcode in the range [@code Begin@endcode, @code End@endcode).
    /// @remark Both @code Begin@endcode and @code End@endcode can be negative indices, which are normalized relative to the tuple size.
    template <tuple_like Tuple, std::ptrdiff_t Begin, std::ptrdiff_t End>
    struct erase : concat<slice_t<Tuple, 0, Begin>, slice_t<Tuple, End>> {};
    template <tuple_like Tuple, std::ptrdiff_t Begin, std::ptrdiff_t End>
    using erase_t = erase<Tuple, Begin, End>::type;
    /// @}

    /// @{
    /// @brief Replaces the element at index @code Idx@endcode in a tuple type @code Tuple@endcode with type @code T@endcode.
    /// @remark The index @code Idx@endcode can be negative, which is normalized relative to the tuple size.
    template <tuple_like Tuple, std::ptrdiff_t Idx, typename T>
    struct replace : concat<slice_t<Tuple, 0, Idx>, std::tuple<T>, slice_t<Tuple, Idx + 1>> {};
    template <tuple_like Tuple, std::ptrdiff_t Idx, typename T>
    using replace_t = replace<Tuple, Idx, T>::type;
    /// @}

    /// @{
    /// @brief Converts an @code ErasedResult@endcode to a @code Result@endcode.
    /// @remark If @code ErasedResult@endcode has a @code type@endcode member, that is returned;
    /// otherwise, @code ErasedResult@endcode itself is returned.
    template <typename ErasedResult>
    struct infer {
        using type = std::conditional_t<requires { typename ErasedResult::type; },
            typename ErasedResult::type, ErasedResult>;
    };
    template <typename ErasedResult>
    using infer_t = infer<ErasedResult>::type;
    /// @}

    namespace detail {
        template <std::size_t Idx, template<typename...> typename Trait, typename TuplesTuple>
        struct map_step;
        template <std::size_t Idx, template<typename...> typename Trait, typename... Tuples>
        struct map_step<Idx, Trait, std::tuple<Tuples...>> {
            using type = infer_t<Trait<std::tuple_element_t<Idx, Tuples>...>>;
        };
        template <template<typename...> typename Trait, typename TuplesTuple, typename IdxSeq>
        struct map;
        template <template<typename...> typename Trait, typename TuplesTuple, std::size_t... Idxs>
        struct map<Trait, TuplesTuple, std::index_sequence<Idxs...>> {
            using type = std::tuple<typename map_step<Idxs, Trait, TuplesTuple>::type...>;
        };
    }
    /// @{
    /// @brief Applies a @code Trait@endcode template to corresponding elements across one or more tuple types.
    /// @remark This creates a new tuple type where each element is the result of applying @code Trait@endcode
    /// to the corresponding elements from the input tuples.
    template <template<typename...> typename Trait, tuple_like Tuple, tuple_like... Tuples>
    struct map : detail::map<Trait, std::tuple<Tuple, Tuples...>, std::make_index_sequence<std::tuple_size_v<Tuple>>> {};
    template <template<typename...> typename Trait, tuple_like Tuple, tuple_like... Tuples>
    using map_t = map<Trait, Tuple, Tuples...>::type;
    /// @}

    /// @{
    /// @brief Applies a @code Trait@endcode template to all elements of a tuple, producing a single result.
    /// @remark The result is either a @code TypeResult@endcode or a @code ValueResult@endcode depending on the trait.
    template <template<typename...> typename Trait, tuple_like T>
    struct reduce : reduce<Trait, make_tuple_t<T>> {};
    template <template<typename...> typename Trait, typename... Ts>
    struct reduce<Trait, std::tuple<Ts...>> : Trait<Ts...> {};
    template <template<typename...> typename Trait, tuple_like T>
    using reduce_t = reduce<Trait, T>::type;
    template <template<typename...> typename Trait, tuple_like T>
    constexpr auto reduce_v = reduce<Trait, T>::value;
    /// @}

    /// @{
    /// @brief Computes the sum of @code value@endcode members from @code ValueResult@endcode types.
    template <typename... ValueResults>
    struct sum : std::integral_constant<std::uintmax_t, (static_cast<std::uintmax_t>(ValueResults::value) + ...)> {};
    template <typename... ValueResults>
    constexpr auto sum_v = sum<ValueResults...>::value;
    /// @}

    /// @{
    /// @brief Computes the product of @code value@endcode members from @code ValueResult@endcode types.
    template <typename... ValueResults>
    struct product : std::integral_constant<std::uintmax_t, (static_cast<std::uintmax_t>(ValueResults::value) * ...)> {};
    template <typename... ValueResults>
    constexpr auto product_v = product<ValueResults...>::value;
    /// @}

    /// @{
    /// @brief Binds template arguments to the front of a @code Trait@endcode template.
    /// @remark This creates a new template that accepts fewer arguments by pre-filling the first arguments.
    template <template<typename...> typename Trait, typename... Args>
    struct bind_front {
        template <typename... Ts>
        struct trait : Trait<Args..., Ts...> {};
    };
    /// @}

    /// @{
    /// @brief Binds template arguments to the back of a @code Trait@endcode template.
    /// @remark This creates a new template that accepts fewer arguments by pre-filling the last arguments.
    template <template<typename...> typename Trait, typename... Args>
    struct bind_back {
        template <typename... Ts>
        struct trait : Trait<Ts..., Args...> {};
    };
    /// @}

    /// @{
    /// @brief Checks if every type in @code SmallTuple@endcode is contained in @code BigTuple@endcode.
    template <tuple_like SmallTuple, tuple_like BigTuple>
    struct subset_of :
        reduce<std::conjunction, map_t<bind_front<contained_in_trait, BigTuple>::template trait, SmallTuple>> {};
    template <tuple_like SmallTuple, tuple_like BigTuple>
    constexpr bool subset_of_v = subset_of<SmallTuple, BigTuple>::value;
    /// @}

    /// @{
    /// @brief Checks if @code SmallTuple@endcode is a strict subset of @code BigTuple@endcode.
    template <tuple_like SmallTuple, tuple_like BigTuple>
    struct strict_subset_of : std::bool_constant<
        std::tuple_size_v<SmallTuple> < std::tuple_size_v<BigTuple> && subset_of<SmallTuple, BigTuple>::value> {};
    template <tuple_like SmallTuple, tuple_like BigTuple>
    constexpr bool strict_subset_of_v = strict_subset_of<SmallTuple, BigTuple>::value;
    /// @}

    /// @{
    /// @brief Converts a pack of constexpr values into a tuple of @code ValueResult@endcode types.
    /// @remark Each value is wrapped in a type with a @code value@endcode member.
    template <auto... Values>
    struct to_value_results {
        using type = std::tuple<typename to_value_results<Values>::type...>;
    };
    template <auto Value>
    struct to_value_results<Value> {
        struct type { static constexpr auto value = Value; };
    };
    template <auto... Values>
    using to_value_results_t = to_value_results<Values...>::type;
    /// @}

    namespace detail {
        template <std::ptrdiff_t Begin, std::ptrdiff_t Step, typename T>
        struct range;
        template <std::ptrdiff_t Begin, std::ptrdiff_t Step, std::ptrdiff_t... Idxs>
        struct range<Begin, Step, std::integer_sequence<std::ptrdiff_t, Idxs...>> :
            pack<to_value_results_t<Begin + Idxs * Step>...> {};
    }
    /// @{
    /// @brief Generates a tuple of @code ValueResult@endcode types representing an arithmetic sequence.
    /// @remark Generates a sequence starting at @code Begin@endcode, incrementing by @code Step@endcode,
    /// until reaching @code End@endcode (exclusive). Returns a tuple of value results.
    template <std::ptrdiff_t End, std::ptrdiff_t Begin = 0, std::ptrdiff_t Step = 1>
    struct range : detail::range<Begin, Step,
        std::make_integer_sequence<std::ptrdiff_t, (End - Begin) / Step>> {};
    template <std::ptrdiff_t End, std::ptrdiff_t Begin = 0, std::ptrdiff_t Step = 1>
    using range_t = range<End, Begin, Step>::type;
    /// @}

    /// @{
    /// @brief Applies a template to a list of types or unpacks a tuple into a template.
    /// @remark If @code Ts@endcode contains a single @code std::tuple@endcode, it is unpacked automatically.
    template <template<typename...> typename Tmpl, typename... Ts>
    struct apply { using type = Tmpl<Ts...>; };
    template <template<typename...> typename Tmpl, typename... Ts>
    struct apply<Tmpl, std::tuple<Ts...>> : apply<Tmpl, Ts...> {};
    template <template<typename...> typename Tmpl, typename... Ts>
    using apply_t = apply<Tmpl, Ts...>::type;
    /// @}

    /// @{
    /// @brief Applies a function to each element of a tuple, returning a new tuple with the results.
    /// @param f A callable that accepts each element of the tuple.
    /// @param t A tuple-like object to transform.
    /// @returns A tuple that represents the results of @code f@endcode applied to each element of @code t@endcode.
    /// @remark The resulting tuple retains qualifiers and references of the result types of @code f@endcode.
    template <tuple_like Tuple>
    constexpr auto transform(const auto& f, Tuple&& t) {
        const auto expand_transform = [&f]<typename... Ts>(Ts&&... elems) {
            return std::tuple<decltype(f(std::forward<Ts>(elems)))...>(f(std::forward<Ts>(elems))...);
        };
        return std::apply(expand_transform, std::forward<Tuple>(t));
    }
    /// @}
}