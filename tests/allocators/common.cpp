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
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>
#include <climits>
#include <atomic>
#include "meta.hpp"

using namespace utils;
using namespace utils::pmr;
using unsync_allocators = std::tuple<
    arena_allocator<std::byte>,
    stack_allocator<std::byte>
>;
using sync_allocators = std::tuple<
    arena_allocator<std::byte, true>,
    stack_allocator<std::byte, true>
>;
using allocators = meta::concat_t<unsync_allocators, sync_allocators>;

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

BOOST_AUTO_TEST_CASE_TEMPLATE(sync_test, A, sync_allocators) {
    using AT = std::allocator_traits<A>;
    std::byte buf[1 << 16];
    typename AT::template rebind_alloc<char> alloc(buf, sizeof(buf));
    typename AT::template rebind_alloc<int> int_alloc(alloc);
    const unsigned num_threads = std::jthread::hardware_concurrency();
    std::vector<char*> ptrs(num_threads * CHAR_MAX);
    std::atomic<std::size_t> top = 0;
    {
        std::vector<std::jthread> threads;
        for (unsigned i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                int* const q = int_alloc.allocate(i);
                for (std::size_t j = 0; j < i; ++j) {
                    *q = 0xDEADBEEF;
                }
                for (char j = 0; j < CHAR_MAX; ++j) {
                    char* const p = alloc.allocate(1);
                    ptrs[top.fetch_add(1, std::memory_order::relaxed)] = p;
                    *p = i;
                }
                int_alloc.deallocate(q, i);
            });
        }
    }
    BOOST_CHECK_EQUAL(top.load(std::memory_order::relaxed), num_threads * CHAR_MAX);
    std::vector<std::size_t> counts(num_threads);
    for (std::size_t j = 0; j < num_threads * CHAR_MAX; ++j) {
        ++counts[*ptrs[j]];
    }
    const auto expected = std::views::repeat(CHAR_MAX, num_threads);
    BOOST_CHECK_EQUAL_COLLECTIONS(counts.begin(), counts.end(), expected.begin(), expected.end());
}