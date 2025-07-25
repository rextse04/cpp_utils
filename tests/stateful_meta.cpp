#define BOOST_TEST_MODULE stateful_meta.hpp
#include <stateful_meta.hpp>
#include <iostream>

using namespace utils::meta;

int main() {
    struct test_var;
    static_assert(!exists(const_var<test_var>{}));
    define<test_var, 0>{};
    static_assert(get(const_var<test_var>{}) == 0);
    static_assert(exists(const_var<test_var>{}));

    static_assert(get_counter<void, true>() == 0);
    static_assert(get_counter<void, true>() == 1);

    set<test_var, 0>{};
    static_assert(get(var<test_var>{}) == 0);
    set<test_var, 1>{};
    static_assert(get(var<test_var>{}) == 1);
}