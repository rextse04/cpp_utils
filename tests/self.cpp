#include "self.hpp"

using namespace utils;

namespace random {
    struct test { UTILS_FIND_SELF };
}

static_assert(std::is_same_v<UTILS_FIND_SELF_TYPE, random::test>);