/* This file is used for testing assembly generated for utils::integer. */

#include <integer.hpp>

using namespace utils::integer_alias;
namespace ib = utils::integral_behavior;
namespace sb = utils::shift_behavior;

int f1(int x) {
    return x & (~x+32);
}

using s = sint::adopt<ib::wrap>;
s f2(s x) {
    return x & (~x+32);
}