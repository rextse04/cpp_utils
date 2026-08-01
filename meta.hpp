#pragma once
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

/**
 * @namespace utils::meta
 *
 * Useful utilities for template metaprogramming.
 */
namespace utils::meta {
    /// @defgroup metais_tuple utils::meta::is_tuple
    /// @brief Checks if `T` is a (real) tuple.
    /// @{
    template <typename T>
    struct is_tuple : std::false_type {};
    template <typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type {};
    template <typename T>
    constexpr bool is_tuple_v = is_tuple<T>::value;
    /// @}

    /// @brief C++ exposition-only concept: `tuple-like`.
    template <typename T>
    concept tuple_like = std::tuple_size<std::remove_cvref_t<T>>::value >= 0;
    /// @brief C++ exposition-only concept: `pair-like`.
    template <typename T>
    concept pair_like = std::tuple_size<std::remove_cvref_t<T>>::value == 2;

    /// @defgroup metapack utils::meta::pack
    /// @brief Wraps a parameter pack into a tuple type.
    /// @remark This is useful for converting variadic template parameters into a tuple type.
    /// @{
    template <typename... Ts>
    struct pack { using type = std::tuple<Ts...>; };
    template <typename... Ts>
    using pack_t = pack<Ts...>::type;
    /// @}

    /// @defgroup metasmart_pack utils::meta::smart_pack
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
    /// @defgroup metamake_tuple utils::meta::make_tuple
    /// @brief Makes a tuple from a `tuple-like` type `T`.
    /// @{
    template <tuple_like T>
    struct make_tuple : detail::make_tuple<T, std::make_index_sequence<std::tuple_size_v<T>>> {};
    template <tuple_like T>
    using make_tuple_t = make_tuple<T>::type;
    /// @}

    /// @defgroup metaactual_index utils::meta::actual_index
    /// @brief Calculates a normalized index (in @f$ [0, N)@f$) from a possibly negative index `Idx`
    /// and a tuple-like type `Tuple` of size `N`.
    /// @{
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    struct actual_index : std::integral_constant<std::size_t, (Idx >= 0) ? Idx : (Idx + std::tuple_size_v<Tuple>)> {};
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    constexpr std::size_t actual_index_v = actual_index<Tuple, Idx>::value;
    /// @}

    /// @defgroup metaat utils::meta::at
    /// @brief Similar to `std::tuple_element` but also accepts negative `Idx`.
    /// @{
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    struct at {
        using type = std::tuple_element_t<actual_index_v<Tuple, Idx>, Tuple>;
    };
    template <tuple_like Tuple, std::ptrdiff_t Idx>
    using at_t = at<Tuple, Idx>::type;
    /// @}

    /// @defgroup metasearch utils::meta::search
    /// @brief Finds the first index `Idx` in [ `Begin` , `End` ) such that
    /// `PredTrait<at_t<Tuple, Idx>, T>::value` is true.
    ///
    /// If such an element does not exist, `End` is returned.
    /// @{
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

    /// @defgroup metacontained_in utils::meta::contained_in
    /// @brief Checks if there exists an index `Idx` in @f$ [\text{Begin},\text{End})@f$ such that
    /// `PredTrait<at_t<Tuple, Idx>, T>::value` is true.
    /// @{
    template <tuple_like Tuple, typename T, template<typename, typename> typename PredTrait = std::is_same,
        std::ptrdiff_t Begin = 0, std::ptrdiff_t End = std::tuple_size_v<Tuple>>
    struct contained_in :
            std::bool_constant<search<Tuple, T, PredTrait, Begin, End>::value != actual_index_v<Tuple, End>> {};
    template <tuple_like Tuple, typename T, template<typename, typename> typename PredTrait = std::is_same>
    constexpr bool contained_in_v = contained_in<Tuple, T, PredTrait>::value;
    template <tuple_like Tuple, typename T>
    struct contained_in_trait : contained_in<Tuple, T> {};
    /// @}

    /// @defgroup metaconcat utils::meta::concat
    /// @brief Concatenate multiple tuple-like types into a single tuple type.
    /// @{
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
    /// @defgroup metaslice utils::meta::slice
    /// @brief Extracts a slice from a tuple type from index `Begin` to index `End` (exclusive).
    ///
    /// Both `Begin` and `End` can be negative indices, which are normalized relative to the tuple size.
    /// If `Begin >= End`, an empty tuple is returned.
    /// @{
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

    /// @defgroup metainsert utils::meta::insert
    /// @brief Inserts a tuple-like type `Inserted` into a tuple type `Tuple` at index `Idx`.
    ///
    /// The index `Idx` can be negative, which is normalized relative to the tuple size.
    /// @{
    template <tuple_like Tuple, std::ptrdiff_t Idx, tuple_like Inserted>
    struct insert : concat<slice_t<Tuple, 0, Idx>, Inserted, slice_t<Tuple, Idx>> {};
    template <tuple_like Tuple, std::ptrdiff_t Idx, tuple_like Inserted>
    using insert_t = insert<Tuple, Idx, Inserted>::type;
    /// @}

    /// @defgroup metaerase utils::meta::erase
    /// @brief Erases elements from a tuple type `Tuple` in the range [`Begin`, `End`).
    ///
    /// Both `Begin` and `End` can be negative indices, which are normalized relative to the tuple size.
    /// @{
    template <tuple_like Tuple, std::ptrdiff_t Begin, std::ptrdiff_t End>
    struct erase : concat<slice_t<Tuple, 0, Begin>, slice_t<Tuple, End>> {};
    template <tuple_like Tuple, std::ptrdiff_t Begin, std::ptrdiff_t End>
    using erase_t = erase<Tuple, Begin, End>::type;
    /// @}

    /// @defgroup metareplace utils::meta::replace
    /// @brief Replaces the element at index `Idx` in a tuple type `Tuple` with type `T`.
    ///
    /// The index `Idx` can be negative, which is normalized relative to the tuple size.
    /// @{
    template <tuple_like Tuple, std::ptrdiff_t Idx, typename T>
    struct replace : concat<slice_t<Tuple, 0, Idx>, std::tuple<T>, slice_t<Tuple, Idx + 1>> {};
    template <tuple_like Tuple, std::ptrdiff_t Idx, typename T>
    using replace_t = replace<Tuple, Idx, T>::type;
    /// @}

    /// @defgroup metainfer utils::meta::infer
    /// @brief Converts an `ErasedResult` to a `Result`.
    ///
    /// If `ErasedResult` has a `type` member, that is returned; otherwise, `ErasedResult` itself is returned.
    /// @{
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
    /// @defgroup metamap utils::meta::map
    /// @brief Applies a `Trait` template to corresponding elements across one or more tuple types.
    ///
    /// This creates a new tuple type where each element is the result of applying `Trait`
    /// to the corresponding elements from the input tuples.
    /// @{
    template <template<typename...> typename Trait, tuple_like Tuple, tuple_like... Tuples>
    struct map : detail::map<Trait, std::tuple<Tuple, Tuples...>, std::make_index_sequence<std::tuple_size_v<Tuple>>> {};
    template <template<typename...> typename Trait, tuple_like Tuple, tuple_like... Tuples>
    using map_t = map<Trait, Tuple, Tuples...>::type;
    /// @}

    /// @defgroup metareduce utils::meta::reduce
    /// @brief Applies a `Trait` template to all elements of a tuple, producing a single result.
    ///
    /// The result is either a `TypeResult` or a `ValueResult` depending on the trait.
    /// @{
    template <template<typename...> typename Trait, tuple_like T>
    struct reduce : reduce<Trait, make_tuple_t<T>> {};
    template <template<typename...> typename Trait, typename... Ts>
    struct reduce<Trait, std::tuple<Ts...>> : Trait<Ts...> {};
    template <template<typename...> typename Trait, tuple_like T>
    using reduce_t = reduce<Trait, T>::type;
    template <template<typename...> typename Trait, tuple_like T>
    constexpr auto reduce_v = reduce<Trait, T>::value;
    /// @}

    /// @defgroup metasum utils::meta::sum
    /// @brief Computes the sum of `value` members from `ValueResult` types.
    /// @{
    template <typename... ValueResults>
    struct sum : std::integral_constant<std::uintmax_t, (static_cast<std::uintmax_t>(ValueResults::value) + ...)> {};
    template <typename... ValueResults>
    constexpr auto sum_v = sum<ValueResults...>::value;
    /// @}

    /// @defgroup metaproduct utils::meta::product
    /// @brief Computes the product of `value` members from `ValueResult` types.
    /// @{
    template <typename... ValueResults>
    struct product : std::integral_constant<std::uintmax_t, (static_cast<std::uintmax_t>(ValueResults::value) * ...)> {};
    template <typename... ValueResults>
    constexpr auto product_v = product<ValueResults...>::value;
    /// @}

    /// @defgroup metaextract utils::meta::extract
    /// @brief Extracts the template arguments from a template instantiation.
    ///
    /// `extract` is both a <i>TypeTrait</i> and a <i>MetaTrait</i>.
    /// If `T` is a template instantiation, `extract<T>::type` is a tuple of its template arguments,
    /// and `extract<T>::trait` is a <i>Trait</i> which rebinds `T` to template arguments.
    /// Otherwise, `extract` is empty.
    /// @{
    template <typename T>
    struct extract {};
    template <template<typename...> typename Tmpl, typename... Args>
    struct extract<Tmpl<Args...>> {
        using type = std::tuple<Args...>;
        template <typename... Ts>
        struct trait {
            using type = Tmpl<Ts...>;
        };
    };
    template <typename T>
    using extract_t = extract<T>::type;
    /// @}

    /// @defgroup metarebind utils::meta::rebind
    /// @brief Rebinds a template instance `T` to `Args`.
    ///
    /// If `T` is in the form `Tmpl<...>`, `rebind<T>::type` is `Tmpl<Args...>`. Otherwise, `rebind<T>::type` is `T`.
    /// @{
    template <typename T, typename... Args>
    struct rebind {
        using type = T;
    };
    template <typename T, typename... Args>
    requires requires { typename extract<T>::template trait<Args...>; }
    struct rebind<T, Args...> {
        using type = extract<T>::template trait<Args...>::type;
    };
    template <typename T, typename... Args>
    using rebind_t = rebind<T, Args...>::type;
    /// @}

    /// @brief Binds template arguments to the front of a `Trait` template.
    ///
    /// This creates a new template that accepts fewer arguments by pre-filling the first arguments.
    template <template<typename...> typename Trait, typename... Args>
    struct bind_front {
        template <typename... Ts>
        struct trait : Trait<Args..., Ts...> {};
    };

    /// @brief Binds template arguments to the back of a `Trait` template.
    ///
    /// This creates a new template that accepts fewer arguments by pre-filling the last arguments.
    template <template<typename...> typename Trait, typename... Args>
    struct bind_back {
        template <typename... Ts>
        struct trait : Trait<Ts..., Args...> {};
    };

    /// @defgroup metasubset_of utils::meta::subset_of
    /// @brief Checks if every type in `SmallTuple` is contained in `BigTuple`.
    /// @{
    template <tuple_like SmallTuple, tuple_like BigTuple>
    struct subset_of :
            reduce<std::conjunction, map_t<bind_front<contained_in_trait, BigTuple>::template trait, SmallTuple>> {};
    template <tuple_like SmallTuple, tuple_like BigTuple>
    constexpr bool subset_of_v = subset_of<SmallTuple, BigTuple>::value;
    /// @}

    /// @defgroup metastrict_subset_of utils::meta::strict_subset_of
    /// @brief Checks if `SmallTuple` is a strict subset of `BigTuple`.
    /// @{
    template <tuple_like SmallTuple, tuple_like BigTuple>
    struct strict_subset_of : std::bool_constant<
                std::tuple_size_v<SmallTuple> < std::tuple_size_v<BigTuple> && subset_of<SmallTuple, BigTuple>::value> {};
    template <tuple_like SmallTuple, tuple_like BigTuple>
    constexpr bool strict_subset_of_v = strict_subset_of<SmallTuple, BigTuple>::value;
    /// @}

    /// @defgroup metato_value_results utils::meta::to_value_results
    /// @brief Converts a pack of constexpr values into a tuple of `ValueResult` types.
    ///
    /// Each value is wrapped in a type with a `value` member.
    /// @{
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
    /// @defgroup metarange utils::meta::range
    /// @brief Generates a tuple of `ValueResult` types representing an arithmetic sequence.
    ///
    /// This generates a sequence starting at `Begin`, incrementing by `Step`,
    /// until reaching `End` (exclusive). Returns a tuple of value results.
    /// @{
    template <std::ptrdiff_t End, std::ptrdiff_t Begin = 0, std::ptrdiff_t Step = 1>
    struct range : detail::range<Begin, Step,
                std::make_integer_sequence<std::ptrdiff_t, (End - Begin) / Step>> {};
    template <std::ptrdiff_t End, std::ptrdiff_t Begin = 0, std::ptrdiff_t Step = 1>
    using range_t = range<End, Begin, Step>::type;
    /// @}

    /// @defgroup metaapply utils::meta::apply
    /// @brief Applies a template to a list of types or unpacks a tuple into a template.
    ///
    /// If `Ts` contains a single `std::tuple`, it is unpacked automatically.
    /// @{
    template <template<typename...> typename Tmpl, typename... Ts>
    struct apply { using type = Tmpl<Ts...>; };
    template <template<typename...> typename Tmpl, typename... Ts>
    struct apply<Tmpl, std::tuple<Ts...>> : apply<Tmpl, Ts...> {};
    template <template<typename...> typename Tmpl, typename... Ts>
    using apply_t = apply<Tmpl, Ts...>::type;
    /// @}

    /// @brief Composite type traits.
    ///
    /// Given <i>TypeTrait</i>s `Trait1`, ..., `TraitN`, for any list of types `T1`, ..., `TM`,
    /// `utils::composite<Trait1, ..., TraitN>::template trait<T1, ..., TM>` gives
    /// `TraitN<...Trait1<T1, ..., TM>::type>::type`, if the expression is well-formed.
    /// @{
    template <template<typename...> typename First, template<typename> typename... After>
    struct composite {
        template <typename... Ts>
        struct trait : composite<After...>::template trait<typename First<Ts...>::type> {};
    };
    template <template<typename...> typename First>
    struct composite<First> {
        template <typename... Ts>
        struct trait : First<Ts...> {};
    };
    /// @}

    /// @defgroup metatransform utils::meta::transform
    /// @brief Applies a function to each element of a tuple, returning a new tuple with the results.
    /// @param f A callable that accepts each element of the tuple.
    /// @param t A tuple-like object to transform.
    /// @returns A tuple that represents the results of `f` applied to each element of `t`.
    ///
    /// The resulting tuple retains qualifiers and references of the result types of `f`.
    template <tuple_like Tuple>
    constexpr auto transform(const auto& f, Tuple&& t) {
        const auto expand_transform = [&f]<typename... Ts>(Ts&&... elems) {
            return std::tuple<decltype(f(std::forward<Ts>(elems)))...>(f(std::forward<Ts>(elems))...);
        };
        return std::apply(expand_transform, std::forward<Tuple>(t));
    }
}
