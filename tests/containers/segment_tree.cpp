#define BOOST_TEST_MODULE containers/segment_tree.hpp
#include "common.hpp"
#include "containers/segment_tree.hpp"
#include <vector>
#include <string>
#include <ranges>
#include <utility>
#include <random>
#include <climits>
#include <algorithm>
#include <array>
#include <numeric>

using namespace utils;

static void basic_test_core(segment_tree<std::string, int>& t, int factor = 1, int size = 10) {
    using enum interval_endpoint_type;
    const auto& ct = t;
    BOOST_CHECK_EQUAL(t.size(), size);
    BOOST_CHECK(!t.empty());
    for (int i = 0; i < size; ++i) {
        char key[2]{};
        key[0] = '0' + i;
        BOOST_CHECK(t.contains(key));
        BOOST_CHECK_EQUAL(ct[key], i * factor);
        BOOST_CHECK_EQUAL(ct.at(key), i * factor);
        BOOST_CHECK(ct.find(key) == ct.begin() + i);
        BOOST_CHECK_EQUAL((*ct.lower_bound(key)).second, i * factor);
        if (i == size - 1) {
            BOOST_CHECK(t.upper_bound(key) == t.end());
        } else {
            const auto it = ct.upper_bound(key);
            BOOST_CHECK_EQUAL((*it).second, (i+1) * factor);
            BOOST_CHECK(it < ct.end());
        }
    }
    BOOST_CHECK(!ct.contains("10"));
    BOOST_CHECK(ct.find("10") == ct.end());
    BOOST_CHECK_THROW(ct.at("10"), std::out_of_range);
    BOOST_CHECK(ct.lower_bound("-1") == ct.begin());
    BOOST_CHECK(ct.upper_bound("-1") == ct.begin());
    BOOST_CHECK(ct.lower_bound("10") == ct.begin() + 2);
    BOOST_CHECK(ct.upper_bound("10") == ct.begin() + 2);
    BOOST_CHECK(ct.lower_bound("A") == ct.end());
    BOOST_CHECK_EQUAL(ct.aggregate(("1" | open, "9" | open)), (45-1-9) * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(("1" | open, "9" | closed)), (45-1) * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(("1" | closed, "9" | open)), (45-9) * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(("1" | closed, "9" | closed)), 45 * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(("15" | open, "85" | closed)), (45-1-9) * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(("5" | closed, "5" | closed)), 5 * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(("5" | open, "5" | closed)), 0);
    BOOST_CHECK_EQUAL(ct.aggregate(("9" | closed, "0" | closed)), 0);
    const int total = size * (size - 1) / 2;
    BOOST_CHECK_EQUAL(ct.total(), total * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(ct.begin(), ct.end()), total * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(ct.begin() + 1, ct.end() - 1), (total - (size - 1)) * factor);
    BOOST_CHECK_EQUAL(ct.aggregate(ct.begin() + 1, ct.begin() + 2), 1 * factor);
}
static void basic_test_empty(segment_tree<std::string, int>& t) {
    using enum interval_endpoint_type;
    const auto& ct = t;
    BOOST_CHECK(t.empty());
    BOOST_CHECK_EQUAL(t.size(), 0);
    BOOST_CHECK(t.begin() == t.end());
    BOOST_CHECK(ct.begin() == ct.end());
    BOOST_CHECK(!ct.contains("0"));
    BOOST_CHECK(ct.find("0") == ct.begin());
    BOOST_CHECK(ct.find("0") == ct.end());
    BOOST_CHECK_THROW(ct.at("0"), std::out_of_range);
    BOOST_CHECK(ct.lower_bound("0") == ct.begin());
    BOOST_CHECK(ct.lower_bound("0") == ct.end());
    BOOST_CHECK(ct.upper_bound("0") == ct.begin());
    BOOST_CHECK(ct.upper_bound("0") == ct.end());
    BOOST_CHECK_EQUAL(ct.aggregate(("0" | closed, "9" | closed)), 0);
}
BOOST_AUTO_TEST_CASE(basic_test) {
    using enum interval_endpoint_type;
    std::vector<std::string> keys = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    std::vector<int> values = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    segment_tree t(std::sorted_equivalent, std::move(keys), std::move(values));
    BOOST_CALL_TEST_PROC(basic_test_core, t);
    t = t;
    BOOST_CALL_TEST_PROC(basic_test_core, t);
    decltype(t) t1(t), t2, t4;
    BOOST_CALL_TEST_PROC(basic_test_empty, t2);
    t2 = t;
    BOOST_CALL_TEST_PROC(basic_test_core, t1);
    BOOST_CALL_TEST_PROC(basic_test_core, t2);
    t["0"] = 0;
    for (auto&& [key, value] : t) {
        value = value * 2;
    }
    BOOST_CALL_TEST_PROC(basic_test_core, t, 2);
    decltype(t) t3 = std::move(t);
    BOOST_CALL_TEST_PROC(basic_test_empty, t);
    BOOST_CALL_TEST_PROC(basic_test_core, t3, 2);
    t4 = std::move(t3);
    BOOST_CALL_TEST_PROC(basic_test_empty, t3);
    BOOST_CALL_TEST_PROC(basic_test_core, t4, 2);
    segment_tree t5(segment_tree_nodes, t4.keys(), t4.nodes());
    BOOST_CALL_TEST_PROC(basic_test_core, t5, 2);

    segment_tree<std::string, int> u({
        {":", 30},
        {"0", 0},
        {"2", 6},
        {"4", 12},
        {"6", 18},
        {"8", 24},
        {"1", 3},
        {"3", 9},
        {"5", 15},
        {"7", 21},
        {"9", 27},
    });
    BOOST_CALL_TEST_PROC(basic_test_core, u, 3, 11);
    swap(t1, t1);
    BOOST_CALL_TEST_PROC(basic_test_core, t1);
    swap(t1, u);
    BOOST_CALL_TEST_PROC(basic_test_core, u);
    BOOST_CALL_TEST_PROC(basic_test_core, t1, 3, 11);
    auto update = t1.make_update();
    t1.make_update(update, ("0" | closed, "9" | closed), 1);
    BOOST_CHECK_EQUAL(t1.total(update), 55 * 3 + 10);
    BOOST_CHECK_EQUAL(t1.aggregate(("1" | closed, "9" | closed), update), 45 * 3 + 9);
    BOOST_CHECK_EQUAL(t1.aggregate(t1.begin() + 1, t1.begin() + 10, update), 45 * 3 + 9);
    BOOST_CHECK_EQUAL(t1.aggregate(("5" | open, ":" | closed), update), 19+22+25+28+30);
    BOOST_CHECK_EQUAL(t1.aggregate(t1.begin() + 6, t1.end(), update), 19+22+25+28+30);
}

static void single_element_test_core(segment_tree<std::string, int>& t, int v) {
    using enum interval_endpoint_type;
    BOOST_CHECK(!t.empty());
    BOOST_CHECK_EQUAL(t.size(), 1);
    BOOST_CHECK(t.contains("0"));
    BOOST_CHECK(!t.contains("1"));
    BOOST_CHECK_EQUAL(t["0"], v);
    BOOST_CHECK_EQUAL(t.at("0"), v);
    BOOST_CHECK_THROW(t.at("1"), std::out_of_range);
    BOOST_CHECK(t.find("0") == t.begin());
    BOOST_CHECK(t.find("1") == t.end());
    BOOST_CHECK(t.lower_bound("0") == t.begin());
    BOOST_CHECK(t.upper_bound("0") == t.end());
    BOOST_CHECK_EQUAL(t.aggregate(("0" | closed, "0" | closed)), v);
    BOOST_CHECK_EQUAL(t.aggregate(("0" | open, "0" | closed)), 0);
    BOOST_CHECK_EQUAL(t.aggregate(("0" | closed, "0" | open)), 0);
    BOOST_CHECK_EQUAL(t.aggregate(("0" | open, "0" | open)), 0);
}
BOOST_AUTO_TEST_CASE(single_element_test) {
    const auto init = {std::pair{std::string("0"), 42}};
    segment_tree t(std::sorted_equivalent, init.begin(), init.end());
    BOOST_CALL_TEST_PROC(single_element_test_core, t, 42);
    t["0"] = 84;
    BOOST_CALL_TEST_PROC(single_element_test_core, t, 84);
}

using randomized_test_ops_list = std::tuple<
    std::pair<std::plus<unsigned>, decltype([]() { return 0u; })>,
    std::pair<std::multiplies<unsigned>, decltype([]() { return 1u; })>,
    std::pair<decltype([](unsigned a, unsigned b) { return std::max(a, b); }), decltype([]() { return 0u; })>,
    std::pair<decltype([](unsigned a, unsigned b) { return std::min(a, b); }), decltype([]() { return UINT_MAX; })>
>;
BOOST_AUTO_TEST_CASE_TEMPLATE(randomized_test, Ops, randomized_test_ops_list) {
    using enum interval_endpoint_type;
    using Sum = Ops::first_type;
    using Identity = Ops::second_type;
    const auto seed = std::random_device()();
    std::mt19937 rng(seed);
    BOOST_TEST_MESSAGE("Seed: " << seed);
    std::uniform_int_distribution<unsigned> mapped_dist(0, UINT_MAX);
    for (std::size_t i = 0; i < 32; ++i) {
        const std::size_t n = std::uniform_int_distribution<std::size_t>(1, 1024)(rng);
        BOOST_TEST_MESSAGE("Iteration " << i << " size: " << n);
        std::vector<unsigned> keys, mapped;
        keys.reserve(n);
        mapped.reserve(n);
        for (std::size_t j = 0; j < n; ++j) {
            keys.push_back(mapped_dist(rng));
            mapped.push_back(mapped_dist(rng));
        }
        segment_tree t(keys, mapped, std::less<unsigned>(), Sum(), Identity());
        auto pairs = views::zip(keys, mapped);
        const auto proj = [](const auto& pair) { return get<0>(pair); };
        ranges::sort(pairs, {}, proj);
        BOOST_CHECK_EQUAL_COLLECTIONS(keys.begin(), keys.end(), t.keys().begin(), t.keys().end());
        BOOST_CHECK_EQUAL_COLLECTIONS(mapped.begin(), mapped.end(), t.nodes().begin(), t.nodes().begin() + t.size());
        const auto test = [&]() {
            std::array itv = {mapped_dist(rng), mapped_dist(rng)};
            ranges::sort(itv);
            BOOST_TEST_CONTEXT("interval [" << itv[0] << ", " << itv[1] << "]") {
                const auto begin = ranges::lower_bound(pairs, itv[0], {}, proj);
                const auto end = ranges::upper_bound(pairs, itv[1], {}, proj);
                unsigned sum = t.identity()();
                for (auto it = begin; it != end; ++it) {
                    sum = t.sum()(sum, get<1>(*it));
                }
                BOOST_CHECK_EQUAL(t.aggregate((itv[0] | closed, itv[1] | closed)), sum);
            }
        };
        for (std::size_t j = 0; j < 128; ++j) BOOST_CALL_TEST_PROC(test);
        std::bernoulli_distribution change_dist(0.125);
        for (auto [idx, ref] : views::enumerate(t)) {
            if (!change_dist(rng)) return;
            const unsigned v = mapped_dist(rng);
            ref = v;
            mapped[idx] = v;
        }
        for (std::size_t j = 0; j < 128; ++j) BOOST_CALL_TEST_PROC(test);
        if constexpr (std::is_same_v<Sum, std::plus<unsigned>>) {
            auto update = t.make_update();
            for (std::size_t j = 0; j < 16; ++j) {
                std::array itv = {mapped_dist(rng), mapped_dist(rng)};
                ranges::sort(itv);
                const unsigned diff = mapped_dist(rng);
                t.make_update(update, (itv[0] | closed, itv[1] | closed), diff);
                const auto begin = ranges::lower_bound(pairs, itv[0], {}, proj);
                const auto end = ranges::upper_bound(pairs, itv[1], {}, proj);
                for (auto it = begin; it != end; ++it) {
                    get<1>(*it) = t.sum()(get<1>(*it), diff);
                }
            }
            for (std::size_t j = 0; j < 128; ++j) BOOST_CALL_TEST_PROC(test);
        }
        BOOST_CHECK_EQUAL(
            t.aggregate((0u | closed, UINT_MAX | closed)),
            std::reduce(mapped.begin(), mapped.end(), t.identity()(), t.sum()));
    }
}