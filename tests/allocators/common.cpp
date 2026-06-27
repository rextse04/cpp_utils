#define BOOST_TEST_MODULE allocators/common
#include "common.hpp"
#include "allocators/allocator_resource.hpp"
#include "allocators/arena_allocator.hpp"
#include "allocators/stack_allocator.hpp"
#include <tuple>
#include <memory_resource>
#include <deque>
#include <ranges>
#include <set>

using namespace utils;
using namespace utils::pmr;
using allocators = std::tuple<
    arena_allocator<std::byte>,
    stack_allocator<std::byte>
>;

BOOST_AUTO_TEST_CASE_TEMPLATE(deque_test, A, allocators) {
    allocator_resource<A> mr(std::pmr::new_delete_resource(), 1 << 16);
    std::pmr::deque<int> d(std::from_range, std::views::iota(0, 1024), &mr);
    BOOST_CHECK_EQUAL(d.size(), 1024);
    for (std::size_t i = 0; i < d.size(); ++i) {
        BOOST_CHECK_EQUAL(d[i], i);
    }
    d.erase(d.begin(), d.begin() + 128);
    BOOST_CHECK_EQUAL(d.size(), 1024 - 128);
    for (std::size_t i = 0; i < d.size(); ++i) {
        BOOST_CHECK_EQUAL(d[i], i + 128);
    }
    d.erase(d.end() - 128, d.end());
    BOOST_CHECK_EQUAL(d.size(), 1024 - 128 * 2);
    for (std::size_t i = 0; i < d.size(); ++i) {
        BOOST_CHECK_EQUAL(d[i], i + 128);
    }
    d.clear();
    BOOST_CHECK_EQUAL(d.size(), 0);
    BOOST_CHECK_THROW(d.resize(1uz << 20), std::bad_alloc);
    BOOST_CHECK_EQUAL(d.size(), 0);
    d.resize(2048);
    BOOST_CHECK_EQUAL(d.size(), 2048);
    for (std::size_t i = 0; i < d.size(); ++i) {
        BOOST_CHECK_EQUAL(d[i], 0);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(set_test, A, allocators) {
    allocator_resource<A> mr(std::pmr::new_delete_resource(), 1 << 16);
    std::pmr::set<int> s(std::from_range, std::views::iota(0, 1024), &mr);
    BOOST_CHECK_EQUAL(s.size(), 1024);
    for (std::size_t i = 0; i < s.size(); ++i) {
        BOOST_CHECK(s.contains(i));
    }
    BOOST_CHECK(!s.contains(1024));
    std::erase_if(s, [](int i) { return i % 2 == 0; });
    BOOST_CHECK_EQUAL(s.size(), 512);
    for (std::size_t i = 0; i < s.size(); ++i) {
        BOOST_CHECK_EQUAL(s.contains(i), i % 2);
    }
    s.clear();
    BOOST_CHECK_EQUAL(s.size(), 0);
    BOOST_CHECK_THROW(s.insert_range(std::views::iota(0, 1 << 16)), std::bad_alloc);
    for (std::size_t i = 0; i < s.size(); ++i) {
        BOOST_CHECK(s.contains(i));
    }
}