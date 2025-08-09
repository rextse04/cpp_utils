#define BOOST_TEST_MODULE functional.hpp
#include "common.hpp"
#include "functional.hpp"
#include <string>
#include <functional>

using namespace utils;

struct test {
    static void f(int) noexcept;
    void g(int, int) const noexcept;
    int h(int) volatile;
};

BOOST_AUTO_TEST_CASE(traits_test) {
#define F(T, U) BOOST_CHECK_EQUAL_TYPES(function_decay_t<T>, U)
#define FL(T, U) F(T, U); BOOST_CHECK_EQUAL_TYPES(lambda_decay_t<T>, U)
    FL(void(int), void(int));
    FL(void(*)(int), void(int));
    FL(void(*)(int) noexcept, void(int));
    FL(void(&)(int), void(int));
    FL(void(&)(int) noexcept, void(int));
    FL(void(&&)(int) noexcept, void(int));
    F(decltype(&test::f), void(int));
    F(decltype(&test::g), void(int, int));
    F(decltype(&test::h), int(int));
    FL(decltype([]{ return 0; }), int(void));
    FL(decltype([] noexcept { return 0; }), int(void));
    FL(decltype([] static { return 0; }), int(void));
    int x = 0;
    F(decltype([&x]{ return x; }), int(void));
    F(decltype([&x] noexcept { return x; }), int(void));
    F(decltype([&x] mutable { return x; }), int(void));
#undef F
#undef FL
}

BOOST_AUTO_TEST_CASE(composition_sanity_test) {
    const auto f = [](auto... args){ return (args + ...); };
    const auto g1 = [](int a, int b) { return a + b; };
    const auto g2 = [](int a) { return a*a; };
    const auto g3 = [](auto... args) { return (args * ...); };

    const auto h1 = composite(f, g1, g2);
    BOOST_CHECK_EQUAL(h1(1, 2, 3), 12);
    const auto h2 = composite(f, bind(5), with(arity<3>{}, g3));
    BOOST_CHECK_EQUAL(h2(1, 2, 3), 11);
}

BOOST_AUTO_TEST_CASE(composition_lifetime_test) {
    const std::string long_str = "This is a really long string to disable small-string optimization for ";
    const auto f = [](const auto&... args){ return (args + ...); };
    std::function<std::string(const std::string&)> h;
    std::string g2 = long_str + "g2. ";
    {
        const auto g1 = [prefix = long_str](const std::string& str) { return prefix + str + ". "; };
        std::string g3 = long_str + "g3.";
        h = composite(f, std::move(g1), bind(g2), bind(std::move(g3)));
        BOOST_CHECK(!g2.empty());
        BOOST_CHECK(g3.empty());
    }
    g2.clear();
    BOOST_CHECK_EQUAL(h("g1"), long_str + "g1. " + long_str + "g2. " + long_str + "g3.");
}