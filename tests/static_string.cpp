#include <iostream>
#include <print>
#include "static_string.hpp"

using namespace utils;
using namespace utils::static_string_literals;

template <basic_static_string Str>
struct F {
    static constexpr auto str = []() {
        auto out = Str;
        out[0] = 'A';
        out.resize(5);
        out.append(" append");
        out.insert(5, " insert");
        out.erase(3, 2);
        out.replace(3, 1, " replace ");
        return out;
    }();
};

using test = F<"abcdefghijklmnopqrstuvwxyz">;
static_assert(test::str == "Abc replace insert append");
static_assert(test::str < "Abcd");

int main() {
    std::cout << test::str << '\n';
    std::println("{}", test::str);
}