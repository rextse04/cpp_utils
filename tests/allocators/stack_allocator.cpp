#define BOOST_TEST_MODULE allocators/stack_allocator.hpp
#include "common.hpp"
#include "allocators/stack_allocator.hpp"
#include <cstddef>
#include <cstdint>

using namespace utils;
using namespace utils::pmr;

alignas(std::max_align_t) std::byte buf[sizeof(detail::stack_allocator_control) * 4 + 32];
stack_allocator<char> alloc(buf, sizeof(buf));
stack_allocator<std::int32_t> i32_alloc(alloc);
stack_allocator<std::int64_t> i64_alloc(i32_alloc);

static void prepare() {
    poison(buf);
    alloc = {buf, sizeof(buf)};
    i32_alloc = alloc;
    i64_alloc = i32_alloc;
}

static void basic_test_full() {
    BOOST_CHECK_THROW(alloc.allocate(1), std::bad_alloc);
    BOOST_CHECK_THROW(i32_alloc.allocate(1), std::bad_alloc);
    BOOST_CHECK_THROW(i64_alloc.allocate(1), std::bad_alloc);
}
BOOST_AUTO_TEST_CASE(basic_test) {
    prepare();
    char* const a = alloc.allocate(2);
    std::int32_t* const b = i32_alloc.allocate(2);
    std::int64_t* const c = i64_alloc.allocate(2);
    BOOST_CALL_TEST_PROC(basic_test_full);
    a[0] = 0; a[1] = 1;
    b[0] = 2; b[1] = 3;
    c[0] = 4; c[1] = 5;
    BOOST_CALL_TEST_PROC(basic_test_full);
    i32_alloc.deallocate(b, 2);
    BOOST_CALL_TEST_PROC(basic_test_full);
    i64_alloc.deallocate(c, 2);
    std::int32_t* const d = i32_alloc.allocate(1);
    char* const e = alloc.allocate(5);
    BOOST_CALL_TEST_PROC(basic_test_full);
    std::ranges::fill_n(d, 1, 6);
    std::ranges::iota(e, e + 5, 0);
    BOOST_CALL_TEST_PROC(basic_test_full);
    alloc.deallocate(e, 5);
    i32_alloc.deallocate(d, 1);
    alloc.deallocate(a, 2);
    const std::size_t fn = sizeof(buf) - sizeof(detail::stack_allocator_control) * 2;
    char* const f = alloc.allocate(fn);
    BOOST_CALL_TEST_PROC(basic_test_full);
    std::ranges::iota(f, f + fn, 0);
    BOOST_CALL_TEST_PROC(basic_test_full);
}